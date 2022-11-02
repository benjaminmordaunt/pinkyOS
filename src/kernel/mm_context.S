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

// Perform callee's responsibilities for register saving
// and stack maintenance.

// Unlike contemporary kernels, a shadow stack is _not_ being
// used as a protection mechanism against stack underruns and
// return address editing. If folks want to implement this
// later as part of a kernel hardening exercise, feel free.

#include "asm_helpers.h"


ASM_FUNC_START(context_switch)
	// Store callee-saved registers (x19-x28 + fp/x29 + sp)
	// ... then store the link register to determine entry point
	//     of this context.

	mov     x16, sp
	stp	x19, x20, [sp], #16
	stp	x21, x22, [sp], #16
	stp	x23, x24, [sp], #16
	stp	x25, x26, [sp], #16
	stp	x27, x28, [sp], #16
	stp	x29, x16, [sp]
	str	lr, [sp]	

	mov	sp, x1

	// Restore callee-saved registers
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
	ldp	x29, x16, [sp]
	ldr	lr, [sp]

	mov	sp, x16	
	ret
ASM_FUNC_END(context_switch)
