
#include "BioDataGeneration.h"

#include "Platform.h"
#include "Random.h"

bool simulateSNP(SequenceRecordUnit &record, const uint32_t pos, Rand rng) {
	
	char currentBase = record.SeqASCIIMut[pos];
	uint8_t currentBaseid = (currentBase >> 1) & 0x03; // Map ASCII to 2-bit (A=0,C=1,T=2,G=3)

	// printf("SNP at pos %u: current base %c (id %u)\n", pos, currentBase, currentBaseid);

	// Randomly select a new base
	// Generate a different base (ensure it's not the same as original)
	uint32_t newBaseid_int = rng.rand_range(2);
	uint8_t newBaseid = newBaseid_int & 0x03;
	if (currentBaseid <= newBaseid)
		newBaseid += 1;

	assert(currentBaseid != newBaseid && "New base should be different from current base");
	assert(newBaseid <= 3 && "Base ID should be between 0 and 3");

	// Apply SNP
	record.SeqASCIIMut[pos] = bases[newBaseid];
	
	return true;
}

double ComputeHammingSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t length) {
    // Hamming distance only defined for equal-length sequences
    uint32_t similar = 0;
	// Count mismatches in overlapping region
	for (uint32_t i = 0; i < length; i++) {
		if (seq1[i] == seq2[i]) {
			similar++;
		}
	}
	return (static_cast<float>(similar) / static_cast<float>(length));
}

std::set<std::string> get_kmers(const std::string& sequence, int k) {
    std::set<std::string> kmers;
    
    // Ensure the sequence is long enough for at least one k-mer
    if (sequence.length() < static_cast<size_t>(k)) {
        return kmers;
    }

    // Slide the window across the sequence
    for (size_t i = 0; i <= sequence.length() - k; ++i) {
        kmers.insert(sequence.substr(i, k));
    }
    
    return kmers;
}

double ComputeJaccardSimilarity(const std::string& seq1, const std::string& seq2, int k) {
    // 1. Tokenize sequences into sets of k-mers
    std::set<std::string> set1 = get_kmers(seq1, k);
    std::set<std::string> set2 = get_kmers(seq2, k);

	// printf("Computing Jaccard Similarity for k=%d\n", k);

	// printf("Set1 size: %zu, Set2 size: %zu\n", set1.size(), set2.size());
	// //print the sets for debugging
	// // --- Printing Section ---
    // std::cout << "--- K-mer Analysis (k=" << k << ") ---" << std::endl;
    // std::cout << "Set1 (" << set1.size() << " unique): ";
    // for (const auto& kmer : set1) std::cout << kmer << " ";
    
    // std::cout << "\nSet2 (" << set2.size() << " unique): ";
    // for (const auto& kmer : set2) std::cout << kmer << " ";
    // std::cout << "\n" << std::endl;


    if (set1.empty() || set2.empty()) return 0.0;

    // 2. Find the intersection
    std::vector<std::string> intersect;
    std::set_intersection(set1.begin(), set1.end(),
                          set2.begin(), set2.end(),
                          std::back_inserter(intersect));

    // 3. Use the Inclusion-Exclusion principle for the Union size
    // |A ∪ B| = |A| + |B| - |A ∩ B|
    size_t intersection_size = intersect.size();
    size_t union_size = set1.size() + set2.size() - intersection_size;

    return static_cast<double>(intersection_size) / union_size;
}

double ComputeCosineSimilarity(const std::string& seq1, const std::string& seq2, int k) {
	// 1. Tokenize sequences into sets of k-mers
	std::set<std::string> set1 = get_kmers(seq1, k);
	std::set<std::string> set2 = get_kmers(seq2, k);

	if (set1.empty() || set2.empty()) return 0.0;

	// 2. Find the intersection
	std::vector<std::string> intersect;
	std::set_intersection(set1.begin(), set1.end(),
						  set2.begin(), set2.end(),
						  std::back_inserter(intersect));

	size_t intersection_size = intersect.size();

	// Cosine similarity = |A ∩ B| / sqrt(|A| * |B|)
	return static_cast<double>(intersection_size) / 
		   std::sqrt(static_cast<double>(set1.size()) * static_cast<double>(set2.size()));
}

double ComputeAngularSimilarity(const std::string& seq1, const std::string& seq2, int k) {
	double cosine_sim = ComputeCosineSimilarity(seq1, seq2, k);
	// Angular similarity = 1 - (angle / π) where angle = arccos(cosine_similarity)
	return 1 - (std::acos(cosine_sim) / M_PI);
}

UnionBitVectorsStruct CreateUnionBitVectors(const std::string& seq1, const std::string& seq2, int k) {
	// Tokenize sequences into sets of k-mers
	std::set<std::string> kmer_set_a = get_kmers(seq1, k);
	std::set<std::string> kmer_set_b = get_kmers(seq2, k);

	if (kmer_set_a.empty() || kmer_set_b.empty()) return {std::vector<char>(), std::vector<char>(), std::vector<std::string>()};	//return empty vectors if either set is empty

    // 1. Create the Universe (Union of both sets)
    // std::set_union requires sorted ranges. std::set is already sorted.
    std::vector<std::string> union_kmers;
    std::set_union(kmer_set_a.begin(), kmer_set_a.end(),
                   kmer_set_b.begin(), kmer_set_b.end(),
                   std::back_inserter(union_kmers));

    // 2. Pre-allocate vectors
    size_t n = union_kmers.size();
	std::vector<char> vec_a(n, '0');
	std::vector<char> vec_b(n, '0');

    // 3. Fill the vectors
    for (size_t i = 0; i < n; ++i) {
        const std::string& kmer = union_kmers[i];
        
        // Use set::count to check for existence (O(log N))
        if (kmer_set_a.count(kmer)) {
            vec_a[i] = '1';
        }
        if (kmer_set_b.count(kmer)) {
            vec_b[i] = '1';
        }
    }

    return {std::move(vec_a), std::move(vec_b), std::vector<std::string>()};
}

/*#-----------------------------------------------------#*/
/*#					Data Generation						#*/
/*#-----------------------------------------------------#*/

SequenceDataGenerator::SequenceDataGenerator(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata){
	assert(sequenceRecordsWithMetadata != nullptr);
	assert(((sequenceRecordsWithMetadata->A_percentage + sequenceRecordsWithMetadata->C_percentage  + sequenceRecordsWithMetadata->G_percentage + sequenceRecordsWithMetadata->T_percentage) == 1) && "Sum of nucleotide probs. should be 1.");
	assert((sequenceRecordsWithMetadata->OriginalSequenceLength > 0) && "Sequence length must be positive");
	assert(sequenceRecordsWithMetadata->KeyCount>0);

	// Initialise seed and reserve memory.
    Rand rng(sequenceRecordsWithMetadata->DatagenSeed);

	if(sequenceRecordsWithMetadata->isBasesDrawnFromUniformDist == false){
		// If not drawn from uniform distribution, we can set custom base percentages here.
		sequenceRecordsWithMetadata->A_percentage = 0.30;
		sequenceRecordsWithMetadata->C_percentage = 0.20;
		sequenceRecordsWithMetadata->G_percentage = 0.20;
		sequenceRecordsWithMetadata->T_percentage = 0.30;
	}
	else{	// Need to mention this path of branch too, since we have an assertion for sum of percentages.
		sequenceRecordsWithMetadata->A_percentage = 0.25;
		sequenceRecordsWithMetadata->C_percentage = 0.25;
		sequenceRecordsWithMetadata->G_percentage = 0.25;
		sequenceRecordsWithMetadata->T_percentage = 0.25;
	}

    sequenceRecordsWithMetadata->Records.resize(sequenceRecordsWithMetadata->KeyCount);	// Allocate memory for sequence records

    uint32_t total_bases = sequenceRecordsWithMetadata->OriginalSequenceLength * sequenceRecordsWithMetadata->KeyCount;
    std::vector<uint8_t> rand_data(total_bases, 0);
    rng.rand_n(&rand_data[0], total_bases);
	
	if(sequenceRecordsWithMetadata->isBasesDrawnFromUniformDist == 1){
		for (uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
			auto& record = sequenceRecordsWithMetadata->Records[rec_idx];

			// setting the size of the sequences
			record.OriginalLength = sequenceRecordsWithMetadata->OriginalSequenceLength;

			record.SeqASCIIOrg.resize(sequenceRecordsWithMetadata->OriginalSequenceLength, 'A');

			for (uint32_t pos = 0; pos < sequenceRecordsWithMetadata->OriginalSequenceLength; pos++) {
				
				uint8_t base = rand_data[(rec_idx * sequenceRecordsWithMetadata->OriginalSequenceLength) + pos] % 4;
				
				record.SeqASCIIOrg[pos] = bases[base];
			}

			/* Debugging Statements! Please never delete!*/
			// Print first 10 sequences for verification
			// if (rec_idx < 10) {
			// 	std::cout << "Record " << rec_idx << " Original Sequence: " << record.SeqASCIIOrg << std::endl;
			// }
		}
	}
	else{
		// Precompute cumulative probabilities
		std::vector<double> cumulative_probs = {
			sequenceRecordsWithMetadata->A_percentage,
			sequenceRecordsWithMetadata->A_percentage + sequenceRecordsWithMetadata->C_percentage,
			sequenceRecordsWithMetadata->A_percentage + sequenceRecordsWithMetadata->C_percentage + sequenceRecordsWithMetadata->G_percentage,
			1.0 // T percentage
		};

		for (uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
			auto& record = sequenceRecordsWithMetadata->Records[rec_idx];

			// setting the size of the sequences
			record.OriginalLength = sequenceRecordsWithMetadata->OriginalSequenceLength;

			record.SeqASCIIOrg.resize(sequenceRecordsWithMetadata->OriginalSequenceLength, 'A');

			for (uint32_t pos = 0; pos < sequenceRecordsWithMetadata->OriginalSequenceLength; pos++) {
				
				double rand_val = static_cast<double>(rand_data[(rec_idx * sequenceRecordsWithMetadata->OriginalSequenceLength) + pos]) / 256.0;	//CONVERTED INTO 0 TO 1

				uint8_t base = 0;
				if (rand_val < cumulative_probs[0]) {
					base = 0; // A
				} else if (rand_val < cumulative_probs[1]) {
					base = 1; // C
				} else if (rand_val < cumulative_probs[2]) {
					base = 3; // G
				} else {
					base = 2; // T
				}
				
				record.SeqASCIIOrg[pos] = bases[base];
				
			}

			/* Debugging Statements! Please never delete!*/
			// Print first 10 sequences for verification
			// if (rec_idx < 10) {
			// 	std::cout << "Record " << rec_idx << " Original Sequence: " << record.SeqASCIIOrg << std::endl;
			// }
		}
	}
    

	std::cout << "DataGeneration: SequenceLength = " << sequenceRecordsWithMetadata->OriginalSequenceLength << ", KeyCount = " << sequenceRecordsWithMetadata->KeyCount << std::endl;
}

void SequenceDataGenerator::decodeSequencesToASCII(const std::vector<uint8_t>& seqTwoBit, std::string& seqASCII, const uint32_t sequenceLength) {
    assert(seqASCII.size() == sequenceLength);
    for (uint32_t pos = 0; pos < sequenceLength; pos++) {
        uint32_t byte_idx = (pos * two_bit_per_base) / 8;
        uint32_t bit_offset = (pos * two_bit_per_base) % 8;
        uint8_t base = (seqTwoBit[byte_idx] >> bit_offset) & 0x03;
        seqASCII[pos] = bases[base];
    }
}

/*#-----------------------------------------------------#*/
/*#					Data Mutation						#*/
/*#-----------------------------------------------------#*/
SequenceDataMutatorSubstitutionOnly::SequenceDataMutatorSubstitutionOnly(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata){
	assert(sequenceRecordsWithMetadata != nullptr);
	assert(sequenceRecordsWithMetadata->Records.size() > 0);

	// Initialise seed and reserve memory.
    Rand rng(sequenceRecordsWithMetadata->DataMutateSeed);

	for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {

		auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
		assert((record.snpRate >= 0.0) && (record.snpRate <= 1.0) && "SNP rate should be between 0 and 1");
	
		// Initialising the mutated sequences with original sequences
		record.MutatedLength = record.OriginalLength;
		record.SeqASCIIMut.resize(record.MutatedLength, 'A');
		record.SeqASCIIMut = record.SeqASCIIOrg;	// Copy original ASCII sequence

		// NOTE: Use a while loop or track position carefully since length changes
		for (int pos = record.OriginalLength - 1; pos >= 0; --pos) {
			uint32_t rand_val = rng.rand_range(1000);
			double sample = static_cast<double>(rand_val) / 1000.0;	// snp or indel
			bool isSnp = (sample < record.snpRate);
			if (isSnp) {
                simulateSNP(record, pos, rng);
            }
        }
	}

	if(sequenceRecordsWithMetadata->DistanceClass == 1){	// Hamming
		for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
			auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
			record.similarity = ComputeHammingSimilarity(record.SeqASCIIOrg, record.SeqASCIIMut, record.OriginalLength);
		}
		std::cout << "DataMutation: Applied SNP mutations only. Distance Class = Hamming." << std::endl;
	}
	if(sequenceRecordsWithMetadata->DistanceClass == 2){	// Jaccard
		for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
			auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
			record.similarity = ComputeJaccardSimilarity(record.SeqASCIIOrg, record.SeqASCIIMut, g_TokenLength);
			// printf("Record %u: Jaccard Similarity = %f\n", rec_idx, record.similarity);
		}
		std::cout << "DataMutation: Applied SNP mutations only. Distance Class = Jaccard." << std::endl;
	}
	if(sequenceRecordsWithMetadata->DistanceClass == 3){	// Cosine
		for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
			auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
			record.similarity = ComputeCosineSimilarity(record.SeqASCIIOrg, record.SeqASCIIMut, g_TokenLength);
		}
		std::cout << "DataMutation: Applied SNP mutations only. Distance Class = Cosine." << std::endl;
	}
	if(sequenceRecordsWithMetadata->DistanceClass == 4){	// Angular
		for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
			auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
			record.similarity = ComputeAngularSimilarity(record.SeqASCIIOrg, record.SeqASCIIMut, g_TokenLength);
		}
		std::cout << "DataMutation: Applied SNP mutations only. Distance Class = Angular." << std::endl;
	}

	// Debugging output
    // for(uint32_t rec_idx = 0; rec_idx < std::min(sequenceRecordsWithMetadata->KeyCount, 10u); rec_idx++) {
    //     auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
    //     if (rec_idx < 10) {
    //         std::cout << "\n=== Record " << rec_idx << " ===" << std::endl;
    //         std::cout << "Original Length: " << record.OriginalLength << std::endl;
    //         std::cout << "Mutated Length:  " << record.MutatedLength << std::endl;
    //         std::cout << "Original: " << record.SeqASCIIOrg << std::endl;
    //         std::cout << "Mutated:  " << record.SeqASCIIMut << std::endl;
	// 		std::cout << "SNP Rate: " << record.snpRate << std::endl;
	// 		std::cout << "Similarity: " << record.similarity << std::endl;
    //     }
    // }	
}


// void SequenceDataMutator::ApplyMutations(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata) {
	
// 	//apply snp and indel mutations here
// 	for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
// 		auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
		
// 		// Initialising the mutated sequences with original sequences
// 		record.MutatedLength = record.OriginalLength;
// 		record.SeqTwoBitMut = record.SeqTwoBitOrg;
// 		record.SeqASCIIMut.resize(record.MutatedLength, 'A');
// 		record.SeqASCIIMut = record.SeqASCIIOrg;	// Copy original ASCII sequence
		
// 		record.EditDistance = 0; 


// 		// Since mutation generation is a bit more complex to handle with bits, we will work on non-encoded sequence for mutation simulation.
// 		// After all mutations are applied, we will re-encode the mutated sequence back to 2-bit format.

// 		// NOTE: Use a while loop or track position carefully since length changes
		
// 		for (int pos = record.OriginalLength - 1; pos >= 0; --pos) {
// 			std::uniform_real_distribution<double> dist(0, 1);	// snp or indel
// 			bool isSnp = (dist(RandGen) < sequenceRecordsWithMetadata->snpRate);
//             bool isIndel = (dist(RandGen) < sequenceRecordsWithMetadata->smallIndelRate);

// 			// Conflict resolution
//             int const MAX_TRIES = 1000;
//             int tryNo = 0;
//             for (; isSnp && isIndel && (tryNo < MAX_TRIES); ++tryNo) {
//                 isSnp = (dist(RandGen) < sequenceRecordsWithMetadata->snpRate);
//                 isIndel = (dist(RandGen) < sequenceRecordsWithMetadata->smallIndelRate);
//                 if (pos == 0)
//                     isIndel = false;
//             }
//             if (tryNo == MAX_TRIES)
//                 isSnp = (isIndel == false);

//             if (isSnp) {
//                 simulateSNP(record, pos, RandGen);
//             }
//             else if (isIndel) {
//                 unsigned upos = static_cast<unsigned>(pos);
//                 simulateSmallIndel(record, upos, RandGen, sequenceRecordsWithMetadata);
//             }
//         }
// 	}

//     // Debugging output
//     // for(uint32_t rec_idx = 0; rec_idx < std::min(sequenceRecordsWithMetadata->KeyCount, 10u); rec_idx++) {
//     //     auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
//     //     if (rec_idx < 10) {
//     //         std::cout << "\n=== Record " << rec_idx << " ===" << std::endl;
//     //         std::cout << "Original Length: " << record.OriginalLength << std::endl;
//     //         std::cout << "Mutated Length:  " << record.MutatedLength << std::endl;
//     //         std::cout << "Original: " << record.SeqASCIIOrg << std::endl;
//     //         std::cout << "Mutated:  " << record.SeqASCIIMut << std::endl;
//     //         // std::cout << "\n--- Similarity Metrics ---" << std::endl;
//     //         // std::cout << "Hamming Dissimilarity: " << record.HammingDissimilarity << std::endl;
//     //         // std::cout << "Jaccard Similarity (k=" << tokenSize << "):    " << record.JaccardSimilarity << std::endl;
//     //         // std::cout << "Edit Distance:               " << record.EditDistance << std::endl;
//     //         // std::cout << "Global Alignment Score:      " << record.GlobalAlignmentScore << std::endl;
//     //         // std::cout << "Local Alignment Score:       " << record.LocalAlignmentScore << std::endl;
//     //     }
//     // }

//     // for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++){
//     //     printf("\n%f", sequenceRecordsWithMetadata->Records[rec_idx].JaccardSimilarity);
//     //    fflush(stdout);
//     // }
// }

// uint8_t SequenceDataMutator::GetBaseAtPosition(const std::vector<uint8_t>& seq, uint32_t pos) {
// 	uint32_t byte_idx = (pos * two_bit_per_base) / 8;
// 	uint32_t bit_offset = (pos * two_bit_per_base) % 8;
// 	return (seq[byte_idx] >> bit_offset) & 0x03;
// }

// void SequenceDataMutator::SetBaseAtPosition(std::vector<uint8_t>& seq, uint32_t pos, uint8_t base) {
// 	uint32_t byte_idx = (pos * two_bit_per_base) / 8;
// 	uint32_t bit_offset = (pos * two_bit_per_base) % 8;
// 	seq[byte_idx] &= ~(0x03 << bit_offset);
// 	seq[byte_idx] |= (base << bit_offset);
// }


// bool SequenceDataMutator::simulateSmallIndel(SequenceRecordUnit &record, uint32_t pos, std::mt19937 &randGen, SequenceRecordsWithMetadata *sequenceRecordsWithMetadata) {
	
// 	// Random distributions
//     std::uniform_int_distribution<int> distSize(sequenceRecordsWithMetadata->minSmallIndelSize,
//                                                 sequenceRecordsWithMetadata->maxSmallIndelSize);
//     std::uniform_int_distribution<int> distDeletion(0, 1);	// 0 for insertion, 1 for deletion

// 	int indelSize = distSize(randGen);
// 	bool isDeletion = distDeletion(randGen);

// 	// Validation checks
//     if (isDeletion) {
//         // Check if deletion would go beyond sequence end
//         if (pos + indelSize >= record.MutatedLength)
//             return false;
//     }

// 	if (isDeletion) {
//         // DELETION: Remove bases
// 		simulateDeletion(record, pos, indelSize);
//     } else {
//         // INSERTION: Add random bases 
//         simulateInsertion(record, pos, indelSize, randGen);
//     }

// 	return true;
// }

// void SequenceDataMutator::simulateDeletion(SequenceRecordUnit &record, uint32_t pos, uint32_t deleteSize) {
    
// 	// Working directly with ASCII string
//     // Erase deleteSize characters starting from pos
//     record.SeqASCIIMut.erase(pos, deleteSize);
//     record.MutatedLength -= deleteSize;
// }

// void SequenceDataMutator::simulateInsertion(SequenceRecordUnit &record, uint32_t pos, uint32_t insertSize, std::mt19937 &randGen) {
//     std::uniform_int_distribution<int> distDNA(0, 3);
    
//     // Generate random bases to insert
//     std::string insertSeq;
//     insertSeq.reserve(insertSize);
    
//     for (uint32_t i = 0; i < insertSize; i++) {
//         uint8_t baseId = distDNA(randGen);
//         insertSeq += bases[baseId];  // bases array: {'A', 'C', 'T', 'G'}
//     }
    
//     // Insert the new sequence at position pos
//     record.SeqASCIIMut.insert(pos, insertSeq);
    
//     // Update length
//     record.MutatedLength += insertSize;
// }

// //------------------------------------------------------------------------------------------

// SequenceDataMetrics::SequenceDataMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata) {
//     // uint32_t tokenSize = 7;	//for default non-tokenized case.

//     // Constructor can be used to initialize any required data structures
//     for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++){
        
//         auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
//         record.HammingDissimilarity = 0.0f;
//         record.HammingDistance = 0;
//         record.HammingSimilarity = 1.0f - record.HammingDissimilarity;

//         record.JaccardSimilarity = 0.0f;
//         record.JaccardDissimilarity = 1.0f - record.JaccardSimilarity;
        
//         record.EditDistance = 0; 
//         record.LocalAlignmentScore = 0; //computeLocalAlignment(record.SeqASCIIOrg, record.SeqASCIIMut, 1, -1, -2);
//         record.GlobalAlignmentScore = 0; //computeGlobalAlignment(record.SeqASCIIOrg, record.SeqASCIIMut, 1, -1, -2);
//         record.LocalCIGARString = "";
//         record.GlobalCIGARString = "";

//         record.HashDistance = 0;
//     }
// }

// void SequenceDataMetrics::ComputeHammingMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata) {
//     // Hamming distance only defined for equal-length sequences
//     // For unequal lengths, we'll pad the shorter one conceptually
//     for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++){
//         auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
//         uint32_t minLen = std::min(record.SeqASCIIOrg.length(), record.SeqASCIIMut.length());
//         uint32_t maxLen = std::max(record.SeqASCIIOrg.length(), record.SeqASCIIMut.length());

//         if (maxLen == 0){
//             record.HammingDissimilarity = 0.0f;
//             record.HammingSimilarity = 1.0f;
//             record.HammingDistance = 0;
//             continue;
//         };
    
//         uint32_t differences = 0;
        
//         // Count mismatches in overlapping region
//         for (uint32_t i = 0; i < minLen; i++) {
//             if (record.SeqASCIIOrg[i] != record.SeqASCIIMut[i]) {
//                 differences++;
//             }
//         }

//         record.HammingDissimilarity = static_cast<float>(differences) / static_cast<float>(maxLen);
//         record.HammingSimilarity = 1.0f - record.HammingDissimilarity;
//         record.HammingDistance = differences;
//     }
// }

// void SequenceDataMetrics::ComputeJaccardMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata, uint32_t tokenSize) {
    
//     for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++){
        
//         auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
        
//         if (record.SeqASCIIOrg.length() < tokenSize || record.SeqASCIIMut.length() < tokenSize) {
//             continue;  // Cannot compute k-mers
//         }

//         std::unordered_set<std::string> kmerSet1;
//         std::unordered_set<std::string> kmerSet2;

//         // Extract k-mers from seq1
//         for (size_t i = 0; i < (record.SeqASCIIOrg.length() - tokenSize + 1); i++) {
//             kmerSet1.insert(record.SeqASCIIOrg.substr(i, tokenSize));
//         }

//         // Extract k-mers from seq2
//         for (size_t i = 0; i < (record.SeqASCIIMut.length() - tokenSize + 1); i++) {
//             kmerSet2.insert(record.SeqASCIIMut.substr(i, tokenSize));
//         }

//         // Compute Jaccard similarity
//         std::vector<std::string> intersection;
//         std::vector<std::string> unionSet;

//         // Compute intersection size manually
//         uint32_t intersectionSize = 0;
//         for (const auto& kmer : kmerSet1) {
//             if (kmerSet2.find(kmer) != kmerSet2.end()) {
//                 intersectionSize++;
//             }
//         }
        
//         uint32_t unionSize = kmerSet1.size() + kmerSet2.size() - intersectionSize;
//         // printf("Jaccard: |A|=%lu, |B|=%lu, |A∩B|=%u, |A∪B|=%u\n", kmerSet1.size(), kmerSet2.size(), intersectionSize, unionSize);


//         if (unionSize == 0) {
//             record.JaccardSimilarity = 1.0f;  // Both sets empty, define similarity as 1
//             record.JaccardDissimilarity = 0.0f;
//             continue;
//         }

//         record.JaccardSimilarity = static_cast<float>(intersectionSize) / static_cast<float>(unionSize);
//         record.JaccardDissimilarity = 1.0f - record.JaccardSimilarity;
//     }
// }

// uint32_t SequenceDataMetrics::computeEditDistance(const std::string& seq1, const std::string& seq2) {
//     uint32_t m = seq1.length();
//     uint32_t n = seq2.length();
    
//     // Create DP table
//     std::vector<std::vector<uint32_t>> dp(m + 1, std::vector<uint32_t>(n + 1, 0));
    
//     // Initialize base cases
//     for (uint32_t i = 0; i <= m; i++) {
//         dp[i][0] = i;  // Cost of deleting all characters from seq1
//     }
//     for (uint32_t j = 0; j <= n; j++) {
//         dp[0][j] = j;  // Cost of inserting all characters from seq2
//     }
    
//     // Fill DP table
//     for (uint32_t i = 1; i <= m; i++) {
//         for (uint32_t j = 1; j <= n; j++) {
//             if (seq1[i-1] == seq2[j-1]) {
//                 // Characters match, no operation needed
//                 dp[i][j] = dp[i-1][j-1];
//             } else {
//                 // Take minimum of three operations
//                 uint32_t substitute = dp[i-1][j-1] + 1;  // Substitution
//                 uint32_t deleteOp = dp[i-1][j] + 1;      // Deletion
//                 uint32_t insert = dp[i][j-1] + 1;        // Insertion
                
//                 dp[i][j] = std::min({substitute, deleteOp, insert});
//             }
//         }
//     }
    
//     return dp[m][n];
// }

// void SequenceDataMetrics::ComputeEditDistanceMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata) {
//     for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++){
//         auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
//         record.EditDistance = computeEditDistance(record.SeqASCIIOrg, record.SeqASCIIMut);
//     }
// }

// int SequenceDataMetrics::computeGlobalAlignment(const std::string& seq1, const std::string& seq2, int match, int mismatch, int gap) {
//     uint32_t m = seq1.length();
//     uint32_t n = seq2.length();
    
//     // Create DP table
//     std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    
//     // Initialize with gap penalties
//     for (uint32_t i = 0; i <= m; i++) {
//         dp[i][0] = i * gap;
//     }
//     for (uint32_t j = 0; j <= n; j++) {
//         dp[0][j] = j * gap;
//     }
    
//     // Fill DP table
//     for (uint32_t i = 1; i <= m; i++) {
//         for (uint32_t j = 1; j <= n; j++) {
//             int matchScore = (seq1[i-1] == seq2[j-1]) ? match : mismatch;
            
//             int diagonal = dp[i-1][j-1] + matchScore;
//             int deleteOp = dp[i-1][j] + gap;
//             int insert = dp[i][j-1] + gap;
            
//             dp[i][j] = std::max({diagonal, deleteOp, insert});
//         }
//     }
    
//     return dp[m][n];
// }

// void SequenceDataMetrics::ComputeGlobalAlignmentMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata, int match, int mismatch, int gap){
//     for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++){
//         auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
//         record.GlobalAlignmentScore = computeGlobalAlignment(record.SeqASCIIOrg, record.SeqASCIIMut, match, mismatch, gap);
//     }
// }

// uint32_t SequenceDataMetrics::computeLocalAlignment(const std::string& seq1, const std::string& seq2, int match, int mismatch, int gap) {
//     uint32_t m = seq1.length();
//     uint32_t n = seq2.length();
    
//     // Create DP table
//     std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    
//     int maxScore = 0;
    
//     // Fill DP table
//     for (uint32_t i = 1; i <= m; i++) {
//         for (uint32_t j = 1; j <= n; j++) {
//             int matchScore = (seq1[i-1] == seq2[j-1]) ? match : mismatch;
            
//             int diagonal = dp[i-1][j-1] + matchScore;
//             int deleteOp = dp[i-1][j] + gap;
//             int insert = dp[i][j-1] + gap;
            
//             // Smith-Waterman: minimum score is 0 (no negative scores)
//             dp[i][j] = std::max({0, diagonal, deleteOp, insert});
            
//             // Track maximum score
//             maxScore = std::max(maxScore, dp[i][j]);
//         }
//     }
    
//     return maxScore;
// }

// void SequenceDataMetrics::ComputeLocalAlignmentMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata, int match, int mismatch, int gap){
//     for(uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++){
//         auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
//         record.LocalAlignmentScore = computeLocalAlignment(record.SeqASCIIOrg, record.SeqASCIIMut, match, mismatch, gap);
//     }
// }


// std::vector<BinStatisticsUnit> SequenceDataMetrics::BinRecords(SequenceRecordsWithMetadata* sequenceRecordsWithMetadata, uint32_t metric, double binStart, double binEnd, double binSize){

//     // Calculate number of bins
//     uint32_t numBins = static_cast<uint32_t>(std::ceil((binEnd - binStart) / binSize));
    
//     // Initialize bins
//     std::vector<BinStatisticsUnit> bins;
//     bins.reserve(numBins);

//     for (uint32_t i = 0; i < numBins; i++) {
//         double start = binStart + i * binSize;
//         double end = start + binSize;
//         bins.emplace_back(start, end);
//     }

//     // Bin each record
//     for (uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
//         const auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
//         float metricValue = -1.0f;


//         if(metric == 1){     // Normalized Hamming Distance
//             metricValue = record.HammingDissimilarity;
//         } 
//         else if(metric == 2){ // Jaccard Dissimilarity
//             metricValue = record.JaccardDissimilarity;
//         }
//         else {
//             // Unsupported metric
//             continue;
//         }
           
        
//         // Find appropriate bin
//         // Handle edge case: value exactly equals binEnd
//         if (metricValue >= binEnd) {
//             // Put in last bin
//             bins[numBins - 1].count++;
//             bins[numBins - 1].recordIndices.push_back(rec_idx);
//         } else if (metricValue < binStart) {
//             // Put in first bin
//             bins[0].count++;
//             bins[0].recordIndices.push_back(rec_idx);
//         } else {
//             // Calculate bin index
//             uint32_t binIndex = static_cast<uint32_t>((metricValue - binStart) / binSize);
            
//             // Ensure we don't exceed array bounds
//             if (binIndex >= numBins) {
//                 binIndex = numBins - 1;
//             }
            
//             bins[binIndex].count++;
//             bins[binIndex].recordIndices.push_back(rec_idx);
//         }
//     }
//     return bins;

// }

// void SequenceDataMetrics::printBinStatistics(const std::vector<BinStatisticsUnit>& bins, uint32_t metric) {

//     if(metric == 1){
//         std::cout << "\n=== Binning by Hamming Dissimilarity ===" << std::endl;
//     } 
//     else if(metric == 2){
//         std::cout << "\n=== Binning by Jaccard Dissimilarity ===" << std::endl;
//     } 
//     else {
//         std::cout << "\n=== Binning by Unknown Metric ===" << std::endl;
//     }
    
//     std::cout << std::string(70, '=') << std::endl;
    
//     uint32_t totalRecords = 0;
//     for (const auto& bin : bins) {
//         totalRecords += bin.count;
//     }
    
//     std::cout << std::fixed << std::setprecision(4);
//     std::cout << std::setw(15) << "Bin Range" 
//               << std::setw(15) << "Count" 
//               << std::setw(15) << "Percentage"
//               << std::setw(25) << "Histogram" << std::endl;
//     std::cout << std::string(70, '-') << std::endl;
    
//     // Find max count for histogram scaling
//     uint32_t maxCount = 0;
//     for (const auto& bin : bins) {
//         if (bin.count > maxCount) maxCount = bin.count;
//     }
    
//     for (const auto& bin : bins) {
//         double percentage = totalRecords > 0 ? (100.0 * bin.count / totalRecords) : 0.0;
        
//         // Create histogram bar
//         int barLength = maxCount > 0 ? (40 * bin.count / maxCount) : 0;
//         std::string bar(barLength, '*');
        
//         std::cout << "[" << std::setw(4) << bin.binStart << ", " 
//                   << std::setw(4) << bin.binEnd << ") "
//                   << std::setw(10) << bin.count 
//                   << std::setw(10) << percentage << "% "
//                   << bar << std::endl;
//     }
    
//     std::cout << std::string(70, '=') << std::endl;
//     std::cout << "Total records: " << totalRecords << std::endl;
// }

// //------------------------------------------------------------------------------------------




// I was here! 1 st



// // ============================================================================
// // EXPORT BINS TO FILE (CSV FORMAT)
// // ============================================================================

// void SequenceDataAnalyzer::exportBinsToFile(
//     const std::vector<BinStatistics>& bins,
//     const std::string& filename) {
    
//     std::ofstream outFile(filename);
//     if (!outFile.is_open()) {
//         std::cerr << "Error: Cannot open file " << filename << " for writing!" << std::endl;
//         return;
//     }
    
//     // Write header
//     outFile << "BinStart,BinEnd,Count,RecordIndices\n";
    
//     // Write bin data
//     for (const auto& bin : bins) {
//         outFile << std::fixed << std::setprecision(4);
//         outFile << bin.binStart << "," << bin.binEnd << "," << bin.count << ",\"";
        
//         // Write record indices (comma-separated)
//         for (size_t i = 0; i < bin.recordIndices.size(); i++) {
//             if (i > 0) outFile << ",";
//             outFile << bin.recordIndices[i];
//         }
//         outFile << "\"\n";
//     }
    
//     outFile.close();
//     std::cout << "Bin statistics exported to: " << filename << std::endl;
// }

// // ============================================================================
// // EXPORT ALL VALUES TO FILE (FOR HISTOGRAM PLOTTING)
// // ============================================================================

// void SequenceDataAnalyzer::exportValuesToFile(
//     SequenceRecordsWithMetadata* sequenceRecordsWithMetadata,
//     const std::string& filename,
//     const std::string& metricName) {
    
//     std::ofstream outFile(filename);
//     if (!outFile.is_open()) {
//         std::cerr << "Error: Cannot open file " << filename << " for writing!" << std::endl;
//         return;
//     }
    
//     outFile << std::fixed << std::setprecision(6);
    
//     for (uint32_t rec_idx = 0; rec_idx < sequenceRecordsWithMetadata->KeyCount; rec_idx++) {
//         const auto& record = sequenceRecordsWithMetadata->Records[rec_idx];
        
//         if (metricName == "NormalizedHammingDistance") {
//             outFile << record.HammingDissimilarity << "\n";
//         } else if (metricName == "JaccardSimilarity") {
//             outFile << record.JaccardSimilarity << "\n";
//         } else if (metricName == "JaccardDissimilarity") {
//             outFile << record.JaccardDissimilarity << "\n";
//         }
//         // Add more metrics as needed
//     }
    
//     outFile.close();
//     std::cout << "Values exported to: " << filename << std::endl;
// }