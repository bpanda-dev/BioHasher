#pragma once
#include <cstdio>
#include <cstdlib>
#include <string_view>

// ANSI color codes (disable if not a TTY)
#define _LSH_RED     "\033[1;31m"
#define _LSH_YELLOW  "\033[1;33m"
#define _LSH_CYAN    "\033[1;36m"
#define _LSH_RESET   "\033[0m"

// Core assertion macro with context
#define BIOLSHASHER_ASSERT(cond, msg)                                                        \
    do {                                                                             \
        if (!(cond)) {                                                               \
            fprintf(stderr, "\n");                                                   \
            fprintf(stderr, _LSH_RED "[BioLSHasher ASSERT FAILED]" _LSH_RESET "\n"); \
            fprintf(stderr, "  Condition : " _LSH_YELLOW #cond _LSH_RESET "\n");   \
            fprintf(stderr, "  Message   : " _LSH_CYAN   msg   _LSH_RESET "\n");   \
            fprintf(stderr, "  Location  : %s:%d\n", __FILE__, __LINE__);           \
            fprintf(stderr, "  Function  : %s\n\n", __PRETTY_FUNCTION__);           \
            std::abort();                                                            \
        }                                                                            \
    } while (0)

// Variant: prints the actual value of a flag/enum when assertion fails
#define BIOLSHASHER_ASSERT_FLAG(cond, msg, flagval)                                    \
    do {                                                                             \
        if (!(cond)) {                                                               \
            fprintf(stderr, "\n");                                                   \
            fprintf(stderr, _LSH_RED "[BioLSHasher ASSERT FAILED]" _LSH_RESET "\n"); \
            fprintf(stderr, "  Condition : " _LSH_YELLOW #cond _LSH_RESET "\n");   \
            fprintf(stderr, "  Message   : " _LSH_CYAN   msg   _LSH_RESET "\n");   \
            fprintf(stderr, "  Flag value: 0x%08x\n", (unsigned)(flagval));         \
            fprintf(stderr, "  Location  : %s:%d\n", __FILE__, __LINE__);           \
            fprintf(stderr, "  Function  : %s\n\n", __PRETTY_FUNCTION__);           \
            std::abort();                                                            \
        }                                                                            \
    } while (0)
    