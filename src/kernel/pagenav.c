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

#include "page.h"

void init_pgtbl_constants(void) {
    int i;

    for (i = _PT_N_MAX; i > -1; i--) {
        PT_LEVEL_ADDR_SHIFT[i] = _PT_PS + i * (_PT_PS - 3);
        PT_LEVEL_PTRS[i]       = 1 << (_PT_PS - 3);
        PT_LEVEL_MASK[i]       = ((va_t)PT_LEVEL_PTRS[i] - 1) << PT_LEVEL_ADDR_SHIFT[i];
    }

    for (i = _PT_N_MAX; i < 5; i++) {
        PT_LEVEL_ADDR_SHIFT[i] = 0;
        PT_LEVEL_PTRS[i]       = 0;
        PT_LEVEL_MASK[i]       = 0;
    }
}

inline unsigned int pgtbl_level_idx(va_t va, int lvl) {
    return ((va & PT_LEVEL_MASK[lvl]) >> PT_LEVEL_ADDR_SHIFT[lvl]);
}

int pgtbl_walk(pgtbl_desc_t *base, va_t va, pgtbl_desc_t **entry_out, int alloc) {
    pgtbl_desc_t *next_desc = base;
    pgtbl_desc_t *alloc_target;
    int i = 0;
    unsigned int addr_shift, ptrs;
    unsigned long long int mask;

    while ((addr_shift = PT_LEVEL_ADDR_SHIFT[i++]) && 
           (ptrs = PT_LEVEL_PTRS[i++]) &&
           (mask = PT_LEVEL_MASK[i++]))
    {
        /* Move to the correct descriptor */
        next_desc = next_desc[pgtbl_level_idx(va, i)];

        /* Fail on block descriptors for now */
        if (*next_desc & (ENTRY_VALID | ENTRY_TABLE) == ENTRY_VALID) {
            kern_print_warn("Block descriptors not supported.");
            return -1;
        }

        /* Check that the descriptor is valid */
        if (!(*next_desc & ENTRY_VALID)) {
            if (!alloc)
                return -1;

            /* Should probably treat this as a more serious failure. */
            if ((alloc_target = page_alloc()) < 0)
                return -1;

            memset(alloc_target, 0, 1 << _PT_PS);

            /* Note that ENTRY_TABLE is being set for tables and page descriptors alike - they present the same in memory. */
            *next_desc = kern_v2p(alloc_target) | ENTRY_VALID | ENTRY_TABLE;

            next_desc = alloc_target;

        } else {
            /* Extract the next level from the entry - the entries are physical addresses */
            next_desc = kern_p2v(next_desc & mask);
        }
    }

    *entry_out = next_desc;
    return 0;
}