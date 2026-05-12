/*
* SMHasher3
 * Copyright (C) 2021-2022  Frank J. T. Wojcik
 * Copyright (C) 2026  Bikram
 *
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

#ifndef BIOLSHASHER_SPECIFICS_H
#define BIOLSHASHER_SPECIFICS_H

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <inttypes.h> // Required for PRId64

//-----------------------------------------------------------------------------
// C "restrict" keyword replacement
#define RESTRICT __restrict

//-----------------------------------------------------------------------------
#if defined(_MSC_VER)
# define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
# define FORCE_INLINE inline __attribute__((always_inline))
#else
# define FORCE_INLINE inline
#endif



//-----------------------------------------------------------------------------
// User-configurable implementations

// Endianness detection
static FORCE_INLINE bool isLE(void) { return true; }
static FORCE_INLINE bool isBE(void) { return false; }

// Threading includes and global state
#if defined(HAVE_THREADS)
  #include <thread>
  extern unsigned g_NCPU;
#else
extern const unsigned g_NCPU;
#endif

//-----------------------------------------------------------------------------
// VLA helper
//
// Allocates on heap when VLA is not available
#if defined(__STDC_NO_VLA__) || defined(_MSC_VER) && !defined(__clang__)
    #include <vector>
    #define VLA_ALLOC(type, name, count) std::vector<type> name(count)
#else
    #define VLA_ALLOC(type, name, count)  \
    _Pragma("GCC diagnostic push");          \
    _Pragma("GCC diagnostic ignored \"-Wvla\""); \
    _Pragma("clang diagnostic ignored \"-Wvla-extension\""); \
    type name[count];                            \
    _Pragma("GCC diagnostic pop");
#endif



#if defined(DEBUG)
  #include <cassert>

  #undef assume
  #define assume(x) assert(x)

  #undef unreachable
  #define unreachable() assert(0)

  #define verify(x) assert(x)
#else
  static void warn_if(bool x, const char * s, const char * fn, uint64_t ln) {
      if (!x) {
          printf("Statement %s is not true: %s:%" PRId64 "\n", s, fn, ln);
      }
  }
  #define verify(x) warn_if(x, #x, __FILE__, __LINE__)
#endif

#define unused(x) (void)(x)


//-----------------------------------------------------------------------------
// Compiler-specific functions and macros

// Compiler branching hints
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define expectp(x,p) __builtin_expect_with_probability(!!(x), 1, (p))
#define unpredictable(x) __builtin_unpredictable(x)

// Compiler behavioral bounds hints
#if defined(__clang__)
#define assume(x) __builtin_assume(x)
#elif defined(__GNUC__)
#define assume(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#else
#define assume(x) ((void)0)
#endif
#define unreachable() __builtin_unreachable()

// // Compiler behavioral bounds hints
// #define assume(x) __builtin_assume(x)
// #define unreachable() __builtin_unreachable()

// Compiler/CPU data prefetching
#define prefetch(ptr) __builtin_prefetch(ptr)

// Function inlining control
// #define FORCE_INLINE __attribute__((__always_inline__)) inline
// #define NEVER_INLINE __attribute__((__noinline__))

// Variable aliasing
#define MAY_ALIAS __attribute__((__may_alias__))

// C "restrict" keyword replacement
#define RESTRICT __restrict

// Integer rotation
// CAUTION: These are deliberately unsafe!
// They assume the rotation value is not 0 or >= {32,64}.
// If an invalid rotation amount is given, the result is undefined behavior.
static inline uint32_t _rotl32(uint32_t v, uint8_t n) {
    return (v << n) | (v >> ((-n) & 31));
}
static inline uint32_t _rotr32(uint32_t v, uint8_t n) {
    return (v >> n) | (v << ((-n) & 31));
}
#define ROTL32(v, n) _rotl32(v, n)
#define ROTR32(v, n) _rotr32(v, n)
static inline uint64_t _rotl64(uint64_t v, uint8_t n) {
    return (v << n) | (v >> ((-n) & 63));
}
static inline uint64_t _rotr64(uint64_t v, uint8_t n) {
    return (v >> n) | (v << ((-n) & 63));
}
#define ROTL64(v, n) _rotl64(v, n)
#define ROTR64(v, n) _rotr64(v, n)

// Population count (popcnt/popcount)
#define popcount4(x) __builtin_popcount(x)
#define popcount8(x) __builtin_popcountll(x)

// Leading zero-bit count (ffs/clz)
// CAUTION: Assumes x is not 0!
#define clz4(x) __builtin_clz(x)
#define clz8(x) __builtin_clzll(x)

// Integer byteswapping (endianness conversion)
// You can use the bit-specific ones directly, but the more generic
// versions below are preferred.
#define BSWAP16(x) __builtin_bswap16(x)
#define BSWAP32(x) __builtin_bswap32(x)
#define BSWAP64(x) __builtin_bswap64(x)

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// Generic wrappers for BSWAP*()
template < typename T >
static FORCE_INLINE T BSWAP(T value) {
    switch(sizeof(T)) {
    case 2: value = BSWAP16((uint16_t)value); break;
    case 4: value = BSWAP32((uint32_t)value); break;
    case 8: value = BSWAP64((uint64_t)value); break;
    default: break;
    }
    return value;
}

template < typename T >
static FORCE_INLINE T COND_BSWAP(T value, bool doit) {
    if (!doit || (sizeof(T) < 2)) { return value; }
    return BSWAP(value);
}


static FORCE_INLINE uint64_t GET_U64(const uint8_t * b, const uint32_t i) {
    uint64_t n;
    memcpy(&n, &b[i], 8);
    return n;
}

static FORCE_INLINE uint32_t GET_U32(const uint8_t * b, const uint32_t i) {
    uint32_t n;
    memcpy(&n, &b[i], 4);
    return n;
}

static FORCE_INLINE uint16_t GET_U16(const uint8_t * b, const uint32_t i) {
    uint16_t n;
    memcpy(&n, &b[i], 2);
    return n;
}

static FORCE_INLINE void PUT_U16(uint16_t n, uint8_t * b, const uint32_t i) {
    memcpy(&b[i], &n, 2);
}

static FORCE_INLINE void PUT_U32(uint32_t n, uint8_t * b, const uint32_t i) {
    memcpy(&b[i], &n, 4);
}

static FORCE_INLINE void PUT_U64(uint64_t n, uint8_t * b, const uint32_t i) {
    memcpy(&b[i], &n, 8);
}

#endif //BIOLSHASHER_SPECIFICS_H