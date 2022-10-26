#include "asm_helpers.h"

#define MAIR_ATTR_IDX(idx, val)                \
	val << (idx << 3)

#define AA64_CPAPR_FPEN_SHIFT	20

/* aarch64 normal memory attributes:
 *  - WB => write-back
 *  - WT => write-through
 *  - NC => non-cacheable
 *  - NT => non-transient
 *  - T  => transient (unused)
 *
 * for the curious - the transient
 * bit is supposed to indicate that
 * "the benefit of caching is for a
 * relatively short period", an
 * apparently useless notion that most
 * implementations ignore.
 *
 * attributes always applied across
 * both inner and outer shareable
 * domains (i.e. 'oooo' == 'iiii').
 */

#define AA64_MEM_NORMAL_WBNT	0xFF
#define AA64_MEM_NORMAL_NC      0x44

/* aarch64 device memory attributes:
 *  - G => gathering (merged transactions)
 *  - R => reordering
 *  - E => early-write ack
 */

#define AA64_MEM_DEVICE_GRE     0x0C
#define AA64_MEM_DEVICE_nGnRE   0x04
#define AA64_MEM_DEVICE_nGnRnE  0x00

#define MAIR_INIT                                  \
	MAIR_ATTR_IDX(0, AA64_MEM_DEVICE_nGnRnE) | \
	MAIR_ATTR_IDX(1, AA64_MEM_DEVICE_nGnRE)  | \
	MAIR_ATTR_IDX(2, AA64_MEM_DEVICE_GRE)    | \
	MAIR_ATTR_IDX(3, AA64_MEM_NORMAL_NC)     | \
	MAIR_ATTR_IDX(4, AA64_MEM_NORMAL_WBNT)

ASM_FUNC_START(__arm64_cpu_start)
	vectors_ptr  .req x0

	ic	ialluis
	tlbi	vmalle1
	dsb	nsh

	/* don't trap FP/SIMD instructions at EL0/EL1 */
	mov	x1, #3 << AA64_CPAPR_FPEN_SHIFT
	msr	cpapr_el1, x1

	msr	mdscr_el1, xzr
	mov     x1, #MAIR_INIT
	msr     mair_el1, x1
	msr     vbar_el1, vectors_ptr

	// TODO: tcr_el1
	//       ttbr_el1
	//       ttbr0_el1
	//       sctlr_el1

	.unreq vectors_ptr
ASM_FUNC_END(__arm64_cpu_start)
