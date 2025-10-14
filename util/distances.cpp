#include "distances.h"

// hammingdistance32o: Hamming distance for two byte arrays, output is 32-bit integer
uint32_t hammingdistance32o(uint8_t *a, uint8_t *b, uint32_t num_bytes) {

	uint32_t distance = 0;

	uint8_t *ptr_a = (uint8_t *)a;
	uint8_t *ptr_b = (uint8_t *)b;

	for (uint32_t i = 0; i < num_bytes; i++) {
		uint8_t byte_a = ptr_a[i];
		uint8_t byte_b = ptr_b[i];
		uint8_t xor_result = byte_a ^ byte_b;

		distance += __builtin_popcount(xor_result);
	}

	return distance;
}



// hammingdistance32i32o: Hamming distance for two 32-bit integers, output is 32-bit integer
uint32_t hammingdistance32i32o(uint32_t a, uint32_t b) {

	uint32_t distance = 0;

	uint32_t xor_result = a ^ b;
	distance = __builtin_popcount(xor_result);

	return distance;
}



// I have to send two arrays of integer type and number of elements.
float jaccardsimilarity(uint32_t *setA, uint32_t *setB, uint32_t setACardinality, uint32_t setBCardinality) {

	uint32_t intersectionCount = 0;
	uint32_t unionCount = 0;

	// Edge cases: if either set is empty
	if (setACardinality == 0 && setBCardinality == 0) {
		return 1.0; // Both sets are empty, define similarity as 1
	}
	// Another case of repetitive elements in the same set is not handled here.
	// Calculate intersection	(order of n^2, can be optimized with sorting or hashing)
	for (uint32_t i = 0; i < setACardinality; i++) {
		for (uint32_t j = 0; j < setBCardinality; j++) {
			if (setA[i] == setB[j]) {
				intersectionCount++;
				break; // Move to the next element in setA
			}
		}
	}

	// Calculate union
	unionCount = setACardinality + setBCardinality - intersectionCount;

	if (unionCount == 0) {
		return 0; // Avoid division by zero; define similarity as 0 when both sets are empty
	}

	// Jaccard similarity is the size of the intersection divided by the size of the union
	float jaccardSimilarity = (float)intersectionCount / unionCount;

	return jaccardSimilarity;
}