/*
 * BioHasher
 * Copyright (C) 2025 IISc
 * Copyright (C) 2021-2023  Frank J. T. Wojcik
 * Copyright (C) 2023       jason
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *     Copyright (C) 2021-2023 Frank J. T. Wojcik
 *     Copyright (C) 2023      jason
 *     Copyright (c) 2010-2012 Austin Appleby
 *     Copyright (c) 2019-2021 Reini Urban
 *     Copyright (c) 2019      Yann Collet
 *
 *     Permission is hereby granted, free of charge, to any person
 *     obtaining a copy of this software and associated documentation
 *     files (the "Software"), to deal in the Software without
 *     restriction, including without limitation the rights to use,
 *     copy, modify, merge, publish, distribute, sublicense, and/or
 *     sell copies of the Software, and to permit persons to whom the
 *     Software is furnished to do so, subject to the following
 *     conditions:
 *
 *     The above copyright notice and this permission notice shall be
 *     included in all copies or substantial portions of the Software.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *     OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *     NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *     HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *     WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *     FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *     OTHER DEALINGS IN THE SOFTWARE.
 */

#include "Platform.h"
#include "Hashinfo.h"
#include "TestGlobals.h"
#include "Random.h"
#include "Analyze.h"
#include "Instantiate.h"
#include "VCode.h"
#include "distances.h"
#include "BioDataGeneration.h"
#include "LSHGlobals.h"
#include "LSHCollisionTest.h"
#include "fstream"
#include <iostream>

/*
Note, the keybits influences the sequence length for bio sequences.
For example, for DNA sequences with 2 bits per base:
keybits = 32 -> sequence length = 16 bases
keybits = 64 -> sequence length = 32 bases
For now, we have fixed the number of keybits to be multiple of 8.
*/

/*-------------------------------------------------------------------------------*/
/*									Collision Test		 						 */
/*-------------------------------------------------------------------------------*/

// 32 bits
std::vector<float> similarity_x_A = {0.0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0};
std::vector<float> similarity_x_B = {0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.90,0.91,0.92,0.93,0.94,0.95,0.96,0.97,0.98,0.99,1.0};
std::vector<float> similarity_x_C = {0.80,0.81,0.82,0.83,0.84,0.85,0.86,0.87,0.88,0.89,0.90,0.91,0.92,0.93,0.94,0.95,0.96,0.97,0.98,0.99,1.0}; // from 0.8 to 1.0
// std::vector<float> sim_vector_step_twentieth_from_098 = {0.980,0.981,0.982,0.983,0.984,0.985,0.986,0.987,0.988,0.989,0.990,0.991,0.992,0.993,0.994,0.995,0.996,0.997,0.998,0.999,1.0};
std::vector<float> sim_vector_step_tenth = similarity_x_B;//{0.0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0};
// {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};//


//TODO: Add support for Levenshtein distance class in the future.
uint32_t setDistanceClassForHashInfo(const HashInfo * hinfo) {
	// Determine the distance class based on the hash function's properties
	if (hinfo->hash_flags & FLAG_HASH_HAMMING_DISTANCE) {
		return 1U; // Hamming distance
	} 
	else if (hinfo->hash_flags & FLAG_HASH_JACCARD_SIMILARITY) {
		return 2U; // Jaccard distance
	}
	else {
		return 0U; // Default or unknown or distance not supported.
	}
}

void FillDistanceVectorFromSimilarity(const std::vector<float>& simrates, uint32_t sequenceLength, uint32_t distanceClass, std::vector<uint32_t>& distances) {
	
	distances.resize(simrates.size());
	uint32_t *p = distances.data();	// pointer to the distances array data
	
	if(distanceClass == 1){ // Hamming distance
		for (size_t i = 0; i < simrates.size(); i++) {
			uint32_t dist = static_cast<uint32_t>((1.0f - simrates[i]) * static_cast<float>(sequenceLength));
			p[i] = dist;
			printf("Simrate: %.4f, Hamming Distance: %u\n", simrates[i], p[i]);
		}
	}
	
	else if(distanceClass == 2){ // Jaccard distance
		int tokenlength = GetLSHTokenLength();  // Get runtime token length
		assert(tokenlength > 0 && "Token length must be greater than 0 for Jaccard distance calculation.");

		for (size_t i = 0; i < simrates.size(); i++) {
			assert(simrates[i]>=0.0f && simrates[i]<=1.0f);
			// For Jaccard similarity, distance = 2*(1 - similarity) / (1 + similarity)
			// Here we scale it by sequence length to get an approximate count of differing elements
			uint32_t dist = (static_cast<uint32_t>(  ((2.0f * (1.0f - simrates[i])) / (1.0f + simrates[i])) * (static_cast<float>(sequenceLength)/static_cast<float>(tokenlength))));
			p[i] = dist & ~1U; // Make it even. Its always even.
			// printf("Simrate: %.4f, Jaccard Distance: %u\n", simrates[i], p[i]);
		}
	}
	
	else{	// Default case: Hamming distance
		for (size_t i = 0; i < simrates.size(); i++) {
			uint32_t dist = static_cast<uint32_t>((1.0f - simrates[i]) * static_cast<float>(sequenceLength));
			p[i] = dist;
		}
		printf("Distance class %u not implemented yet!\n", distanceClass);
	}
}

template <typename hashtype>
bool LSHCollisionTestImplInner(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, const std::vector<uint32_t>& distances, HashFn hash, std::vector<std::vector<float>>& collisionRateVec, const seed_t seed) {
	
	// Implementation for Hamming distance based LSH collision test.
	// This function will generate data, mutate it, compute hashes, and record collision rates.

	uint32_t distanceIdx = 0;
	uint32_t trialIdx = 0;

	/*-------------------------------------------------------------------------------*/
	/*							Seeding the rand functions							 */
	/*-------------------------------------------------------------------------------*/
	seed_t dataGenSeed = (seed*3) + 31*gGoldenRatio; // Seed for data generation
	seed_t dataGenSeedOffset = 3; // Offset to change the seed for data generation

	seed_t MutationSeed = (seed*7) + 33*gGoldenRatio; // Seed for mutation
	seed_t MutationSeedOffset = 7; //  offset to change the seed for mutation

	seed_t HashFamilySeed = seed;
	seed_t HashFamilySeedOffset = gGoldenRatio;

	// printf("????? %d\n", sequenceRecordsWithMetadata->KeyCount);
	/*-------------------------------------------------------------------------------*/
	/*							LSH Collision Test Logic							 */
	/*-------------------------------------------------------------------------------*/
	for(distanceIdx=0; distanceIdx < distances.size(); distanceIdx++){
		
		float a_percentage = 0.25f;
		float c_percentage = 0.25f;
		float g_percentage = 0.25f;
		float t_percentage = 0.25f;
		assert((a_percentage + c_percentage + g_percentage + t_percentage) == 1.0f);

		/* Generate the samples for each dissimilarity index */
		sequenceRecordsWithMetadata->A_percentage = a_percentage;
		sequenceRecordsWithMetadata->C_percentage = c_percentage;
		sequenceRecordsWithMetadata->G_percentage = g_percentage;
		sequenceRecordsWithMetadata->T_percentage = t_percentage;
		
		DataGeneration dataGen(sequenceRecordsWithMetadata, dataGenSeed);	// Constructor to initialize data generation parameters.
		dataGen.FillRecords(sequenceRecordsWithMetadata);		// Filling the records.
		
		dataGenSeed += dataGenSeedOffset;
		
		// Note: Please do not delete the following comment under any circumstance! Explaination for certain design choices is present here.
		/* At this level, we should not be able to see how we are changing the values and what is the mutated sequence.*/
		// Introduction of mutations to sequence requires knowledge of distance class. 
		// For case(1): Hamming distance requires that the mutations be substitutions only.
		// For case(2): There will be two types of abstract visualisation of the input sequences, either they can be seen as sequences or a concatenation of tokens (for Jaccard).
		// For eg: ATGCTGATCGGGCAGC can be seen as a sequence of length 16 or as a set of tokens of length 4: {ATGC, TGAT, CGGG, CAGC}.
		// The above two case justifies the need of distance class information at this level.
		// Need to add more classes here. 


		bool isMutationRate = false;
		MutationEngine mutationEngine(sequenceRecordsWithMetadata->DistanceClass, distances[distanceIdx], isMutationRate);	// Initialising mutation engine with absolute edit distance.
		mutationEngine.MutateRecords(sequenceRecordsWithMetadata, distances[distanceIdx], MutationSeed);

		// if(sequenceRecordsWithMetadata->DistanceClass == 1){
		// 	printf("Mutating records for Hamming distance: %u\n", distances[distanceIdx]);
		// }
		// else if(sequenceRecordsWithMetadata->DistanceClass == 2){
		// 	printf("Mutating records for Jaccard distance: %u\n", distances[distanceIdx]);
		// }
		
		MutationSeed += MutationSeedOffset;

		for(trialIdx=0; trialIdx < sequenceRecordsWithMetadata->KeyCount; trialIdx++){
			float collisionCount = 0;
			float collisionRate = 0;
			seed_t currentHashSeed = HashFamilySeed + (HashFamilySeedOffset * trialIdx);
			// printf("Trial %u: Hash Family Seed: %llu, bit vector Length in bytes: %zu\n", trialIdx, (unsigned long long)currentHashSeed, sequenceRecordsWithMetadata->Records[trialIdx].SeqTwoBitOrg.size());
			ExtBlob k1(&(sequenceRecordsWithMetadata->Records[trialIdx].SeqTwoBitOrg[0]), sequenceRecordsWithMetadata->Records[trialIdx].SeqTwoBitOrg.size());
			ExtBlob k2(&(sequenceRecordsWithMetadata->Records[trialIdx].SeqTwoBitMut[0]), sequenceRecordsWithMetadata->Records[trialIdx].SeqTwoBitMut.size());

			for (unsigned i = 0; i < sequenceRecordsWithMetadata->HashCount; i++) {
				hashtype hashOrg;
				hashtype hashMut;

				seed_t currentSeed = currentHashSeed + (HashFamilySeedOffset * i); // Different seed per hash

				hash(k1, sequenceRecordsWithMetadata->Records[trialIdx].SeqTwoBitOrg.size(), currentSeed, &hashOrg);
				hash(k2, sequenceRecordsWithMetadata->Records[trialIdx].SeqTwoBitMut.size(), currentSeed, &hashMut);

				if(hashOrg == hashMut){
					collisionCount++;
				}
			}
			collisionRate = collisionCount / static_cast<float>(sequenceRecordsWithMetadata->HashCount);
			collisionRateVec[distanceIdx][trialIdx] = collisionRate;
			// printf("DistanceIdx: %u, TrialIdx: %u, Collision Rate: %.4f\n", distanceIdx, trialIdx, collisionRate);
		}
	}

	// printf("LSHCollisionTestImplHamming called with sequenceLength=%u, keycount=%u\n", sequenceRecordsWithMetadata->OriginalSequenceLength, sequenceRecordsWithMetadata->KeyCount);
	return true;
}

template <typename hashtype>
static bool LSHCollisionTestImpl( const HashInfo * hinfo, unsigned keybits, flags_t flags, std::ofstream &out_file) {

	bool result = true;	//TODO: Update this based on test results.


	const uint32_t seqLen = keybits/2;					// Length of the sequence to be generated.
	const unsigned keybytes = ((keybits + 8 - 1) / 8);	// Number of bytes in the key. Note that the keybits have to be a multiple of 8.
	assert(keybits % 8 == 0 && "Keybits should be multiples of 8."); // Ensure keybits is a multiple of 8.

	const int tokenlength = GetLSHTokenLength();  		// Get runtime token length

	HashFn hash = hinfo->hashFn(g_hashEndian);

	const seed_t seed = hinfo->Seed(g_seed);

	const unsigned keycount = 100; //000; //100000; //1024; //512 * 1024 * ((hinfo->bits <= 64) ? 3 : 4);   // Number of keys to generate and test.
	
    const size_t hashcount = 100; //000; //1024;	// Number of hashes(from the hash family) to compute per key

	// File header
	out_file << ":1:LSH Collision Test Results\n";
	out_file << ":2:" << "Hashname," << "Keybits," << "Tokenlength" << std::endl;
	out_file << ":3:" << hinfo->name << "," << keybits << "," << tokenlength << std::endl;
    
	if (!REPORT(VERBOSE, flags)) {
		printf("LSH Collision Test: Key Size = %3u bits (%2u bytes), Keys = %8u, Hashes per Key = %4zu\n",
           keybits, (unsigned)keybytes, keycount, hashcount);

		printf("Hash Function: %s\n", hinfo->name);
		printf("Hash Bits: %u\n", hinfo->bits);
    }

	SequenceRecordsWithMetadataStruct sequenceRecordsWithMetadata;

	sequenceRecordsWithMetadata.OriginalSequenceLength = seqLen;
	sequenceRecordsWithMetadata.DistanceClass = setDistanceClassForHashInfo(hinfo);		//TODO: Add more distance classes.
	sequenceRecordsWithMetadata.KeyCount = keycount;
	sequenceRecordsWithMetadata.HashCount = hashcount;

	/*-------------------------------------------------------------------------------*/
	/*						Similarity and Distance Computation						 */
	/*-------------------------------------------------------------------------------*/
	printf("Using Distance Class: %u\n", sequenceRecordsWithMetadata.DistanceClass);
	
	/*Similarity and Distance vector preparation*/
	std::vector<float> simrates = sim_vector_step_tenth;
	
	printf("Similarity rates: ");
	for (size_t i = 0; i < simrates.size(); i++) {
		printf("%.2f ", simrates[i]);
	}
	printf("\n");

	/* Based on the distance type, similarity may have different values. so here will be compute that distance vector.*/
	std::vector<uint32_t> distances;
	FillDistanceVectorFromSimilarity(simrates, seqLen, sequenceRecordsWithMetadata.DistanceClass, distances);
	
	printf("\nCorresponding distances (for sequence length %u): ", seqLen);
	for (size_t i = 0; i < distances.size(); i++) {
		printf("%d ", distances[i]);
	}
	printf("\n");

	
	// fprint the simrates in one line in the output file.
	out_file << ":4:" ;
	for (size_t i = 0; i < simrates.size(); i++) {
		if (i == simrates.size() - 1)
			out_file << simrates[i] << "\n";
		else
			out_file << simrates[i] << ",";
	}

	// fprint the distances in one line in the output file.
	out_file << ":5:" ;
	for (size_t i = 0; i < distances.size(); i++) {
		if (i == distances.size() - 1)
			out_file << distances[i] << "\n";
		else
			out_file << distances[i] << ",";
	}

	out_file << ":6: For metadata. token length determines the number of distinct tokens. Need to focus\n"; 
	

	/*-------------------------------------------------------------------------------*/

	// An empty 2d vector to store collisions <-- This array will be printed to the file for plotting.
	std::vector<std::vector<float>> collisionRateVec(simrates.size(), std::vector<float>(keycount, 0.0f));

	/*Data generation: Each distance metric needs a different way of generation of input data.*/

	LSHCollisionTestImplInner<hashtype>(&sequenceRecordsWithMetadata, distances, hash, collisionRateVec, seed);

	// Print the collision rates to the output file.
	uint32_t distanceIdx = 0;
	uint32_t trialIdx = 0;
	for(distanceIdx=0; distanceIdx < distances.size(); distanceIdx++){		
		for(trialIdx=0; trialIdx < keycount; trialIdx++){
			if(trialIdx == keycount-1)
				out_file << collisionRateVec[distanceIdx][trialIdx] << "\n";
			else
				out_file << collisionRateVec[distanceIdx][trialIdx] << ",";
		}
	}

    
    return result;	//TODO: For now, the result is always true. We need to add logic to find where the test fails.
}


//----------------------------------------------------------------------------//
template <typename hashtype>
bool LSHCollisionTest( const HashInfo * hinfo, bool extra, flags_t flags ) {
	
	std::ofstream out_file("../results/collisionResults_" + std::string(hinfo->name)  +".csv");
	if (!out_file.is_open()) {
		std::cerr << "Error: Could not open output file" << std::endl;
		exit(EXIT_FAILURE);
	} 
    
	if(extra){
		printf("Extra flag is set. Running extended tests where applicable.\n");
	}

    bool result = true;

    printf("[[[ LSH Collision Tests ]]]\n\n");

	// A token of length toklen will produce keys of length 2*token bits for DNA sequences (2 bits per base).
	// If the hash has tokenisation property, then we need to test for multiple token lengths
	// Otherwise, we just use a token length of 0 (no tokenisation)
	// This is because the LSH collision test is primarily designed for tokenised hashes like minhash and simhash
	// which have the tokenisation property.
	// For other hashes, we just use a token length of 0 (i.e. no tokenisation).
	std::vector<uint32_t> tokenlengths;
	if(hinfo->hasTokenisationProperty()){
		printf("Hash %s has tokenisation property. Testing multiple token lengths.\n", hinfo->name);
		tokenlengths = {4 ,7, 13, 21, 31, 33}; //create_tokens();
	}
	else{
		tokenlengths = {0}; // No tokenization
	}

	std::vector<uint32_t> sequenceLengths = {16, 24, 32, 48, 64, 80, 96, 128, 256, 512, 1024, 2048, 4096, 8192}; // Corresponding sequence lengths for DNA (2 bits per base)

	for(const auto & toklen : tokenlengths){
		printf("=====================================\n");
		printf("Token length: %d\n", toklen);
		printf("=====================================\n");
		
		// Set LSH global variables for this token length
		SetLSHTestActive(true);
		SetLSHTokenLength(toklen > 0 ? toklen : 0);  // Use token length or default	// This is a redundant check but added for safety.
		
		for(const auto & seqLen : sequenceLengths){
			printf("-------------------------------------\n");
			printf("Sequence length: %u\n", seqLen);
			printf("-------------------------------------\n");
			// For DNA sequences, keybits = 2 * sequence length
			uint32_t keybits = seqLen * 2;	// Eg: Keybits = 32 Sequence length = 16
			
			
			//------------------------------ Need reworking ------------------------------//
			// --- Sanity Checks for Test Configuration Only valid for tokenised hashes ---	//
			if(hinfo->hasTokenisationProperty()){
				// Condition 1: Skipping if token length is greater than sequence length.
				if (toklen > seqLen) {
					printf("Skipping: Token length (%u) > sequence length (%u).\n", toklen, seqLen);
					continue;
				}

				// Calculate the number of tokens the sequence will be split into.
				const uint64_t tokensInSequence = (seqLen + toklen - 1) / toklen; // Ceiling division	//Cardinality
				// Condition 2: Skipping if the sequence is too short to provide enough tokens for meaningful mutation analysis and avoid natural repetition.
				const uint64_t MIN_TOKENS_FOR_MUTATION = 20; // A more reasonable threshold.
				if (tokensInSequence < MIN_TOKENS_FOR_MUTATION) {
					printf("Skipping: Sequence with %llu tokens is too short (min is %llu).\n", (unsigned long long)tokensInSequence, (unsigned long long)MIN_TOKENS_FOR_MUTATION);
					continue;
				}

				// Condition 3: Skipping if the token space is too small, which would lead to high collision rates
				// even for random data, making the LSH property test less meaningful.
				if (toklen > 0) {
					uint64_t maxPossibleTokens = (toklen >= 32) ? UINT64_MAX : (1ULL << (2 * toklen));
					
					// This checks if the number of tokens in the sequence is a large fraction of the total possible unique tokens.
					// For example, skip if tokensInSequence > 1% of maxPossibleTokens.
					if (maxPossibleTokens / 100 < tokensInSequence) {
						printf("Skipping: High token repetition expected. Tokens in sequence (%llu) vs. max possible (%llu).\n", (unsigned long long)tokensInSequence, (unsigned long long)maxPossibleTokens);
						continue;
					}
				}
			}
			//------------------------------ Need reworking ------------------------------//

			printf("\nTesting hash: %s with keybits: %u (sequence length: %u), token length: %d\n", hinfo->name, keybits, seqLen, toklen);
			result &= LSHCollisionTestImpl<hashtype>(hinfo, keybits, flags, out_file);
		}
	}

    // Cleanup: Reset LSH global variables after test completion
    SetLSHTestActive(false);

    printf("%s\n", result ? "" : g_failstr);
	out_file.close();
    return result;
}

INSTANTIATE(LSHCollisionTest, HASHTYPELIST);