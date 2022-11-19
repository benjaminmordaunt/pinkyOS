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
#define PM_BUDDY_BM_OFFSET(order, maxord)               \
            ((uint32_t)(1 << (MAX(maxord - order, 0))))

/* Represents a contiguous span of physical memory. Exclusive of end. */
struct pm_extent {
    va_t start;
    va_t end;
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
    void                 *heap_start;
    list_head_t           orders[PM_BUDDY_MAX_ORDER];
    int                   maxord;
} pm_physmap_up;

int _pm_physmap_init(void *start, pa_t size, int order, struct pm_extent *keepout_head) {
    struct pm_extent *keepout = keepout_head;
    pa_t ordsz = (1 << (_PT_PS + order));
    
    list_head_init(start);

    /* pm_physmap_init already guaranteed size will be a PAGE_SIZE multiple */
    pm_physmap_free_region(start, start + size, start + ordsz);

    for (; keepout; keepout = keepout->lentry.next) {
        if (keepout->start >= keepout->end)
            panic("pm_physmap_init: malformed or zero-size keepout entry");

        /* Check whether hole is completely irrelevant */
        if ((keepout->start < start && keepout->end <= start)
         || (keepout->start >= start + ordsz))
            continue;

        pm_physmap_free_region(start, keepout->start, keepout->end);
    }

    return 0;
}

int pm_physmap_init(struct pm_extent *physmem_ext, struct pm_extent *keepout_head) {
    pa_t physmem_sz = physmem_ext->end - physmem_ext->start;
    struct pm_extent *keepout = keepout_head;
    struct pm_physmap *pmap = &pm_physmap_up;
    struct list_head_t *ord;
    int maxord, i, rc;

    if (physmem_ext->start != ALIGN_DOWN(physmem_ext->start, _PT_PS)
    || (physmem_ext->end   != ALIGN_DOWN(physmem_ext->end,   _PT_PS))) {
        panic("pm_physmap_init: addressable extent vas must be at least page aligned");
    }

    if (physmem_sz & ((1 << (PM_BUDDY_MAX_ORDER + _PT_PS)) - 1) != 0)
        panic("pm_physmap_init: addressable range cannot be covered by buddy allocator");

    pmap->heap_start = (uint32_t *)physmem_ext->start;

    maxord = ffs(physmem_sz) + 1 - _PT_PS;
    rc = _pm_physmap_init(pmap->heap_start, maxord, keepout_head);

    for (; keepout; keepout = keepout->lentry.next) {
        pm_physmap_alloc(keepout->start, keepout->end - keepout->start, PM_KEEPOUT);
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
