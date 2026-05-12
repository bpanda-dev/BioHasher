

#ifndef BIOLSHASHER_TESTGLOBALS_H
#define BIOLSHASHER_TESTGLOBALS_H
#include <cstdint>

#include "Blob.h"

typedef uint32_t flags_t;

#define REPORT(flagname, var) (!!(var & FLAG_REPORT_ ## flagname))

#define FLAG_REPORT_QUIET        (1 << 0)
#define FLAG_REPORT_VERBOSE      (1 << 1)
#define FLAG_REPORT_DIAGRAMS     (1 << 2)
#define FLAG_REPORT_MORESTATS    (1 << 3)
#define FLAG_REPORT_PROGRESS     (1 << 4)

//--------
// What each test suite prints upon failure
inline const char * g_failstr = "*********FAIL*********\n";

#endif //BIOLSHASHER_TESTGLOBALS_H
