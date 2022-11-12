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

#define PA_BITS                                               48                        

typedef unsigned long long int va_t; 

unsigned int PT_LEVEL_ADDR_SHIFT[5];
unsigned int PT_LEVEL_PTRS[5];
va_t         PT_LEVEL_MASK[5];

#ifdef PAGE_SIZE_4K
    #define _PT_N_MAX     3
    #define _PT_PS       12
#endif /* PAGE_SIZE_4K */

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
