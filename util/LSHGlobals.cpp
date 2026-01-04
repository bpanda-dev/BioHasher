#include "LSHGlobals.h"

/*
Currently the num_signatures variable is not used anywhere. It can be useful in future
if we want to test different number of signatures in Minhash or other LSH functions.
*/


// Global variables for runtime communication
const uint32_t g_bincount_full = 4000;

uint32_t g_TokenLength = 0;     // Default token/kmer length
// uint32_t gNumSignatures = 32;   // Default number of signatures	
bool g_IsTestActive = false;     // Whether test is currently running

const uint32_t g_GoldenRatio = 0x9e3779b1;	

// Setter functions (called by LSHCollision test)
void SetTokenLength(uint32_t length) {
    g_TokenLength = length;
}

// void SetNumSignatures(uint32_t num) {
//     gNumSignatures = num;
// }

void SetIsTestActive(bool active) {
    g_IsTestActive = active;
}

// Getter functions (called by hash functions)
uint32_t GetTokenLength() {
    return g_TokenLength;
}

// uint32_t GetNumSignatures() {
//     return g_NumSignatures;
// }

bool IsTestActive() {
    return g_IsTestActive;
}