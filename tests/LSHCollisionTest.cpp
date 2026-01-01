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


/*-------------------------------------------------------------------------------*/
/*									Collision Test		 						 */
/*-------------------------------------------------------------------------------*/

template <typename hashtype>
bool LSHCollisionTestInnerInner(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata, HashFn hash, const seed_t seed, std::ofstream &out_file) {





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
static bool LSHCollisionTestInner( const HashInfo * hinfo, const uint32_t seqLen, flags_t flags, std::ofstream &out_file) {

	bool result = true;	//TODO: Update this based on test results.

	const uint32_t keybytes = seqLen; //Since 8 bytes = 1 base.

	const int tokenlength = GetLSHTokenLength();  		// Get runtime token length

	HashFn hash = hinfo->hashFn(g_hashEndian);

	const seed_t seed = hinfo->Seed(g_seed);

	const unsigned keycount = 1000;	// Number of keys to generate and test.
	
    const size_t hashcount = 1000;	// Number of hashes(from the hash family) to compute per key

	// File header
	out_file << ":1:LSH Collision Test Results\n";
	out_file << ":2:" << "Hashname," << "SequenceLength," << "TokenLength" << std::endl;
	out_file << ":3:" << hinfo->name << "," << seqLen << "," << tokenlength << std::endl;
    
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

	sequenceRecordsWithMetadata.binsize = 0.01f;	// Bin size for distance metrics
	sequenceRecordsWithMetadata.binstart = 0.0f;	// Bin start
	sequenceRecordsWithMetadata.binend = 1.01f;		// Bin end
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

	LSHCollisionTestInnerInner<hashtype>(&sequenceRecordsWithMetadata, hash, seed, out_file);

    return result;	//TODO: For now, the result is always true. We need to add logic to find where the test fails.
}


//----------------------------------------------------------------------------//
template <typename hashtype>
bool LSHCollisionTest( const HashInfo * hinfo, bool extra, flags_t flags ) {
	
	printf("[[[ LSH Collision Tests ]]]\n\n");
	if(extra){
		printf("Extra flag is set. Running extended tests where applicable.\n");
	}

	bool result = true;

	// Create a output file for storing the results.
	std::ofstream out_file("../results/collisionResults_" + std::string(hinfo->name)  +".csv");
	if (!out_file.is_open()) {
		std::cerr << "Error: Could not open output file" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Create a code for generating an output file name based on hash name.
	
	/*
		If the hash has tokenisation property, then we need to test for multiple token lengths
		Otherwise, we just use a token length of 0 (no tokenisation)
	*/
	std::vector<uint32_t> tokenlengths;
	if(hinfo->hasTokenisationProperty()){
		printf("Hash %s has tokenisation property. Testing multiple token lengths.\n", hinfo->name);
		tokenlengths = {4 ,7, 13, 21, 31, 33}; //create_tokens();
	}
	else{
		tokenlengths = {0}; // No tokenization
	}
	
	std::vector<uint32_t> sequenceLengths = {16, 24, 32, 48, 64, 80, 96, 128, 256, 512, 1024, 2048, 4096, 8192};

	for(const auto & toklen : tokenlengths){
		
		SetIsTestActive(true);

		for(const auto & seqLen : sequenceLengths){
			printf("=====================================\n");
			printf("Sequence length: %u \tToken length: %d\n", seqLen, toklen);
			printf("=====================================\n");
			// For DNA sequences, keybits = 8 * sequence length
			uint32_t keybits = seqLen * 8;	// Eg: Keybits = Sequence length times 8 (8 bits per base)
			
			
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

			printf("\nTesting hash: %s with keybits: %u (sequence length: %u), token length: %d\n", hinfo->name, keybits, seqLen, toklen);
			result &= LSHCollisionTestInner<hashtype>(hinfo, seqLen, flags, out_file);
		}
	}
		
    // Cleanup: Reset LSH global variables after test completion
    SetLSHTestActive(false);

    printf("%s\n", result ? "" : g_failstr);
	out_file.close();
    return result;
}

INSTANTIATE(LSHCollisionTest, HASHTYPELIST);