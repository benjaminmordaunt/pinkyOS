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

/* A simple implementation of a uni-processor (no notion of per-CPU pagesets) 
   buddy allocator to kickstart development of pinkyOS.
 */

#include "kmem.h"
#include <types.h>
#include <inttypes.h>
#include "page.h"

/* XXX: Use a routine to determine maximum required order based on physical memory
        extent. For now, the max order will be hard-coded to 20, which allows for management 
        of up to a single 4GiB contiguous space.
*/

#define PM_BUDDY_MAX_ORDER                             10

/* Represents a contiguous span of physical memory. Exclusive of end. */
struct pm_extent {
    pa_t start;
    pa_t end;
    list_head_t lentry;
};

/* Represents a collection of pm_extent. Typically used
   for expressing keep-out zones (memory holes) described
   by the bootloader. */
/* XXX: Standard linked list API for pinkyOS. */

/* A single order manages memory of granularity 2^ord, where ord
   is the index of the order in the parent pm_physmap.orders array. 
   alloc_head references a null-pointer, or a pointer to the first 
   pm_buddy_block structure if at least one (two with its buddy) is
   present in the buddy system. */

struct pm_physmap {
    list_head_t           *pgdat_start;
    void                  *heap_start;
    list_head_t           *orders[PM_BUDDY_MAX_ORDER]; /* ptrs into pgdat region */
    int                   maxord;
    int                   poff;     /* A global page offset for all resolutions from orders */
} pm_physmap_up;

#define pm_order_offset(ord, maxord) \
            (1 << ((maxord - ord)))

int pm_block_offset(struct pm_physmap *pmap, pa_t addr, int ord) {
    pa_t raddr = addr - (pa_t)pmap->heap_start - (1 << (pmap->poff + _PT_PS));
    int idx;

    /* If the block cannot be expressed at this granularity, return -1 */
    if (raddr % (1 << (_PT_PS + ord)))
        return -1;

    idx = raddr / (1 << (_PT_PS + ord));
    return idx;
}

/* Splits a physical block into a child and its buddy. Block needs to be valid */
int _pm_block_split(struct pm_physmap *pmap, int block_offset, int order) {
    list_head_t *child, *buddy, *parent;
    char *pagemapent;

    /* XXX: Missing all sorts at the moment, is this block mapped? 
            Propagating of flags, permissions etc. */

    child = &pmap->pgdat_start[pm_order_offset(order - 1, pmap->maxord) + (block_offset << 1)];
    buddy = child + 1;

    list_head_init(child);
    child->next = buddy;
    list_head_init(buddy);
    buddy->prev = child;

    if (list_entry_valid(&pmap->orders[order - 1])) {
        buddy->next = &pmap->orders[order - 1];
        pmap->orders[order - 1]->prev = buddy;
    }

    pmap->orders[order - 1] = child;

    parent = &pmap->pgdat_start[pm_order_offset(order, pmap->maxord) + block_offset];
    if (parent->prev == LIST_HEAD_TERM) {
        if (parent->next && parent->next != LIST_TAIL_TERM) {
            parent->next->prev = LIST_HEAD_TERM;
        }
    } else if (parent->next == LIST_TAIL_TERM) {
        if (parent->prev && parent->prev != LIST_HEAD_TERM) {
            parent->prev->next = LIST_TAIL_TERM;
        }
    } else if (list_entry_valid(parent)) {
        parent->prev->next = parent->next;
        parent->next->prev = parent->prev;
    } else {
        panic("_pm_block_split: parent block was invalid");
    }

    /* Explicitly invalidate the block. Even though it is no longer in a list,
       it may still be explicitly referred to by offset. */
    parent->next = 0;
    parent->prev = 0;

    return 0;
}

/* Ensures a certain order of block granularity exists within the map for addr.
   addr is page aligned. addr needs to be in the max-order-offset space */
int pm_physmap_assert_split(struct pm_physmap *pmap, void *addr, int order) {
    int block_offset, iord;
    list_head_t *block;
    pa_t tgt_block_base;

    /* If the block already exists at a finer granularity, do nothing. */    
    /* Find the higher order block and request splits */
    for (iord = 0; iord <= pmap->maxord; iord++) {
        tgt_block_base = ALIGN_DOWN((pa_t)(addr), _PT_PS + iord);
        block_offset = pm_block_offset(pmap, tgt_block_base, iord);
        block = &pmap->pgdat_start[pm_order_offset(iord, pmap->maxord) + block_offset];

        if (list_entry_valid(block)) {
            if (iord > order) {
                _pm_block_split(pmap, block_offset, iord);
                iord -= 2;
                continue;
            }
            return 1;
        }
    }

    return 0;
}

int pm_physmap_mark_region(struct pm_physmap *pmap, void *start, void *end) {
    int start_min_ord, end_min_ord;
    pa_t start_align_goal;
    int iord;

    /* Ensure we have sufficient granularity of control over ALIGN_DOWN(start, _PT_PS) */
    start_min_ord = 0;
    start_align_goal = ALIGN_DOWN((pa_t)start, _PT_PS);

    /* Aritmetic coming up which operates on pgdat, so need to offset */
    start_align_goal -= (pmap->poff << _PT_PS);

    for (iord = pmap->maxord; iord; iord--) {
        /* Get the highest order that also aligns with this page to
           minimise splitting. */

        if (start_align_goal == ALIGN_DOWN((pa_t)start, _PT_PS + iord)) {
            start_min_ord = iord;
            break;
        }
    }
    pm_physmap_assert_split(pmap, start_align_goal, start_min_ord);
}

int pm_physmap_init(struct pm_extent *physmem_ext, struct pm_extent *keepout_head) {
    pa_t physmem_sz = physmem_ext->end - physmem_ext->start;
    pa_t ordsz;
    struct pm_extent *keepout = keepout_head;
    struct pm_physmap *pmap = &pm_physmap_up;
    list_head_t *ord;
    int ordidx, maxord, i, rc, pgdat_entries;

    if (physmem_ext->start != ALIGN_DOWN(physmem_ext->start, _PT_PS)
    || (physmem_ext->end   != ALIGN_DOWN(physmem_ext->end,   _PT_PS))) {
        panic("pm_physmap_init: addressable extent vas must be at least page aligned");
    }

    if (physmem_sz & ((1 << (PM_BUDDY_MAX_ORDER + _PT_PS)) - 1) != 0)
        panic("pm_physmap_init: addressable range cannot be covered by buddy allocator");

    maxord = ffs(physmem_sz) + 1 - _PT_PS;
    ordsz = (1 << (_PT_PS + maxord));

    pgdat_entries = (1 << (maxord + 1));

    pmap->pgdat_start = (void*)physmem_ext->start;
    pmap->heap_start = ALIGN_UP((pa_t)pmap->pgdat_start + pgdat_entries * sizeof(list_head_t), _PT_PS);
    
    /* How many order-0 pages are we off from being order-maxord aligned? 
       This will affect all block offset calculations in pm_physmap. */
    pmap->poff = ((pa_t)pmap->heap_start - ALIGN_DOWN((pa_t)pmap->heap_start, maxord)) >> _PT_PS;

    for (ord = pmap->pgdat_start, ordidx = 0; ordidx < maxord; ordidx++) {
        list_head_init_invalid(ord);
        pmap->orders[ordidx] = ord;
        ord += (1 << (maxord - ordidx));
    }
    list_head_init(ord);
    pmap->orders[ordidx] = ord;

    /* Revise available size after bookkeeping */
    physmem_sz = physmem_ext->end - (pa_t)pmap->heap_start;

    /* Already guaranteed size will be a PAGE_SIZE multiple */
    pm_physmap_mark_region(pmap, pmap->heap_start + physmem_sz, pmap->heap_start + ordsz);

    for (; keepout; keepout = keepout->lentry.next) {
        if (keepout->start >= keepout->end)
            panic("pm_physmap_init: malformed or zero-size keepout entry");

        /* XXX: Check whether hole is over pgdat area - panic for now */
        if ((keepout->start >= pmap->pgdat_start && keepout->start < pmap->pgdat_start + 
                ((pa_t)pmap->heap_start - (pa_t)pmap->pgdat_start))
         || (keepout->end > pmap->pgdat_start && keepout->end <= pmap->pgdat_start + 
                ((pa_t)pmap->heap_start - (pa_t)pmap->pgdat_start)))
            panic("pm_physmap_init: keepout entry will cause buddy bookkeeping corruption");

        /* Check whether hole is completely irrelevant */
        if ((keepout->start < pmap->pgdat_start && keepout->end <= pmap->pgdat_start)
         || (keepout->start >= pmap->heap_start + ordsz))
            continue;

        pm_physmap_mark_region(pmap, keepout->start, keepout->end);
    }

    return rc;
}

// ----- TO BE MOVED BEGIN -----

int strcmp(const char *p, const char *q)
{
    register const unsigned char *s1 = (const unsigned char *) p;
    register const unsigned char *s2 = (const unsigned char *) q;
    unsigned char c1, c2;

    do {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if (c1 == '\0')
            goto str_term;
    } while (c1 == c2);

str_term:
    return c1 - c2;    
}

// ----- TO BE MOVED END -----
