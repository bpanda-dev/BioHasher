#pragma once
#include <cstdint>

// Global variables for LSH runtime communication
extern uint32_t gLSHTokenLength;
extern uint32_t gLSHNumSignatures;
extern bool gLSHTestActive;
extern const uint32_t gGoldenRatio;	

// Setter functions (called by LSHCollision test)
void SetLSHTokenLength(uint32_t length);
void SetLSHNumSignatures(uint32_t num);
void SetLSHTestActive(bool active);

// Getter functions (called by hash functions)
uint32_t GetLSHTokenLength();
uint32_t GetLSHNumSignatures();
bool IsLSHTestActive();