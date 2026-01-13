#pragma once
#include <cstdint>

// Global variables for runtime communication
extern uint32_t g_TokenLength;
// extern uint32_t g_NumSignatures;
extern bool g_IsTestActive;
extern const uint32_t g_GoldenRatio;	

extern const uint32_t g_bincount_full;

extern const uint32_t g_subseqHash1_subseq_len; // Default subsequence length for SubseqHash1 : this is k
extern const uint32_t g_subseqHash1_d;           // Default 'p' value for SubseqHash1	: this is d

// Setter functions (called by LSHCollision test)
void SetTokenLength(uint32_t length);
// void SetNumSignatures(uint32_t num);
void SetIsTestActive(bool active);

// Getter functions (called by hash functions)
uint32_t GetTokenLength();
// uint32_t GetNumSignatures();
bool IsTestActive();