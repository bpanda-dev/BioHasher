#include "LSHGlobals.h"

/*
Currently the num_signatures variable is not used anywhere. It can be useful in future
if we want to test different number of signatures in Minhash or other LSH functions.
*/


// Global variables for runtime communication
uint32_t gTokenLength = 13;     // Default token/kmer length
// uint32_t gNumSignatures = 32;   // Default number of signatures	
bool gIsTestActive = false;     // Whether test is currently running

const uint32_t gGoldenRatio = 0x9e3779b1;	

// Setter functions (called by LSHCollision test)
void SetTokenLength(uint32_t length) {
    gTokenLength = length;
}

// void SetNumSignatures(uint32_t num) {
//     gNumSignatures = num;
// }

void SetIsTestActive(bool active) {
    gIsTestActive = active;
}

// Getter functions (called by hash functions)
uint32_t GetTokenLength() {
    return gTokenLength;
}

// uint32_t GetNumSignatures() {
//     return gNumSignatures;
// }

bool IsTestActive() {
    return gIsTestActive;
}