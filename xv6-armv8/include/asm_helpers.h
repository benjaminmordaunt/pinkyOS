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

/* For now, only support strong ELF linkage. Weak symbols at this
 * level are pretty useless nowadays and some dynamic linkers still
 * work off of link order instead of symbol strength. If there is
 * a good argument for weak symbols, add later.
 */

#define ASM_SYM_L_GLOBAL(name)                  \
	.globl name

#define ASM_SYM_L_LOCAL(name)                   /* nothing */

#define ASM_SYM_START(name, linkage)            \
	linkage(name) ;                         \
	name:                                   

#define ASM_FUNC_START(name)                    \
	ASM_SYM_START(name, ASM_SYM_L_GLOBAL)

/* ASM_FUNC_END is mostly benign, except it marks the symbol as a
 * "function" instead of an "object". What this actually means for
 * any of the tooling isn't completely clear. Also log the size
 * in a temporary local (.L) so it is discarded appropriately e.g.
 * when using ld -X, but is available for tools that need it.
 */

#define ASM_FUNC_END(name)                     \
	.type name function ;                  \
	.set .L__sym_size_##name, .-name ;     \
	.size name, .L__sym_size_##name

