//
// Created by Bikram Kumar Panda on 07/04/26.
//

#ifndef BIOHASHER_SPECIFICS_H
#define BIOHASHER_SPECIFICS_H

#include <cstdint>
#include <cstring>


// Threading includes and global state
#if defined(HAVE_THREADS)
  #ifdef _MSC_VER
    #pragma push_macro("unreachable")
    #undef unreachable
  #endif
  #include <thread>
  #ifdef _MSC_VER
    #pragma pop_macro("unreachable")
  #endif
  extern unsigned g_NCPU;
#else
extern const unsigned g_NCPU;
#endif


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


//-----------------------------------------------------------------------------
// Compiler-specific functions and macros

// Compiler branching hints
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define expectp(x,p) __builtin_expect_with_probability(!!(x), 1, (p))
#define unpredictable(x) __builtin_unpredictable(x)

// Compiler behavioral bounds hints
#define assume(x) __builtin_assume(x)
#define unreachable() __builtin_unreachable()

// Compiler/CPU data prefetching
#define prefetch(ptr) __builtin_prefetch(ptr)

// Function inlining control
#define FORCE_INLINE __attribute__((__always_inline__)) inline
#define NEVER_INLINE __attribute__((__noinline__))

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

#endif //BIOHASHER_SPECIFICS_H