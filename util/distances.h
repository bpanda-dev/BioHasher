#pragma once
// #include "Platform.h"
#include <cstdint>

// Naming convention: functionname_inputsize_outputsize

uint32_t hammingdistance32o(uint8_t *a, uint8_t *b, uint32_t num_bytes);

uint32_t hammingdistance32i32o(uint32_t a, uint32_t b);
uint64_t hammingdistance32i32o(uint64_t a, uint64_t b);



float jaccardsimilarity(uint32_t *setA, uint32_t *setB, uint32_t setACardinality, uint32_t setBCardinality);

