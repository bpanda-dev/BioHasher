#include <cstdlib>
#include <iostream>
#include "version.h"
#include "Hashlib.h"
#include "TestGlobals.h"


#include "LSHCollision.h"
#include "ApproxNearestNeighbour.h"

static bool g_testAll;
static bool g_testLSHCollision;
static bool g_testLSHApproxNearestNeighbour;

struct TestOpts {
    bool &       var;
    bool         defaultvalue;
    const char * name;
};

static TestOpts g_testopts[] = {
    { g_testLSHCollision,   false,   "LSHCollision" },
    { g_testLSHApproxNearestNeighbour,   false,    "LSHApproxNearestNeighbour" },
};

static void set_default_tests( bool enable ) {
    for (size_t i = 0; i < sizeof(g_testopts) / sizeof(TestOpts); i++) {
        if (enable) {
            g_testopts[i].var = g_testopts[i].defaultvalue;
        } else if (g_testopts[i].defaultvalue) {
            g_testopts[i].var = false;
        }
    }
}

static void parse_tests( const char * str, bool enable_tests ) {
    while (*str != '\0') {
        size_t       len;
        const char * p = strchr(str, ',');
        if (p == NULL) {
            len = strlen(str);
        } else {
            len = p - str;
        }

        struct TestOpts * found = NULL;
        bool foundmultiple      = false;
        for (size_t i = 0; i < sizeof(g_testopts) / sizeof(TestOpts); i++) {
            const char * testname = g_testopts[i].name;
            // Allow the user to specify test names by case-agnostic
            // unique prefix.
            if (strncasecmp(str, testname, len) == 0) {
                if (found != NULL) {
                    foundmultiple = true;
                }
                found = &g_testopts[i];
                if (testname[len] == '\0') {
                    // Exact match found, don't bother looking further, and
                    // don't error out.
                    foundmultiple = false;
                    break;
                }
            }
        }
        if (foundmultiple) {
            printf("Ambiguous test name: --%stest=%*s\n", enable_tests ? "" : "no", (int)len, str);
            goto error;
        }
        if (found == NULL) {
            printf("Invalid option: --%stest=%*s\n", enable_tests ? "" : "no", (int)len, str);
            goto error;
        }

        // printf("%sabling test %s\n", enable_tests ? "en" : "dis", testname);
        found->var = enable_tests;

        // If "All" tests are being enabled or disabled, then adjust the individual
        // test variables as instructed. Otherwise, if a material "All" test (not
        // just a speed-testing test) is being specifically disabled, then don't
        // consider "All" tests as being run.
        if (&found->var == &g_testAll) {
            set_default_tests(enable_tests);
        } else if (!enable_tests && found->defaultvalue) {
            g_testAll = false;
        }

        if (p == NULL) {
            break;
        }
        str += len + 1;
    }

    return;

  error:
    printf("Valid tests: --test=%s", g_testopts[0].name);
    for (size_t i = 1; i < sizeof(g_testopts) / sizeof(TestOpts); i++) {
        printf(",%s", g_testopts[i].name);
    }
    printf(" \n");
    exit(1);
}

void usage() {
    printf("Usage: BioLSHasher [--test=<testname>[,...]] [--verbose] [--ncpu=N]\n"
           "                 [<hashname>]\n"
           "\n"
           "       BioLSHasher [--list]|[--listnames]|[--tests]|[--version]\n"
           "\n"
           "  Hashnames can be supplied using any case letters.\n");
}
//-----------------------------------------------------------------------------


static void examplehash_initialisation() {
    static bool initialized = false;
    if (!initialized) {   
        //---- Variables used across tests which can be defined by user ---//
        g_verySlowNAggCases = 50000;
        g_verySlowNSeq = 1000;
        g_verySlowNHashes = 500;

        g_SlowNAggCases = 50000;
        g_SlowNSeq = 1000;
        g_SlowNHashes = 500;

        g_NAggCases = 50000;
        g_NSeq = 1000;
        g_NHashes = 200;

        g_ShortSequenceLength = 50;
        g_LongSequenceLength = 50;

        g_start_B = 1;
        g_start_R = 1;
        g_MAX_B = 2;
        g_MAX_R = 3;

        //---------------------------------------------------------------------------------------
        uint32_t g_NAggCasesApproxNNTest = 50000;
        uint32_t g_sequenceLengthForApproxNNTest = 50; // Sequence length for Approx Nearest Neighbour test. Adjust as needed.
        uint32_t g_Nseq_in_Database = 50000; // Number of sequences in the reference database for the Approx Nearest Neighbour test. Adjust as needed.
        uint32_t g_numQueriesForApproxNNTest = 1000; // Number of query sequences to generate for the Approx Nearest Neighbour test. Adjust as needed.

        uint32_t g_avgRunsForApproxNN = 2;

        std::vector<double> g_cValuesApproxNNTest = {1}; //{0.5, 0.6, 0.7, 0.8, 0.9, 0.95}; // c-ANN approximation factors to sweep. Each c < 1 defines the boundary as c * target_sim_low.

        uint32_t g_ANN_start_B = 1; // Starting value of b (hashes per table) for the Approx Nearest Neighbour test. Adjust as needed.
        uint32_t g_ANN_start_R = 1; // Starting value of r (number of tables) for the Approx Nearest Neighbour test. Adjust as needed.
        uint32_t g_ANN_MAX_B = 2; // Maximum value of b (hashes per table) to test in the Approx Nearest Neighbour test. Adjust as needed.
        uint32_t g_ANN_MAX_R = 3; // Maximum value of r (number of tables) to test in the Approx Nearest Neighbour test. Adjust as needed.

        double g_simThresholdForApproxNNTest = 0.95; // Similarity threshold for Approx Nearest Neighbour test. Adjust as needed.

        //---------------------------------------------------------------------------------------

        // Global variables for runtime communication
        const uint32_t g_bincount_full = 1000;

        initialized = true;
    }
}

//-----------------------------------------------------------------------------

template <typename hashtype>
static bool test( const HashInfo * hInfo, const flags_t flags ) {
    bool result  = true;

    if(hInfo->name == std::string("exampleHash")) {
        printf("Running example hash. The parameters are set for faster computation");
        examplehash_initialisation();
    }

    if (g_testAll) {
        printf("-------------------------------------------------------------------------------\n");
    }

    if (!hInfo->Init()) {
        printf("Hash initialization failed! Cannot continue.\n");
        exit(1);
    }

    //-----------------------------------------------------------------------------
    // Sanity tests
    FILE * outfile;
    if (g_testAll) {
        outfile = stdout;
    } else {
        outfile = stderr;
    }

    fprintf(outfile, "--- Testing %s :\"%s\" ---", hInfo->name, hInfo->desc);
        
    fprintf(outfile, "\n\n");

    //-----------------------------------------------------------------------------
    // Test for Collision Curve for LSH family of hashes

    if (g_testLSHCollision) {
        result &= LSHCollisionTest<hashtype>(hInfo, flags);
        if (!result) { exit(-1); }   // Failure case.
    }

    //-----------------------------------------------------------------------------
    // Test for Nearest Neighbour search for LSH family of hashes

    if (g_testLSHApproxNearestNeighbour) {
        result &= LSHApproxNearestNeighbourTest<hashtype>(hInfo, flags);
        if (!result) { exit(-1); }   // Failure case.
    }



    return result;
}

//-----------------------------------------------------------------------------

static bool testHash( const char * name, const flags_t flags ) {
    const HashInfo * hInfo;

    if ((hInfo = findHash(name)) == NULL) {
        printf("Invalid hash '%s' specified\n", name);
        return false;
    }

    // printf("Testing hash %s\n", name);
    // printf("%u\n",hInfo->bits);

    // If you extend these statements by adding a new bitcount/type, you
    // need to adjust HASHTYPELIST in util/Instantiate.h also.
    if (hInfo->bits == 32) {
        return test<Blob<32>>(hInfo, flags);
    }
    if (hInfo->bits == 64) {
        return test<Blob<64>>(hInfo, flags);
    }
    if (hInfo->bits == 128) {
        return test<Blob<128>>(hInfo, flags);
    }
    if (hInfo->bits == 160) {
        return test<Blob<160>>(hInfo, flags);
    }
    if (hInfo->bits == 224) {
        return test<Blob<224>>(hInfo, flags);
    }
    if (hInfo->bits == 256) {
        return test<Blob<256>>(hInfo, flags);
    }

    printf("Invalid hash bit width %d for hash '%s'", hInfo->bits, hInfo->name);

    return false;
}

//-----------------------------------------------------------------------------

int main( int argc, const char ** argv ){

    const char * hashToTest  = "";

    if (argc < 2) {
        printf("No test hash given on command line\n");
        usage();
    }

    flags_t flags = FLAG_REPORT_PROGRESS;

    for (int argnb = 1; argnb < argc; argnb++) {

        const char * const arg = argv[argnb];

        if (strncmp(arg, "--", 2) == 0) {

            if (strcmp(arg, "--help") == 0) {
                usage();
                exit(0);
            }
            if (strcmp(arg, "--list") == 0) {
                listHashes(false);
                exit(0);
            }
            if (strcmp(arg, "--listnames") == 0) {
                listHashes(true);
                exit(0);
            }
            if (strcmp(arg, "--tests") == 0) {
                printf("Valid tests:\n");
                for (auto & g_testopt : g_testopts) {
                    printf("  %s\n", g_testopt.name);
                }
                exit(0);
            }
            if (strcmp(arg, "--version") == 0) {
                printf("BioLSHasher %s\n", VERSION);
                exit(0);
            }
            if (strcmp(arg, "--verbose") == 0) {
                flags |= FLAG_REPORT_VERBOSE;
                flags |= FLAG_REPORT_MORESTATS;
                flags |= FLAG_REPORT_DIAGRAMS;
                continue;
            }
            if (strncmp(arg, "--ncpu=", 7) == 0) {
                #if defined(HAVE_THREADS)
                                errno = 0;
                                char *   endptr;
                                long int Ncpu = strtol(&arg[7], &endptr, 0);
                                if ((errno != 0) || (arg[7] == '\0') || (*endptr != '\0') || (Ncpu < 1)) {
                                    printf("Error parsing cpu number \"%s\"\n", &arg[7]);
                                    exit(1);
                                }
                                // if (Ncpu > 32) { //In our machine lets not bound this.
                                //     printf("WARNING: limiting to 32 threads\n");
                                //     Ncpu = 32;
                                // }
                                g_NCPU = Ncpu;
                                continue;
                #else
                                printf("WARNING: compiled without threads; ignoring --ncpu\n");
                                continue;
                #endif
            }
            if (strncmp(arg, "--test=", 6) == 0) {
                // If a list of tests is given, only test those
                g_testAll = false;
                parse_tests(&arg[7], true);
                continue;
            }
            if (strncmp(arg, "--notest=", 8) == 0) {
                parse_tests(&arg[9], false);
                continue;
            }
            // invalid command
            printf("Invalid command \n");
            usage();
            exit(1);
        }
        // Not a command ? => interpreted as hash name
        hashToTest = arg;
    }

    // Check that at least one test was selected
    bool anyTestSelected = false;
    for (size_t i = 0; i < sizeof(g_testopts) / sizeof(TestOpts); i++) {
        if (g_testopts[i].var) {
            anyTestSelected = true;
            break;
        }
    }
    if (!anyTestSelected) {
        printf("Error: No test specified. Use --test=<testname> to select a test.\n");
        printf("Valid tests: ");
        for (size_t i = 0; i < sizeof(g_testopts) / sizeof(TestOpts); i++) {
            if (i > 0) printf(", ");
            printf("%s", g_testopts[i].name);
        }
        printf("\n");
        exit(1);
    }

    if (hashToTest[0] == '\0') {
        printf("Error: No hash name specified.\n");
        usage();
        exit(1);
    }

    bool result = testHash(hashToTest, flags);


    return 0;
}