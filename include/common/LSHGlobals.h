#pragma once
#include <cstdint>

// Global variables for runtime communication
extern uint32_t gTokenLength;
// extern uint32_t gNumSignatures;
extern bool gIsTestActive;
extern const uint32_t gGoldenRatio;	

// Setter functions (called by LSHCollision test)
void SetTokenLength(uint32_t length);
// void SetNumSignatures(uint32_t num);
void SetIsTestActive(bool active);

// Getter functions (called by hash functions)
uint32_t GetTokenLength();
// uint32_t GetNumSignatures();
bool IsTestActive();