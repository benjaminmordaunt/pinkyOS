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

#include "bitarith.h"
#include "kmem.h"
#include "types.h"
#include <inttypes.h>
#include "page.h"

/* XXX: Use a routine to determine maximum required order based on physical memory
        extent. For now, the max order will be hard-coded to 20, which allows for management 
        of up to a single 4GiB contiguous space.
*/

#define VM_MAX_ORDER                             10

/* Represents a contiguous span of physical memory. Exclusive of end. */
struct pm_extent {
    uintptr_t start;
    uintptr_t end;
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

struct vm_page_struct {
    list_head_t fle;     /* freelist entry - must be first in struct */
    uint8_t     order;   /* order of this block */
    uint8_t     flags;   /* page flags as applied by the vm system - see vm_page.h */
} __packed;

struct pm_physmap {
    struct vm_page_struct *pgdat_start;
    void                  *heap_start;
    struct vm_page_struct *orders[VM_MAX_ORDER + 1]; /* ptrs into pgdat region */
    int                   maxord;
    int                   poff;     /* A global page offset for all resolutions from orders */
} pm_physmap_up;

#define pm_order_offset(ord, maxord) \
            (1 << ((maxord - ord)))

int pm_block_offset(struct pm_physmap *pmap, uintptr_t addr, int ord) {
    uintptr_t raddr = addr - (uintptr_t)pmap->heap_start - (1 << (pmap->poff + _PT_PS));
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

/* Used to apply attributes to blocks without actually using them. */
/* `start` and `end` are in MOS. */
void _kmalloc2(struct pm_physmap *pmap, void *start, void *end, uint8_t flags) {
    struct vm_page_struct *block;
    uintptr_t startp = (uintptr_t)start, endp = (uintptr_t)end;
    uintptr_t blockstartp, blockendp;

    int order = pmap->maxord;
    
    /* The complete condition here is when the all queried blocks that lie within [start, end-1]
       are contained completely within [start, end-1]. This will always be possible at some point,
       assuming start and end are order-0 aligned. */
    for (; order > 0; order--) {
        block = pmap->orders[order];

        for (;;) {
            if (!list_entry_valid(&block->fle))
                break;

            blockstartp = (uintptr_t)VM_HEAP_FROM_PGDAT(block);
            blockendp = blockstartp + VM_HEAP_BLOCK_SIZE(order);

            /* Is this block completely within [start, end-1]? */
            if (VM_HEAP_FROM_PGDAT(block) >= startp &&
                (VM_HEAP_FROM_PGDAT(block) + VM_HEAP_BLOCK_SIZE(order)) <= endp) {
                list_remove(block);
                block->flags |= VM_PAGE_KEEPOUT;
            }
            else
            /* Is this block partially overlapping [start, end-1]? */
            if ((startp >= blockstartp && startp < blockendp)
             || (endp > blockstartp && endp <= blockendp)) {
                // Remove this block, add its children to the freelist,
                // and let the next order iteration pick them up to do the same checks.

                list_remove(block);
                _kfree(block, order - 1);
                _kfree(block + VM_ORDER_BLOCK_OFFSET(order - 1, 1), order - 1);
            }

            block = (struct vm_page_struct *)block->fle.next;
            if (block == LIST_TAIL_TERM)
                break;
        }
    }
}

/* Ensures a certain order of block granularity exists within the map for addr.
   addr is page aligned. addr needs to be in the max-order-offset space */
// int pm_physmap_assert_split(struct pm_physmap *pmap, void *addr, int order) {
//     int block_offset, iord;
//     list_head_t *block;
//     uintptr_t tgt_block_base;

//     /* If the block already exists at a finer granularity, do nothing. */    
//     /* Find the higher order block and request splits */
//     for (iord = 0; iord <= pmap->maxord; iord++) {
//         tgt_block_base = ALIGN_DOWN((uintptr_t)(addr), _PT_PS + iord);
//         block_offset = pm_block_offset(pmap, tgt_block_base, iord);
//         block = &pmap->pgdat_start[pm_order_offset(iord, pmap->maxord) + block_offset];

//         if (list_entry_valid(block)) {
//             if (iord > order) {
//                 _pm_block_split(pmap, block_offset, iord);
//                 iord -= 2;
//                 continue;
//             }
//             return 1;
//         }
//     }

//     return 0;
// }

// int pm_physmap_mark_region(struct pm_physmap *pmap, void *start, void *end) {
//     int start_min_ord, end_min_ord;
//     uintptr_t start_align_goal;
//     int iord;

//     /* Ensure we have sufficient granularity of control over ALIGN_DOWN(start, _PT_PS) */
//     start_min_ord = 0;
//     start_align_goal = ALIGN_DOWN((uintptr_t)start, _PT_PS);

//     /* Aritmetic coming up which operates on pgdat, so need to offset */
//     start_align_goal -= (pmap->poff << _PT_PS);

//     for (iord = pmap->maxord; iord; iord--) {
//         /* Get the highest order that also aligns with this page to
//            minimise splitting. */

//         if (start_align_goal == ALIGN_DOWN((uintptr_t)start, _PT_PS + iord)) {
//             start_min_ord = iord;
//             break;
//         }
//     }
//     pm_physmap_assert_split(pmap, start_align_goal, start_min_ord);
// }

int pm_physmap_init(struct pm_extent *physmem_ext, struct pm_extent *keepout_head) {
    uintptr_t physmem_sz = physmem_ext->end - physmem_ext->start;
    uintptr_t ordsz;
    struct pm_extent *keepout = keepout_head;
    struct pm_physmap *pmap = &pm_physmap_up;
    list_head_t *ord;
    int ordidx, maxord, i, j, rc, pgdat_entries;

    if (!IS_ALIGNED_LSL(physmem_ext->start, _PT_PS)
    || (!IS_ALIGNED_LSL(physmem_ext->end, _PT_PS))) {
        panic("pm_physmap_init: addressable extent vas must be at least page aligned");
    }

    if (physmem_sz <= (1 << (VM_MAX_ORDER + _PT_PS)))
        panic("pm_physmap_init: addressable range cannot be covered by buddy allocator");

    maxord = ffs64(physmem_sz) - _PT_PS;
    ordsz = (1 << (_PT_PS + maxord));
    pgdat_entries = (1 << (maxord));

    pmap->pgdat_start = (struct vm_page_struct*)physmem_ext->start;
    pmap->heap_start = ALIGN_UP((uintptr_t)pmap->pgdat_start + pgdat_entries * sizeof(struct vm_page_struct), _PT_PS);
    
    /* How many order-0 pages are we off from being order-maxord aligned? 
       This will affect all block offset calculations in pm_physmap. */
    pmap->poff = ((uintptr_t)pmap->heap_start - ALIGN_DOWN((uintptr_t)pmap->heap_start, maxord)) >> _PT_PS;

    for (ord = pmap->pgdat_start, ordidx = 0; ordidx < maxord; ordidx++) {
        list_head_init_invalid(ord);
        pmap->orders[ordidx] = ord;
        ord += (1 << (maxord - ordidx));
    }
    list_head_init(ord);
    pmap->orders[ordidx] = ord;

    /* Revise available size after bookkeeping */
    physmem_sz = physmem_ext->end - (uintptr_t)pmap->heap_start;

    /* Already guaranteed size will be a PAGE_SIZE multiple */
    pm_physmap_mark_region(pmap, pmap->heap_start + physmem_sz, pmap->heap_start + ordsz);

    for (; keepout; keepout = keepout->lentry.next) {
        if (keepout->start >= keepout->end)
            panic("pm_physmap_init: malformed or zero-size keepout entry");

        /* XXX: Check whether hole is over pgdat area - panic for now */
        if ((keepout->start >= pmap->pgdat_start && keepout->start < pmap->pgdat_start + 
                ((uintptr_t)pmap->heap_start - (uintptr_t)pmap->pgdat_start))
         || (keepout->end > pmap->pgdat_start && keepout->end <= pmap->pgdat_start + 
                ((uintptr_t)pmap->heap_start - (uintptr_t)pmap->pgdat_start)))
            panic("pm_physmap_init: keepout entry will cause buddy bookkeeping corruption");

        /* Check whether hole is completely irrelevant */
        if ((keepout->start < pmap->pgdat_start && keepout->end <= pmap->pgdat_start)
         || (keepout->start >= pmap->heap_start + ordsz))
            continue;

        pm_physmap_mark_region(pmap, keepout->start, keepout->end);
    }

    return rc;
}

/* The part of the kmalloc routine which can be reentered to get
   larger block sizes. i.e. it deals with pgdat space (struct vm_page_struct *) 
   instead of heap space (void *). */
struct vm_page_struct *_kmalloc(int order) {
    struct vm_page_struct *block;
    struct pm_physmap *pmap = &pm_physmap_up;

    /* This can occur in two ways. Either the kmalloc caller requested an invalid
       order, or recursion has led us to this termination condition. */
    if (order > pmap->maxord)
        panic("kmalloc: order exceeds maximum for this map");

    block = pmap->orders[order];
    if (!block) {
        /* Freelist empty - get a larger page and _kfree the buddy 
           back onto this freelist */
        block = _kmalloc(order + 1);

        /* The _kfree operation doesn't just liberate the unused buddy,
           it actually _performs_ the split from (order + 1) to order. */
        _kfree(block + VM_ORDER_BLOCK_OFFSET(order, 1), order);
    }

    list_remove(&block->fle);

    return block;
}

/* Generic kernel memory allocation routine */
void *kmalloc(int order) {
    struct vm_page_struct *block;
    struct pm_physmap *pmap = &pm_physmap_up;

    block = _kmalloc(order);
    return VM_HEAP_FROM_PGDAT(block);
}

 void _kfree(struct vm_page_struct *block, int order) {
    struct vm_page_struct *target;
    struct pm_physmap *pmap = &pm_physmap_up;
    int child, is_lo_buddy;

    if (list_entry_valid(&block->fle))
        panic("kfree: double free or overlapping orders");

    /* Buddy merge check */
    if (order < VM_MAX_ORDER) {
        is_lo_buddy = (VM_GET_BUDDY_TYPE(block) == 0);
        target = is_lo_buddy ? (block + VM_ORDER_BLOCK_OFFSET(order, 1))   
                             : (block - VM_ORDER_BLOCK_OFFSET(order, 1));

        /* Travel up the orders... */
        if (list_entry_valid(&target->fle)) {
            list_remove(&block->fle);
            list_remove(&target->fle);

            target = is_lo_buddy ? (block) : (target);
            _kfree(target, order + 1);
        } 
        /* Our buddy cannot be coalesced... */
        else {
            list_head_init(&block->fle);

            if (pmap->orders[order]) 
                block->fle.next = &pmap->orders[order]->fle;
            else
                block->fle.next = LIST_TAIL_TERM;

            pmap->orders[order] = block;
        }
    } else {
        list_head_init(&block->fle);
        block->fle.next = LIST_TAIL_TERM;

        pmap->orders[order] = block;
    }
}

void kfree(void *addr, int order) {
    struct vm_page_struct *block;
    struct pm_physmap *pmap = &pm_physmap_up;
    char *mos_addr = VM_ADDR_TO_MOS(addr);

    if ((order < 0) || (order > VM_MAX_ORDER) || 
        (!IS_ALIGNED_LSL((uintptr_t)mos_addr, order + _PT_PS)))
        panic("kfree: order out of range or addr unaligned with order");

    block = VM_PGDAT_FROM_HEAP(mos_addr);
    _kfree(block, order);
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
