/*
 * BioHasher
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
#include "BioDataGeneration.h"
#include "LSHCollisionTest.h"
#include "fstream"
#include <iostream>

//-----------------------------------------------------------------------------
/*
The following Impl function doesn't discover the key length - it tests multiple predetermined lengths:

24 bits (3 bytes) - Small keys
32 bits (4 bytes) - Common integer size
64 bits (8 bytes) - Common for larger data
160 bits (20 bytes) - Extended test (if --extra flag)
256 bits (32 bytes) - Large key test (if --extra flag)
*/


//32 bits
std::vector<float> similarity_x_A = {0.0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0};
std::vector<float> similarity_x_B = {0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.90,0.91,0.92,0.93,0.94,0.95,0.96,0.97,0.98,0.99,1.0};
std::vector<float> similarity_x_C = {0.80,0.81,0.82,0.83,0.84,0.85,0.86,0.87,0.88,0.89,0.90,0.91,0.92,0.93,0.94,0.95,0.96,0.97,0.98,0.99,1.0}; // from 0.8 to 1.0
// std::vector<float> sim_vector_step_twentieth_from_098 = {0.980,0.981,0.982,0.983,0.984,0.985,0.986,0.987,0.988,0.989,0.990,0.991,0.992,0.993,0.994,0.995,0.996,0.997,0.998,0.999,1.0};
std::vector<float> sim_vector_step_tenth = similarity_x_C;//{0.0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0};
// {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};//


// from 0.8 to 1.0
const std::vector<int> tokenLengths = {13, 21, 31}; // to be only used for tokenized hashes like minhash and simhash

//low token size 13
//medium token size 21
//high token size 31

 
// Note, the keybits influences the sequence length for bio sequences.
// For example, for DNA sequences with 2 bits per base:
// keybits = 32 -> sequence length = 16 bases
// keybits = 64 -> sequence length = 32 bases
template <typename hashtype>
static bool LSHCollisionTestImpl( const HashInfo * hinfo, unsigned keybits, const seed_t seed, flags_t flags, std::ofstream &out_file) {
	
	/*Initialisation part*/
    bool result = true; //The default result is true unless a test fails.

    const uint32_t sequenceLength = keybits/2; // Length of the sequence to be generated.

    const HashFn   hash     = hinfo->hashFn(g_hashEndian);                  // Get the appropriate hash function for the specified endian-ness
    const unsigned keycount = 1000; //1024; //512 * 1024 * ((hinfo->bits <= 64) ? 3 : 4);   // Number of keys to generate and test. More bits will require more keys for better statistical significance.
    unsigned       keybytes = keybits / 8;                                  // Number of bytes in the key. Note that the keybits have to be a multiple of 8.

    const size_t hashcount = 2000; //1024; // Number of hashes(from the hash family) to compute per key: 1024 hashes per key.

    printf("LSH Collision Test: Key Size = %3u bits (%2u bytes), Keys = %8zu, Hashes per Key = %4zu\n",
           keybits, (unsigned)keybytes, keycount, hashcount);

    printf("Hash Function: %s\n", hinfo->name);
    printf("Hash Bits: %u\n", hinfo->bits);
    
	SequenceRecord records;
	records.SeqLength = sequenceLength;
	records.DistanceClass = 0;	//Hamming distance

	std::vector<float> simrates = sim_vector_step_tenth; //sim_vector_step_twentieth;
    std::vector<float> dissimrates(simrates.size());
    std::vector<int> distances(simrates.size());
    for (size_t i = 0; i < simrates.size(); i++) {
        dissimrates[i] = 1.0f - simrates[i];
        distances[i] = static_cast<int>(dissimrates[i] * sequenceLength);
    }
	printf("Dissimilarity rates: ");
	for (size_t i = 0; i < dissimrates.size(); i++) {
		printf("%.2f ", dissimrates[i]);
	}
	printf("\nCorresponding Hamming distances (for sequence length %u): ", sequenceLength);
	for (size_t i = 0; i < distances.size(); i++) {
		printf("%d ", distances[i]);
	}
	printf("\n");

	// An empty 2d vector to store collisions <-- This array will be printed to the file for plotting.
	std::vector<std::vector<float>> collisionRateVec(simrates.size(), std::vector<float>(keycount, 0.0f));

	/*Part 2: Data generation and logic*/

	uint32_t dissimilarityIdx = 0;
	uint32_t trialIdx = 0;

	seed_t dataGenSeed = seed + 3; // Seed for data generation
	seed_t MutationSeed = seed + 7; // Seed for mutation
	seed_t HashFamilySeed = seed;
	seed_t HashFamilySeedIncrement = 0x9e3779b9;

	seed_t MutationOffset = 13; //  offset to change the seed for mutation
	seed_t dataGenOffset = 11; // Offset to change the seed for data generation


	for(dissimilarityIdx=0; dissimilarityIdx < dissimrates.size(); dissimilarityIdx++){
		
		// Generate the samples for each dissimilarity index
		DataGeneration<hashtype> dataGen(&records, sequenceLength, keycount, dataGenSeed);	// SEEDING
		dataGen.FillRecords(&records, dataGenSeed);

		// Add debug output here:
		// printf("Debug: sequenceLength=%u, keycount=%u, keybytes=%u\n", sequenceLength, keycount, keybytes);
		// printf("Debug: SeqTwoBitOrg size=%zu, SeqTwoBitMut size=%zu\n", 
		// 	records.SeqTwoBitOrg.size(), records.SeqTwoBitMut.size());
		// printf("Debug: Expected size=%u\n", keycount * keybytes);

		MutationEngine<hashtype> mutationEngine(sequenceLength, dissimrates[dissimilarityIdx], MutationSeed, false, keycount);	// SEEDING
		mutationEngine.MutateRecordsWithSubstitutions(&records, distances[dissimilarityIdx]);

		// Compute collision rates for each samples across multiple hash functions in the hash family.
		for(trialIdx=0; trialIdx < keycount; trialIdx++){
			
			float collisionCount = 0;
			float collisionRate = 0;
			seed_t currentHashSeed = HashFamilySeed + (HashFamilySeedIncrement * trialIdx);
			// HashFamilySeed += HashFamilySeedIncrement; // Different starting seed for each key

			// Add bounds checking
            if (trialIdx * keybytes >= records.SeqTwoBitOrg.size() || 
                trialIdx * keybytes >= records.SeqTwoBitMut.size()) {
                printf("ERROR: Index out of bounds! trialIdx=%u, keybytes=%u\n", trialIdx, keybytes);
                return false;
            }

			ExtBlob k1(&records.SeqTwoBitOrg[trialIdx * keybytes], keybytes);
			ExtBlob k2(&records.SeqTwoBitMut[trialIdx * keybytes], keybytes);
			// k1.printbits();
			// k2.printbits();

			for (unsigned i = 0; i < hashcount; i++) {
				hashtype hashOrg;
				hashtype hashMut;

				seed_t currentSeed = currentHashSeed + (HashFamilySeedIncrement * i); // Different seed per hash

				hash(k1, keybytes, currentSeed, &hashOrg);
				// addVCodeInput(k1, keybytes);

				hash(k2, keybytes, currentSeed, &hashMut);
				// addVCodeInput(k, keybytes);
				
				// Check for collision
				if (hashOrg == hashMut) {
					collisionCount++;
				}
			}

			collisionRate = collisionCount / static_cast<float>(hashcount);
			collisionRateVec[dissimilarityIdx][trialIdx] = collisionRate;
		}
		dataGenSeed +=  dataGenOffset; // Change the seed for data generation for next iteration.
		MutationSeed += MutationOffset; // Change the seed for mutation for next iteration.
	}

	for(dissimilarityIdx=0; dissimilarityIdx < dissimrates.size(); dissimilarityIdx++){		
		for(trialIdx=0; trialIdx < keycount; trialIdx++){
			if(trialIdx == keycount-1)
				out_file << collisionRateVec[dissimilarityIdx][trialIdx] << "\n";
			else
				out_file << collisionRateVec[dissimilarityIdx][trialIdx] << ",";
		}
	}

    // For now, the result is always true. we need to add logic to find where the test fails.
    return result;
}

//----------------------------------------------------------------------------

template <typename hashtype>
bool LSHCollisionTest( const HashInfo * hinfo, bool extra, flags_t flags ) {

	std::ofstream out_file("collisionResults_" + std::string(hinfo->name) + "_" + ".csv");
	if (!out_file.is_open()) {
		std::cerr << "Error: Could not open output file" << std::endl;
		exit(EXIT_FAILURE);
	} 
    
    bool result = true;

    printf("[[[ LSH Collision Tests ]]]\n\n");

	const seed_t seed = hinfo->Seed(g_seed);
    // const seed_t seed = 84574;

    // Set up various key sizes to test.
    // result &= LSHCollisionTestImpl<hashtype>(hinfo, 32, seed, flags, out_file);   // Keybits = 32 Sequence length = 16
    // result &= LSHCollisionTestImpl<hashtype>(hinfo, 48, seed, flags, out_file);   // Keybits = 48 Sequence length = 24
    // result &= LSHCollisionTestImpl<hashtype>(hinfo, 64, seed, flags, out_file);   // Keybits = 64 Sequence length = 32
    // result &= LSHCollisionTestImpl<hashtype>(hinfo, 96, seed, flags, out_file);   // Keybits = 96 Sequence length = 48
    result &= LSHCollisionTestImpl<hashtype>(hinfo, 128, seed, flags, out_file);   // Keybits = 128 Sequence length = 64
    if (extra && !hinfo->isVerySlow()) {    // if the extra flag is given, and the hash is not very slow then we can also test longer keys
        result &= LSHCollisionTestImpl<hashtype>(hinfo, 160, seed, flags, out_file);
        result &= LSHCollisionTestImpl<hashtype>(hinfo, 256, seed, flags, out_file);
    }

    printf("%s\n", result ? "" : g_failstr);

	out_file.close();
    return result;
}

INSTANTIATE(LSHCollisionTest, HASHTYPELIST);