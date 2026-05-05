/*
 * FNV and similar hashes
 * Copyright (C) 2021-2022  Frank J. T. Wojcik
 * Copyright (c) 2014-2021 Reini Urban
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 *    * Neither the name of Google Inc. nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <cassert>

#include "Mathmult.h"
#include "specifics.h"
#include "Hashlib.h"
#include <random>
#include <vector>
#include <cstdint>

// All seeding below this is homegrown for SMHasher3

template <typename hashT>
static void FNV1a( const void * in, const size_t len, const seed_t seed, void * out ) {
    const uint8_t * data = (const uint8_t *)in;
    const hashT     C1   = (sizeof(hashT) == 4) ? UINT32_C(2166136261) :
                                                  UINT64_C(0xcbf29ce484222325);
    const hashT     C2   = (sizeof(hashT) == 4) ? UINT32_C(  16777619) :
                                                  UINT64_C(0x100000001b3);
    hashT h = (hashT)seed;

    h ^= C1;
    for (size_t i = 0; i < len; i++) {
        h ^= data[i];
        h *= C2;
    }

    memcpy(out, &h, sizeof(h));
}

static void FNV1a_128( const void * in, const size_t len, const seed_t seed, void * out ) {
    const uint8_t * data = (const uint8_t *)in;
    const uint64_t  C1lo = UINT64_C(0x62b821756295c58d);
    const uint64_t  C1hi = UINT64_C(0x6c62272e07bb0142);
    const uint64_t  C2   = UINT64_C(0x13b);

    uint64_t s0, s1;
    uint64_t hash[2] = { seed, seed };

    hash[0] ^= C1hi;
    hash[1] ^= C1lo;
    for (size_t i = 0; i < len; i++) {
        hash[1] ^= data[i];
        MathMult::mult64_128(s1, s0, C2, hash[1]);
        s0 += (hash[1] << 24) + C2 * hash[0];
        hash[0] = s0;
        hash[1] = s1;
    }

    PUT_U64(hash[0], (uint8_t *)out, 0);
    PUT_U64(hash[1], (uint8_t *)out, 8);
}

// Also https://www.codeproject.com/articles/716530/fastest-hash-function-for-table-lookups-in-c
// Also https://github.com/golang/go/, src/hash/fnv/fnv.go
REGISTER_FAMILY(fnv,
   $.src_url    = "http://www.sanmayce.com/Fastest_Hash/index.html",
   $.src_status = HashFamilyInfo::SRC_STABLEISH
 );

REGISTER_HASH(FNV_1a_32,
   $.desc       = "32-bit bytewise FNV-1a (Fowler-Noll-Vo)",
   $.hash_flags = 0,
   $.impl_flags = FLAG_IMPL_MULTIPLY | FLAG_IMPL_LICENSE_MIT | FLAG_IMPL_VERY_SLOW,
   $.bits       = 32,
   $.hashfn     = FNV1a<uint32_t>
 );

REGISTER_HASH(FNV_1a_64,
   $.desc       = "64-bit bytewise FNV-1a (Fowler-Noll-Vo)",
   $.hash_flags = 0,
   $.impl_flags =
         FLAG_IMPL_MULTIPLY_64_64 |
         FLAG_IMPL_LICENSE_MIT    |
         FLAG_IMPL_VERY_SLOW,
   $.bits = 64,
   $.hashfn   = FNV1a<uint64_t>
//    $.badseeds        = { 0xcbf29ce484222325 }
 );

REGISTER_HASH(FNV_1a_128,
   $.desc       = "128-bit bytewise FNV-1a (Fowler-Noll-Vo), from Golang",
   $.hash_flags = 0,
   $.impl_flags =
         FLAG_IMPL_MULTIPLY_64_128    |
         FLAG_IMPL_LICENSE_BSD        |
         FLAG_IMPL_VERY_SLOW,
   $.bits = 128,
   $.hashfn   = FNV1a_128
 );
