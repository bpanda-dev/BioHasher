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
#include "fstream"
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

void perform_sanity_checks_for_test_configuration(const HashInfo *hinfo,const uint32_t tokenlength,const uint32_t referenceLen,const uint32_t windowlength) {

  // Sanity check of windowlength.
  if (windowlength > referenceLen) {
    printf("Error: Window length (%u) > reference length (%u).\n", windowlength,referenceLen);
    exit(EXIT_FAILURE);
  }

  // Calculate the number of windows the sequence will be split into.
  // const uint64_t windowsInSequence = (referenceLen - windowlength + 1);
  const uint64_t windowsInSequence = (referenceLen/windowlength); // Using non-overlapping windows for sanity check, as this is what the LSH test will use.
  // Condition 2 : Skipping if the sequence is too short to provide enough
  // windows
  const uint64_t MIN_WINDOWS = 50; // A more reasonable threshold.
  if (windowsInSequence < MIN_WINDOWS) {
    printf("Skipping: Sequence with %llu windows is too short (min is %llu).\n", (unsigned long long)windowsInSequence,(unsigned long long)MIN_WINDOWS);
    exit(EXIT_FAILURE);
  }

  // Condition 3 : Skipping if the window space is too small, which would lead
  // to high collision rates even for random data, making the LSH property test
  // less meaningful.
  if (windowlength > 0) {
    uint64_t maxPossibleWindows = (windowlength >= 32) ? UINT64_MAX : (1ULL << (2 * windowlength));
    // This checks if the number of windows in the sequence is a large fraction
    // of the total possible unique windows. For example, skip if
    // windowsInSequence > 1% of maxPossibleWindows.
    if (maxPossibleWindows / 100 < windowsInSequence) {
      printf("Skipping: High window repetition expected. Windows in sequence (%llu) vs. max possible (%llu).\n", (unsigned long long)windowsInSequence,(unsigned long long)maxPossibleWindows);
      exit(EXIT_FAILURE);
    }
  }
  g_window_length = windowlength; // Set global window length for testing

  // --- Sanity Checks for Test Configuration Only valid for tokenised hashes
  // ---	//
  if (hinfo->hasTokenisationProperty()) {
    // Condition 1: Skipping if token length is greater than sequence length.
    if (tokenlength > windowlength) {
      printf("Skipping: Token length (%u) > sequence length (%u).\n", tokenlength, windowlength);
      exit(EXIT_FAILURE);
    }

    // Calculate the number of tokens the sequence will be split into.
    const uint64_t tokensInSequence = (windowlength - tokenlength + 1); // Cardinality
    // Condition 2: Skipping if the sequence is too short to provide enough
    // tokens for meaningful mutation analysis and avoid natural repetition.
    const uint64_t MIN_TOKENS_FOR_MUTATION = 20; // A more reasonable threshold.
    if (tokensInSequence < MIN_TOKENS_FOR_MUTATION) {
      printf( "Skipping: Sequence with %llu tokens is too short (min is %llu).\n", (unsigned long long)tokensInSequence, (unsigned long long)MIN_TOKENS_FOR_MUTATION);
      exit(EXIT_FAILURE);
    }

    // Condition 3: Skipping if the token space is too small, which would lead
    // to high collision rates even for random data, making the LSH property
    // test less meaningful.
    if (tokenlength > 0) {
      uint64_t maxPossibleTokens =
          (tokenlength >= 32) ? UINT64_MAX : (1ULL << (2 * tokenlength));

      // This checks if the number of tokens in the sequence is a large fraction
      // of the total possible unique tokens. For example, skip if
      // tokensInSequence > 10% of maxPossibleTokens.
      if (maxPossibleTokens / 10 < tokensInSequence) {
        printf("Skipping: High token repetition expected. Tokens in sequence (%llu) vs. max possible (%llu).\n", (unsigned long long)tokensInSequence, (unsigned long long)maxPossibleTokens);
        exit(EXIT_FAILURE);
      }
    }

    g_TokenLength = tokenlength; // Set global token length for testing
  }
}

static sim_bins_struct LSHCollisionTestInnerAgg(SequenceRecordsWithMetadataStruct &sequenceRecordsForAgg, SeedGenerator &seedGen, std::ofstream &out_file) {

  printf("Inside get_params_aggregated_in_bins\n");

  sim_bins_struct sim_bins_agg; // Bin to store the error parameters

  std::vector<double> rand_error_param(sequenceRecordsForAgg.KeyCount, 0.0);
  std::vector<double> similarity_values(sequenceRecordsForAgg.KeyCount, 0.0);

  printf("\n-----------------Start AGG----------------");
  if (g_mutation_model == MUTATION_MODEL_SIMPLE_SNP_ONLY) {
    SequenceDataMutatorSubstitutionOnly dataMutAgg(&sequenceRecordsForAgg, &rand_error_param);
  } else if (g_mutation_model == MUTATION_MODEL_GEOMETRIC_MUTATOR) {
    printf("Using geometric mutator model for data mutation in aggregation phase.\n");
    SequenceDataMutatorGeometric dataMutAgg(&sequenceRecordsForAgg, &rand_error_param);
  }
  printf("\n-----------------End AGG----------------");

  // Print Similarity values
  out_file << ":13:";
  for (size_t i = 0; i < sequenceRecordsForAgg.KeyCount; i++) {
    if (i == sequenceRecordsForAgg.KeyCount - 1)
      out_file << rand_error_param[i] << "\n";
    else
      out_file << rand_error_param[i] << ",";
  }

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
      // printf("Bin %zu: Sum = %f, Count = %u\n", bin_idx, sum,
      // sim_bins_agg.bin_fill_count[bin_idx]);
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
    printf(
        "Hash %s has tokenisation property. Testing multiple token lengths.\n",
        hinfo->name);
    tokenlength = 13;
  } else {
    tokenlength = 0; // No tokenization
  }

  // Note: The tokenlength is different from window size. Token is like a kmer.
  // Window size is the length of the sequence that we are hashing.

  uint32_t referenceLen = 10000;
  uint32_t windowlength = 45;

  perform_sanity_checks_for_test_configuration(hinfo, tokenlength, referenceLen, windowlength);

  if (windowlength == 0) { // Redundant, but just in case the sanity check is bypassed for some reason.
    printf("Error: Window length of 0 is not valid for this test.\n");
    exit(EXIT_FAILURE); // Window length of 0 is not valid for this test.
  }

  // -----------------------------------------------------------------------------------------------------
  // // Generate long sequence.

  seed_t baseSeed = g_GoldenRatio ^ std::chrono::system_clock::now().time_since_epoch().count();
  SeedGenerator seedGen(baseSeed);

  SequenceRecordsWithMetadataStruct ReferenceSequenceRecord;
  ReferenceSequenceRecord.KeyCount = 1; // Will contain one sequence
  ReferenceSequenceRecord.OriginalSequenceLength = referenceLen;
  ReferenceSequenceRecord.DistanceClass = 0;
  ReferenceSequenceRecord.isBasesDrawnFromUniformDist = true;
  ReferenceSequenceRecord.DatagenSeed = seedGen.nextSeed();
  ReferenceSequenceRecord.DataMutateSeed =
      0; // No mutation for reference sequence

  SequenceDataGenerator dataGenReference(&ReferenceSequenceRecord);

  // print the reference sequence to the output file
  out_file << ":0:Reference Sequence\n";
  out_file << ReferenceSequenceRecord.OriginalSequenceLength << std::endl;
  out_file << ReferenceSequenceRecord.Records[0].SeqASCIIOrg << std::endl;

  // -----------------------------------------------------------------------------------------------------
  // //
  // Extract all k-mers (windows) from the reference sequence.
  // k-mer size = windowlength, using a sliding window with stride 1.
  const std::string &refSeq = ReferenceSequenceRecord.Records[0].SeqASCIIOrg;

  // Store k-mers with their positions for nearest neighbour lookups
  struct KmerEntry {
    std::string kmer;
    uint32_t position; // Start position in the reference sequence
  };

  std::vector<KmerEntry> referenceKmers;

  if (refSeq.length() >= windowlength) {
    // uint32_t numKmers = static_cast<uint32_t>(refSeq.length() - windowlength + 1);
    uint32_t numKmers = static_cast<uint32_t>(refSeq.length() / windowlength);
    referenceKmers.reserve(numKmers);

    for (uint32_t pos = 0; pos <= refSeq.length() - windowlength; pos += windowlength) {
      referenceKmers.push_back({refSeq.substr(pos, windowlength), pos});
    }
  }

  printf("Extracted %zu k-mers (window size = %u) from reference sequence of length %u.\n", referenceKmers.size(), windowlength, referenceLen);

  // Also build a set of unique k-mers for fast lookup / deduplication if needed
  std::unordered_map<std::string, std::vector<uint32_t>> uniqueKmerPositions;
  for (const auto &entry : referenceKmers) {
    uniqueKmerPositions[entry.kmer].push_back(entry.position);
  }

  printf("Unique k-mers: %zu out of %zu total.\n", uniqueKmerPositions.size(), referenceKmers.size());

  // Write k-mer stats to output file
  out_file << ":2:Kmer Stats\n";
  out_file << "total_kmers," << referenceKmers.size() << std::endl;
  out_file << "unique_kmers," << uniqueKmerPositions.size() << std::endl;

  // -----------------------------------------------------------------------------------------------------
  // // Generate query sequences by randomly sampling from the unique k-mers in
  // the reference.
  const uint32_t numQueries = 100;

  // Collecting unique k-mers into a vector for random access
  std::vector<std::string> uniqueKmerList;
  uniqueKmerList.reserve(uniqueKmerPositions.size());
  for (const auto &entry : uniqueKmerPositions) {
    uniqueKmerList.push_back(entry.first);
  }

  printf("Sampling %u query k-mers from %zu unique k-mers.\n", numQueries, uniqueKmerList.size());

  if (uniqueKmerList.size() < numQueries) {
    printf("Warning: Not enough unique k-mers (%zu) to sample %u queries. Sampling with replacement.\n", uniqueKmerList.size(), numQueries);
  }

  Rand rngQuery(seedGen.nextSeed()); // seeded RNG for reproducible sampling

  SequenceRecordsWithMetadataStruct QuerySequenceRecord;
  QuerySequenceRecord.KeyCount = numQueries;
  QuerySequenceRecord.OriginalSequenceLength = windowlength; // Queries are same length as k-mers
  QuerySequenceRecord.isBasesDrawnFromUniformDist = true;
  QuerySequenceRecord.DatagenSeed = 0; // Not used for data generation here
  QuerySequenceRecord.DataMutateSeed = seedGen.nextSeed();
  QuerySequenceRecord.DistanceClass =  setDistanceClassForHashInfo(hinfo->hash_flags);

  // Manual population of query sequences by sampling from unique k-mers
  QuerySequenceRecord.Records.resize(numQueries);
  for (uint32_t i = 0; i < numQueries; i++) {
    uint32_t idx = rngQuery.rand_range(static_cast<uint32_t>(uniqueKmerList.size()));
    QuerySequenceRecord.Records[i].SeqASCIIOrg = uniqueKmerList[idx];
    QuerySequenceRecord.Records[i].OriginalLength = windowlength;
  }

  printf("Sampled %u query k-mers (with replacement) from reference unique k-mers.\n", numQueries);

  printf("Window Length: %u\n", windowlength);

  // Print the query sequences to the output file
  out_file << ":1:Query Sequences\n";
  out_file << QuerySequenceRecord.KeyCount << std::endl;
  for (uint32_t i = 0; i < QuerySequenceRecord.KeyCount; i++) {
    out_file << QuerySequenceRecord.Records[i].SeqASCIIOrg << std::endl;
  }

  // Perform aggegration to get the bins from 95% to 100% similarity.
  sim_bins_struct sim_bins = LSHCollisionTestInnerAgg(QuerySequenceRecord, seedGen, out_file);

  // print bin means and stddevs using
  if (REPORT(VERBOSE, flags)) {
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

  // printf("\n>>\n");
  // // print the query, the mutated sequence and the similarity values to terminal
  // for (uint32_t i = 0; i < QuerySequenceRecord.KeyCount; i++) {
  //   printf("Foundational Parameter for record %u: %f\n", i,
  //          QuerySequenceRecord.Records[i].foundationalParameter);
  //   printf("Similarity for record %u: %f\n", i,
  //          QuerySequenceRecord.Records[i].similarity);
  //   printf("Query Sequence: %s\n",
  //          QuerySequenceRecord.Records[i].SeqASCIIOrg.c_str());
  //   // printf("Mutated Sequence: %s\n",
  //   // QuerySequenceRecord.Records[i].SeqASCIIMut.c_str());
  // }

  // Generate new error parameters targeting similarity range [0.95, 1.0]

  const double target_sim_low = 0.95;  // 95% similarity
  const double target_sim_high = 1.00; // 100% similarity (no mutation)

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
      // printf("Attempt %u: Sampled bin ID = %d\n", attempts + 1,
      // sampled_binid);
      bin_fill_count = sim_bins.bin_fill_count[sampled_binid];
      attempts++;
    }

    if (bin_fill_count == 0) {
      printf("Warning: Could not find non-empty bin after %u attempts\n", max_attempts);
      // sequenceRecordsforTest.Records[idx].snpRate = 1.0; // Assign a default
      // value
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

  printf(
      "Re-sampled mutated %u sequences targeting similarity in [%.2f, %.2f].\n",
      QuerySequenceRecord.KeyCount, target_sim_low, target_sim_high);

  // -----------------------------------------------------------------------------------------------------
  // //
  // print the query, the mutated sequence and the similarity values to terminal
  printf("\n\n");
  for (uint32_t i = 0; i < QuerySequenceRecord.KeyCount; i++) {
    printf("Foundational Parameter for record %u: %f\n", i, QuerySequenceRecord.Records[i].foundationalParameter);
    printf("Similarity for record %u: %f\n", i, QuerySequenceRecord.Records[i].similarity);
    printf("Query Sequence: %s\n", QuerySequenceRecord.Records[i].SeqASCIIOrg.c_str());
    printf("Mutated Sequence: %s\n", QuerySequenceRecord.Records[i].SeqASCIIMut.c_str());
  }

  // Write results to output file
  out_file << ":14:Re-sampled similarity values)\n";
  for (uint32_t i = 0; i < QuerySequenceRecord.KeyCount; i++) {
    out_file << QuerySequenceRecord.Records[i].similarity;
    if (i < QuerySequenceRecord.KeyCount - 1)
      out_file << ",";
  }
  out_file << "\n";

  // Debug: Print similarity statistics across all query records
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

  // -----------------------------------------------------------------------------------------------------
  // // Compute Ground Truth: For each mutated query, find the closest 10 k-mers
  // in the reference. For each query, compute similarity to ALL unique
  // reference k-mers using the selected distance metric, then pick the top 10.
  // Each k-mer stores all its positions in the reference, sorted most-similar
  // first.
  // -----------------------------------------------------------------------------------------------------
  // //
  const uint32_t TOP_K = 10;
  uint32_t distanceClass = QuerySequenceRecord.DistanceClass;

  struct GroundTruthEntry {
    std::string kmer;
    double similarity;
    std::vector<uint32_t> positions; // All positions of this kmer in the reference
  };

  std::vector<std::vector<GroundTruthEntry>> groundTruthNearest(QuerySequenceRecord.KeyCount);

  printf("Computing ground truth top-%u nearest k-mers for %u queries across %zu unique reference k-mers...\n", TOP_K, QuerySequenceRecord.KeyCount, uniqueKmerPositions.size());

  for (uint32_t q_idx = 0; q_idx < QuerySequenceRecord.KeyCount; q_idx++) {
    const std::string &querySeq = QuerySequenceRecord.Records[q_idx].SeqASCIIMut;

    // Step 1: Compute similarity between this query and every unique k-mer in the reference
    std::vector<std::pair<double, std::string>> simPerUniqueKmer;
    simPerUniqueKmer.reserve(uniqueKmerPositions.size());
    for (const auto &entry : uniqueKmerPositions) {
      const std::string &refKmer = entry.first;
      double sim = 0.0;
      if (distanceClass == 1) { // Hamming
        sim = ComputeHammingSimilarity(querySeq, refKmer, windowlength);
      } else if (distanceClass == 2) { // Jaccard
        sim = ComputeJaccardSimilarity(querySeq, refKmer, g_TokenLength);
      } else if (distanceClass == 3) { // Cosine
        sim = ComputeCosineSimilarity(querySeq, refKmer, g_TokenLength);
      } else if (distanceClass == 4) { // Angular
        sim = ComputeAngularSimilarity(querySeq, refKmer, g_TokenLength);
      } else if (distanceClass == 5) { // Edit Distance
        sim = ComputeEditSimilarity(querySeq, refKmer);
      }
      simPerUniqueKmer.push_back({sim, refKmer});
    }
    // Step 2: Sort by similarity in descending order (most similar first)
    std::sort(simPerUniqueKmer.begin(), simPerUniqueKmer.end(), 
              [](const std::pair<double, std::string> &a, const std::pair<double, std::string> &b) 
                {
                  return a.first > b.first;
                });


    // Debug: Print top 10 similarities for this query
    printf("Top %u nearest k-mers for query %u:\n", TOP_K, q_idx);
    for (uint32_t i = 0; i < TOP_K && i < simPerUniqueKmer.size(); i++) {
      printf("%u: %s (similarity = %.2f)\n", i + 1, simPerUniqueKmer[i].second.c_str(), simPerUniqueKmer[i].first);
    }
    
    // Step 3: Take the top-K unique k-mers, each with all their reference positions.
    // If multiple k-mers share the same similarity at the boundary, we
    // simply stop once TOP_K is reached. 	
    std::vector<GroundTruthEntry>& topK = groundTruthNearest[q_idx];
    topK.reserve(TOP_K); 
    for(const auto& kmerSim : simPerUniqueKmer){ 		
        if(topK.size() >= TOP_K)
          break;
        const std::vector<uint32_t>& positions = uniqueKmerPositions.at(kmerSim.second);
        topK.push_back({kmerSim.second, kmerSim.first, positions});
    }
    if(q_idx % 200 == 0){
      printf("  Ground truth progress: %u / %u queries done.\n", q_idx, QuerySequenceRecord.KeyCount);
    }
  }
  
  printf("Ground truth computation complete for %u queries.\n",  QuerySequenceRecord.KeyCount);
  
  // Write ground truth to output file
  out_file << ":15:Ground Truth Top-" << TOP_K << " Nearest K-mers\n";
  for(uint32_t q_idx = 0; q_idx < QuerySequenceRecord.KeyCount; q_idx++){
    out_file << "Q" << q_idx << ":";
    for(size_t k = 0; k < groundTruthNearest[q_idx].size(); k++){
      const auto& entry = groundTruthNearest[q_idx][k];
      out_file << entry.kmer << "," << entry.similarity << ",[";
      for(size_t p = 0; p < entry.positions.size(); p++){
        out_file << entry.positions[p];
        if(p < entry.positions.size() - 1) out_file << "|";
      }
      out_file << "]";
      if(k < groundTruthNearest[q_idx].size() - 1) out_file << ";";
    }
    out_file << "\n";
  }

  // Compute Hash tables. using 2 AND and 3 tables.

  seed_t hashSeed1A = seedGen.nextSeed();
  seed_t hashSeed1B = seedGen.nextSeed();
  seed_t hashSeed1C = seedGen.nextSeed();
  seed_t hashSeed1D = seedGen.nextSeed();
  
  seed_t hashSeed2A = seedGen.nextSeed();
  seed_t hashSeed2B = seedGen.nextSeed();
  seed_t hashSeed2C = seedGen.nextSeed();
  seed_t hashSeed2D = seedGen.nextSeed();

  seed_t hashSeed3A = seedGen.nextSeed();
  seed_t hashSeed3B = seedGen.nextSeed();
  seed_t hashSeed3C = seedGen.nextSeed();
  seed_t hashSeed3D = seedGen.nextSeed();

  // Define a hash table type: mapping hash value to vector of kmer positions
  using HashTable = std::unordered_map<uint64_t, std::vector<uint32_t>>;

  HashFn hash = hinfo->hashFn(g_hashEndian);
  if (!hash) {
      printf("Error: hash function pointer is null!\n");
      exit(EXIT_FAILURE);
  }
  // Helper: Compute AND of two hashes for a k-mer
  auto compute_hash_and = [&](const std::string& kmer, seed_t seedA, seed_t seedB, seed_t seedC, seed_t seedD) -> uint64_t {
      uint64_t hashA = 0;
      uint64_t hashB = 0;
      uint64_t hashC = 0;
      uint64_t hashD = 0;

      const uint8_t* strPtr = (const uint8_t*)kmer.c_str();
      size_t strLen = kmer.length();
      printf("Processing k-mer '%s' of length %zu\n", kmer.c_str(), strLen);
      
      hash(strPtr, strLen, seedA, &hashA);
      hash(strPtr, strLen, seedB, &hashB);
      hash(strPtr, strLen, seedC, &hashC);
      hash(strPtr, strLen, seedD, &hashD);

      return hashA & hashB & hashC & hashD;
  };

  // Create three hash tables
  HashTable hashTable1, hashTable2, hashTable3;

  // Populate hashTable1: AND of hashes with hashSeed1A and hashSeed1B
  for (const auto& entry : referenceKmers) {
      uint64_t hash_val = compute_hash_and(entry.kmer, hashSeed1A, hashSeed1B, hashSeed1C, hashSeed1D);
      printf("Hash AND for k-mer '%s': 0x%016llx at position %u\n", entry.kmer.c_str(), (unsigned long long)hash_val, entry.position);
      hashTable1[hash_val].push_back(entry.position);
  }

  // Populate hashTable2: AND of hashes with hashSeed2A and hashSeed2B
  for (const auto& entry : referenceKmers) {
      uint64_t hash_val = compute_hash_and(entry.kmer, hashSeed2A, hashSeed2B, hashSeed2C, hashSeed2D);
       printf("Hash AND for k-mer '%s': 0x%016llx at position %u\n", entry.kmer.c_str(), (unsigned long long)hash_val, entry.position);
      hashTable2[hash_val].push_back(entry.position);
  }

  // Populate hashTable3: AND of hashes with hashSeed3A and hashSeed3B
  for (const auto& entry : referenceKmers) {
      uint64_t hash_val = compute_hash_and(entry.kmer, hashSeed3A, hashSeed3B, hashSeed3C, hashSeed3D);
      hashTable3[hash_val].push_back(entry.position);
  }
  
  auto print_hash_table = [](const HashTable& table, const std::string& name) {
      printf("Contents of %s:\n", name.c_str());
      for (const auto& [hash_val, positions] : table) {
          printf("Hash: 0x%016llx -> Positions: [", (unsigned long long)hash_val);
          for (size_t i = 0; i < positions.size(); ++i) {
              printf("%u", positions[i]);
              if (i < positions.size() - 1) printf(", ");
          }
          printf("]\n");
      }
      printf("\n");
  };

  // Print all three hash tables
  print_hash_table(hashTable1, "HashTable1");
  print_hash_table(hashTable2, "HashTable2");
  print_hash_table(hashTable3, "HashTable3");

  
  // Query phase: For each query, search in hashtable 1, hashtable2 and hash table3.
  printf("\n\n========== LSH QUERY PHASE ==========\n");

  // Helper: Deduplicate and sort positions
  auto aggregate_positions = [](const std::vector<std::vector<uint32_t>>& position_lists) -> std::vector<uint32_t> {
      std::set<uint32_t> unique_positions;
      for (const auto& list : position_lists) {
          for (uint32_t pos : list) {
              unique_positions.insert(pos);
          }
      }
      return std::vector<uint32_t>(unique_positions.begin(), unique_positions.end());
  };

  // Accumulate metrics across all queries
  uint32_t total_true_positives = 0;
  uint32_t total_false_positives = 0;
  uint32_t total_false_negatives = 0;
  uint32_t total_queries_with_results = 0;
  std::vector<double> recall_per_query;
  std::vector<double> precision_per_query;

  // For each query
  for (uint32_t q_idx = 0; q_idx < QuerySequenceRecord.KeyCount; q_idx++) {
      const std::string& querySeq = QuerySequenceRecord.Records[q_idx].SeqASCIIMut;
      
      // Compute hash values for this query using the same seeds
      uint64_t query_hash1 = compute_hash_and(querySeq, hashSeed1A, hashSeed1B, hashSeed1C, hashSeed1D);
      uint64_t query_hash2 = compute_hash_and(querySeq, hashSeed2A, hashSeed2B, hashSeed2C, hashSeed2D);
      uint64_t query_hash3 = compute_hash_and(querySeq, hashSeed3A, hashSeed3B, hashSeed3C, hashSeed3D);

      // Look up in all three hash tables
      std::vector<std::vector<uint32_t>> position_lists;
      
      if (hashTable1.find(query_hash1) != hashTable1.end()) {
          position_lists.push_back(hashTable1.at(query_hash1));
      }
      if (hashTable2.find(query_hash2) != hashTable2.end()) {
          position_lists.push_back(hashTable2.at(query_hash2));
      }
      if (hashTable3.find(query_hash3) != hashTable3.end()) {
          position_lists.push_back(hashTable3.at(query_hash3));
      }

      // Aggregate and deduplicate positions
      std::vector<uint32_t> lsh_results = aggregate_positions(position_lists);
      
      // Convert to sets for easier comparison
      std::set<uint32_t> lsh_results_set(lsh_results.begin(), lsh_results.end());
      
      // Collect ground truth positions
      const auto& ground_truth = groundTruthNearest[q_idx];
      std::set<uint32_t> ground_truth_positions;
      for (const auto& entry : ground_truth) {
          for (uint32_t pos : entry.positions) {
              ground_truth_positions.insert(pos);
          }
      }
      
      // Calculate TP, FP, FN for this query
      uint32_t true_positives = 0;      // In both LSH and ground truth
      uint32_t false_positives = 0;     // In LSH but NOT in ground truth
      uint32_t false_negatives = 0;     // In ground truth but NOT in LSH
      
      // Count true positives and false positives
      for (uint32_t pos : lsh_results_set) {
          if (ground_truth_positions.find(pos) != ground_truth_positions.end()) {
              true_positives++;
          } else {
              false_positives++;
          }
      }
      
      // Count false negatives
      for (uint32_t pos : ground_truth_positions) {
          if (lsh_results_set.find(pos) == lsh_results_set.end()) {
              false_negatives++;
          }
      }
      
      // Calculate Recall and Precision for this query
      double recall = 0.0;
      double precision = 0.0;
      
      // Recall = TP / (TP + FN) = TP / |ground_truth|
      if ((true_positives + false_negatives) > 0) {
          recall = (double)true_positives / (true_positives + false_negatives);
      }
      
      // Precision = TP / (TP + FP) = TP / |LSH_results|
      if ((true_positives + false_positives) > 0) {
          precision = (double)true_positives / (true_positives + false_positives);
      }
      
      recall_per_query.push_back(recall);
      precision_per_query.push_back(precision);
      
      printf("\nQuery %u: '%s'\n", q_idx, querySeq.c_str());
      printf("  Hash1: 0x%016llx -> %zu positions\n", (unsigned long long)query_hash1,hashTable1.find(query_hash1) != hashTable1.end() ? hashTable1.at(query_hash1).size() : 0);
      printf("  Hash2: 0x%016llx -> %zu positions\n", (unsigned long long)query_hash2,hashTable2.find(query_hash2) != hashTable2.end() ? hashTable2.at(query_hash2).size() : 0);
      printf("  Hash3: 0x%016llx -> %zu positions\n", (unsigned long long)query_hash3,hashTable3.find(query_hash3) != hashTable3.end() ? hashTable3.at(query_hash3).size() : 0);
      printf("  Aggregated candidates (deduplicated): %zu\n", lsh_results.size());

      // Print aggregated candidates (deduplicated)
      printf("  Aggregated candidates: [");
      for (size_t i = 0; i < lsh_results.size(); i++) {
          printf("%u", lsh_results[i]);
          if (i < lsh_results.size() - 1) printf(", ");
      }
      printf("]\n");

      printf("  Ground truth size: %zu\n", ground_truth_positions.size());

      // Print ground truth positions
      printf("  Ground truth positions: [");
      size_t gt_idx = 0;
      for (uint32_t pos : ground_truth_positions) {
          printf("%u", pos);
          if (++gt_idx < ground_truth_positions.size()) printf(", ");
      }
      printf("]\n");

      printf("  TP (in both LSH and GT): %u\n", true_positives);
      printf("  FP (in LSH but not GT): %u\n", false_positives);
      printf("  FN (in GT but not LSH): %u\n", false_negatives);
      printf("  Recall: %.4f\n", recall);
      printf("  Precision: %.4f\n", precision);
      
      // Accumulate metrics
      if (lsh_results.size() > 0) {
          total_queries_with_results++;
      }
      total_true_positives += true_positives;
      total_false_positives += false_positives;
      total_false_negatives += false_negatives;

      // Write results to output file
      out_file << ":16:Query " << q_idx << " LSH Results\n";
      out_file << "LSH_candidates," << lsh_results.size() << "\n";
      out_file << "Ground_truth_count," << ground_truth_positions.size() << "\n";
      out_file << "True_positives," << true_positives << "\n";
      out_file << "False_positives," << false_positives << "\n";
      out_file << "False_negatives," << false_negatives << "\n";
      out_file << "Recall," << recall << "\n";
      out_file << "Precision," << precision << "\n";
      out_file << "Candidate_positions,";
      for (size_t i = 0; i < lsh_results.size(); i++) {
          out_file << lsh_results[i];
          if (i < lsh_results.size() - 1) out_file << "|";
      }
      out_file << "\n";
      out_file << "Ground_truth_positions,";
      size_t gt_count = 0;
      for (uint32_t pos : ground_truth_positions) {
          out_file << pos;
          if (++gt_count < ground_truth_positions.size()) out_file << "|";
      }
      out_file << "\n";
  }

  // Calculate aggregate metrics
  double avg_recall = 0.0;
  double avg_precision = 0.0;

  if (QuerySequenceRecord.KeyCount > 0) {
      for (double r : recall_per_query) {
          avg_recall += r;
      }
      avg_recall /= QuerySequenceRecord.KeyCount;
      
      for (double p : precision_per_query) {
          avg_precision += p;
      }
      avg_precision /= QuerySequenceRecord.KeyCount;
  }

  // Calculate macro-averaged F1 score
  double f1_score = 0.0;
  if ((avg_recall + avg_precision) > 0) {
      f1_score = 2.0 * (avg_recall * avg_precision) / (avg_recall + avg_precision);
  }

  // Calculate overall recall and precision across all queries
  double overall_recall = 0.0;
  double overall_precision = 0.0;

  if ((total_true_positives + total_false_negatives) > 0) {
      overall_recall = (double)total_true_positives / (total_true_positives + total_false_negatives);
  }

  if ((total_true_positives + total_false_positives) > 0) {
      overall_precision = (double)total_true_positives / (total_true_positives + total_false_positives);
  }

  double overall_f1 = 0.0;
  if ((overall_recall + overall_precision) > 0) {
      overall_f1 = 2.0 * (overall_recall * overall_precision) / (overall_recall + overall_precision);
  }

  // Print summary statistics
  printf("\n========== LSH QUERY SUMMARY ==========\n");
  printf("Total queries: %u\n", QuerySequenceRecord.KeyCount);
  printf("Queries with LSH results: %u\n", total_queries_with_results);
  printf("\n--- Aggregate Metrics ---\n");
  printf("Total True Positives (TP): %u\n", total_true_positives);
  printf("Total False Positives (FP): %u\n", total_false_positives);
  printf("Total False Negatives (FN): %u\n", total_false_negatives);
  printf("\n--- Overall Metrics (Micro-averaged) ---\n");
  printf("Overall Recall: %.4f\n", overall_recall);
  printf("Overall Precision: %.4f\n", overall_precision);
  printf("Overall F1-Score: %.4f\n", overall_f1);
  printf("\n--- Per-Query Average (Macro-averaged) ---\n");
  printf("Average Recall: %.4f\n", avg_recall);
  printf("Average Precision: %.4f\n", avg_precision);
  printf("Average F1-Score: %.4f\n", f1_score);

  // Write summary to output file
  out_file << ":17:LSH Query Summary\n";
  out_file << "Total_queries," << QuerySequenceRecord.KeyCount << "\n";
  out_file << "Queries_with_results," << total_queries_with_results << "\n";
  out_file << "Total_TP," << total_true_positives << "\n";
  out_file << "Total_FP," << total_false_positives << "\n";
  out_file << "Total_FN," << total_false_negatives << "\n";
  out_file << "Overall_Recall," << overall_recall << "\n";
  out_file << "Overall_Precision," << overall_precision << "\n";
  out_file << "Overall_F1_Score," << overall_f1 << "\n";
  out_file << "Average_Recall," << avg_recall << "\n";
  out_file << "Average_Precision," << avg_precision << "\n";
  out_file << "Average_F1_Score," << f1_score << "\n";

  out_file.close();



  //

  // For each query, compute the hash and check if it got the c-NN right.

  // For each query, compute the hash and find the nearest neighbour in the hash
  // table. Compute the distance to the nearest neighbour and compare it to the
  // expected distance based on the mutation parameters.

  // You generate sequences, and then as the mutation rate increases, can you
  // see how the recall drops?

  // std::vector<uint32_t> sequenceLengths;

  // if(hinfo->isSmallSequenceLength()){
  // 	printf("Hash %s is marked as very slow. Limiting test parameters for
  // practicality.\n", hinfo->name); 	sequenceLengths = {60}; //{20,30,40};
  // //{512};
  // }
  // else{
  // 	sequenceLengths = {512}; //{16, 24, 32, 48, 64, 80, 96, 128, 256, 512,
  // 1024, 2048, 4096, 8192};
  // }

  // seed_t baseSeed = g_GoldenRatio; // Base seed for reproducibility

  // seed_t flagsSeedOffset = 101; // Offset to change the seed based on flags
  // for(const auto & toklen : tokenlengths){

  // 	SetIsTestActive(true);

  // 	for(const auto & seqLen : sequenceLengths){
  // 		printf("=====================================\n");
  // 		printf("Sequence length: %u \tToken length: %d\n", seqLen,
  // toklen); 		printf("=====================================\n");
  // 		// For DNA sequences, keybits = 8 * sequence length
  // 		uint32_t keybits = seqLen * 8;	// Eg: Keybits = Sequence length
  // times 8 (8 bits per base)

  //

  // 		baseSeed += flagsSeedOffset;
  // 		printf("\nTesting hash: %s with keybits: %u (sequence length:
  // %u), token length: %d\n", hinfo->name, keybits, seqLen, toklen);
  // result &= LSHApproxNearestNeighbourTestInner<hashtype>(hinfo, baseSeed,
  // seqLen, flags, out_file);
  // 	}
  // }

  // // Cleanup: Reset LSH global variables after test completion
  // SetIsTestActive(false);

  // printf("%s\n", result ? "" : g_failstr);
  out_file.close();
  return result;
}

INSTANTIATE(LSHApproxNearestNeighbourTest, HASHTYPELIST);