#include "LSHGlobals.h"

//Currently the num_signatures variable is not used anywhere. It can be useful in future
//if we want to test different number of signatures in Minhash or other LSH functions.


// Global variables for LSH runtime communication
uint32_t gLSHTokenLength = 13;        // Default token/kmer length
uint32_t gLSHNumSignatures = 32;     // Default number of signatures	// Useful in Minhash
bool gLSHTestActive = false;    // Whether LSH test is currently running

const uint32_t gGoldenRatio = 0x9e3779b1;	

// Setter functions (called by LSHCollision test)
void SetLSHTokenLength(uint32_t length) {
    gLSHTokenLength = length;
}

void SetLSHNumSignatures(uint32_t num) {
    gLSHNumSignatures = num;
}

void SetLSHTestActive(bool active) {
    gLSHTestActive = active;
}

// Getter functions (called by hash functions)
uint32_t GetLSHTokenLength() {
    return gLSHTokenLength;
}

uint32_t GetLSHNumSignatures() {
    return gLSHNumSignatures;
}

bool IsLSHTestActive() {
    return gLSHTestActive;
}