#include "specifics.h"
#include "HashInfo.h"
#include "TestGlobals.h"
#include "Instantiate.h"

#include "BioDataGeneration.h"
#include "LSHGlobals.h"
#include "ApproxNearestNeighbour.h"

#include <random>
#include <fstream>
#include <iostream>
#include <filesystem>

#include <algorithm>
#include <map>
#include <unordered_map>
#include <set>

#if defined(HAVE_THREADS)
#include <atomic>
#endif

struct common_params_struct {
  uint32_t seqLen;
  seed_t DatagenSeed;
  seed_t DataMutateSeed;
  bool isBasesDrawnFromUniformDist;
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
    static inline uint64_t combine_signatures(uint64_t seed, uint64_t hash_val) {
        // Based on golden ratio mix (similar to boost::hash_combine but 64-bit) https://stackoverflow.com/a/2595226
        return seed ^ (hash_val + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
    }

    [[nodiscard]] uint64_t compute_band_signature(const std::string& kmer, uint32_t table_idx) const {
        uint64_t band_signature = 0; // Initial seed for the combiner
        const auto* strPtr = reinterpret_cast<const uint8_t *>(kmer.c_str());
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

    [[nodiscard]] std::vector<uint32_t> query(const std::string& query_seq) const {
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
        return {unique_candidates.begin(), unique_candidates.end()};
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
                    printf("  Hash: 0x%016llx -> Positions: [", static_cast<unsigned long long>(pair.first));
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


static sim_bins_struct LSHInnerAgg(const HashInfo * hinfo,SequenceRecordsWithMetadataStruct &sequenceRecordsForAgg, SeedGenerator &seedGen, std::ofstream &out_file) {
    printf("Inside get_params_aggregated_in_bins\n");

    sim_bins_struct sim_bins_agg; // Bin to store the error parameters

    std::vector<double> rand_error_param(sequenceRecordsForAgg.KeyCount, 0.0);
    std::vector<double> similarity_values(sequenceRecordsForAgg.KeyCount, 0.0);

    printf("\n-----------------Start AGG----------------\n");
        if (g_mutation_model == MUTATION_MODEL_SIMPLE_SNP_ONLY) {
        [[maybe_unused]] SequenceDataMutatorSubstitutionOnly dataMutAgg(&sequenceRecordsForAgg, &rand_error_param, hinfo->similarityfn);
    } else if (g_mutation_model == MUTATION_MODEL_GEOMETRIC_MUTATOR) {
        printf("Using geometric mutator model for data mutation in aggregation phase.\n");
        [[maybe_unused]] SequenceDataMutatorGeometric dataMutAgg(&sequenceRecordsForAgg, &rand_error_param, hinfo->similarityfn);
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
        double sim_value = similarity_values[i];
        auto bin_index = static_cast<uint32_t>((sim_value - sequenceRecordsForAgg.binstart) / sequenceRecordsForAgg.binsize);
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
        }
        else {
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
        }
        else if (current_fill == 0) {
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

void print_uniqueKmerPositions(const std::unordered_map<std::string, std::vector<uint32_t>>&  uniqueKmerPositions) {
    // --- Code to print uniqueKmerPositions (for debugging/verification) ---
    printf("\n--- Contents of uniqueKmerPositions (%zu unique kmers) ---\n", uniqueKmerPositions.size());
    uint32_t print_count = 0;
    for (const auto& pair : uniqueKmerPositions) {
        printf("Kmer: %s -> Positions: [", pair.first.c_str());
        for (size_t i = 0; i < pair.second.size(); ++i) {
            printf("%u", pair.second[i]);
            if (i < pair.second.size() - 1) {
                printf(", ");
            }
        }
        printf("]\n");
        if (++print_count >= 10 && uniqueKmerPositions.size() > 10) { // Limit output to first 10 for large maps
            printf("... (showing first 10 entries of %zu)\n", uniqueKmerPositions.size());
            break;
        }
    }
    printf("----------------------------------------------------------\n\n");
}

//----------------------------------------------------------------------------//
template <typename hashtype>
bool LSHApproxNearestNeighbourTest(const HashInfo *hinfo, flags_t flags) {
    printf("[[[ Approximate Nearest Neighbour Test ]]]\n\n");

    // assert(hinfo->hashfn == nullptr && "Error: hash function pointer is null!");
    HashFn hash = hinfo->hashFn();
    if (!hash) {
        printf("Error: hash function pointer is null!\n");
        exit(EXIT_FAILURE);
    }

    assert(hinfo->isLocalSensitive() && "Flag FLAG_HASH_LOCALITY_SENSITIVE not found. Please ensure that your LSH hash function is defined with FLAG_HASH_LOCALITY_SENSITIVE tag.");
    assert(hinfo->similarityfn != nullptr && "LSH function should be defined with a similarity metric.");
    assert((std::strlen(hinfo->similarity_name) > 0) && "The similarity metric should be named.");


    bool result = true;

    std::string filename = "../results/ApproxNearestNeighbourResults_" + std::string(hinfo->name) + ".csv";
    std::ios_base::openmode mode = std::ios::trunc; // Default: replace
    if (std::filesystem::exists(filename)) {
        mode = std::ios::app; // File exists: append
    }

    std::ofstream out_file(filename, mode);
    if (!out_file.is_open()) {
        throw std::runtime_error("Error: Could not open output file");
    }

    uint32_t sequenceLength;
    if(hinfo->onlyShortSequenceLength()){
        printf("Hash %s is marked for only Short Sequence Length.\n", hinfo->name);
        sequenceLength = g_sequenceLengthForApproxNNTest;  // Same
    }
    else{
        sequenceLength = g_sequenceLengthForApproxNNTest; // Same
    }

    SetIsTestActive(true);

    SeedGenerator seedGen(g_GoldenRatio ^ std::chrono::system_clock::now().time_since_epoch().count());

    // -----------------------------------------------------------------------------------------------------
    // 1. Generate Ref sequences.
    SequenceRecordsWithMetadataStruct ReferenceSequenceRecords;
    ReferenceSequenceRecords.KeyCount = g_Nseq_in_Database;
    ReferenceSequenceRecords.OriginalSequenceLength = sequenceLength;
    ReferenceSequenceRecords.isBasesDrawnFromUniformDist = true;
    ReferenceSequenceRecords.DataGenSeed = seedGen.nextSeed();
    ReferenceSequenceRecords.DataMutateSeed = 0; // not applicable

    [[maybe_unused]] SequenceDataGenerator dataGenReference(&ReferenceSequenceRecords);

    // print the reference sequence to the output file  //TODO: Add a random number as suffix. and proper output file positing.
    writeSequencesToFile(ReferenceSequenceRecords, "../results/reference_sequences.txt", 0);

    // -----------------------------------------------------------------------------------------------------
    // 2. Generate query sequences by randomly sampling from the reference sequences.

    const uint32_t numQueries = g_numQueriesForApproxNNTest;
    assert(numQueries < g_Nseq_in_Database && "Number of queries exceeds number of available reference sequences"); // Ensure we don't sample more queries than available sequences

    std::mt19937 rngQuery(seedGen.nextSeed());
    std::uniform_int_distribution<uint32_t> distribution(0, g_Nseq_in_Database - 1);    // [a,b], both boundaries are inclusive, thus subtracted 1.

    SequenceRecordsWithMetadataStruct QuerySequenceRecords;
    QuerySequenceRecords.KeyCount = numQueries;
    QuerySequenceRecords.OriginalSequenceLength = sequenceLength; // Sampled Queries which have not yet mutated are same length as reference.
    QuerySequenceRecords.isBasesDrawnFromUniformDist = g_isBasesDrawnFromUniformDistribution;
    QuerySequenceRecords.DataGenSeed = 0; // Not applicable, as we are sampling from reference sequences rather than generating new random sequences.
    QuerySequenceRecords.DataMutateSeed = seedGen.nextSeed();

    // Manual population of query sequences by sampling from unique k-mers
    QuerySequenceRecords.Records.resize(numQueries);
    for (uint32_t i = 0; i < numQueries; i++) {
        uint32_t idx = distribution(rngQuery);
        QuerySequenceRecords.Records[i].SeqASCIIOrg = ReferenceSequenceRecords.Records[idx].SeqASCIIOrg;
        QuerySequenceRecords.Records[i].OriginalLength = sequenceLength;
    }

    // Print the query sequences to the output file
    writeSequencesToFile(QuerySequenceRecords, "../results/query_sequences.txt", 0);

    // -----------------------------------------------------------------------------------------------------
    // 3. Perform aggegration to get the bins from simx% to simy% similarity (note: simy>simx).

    SequenceRecordsWithMetadataStruct AggSequenceRecord;
    AggSequenceRecord.KeyCount = g_NAggCasesApproxNNTest;
    AggSequenceRecord.OriginalSequenceLength = sequenceLength; // Sampled Queries which have not yet mutated are same length as reference.
    AggSequenceRecord.isBasesDrawnFromUniformDist = g_isBasesDrawnFromUniformDistribution;
    AggSequenceRecord.DataGenSeed = 0; // Not applicable, as we are sampling from reference sequences rather than generating new random sequences.
    AggSequenceRecord.DataMutateSeed = seedGen.nextSeed();

    [[maybe_unused]] SequenceDataGenerator dataGenAgg(&AggSequenceRecord);
    sim_bins_struct sim_bins = LSHInnerAgg(hinfo,AggSequenceRecord, seedGen, out_file);

    // remove the agg sequence records to free up memory, as we no longer need them.
    // We have already extracted the binned error parameters and their statistics
    // that we need for the next steps.
    AggSequenceRecord.Records.clear();
    AggSequenceRecord.Records.shrink_to_fit();

    // print bin means and stddevs using
    if(REPORT(VERBOSE, flags)) {
        for (size_t bin_idx = 0; bin_idx < sim_bins.bin_error_parameters_mean.size(); bin_idx++) {
          printf("Bin %zu: Count %d, Mean = %0.2f, Stddev = %0.2f\n", bin_idx, sim_bins.bin_fill_count[bin_idx], sim_bins.bin_error_parameters_mean[bin_idx], sim_bins.bin_error_parameters_stddev[bin_idx]);
        }
    }

    // Reset mutation-related fields in each record
    for (uint32_t i = 0; i < QuerySequenceRecords.KeyCount; i++) {
        QuerySequenceRecords.Records[i].SeqASCIIMut.clear();
        QuerySequenceRecords.Records[i].similarity = 0.0;
        QuerySequenceRecords.Records[i].foundationalParameter = 0.0;
        QuerySequenceRecords.Records[i].snpRate = 0.0;
        QuerySequenceRecords.Records[i].delRate = 0.0;
        QuerySequenceRecords.Records[i].stayRate = 0.0;
        QuerySequenceRecords.Records[i].insmean = 0.0;
        QuerySequenceRecords.Records[i].insRate = 0.0;
    }

    // Generate new error parameters targeting similarity range [low, high]
    constexpr double target_sim_low = 0.95; // Only two digits are allowed after decimal.
    constexpr double target_sim_high = 1.0; // Only two digits are allowed after decimal.

    // Convert similarity values to bin indices using the same formula as the binning step
    // bin_index = (sim - binstart) / binsize, clamped to [0, bincount-1]
    // This makes the intent explicit and stays consistent with how bins are defined.
    auto sim_to_bin_idx = [&](double sim) -> int32_t {
        int32_t idx = static_cast<int32_t>((sim - AggSequenceRecord.binstart) / AggSequenceRecord.binsize);
        return std::clamp(idx, 0, static_cast<int32_t>(AggSequenceRecord.bincount) - 1);
    };

    const int32_t target_bin_low  = sim_to_bin_idx(target_sim_low);   // = 95
    const int32_t target_bin_high = sim_to_bin_idx(target_sim_high);  // = 99, not 100

    Randbin rng_bin_sampler(seedGen.nextSeed());
    Randbin rng_bin_params_sampler(seedGen.nextSeed());

    uint32_t skipped_sequences = 0;
    for (uint32_t idx = 0; idx < QuerySequenceRecords.KeyCount; idx++) {
        uint32_t bin_fill_count = 0;
        int sampled_binid = -1;
        uint32_t attempts = 0;
        constexpr uint32_t max_attempts = 1000;

        while (bin_fill_count == 0 && attempts < max_attempts) {
            sampled_binid = rng_bin_sampler.rand_custom_range(target_bin_low, target_bin_high);
            // printf("Attempt %u: Sampled bin ID = %d\n", attempts + 1,sampled_binid);
            bin_fill_count = sim_bins.bin_fill_count[sampled_binid];  // get the number of entries in this bin. We get this from the agg computation step.
            attempts++;
        }

        if (bin_fill_count == 0) {
            printf("Warning: Could not find non-empty bin after %u attempts\n", max_attempts);
            // sequenceRecordsforTest.Records[idx].snpRate = 1.0; // Assign a default value TODO: Need to handle this.
            skipped_sequences++;
            continue; // Skip this sequence or handle error
        }

        uint32_t rand_param_idx = rng_bin_params_sampler.rand_range(bin_fill_count);
        double sampled_error_param = sim_bins.bin_error_parameters[sampled_binid][rand_param_idx];
        QuerySequenceRecords.Records[idx].foundationalParameter = sampled_error_param;
    }

    if (skipped_sequences > 0) {
        printf("Skipped %u sequences due to empty bins.\n", skipped_sequences);
    }

    if (g_mutation_model == MUTATION_MODEL_SIMPLE_SNP_ONLY) {
        [[maybe_unused]] SequenceDataMutatorSubstitutionOnly dataMutTest(&QuerySequenceRecords, hinfo->similarityfn);
        printf("Completed mutation using simple SNP only model.\n");
    } else if (g_mutation_model == MUTATION_MODEL_GEOMETRIC_MUTATOR) {
        [[maybe_unused]] SequenceDataMutatorGeometric dataMutTest(&QuerySequenceRecords, hinfo->similarityfn);
        printf("Completed mutation using geometric mutator model.\n");
    }

    printf("Re-sampled mutated %u sequences targeting similarity in [%.2f, %.2f].\n", QuerySequenceRecords.KeyCount, target_sim_low, target_sim_high);

    writeSequencesToFile(QuerySequenceRecords, "../results/query_sequences.txt", 2);

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
    for (uint32_t i = 0; i < QuerySequenceRecords.KeyCount; i++) {
    double s = QuerySequenceRecords.Records[i].similarity;
    sum_sim += s;
    if (s < min_sim)
      min_sim = s;
    if (s > max_sim)
      max_sim = s;
    }
    double mean_sim = sum_sim / QuerySequenceRecords.KeyCount;

    double sq_diff_sum = 0.0;
    for (uint32_t i = 0; i < QuerySequenceRecords.KeyCount; i++) {
        double diff = QuerySequenceRecords.Records[i].similarity - mean_sim;
        sq_diff_sum += diff * diff;
    }
    double stddev_sim = sqrt(sq_diff_sum / QuerySequenceRecords.KeyCount);

    printf("Similarity stats (%u records): Mean = %f, Min = %f, Max = %f, Stddev = %f\n", QuerySequenceRecords.KeyCount, mean_sim, min_sim, max_sim, stddev_sim);

    //-----------------------------------------------------------------------------------------------------
    // 4: Uniquification
    // It is important that we perform this uniquification, since in certain cases of probability distributions(low complexity)
    // there is a high chance of generating duplicate sequences in the reference.

    // Create a mapping of unique k-mers in the reference to their positions for efficient similarity lookups.
    std::unordered_map<std::string, std::vector<uint32_t>> uniqueKmerPositions;
    for (uint32_t pos = 0; pos < g_Nseq_in_Database; pos++) {
        std::string sequence = ReferenceSequenceRecords.Records[pos].SeqASCIIOrg;
        uniqueKmerPositions[sequence].push_back(pos);
    }
    // print_uniqueKmerPositions(uniqueKmerPositions);

    // -----------------------------------------------------------------------------------------------------
    // 5: Compute Ground Truth for c-ANN: For each mutated query, compute similarity to all reference sequences
    // and store entires above the minimum c threshold floor. At evaluation time, for each c value,
    // the true neighbour set = all entries with sim >= c * target_sim_low.
    const double min_c = *std::min_element(g_cValuesApproxNNTest.begin(), g_cValuesApproxNNTest.end());
    const double sim_floor = min_c * target_sim_low; // Only storing entries above this

    struct GroundTruthEntry {
        uint32_t position; // Position in the reference database
        double similarity;
    };

    std::vector<std::vector<GroundTruthEntry>> groundTruthAll(QuerySequenceRecords.KeyCount);

    printf("Computing ground truth (c-ANN) for %u queries across %u reference sequences (sim floor = %.4f)...\n", QuerySequenceRecords.KeyCount, g_Nseq_in_Database, sim_floor);

    for (uint32_t q_idx = 0; q_idx < QuerySequenceRecords.KeyCount; q_idx++) {
        const std::string &querySeq = QuerySequenceRecords.Records[q_idx].SeqASCIIMut;
        uint32_t mut_len = QuerySequenceRecords.Records[q_idx].MutatedLength;

        // Step A: Compute similarity between this query and EVERY sequence in the reference,
        // keeping only those above the minimum threshold floor.
        for (uint32_t ref_pos = 0; ref_pos < g_Nseq_in_Database; ++ref_pos) {
            const std::string &refSeq = ReferenceSequenceRecords.Records[ref_pos].SeqASCIIOrg;
            double sim = hinfo->similarityfn(querySeq, refSeq, mut_len, sequenceLength);
            if (sim >= sim_floor) {
                groundTruthAll[q_idx].push_back({ref_pos, sim});
            }
        }

        // Step B: Sort by similarity  (most similar first) (useful for debugging/inspection)
        std::sort(groundTruthAll[q_idx].begin(), groundTruthAll[q_idx].end(),
              [](const GroundTruthEntry &a, const GroundTruthEntry &b) {
                return a.similarity > b.similarity;
              });

        if(q_idx % 100 == 0 && q_idx > 0){
          printf("  Ground truth progress: %u / %u queries done.\n", q_idx, QuerySequenceRecords.KeyCount);
        }
    }

    printf("Ground truth computation complete for %u queries.\n", QuerySequenceRecords.KeyCount);
  
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
  
    // -----------------------------------------------------------------------------------------------------
    // 6. LSH Index Construction and Querying

    // Define LSH parameters and number of runs
    const uint32_t NUM_RUNS = g_avgRunsForApproxNN;
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
    out_file << ":2:" << "Hashname," << "SequenceLength," << "Distance Metric," << "Mutation Model," << "Mutation Expression" << std::endl;
    out_file << ":3:" << hinfo->name << "," << sequenceLength << "," << hinfo->similarity_name << "," << g_mutation_model << "," << g_mutation_expression_type << std::endl;

    // Print hash function ultra-specific parameters to the output file
    hinfo->printParameters(out_file);

    out_file << ":5:b,r,c,Avg_Recall,Avg_Precision,Avg_FPR,Avg_F1_Score\n";

    for (const auto& br_pair : br_pairs) {
        uint32_t b = br_pair.first;  // Hashes per table
        uint32_t r = br_pair.second; // Number of tables

        printf("\n\n==================================================================\n");
        printf("Running experiment for b=%u, r=%u over %u runs\n", b, r, NUM_RUNS);
        printf("==================================================================\n");

        // Per-c accumulators: for each c value, accumulate metrics across runs
        std::map<double, std::vector<double>> all_runs_avg_recall;
        std::map<double, std::vector<double>> all_runs_avg_precision;
        std::map<double, std::vector<double>> all_runs_avg_fpr;
        std::map<double, std::vector<double>> all_runs_avg_f1;

        for (uint32_t run = 0; run < NUM_RUNS; ++run) {
            printf("\n--- Run %u/%u for b=%u, r=%u ---\n", run + 1, NUM_RUNS, b, r);

            ANN_LSH_Index lsh_index(r, b, hash, seedGen);

            // Populate Index from the reference database
            for (uint32_t i = 0; i < g_Nseq_in_Database; ++i) {
                lsh_index.insert(ReferenceSequenceRecords.Records[i].SeqASCIIOrg, i);
            }
      
            // ============================================================================
            // Query and Evaluation Phase for this run
            // ============================================================================
            // Per-c, per-query metric vectors
            std::map<double, std::vector<double>> recall_per_query;
            std::map<double, std::vector<double>> precision_per_query;
            std::map<double, std::vector<double>> fpr_per_query;

            for (uint32_t q_idx = 0; q_idx < QuerySequenceRecords.KeyCount; q_idx++) {
                const std::string& querySeq = QuerySequenceRecords.Records[q_idx].SeqASCIIMut;
                std::vector<uint32_t> lsh_results = lsh_index.query(querySeq);
                std::set<uint32_t> lsh_results_set(lsh_results.begin(), lsh_results.end());

                const auto &ground_truth_full = groundTruthAll[q_idx];

                // Evaluate for each c value
                for (double c : g_cValuesApproxNNTest) {
                    double sim_threshold = c * target_sim_low;

                    // Build ground truth set: all entries with sim >= c * target_sim_low
                    std::set<uint32_t> ground_truth_positions;
                    for (const auto &entry : ground_truth_full) {
                        if (entry.similarity >= sim_threshold) {
                            ground_truth_positions.insert(entry.position);
                        }
                    }

                    uint32_t true_positives = 0;
                    uint32_t false_positives = 0;

                    for (uint32_t pos : lsh_results_set) {
                        if (ground_truth_positions.count(pos)) {    //this way we didnt need repeat removal.
                            true_positives++;
                        } else {
                            false_positives++;
                        }
                    }

                    int false_negatives = static_cast<int>(ground_truth_positions.size()) - true_positives;
                    assert(false_negatives >= 0);

                    double recall = (true_positives + false_negatives > 0) ? static_cast<double>(true_positives)/(true_positives + false_negatives):0.0;
                    double precision = (true_positives + false_positives > 0) ? static_cast<double>(true_positives) /(true_positives + false_positives): 0.0;

                    uint32_t total_negatives = g_Nseq_in_Database - static_cast<uint32_t>(ground_truth_positions.size());
                    double false_positive_rate = (total_negatives > 0) ? static_cast<double>(false_positives) / total_negatives : 0.0;

                    recall_per_query[c].push_back(recall);
                    precision_per_query[c].push_back(precision);
                    fpr_per_query[c].push_back(false_positive_rate);
                }
            }

            //-------------------------------------------------------------------------------------
            // Calculate macro-averaged metrics for this single run, per c
            for (double c : g_cValuesApproxNNTest) {
                double run_avg_recall = 0.0;
                double run_avg_precision = 0.0;
                double run_avg_fpr = 0.0;

                if (QuerySequenceRecords.KeyCount > 0) {
                    for (double r_val : recall_per_query[c])
                        run_avg_recall += r_val;
                    run_avg_recall /= QuerySequenceRecords.KeyCount;

                    for (double p_val : precision_per_query[c])
                        run_avg_precision += p_val;
                    run_avg_precision /= QuerySequenceRecords.KeyCount;

                    for (double fpr_val : fpr_per_query[c])
                        run_avg_fpr += fpr_val;
                    run_avg_fpr /= QuerySequenceRecords.KeyCount;
                }

                double run_f1_score = 0.0;
                if ((run_avg_recall + run_avg_precision) > 0) {
                    run_f1_score = 2.0 * (run_avg_recall * run_avg_precision) / (run_avg_recall + run_avg_precision);
                }

                printf("  Run %u c=%.2f: Recall=%.4f, Precision=%.4f, FPR=%.4f, F1=%.4f\n", run + 1, c, run_avg_recall, run_avg_precision, run_avg_fpr, run_f1_score);

                // Store metrics for this run
                all_runs_avg_recall[c].push_back(run_avg_recall);
                all_runs_avg_precision[c].push_back(run_avg_precision);
                all_runs_avg_fpr[c].push_back(run_avg_fpr);
                all_runs_avg_f1[c].push_back(run_f1_score);
            }
        }

        // Average the metrics across all runs for the current (b, r) pair, per c
        for (double c : g_cValuesApproxNNTest) {
            double final_avg_recall = 0.0;
            double final_avg_precision = 0.0;
            double final_avg_fpr = 0.0;
            double final_avg_f1 = 0.0;

            for (double val : all_runs_avg_recall[c])
                final_avg_recall += val;
            final_avg_recall /= NUM_RUNS;

            for (double val : all_runs_avg_precision[c])
                final_avg_precision += val;
            final_avg_precision /= NUM_RUNS;

            for (double val : all_runs_avg_fpr[c])
                final_avg_fpr += val;
            final_avg_fpr /= NUM_RUNS;

            for (double val : all_runs_avg_f1[c])
                final_avg_f1 += val;
            final_avg_f1 /= NUM_RUNS;

            printf("\n--- Average Metrics for b=%u, r=%u, c=%.2f (over %u runs) ---\n", b, r, c, NUM_RUNS);
            printf("Average Recall:    %.4f\n", final_avg_recall);
            printf("Average Precision: %.4f\n", final_avg_precision);
            printf("Average FPR:       %.4f\n", final_avg_fpr);
            printf("Average F1-Score:  %.4f\n", final_avg_f1);
            printf("-----------------------------------------------------\n");

          // Write summary for this (b, r, c) triple to file
          out_file << ":6:" << b << "," << r << "," << c << ","
                   << final_avg_recall << "," << final_avg_precision << ","
                   << final_avg_fpr << "," << final_avg_f1 << "\n";
        }
    }

    // Cleanup: Reset LSH global variables after test completion
    SetIsTestActive(false);

    // printf("%s\n", result ? "" : g_failstr);
    out_file.close();
    return result;
}

INSTANTIATE(LSHApproxNearestNeighbourTest, HASHTYPELIST);
