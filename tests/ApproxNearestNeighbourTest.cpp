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
#include <filesystem>

#if defined(HAVE_THREADS)
  #include <atomic>
#endif


//----------------------------------------------------------------------------//
template <typename hashtype>
bool LSHApproxNearestNeighbourTest( const HashInfo * hinfo, bool extra, flags_t flags) {
	
	printf("[[[ Approximate Nearest Neighbour Test ]]]\n\n");
	if(extra){
		printf("Extra flag is set. Running extended tests where applicable.\n");
	}

	bool result = true;

	// Create a output file for storing the results.
	std::string filename = "../results/ApproxNearestNeighbourResults_" + std::string(hinfo->name) + ".csv";

	std::ios_base::openmode mode = std::ios::trunc;  // Default: replace
	if (std::filesystem::exists(filename)) {
		mode = std::ios::app;  // File exists: append
	}

	std::ofstream out_file(filename, mode);
	if (!out_file.is_open()) {
		std::cerr << "Error: Could not open output file" << std::endl;
		exit(EXIT_FAILURE);
	}


	// Check the hash function and see if is hamming. If hamming then set the mutation model to simple SNP only.
	if(hinfo->hash_flags & FLAG_HASH_HAMMING_SIMILARITY){
		printf("Hash %s uses Hamming similarity. Setting mutation model to simple SNP only for testing.\n", hinfo->name);
		g_mutation_model = MUTATION_MODEL_SIMPLE_SNP_ONLY;
	}
	
	/*
		If the hash has tokenisation property, then we need to test for multiple token lengths
		Otherwise, we just use a token length of 0 (no tokenisation)
	*/
	uint32_t tokenlength;
	if(hinfo->hasTokenisationProperty()){
		printf("Hash %s has tokenisation property. Testing multiple token lengths.\n", hinfo->name);
		tokenlength = 13;//{4 ,7, 13, 21, 31, 33}; //create_tokens();
	}
	else{
		tokenlength = 0; // No tokenization
	}

	// Note: The tokenlength is the window size.
	

	// ----------------------------------------------------------------------------------------------------- //
	// Generate long sequence. 
	
	uint32_t seqLen = 10000;

	// --- Sanity Checks for Test Configuration Only valid for tokenised hashes ---	//
	if(hinfo->hasTokenisationProperty()){
		// Condition 1: Skipping if token length is greater than sequence length.
		if (tokenlength > seqLen) {
			printf("Skipping: Token length (%u) > sequence length (%u).\n", tokenlength, seqLen);
		}

		// Calculate the number of tokens the sequence will be split into.
		const uint64_t tokensInSequence = (seqLen + tokenlength - 1) / tokenlength; // Ceiling division	//Cardinality
		// Condition 2: Skipping if the sequence is too short to provide enough tokens for meaningful mutation analysis and avoid natural repetition.
		const uint64_t MIN_TOKENS_FOR_MUTATION = 20; // A more reasonable threshold.
		if (tokensInSequence < MIN_TOKENS_FOR_MUTATION) {
			printf("Skipping: Sequence with %llu tokens is too short (min is %llu).\n", (unsigned long long)tokensInSequence, (unsigned long long)MIN_TOKENS_FOR_MUTATION);
		}

		// Condition 3: Skipping if the token space is too small, which would lead to high collision rates
		// even for random data, making the LSH property test less meaningful.
		if (tokenlength > 0) {
			uint64_t maxPossibleTokens = (tokenlength >= 32) ? UINT64_MAX : (1ULL << (2 * tokenlength));
			
			// This checks if the number of tokens in the sequence is a large fraction of the total possible unique tokens.
			// For example, skip if tokensInSequence > 1% of maxPossibleTokens.
			if (maxPossibleTokens / 100 < tokensInSequence) {
				printf("Skipping: High token repetition expected. Tokens in sequence (%llu) vs. max possible (%llu).\n", (unsigned long long)tokensInSequence, (unsigned long long)maxPossibleTokens);
			}
		}

		g_TokenLength = tokenlength;	// Set global token length for testing
	}

	seed_t DatagenSeed = g_GoldenRatio + 17;		// Seed for data generation
	bool isBasesDrawnFromUniformDist = true;

	SequenceRecordsWithMetadataStruct ReferenceSequenceRecord;	// Will contain one sequence
	
	ReferenceSequenceRecord.OriginalSequenceLength = seqLen;
	ReferenceSequenceRecord.DistanceClass = 0;
	ReferenceSequenceRecord.isBasesDrawnFromUniformDist = isBasesDrawnFromUniformDist;
	ReferenceSequenceRecord.DatagenSeed = DatagenSeed;
	ReferenceSequenceRecord.DataMutateSeed = 0;	// No mutation for reference sequence
	ReferenceSequenceRecord.KeyCount = 1;

	SequenceDataGenerator dataGenReference(&ReferenceSequenceRecord);

	
	//print the reference sequence to the output file
	out_file << ":0:Reference Sequence\n";
	out_file << ReferenceSequenceRecord.OriginalSequenceLength << std::endl;
	out_file << ReferenceSequenceRecord.Records[0].SeqASCIIOrg << std::endl;
	
	// ----------------------------------------------------------------------------------------------------- //
	// Generate the Hash table.



	// ----------------------------------------------------------------------------------------------------- //
	// Generate Queries and compute ground truth.



	//
	

	
	
	

	



	// Generate queries randomly. Or we can generate queries by mutating the unique kmers from the input sequence. 

	// Compute the ground truth. <Time tasking> <Parallelize>

	// For each query, compute the hash and check if it got the c-NN right. 




	// For each query, compute the hash and find the nearest neighbour in the hash table. Compute the distance to the nearest neighbour and compare it to the expected distance based on the mutation parameters.






	//You generate sequences, and then as the mutation rate increases, can you see how the recall drops?

















	// std::vector<uint32_t> sequenceLengths;

	// if(hinfo->isSmallSequenceLength()){
	// 	printf("Hash %s is marked as very slow. Limiting test parameters for practicality.\n", hinfo->name);
	// 	sequenceLengths = {60}; //{20,30,40}; //{512};
	// }
	// else{
	// 	sequenceLengths = {512}; //{16, 24, 32, 48, 64, 80, 96, 128, 256, 512, 1024, 2048, 4096, 8192};
	// }

	// seed_t baseSeed = g_GoldenRatio; // Base seed for reproducibility
	
	// seed_t flagsSeedOffset = 101; // Offset to change the seed based on flags
	// for(const auto & toklen : tokenlengths){
		
	// 	SetIsTestActive(true);

	// 	for(const auto & seqLen : sequenceLengths){
	// 		printf("=====================================\n");
	// 		printf("Sequence length: %u \tToken length: %d\n", seqLen, toklen);
	// 		printf("=====================================\n");
	// 		// For DNA sequences, keybits = 8 * sequence length
	// 		uint32_t keybits = seqLen * 8;	// Eg: Keybits = Sequence length times 8 (8 bits per base)
			
			
	// 		

	// 		baseSeed += flagsSeedOffset;
	// 		printf("\nTesting hash: %s with keybits: %u (sequence length: %u), token length: %d\n", hinfo->name, keybits, seqLen, toklen);
	// 		result &= LSHApproxNearestNeighbourTestInner<hashtype>(hinfo, baseSeed, seqLen, flags, out_file);
	// 	}
	// }
		
    // // Cleanup: Reset LSH global variables after test completion
    // SetIsTestActive(false);

    // printf("%s\n", result ? "" : g_failstr);
	out_file.close();
    return result;
}

INSTANTIATE(LSHApproxNearestNeighbourTest, HASHTYPELIST);