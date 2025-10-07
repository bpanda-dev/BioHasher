#include "LSHGlobals.h"

//Currently the num_signatures variable is not used anywhere. It can be useful in future
//if we want to test different number of signatures in Minhash or other LSH functions.


// Global variables for LSH runtime communication
int g_lsh_token_length = 13;        // Default token/kmer length
int g_lsh_num_signatures = 32;     // Default number of signatures	// Useful in Minhash
bool g_lsh_test_active = false;    // Whether LSH test is currently running

// Setter functions (called by LSHCollision test)
void set_lsh_token_length(int length) {
    g_lsh_token_length = length;
}

void set_lsh_num_signatures(int num) {
    g_lsh_num_signatures = num;
}

void set_lsh_test_active(bool active) {
    g_lsh_test_active = active;
}

// Getter functions (called by hash functions)
int get_lsh_token_length() {
    return g_lsh_token_length;
}

int get_lsh_num_signatures() {
    return g_lsh_num_signatures;
}

bool is_lsh_test_active() {
    return g_lsh_test_active;
}