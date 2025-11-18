



// static const char bases[5] = {'A', 'C', 'G', 'T', 'N'};	// Including ambiguous base 'N' for random sequence generation




template <typename hashtype>  // Add hashtype template parameter
class MutationEngine {
	private:
		
		
	public:

		MutationEngine(uint32_t sequenceLength, uint32_t currentEditDistance, seed_t seed, bool isMutationRateEnabled = false, uint32_t keyCount = 1024):SequenceLength(sequenceLength), 
			KeyCount(keyCount), CurrentEditDistance(currentEditDistance), IsMutationRate(isMutationRateEnabled), Seed(seed) {
				assert(sequenceLength > 0);
				assert(sequenceLength > 13);
				assert(currentEditDistance >= 0);

			const uint32_t NumBitsPerSequence = (int)(two_bit_per_base) * sequenceLength;
			const uint32_t NumBytesPerSequence = (NumBitsPerSequence + 7) / 8; // Round up to nearest byte
			KeyBytes = NumBytesPerSequence;
		}
		

		void MutateRecordsWithSubstitutions(SequenceRecordSubstitutionOnly *records, uint32_t editDistance) {
			assert(records != nullptr);
			assert(editDistance >= 0 && editDistance <= SequenceLength);

			printf("MutateRecordsWithSubstitutions: SequenceLength=%d, KeyCount=%d, EditDistance=%d, IsMutationRate=%d\n", 
				SequenceLength, KeyCount, editDistance, IsMutationRate);
			
			
			if(IsMutationRate){

			}else{
				// If mutation rate is not enabled, we need to handle absolute number of mutations.
				assert(editDistance >= 0 && editDistance <= SequenceLength);

				std::vector<int> indices(SequenceLength+1);
				// std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ...
				
				// Using the SMHasher3 Rand for shuffling
				Rand r(84574 + 3, sizeof(uint32_t)); // Unique seed per record	// we may need to move it outside the loop for better performance.
				Rand rmutatebase( 84574 + 7, sizeof(uint8_t) ); 

				for(uint32_t record_idx=0; record_idx < KeyCount; record_idx++){
					// For each record, we need to mutate 'editDistance' number of positions.
					
					std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ...

					// Fisher-Yates shuffle using SMHasher3 Rand
					for (int i = SequenceLength; i > 0; i--) {
						int j = r.rand_range(i + 1); // Random index from 0 to i
						std::swap(indices[i], indices[j]);
					}

					// print indices
					for(uint32_t idx=0; idx < SequenceLength; idx++){
						std::cout << indices[idx] << " ";
					}
					std::cout << std::endl;
            
					// Mutate the sequence at these indices
					for(uint32_t i = 0; i < editDistance; i++){
						int pos = indices[i];
						printf("%d,%d,%d:%d:%d:%d\n",editDistance,i,SequenceLength,record_idx, pos, record_idx * KeyBytes + (pos * two_bit_per_base) / 8);
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

		

		// void print_sequence(const std::vector<uint8_t> &seq, uint32_t seqLength) {
		// 	for (uint32_t i = 0; i < seqLength; i++) {
		// 		uint8_t base = (seq[i / 4] >> (2 * (i % 4))) & 0x03; // Extract the 2-bit base
		// 		std::cout << bases[base];
		// 	}
		// 	std::cout << std::endl;
		// }
};
