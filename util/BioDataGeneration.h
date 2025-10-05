#pragma once

#include <iostream>
#include <vector>
#include <cassert>
#include <random>
#include <numeric>


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


static const char bases[4] = {'A', 'C', 'G', 'T'};
static constexpr size_t NUM_BASES = sizeof(bases)/sizeof(bases[0]);

// static const char bases[5] = {'A', 'C', 'G', 'T', 'N'};	// Including ambiguous base 'N' for random sequence generation


// template <typename hashtype>	// SOA version. More cache friendly. => Better performance.
struct SequenceRecord {
	uint32_t SeqLength;		// Length of the sequence
	uint32_t DistanceClass;	// the type of distance being used 

	std::vector<std::string> SeqOriginal;	// x1 Sequences. {A,T,G,C}
	std::vector<uint8_t> SeqTwoBitOrg;     	// x1 Sequences in 2-bit encoding. {0,1}
	std::vector<uint8_t> SeqTwoBitMut;		// x1 Sequences in 2-bit encoding. {0,1}
    std::vector<uint32_t> EditDistance;		// x1 Edit distance between the original and mutated sequence
    // std::vector<hashtype> Hash;         	// x2 Hash values. <HASHTYPE> 
    std::vector<uint32_t> HashDistance; 	// x1 Hamming distance between the two hash values
};




template <typename hashtype>
class DataGeneration{
	private:
		uint32_t SequenceLength;
		uint32_t KeyBytes;
		uint32_t KeyCount;				// Num Trials
		
	public:

		// Constructor for simulated data generation module.
		DataGeneration(SequenceRecord *records, uint32_t sequenceLength, uint32_t keyCount, seed_t seed): 
			SequenceLength(sequenceLength), KeyCount(keyCount) {  
			assert(records != nullptr);
			assert(sequenceLength > 0);
			assert(keyCount > 0);
			
			// Generate random sequences.
			records->SeqLength = sequenceLength;
			records->DistanceClass = 0; // Default distance class. Hamming distance
			
			const uint32_t NumBitsPerSequence = (int)(two_bit_per_base) * sequenceLength;
			const uint32_t NumBytesPerSequence = (NumBitsPerSequence + 7) / 8; // Round up to nearest byte

			records->SeqTwoBitOrg.resize(KeyCount * NumBytesPerSequence);
			records->SeqTwoBitMut.resize(KeyCount * NumBytesPerSequence);
			records->EditDistance.resize(KeyCount);
			// records->Hash.resize(2 * KeyCount); 
			records->HashDistance.resize(KeyCount);
			KeyBytes = NumBytesPerSequence;

			// std::cout << "DataGeneration: SequenceLength = " << SequenceLength << ", KeyCount = " << KeyCount << ", KeyBytes = " << KeyBytes << std::endl;
		}

		// Fill the records with random sequences and their mutated versions.
		void FillRecords(SequenceRecord *records, seed_t seed){
			assert(records != nullptr);

			// Rand r( 84574, KeyBytes );
			Rand r( seed, KeyBytes );
			RandSeq rs = r.get_seq(SEQ_DIST_1, KeyBytes);
			rs.write(records->SeqTwoBitOrg.data(), 0, KeyCount);
			
			records->SeqTwoBitMut = records->SeqTwoBitOrg;

			// Print first 5 sequences for verification
			// for(int i=0; i < std::min(5u, KeyCount); i++){
			// 	std::cout << "Original Sequence " << i << ": ";
			// 	for(int j=0; j < SequenceLength; j++){
			// 		uint8_t base = (records->SeqTwoBitOrg[i * KeyBytes + (j * two_bit_per_base) / 8] >> (2 * (j % 4))) & 0x03;
			// 		std::cout << bases[base];
			// 	}
			// 	std::cout << std::endl;
			// }
		}
};

template <typename hashtype>  // Add hashtype template parameter
class MutationEngine {
	private:
		uint32_t SequenceLength;
		uint32_t KeyBytes;
		uint32_t KeyCount;
		float TotalErrorRate;
		// float SubstitutionRate;
		// float InsertionRate;
		// float DeletionRate;
		float CurrentErrorRate;
		bool IsMutationRate;

	public:

		MutationEngine(uint32_t sequenceLength, float currentErrorRate, seed_t seed, bool isMutationRateEnabled = false, uint32_t keyCount = 1024):SequenceLength(sequenceLength), 
			KeyCount(keyCount), CurrentErrorRate(currentErrorRate), IsMutationRate(isMutationRateEnabled) {
				assert(sequenceLength > 0);
				assert(sequenceLength > 13);
				assert(currentErrorRate >= 0.0 && currentErrorRate <= 1.0);

			const uint32_t NumBitsPerSequence = (int)(two_bit_per_base) * sequenceLength;
			const uint32_t NumBytesPerSequence = (NumBitsPerSequence + 7) / 8; // Round up to nearest byte
			KeyBytes = NumBytesPerSequence;
		}
		

		void MutateRecordsWithSubstitutions(SequenceRecord *records, uint32_t editDistance) {
			assert(records != nullptr);
			assert(editDistance >= 0 && editDistance <= SequenceLength);
			
			
			if(IsMutationRate){

			}else{
				// If mutation rate is not enabled, we need to handle absolute number of mutations.
				assert(editDistance >= 0 && editDistance <= SequenceLength);

				std::vector<int> indices(SequenceLength);
				// std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ...
				
				// Using the SMHasher3 Rand for shuffling
				Rand r(84574 + 3, sizeof(uint32_t)); // Unique seed per record	// we may need to move it outside the loop for better performance.
				Rand rmutatebase( 84574 + 7, sizeof(uint8_t) ); 

				for(uint32_t record_idx=0; record_idx < KeyCount; record_idx++){
					// For each record, we need to mutate 'editDistance' number of positions.
					
					std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ...

					// Fisher-Yates shuffle using SMHasher3 Rand
					for (int i = SequenceLength - 1; i > 0; i--) {
						int j = r.rand_range(i + 1); // Random index from 0 to i
						std::swap(indices[i], indices[j]);
					}

					//print indices
					// for(int idx=0; idx < SequenceLength; idx++){
					// 	std::cout << indices[idx] << " ";
					// }
					// std::cout << std::endl;
            
					// Mutate the sequence at these indices
					for(uint32_t i = 0; i < editDistance; i++){
						int pos = indices[i];
						uint8_t original_base = (records->SeqTwoBitMut[record_idx * KeyBytes + (pos * two_bit_per_base) / 8] >> (2 * (pos % 4))) & 0x03; // Extract the 2-bit base
						uint8_t new_base = mutate_base(original_base, rmutatebase);
						records->SeqTwoBitMut[record_idx * KeyBytes + (pos * two_bit_per_base) / 8] &= ~(0x03 << (2 * (pos % 4))); // Clearing the original base
						records->SeqTwoBitMut[record_idx * KeyBytes + (pos * two_bit_per_base) / 8] |= (new_base << (2 * (pos % 4))); // Setting the new base
						// std::cout << "Record " << record_idx << ", Position " << pos << ": " << bases[original_base] << " -> " << bases[new_base] << std::endl;
						// for(int idx=0; idx < SequenceLength; idx++){
						// 	uint8_t base = (records->SeqTwoBitMut[record_idx * KeyBytes + (idx * two_bit_per_base) / 8] >> (2 * (idx % 4))) & 0x03; // Extract the 2-bit base
						// 	std::cout << bases[base];
						// }
						
					}
					records->EditDistance[record_idx] = editDistance;
				}
			}

			// Print first 5 sequences for verification
			// for(int i=0; i < std::min(5u, KeyCount); i++){
			// 	std::cout << "Original Sequence " << i << ": ";
			// 	for(int j=0; j < SequenceLength; j++){
			// 		uint8_t base = (records->SeqTwoBitOrg[i * KeyBytes + (j * two_bit_per_base) / 8] >> (2 * (j % 4))) & 0x03;
			// 		std::cout << bases[base];
			// 	}
			// 	std::cout << std::endl;
			// }
			// // Print first 5 sequences for verification
			// for(int i=0; i < std::min(5u, KeyCount); i++){
			// 	std::cout << "Mutated Sequence " << i << ": ";
			// 	for(int j=0; j < SequenceLength; j++){
			// 		uint8_t base = (records->SeqTwoBitMut[i * KeyBytes + (j * two_bit_per_base) / 8] >> (2 * (j % 4))) & 0x03;
			// 		std::cout << bases[base];
			// 	}
			// 	std::cout << std::endl;
			// }
				
		}

		uint8_t mutate_base(uint8_t original_base, Rand &rmutatebase) {
			assert(original_base < NUM_BASES);
			uint8_t new_base;
			do {
				new_base = rmutatebase.rand_range(NUM_BASES); // Random base from 0 to NUM_BASES-1
			} while (new_base == original_base); // Ensure it's different from the original
			return new_base;
		}

		// void print_sequence(const std::vector<uint8_t> &seq, uint32_t seqLength) {
		// 	for (uint32_t i = 0; i < seqLength; i++) {
		// 		uint8_t base = (seq[i / 4] >> (2 * (i % 4))) & 0x03; // Extract the 2-bit base
		// 		std::cout << bases[base];
		// 	}
		// 	std::cout << std::endl;
		// }
};
