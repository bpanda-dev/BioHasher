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

#include "LSHGlobals.h"
#include "LSHCollisionTestAutoDist.h"
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
std::vector<float> sim_vector_step_tenth = similarity_x_A;

uint32_t LESS_RECORDS_THRESHOLD = 10; // Threshold to decide if there are less records in a bin

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


// struct BoxPlotStats {
//     float minimum;
//     float q1;           // 25th percentile
//     float median;       // 50th percentile
//     float q2;           // Same as median
//     float q3;           // 75th percentile
//     float maximum;
//     float lowerWhisker; // With outlier detection
//     float upperWhisker; // With outlier detection
//     float iqr;          // Interquartile range
// };
// BoxPlotStats computeBoxPlotStats(std::vector<float> values) {
//     BoxPlotStats stats;
    
//     if (values.empty()) {
//         // Return zeros for empty data
//         return stats;
//     }
    
//     // Sort the values
//     std::sort(values.begin(), values.end());
    
//     size_t n = values.size();
    
//     // 1. Minimum and Maximum
//     stats.minimum = values[0];
//     stats.maximum = values[n - 1];
    
//     // 2. Median (Q2, 50th percentile)
//     if (n % 2 == 0) {
//         stats.median = (values[n/2 - 1] + values[n/2]) / 2.0f;
//     } else {
//         stats.median = values[n/2];
//     }
//     stats.q2 = stats.median;
    
//     // 3. Q1 (25th percentile)
//     size_t q1_idx = n / 4;
//     if (n % 4 == 0) {
//         stats.q1 = (values[q1_idx - 1] + values[q1_idx]) / 2.0f;
//     } else {
//         stats.q1 = values[q1_idx];
//     }
    
//     // 4. Q3 (75th percentile)
//     size_t q3_idx = (3 * n) / 4;
//     if ((3 * n) % 4 == 0) {
//         stats.q3 = (values[q3_idx - 1] + values[q3_idx]) / 2.0f;
//     } else {
//         stats.q3 = values[q3_idx];
//     }
    
//     // 5. IQR (Interquartile Range)
//     stats.iqr = stats.q3 - stats.q1;
    
//     // 6. Whiskers (with outlier detection)
//     float lowerBound = stats.q1 - 1.5f * stats.iqr;
//     float upperBound = stats.q3 + 1.5f * stats.iqr;
    
//     // Find actual whisker values (first/last value within bounds)
//     stats.lowerWhisker = stats.minimum;
//     for (float val : values) {
//         if (val >= lowerBound) {
//             stats.lowerWhisker = val;
//             break;
//         }
//     }
    
//     stats.upperWhisker = stats.maximum;
//     for (auto it = values.rbegin(); it != values.rend(); ++it) {
//         if (*it <= upperBound) {
//             stats.upperWhisker = *it;
//             break;
//         }
//     }
    
//     return stats;
// }

//----------------------------------------------------------------------------//
template <typename hashtype>
bool LSHCollisionTestImplInner(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata, HashFn hash, const seed_t seed, std::ofstream &out_file) {
	printf("LSHCollisionTestImplInner: Generating sequences and computing collisions...\n");

	// This function will generate data, mutate it, compute hashes, and record collision rates.
	uint32_t binIdx = 0;
	uint32_t trialIdx = 0;

	

		/*-------------------------------------------------------------------------------*/
		/*							Seeding the rand functions							 */
		/*-------------------------------------------------------------------------------*/
		// 	seed_t dataGenSeed = (seed*3) + 31*gGoldenRatio; // Seed for data generation
		// 	seed_t dataGenSeedOffset = 3; // Offset to change the seed for data generation

		// 	seed_t MutationSeed = (seed*7) + 33*gGoldenRatio; // Seed for mutation
		// 	seed_t MutationSeedOffset = 7; //  offset to change the seed for mutation


		// printf("????? %d\n", sequenceRecordsWithMetadata->KeyCount);
		
		/*-------------------------------------------------------------------------------*/
		/*							LSH Collision Test Logic							 */
		/*-------------------------------------------------------------------------------*/
		
		assert((sequenceRecordsWithMetadata->A_percentage + sequenceRecordsWithMetadata->C_percentage + sequenceRecordsWithMetadata->G_percentage + sequenceRecordsWithMetadata->T_percentage) == 1.0f);
		SequenceDataGenerator dataGen(sequenceRecordsWithMetadata);	// Constructor to initialize data generation parameters.
		dataGen.FillRecords(sequenceRecordsWithMetadata);		// Filling the records.


		// Here we select the model from the set of models and vary the mutation rates in the parameter space.
		//--------------------
		float snpRates[21] = {0.01,0.05,0.10,0.15,0.20,0.25,0.30,0.35,0.40,0.45,0.50,0.55,0.60,0.65,0.70,0.75,0.80,0.85,0.90,0.95,0.99}; 
		float indelRates[21] = {0.0,0.0, 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
		//--------------------
		size_t length_paramsweep = sizeof(snpRates)/sizeof(snpRates[0]);	// Only for hamming
		
		for(uint32_t paramsweep_i = 0; paramsweep_i < length_paramsweep; paramsweep_i++){
			sequenceRecordsWithMetadata->snpRate = snpRates[paramsweep_i];
			sequenceRecordsWithMetadata->smallIndelRate = 0; 	//0.005;
			sequenceRecordsWithMetadata->minSmallIndelSize = 2;
			sequenceRecordsWithMetadata->maxSmallIndelSize = 5;

			out_file << ":5:" << sequenceRecordsWithMetadata->snpRate << "," << sequenceRecordsWithMetadata->smallIndelRate << "," << sequenceRecordsWithMetadata->minSmallIndelSize << "," << sequenceRecordsWithMetadata->maxSmallIndelSize << std::endl;

			SequenceDataMutator dataMutate(sequenceRecordsWithMetadata);
			dataMutate.ApplyMutations(sequenceRecordsWithMetadata);

			SequenceDataMetrics dataMutateMetrics(sequenceRecordsWithMetadata);
			dataMutateMetrics.ComputeHammingMetrics(sequenceRecordsWithMetadata);
			
			auto bins = dataMutateMetrics.BinRecords(sequenceRecordsWithMetadata, 1, sequenceRecordsWithMetadata->binstart, sequenceRecordsWithMetadata->binend, sequenceRecordsWithMetadata->binsize);
			dataMutateMetrics.printBinStatistics(bins, 1);

			printf("Bin Count: %d\n", sequenceRecordsWithMetadata->bincount);

			// For each bin, compute collision rates
			for(binIdx=0; binIdx < sequenceRecordsWithMetadata->bincount; binIdx++){		
				uint32_t recordsInBin = bins[binIdx].count;
				if(recordsInBin < 10) {
					// Not enough records to compute collisions
					continue;
				}
				
				//create a new array to hold collision rates for each record in the bin
				std::vector<float> BinCollisionRate(recordsInBin, 0.0f);

				printf("Processing Bin %u with %u records...\n", binIdx, recordsInBin);

				// For each record in the bin, compute hashes and check for collisions
				for(trialIdx=0; trialIdx < recordsInBin; trialIdx++){
					
					SequenceRecordUnit &record = sequenceRecordsWithMetadata->Records[bins[binIdx].recordIndices[trialIdx]];
					float collisionCount = 0.0f;
					float collisionRate = 0.0f;
					
					ExtBlob k1(&(record.SeqTwoBitOrg.data()[0]), record.OriginalLength);
					ExtBlob k2(&(record.SeqTwoBitMut.data()[0]), record.MutatedLength);

					// printf(">>>>>>>> %d",sequenceRecordsWithMetadata->HashCount);

					for(uint32_t hash_i = 0; hash_i < sequenceRecordsWithMetadata->HashCount; hash_i++){
						
						// Compute hash
						hashtype hashsignatureOrg;
						hashtype hashsignatureMut;
						seed_t currentSeed = seed ^ (hash_i * 0x9e3779b9);//seed + hash_i;
						// printf("Current Seed: %u\n", currentSeed);
						hash(k1, record.OriginalLength, currentSeed, &hashsignatureOrg);
						hash(k2, record.MutatedLength, currentSeed, &hashsignatureMut);
						
						if(hashsignatureOrg == hashsignatureMut){
							collisionCount++;
						}
					}
					// Compute collision rate for this record
					collisionRate = static_cast<float>(collisionCount) / static_cast<float>(sequenceRecordsWithMetadata->HashCount);
					BinCollisionRate[trialIdx] = collisionRate;
				}

				float avgCollisionRate = 0.0f;
				for (const auto& rate : BinCollisionRate) {
					avgCollisionRate += rate;
				}
				avgCollisionRate /= static_cast<float>(BinCollisionRate.size());

				out_file << ":6:" << binIdx << ": " << avgCollisionRate << std::endl;
				
				printf("Bin#%u: Average Collision Rate = %.4f\n", binIdx, avgCollisionRate);
				
				// BoxPlotStats trailstats = computeBoxPlotStats(BinCollisionRate);
				// collisionRateVec[binIdx][0] = avgCollisionRate; //trailstats.median;
			}
		}

		// SequenceDataMutator dataMutate(sequenceRecordsWithMetadata);
		// dataMutate.ApplyMutations(sequenceRecordsWithMetadata);

		// SequenceDataMetrics dataMutateMetrics(sequenceRecordsWithMetadata);
		// dataMutateMetrics.ComputeHammingMetrics(sequenceRecordsWithMetadata);
		// dataMutateMetrics.ComputeJaccardMetrics(sequenceRecordsWithMetadata, 7);
		// dataMutateMetrics.ComputeEditDistanceMetrics(sequenceRecordsWithMetadata);
		// dataMutateMetrics.ComputeGlobalAlignmentMetrics(sequenceRecordsWithMetadata);
		// dataMutateMetrics.ComputeLocalAlignmentMetrics(sequenceRecordsWithMetadata);

		// auto bins = dataMutateMetrics.BinRecords(sequenceRecordsWithMetadata, 1, sequenceRecordsWithMetadata->binstart, sequenceRecordsWithMetadata->binend, sequenceRecordsWithMetadata->binsize);
		// dataMutateMetrics.printBinStatistics(bins, 1);

		// auto bins = dataMutateMetrics.BinRecords(sequenceRecordsWithMetadata, 2, sequenceRecordsWithMetadata->binstart, sequenceRecordsWithMetadata->binend, sequenceRecordsWithMetadata->binsize);
		// dataMutateMetrics.printBinStatistics(bins, 2);
		
		// // For each bin, compute collision rates
		// for(binIdx=0; binIdx < sequenceRecordsWithMetadata->bincount; binIdx++){		
		// 	uint32_t recordsInBin = bins[binIdx].count;
		// 	if(recordsInBin < 10) {
		// 		// Not enough records to compute collisions
		// 		continue;
		// 	}
		// 	//create a new array to hold collision rates for each record in the bin
		// 	std::vector<float> BinCollisionRate(recordsInBin, 0.0f);

		// 	// For each record in the bin, compute hashes and check for collisions
		// 	for(trialIdx=0; trialIdx < recordsInBin; trialIdx++){
				
		// 		SequenceRecordUnit &record = sequenceRecordsWithMetadata->Records[bins[binIdx].recordIndices[trialIdx]];
		// 		float collisionCount = 0.0f;
		// 		float collisionRate = 0.0f;
				
		// 		ExtBlob k1(&(record.SeqTwoBitOrg.data()[0]), record.OriginalLength);
		// 		ExtBlob k2(&(record.SeqTwoBitMut.data()[0]), record.MutatedLength);

		// 		for(uint32_t hash_i = 0; hash_i < sequenceRecordsWithMetadata->HashCount; hash_i++){
		// 			// Compute hash
		// 			hashtype hashsignatureOrg;
		// 			hashtype hashsignatureMut;
		// 			seed_t currentSeed = seed + hash_i;
		// 			hash(k1, record.OriginalLength, currentSeed, &hashsignatureOrg);
		// 			hash(k2, record.MutatedLength, currentSeed, &hashsignatureMut);
					
		// 			if(hashsignatureOrg == hashsignatureMut){
		// 				collisionCount++;
		// 			}
		// 		}
		// 		// Compute collision rate for this record
		// 		collisionRate = static_cast<float>(collisionCount) / static_cast<float>(sequenceRecordsWithMetadata->HashCount);
		// 		BinCollisionRate[trialIdx] = collisionRate;
		// 	}
			
			
		// 	float avgCollisionRate = 0.0f;
		// 	for (const auto& rate : BinCollisionRate) {
		// 		avgCollisionRate += rate;
		// 	}
		// 	avgCollisionRate /= static_cast<float>(BinCollisionRate.size());
		// 	printf("Bin %u: Average Collision Rate = %.4f\n", binIdx, avgCollisionRate);
			
		// 	// BoxPlotStats trailstats = computeBoxPlotStats(BinCollisionRate);

		// 	collisionRateVec[binIdx][0] = avgCollisionRate; //trailstats.median;
		// 	// collisionRateVec[binIdx][1] = avgCollisionRate; //trailstats.q1;
		// 	// collisionRateVec[binIdx][2] = avgCollisionRate; //trailstats.q3;
		// 	// collisionRateVec[binIdx][3] = avgCollisionRate; //trailstats.minimum;
		// 	// collisionRateVec[binIdx][4] = avgCollisionRate; //trailstats.maximum;
		// 	// collisionRateVec[binIdx][5] = avgCollisionRate; //trailstats.lowerWhisker;
		// 	// collisionRateVec[binIdx][6] = avgCollisionRate; //trailstats.upperWhisker;
		// 	// collisionRateVec[binIdx][7] = avgCollisionRate; //trailstats.iqr;

		// 	// |
		// 	// |    ← Maximum (or upper whisker)
		// 	// ┬
		// 	// │
		// 	// ├───┐
		// 	// │   │  ← Q3 (75th percentile)
		// 	// │   │
		// 	// ├───┤  ← Median (Q2, 50th percentile)
		// 	// │   │
		// 	// │   │  ← Q1 (25th percentile)
		// 	// └───┘
		// 	// │
		// 	// ┴
		// 	// |    ← Minimum (or lower whisker)
		// 	// |

		// }
	return true;	//TODO: Update this based on test results.
}

template <typename hashtype>
static bool LSHCollisionTestImpl( const HashInfo * hinfo, unsigned keybits, flags_t flags, std::ofstream &out_file) {

	bool result = true;	//TODO: Update this based on test results.


	const uint32_t seqLen = keybits/8;					// Length of the sequence to be generated.
	const unsigned keybytes = seqLen;					// Key size in bytes.

	const int tokenlength = GetLSHTokenLength();  		// Get runtime token length

	HashFn hash = hinfo->hashFn(g_hashEndian);

	const seed_t seed = hinfo->Seed(g_seed);	//The hash seed. Should we use it to generate data?

	const unsigned keycount = 10000; //000; //100000; //1024; //512 * 1024 * ((hinfo->bits <= 64) ? 3 : 4);   // Number of keys to generate and test.
	
    const size_t hashcount = 2; //000; //1024;	// Number of hashes(from the hash family) to compute per key

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


	SequenceRecordsWithMetadata sequenceRecordsWithMetadata;
	
	sequenceRecordsWithMetadata.OriginalSequenceLength = seqLen;
	sequenceRecordsWithMetadata.DistanceClass = setDistanceClassForHashInfo(hinfo);		//TODO: Add more distance classes.

	sequenceRecordsWithMetadata.A_percentage = 0.25;
    sequenceRecordsWithMetadata.C_percentage = 0.25;
    sequenceRecordsWithMetadata.G_percentage = 0.25;
    sequenceRecordsWithMetadata.T_percentage = 0.25;

	sequenceRecordsWithMetadata.KeyCount = keycount; // 1 million sequences
	sequenceRecordsWithMetadata.HashCount = hashcount; // 100 hashes per sequence

    sequenceRecordsWithMetadata.DatagenSeed = 42;
    sequenceRecordsWithMetadata.DataMutateSeed = 43;

	

	/*-------------------------------------------------------------------------------*/
	/*						Similarity and Distance Computation						 */
	/*-------------------------------------------------------------------------------*/
	printf("Using Distance Class: %u\n", sequenceRecordsWithMetadata.DistanceClass);

	sequenceRecordsWithMetadata.binsize = 0.1f;	// Bin size for distance metrics
	sequenceRecordsWithMetadata.binstart = 0.0f;	// Bin start
	sequenceRecordsWithMetadata.binend = 1.0f;		// Bin end
	sequenceRecordsWithMetadata.bincount = static_cast<uint32_t>(std::ceil((sequenceRecordsWithMetadata.binend - sequenceRecordsWithMetadata.binstart) / sequenceRecordsWithMetadata.binsize));
	
	printf("Number of bins for distance metrics: %u\n", sequenceRecordsWithMetadata.bincount);

	// fprint the simrates in one line in the output file.
	out_file << ":4:" ;
	float temp_binstart = sequenceRecordsWithMetadata.binstart;
	for (size_t i = 0; i <= sequenceRecordsWithMetadata.bincount; i++) {
		if (i == sequenceRecordsWithMetadata.bincount)
			out_file << (temp_binstart + (i * sequenceRecordsWithMetadata.binsize)) << "\n";
		else
			out_file << (temp_binstart + (i * sequenceRecordsWithMetadata.binsize)) << ",";
	}

	// out_file << ":5: For metadata. token length determines the number of distinct tokens. Need to focus\n"; 
	

	/*-------------------------------------------------------------------------------*/
	LSHCollisionTestImplInner<hashtype>(&sequenceRecordsWithMetadata, hash, seed, out_file);

	// An empty 2d vector to store collisions <-- This array will be printed to the file for plotting.
	// std::vector<std::vector<float>> collisionRateVec(sequenceRecordsWithMetadata.bincount, std::vector<float>(1, 0.0f));

	
	// // Print the collision rates to the output file.
	// uint32_t binIdx = 0;
	// uint32_t statIdx = 0;
	// for(binIdx=0; binIdx < sequenceRecordsWithMetadata.bincount; binIdx++){		
	// 	for(statIdx=0; statIdx < 1; statIdx++){
	// 		if(statIdx == 1 - 1)
	// 			out_file << collisionRateVec[binIdx][statIdx] << "\n";
	// 		else
	// 			out_file << collisionRateVec[binIdx][statIdx] << ",";
	// 	}
	// }
    
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

	// A token of length toklen will produce keys of length 8*toklen bits for DNA sequences (8 bits per base).
	// If the hash has tokenisation property, then we need to test for multiple token lengths
	// Otherwise, we just use a token length of 0 (no tokenisation)
	// This is because the LSH collision test is primarily designed for tokenised hashes like minhash and simhash
	// which have the tokenisation property.
	// For other hashes, we just use a token length of 0 (i.e. no tokenisation).
	std::vector<uint32_t> tokenlengths;
	if(hinfo->hasTokenisationProperty()){
		printf("Hash %s has tokenisation property. Testing multiple token lengths.\n", hinfo->name);
		tokenlengths = {13}; //{4 ,7, 13, 21, 31, 33}; //create_tokens();
	}
	else{
		tokenlengths = {0}; // No tokenization
	}

	std::vector<uint32_t> sequenceLengths = {256}; //, 32, 48, 64, 80, 96, 128, 256, 512, 1024, 2048, 4096, 8192}; // Corresponding sequence lengths for DNA (8 bits per base)

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
			// For DNA sequences, keybits = 8 * sequence length
			uint32_t keybits = seqLen * 8;	// Eg: Keybits = Sequence length times 8 (8 bits per base)
			
			
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