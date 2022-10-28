// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
    uint	magic;  // must equal ELF_MAGIC
    uchar	elf[12];
    ushort	type;
    ushort	machine;
    uint	version;
    uint64	entry;
    uint64	phoff;
    uint64	shoff;
    uint	flags;
    ushort	ehsize;
    ushort	phentsize;
    ushort	phnum;
    ushort	shentsize;
    ushort	shnum;
    ushort	shstrndx;
};

// Program header
struct proghdr {
    uint	type;
    uint	flags;
    uint64	off;
    uint64	vaddr;
    uint64	paddr;
    uint64	filesz;
    uint64	memsz;
    uint64	align;
};

// Section header
struct secthdr {
    uint	name;
    uint	type;
    uint64	flags;
    uint64	addr;
    uint64	off;
    uint64	sz;
    uint	link;
    uint	info;
    uint64	addralign;
    uint64	entsz;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
