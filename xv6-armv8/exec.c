#include "types.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"
#include "arm.h"

extern uint64 llvaddr;

// load a user program for execution
int exec (char *path, char **argv)
{
    struct elfhdr elf;
    struct inode *ip;
    struct proghdr ph;
    pgd_t *pgdir;
    pgd_t *oldpgdir;
    char *s;
    char *last;
    int i;
    int off;
    uint argc;
    uint64 sz;
    uint64 sp;
    uint64 ustack[3 + MAXARG + 1];

    if ((ip = namei(path)) == 0) {
        return -1;
    }

    ilock(ip);
    pgdir = 0;

    // Check ELF header
    if (readi(ip, (char*) &elf, 0, sizeof(elf)) < sizeof(elf)) {
        goto bad;
    }

    if (elf.magic != ELF_MAGIC) {
        goto bad;
    }

    if ((pgdir = kpt_alloc()) == 0) {
        goto bad;
    }

    // Load program into memory.
    sz = 0;
    
    for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)) {
        if (readi(ip, (char*) &ph, off, sizeof(ph)) != sizeof(ph)) {
            goto bad;
        }

        if (ph.type != ELF_PROG_LOAD) {
            continue;
        }

        if (ph.memsz < ph.filesz) {
            goto bad;
        }

        if ((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0) {
            goto bad;
        }

        if (loaduvm(pgdir, (char*) ph.vaddr, ip, ph.off, ph.filesz) < 0) {
            goto bad;
        }
    }

#ifdef CONFIG_DEBUG
    struct secthdr sh;
    uint strndx;
    uint shmax = elf.shnum + 1;
    char chreadblk[256], chridx = 0;
    char *chwp = &chreadblk[0];

    // Check where .text was loaded so that we know where to point
    // our debugger for a given image. If this check fails for whatever
    // reason, just continue.

    for (i = 0, off = elf.shoff; i < elf.shnum; i++, off += sizeof(sh)) {
        if (readi(ip, (char*) &sh, off, sizeof(sh)) != sizeof(sh)) {
	       continue;
	}
        
	if (i == elf.shstrndx) {
            strndx = 0;

	    // TODO: Implement efficient single-byte reads for strings. 
	    while (shmax--) {
                while (readi(ip, chwp, sh.off+(chridx++), 1) == 1) {
		        if (*chwp++ == '\0' || (chwp - &chreadblk[0] >= sizeof(chreadblk))) break;
		}

	    	if (!strcmp(chreadblk, ".text")) {
		    break;
	    	}

	        strndx++;
		chwp = &chreadblk[0];
	    }

	    shmax = elf.shnum + 1;
	}
    }

    for (i = 0, off = elf.shoff; i < elf.shnum; i++, off += sizeof(sh)) {
	if (readi(ip, (char*) &sh, off, sizeof(sh)) != sizeof(sh)) {
		continue;
	}

	if (sh.name == strndx) {
	       cprintf("exec: loaded %s @ 0x%p [|.text| %db]\n", path, llvaddr, sh.sz);
	       break;
	}
    }

#endif /* CONFIG_DEBUG */

    iunlockput(ip);
    ip = 0;

    // Allocate two pages at the next page boundary.
    // Make the first inaccessible.  Use the second as the user stack.
    sz = align_up (sz, PTE_SZ);

    if ((sz = allocuvm(pgdir, sz, sz + 2 * PTE_SZ)) == 0) {
        goto bad;
    }

    clearpteu(pgdir, (char*) (sz - 2 * PTE_SZ));

    sp = sz;

    // Push argument strings, prepare rest of stack in ustack.
    for (argc = 0; argv[argc]; argc++) {
        if (argc >= MAXARG) {
            goto bad;
        }

        sp = (sp - (strlen(argv[argc]) + 1)) & ~7;

        if (copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0) {
            goto bad;
        }

        ustack[argc] = sp;
    }

    ustack[argc] = 0;

    // in ARM, parameters are passed in r0 and r1
    proc->tf->r0 = argc;
    proc->tf->r1 = sp - (argc + 1) * 8;

    sp -= (argc + 1) * 8;

    if (copyout(pgdir, sp, ustack, (argc + 1) * 8) < 0) {
        goto bad;
    }

    // Save program name for debugging.
    for (last = s = path; *s; s++) {
        if (*s == '/') {
            last = s + 1;
        }
    }

    safestrcpy(proc->name, last, sizeof(proc->name));

    // Commit to the user image.
    oldpgdir = proc->pgdir;
    proc->pgdir = pgdir;
    proc->sz = sz;
    proc->tf->elr = elf.entry;
    proc->tf->sp = sp;

    switchuvm(proc);
    freevm(oldpgdir);
    return 0;

    bad: if (pgdir) {
        freevm(pgdir);
    }

    if (ip) {
        iunlockput(ip);
    }
    return -1;
}
