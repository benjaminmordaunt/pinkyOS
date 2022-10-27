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

#define AA64_MAIR_ATTR_IDX(idx, val)                \
	val << (idx << 3)

#define AA64_CPACR_FPEN_SHIFT	20

/* aarch64 normal memory attributes:
 *  - WB => write-back
 *  - WT => write-through
 *  - NC => non-cacheable
 *  - NT => non-transient
 *  - T  => transient (unused)
 *
 * for the curious - the transient
 * bit is supposed to indicate that
 * "the benefit of caching is for a
 * relatively short period", an
 * apparently useless notion that most
 * implementations ignore.
 *
 * attributes always applied across
 * both inner and outer shareable
 * domains (i.e. 'oooo' == 'iiii').
 */

#define AA64_MEM_NORMAL_WBNT	0xFF
#define AA64_MEM_NORMAL_NC      0x44

/* aarch64 device memory attributes:
 *  - G => gathering (merged transactions)
 *  - R => reordering
 *  - E => early-write ack
 */

#define AA64_MEM_DEVICE_GRE     0x0C
#define AA64_MEM_DEVICE_nGnRE   0x04
#define AA64_MEM_DEVICE_nGnRnE  0x00

#define AA64_TCR_T0SZ_SHIFT	0x00
#define AA64_TCR_EPD0_SHIFT	0x07
#define AA64_TCR_IRGN0_SHIFT	0x08
#define AA64_TCR_ORGN0_SHIFT	0x0A
#define AA64_TCR_SH0_SHIFT	0x0C
#define AA64_TCR_TG0_SHIFT	0x0E
#define AA64_TCR_T1SZ_SHIFT	0x10
#define AA64_TCR_A1_SHIFT	0x16
#define AA64_TCR_EPD1_SHIFT	0x17
#define AA64_TCR_IRGN1_SHIFT	0x18
#define AA64_TCR_ORGN1_SHIFT	0x1A
#define AA64_TCR_SH1_SHIFT	0x1C
#define AA64_TCR_TG1_SHIFT	0x1E
#define AA64_TCR_IPS_SHIFT	0x20
#define AA64_TCR_AS_SHIFT	0x24
#define AA64_TCR_TBI0_SHIFT	0x25
#define AA64_TCR_TBI1_SHIFT	0x26
#define AA64_TCR_HA_SHIFT	0x27
#define AA64_TCR_HD_SHIFT	0x28
#define AA64_TCR_HPD0_SHIFT	0x29
#define AA64_TCR_HPD1_SHIFT	0x2A
#define AA64_TCR_HWU059_SHIFT	0x2B
#define AA64_TCR_HWU060_SHIFT	0x2C
#define AA64_TCR_HWU061_SHIFT	0x2D
#define AA64_TCR_HWU062_SHIFT	0x2E
#define AA64_TCR_HWU159_SHIFT	0x2F
#define AA64_TCR_HWU160_SHIFT	0x30
#define AA64_TCR_HWU161_SHIFT	0x31
#define AA64_TCR_HWU162_SHIFT	0x32
#define AA64_TCR_TBID0_SHIFT	0x33
#define AA64_TCR_TBID1_SHIFT	0x34
#define AA64_TCR_NFD0_SHIFT	0x35
#define AA64_TCR_NFD1_SHIFT	0x36
#define AA64_TCR_E0PD0_SHIFT	0x37
#define AA64_TCR_E0PD1_SHIFT	0x38
#define AA64_TCR_TCMA0_SHIFT	0x39
#define AA64_TCR_TCMA1_SHIFT	0x3A
#define AA64_TCR_DS_SHIFT	0x3B
