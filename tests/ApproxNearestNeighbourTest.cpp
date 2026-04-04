#include "Platform.h"
#include "Hashinfo.h"
#include "TestGlobals.h"
#include "Random.h"
#include "Analyze.h"
#include "Instantiate.h"
#include "VCode.h"

// #include "distances.h"
#include "BioDataGeneration.h"
#include "LSHGlobals.h"
#include "LSHCollisionTest.h"
#include <fstream>
#include <iostream>
#include <filesystem>

#include <unordered_map>

#if defined(HAVE_THREADS)
#include <atomic>
#endif

struct common_params_struct {
  uint32_t seqLen;
  seed_t DatagenSeed;
  seed_t DataMutateSeed;
  uint32_t tokenlength;
  bool isBasesDrawnFromUniformDist;
  uint32_t distanceClass;
};

// ============================================================================
// ANN LSH Index Data Structure
// ============================================================================
class ANN_LSH_Index {
private:
    uint32_t num_tables_r;
    uint32_t hashes_per_table_b;
    HashFn hash_func;
    
    // Seeds: Outer vector is tables (r), inner vector is hash functions (b)
    std::vector<std::vector<seed_t>> table_seeds;
    
    // Hash Tables: Outer vector corresponds to the r tables
    std::vector<std::unordered_map<uint64_t, std::vector<uint32_t>>> hash_tables;

    // This is a robust hash combiner to replace bitwise AND, which can
    // heavily bias the signature distribution toward 0.
    inline uint64_t combine_signatures(uint64_t seed, uint64_t hash_val) const {
        // Based on golden ratio mix (similar to boost::hash_combine but 64-bit) https://stackoverflow.com/a/2595226
        return seed ^ (hash_val + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
    }

    uint64_t compute_band_signature(const std::string& kmer, uint32_t table_idx) const {
        uint64_t band_signature = 0; // Initial seed for the combiner
        const uint8_t* strPtr = (const uint8_t*)kmer.c_str();
        size_t strLen = kmer.length();

        for (uint32_t k = 0; k < hashes_per_table_b; k++) {
            uint64_t current_hash = 0;//TODO: make the output datatype flexible based on the hash function's output type.
            hash_func(strPtr, strLen, table_seeds[table_idx][k], &current_hash);
            band_signature = combine_signatures(band_signature, current_hash);
        }
        return band_signature;
    }

public:
    ANN_LSH_Index(uint32_t r, uint32_t b, HashFn h_fn, SeedGenerator& seedGen) 
        : num_tables_r(r), hashes_per_table_b(b), hash_func(h_fn) {
        
        table_seeds.resize(r, std::vector<seed_t>(b));
        hash_tables.resize(r);

        // Initialize b independent hash seeds for each of the L tables
        for (uint32_t l = 0; l < r; l++) {
            for (uint32_t k = 0; k < b; k++) {
                table_seeds[l][k] = seedGen.nextSeed();
            }
        }
    }

    void insert(const std::string& kmer, uint32_t position) {
        for (uint32_t l = 0; l < num_tables_r; l++) {
            uint64_t band_signature = compute_band_signature(kmer, l);
            hash_tables[l][band_signature].push_back(position);
        }
    }

    std::vector<uint32_t> query(const std::string& query_seq) const {
        std::set<uint32_t> unique_candidates;

        for (uint32_t l = 0; l < num_tables_r; l++) {
            uint64_t band_signature = compute_band_signature(query_seq, l);
            auto it = hash_tables[l].find(band_signature);
            if (it != hash_tables[l].end()) {
                for (uint32_t pos : it->second) {
                    unique_candidates.insert(pos);
                }
            }
        }
        return std::vector<uint32_t>(unique_candidates.begin(), unique_candidates.end());
    }

    void print_stats() const {
        printf("\n--- LSH Index Statistics ---\n");
        printf("Tables (L): %u, Hashes per Table (K): %u\n", num_tables_r, hashes_per_table_b);
        for (uint32_t l = 0; l < num_tables_r; l++) {
            size_t total_items = 0;
            for(const auto& bucket : hash_tables[l]) {
                total_items += bucket.second.size();
            }
            printf("Table %u: %zu distinct buckets, %zu total items\n", l, hash_tables[l].size(), total_items);
        }
        printf("----------------------------\n\n");
    }
    void print_tables(size_t limit_per_table = 1000) const {
        printf("\n--- LSH Index Contents ---\n");
        for (uint32_t l = 0; l < num_tables_r; ++l) {
            printf("--- Table %u ---\n", l);
            if (hash_tables[l].empty()) {
                printf("  (empty)\n");
            } else {
                size_t count = 0;
                for (const auto& pair : hash_tables[l]) {
                    if (count >= limit_per_table) {
                        printf("... and %zu more buckets\n", hash_tables[l].size() - count);
                        break;
                    }
                    printf("  Hash: 0x%016llx -> Positions: [", (unsigned long long)pair.first);
                    for (size_t i = 0; i < pair.second.size(); ++i) {
                        printf("%u", pair.second[i]);
                        if (i < pair.second.size() - 1) {
                            printf(", ");
                        }
                    }
                    printf("]\n");
                    count++;
                }
            }
            printf("\n");
        }
        printf("------------------------\n\n");
    }
};


// void perform_sanity_checks_for_test_configuration(const HashInfo *hinfo,const uint32_t tokenlength,const uint32_t referenceLen,const uint32_t windowlength) {

//   // Sanity check of windowlength. It should always be less than the reference length, otherwise the test is not meaningful.
//   if (windowlength > referenceLen) {
//     printf("Error: Window length (%u) > reference length (%u).\n", windowlength,referenceLen);
//     exit(EXIT_FAILURE);
//   }

//   // Calculate the number of windows the sequence will be split into.

//   // Overlapping windows
//   // const uint64_t windowsInSequence = (referenceLen - windowlength + 1);
  
//   // Non-overlapping windows (more realistic for LSH test, as this is what the LSH test will use)
//   const uint64_t windowsInSequence = (referenceLen/windowlength); // Using non-overlapping windows for sanity check, as this is what the LSH test will use.
  
  
//   // Condition 2 : Skipping if the sequence is too short to provide enough windows
//   const uint64_t MIN_WINDOWS = 50; // A more reasonable threshold.
//   if (windowsInSequence < MIN_WINDOWS) {
//     printf("Skipping: Sequence with %llu windows is too short (min is %llu).\n", (unsigned long long)windowsInSequence,(unsigned long long)MIN_WINDOWS);
//     exit(EXIT_FAILURE);
//   }

//   // Condition 3 : Skipping if the window space is too small, which would lead to high collision rates even for random data, making the LSH property test less meaningful.
//   if (windowlength > 0) {
//     uint64_t maxPossibleWindows = (windowlength >= 32) ? UINT64_MAX : (1ULL << (2 * windowlength));
//     // This checks if the number of windows in the sequence is a large fraction
//     // of the total possible unique windows. For example, skip if
//     // windowsInSequence > 1% of maxPossibleWindows.
//     if (maxPossibleWindows / 100 < windowsInSequence) {
//       printf("Skipping: High window repetition expected. Windows in sequence (%llu) vs. max possible (%llu).\n", (unsigned long long)windowsInSequence,(unsigned long long)maxPossibleWindows);
//       exit(EXIT_FAILURE);
//     }
//   }
//   g_window_length = windowlength; // Set global window length for testing

//   // --- Sanity Checks for Test Configuration Only valid for tokenised hashes
//   // ---	//
//   if (hinfo->hasTokenisationProperty()) {
//     // Condition 1: Skipping if token length is greater than sequence length.
//     if (tokenlength > windowlength) {
//       printf("Skipping: Token length (%u) > sequence length (%u).\n", tokenlength, windowlength);
//       exit(EXIT_FAILURE);
//     }

//     // Calculate the number of tokens the sequence will be split into.
//     const uint64_t tokensInSequence = (windowlength - tokenlength + 1); // Cardinality
//     // Condition 2: Skipping if the sequence is too short to provide enough
//     // tokens for meaningful mutation analysis and avoid natural repetition.
//     const uint64_t MIN_TOKENS_FOR_MUTATION = 20; // A more reasonable threshold.
//     if (tokensInSequence < MIN_TOKENS_FOR_MUTATION) {
//       printf( "Skipping: Sequence with %llu tokens is too short (min is %llu).\n", (unsigned long long)tokensInSequence, (unsigned long long)MIN_TOKENS_FOR_MUTATION);
//       exit(EXIT_FAILURE);
//     }

//     // Condition 3: Skipping if the token space is too small, which would lead
//     // to high collision rates even for random data, making the LSH property
//     // test less meaningful.
//     if (tokenlength > 0) {
//       uint64_t maxPossibleTokens =
//           (tokenlength >= 32) ? UINT64_MAX : (1ULL << (2 * tokenlength));

//       // This checks if the number of tokens in the sequence is a large fraction
//       // of the total possible unique tokens. For example, skip if
//       // tokensInSequence > 10% of maxPossibleTokens.
//       if (maxPossibleTokens / 10 < tokensInSequence) {
//         printf("Skipping: High token repetition expected. Tokens in sequence (%llu) vs. max possible (%llu).\n", (unsigned long long)tokensInSequence, (unsigned long long)maxPossibleTokens);
//         exit(EXIT_FAILURE);
//       }
//     }

//     g_TokenLength = tokenlength; // Set global token length for testing
//   }
// }

static sim_bins_struct LSHCollisionTestInnerAgg(SequenceRecordsWithMetadataStruct &sequenceRecordsForAgg, SeedGenerator &seedGen, std::ofstream &out_file) {

  printf("Inside get_params_aggregated_in_bins\n");

  sim_bins_struct sim_bins_agg; // Bin to store the error parameters

  std::vector<double> rand_error_param(sequenceRecordsForAgg.KeyCount, 0.0);
  std::vector<double> similarity_values(sequenceRecordsForAgg.KeyCount, 0.0);

  printf("\n-----------------Start AGG----------------\n");
  if (g_mutation_model == MUTATION_MODEL_SIMPLE_SNP_ONLY) {
    SequenceDataMutatorSubstitutionOnly dataMutAgg(&sequenceRecordsForAgg, &rand_error_param);
  } else if (g_mutation_model == MUTATION_MODEL_GEOMETRIC_MUTATOR) {
    printf("Using geometric mutator model for data mutation in aggregation phase.\n");
    SequenceDataMutatorGeometric dataMutAgg(&sequenceRecordsForAgg, &rand_error_param);
  }
  printf("\n-----------------End AGG----------------");

  // Print Similarity values
  // out_file << ":13:";
  // for (size_t i = 0; i < sequenceRecordsForAgg.KeyCount; i++) {
  //   if (i == sequenceRecordsForAgg.KeyCount - 1)
  //     out_file << rand_error_param[i] << "\n";
  //   else
  //     out_file << rand_error_param[i] << ",";
  // }

  // Extract similarity values
  for (uint32_t idx = 0; idx < sequenceRecordsForAgg.KeyCount; idx++) {
    similarity_values[idx] = sequenceRecordsForAgg.Records[idx].similarity;
  }

  // Perform binning on similarity values
  for (size_t i = 0; i < sequenceRecordsForAgg.KeyCount; i++) {
    float sim_value = similarity_values[i];
    uint32_t bin_index =
        static_cast<uint32_t>((sim_value - sequenceRecordsForAgg.binstart) / sequenceRecordsForAgg.binsize);
    if (bin_index >= sequenceRecordsForAgg.bincount) {
      bin_index = sequenceRecordsForAgg.bincount - 1; // Clamp to last bin
    }

    uint32_t bin_fill_count = sim_bins_agg.bin_fill_count[bin_index];

    if (bin_fill_count == g_bincount_full) {
      continue; // To avoid excessive memory usage, we limit to x entries per bin.
    }

    sim_bins_agg.bin_error_parameters[bin_index][bin_fill_count] = rand_error_param[i];
    sim_bins_agg.bin_fill_count[bin_index]++;
  }

  // Compute mean and stddev for each bin
  for (size_t bin_idx = 0; bin_idx < sequenceRecordsForAgg.bincount; bin_idx++) {
    const auto &params = sim_bins_agg.bin_error_parameters[bin_idx];
    if (!params.empty()) {
      // Compute mean
      double sum = 0.0;
      for (uint32_t param_idx = 0; param_idx < sim_bins_agg.bin_fill_count[bin_idx]; param_idx++) {
        sum += params[param_idx];
      }
      printf("Bin %zu: Sum = %f, Count = %u\n", bin_idx, sum, sim_bins_agg.bin_fill_count[bin_idx]);
      double mean = sum / sim_bins_agg.bin_fill_count[bin_idx];
      sim_bins_agg.bin_error_parameters_mean[bin_idx] = mean;

      // Compute stddev
      double sq_sum = 0.0;
      for (uint32_t param_idx = 0; param_idx < sim_bins_agg.bin_fill_count[bin_idx]; param_idx++) {
        sq_sum += (params[param_idx] - mean) * (params[param_idx] - mean);
      }
      double stddev = std::sqrt(sq_sum / sim_bins_agg.bin_fill_count[bin_idx]);
      sim_bins_agg.bin_error_parameters_stddev[bin_idx] = stddev;

    } else {
      sim_bins_agg.bin_error_parameters_mean[bin_idx] = 0.0;
      sim_bins_agg.bin_error_parameters_stddev[bin_idx] = 0.0;
    }
  }

  // // Print each bin's fill count
  // for (size_t bin_idx = 0; bin_idx < sequenceRecordsForAgg.bincount;
  // bin_idx++) { 	printf("Bin %zu: Fill Count = %u", bin_idx,
  // sim_bins_agg.bin_fill_count[bin_idx]);
  // 	// Print mean and stddev
  // 	printf(", Mean = %0.2f, Stddev = %0.2f\n",
  // sim_bins_agg.bin_error_parameters_mean[bin_idx],
  // sim_bins_agg.bin_error_parameters_stddev[bin_idx]);
  // 	// // Print error parameters in this bin
  // 	// printf(" | Error Parameters: ");
  // 	// for (size_t j = 0; j < sim_bins_agg.bin_fill_count[bin_idx]; j++) {
  // 	// 	printf("%f ", sim_bins_agg.bin_error_parameters[bin_idx][j]);
  // 	// }
  // 	// printf("\n");
  // }

  //--------------------------------------------	//
  // Fill the partially filled bins using the 	//
  // mean and stddev values computed above. 		//
  //--------------------------------------------	//
  // Now, I want to go through each bin, and if its fill count is less than
  // g_bincount_full, I want to fill the remaining entries with random samples
  // from a normal distribution defined by the mean and stddev of that bin.

  std::mt19937 gen(seedGen.nextSeed()); // Seeded RNG for reproducibility

  for (size_t bin_idx = 0; bin_idx < sequenceRecordsForAgg.bincount; bin_idx++) {
    uint32_t current_fill = sim_bins_agg.bin_fill_count[bin_idx];

    if (current_fill < g_bincount_full && current_fill > 0) {
      // Get the mean and stddev for this bin
      double mean = sim_bins_agg.bin_error_parameters_mean[bin_idx];
      double stddev = sim_bins_agg.bin_error_parameters_stddev[bin_idx];

      // If stddev is 0 or very small, use a small default to avoid degenerate
      // distribution
      if (stddev < 1e-9) {
        stddev = 0.01;
      }

      std::normal_distribution<double> normal_dist(mean, stddev);

      // Fill remaining entries
      for (uint32_t fill_idx = current_fill; fill_idx < g_bincount_full; fill_idx++) {
        double sampled_value = normal_dist(gen);

        // Clamp to valid range [0.0, 1.0] since these are error rates
        sampled_value = std::max(0.0, std::min(1.0, sampled_value));

        sim_bins_agg.bin_error_parameters[bin_idx][fill_idx] = sampled_value;
        sim_bins_agg.bin_fill_count[bin_idx]++;
      }
    } else if (current_fill == 0) {

      // Todo: Add the part where in angular similarity, we geniuinely have
      // empty bins from 0 to 49

      // // Bin is completely empty - use bin center as mean with small stddev
      // double bin_center = sequenceRecordsForAgg.binstart + (bin_idx + 0.5) *
      // sequenceRecordsForAgg.binsize; double default_stddev =
      // sequenceRecordsForAgg.binsize / 4.0;  // Quarter of bin width

      // std::normal_distribution<double> normal_dist(bin_center,
      // default_stddev);

      // for (uint32_t fill_idx = 0; fill_idx < g_bincount_full; fill_idx++) {
      // 	double sampled_value = normal_dist(gen);
      // 	sampled_value = std::max(0.0, std::min(1.0, sampled_value));

      // 	sim_bins_agg.bin_error_parameters[bin_idx][fill_idx] =
      // sampled_value; 	sim_bins_agg.bin_fill_count[bin_idx]++;
      // }

      // // Update mean and stddev for this bin
      // sim_bins_agg.bin_error_parameters_mean[bin_idx] = bin_center;
      // sim_bins_agg.bin_error_parameters_stddev[bin_idx] = default_stddev;

      sim_bins_agg.bin_fill_count[bin_idx] = 0;
      sim_bins_agg.bin_error_parameters_mean[bin_idx] = 0.0;
      sim_bins_agg.bin_error_parameters_stddev[bin_idx] = 0.0;
    }
  }
  // Clean up
  // sequenceRecordsForAgg.Records.clear();
  // sequenceRecordsForAgg.Records.shrink_to_fit();  // Actually releases the
  // memory

  return sim_bins_agg;
}



// Function to write sequences to a file
void writeSequencesToFile(const SequenceRecordsWithMetadataStruct& sequenceRecords, const std::string& outputFilename, int flag) {
    // Open the file in write mode
    std::ofstream outFile(outputFilename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file " << outputFilename << " for writing.\n";
        return;
    }

    if(flag == 0){
      // Iterate over the records and write each original sequence to the file
      for (const auto& record : sequenceRecords.Records) {
          outFile << record.SeqASCIIOrg << "\n";
      }
    }
    else if(flag == 1){
      // Iterate over the records and write each mutated sequence to the file
      for (const auto& record : sequenceRecords.Records) {
          outFile << record.SeqASCIIMut << "\n";
      }
    }
    else if(flag == 2){
      // Iterate over the records and write each original and mutated sequence to the file
      for (const auto& record : sequenceRecords.Records) {
          outFile << "Original: " << record.SeqASCIIOrg << "\n";
          outFile << "Mutated: " << record.SeqASCIIMut << "\n";
          outFile << "Similarity: " << record.similarity << "\n";
          outFile << "-------------------------\n";
      }
    }
    

    // Close the file
    outFile.close();
    std::cout << "Original sequences written to " << outputFilename << "\n";
}

//----------------------------------------------------------------------------//
template <typename hashtype>
bool LSHApproxNearestNeighbourTest(const HashInfo *hinfo, bool extra, flags_t flags) {
  
  printf("[[[ Approximate Nearest Neighbour Test ]]]\n\n");
  if (extra) {
    printf("Extra flag is set. Running extended tests where applicable.\n");
  }

  bool result = true;

  // Create a output file for storing the results.
  std::string filename = "../results/ApproxNearestNeighbourResults_" + std::string(hinfo->name) + ".csv";

  std::ios_base::openmode mode = std::ios::trunc; // Default: replace
  if (std::filesystem::exists(filename)) {
    mode = std::ios::app; // File exists: append
  }

  std::ofstream out_file(filename, mode);
  if (!out_file.is_open()) {
    std::cerr << "Error: Could not open output file" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Check the hash function and see if is hamming. If hamming then set the
  // mutation model to simple SNP only.
  if (hinfo->hash_flags & FLAG_HASH_HAMMING_SIMILARITY) {
    printf("Hash %s uses Hamming similarity. Setting mutation model to simple SNP only for testing.\n", hinfo->name);
    g_mutation_model = MUTATION_MODEL_SIMPLE_SNP_ONLY;
  }

  /*
      If the hash has tokenisation property, then we need to test for
      multiple token lengths Otherwise, we just use a token length of 0 (no
      tokenisation)
  */
  uint32_t tokenlength;
  if (hinfo->hasTokenisationProperty()) {
    printf("Hash %s has tokenisation property. Testing multiple token lengths.\n",hinfo->name);
    tokenlength = g_tokenLengths_array[0]; //TODO: make it similar to collision curve test.
  } else {
    tokenlength = 0; // No tokenization
  }

  printf("Using token length: %u\n", tokenlength);

  // Note: The tokenlength is different from window size. Token is like a kmer.
  // Window size is the length of the sequence that we are hashing.

  uint32_t sequenceLength = g_sequenceLength_ANN;

  // TODO: Clear it.
  // perform_sanity_checks_for_test_configuration(hinfo, tokenlength, referenceLen, sequenceLength);

  // -----------------------------------------------------------------------------------------------------
  // 1. Generate Ref sequences.

  seed_t baseSeed = g_GoldenRatio ^ std::chrono::system_clock::now().time_since_epoch().count();
  SeedGenerator seedGen(baseSeed);

  SequenceRecordsWithMetadataStruct ReferenceSequenceRecord;
  ReferenceSequenceRecord.KeyCount = g_Nseq_in_Database; // Will contain one sequence
  ReferenceSequenceRecord.OriginalSequenceLength = sequenceLength;
  ReferenceSequenceRecord.DistanceClass = 0; // not applicable
  ReferenceSequenceRecord.isBasesDrawnFromUniformDist = true;
  ReferenceSequenceRecord.DatagenSeed = seedGen.nextSeed();
  ReferenceSequenceRecord.DataMutateSeed = 0; // not applicable

  SequenceDataGenerator dataGenReference(&ReferenceSequenceRecord);

  // print the reference sequence to the output file  //TODO: Add a random number as suffix. and proper output file positing.
  writeSequencesToFile(ReferenceSequenceRecord, "../results/reference_sequences.txt", 0);

  // -----------------------------------------------------------------------------------------------------
  // 2. Generate query sequences by randomly sampling from the reference sequences.

  const uint32_t numQueries = g_numQueriesForApproxNNTest;
  assert(numQueries < g_Nseq_in_Database && "Number of queries exceeds number of available sequences"); // Ensure we don't sample more queries than available sequences

  Rand rngQuery(seedGen.nextSeed()); // seeded RNG for reproducible sampling

  SequenceRecordsWithMetadataStruct QuerySequenceRecord;
  QuerySequenceRecord.KeyCount = numQueries;
  QuerySequenceRecord.OriginalSequenceLength = sequenceLength; // Sampled Queries which have not yet mutated are same length as reference.
  QuerySequenceRecord.isBasesDrawnFromUniformDist = true;
  QuerySequenceRecord.DatagenSeed = 0; // Not applicable, as we are sampling from reference sequences rather than generating new random sequences.
  QuerySequenceRecord.DataMutateSeed = seedGen.nextSeed();
  QuerySequenceRecord.DistanceClass =  setDistanceClassForHashInfo(hinfo->hash_flags);

  // Manual population of query sequences by sampling from unique k-mers
  QuerySequenceRecord.Records.resize(numQueries);
  for (uint32_t i = 0; i < numQueries; i++) {
    uint32_t idx = rngQuery.rand_range(static_cast<uint32_t>(ReferenceSequenceRecord.Records.size()));
    QuerySequenceRecord.Records[i].SeqASCIIOrg = ReferenceSequenceRecord.Records[idx].SeqASCIIOrg;
    QuerySequenceRecord.Records[i].OriginalLength = sequenceLength;
  }

  // Print the query sequences to the output file
  writeSequencesToFile(QuerySequenceRecord, "../results/query_sequences.txt", 0);

  // 3. Perform aggegration to get the bins from 95% to 100% similarity.
  SequenceRecordsWithMetadataStruct AggSequenceRecord;
  AggSequenceRecord.KeyCount = g_norm_N_agg_cases;
  AggSequenceRecord.OriginalSequenceLength = sequenceLength; // Sampled Queries which have not yet mutated are same length as reference.
  AggSequenceRecord.isBasesDrawnFromUniformDist = true;
  AggSequenceRecord.DatagenSeed = 0; // Not applicable, as we are sampling from reference sequences rather than generating new random sequences.
  AggSequenceRecord.DataMutateSeed = seedGen.nextSeed();
  AggSequenceRecord.DistanceClass =  setDistanceClassForHashInfo(hinfo->hash_flags);
  
  SequenceDataGenerator dataGenAgg(&AggSequenceRecord);
  sim_bins_struct sim_bins = LSHCollisionTestInnerAgg(AggSequenceRecord, seedGen, out_file);
  // remove the agg sequence records to free up memory, as we no longer need them. We have already extracted the binned error parameters and their statistics that we need for the next steps.
  AggSequenceRecord.Records.clear();
  AggSequenceRecord.Records.shrink_to_fit();
  
  
  // print bin means and stddevs using
  if(REPORT(VERBOSE, flags)) {
    for (size_t bin_idx = 0; bin_idx < sim_bins.bin_error_parameters_mean.size(); bin_idx++) {
      printf("Bin %zu: Count %d, Mean = %0.2f, Stddev = %0.2f\n", bin_idx, sim_bins.bin_fill_count[bin_idx], sim_bins.bin_error_parameters_mean[bin_idx], sim_bins.bin_error_parameters_stddev[bin_idx]);
    }
  }

  // Reset mutation-related fields in each record
  for (uint32_t i = 0; i < QuerySequenceRecord.KeyCount; i++) {
    QuerySequenceRecord.Records[i].SeqASCIIMut.clear();
    QuerySequenceRecord.Records[i].similarity = 0.0;
    QuerySequenceRecord.Records[i].foundationalParameter = 0.0;
    QuerySequenceRecord.Records[i].snpRate = 0.0;
    QuerySequenceRecord.Records[i].delRate = 0.0;
    QuerySequenceRecord.Records[i].stayRate = 0.0;
    QuerySequenceRecord.Records[i].insmean = 0.0;
    QuerySequenceRecord.Records[i].insRate = 0.0;
  }

  // Generate new error parameters targeting similarity range [0.95, 1.0]

  const double target_sim_low = 0.90;  // 95% similarity
  const double target_sim_high = 1.0; // 100% similarity (no mutation)

  Randbin rng_bin_sampler(seedGen.nextSeed());
  Randbin rng_bin_params_sampler(seedGen.nextSeed());

  uint32_t skipped_sequences = 0;
  for (uint32_t idx = 0; idx < QuerySequenceRecord.KeyCount; idx++) {
    uint32_t bin_fill_count = 0;
    int sampled_binid = -1;
    uint32_t attempts = 0;
    const uint32_t max_attempts = 1000;

    while (bin_fill_count == 0 && attempts < max_attempts) {
      sampled_binid = rng_bin_sampler.rand_custom_range(target_sim_low * 100, target_sim_high * 100);
      // printf("Attempt %u: Sampled bin ID = %d\n", attempts + 1,sampled_binid);
      bin_fill_count = sim_bins.bin_fill_count[sampled_binid];  // get the number of entries in this bin. We get this from the agg computation step.
      attempts++;
    }

    if (bin_fill_count == 0) {
      printf("Warning: Could not find non-empty bin after %u attempts\n", max_attempts);
      // sequenceRecordsforTest.Records[idx].snpRate = 1.0; // Assign a default value
      skipped_sequences++;
      continue; // Skip this sequence or handle error
    }

    uint32_t rand_param_idx = rng_bin_params_sampler.rand_range(bin_fill_count);
    double sampled_error_param = sim_bins.bin_error_parameters[sampled_binid][rand_param_idx];
    QuerySequenceRecord.Records[idx].foundationalParameter = sampled_error_param;
  }

  if (skipped_sequences > 0) {
    printf("Skipped %u sequences due to empty bins.\n", skipped_sequences);
  }

  if (g_mutation_model == MUTATION_MODEL_SIMPLE_SNP_ONLY) {
    SequenceDataMutatorSubstitutionOnly dataMutTest(&QuerySequenceRecord);
    printf("Completed mutation using simple SNP only model.\n");
  } else if (g_mutation_model == MUTATION_MODEL_GEOMETRIC_MUTATOR) {
    SequenceDataMutatorGeometric dataMutTest(&QuerySequenceRecord);
    printf("Completed mutation using geometric mutator model.\n");
  }

  printf("Re-sampled mutated %u sequences targeting similarity in [%.2f, %.2f].\n", QuerySequenceRecord.KeyCount, target_sim_low, target_sim_high);

  writeSequencesToFile(QuerySequenceRecord, "../results/query_sequences.txt", 2);

  // print the query, the mutated sequence and the similarity values to terminal
  // printf("\n\n");
  // for (uint32_t i = 0; i < QuerySequenceRecord.KeyCount; i++) {
  //   printf("Foundational Parameter for record %u: %f\n", i, QuerySequenceRecord.Records[i].foundationalParameter);
  //   printf("Similarity for record %u: %f\n", i, QuerySequenceRecord.Records[i].similarity);
  //   printf("Query Sequence: %s\n", QuerySequenceRecord.Records[i].SeqASCIIOrg.c_str());
  //   printf("Mutated Sequence: %s\n", QuerySequenceRecord.Records[i].SeqASCIIMut.c_str());
  // }

  // Debug section: Print similarity statistics across all query records
  double sum_sim = 0.0;
  double min_sim = 1.0;
  double max_sim = 0.0;
  for (uint32_t i = 0; i < QuerySequenceRecord.KeyCount; i++) {
    double s = QuerySequenceRecord.Records[i].similarity;
    sum_sim += s;
    if (s < min_sim)
      min_sim = s;
    if (s > max_sim)
      max_sim = s;
  }
  double mean_sim = sum_sim / QuerySequenceRecord.KeyCount;

  double sq_diff_sum = 0.0;
  for (uint32_t i = 0; i < QuerySequenceRecord.KeyCount; i++) {
    double diff = QuerySequenceRecord.Records[i].similarity - mean_sim;
    sq_diff_sum += diff * diff;
  }
  double stddev_sim = sqrt(sq_diff_sum / QuerySequenceRecord.KeyCount);

  printf("Similarity stats (%u records): Mean = %f, Min = %f, Max = %f, Stddev = %f\n", QuerySequenceRecord.KeyCount, mean_sim, min_sim, max_sim, stddev_sim);

  //-----------------------------------------------------------------------------------------------------

  // ReferenceSequenceRecord.Records[999999].SeqASCIIOrg = ReferenceSequenceRecord.Records[999998].SeqASCIIOrg;   //just for testing

  // It is importnat that we perform this uniqification, since in certain cases there is a high chance of generating duplicate sequences in the reference.
  
  // Create a mapping of unique k-mers in the reference to their positions for efficient similarity lookups.
  std::unordered_map<std::string, std::vector<uint32_t>> uniqueKmerPositions;
  for (uint32_t pos = 0; pos < g_Nseq_in_Database; pos++) {
    std::string sequence = ReferenceSequenceRecord.Records[pos].SeqASCIIOrg;
    uniqueKmerPositions[sequence].push_back(pos);
  }

  // // --- Code to print uniqueKmerPositions (for debugging/verification) ---
  // printf("\n--- Contents of uniqueKmerPositions (%zu unique kmers) ---\n", uniqueKmerPositions.size());
  // uint32_t print_count = 0;
  // for (const auto& pair : uniqueKmerPositions) {
  //     printf("Kmer: %s -> Positions: [", pair.first.c_str());
  //     for (size_t i = 0; i < pair.second.size(); ++i) {
  //         printf("%u", pair.second[i]);
  //         if (i < pair.second.size() - 1) {
  //             printf(", ");
  //         }
  //     }
  //     printf("]\n");
  //     if (++print_count >= 10 && uniqueKmerPositions.size() > 10) { // Limit output to first 10 for large maps
  //         printf("... (showing first 10 entries of %zu)\n", uniqueKmerPositions.size());
  //         break;
  //     }
  // }
  // printf("----------------------------------------------------------\n\n");


  // -----------------------------------------------------------------------------------------------------
  // Compute Ground Truth: For each mutated query, find the TOP_K closest sequences
  // in the entire reference database via brute-force comparison.
  // -----------------------------------------------------------------------------------------------------
  const uint32_t TOP_K = 1; // Find top 10 nearest neighbors
  uint32_t distanceClass = QuerySequenceRecord.DistanceClass;

  struct GroundTruthEntry {
    uint32_t position; // Position in the reference database
    double similarity;
  };
  
  std::vector<std::vector<GroundTruthEntry>> groundTruthNearest(QuerySequenceRecord.KeyCount);

  printf("Computing ground truth top-%u nearest sequences for %u queries across %u reference sequences...\n", TOP_K, QuerySequenceRecord.KeyCount, g_Nseq_in_Database);

  for (uint32_t q_idx = 0; q_idx < QuerySequenceRecord.KeyCount; q_idx++) {
    const std::string &querySeq = QuerySequenceRecord.Records[q_idx].SeqASCIIMut;
    // Step 1: Compute similarity between this query and EVERY sequence in the reference
    std::vector<std::pair<double, uint32_t>> allSimilarities;
    allSimilarities.reserve(g_Nseq_in_Database);

    for (uint32_t ref_pos = 0; ref_pos < g_Nseq_in_Database; ++ref_pos) {
      const std::string &refSeq = ReferenceSequenceRecord.Records[ref_pos].SeqASCIIOrg;
      double sim = 0.0;
      if (distanceClass == 1) { // Hamming
        sim = ComputeHammingSimilarity(querySeq, refSeq, sequenceLength);
      } else if (distanceClass == 2) { // Jaccard
        sim = ComputeJaccardSimilarity(querySeq, refSeq, g_TokenLength);
      } else if (distanceClass == 3) { // Cosine
        sim = ComputeCosineSimilarity(querySeq, refSeq, g_TokenLength);
      } else if (distanceClass == 4) { // Angular
        sim = ComputeAngularSimilarity(querySeq, refSeq, g_TokenLength);
      } else if (distanceClass == 5) { // Edit Distance
        sim = ComputeEditSimilarity(querySeq, refSeq);
      }
      allSimilarities.push_back({sim, ref_pos});
    }

    // Step 2: Sort by similarity in descending order (most similar first)
    std::sort(allSimilarities.begin(), allSimilarities.end(), 
              [](const std::pair<double, uint32_t> &a, const std::pair<double, uint32_t> &b) {
      return a.first > b.first;
    });
    
    // Step 3: Take the top-K entries
    std::vector<GroundTruthEntry>& topK = groundTruthNearest[q_idx];
    topK.reserve(TOP_K);
    for (uint32_t i = 0; i < TOP_K && i < allSimilarities.size(); ++i) {
        topK.push_back({allSimilarities[i].second, allSimilarities[i].first});
    }

    if(q_idx % 100 == 0 && q_idx > 0){
      printf("  Ground truth progress: %u / %u queries done.\n", q_idx, QuerySequenceRecord.KeyCount);
    }
  }

  printf("Ground truth computation complete for %u queries.\n", QuerySequenceRecord.KeyCount);
  
  // Write ground truth to output file
  // out_file << ":15:Ground Truth Top-" << TOP_K << " Nearest Sequences\n";
  // for(uint32_t q_idx = 0; q_idx < QuerySequenceRecord.KeyCount; q_idx++){
  //   out_file << "Q" << q_idx << ":";
  //   for(size_t k = 0; k < groundTruthNearest[q_idx].size(); k++){
  //     const auto& entry = groundTruthNearest[q_idx][k];
  //     out_file << entry.position << "," << entry.similarity;
  //     if(k < groundTruthNearest[q_idx].size() - 1) out_file << ";";
  //   }
  //   out_file << "\n";
  // }
  
  // ============================================================================
  // LSH Index Construction and Querying
  // ============================================================================

  HashFn hash = hinfo->hashFn(g_hashEndian);
  if (!hash) {
      printf("Error: hash function pointer is null!\n");
      exit(EXIT_FAILURE);
  }

  // Define LSH parameters and number of runs
  const uint32_t NUM_RUNS = g_ANN_runs_for_avg;
  std::vector<std::pair<uint32_t, uint32_t>> br_pairs;

  // Programmatically fill br_pairs from b=1 to 10 and r=1 to 10
  assert(g_ANN_start_B > 0 && g_ANN_start_R > 0 && g_ANN_MAX_B >= g_ANN_start_B && g_ANN_MAX_R >= g_ANN_start_R);

  for (uint32_t b_val = g_ANN_start_B; b_val <= g_ANN_MAX_B; ++b_val) {
      for (uint32_t r_val = g_ANN_start_R; r_val <= g_ANN_MAX_R; ++r_val) {
          br_pairs.push_back({b_val, r_val});
      }
  }
  printf("Generated %zu (b,r) pairs for testing (b from %u to %u, r from %u to %u).\n", br_pairs.size(), g_ANN_start_B, g_ANN_MAX_B, g_ANN_start_R, g_ANN_MAX_R);


  // File header
  out_file << ":1:LSH Approx Nearest Neighbour Summary\n";
  out_file << ":2:" << "Hashname," << "SequenceLength," << "TokenLength,"
           << "Distance Metric," << "Mutation Model," << "Mutation Expression"
           << std::endl;
  out_file << ":3:" << hinfo->name << "," << g_sequenceLength_ANN << "," << g_TokenLength << ","
           << setDistanceClassForHashInfo(hinfo->hash_flags) << ","
           << g_mutation_model << "," << g_mutation_expression_type
           << std::endl;
  // Print hash function ultra-specific parameters to the output file
  hinfo->printParameters(out_file);

  out_file << ":5:b,r,Avg_Recall,Avg_Precision,Avg_FPR,Avg_F1_Score\n";
  

  
  for (const auto& br_pair : br_pairs) {
    uint32_t b = br_pair.first;  // Hashes per table
    uint32_t r = br_pair.second; // Number of tables

    printf("\n\n==================================================================\n");
    printf("Running experiment for b=%u, r=%u over %u runs\n", b, r, NUM_RUNS);
    printf("==================================================================\n");
    
    std::vector<double> all_runs_avg_recall;
    std::vector<double> all_runs_avg_precision;
    std::vector<double> all_runs_avg_fpr;
    std::vector<double> all_runs_avg_f1;

    for (uint32_t run = 0; run < NUM_RUNS; ++run) {
      printf("\n--- Run %u/%u for b=%u, r=%u ---\n", run + 1, NUM_RUNS, b, r);

      ANN_LSH_Index lsh_index(r, b, hash, seedGen);

      // Populate Index from the reference database
      for (uint32_t i = 0; i < g_Nseq_in_Database; ++i) {
          lsh_index.insert(ReferenceSequenceRecord.Records[i].SeqASCIIOrg, i);
      }
      
      // ============================================================================
      // Query and Evaluation Phase for this run
      // ============================================================================
      std::vector<double> recall_per_query;
      std::vector<double> precision_per_query;
      std::vector<double> fpr_per_query;

      for (uint32_t q_idx = 0; q_idx < QuerySequenceRecord.KeyCount; q_idx++) {
        const std::string& querySeq = QuerySequenceRecord.Records[q_idx].SeqASCIIMut;
        std::vector<uint32_t> lsh_results = lsh_index.query(querySeq);
        std::set<uint32_t> lsh_results_set(lsh_results.begin(), lsh_results.end());
        
        const auto& ground_truth = groundTruthNearest[q_idx];
        std::set<uint32_t> ground_truth_positions;
        for (const auto& entry : ground_truth) {
            ground_truth_positions.insert(entry.position);
        }
        
        uint32_t true_positives = 0;
        uint32_t false_positives = 0;
        
        for (uint32_t pos : lsh_results_set) {
            if (ground_truth_positions.count(pos)) {
                true_positives++;
            } else {
                false_positives++;
            }
        }
        
        uint32_t false_negatives = ground_truth_positions.size() - true_positives;
        assert(false_negatives >= 0);
        
        double recall = (true_positives + false_negatives > 0) ? (double)true_positives / (true_positives + false_negatives) : 0.0;
        double precision = (true_positives + false_positives > 0) ? (double)true_positives / (true_positives + false_positives) : 0.0;
        
        uint32_t total_negatives = g_Nseq_in_Database - ground_truth_positions.size();
        double false_positive_rate = (total_negatives > 0) ? (double)false_positives / total_negatives : 0.0;

        recall_per_query.push_back(recall);
        precision_per_query.push_back(precision);
        fpr_per_query.push_back(false_positive_rate);
      }

      // Calculate macro-averaged metrics for this single run
      double run_avg_recall = 0.0;
      double run_avg_precision = 0.0;
      double run_avg_fpr = 0.0;

      if (QuerySequenceRecord.KeyCount > 0) {
          for (double r_val : recall_per_query) run_avg_recall += r_val;
          run_avg_recall /= QuerySequenceRecord.KeyCount;
          
          for (double p_val : precision_per_query) run_avg_precision += p_val;
          run_avg_precision /= QuerySequenceRecord.KeyCount;

          for (double fpr_val : fpr_per_query) run_avg_fpr += fpr_val;
          run_avg_fpr /= QuerySequenceRecord.KeyCount;
      }

      double run_f1_score = 0.0;
      if ((run_avg_recall + run_avg_precision) > 0) {
          run_f1_score = 2.0 * (run_avg_recall * run_avg_precision) / (run_avg_recall + run_avg_precision);
      }

      printf("  Run %u Metrics: Recall=%.4f, Precision=%.4f, FPR=%.4f, F1=%.4f\n", run + 1, run_avg_recall, run_avg_precision, run_avg_fpr, run_f1_score);

      // Store metrics for this run
      all_runs_avg_recall.push_back(run_avg_recall);
      all_runs_avg_precision.push_back(run_avg_precision);
      all_runs_avg_fpr.push_back(run_avg_fpr);
      all_runs_avg_f1.push_back(run_f1_score);
    }

    // Average the metrics across all runs for the current (b, r) pair
    double final_avg_recall = 0.0;
    double final_avg_precision = 0.0;
    double final_avg_fpr = 0.0;
    double final_avg_f1 = 0.0;

    for(double val : all_runs_avg_recall) final_avg_recall += val;
    final_avg_recall /= NUM_RUNS;

    for(double val : all_runs_avg_precision) final_avg_precision += val;
    final_avg_precision /= NUM_RUNS;

    for(double val : all_runs_avg_fpr) final_avg_fpr += val;
    final_avg_fpr /= NUM_RUNS;

    for(double val : all_runs_avg_f1) final_avg_f1 += val;
    final_avg_f1 /= NUM_RUNS;

    printf("\n--- Average Metrics for b=%u, r=%u (over %u runs) ---\n", b, r, NUM_RUNS);
    printf("Average Recall:    %.4f\n", final_avg_recall);
    printf("Average Precision: %.4f\n", final_avg_precision);
    printf("Average FPR:       %.4f\n", final_avg_fpr);
    printf("Average F1-Score:  %.4f\n", final_avg_f1);
    printf("-----------------------------------------------------\n");

    // Write summary for this (b,r) pair to file
    out_file <<":6:" << b << "," << r << "," << final_avg_recall << "," << final_avg_precision << "," << final_avg_fpr << "," << final_avg_f1 << "\n";
  }

  // Cleanup: Reset LSH global variables after test completion
  SetIsTestActive(false);

  // printf("%s\n", result ? "" : g_failstr);
  out_file.close();
  return result;
}

INSTANTIATE(LSHApproxNearestNeighbourTest, HASHTYPELIST);