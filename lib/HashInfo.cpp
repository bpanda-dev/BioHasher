/*
* SMHasher3
 * Copyright (C) 2021-2022  Frank J. T. Wojcik
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "Hashlib.h"

#include <cstdio>
#include <string>
#include <algorithm>


const char * HashInfo::_fixup_name( const char * in ) {
    // Since dashes can't be in C/C++ identifiers, but humans want
    // them in names, replace underscores with dashes. Similarly,
    // replace double underscores with dots.
    std::string out( in );

    do {
        size_t p = out.find("__");
        if (p == std::string::npos) { break; }
        out.replace(p, 2, ".");
    } while (true);
    std::replace(&out[0], &out[out.length()], '_', '-');
    return strdup(out.c_str());
}

const char * HashFamilyInfo::_fixup_name( const char * in ) {
    return HashInfo::_fixup_name(in);
}


