#pragma once

#include <iostream>
#include <vector>
#include <cassert>
#include <random>
#include <numeric>
#include <variant>
#include <bitset>
#include "LSHGlobals.h"

/*
There are two ways to introduce mutations in the sequences:
1. By specifying an error rate (between 0 and 1) for each nucleotide. 
   So, for a sequence of length N and an error rate of e, the expected number of mutations
   will be ceil(e * N). This is useful when we want to simulate a certain error rate.
2. By specifying an absolute number of mutations (between 0 and sequence_length). This is useful when we want to test
   the hash function's sensitivity to a certain number of mutations.
   Currently we are using the second method.
   We are only using substitutions for now.


Each distance class requires different handling in terms of data generation and mutation.

---Hamming Distance---
Hamming distance measures the number of positions at which the corresponding symbols are different between two strings of equal length.
Since we are focusing on equal length, indels (insertions and deletions) are not applicable here. 
In hamming distance, we can simply substitute bases at random positions.
For example, if we have the sequence "ACGT" and we want to introduce a mutation,
we can randomly select a position (say position 2, which is 'G') and change it to another base (say 'T'),
resulting in the mutated sequence "ACTT". This maintains the sequence length and only alters the specified number of bases.
--- Processing Record 0 (Hamming) ---
Sequence length: 16 bases
Edit distance: 4 substitutions
  Position 12: A -> C
  Position 3: G -> T
  Position 8: C -> A
  Position 1: T -> G
  Original: ATCGACGTACGTACGT
  Mutated:  AGCGACGTCCGTACCT
  Actual differences: 4 (expected: 4)

✓ Hamming mutation completed for 1000 records

---Jaccard Distance---
since setA will be equal to setB in terms of cardinality, therefore, we can only perform on even number of distances.
Original: SeqTwoBitOrg (flat bit vector)
    ↓ tokenizeSequence
Tokens: setA.tokens (vector of vectors)
    ↓ copy to setB
Tokens: setB.tokens (vector of vectors)
    ↓ mutate selected tokens
Modified: setB.tokens (vector of vectors with mutations)
    ↓ reconstructSequence (CONCATENATION HAPPENS HERE)
Result: SeqTwoBitMut (flat bit vector with mutations)
*/


// Aspect	Hamming	Jaccard
// Operates on	Individual bases	Tokens (k-mers)
// Length change	No (always equal length)	No (equal set cardinality)
// Mutation type	Substitution only	Token replacement
// Unit	Single nucleotide (2 bits)	Token of k nucleotides
// Randomization	Position-based	Token index-based
// Edit distance	Number of bases changed	Number of tokens changed / 2

/*
There are two ways to introduce mutations in the sequences:
1. By specifying an error rate (between 0 and 1) for each nucleotide. 
   So, for a sequence of length N and an error rate of e, the expected number of mutations
   will be ceil(e * N). This is useful when we want to simulate a certain error rate.
2. By specifying an absolute number of mutations (between 0 and sequence_length). This is useful when we want to test
   the hash function's sensitivity to a certain number of mutations.
   Currently we are using the second method.
   We are only using substitutions for now.
*/

#define two_bit_per_base 2 // 2 bits per base for A,C,G,T

static const char bases[4] = {'A', 'C', 'T', 'G'};
static constexpr size_t NUM_BASES = sizeof(bases)/sizeof(bases[0]);

// // Distance-specific metadata structures
// struct HammingMetadataStruct {
// 	uint32_t tempvariable; // To be used later for describing Hamming-specific metadata
//     // Currently empty, but could add Hamming-specific fields
//     // e.g., position-wise mutation probabilities
// };

// struct JaccardMetadataStruct {
//     uint32_t SetCardinality;
//     uint32_t TokenSize;
//     // std::vector<uint32_t> SetAIndices;  // Indices of elements in set A
//     // std::vector<uint32_t> SetBIndices;  // Indices of elements in set B
// };

// struct LevenshteinMetadataStruct {
//     // For future: insertion/deletion support
//     std::vector<uint32_t> OriginalLengths;
//     std::vector<uint32_t> MutatedLengths;

// 	float InsertionRate;     // Probability of insertion
//     float DeletionRate;      // Probability of deletion
//     float SubstitutionRate;  // Probability of substitution
//     uint32_t MaxLengthChange; // Maximum allowed length change
// };

struct SequenceRecordUnitStruct{
	// Original sequence
	std::vector<uint8_t> SeqTwoBitOrg;     	// x1 Sequences in 2-bit encoding. {0,1}
	std::string SeqASCIIOrg;			// x1 Original sequence in ASCII for reference
	uint32_t OriginalLength;

	// Mutated sequence (may differ in length after indels)
	std::vector<uint8_t> SeqTwoBitMut;		// x1 Sequences in 2-bit encoding. {0,1}
	std::string SeqASCIIMut;			// x1 Mutated sequence in ASCII for reference
	uint32_t MutatedLength;

	// Distance metrics
	uint32_t EditDistance;					// x1 Edit distance between the original and mutated sequence	
	uint32_t HashDistance; 					// x1 Hamming distance between the two hash values

	// Optional: Track mutation operations for analysis
    // struct MutationOp {
    //     enum Type { SUBSTITUTION, INSERTION, DELETION } type;
    //     uint32_t position;
    //     uint8_t old_base;  // For substitution/deletion
    //     uint8_t new_base;  // For substitution/insertion
    // };
    // std::vector<MutationOp> mutations;  // Optional: track what was done
};

struct SequenceRecordsWithMetadataStruct{
	uint32_t KeyCount;                    // Number of sequence pairs
    uint32_t OriginalSequenceLength;      // Original length (before mutations)
    uint32_t DistanceClass;               // 1=Hamming, 2=Jaccard, 3=Levenshtein
    seed_t Seed;                          // For reproducibility
	uint32_t HashCount;


	double A_percentage;	// Percentage of A bases in the original sequence
	double C_percentage;	// Percentage of C bases in the original sequence
	double G_percentage;	// Percentage of G bases in the original sequence
	double T_percentage;	// Percentage of T bases in the original sequence
	
	// Distance-specific metadata using variant
	uint32_t JaccardTokenSize;         // Token size for Jaccard distance
	uint32_t JaccardSetCardinality; // Set cardinality for Jaccard distance

	// Array of sequence records (AoS)
    std::vector<SequenceRecordUnitStruct> Records;

	 // Constructor with default values
    SequenceRecordsWithMetadataStruct() 
        : KeyCount(0), 
          OriginalSequenceLength(0), 
          DistanceClass(0),
          Seed(0),
          HashCount(0),
          A_percentage(0.25f),
          C_percentage(0.25f),
          G_percentage(0.25f),
          T_percentage(0.25f),
		  JaccardTokenSize(0),	
		  JaccardSetCardinality(0){
    }

};


class DataGeneration{
	private:
		void decodeSequencesToASCII(const std::vector<uint8_t>& seqTwoBit, std::string& seqASCII, uint32_t sequenceLength) {
			assert(seqASCII.size() == sequenceLength);
			for (uint32_t pos = 0; pos < sequenceLength; pos++) {
				uint32_t byte_idx = (pos * two_bit_per_base) / 8;
				uint32_t bit_offset = (pos * two_bit_per_base) % 8;
				uint8_t base = (seqTwoBit[byte_idx] >> bit_offset) & 0x03;
				seqASCII[pos] = bases[base];
			}
		}

		void generateWithDistribution(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, Rand &r) {
			// Generating sequences respecting nucleotide distribution
			for (uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
				auto& record = sequenceRecordsWithMetadata->Records[rec_idx];

				// setting the size of the sequences
				record.OriginalLength = sequenceRecordsWithMetadata->OriginalSequenceLength;
				record.EditDistance = 0;
				record.HashDistance = 0;
				
				// Allocate bit vectors
				uint32_t keyBytes = ((sequenceRecordsWithMetadata->OriginalSequenceLength * two_bit_per_base ) + 7) / 8;
				record.SeqTwoBitOrg.resize(keyBytes, 0);
				record.SeqASCIIOrg.resize(sequenceRecordsWithMetadata->OriginalSequenceLength, 'A');
				
				for (uint32_t pos = 0; pos < sequenceRecordsWithMetadata->OriginalSequenceLength; pos++) {
					// Generate base according to distribution
					double rand_val = r.rand_u64() / (double)UINT64_MAX;
					uint8_t base;
					
					if (rand_val < sequenceRecordsWithMetadata->A_percentage) {
						base = 0; // A
					} else if (rand_val < (sequenceRecordsWithMetadata->A_percentage + sequenceRecordsWithMetadata->C_percentage)) {
						base = 1; // C
					} else if (rand_val < (sequenceRecordsWithMetadata->A_percentage + sequenceRecordsWithMetadata->C_percentage + 
							sequenceRecordsWithMetadata->G_percentage)) {
						base = 3; // G
					} else {
						base = 2; // T
					}
					
					// Set base in 2-bit encoding
					uint32_t byte_idx = (pos * two_bit_per_base) / 8;
					uint32_t bit_offset = (pos * two_bit_per_base) % 8;
					
					record.SeqTwoBitOrg[byte_idx] &= ~(0x03 << bit_offset);	// reseting the bits at the position
					record.SeqTwoBitOrg[byte_idx] |= (base << bit_offset);	// setting the new base
				}

				// For reference, also create ASCII representation
				decodeSequencesToASCII(record.SeqTwoBitOrg, record.SeqASCIIOrg, record.OriginalLength);


				/* Debugging Statements! Please never delete!*/
				// // Print first 10 sequences for verification
				// if (rec_idx < 10) {
				// 	std::cout << "Record " << rec_idx << " Original Sequence: " << record.SeqASCIIOrg << std::endl;
				// }

				// // Print the bit representation for the first 10 sequences for verification, 2 bit encoded form. Offset to print is 2 bits
				// if (rec_idx < 10) {
				// 	std::cout << "Record " << rec_idx << " Original 2-bit: ";
				// 	for (uint32_t pos = 0; pos < sequenceRecordsWithMetadata->OriginalSequenceLength; pos++) {
				// 		uint32_t byte_idx = (pos * two_bit_per_base) / 8;
				// 		uint32_t bit_offset = (pos * two_bit_per_base) % 8;
				// 		uint8_t base = (record.SeqTwoBitOrg[byte_idx] >> bit_offset) & 0x03;
				// 		std::cout << std::bitset<2>(base);
				// 	}
				// 	std::cout << std::endl;
				// }
			}
		}
	
	public:
		// Constructor to initialize data generation parameters.
		DataGeneration(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, seed_t seed){
			assert(sequenceRecordsWithMetadata != nullptr);
			sequenceRecordsWithMetadata->Seed = seed;
			// std::cout << "DataGeneration: SequenceLength = " << sequenceRecordsWithMetadata->OriginalSequenceLength << ", KeyCount = " << sequenceRecordsWithMetadata->KeyCount << std::endl;
		}
		
		// Filling the records.
		void FillRecords(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata){
			assert(sequenceRecordsWithMetadata != nullptr);
			Rand r(sequenceRecordsWithMetadata->Seed);

			// Allocate memory for sequence records
			sequenceRecordsWithMetadata->Records.resize(sequenceRecordsWithMetadata->KeyCount);
			
			// Generate sequences with specified nucleotide distribution
			generateWithDistribution(sequenceRecordsWithMetadata, r);
			
		}
};
//----------------------------------------------------------------------------//

class MutationEngine{
	private:
		// float TotalErrorRate;
		// float SubstitutionRate;
		// float InsertionRate;
		// float DeletionRate;
		
		// // The following mutation rates are for future use when we implement more complex mutations. SVs. Requires the sequence to be longer. 
		// float TranslocationRate;
		// float DuplicationRate;
		// float InversionRate;
		// float TandemDuplicationRate;
		// float TransversionRate;
		
		
		uint32_t DistanceClass;

		uint32_t CurrentDistance;
		bool IsMutationRateEnabled;

		void decodeSequencesToASCII(const std::vector<uint8_t>& seqTwoBit, std::string& seqASCII, uint32_t sequenceLength) {
			assert(seqASCII.size() == sequenceLength);
			for (uint32_t pos = 0; pos < sequenceLength; pos++) {
				uint32_t byte_idx = (pos * two_bit_per_base) / 8;
				uint32_t bit_offset = (pos * two_bit_per_base) % 8;
				uint8_t base = (seqTwoBit[byte_idx] >> bit_offset) & 0x03;
				seqASCII[pos] = bases[base];
			}
		}

		/*-------------------------------------------------------------------------------*/
		struct TokenizedSequence {
			std::vector<std::vector<uint8_t>> tokens;  // Each token is a separate bit vector
			uint32_t tokenLength;                       // Length in bases
			uint32_t tokenBits;                         // Length in bits (tokenLength * 2)
			uint32_t tokenBytes;                        // Length in bytes
			bool hasPadding;                            // Flag if padding was added
		};

		std::vector<uint8_t> extractToken(const std::vector<uint8_t>& sequence, uint32_t tokenIndex, uint32_t tokenLength) {
			uint32_t tokenBits = tokenLength * 2;
			uint32_t tokenBytes = (tokenBits + 7) / 8;
			std::vector<uint8_t> token(tokenBytes, 0);
			
			// Starting bit position for this token
			uint32_t startBase = tokenIndex * tokenLength;
			
			// Copy nucleotides (2 bits each) for this token
			for (uint32_t base = 0; base < tokenLength; base++) {
				uint32_t srcBasePos = startBase + base;
				uint32_t srcBitPos = srcBasePos * 2;  // Convert base position to bit position
				uint32_t srcByteIdx = srcBitPos / 8;
				uint32_t srcBitOffset = srcBitPos % 8;
				
				uint32_t dstBitPos = base * 2;  // Destination bit position in token
				uint32_t dstByteIdx = dstBitPos / 8;
				uint32_t dstBitOffset = dstBitPos % 8;
				
				// Extract 2-bit nucleotide from source
				uint8_t nucleotide = (sequence[srcByteIdx] >> srcBitOffset) & 0x03;
				
				// Set 2-bit nucleotide in destination
				token[dstByteIdx] |= (nucleotide << dstBitOffset);
			}
			
			return token;
		}

		void setToken(std::vector<uint8_t>& sequence, uint32_t tokenIndex, uint32_t tokenLength, const std::vector<uint8_t>& token) {
			uint32_t startBase = tokenIndex * tokenLength;
			
			for (uint32_t base = 0; base < tokenLength; base++) {
				uint32_t dstBasePos = startBase + base;
				uint32_t dstBitPos = dstBasePos * 2;
				uint32_t dstByteIdx = dstBitPos / 8;
				uint32_t dstBitOffset = dstBitPos % 8;
				
				uint32_t srcBitPos = base * 2;
				uint32_t srcByteIdx = srcBitPos / 8;
				uint32_t srcBitOffset = srcBitPos % 8;
				
				// Extract 2-bit nucleotide from token
				uint8_t nucleotide = (token[srcByteIdx] >> srcBitOffset) & 0x03;
				
				// Clear the 2 bits in destination
				sequence[dstByteIdx] &= ~(0x03 << dstBitOffset);
				
				// Set the 2 bits in destination
				sequence[dstByteIdx] |= (nucleotide << dstBitOffset);
			}
		}

		std::vector<uint8_t> generateRandomToken(uint32_t tokenLength, Rand& r) {
			uint32_t tokenBits = tokenLength * 2;
			uint32_t tokenBytes = (tokenBits + 7) / 8;
			std::vector<uint8_t> token(tokenBytes, 0);
			
			for (uint32_t base = 0; base < tokenLength; base++) {	//TODO: THE random function should not be repetitive.
				uint8_t randomNucleotide = (r.rand_range(4)) & 0x03;  // 0-3 (A, C, T, G)
				
				uint32_t bitPos = base * 2;
				uint32_t byteIdx = bitPos / 8;
				uint32_t bitOffset = bitPos % 8;
				
				// Set the 2-bit nucleotide
				token[byteIdx] |= (randomNucleotide << bitOffset);
			}
			
			return token;
		}

		// Tokenize sequence with padding support
		TokenizedSequence tokenizeSequence(const std::vector<uint8_t>& sequence, uint32_t sequenceLength, uint32_t tokenLength) {
			
			TokenizedSequence result;
			result.tokenLength = tokenLength;
			result.tokenBits = tokenLength * 2;
			result.tokenBytes = ((tokenLength * 2) + 7) / 8;
			result.hasPadding = false;
        
			uint32_t numCompleteTokens = sequenceLength / tokenLength;
			uint32_t remainingBases = sequenceLength % tokenLength;
			
			if (remainingBases > 0) {
				result.hasPadding = true;
				// printf("WARNING: Sequence length %u not divisible by token length %u. Adding %u padding bases.\n", sequenceLength, tokenLength, tokenLength - remainingBases);
			}
			
			// Extract complete tokens
			for (uint32_t i = 0; i < numCompleteTokens; i++) {
				result.tokens.push_back(extractToken(sequence, i, tokenLength));
			}
			
			// Handle padding if needed
			if (remainingBases > 0) {
				std::vector<uint8_t> paddedToken(result.tokenBytes, 0);
				
				// Copy remaining bases
				for (uint32_t base = 0; base < remainingBases; base++) {
					uint32_t srcBasePos = numCompleteTokens * tokenLength + base;
					uint32_t srcBitPos = srcBasePos * 2;
					uint32_t srcByteIdx = srcBitPos / 8;
					uint32_t srcBitOffset = srcBitPos % 8;
					
					// Extract nucleotide
					uint8_t nucleotide = (sequence[srcByteIdx] >> srcBitOffset) & 0x03;
					
					// Set in padded token
					uint32_t dstBitPos = base * 2;
					uint32_t dstByteIdx = dstBitPos / 8;
					uint32_t dstBitOffset = dstBitPos % 8;
					
					paddedToken[dstByteIdx] |= (nucleotide << dstBitOffset);
				}
				
				// Padding (zeros) is implicit
				result.tokens.push_back(paddedToken);
			}
			
			return result;
		}

		// Reconstruct sequence from tokens
		std::vector<uint8_t> reconstructSequence(const TokenizedSequence& tokenized, uint32_t originalSequenceLength) {
			uint32_t sequenceBits = originalSequenceLength * 2;
			uint32_t sequenceBytes = (sequenceBits + 7) / 8;
			std::vector<uint8_t> sequence(sequenceBytes, 0);
			
			for (uint32_t tokenIdx = 0; tokenIdx < tokenized.tokens.size(); tokenIdx++) {
				setToken(sequence, tokenIdx, tokenized.tokenLength, tokenized.tokens[tokenIdx]);
			}
			
			return sequence;
		}

		void MutateRecordsForJaccard(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, const uint32_t currentEditDistance, const seed_t seed){
			

			uint32_t tokenLength = sequenceRecordsWithMetadata->JaccardTokenSize;
    		assert(tokenLength > 0 && "Token length must be greater than 0");	//Redundant check

			// Random number generators! Need to focus on the seeding parameters. So, TODO.
			Rand rIndices(seed, sizeof(uint32_t));    // For selecting indices
			Rand rToken(seed + 1, sizeof(uint8_t));   // For generating new tokens

			// Processing each record
			for (uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
				auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
				
				// printf("\n--- Processing Record %u ---\n", rec_idx);
				// printf("Original sequence length: %u bases\n", record.OriginalLength);
				// printf("Token length: %u bases\n", tokenLength);
				
				// Step 1: Copy SeqTwoBitOrg to SeqTwoBitMut to create set B (initially identical to set A)
				record.SeqTwoBitMut = record.SeqTwoBitOrg;
				record.MutatedLength = record.OriginalLength;

				// printf("%d,%d\n",record.OriginalLength, record.MutatedLength);

				// Step 2: Tokenize both sequences
				TokenizedSequence setA = tokenizeSequence(record.SeqTwoBitOrg, record.OriginalLength, tokenLength);
				TokenizedSequence setB = tokenizeSequence(record.SeqTwoBitMut, record.MutatedLength, tokenLength);

				uint32_t numTokens = setB.tokens.size();
				// printf("Number of tokens: %u\n", numTokens);

				// Resize if there was padding. First I am rezing the vectors to be safe and clearing them.
				record.OriginalLength = tokenLength * numTokens;
				record.MutatedLength = tokenLength * numTokens;

				// printf("%d,%d\n",record.OriginalLength, record.MutatedLength);

				record.SeqTwoBitOrg.resize(record.OriginalLength, 0);
				record.SeqTwoBitMut.resize(record.MutatedLength, 0);
				
				// if (setB.hasPadding) {
				// 	printf("Note: Padding was added to the last token\n");
				// }

				//small check for personal sanity
				if(sequenceRecordsWithMetadata->JaccardSetCardinality != numTokens){
					fprintf(stderr, "ERROR: Jaccard metadata cardinality %u does not match actual number of tokens %u calculated before in parent function.\n", 
							sequenceRecordsWithMetadata->JaccardSetCardinality, numTokens);
					exit(EXIT_FAILURE);
				}

				// Step 3: Calculate number of tokens to change // This is redundant. since at call of the test, we do verify it.
				uint32_t tokensToChange = currentEditDistance / 2;
				
				if (tokensToChange > numTokens) {
					fprintf(stderr, "ERROR: Cannot change %u tokens when only %u tokens exist\n", 
							tokensToChange, numTokens);
					exit(EXIT_FAILURE);
				}

				// Step 4: Select random unique indices to change in set B
				std::vector<uint32_t> tokenIndices(numTokens);
				std::iota(tokenIndices.begin(), tokenIndices.end(), 0);
				
				// Fisher-Yates shuffle
				for (int i = numTokens - 1; i > 0; i--) {
					int j = rIndices.rand_range(i + 1);
					std::swap(tokenIndices[i], tokenIndices[j]);
				}

				// Step 5: Replace selected tokens in set B with new random tokens
				for (uint32_t i = 0; i < tokensToChange; i++) {
					uint32_t tokenIdx = tokenIndices[i];
					
					// Generate a new random token (different from original)
					std::vector<uint8_t> oldToken = setB.tokens[tokenIdx];
					std::vector<uint8_t> newToken;
					
					// Ensure the new token is different from the old one
					int maxAttempts = 100;
					int attempts = 0;
					do {
						newToken = generateRandomToken(tokenLength, rToken);
						attempts++;
					} while (newToken == oldToken && attempts < maxAttempts);
					
					if (attempts >= maxAttempts) {
						fprintf(stderr, "WARNING: Could not generate unique token after %d attempts\n", 
								maxAttempts);
					}
					
					// Replace the token in set B
					setB.tokens[tokenIdx] = newToken;
					
					// printf("  Changed token %u (index %u)\n", i, tokenIdx);
					
					// Debug: Print token change (first few bases)
					// if (rec_idx < 3 && i < 20) {  // Print details for first few records and tokens
					// 	printf("    Old token: ");
					// 	for (uint32_t b = 0; b < std::min(tokenLength, 8u); b++) {
					// 		uint32_t bitPos = b * 2;
					// 		uint32_t byteIdx = bitPos / 8;
					// 		uint32_t bitOffset = bitPos % 8;
					// 		uint8_t base = (oldToken[byteIdx] >> bitOffset) & 0x03;
					// 		printf("%c", bases[base]);
					// 	}
					// 	printf("\n    New token: ");
					// 	for (uint32_t b = 0; b < std::min(tokenLength, 8u); b++) {
					// 		uint32_t bitPos = b * 2;
					// 		uint32_t byteIdx = bitPos / 8;
					// 		uint32_t bitOffset = bitPos % 8;
					// 		uint8_t base = (newToken[byteIdx] >> bitOffset) & 0x03;
					// 		printf("%c", bases[base]);
					// 	}
					// 	printf("\n");
					// }
				}

				// Step 6: Reconstruct the mutated sequence from the modified set B
				record.SeqTwoBitOrg = reconstructSequence(setA, record.OriginalLength);
        		record.SeqTwoBitMut = reconstructSequence(setB, record.MutatedLength);

				// printf("%d,%d\n",record.OriginalLength, record.MutatedLength);

				// Step 7: Update metadata
        		record.EditDistance = currentEditDistance;	//set distance
				
				// Decode to ASCII for verification
				record.SeqASCIIOrg.resize(record.OriginalLength, 'A');
				decodeSequencesToASCII(record.SeqTwoBitOrg, record.SeqASCIIOrg, record.OriginalLength);

				// Decode mutated sequence to ASCII
				record.SeqASCIIMut.resize(record.MutatedLength, 'A');
				decodeSequencesToASCII(record.SeqTwoBitMut, record.SeqASCIIMut, record.MutatedLength);

				//--------------------------------
				// // print tokens
				// for (uint32_t t = 0; t < numTokens; t++) {
				// 	printf("Token %u: ", t);
				// 	for (uint32_t b = 0; b < tokenLength; b++) {
				// 		uint32_t bitPos = b * 2;
				// 		uint32_t byteIdx = bitPos / 8;
				// 		uint32_t bitOffset = bitPos % 8;
				// 		uint8_t base = (setA.tokens[t][byteIdx] >> bitOffset) & 0x03;
				// 		printf("%c", bases[base]);
				// 	}
				// 	printf("\n");
				// }
				// for (uint32_t t = 0; t < numTokens; t++) {
				// 	printf("Token %u: ", t);
				// 	for (uint32_t b = 0; b < tokenLength; b++) {
				// 		uint32_t bitPos = b * 2;
				// 		uint32_t byteIdx = bitPos / 8;
				// 		uint32_t bitOffset = bitPos % 8;
				// 		uint8_t base = (setB.tokens[t][byteIdx] >> bitOffset) & 0x03;
				// 		printf("%c", bases[base]);
				// 	}
				// 	printf("\n");
				// }

				//------------------------------------


				// Redundant part. Moved to decodeSequencesToASCII function.
				//------------------------------------
				// std::string seqMutASCII(record.OriginalLength, 'A');
				// for (uint32_t pos = 0; pos < record.OriginalLength; pos++) {
				// 	if(pos >= seqMutASCII.size()){
				// 		fprintf(stderr, "ERROR: pos %u out of bounds for seqMutASCII of size %zu\n", 
				// 				pos, seqMutASCII.size());
				// 		continue;
				// 	}
				// 	uint32_t byte_idx = (pos * two_bit_per_base) / 8;
				// 	uint32_t bit_offset = (pos * two_bit_per_base) % 8;
				// 	uint8_t base = (record.SeqTwoBitMut[byte_idx] >> bit_offset) & 0x03;
				// 	seqMutASCII[pos] = bases[base];
				// }
				// record.SeqASCIIMut = seqMutASCII;
				//------------------------------------
				
				// Debug: Print sequences for first few records
				// if (rec_idx < 3) {
				// 	printf("  Original: %s\n", record.SeqASCIIOrg.c_str());
				// 	printf("  Mutated:  %s\n", record.SeqASCIIMut.c_str());
				// }
			}
			// printf("\n Jaccard mutation completed for %u records\n", sequenceRecordsWithMetadata->KeyCount);
		}

		/*-------------------------------------------------------------------------------*/
		// Mutate a single base to a different base // Currently unused
		uint8_t MutateBase(uint8_t original_base, Rand &rmutatebase) {
			assert(original_base < NUM_BASES);
			uint8_t new_base;
			int attempts = 0;
			do {
				new_base = rmutatebase.rand_range(NUM_BASES) & 0x03;
				attempts++;
			} while (new_base == original_base && attempts < 10);
			
			// Fallback if we can't generate a different base
			if (new_base == original_base) {
				new_base = (original_base + 1) & 0x03;  // Simple increment
			}
			
			return new_base;
		}

		// Helper: Get base at position // Currently unused
		uint8_t GetBaseAtPosition(const std::vector<uint8_t>& seq, uint32_t pos) {
			uint32_t byte_idx = (pos * two_bit_per_base) / 8;
			uint32_t bit_offset = (pos * two_bit_per_base) % 8;
			return (seq[byte_idx] >> bit_offset) & 0x03;
		}
		
		// Helper: Set base at position	// Currently unused
		void SetBaseAtPosition(std::vector<uint8_t>& seq, uint32_t pos, uint8_t base) {
			uint32_t byte_idx = (pos * two_bit_per_base) / 8;
			uint32_t bit_offset = (pos * two_bit_per_base) % 8;
			seq[byte_idx] &= ~(0x03 << bit_offset);
			seq[byte_idx] |= (base << bit_offset);
		}
		
		void MutateRecordsForHamming(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, const uint32_t currentEditDistance, const seed_t seed){
			
			// Random number generators
			Rand rIndices(seed, sizeof(uint32_t));        // For selecting positions to mutate
			Rand rBases(seed + 1, sizeof(uint8_t));       // For generating new bases
			
			// Process each record
			for (uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
				auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
				
				// if (rec_idx < 3) {  // Debug output for first few records
				// 	printf("\n--- Processing Record %u (Hamming) ---\n", rec_idx);
				// 	printf("Sequence length: %u bases\n", record.OriginalLength);
				// 	printf("Edit distance: %u substitutions\n", currentEditDistance);
				// }
				
				// Step 1: Copy original sequence to mutated sequence
				record.SeqTwoBitMut = record.SeqTwoBitOrg;
				record.MutatedLength = record.OriginalLength;  // Length doesn't change for Hamming
				
				// Step 2: Create position indices for random selection
				std::vector<uint32_t> positionIndices(record.OriginalLength);
				std::iota(positionIndices.begin(), positionIndices.end(), 0);
				
				// Step 3: Fisher-Yates shuffle to randomly select positions
				for (int i = record.OriginalLength - 1; i > 0; i--) {
					int j = rIndices.rand_range(i + 1);
					std::swap(positionIndices[i], positionIndices[j]);
				}
				
				// Step 4: Mutate bases at selected positions
				for (uint32_t mut_idx = 0; mut_idx < currentEditDistance; mut_idx++) {
					uint32_t pos = positionIndices[mut_idx];
					
					// Calculate bit position for this base
					uint32_t bitPos = pos * 2;  // 2 bits per base
					uint32_t byteIdx = bitPos / 8;
					uint32_t bitOffset = bitPos % 8;
					
					// Bounds check
					if (byteIdx >= record.SeqTwoBitMut.size()) {
						fprintf(stderr, "ERROR: byteIdx %u out of bounds (size %zu) at position %u\n", 
								byteIdx, record.SeqTwoBitMut.size(), pos);
						continue;
					}
					
					// Extract original base (2 bits)
					uint8_t original_base = (record.SeqTwoBitMut[byteIdx] >> bitOffset) & 0x03;
					
					// Generate a different base (ensure it's not the same as original)
					uint8_t new_base;
					int maxAttempts = 10;
					int attempts = 0;
					do {
						new_base = rBases.rand_range(4) & 0x03;  // Random base 0-3
						attempts++;
					} while (new_base == original_base && attempts < maxAttempts);
					
					if (new_base == original_base && attempts >= maxAttempts) {
						// Fallback: just increment (wraps around 0-3)
						new_base = (original_base + 1) & 0x03;
					}
					
					// Clear the 2 bits at this position
					record.SeqTwoBitMut[byteIdx] &= ~(0x03 << bitOffset);
					
					// Set the new base
					record.SeqTwoBitMut[byteIdx] |= (new_base << bitOffset);
					
					// // Debug output for first few mutations
					// if (rec_idx < 3 && mut_idx < 5) {
					// 	printf("  Position %u: %c -> %c\n", pos, 
					// 		bases[original_base], bases[new_base]);
					// }
				}
				
				// Step 5: Update metadata
				record.EditDistance = currentEditDistance;
				
				// Step 6: Decode to ASCII for verification
				record.SeqASCIIMut.resize(record.MutatedLength, 'A');
				for (uint32_t pos = 0; pos < record.MutatedLength; pos++) {
					uint32_t bitPos = pos * 2;
					uint32_t byteIdx = bitPos / 8;
					uint32_t bitOffset = bitPos % 8;
					uint8_t base = (record.SeqTwoBitMut[byteIdx] >> bitOffset) & 0x03;
					record.SeqASCIIMut[pos] = bases[base];
				}
				
				// // Debug: Print sequences for first few records
				// if (rec_idx < 3) {
				// 	printf("  Original: %s\n", record.SeqASCIIOrg.c_str());
				// 	printf("  Mutated:  %s\n", record.SeqASCIIMut.c_str());
					
				// 	// Count actual differences for verification
				// 	uint32_t differences = 0;
				// 	for (uint32_t pos = 0; pos < record.OriginalLength; pos++) {
				// 		if (record.SeqASCIIOrg[pos] != record.SeqASCIIMut[pos]) {
				// 			differences++;
				// 		}
				// 	}
				// 	printf("  Actual differences: %u (expected: %u)\n", differences, currentEditDistance);
				// }
			}
			
			// printf("\n Hamming mutation completed for %u records\n",  sequenceRecordsWithMetadata->KeyCount);
		}

		/*-------------------------------------------------------------------------------*/
	public:
		// Constructor for mutation engine.
		MutationEngine(const uint32_t distanceClass, const uint32_t currentEditDistance,  const bool isMutationRateEnabled){
			DistanceClass = distanceClass;
			CurrentDistance = currentEditDistance;
			IsMutationRateEnabled = isMutationRateEnabled;	
		}

		void MutateRecords(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, const uint32_t currentEditDistance, const seed_t seed){
			if(DistanceClass == 1){	// Hamming Distance
				// Check if error rate is zero for indels and other mutations. Only contain substitutions. // We need to have indels which are same length.
				// printf("Mutating records for Hamming distance: %u\n", currentEditDistance);
				MutateRecordsForHamming(sequenceRecordsWithMetadata, currentEditDistance, seed);
			}
			else if(DistanceClass == 2){	// Jaccard Distance	 // SET type // TYPE 1
				
				// Jaccard distance requires tokenization and set operations.
				const uint32_t tokenlength = GetLSHTokenLength();  		// Get runtime token length
				if(tokenlength == 0){
					fprintf(stderr, "ERROR: Jaccard distance requires non-zero token length for tokenization.\n");
					exit(EXIT_FAILURE);
				}
				// Initialize Jaccard metadata if not already done
				sequenceRecordsWithMetadata->JaccardSetCardinality = (sequenceRecordsWithMetadata->OriginalSequenceLength + tokenlength - 1)/tokenlength;	//This handles padding.
				sequenceRecordsWithMetadata->JaccardTokenSize = tokenlength;

				// printf("Mutating records for Jaccard distance: %u\n", currentEditDistance);
				MutateRecordsForJaccard(sequenceRecordsWithMetadata, currentEditDistance, seed);
			}
			else if(DistanceClass == 3){	// Levenshtein Distance
				// For future: Implement Levenshtein distance mutations with indels.
				printf("Levenshtein distance mutation not implemented yet.\n");
			}
			else{
				fprintf(stderr, "ERROR: Unknown Distance Class %u\n", DistanceClass);
				exit(EXIT_FAILURE);
			}
		}
};





// class MutationEngine{
// 	private:
// 		float TotalErrorRate;
// 		// float SubstitutionRate;
// 		// float InsertionRate;
// 		// float DeletionRate;
		
// 		uint32_t CurrentEditDistance;
// 		bool IsMutationRate;
// 	public:
// 		
// 		MutationEngine(const uint32_t currentEditDistance, const bool isMutationRateEnabled):CurrentEditDistance(currentEditDistance), IsMutationRate(isMutationRateEnabled) {
			
// 		}

// 		uint8_t mutate_base(uint8_t original_base, Rand &rmutatebase) {
// 			assert(original_base < NUM_BASES);
// 			uint8_t new_base;
// 			do {
// 				new_base = rmutatebase.rand_range(NUM_BASES); // Random base from 0 to NUM_BASES-1
// 			} while (new_base == original_base); // Ensure it's different from the original
// 			return new_base;
// 		}

// 		void MutateRecordsWithSubstitutionsHamming(SequenceRecordSubstitutionOnly *records, const uint32_t currentEditDistance, const seed_t seed) {
			
// 			assert(records != nullptr);
// 			assert(currentEditDistance <= records->SequenceLength);

// 			// printf("MutateRecordsWithSubstitutions: SequenceLength=%d, KeyCount=%d, EditDistance=%d, IsMutationRate=%d\n", records->SequenceLength, records->KeyCount, currentEditDistance, IsMutationRate);
			
			
// 			if(IsMutationRate){

// 			}else{
// 				// If mutation rate is not enabled, we need to handle absolute number of mutations.
// 				assert(currentEditDistance <= records->SequenceLength);

// 				std::vector<int> indices(records->SequenceLength);
				
// 				Rand r(seed, sizeof(uint32_t)); // Unique seed per record. Used for shuffling indices.
// 				Rand rmutatebase( seed + 1, sizeof(uint8_t)); // For mutating bases. used in rmutatebase function.

// 				for(uint32_t record_idx=0; record_idx < records->KeyCount; record_idx++){
// 					// For each record, we need to mutate 'currentEditDistance' number of positions.
// 					std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ... , SequenceLength

// 					// Fisher-Yates shuffle using SMHasher3 Rand
// 					int n = static_cast<int>(records->SequenceLength);
// 					for (int i = (n - 1); i > 0; i--) {
// 						int j = r.rand_range(i + 1); // Random index from 0 to i
// 						std::swap(indices[i], indices[j]);
// 					}
					
// 					// print indices
// 					// for(uint32_t idx=0; idx < records->SequenceLength; idx++){
// 					// 	std::cout << indices[idx] << " ";
// 					// }
// 					// std::cout << std::endl;
            
// 					// Mutate the sequence at these indices
// 					for(uint32_t i = 0; i < currentEditDistance; i++){	
// 						int pos = indices[i];
// 						// printf("%d,%d,%d:%d:%d:%d\n",currentEditDistance,i,records->SequenceLength,record_idx, pos, record_idx * records->KeyBytes + (pos * two_bit_per_base) / 8);
						
// 						uint8_t original_base = (records->SeqTwoBitMut[(record_idx * records->KeyBytes) + ((pos * two_bit_per_base) / 8)] >> (2 * (pos % 4))) & 0x03; // Extract the 2-bit base
						
// 						uint8_t new_base = mutate_base(original_base, rmutatebase);

// 						records->SeqTwoBitMut[(record_idx * records->KeyBytes) + ((pos * two_bit_per_base) / 8)] &= static_cast<uint8_t>(~(0x03 << (2 * (pos % 4)))); // Clearing the original base
// 						records->SeqTwoBitMut[(record_idx * records->KeyBytes) + ((pos * two_bit_per_base) / 8)] |= static_cast<uint8_t>((new_base << (2 * (pos % 4)))); // Setting the new base

// 						// std::cout << "Record " << record_idx << ", Position " << pos << ": " << bases[original_base] << " -> " << bases[new_base] << std::endl;
// 						// for(int idx=0; idx < SequenceLength; idx++){
// 						// 	uint8_t base = (records->SeqTwoBitMut[record_idx * KeyBytes + (idx * two_bit_per_base) / 8] >> (2 * (idx % 4))) & 0x03; // Extract the 2-bit base
// 						// 	std::cout << bases[base];
// 						// }
// 					}
// 					records->EditDistance[record_idx] = currentEditDistance;
// 				}
// 			}
// 		}

// 		// void MutateRecordsWithSubstitutionsJaccard(SequenceRecordSubstitutionOnly *records, const uint32_t currentEditDistance, const seed_t seed) {
			
// 		// 	assert(records != nullptr);
// 		// 	records->SetCardinality = records->SequenceLength / records->TokenSize; // For Jaccard distance, set cardinality is sequence length / token size.
			
// 		// 	assert((currentEditDistance % 2 == 0) && "Jaccard distance requires even edit distance");
// 		// 	assert((currentEditDistance <= 2*(records->SetCardinality)) && "Edit distance cannot exceed set cardinality");
			
// 		// 	// For Jaccard distance:
// 		// 	// - We have two sets A and B with equal cardinality
// 		// 	// - Edit distance = number of elements to swap between sets
			
// 		// 	if (IsMutationRate) {
// 		// 		// Handle mutation rate case if needed
// 		// 	} else {
				
// 		// 		Rand r(seed, sizeof(uint32_t));

// 		// 		std::vector<int> indices(records->SetCardinality);

				
// 		// 		for (uint32_t record_idx = 0; record_idx < records->KeyCount; record_idx++) {
// 		// 			// Fill indices with 0, 1, 2, ..., SetCardinality-1
// 		// 			std::iota(indices.begin(), indices.end(), 0);
					
// 		// 			// Fisher-Yates shuffle
// 		// 			int n = static_cast<int>(records->SetCardinality);
// 		// 			for (int i = n - 1; i > 0; i--) {
// 		// 				int j = r.rand_range(i + 1);
// 		// 				std::swap(indices[i], indices[j]);
// 		// 			}
					
// 		// 			// For Jaccard distance, we need to:
// 		// 			// 1. Select currentEditDistance/2 elements from set A
// 		// 			// 2. Replace them with new elements (not in A or B)
// 		// 			// 3. Select currentEditDistance/2 elements from set B
// 		// 			// 4. Replace them with new elements (not in A or B)
// 		// 			//
// 		// 			// This ensures: |A ∩ B| decreases by currentEditDistance
// 		// 			//               |A ∪ B| increases by currentEditDistance
					
// 		// 			uint32_t elementsToChange = currentEditDistance / 2;
					
// 		// 			// Process first half: change elements in set A
// 		// 			for (uint32_t i = 0; i < elementsToChange; i++) {
// 		// 				int element_idx = indices[i];
						
// 		// 				// Calculate byte offset for this element in set A
// 		// 				// Each element is TokenSize bases long
// 		// 				uint32_t element_start_base = element_idx * records->TokenSize;
						
// 		// 				// Mutate all bases in this token to create a new unique element
// 		// 				for (uint32_t base_offset = 0; base_offset < records->TokenSize; base_offset++) {
// 		// 					uint32_t pos = element_start_base + base_offset;
							
// 		// 					// Calculate bit position and byte index
// 		// 					uint32_t global_bit_pos = pos * two_bit_per_base;
// 		// 					size_t byte_index = (record_idx * records->SetCardinality * records->KeyBytes) + 
// 		// 									(global_bit_pos / 8);
// 		// 					uint32_t bit_offset = global_bit_pos % 8;
							
// 		// 					// Bounds check
// 		// 					if (byte_index >= records->SeqTwoBitMut.size()) {
// 		// 						fprintf(stderr, "ERROR: byte_index %zu out of bounds\n", byte_index);
// 		// 						continue;
// 		// 					}
							
// 		// 					// Extract original base
// 		// 					uint8_t original_base = (records->SeqTwoBitMut[byte_index] >> bit_offset) & 0x03;
							
// 		// 					// Mutate to a different base
// 		// 					uint8_t new_base = mutate_base(original_base, rmutatebase);
							
// 		// 					// Clear and set new base
// 		// 					records->SeqTwoBitMut[byte_index] &= static_cast<uint8_t>(~(0x03 << bit_offset));
// 		// 					records->SeqTwoBitMut[byte_index] |= static_cast<uint8_t>((new_base & 0x03) << bit_offset);
// 		// 				}
// 		// 			}
					
// 		// 			records->EditDistance[record_idx] = currentEditDistance;
// 		// 		}
// 		// 	}
// 		// }
// };




// // Creating Hamming distance records
// SequenceRecordSubstitutionOnly records;
// records.base.DistanceClass = 1;
// records.base.SequenceLength = 128;
// records.base.KeyCount = 1000;
// records.distanceMetadata = HammingMetadata{};

// // Creating Jaccard distance records
// SequenceRecordSubstitutionOnly jaccardRecords;
// jaccardRecords.base.DistanceClass = 2;
// jaccardRecords.base.SequenceLength = 128;
// jaccardRecords.base.KeyCount = 1000;
// jaccardRecords.distanceMetadata = JaccardMetadata{
//     .SetCardinality = 16,
//     .TokenSize = 8
// };

// // Access pattern
// if (auto* jaccard = records.asJaccard()) {
//     uint32_t cardinality = jaccard->SetCardinality;
//     // Work with Jaccard-specific data
// }




// 	Rand r( seed, records->KeyBytes );
	// 	RandSeq rs = r.get_seq(SEQ_DIST_1, records->KeyBytes);

	// 	if(records->DistanceClass == 1){
	// 		rs.write(&(records->SeqTwoBitOrg[0]), 1, records->KeyCount);
	// 	}
	// 	else if(records->DistanceClass == 2){
	// 		records->SetCardinality = static_cast<uint32_t>(records->SequenceLength/records->TokenSize); // For Jaccard distance, set cardinality is sequence length / token size.
	// 		rs.write(&(records->SeqTwoBitOrg[0]), 1, (records->SetCardinality * records->KeyCount));
	// 	}
	// 	records->SeqTwoBitMut = records->SeqTwoBitOrg;
		
	// 	// for(int i=0; i < std::min(5u, records->KeyCount); i++){
	// 	// 	std::cout << "Original Sequence " << i << ": ";
	// 	// 	for(int j=0; j < records->SequenceLength; j++){
	// 	// 		uint8_t base = (records->SeqTwoBitOrg[i * records->KeyBytes + (j * two_bit_per_base) / 8] >> (2 * (j % 4))) & 0x03;
	// 	// 		std::cout << bases[base];
	// 	// 	}
	// 	// 	std::cout << std::endl;
	// 	// }
	// }