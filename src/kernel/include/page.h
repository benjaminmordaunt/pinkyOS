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

#ifndef H_PAGE
#define H_PAGE

#define PA_BITS                                               48   

#define ENTRY_VALID                                     (1 << 0)
#define ENTRY_TABLE                                     (1 << 1)

typedef unsigned long long int va_t;
typedef unsigned long long int pgtbl_desc_t;
typedef unsigned long long int pgtbl_attrs_t;

unsigned int PT_LEVEL_ADDR_SHIFT[5];
unsigned int PT_LEVEL_PTRS[5];
va_t         PT_LEVEL_MASK[5];

#ifdef PAGE_SIZE_4K
    #define _PT_N_MAX     3
    #define _PT_PS       12

    /* Next level table address mask for 4K granule and 48-bit OA */
    #define _PT_TABLE_ADDR_SHIFT                                              _PT_PS
    #define _PT_TABLE_ADDR_SIZE              (1 << (PA_BITS - _PT_TABLE_ADDR_SHIFT))
    #define _PT_TABLE_ADDR_MASK  ((_PT_TABLE_ADDR_SIZE - 1) << _PT_TABLE_ADDR_SHIFT)

    /* Block output address mask inferred */

    /* Page address mask for 4K granule and 48-bit OA - same as table */
    #define _PT_PAGE_ADDR_SHIFT                                 _PT_TABLE_ADDR_SHIFT
    #define _PT_PAGE_ADDR_SIZE                                   _PT_TABLE_ADDR_SIZE
    #define _PT_PAGE_ADDR_MASK                                   _PT_TABLE_ADDR_MASK
#endif /* PAGE_SIZE_4K */

#define PAGE_SIZE                                                      (1 << _PT_PS)

/* Memory alignment */
#define ALIGN_DOWN(addr, align) \
            (addr & ~((1 << align) - 1))

#define ALIGN_UP(addr, align) \
            (ALIGN_DOWN(addr, align) + (1 << align))

void init_pgtbl_constants(void);
inline unsigned int pgtbl_level_idx(va_t va, int lvl);

int pgtbl_walk(pgtbl_desc_t *base, va_t va, pgtbl_desc_t **entry_out, int alloc);
int pgtbl_map_pages(pgtbl_desc_t *base, va_t start, va_t end, pa_t phys_start, pgtbl_attrs_t attrs);

#endif /* H_PAGE */
