/*
 * DoNothing Hash
 * Copyright (C) 2021-2022  Frank J. T. Wojcik
 * Copyright (c) 2014-2021 Reini Urban
 * Copyright (c) 2015      Paul G
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
 */
#include "Hashlib.h"

#include <stdio.h>


#define unused(x) (void)(x)


static void DoNothingHash( const void * in, const size_t len, const seed_t seed, void * out ) {
    unused(in); unused(len); unused(seed); unused(out);
    // printf("Hello World");
}

REGISTER_FAMILY(donothing,
   $.src_url    = "https://github.com/rurban/smhasher/blob/master/Hashes.cpp",
   $.src_status = HashFamilyInfo::SRC_FROZEN
 );

// Speed tests use this as a baseline hash for measuring overhead. Don't
// mess with it too much.
REGISTER_HASH(donothing_32,
   $.desc       = "Do-Nothing function (measure call overhead)",
   $.hash_flags = FLAG_HASH_LOCALITY_SENSITIVE,
   $.impl_flags = FLAG_IMPL_SLOW    |   FLAG_IMPL_LICENSE_MIT,
   $.bits = 32,
   $.hashfn   = DoNothingHash
 );
