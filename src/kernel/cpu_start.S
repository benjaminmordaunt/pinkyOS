// Copyright 2022 Benjamin John Mordaunt
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "asm_helpers.h"
#include "sysreg.h"

#define MAIR_INIT                                                       \
	(AA64_MAIR_ATTR_IDX(MEMATTR_DEVICE, AA64_MEM_DEVICE_nGnRnE)         | \
	 AA64_MAIR_ATTR_IDX(MEMATTR_DEVICE_RELAXED, AA64_MEM_DEVICE_nGnRE)  | \
	 AA64_MAIR_ATTR_IDX(MEMATTR_NORMAL_UNCACHEABLE, AA64_MEM_NORMAL_NC) | \
	 AA64_MAIR_ATTR_IDX(MEMATTR_NORMAL_WRITEBACK, AA64_MEM_NORMAL_WBNT))

/* set up a sensible translation scheme for now */
#define TCR_INIT                                          \
	(FIELD_ALIGN(32, AA64_TCR_T0SZ_SHIFT)                 | \
	FIELD_ALIGN(1, AA64_TCR_ORGN0_SHIFT)                  | \
	FIELD_ALIGN(1, AA64_TCR_IRGN0_SHIFT)                  | \
	FIELD_ALIGN(3, AA64_TCR_SH0_SHIFT)                    | \
	FIELD_ALIGN(32, AA64_TCR_T1SZ_SHIFT)                  | \
	FIELD_ALIGN(1, AA64_TCR_ORGN1_SHIFT)                  | \
	FIELD_ALIGN(1, AA64_TCR_IRGN1_SHIFT)                  | \
	FIELD_ALIGN(3, AA64_TCR_SH1_SHIFT)                    | \
	FIELD_ALIGN(2, AA64_TCR_TG1_SHIFT)                    | \
	FIELD_ALIGN(4, AA64_TCR_IPS_SHIFT)                    | \
	FIELD_ALIGN(1, AA64_TCR_AS_SHIFT)                     | \
	FIELD_ALIGN(1, AA64_TCR_TBI0_SHIFT))

ASM_FUNC_START(__arm64_cpu_start)
	vectors_ptr	.req	x0
	kernel_ttb      .req    x1
	user_ttb        .req    x2

	ic		ialluis
	tlbi		vmalle1
	dsb		nsh

	/* don't trap FP/SIMD instructions at EL0/EL1 */
	mov		x3, #3 << AA64_CPACR_FPEN_SHIFT
	msr		cpacr_el1, x3

	msr		mdscr_el1, xzr
	ldr		x3, =MAIR_INIT
	msr		mair_el1, x3
	msr		vbar_el1, vectors_ptr

	ldr		x3, =TCR_INIT
	msr		tcr_el1, x3

	/* initialize ttbr0 and ttbr1 page table base registers */
	msr 		ttbr0_el1, user_ttb
	msr		ttbr1_el1, kernel_ttb
	
	/* enable the mmu for el0/1 translations */
	mrs		x3, sctlr_el1
	orr		x3, x3, #0x1	
	msr		sctlr_el1, x3

	.unreq 		vectors_ptr
	.unreq		kernel_ttb
	.unreq		user_ttb
ASM_FUNC_END(__arm64_cpu_start)
