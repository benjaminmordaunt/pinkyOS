// BSP support routine
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "mmu.h"

extern void* end;

struct cpu	cpus[NCPU];
struct cpu	*cpu;

#define MB (1024*1024)

void kmain (void)
{
    uint64 mpidr;
    int cpu_id;
     
    // If we're on a uniprocessor system, just use &cpus[0].
    // Otherwise, generating a linear sequential CPU ID based on MPIDR_EL1.{Aff3..Aff0}
    // is rather challenging. For now, make the (awful) assumption that we can just use Aff0
    // for CPU indexing. This will have an awful effect if there is ever a collision.

    asm("MRS %[r], MPIDR_EL1": [r]"=r" (mpidr): :);
    if (mpidr & MPIDR_EL1_U)
        cpu = &cpus[0];
    else {
        _puts("FIXME: Using only MPIDR_EL1.Aff0 for PE ID\n");
        cpu_id = (mpidr >> MPIDR_EL1_AFF0) & 0xFF;
        if (cpu_id > NCPU) {
	    _puts("CPU id out of bounds\n");
            while (1) ;            
        }
        _putint("[", cpu_id, "] kmain\n");
        cpu = &cpus[cpu_id];
    }

    uart_init (P2V(UART0));
    _puts("kmain: uart_init complete\n");

    init_vmm ();
    kpt_freerange (align_up(&end, PT_SZ), P2V_WO(INIT_KERNMAP));
    paging_init (INIT_KERNMAP, PHYSTOP);
    _puts("kmain: paging_init complete\n");

    kmem_init ();
    kmem_init2(P2V(INIT_KERNMAP), P2V(PHYSTOP));
    _puts("kmain: kmem_init complete\n");

    trap_init ();				// vector table and stacks for models
     
    gic_init(P2V(VIC_BASE));			// arm v2 gic init
    uart_enable_rx ();				// interrupt for uart
    consoleinit ();				// console
    pinit ();					// process (locks)

    binit ();					// buffer cache
    fileinit ();				// file table
    iinit ();					// inode cache
    ideinit ();					// ide (memory block device)

#ifdef INCLUDE_REMOVED
    timer_init (HZ);				// the timer (ticker)
#endif

    sti ();
    userinit();					// first user process
    
    _puts("kmain: entering scheduler\n");
    scheduler();				// start running processes
}
