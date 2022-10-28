// Author: Benjamin Mordaunt <crawford.benjamin15@gmail.com>

// Defines based on QEMU's virtio_blk.h and standard-headers/linux/virtio_config.h, exposing the Virt I/O 1.1 standard here: https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.pdf

// The legacy MMIO interface will be used to connect to QEMU virtio blk device.

// 2.1 Device Status Field
// Used for state-machine style negotiation between driver and device
#define VIRTIO_CONFIG_S_ACKNOWLEDGE  1
#define VIRTIO_CONFIG_S_DRIVER       2
#define VIRTIO_CONFIG_S_DRIVER_OK    4
#define VIRTIO_CONFIG_S_FEATURES_OK  8
#define VIRTIO_CONFIG_S_FAILED       128

// Number of available VirtIO descriptors
#define NUM_DESC 8

// 4.2.4 MMIO Device Legacy Register Layout
#define VIRTIO_MMIO_MAGICVALUE       0x000
#define VIRTIO_MMIO_VERSION          0x004 /* 1 for legacy */
#define VIRTIO_MMIO_DEVICEID         0x008 /* 2 for block */
#define VIRTIO_MMIO_VENDORID         0x00C
#define VIRTIO_MMIO_HOSTFEATURES     0x010
#define VIRTIO_MMIO_HOSTFEATURESEL   0x014
#define VIRTIO_MMIO_GUESTFEATURES    0x020
#define VIRTIO_MMIO_GUESTFEATURESSEL 0x024
#define VIRTIO_MMIO_GUESTPAGESIZE    0x028
#define VIRTIO_MMIO_QUEUESEL         0x030
#define VIRTIO_MMIO_QUEUENUMMAX      0x034
#define VIRTIO_MMIO_QUEUENUM         0x038
#define VIRTIO_MMIO_QUEUEALIGN       0x03C
#define VIRTIO_MMIO_QUEUEPFN         0x040
#define VIRTIO_MMIO_QUEUENOTIFY      0x050
#define VIRTIO_MMIO_INTERRUPTSTATUS  0x060
#define VIRTIO_MMIO_INTERRUPTACK     0x064
#define VIRTIO_MMIO_STATUS           0x070
#define VIRTIO_MMIO_CONFIG           0x100 /* base address */

// Virtqueue Descriptors
struct virtq_desc {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next; /* only valid if flags & NEXT */
};

// Virtqueue Available Ring (used by driver to offer available buffers to device)
struct virtq_avail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[NUM_DESC];
	uint16_t used_event; /* unused */
};

// Virtqueue Used Ring - entries are pairs descriing head entry of descriptor chain describing the buffer, len is the number of bytes populated.
struct virtq_used_elem {
	uint32_t id;
	uint32_t len;
};

struct virtq_used {
	uint16_t flags;
	uint16_t idx;
	struct virtq_used_elem ring[NUM_DESC];
	uint16_t unused;
};	

// Block device operations
struct virtio_blk_req {
	uint32_t type;
	uint32_t reserved;	
	uint64_t sector;
};

// This request should be followed up by a buffer to receive sector data (u8 data[512]) and a one-byte status

#define VIRTIO_BLK_T_IN    0
#define VIRTIO_BLK_T_OUT   1
}

