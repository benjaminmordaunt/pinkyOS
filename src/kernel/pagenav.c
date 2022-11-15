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

#include "sysreg.h"
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

/* Walk the page table at `base`. Do not apply attributes to the entry, aside from
   those necessary for navigation. Use pgtbl_map_pages to actually perform a "proper"
   mapping. */
int pgtbl_walk(pgtbl_desc_t *base, va_t va, pgtbl_desc_t **entry_out, int alloc) {
    pgtbl_desc_t *next_desc = base;
    pgtbl_desc_t *alloc_target;
    int i = 0;
    unsigned long long int mask;

    while ((mask = PT_LEVEL_MASK[i++]))
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

            memset(alloc_target, 0, PAGE_SIZE);

            /* Note that ENTRY_TABLE is being set for tables and page descriptors alike - they present the same in memory. */
            *next_desc = kern_v2p(alloc_target) | ENTRY_VALID | ENTRY_TABLE;

            next_desc = alloc_target;

        } else {
            /* Extract the next level from the entry - the entries are physical addresses */
            next_desc = kern_p2v(*next_desc & mask);
        }
    }

    *entry_out = next_desc;
    return 0;
}

/* Maps a physically contiguous block of memory from `phys_start`, over a length of end - start
   (end is exclusive). All addresses should be page-aligned. */
int pgtbl_map_pages(pgtbl_desc_t *base, va_t start, va_t end, pa_t phys_start, pgtbl_attrs_t attrs, int mode) {
    va_t vpage_base = start;
    pa_t ppage_base = phys_start;
    pgtbl_desc_t *pgtbl_entry;
    unsigned long long int mode_attr;

    KASSERT(mode >= 0 && mode < MEMATTR_MAX_IDX, 
        ("pgtbl_map_pages: memory mode %d out of range", mode));
    KASSERT(start == ALIGN_DOWN(start, _PT_PS),
        ("pgtbl_map_pages: start va not page aligned: 0x%llx", start));
    KASSERT(end == ALIGN_DOWN(end, _PT_PS),
        ("pgtbl_map_pages: end va not page aligned: 0x%llx", end));
    KASSERT(phys_start == ALIGN_DOWN(phys_start, _PT_PS),
        ("pgtbl_map_pages: phys_start pa not page aligned: 0x%llx", phys_start));

    while (vpage_base + PAGE_SIZE < end) {
        if (pgtbl_walk(base, vpage_base, &pgtbl_entry, 1) < 0)
            return -1;

        if (*pgtbl_entry & ENTRY_VALID)
            return -1;

        mode_attr = (mode & 7) << PT_DESC_STAGE1_LOWER_MEMATTR_SHIFT;

        /* XXX: Should mask attrs application to ensure it doesn't write over address bits. */
        *pgtbl_entry = ppage_base | attrs | ENTRY_VALID | ENTRY_TABLE | mode_attr;

        vpage_base += PAGE_SIZE;
        ppage_base += PAGE_SIZE;
    }

    return 0;
}