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

/* A simple implementation of a buddy allocator to kickstart development of pinkyOS.
 */

#include "kmem.h"
#include <types.h>
#include <inttypes.h>
#include "page.h"

/* XXX: Use a routine to determine maximum required order based on physical memory
        extent. For now, the max order will be hard-coded to 20, which allows for management 
        of up to a single 4GiB contiguous space.
*/

#define PM_BUDDY_MAX_ORDER                             20       
#define PM_BUDDY_BM_OFFSET(order, maxord)               \
            ((uint32_t)(1 << (MAX(maxord - order, 0))))

/* Represents a contiguous span of physical memory. Exclusive of end. */
struct pm_extent {
    va_t start;
    va_t end;
    struct vm_extent *next;
};

/* Represents a collection of pm_extent. Typically used
   for expressing keep-out zones (memory holes) described
   by the bootloader. */
/* XXX: Standard linked list API for pinkyOS. */

struct pm_buddy_block {
    struct pm_buddy_block *next; /* References next free block, possibly its buddy. */
    struct pm_buddy_block *prev; /* References previous free block, possibly its buddy.*/
};

/* A single order manages memory of granularity 2^ord, where ord
   is the index of the order in the parent pm_physmap.orders array. 
   alloc_head references a null-pointer, or a pointer to the first 
   pm_buddy_block structure if at least one (two with its buddy) is
   present in the buddy system. */
struct pm_buddy_order {
    uint32_t              *bitmap; /* The "base address" of this order in the global bitmap */
};

struct pm_physmap {
    uint32_t              *journal_start;
    uint32_t              *heap_start;
    struct pm_buddy_order orders[PM_BUDDY_MAX_ORDER + 1];
    int                   maxord;
} pm_physmap_up;

int pm_physmap_init(struct pm_extent *physmem_ext, struct pm_extent *keepout_head) {
    pa_t physmem_sz = physmem_ext->end - physmem_ext->start;
    struct pm_extent *keepout = keepout_head;
    struct pm_physmap pmap = pm_physmap_up;
    struct pm_buddy_order ord;
    int nbitmaps, maxord, i;

    if (physmem_ext->start != ALIGN_DOWN(physmem_ext->start, _PT_PS)
    || (physmem_ext->end   != ALIGN_DOWN(physmem_ext->end,   _PT_PS))) {
        panic("pm_physmap_init: addressable extent vas must be at least page aligned");
    }

    if (physmem_sz & ((1 << PM_BUDDY_MAX_ORDER) - 1) != 0)
        panic("pm_physmap_init: addressable range cannot be covered by buddy allocator");

    pmap.journal_start = (uint32_t *)physmem_ext->start;
    
    /* Need to know nbitmaps to find how large the bookkeeping journal needs to be,
       but the more we eat into the heap, the (potentially) fewer bitmaps we need.
       Just overprovision and avoid the nasty math for now. */
    nbitmaps = 1 << MAX(ffs(physmem_sz) - 3, 0);
    if (physmem_sz < nbitmaps * sizeof(uint32_t))
        panic("pm_physmap_init: insufficient address space for buddy bookkeeping");

    for (i = 0; i < nbitmaps; i++) {
        pmap.journal_start[i] = UINT32_MAX;
    }

    pmap.heap_start = &pmap.journal_start[i]; 
    
    /* Still cannot update physmem_sz as there cannot be disparity between maxord
       and the size of the bitmap for the bitmap offset calc routines. */
    pmap.maxord = MAX(ffs(physmem_sz) + 1 - _PT_PS, 0);
    for (i = 0; i < maxord; i++) {
        pmap.orders[i].bitmap = pmap.journal_start + PM_BUDDY_BM_OFFSET(i, maxord);
    }
    pmap.orders[i] = (struct pm_buddy_order){ 0 };

    for (;; keepout = keepout->next) {
        pm_physmap_alloc(keepout->start, keepout->end - keepout->start, PM_KEEPOUT);
    }
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
