// Memory layout

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNBASE  		0xFFFFFFFF00000000	
// First kernel virtual ram address 
// V:0xFFFF_FFFF_4000_0000 ==> P:0x40000000  (PHY_START)

// we first map 2MB low memory containing kernel code.
#define INIT_KERN_SZ	0x200000
#define INIT_KERNMAP 	(INIT_KERN_SZ + PHY_START)

// VirtIO MMIO base
// Each MMIO transport has size 0x200
#define VIRT_MMIO_BASE      0x0a000000

#define NUM_VIRTIO_TRANSPORTS 32

// From a15irqmap
#define VIRT_MMIO_IRQ_START 16 
#define VIRT_MMIO_IRQ_END (VIRT_MMIO_IRQ_START + NUM_VIRTIO_TRANSPORTS - 1) /* inclusive */

#ifndef __ASSEMBLER__

static inline uint64 v2p(void *a) { return ((uint64) (a))  - (uint64)KERNBASE; }
static inline void *p2v(uint64 a) { return (void *) ((a) + (uint64)KERNBASE); }

#endif

#define V2P(a) (((uint64) (a)) - (uint64)KERNBASE)
#define P2V(a) (((void *) (a)) + (uint64)KERNBASE)

#define V2P_WO(x) ((x) - (uint64)KERNBASE)    // same as V2P, but without casts
#define P2V_WO(x) ((x) + (uint64)KERNBASE)    // same as V2P, but without casts
