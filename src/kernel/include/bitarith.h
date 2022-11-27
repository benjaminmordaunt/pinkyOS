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

// Outside of strict ISO-C mode, the ffs* functions may be defined already.
// Even though we should be building pinkyOS with -std=c11, check if the compiler already exposes them.

#ifndef ffs /* assume friends are also not defined */
    // #define ffs   __builtin_ffs
    #define ffs32  __builtin_ffsl
    #define ffs64  __builtin_ffsll
#endif /* ffs */
