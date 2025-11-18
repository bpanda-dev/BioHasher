

				

				

					
					// print indices
					for(uint32_t idx=0; idx < records->SetCardinality; idx++){
						std::cout << indices[idx] << " ";
					}
					std::cout << std::endl;

					


				}

				
				// std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ...
				
				// Using the SMHasher3 Rand for shuffling
				Rand r(84574 + 3, sizeof(uint32_t)); // Unique seed per record	// we may need to move it outside the loop for better performance.
				Rand rmutatebase( 84574 + 7, sizeof(uint8_t) ); 

				for(uint32_t record_idx=0; record_idx < KeyCount; record_idx++){
					// For each record, we need to mutate 'editDistance' number of positions.
					
					

					

					
            
					// Mutate the sequence at these indices
					for(uint32_t i = 0; i < editDistance; i++){
						
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