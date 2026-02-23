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
#include "LSHCollisionAndOrTest.h"
#include "fstream"
#include <iostream>
#include <filesystem>

#if defined(HAVE_THREADS)
  #include <atomic>
#endif

/*-------------------------------------------------------------------------------*/
/*									Collision Test		 						 */
/*-------------------------------------------------------------------------------*/

struct common_params_struct{
	uint32_t seqLen;
	seed_t DatagenSeed;
	seed_t DataMutateSeed;
	uint32_t tokenlength;
	bool isBasesDrawnFromUniformDist;
	uint32_t distanceClass;
};


template <typename hashtype>
static void LSHCollisionTestInnerInnerWorker(const HashInfo * hinfo, uint32_t N_seq, uint32_t N_hash, HashFn hash, seed_t HashSeed, common_params_struct &common_params, SequenceRecordsWithMetadataStruct &sequenceRecordsforTest,const std::vector<UnionBitVectorsStruct> *precomputedVectors, double * AverageCollision,int start, int end, uint32_t and_param, uint32_t or_param){

	const bool useUniverseOpt = hinfo->hasUniverseVectorOptimisation();

	std::vector<hashtype> hash_val_org = std::vector<hashtype>(and_param*or_param);
	std::vector<hashtype> hash_val_mut = std::vector<hashtype>(and_param*or_param);

	std::vector<seed_t> andor_seeds = std::vector<seed_t>(and_param*or_param, 0);

	for(int rec_idx = start; rec_idx < end; rec_idx++){
		SequenceRecordUnit &record = sequenceRecordsforTest.Records[rec_idx];
		uint32_t collision_count = 0;
		
		// --- OPTIMIZATION START ---
        // We prepare the data ONCE per sequence, not N_hash times.
        // This prevents millions of memory allocations and lock contentions.
        if (useUniverseOpt && precomputedVectors != nullptr) {
            // Allocate vectors once
            const UnionBitVectorsStruct &unionBitVectors = (*precomputedVectors)[rec_idx];
            
            // Constant pointers to data for speed
            const void* vecA_ptr = unionBitVectors.vec_a.data();
            const size_t vecA_len = unionBitVectors.vec_a.size();
            const void* vecB_ptr = unionBitVectors.vec_b.data();
            const size_t vecB_len = unionBitVectors.vec_b.size();

            for(uint32_t hash_idx = 0; hash_idx < N_hash; hash_idx++){
                
				std::vector<uint32_t> and_flags = std::vector<uint32_t>(or_param, 0); 
				for(uint32_t or_i = 0; or_i < or_param; or_i++){
					and_flags[or_i] = 1;
					for(uint32_t and_i = 0; and_i < and_param; and_i++){
						andor_seeds[or_i * and_param + and_i] = HashSeed + hash_idx + (or_i * and_param + and_i)*1337;
						
						hash(vecA_ptr, vecA_len, andor_seeds[or_i * and_param + and_i], &hash_val_org[or_i * and_param + and_i]);
						hash(vecB_ptr, vecB_len, andor_seeds[or_i * and_param + and_i], &hash_val_mut[or_i * and_param + and_i]);
					}
					// ANDing
					for(uint32_t i = 0; i < and_param; i++){
						if(hash_val_org[or_i * and_param + i] != hash_val_mut[or_i * and_param + i]){
							and_flags[or_i] = 0;
							break;
						}
					}
				}

				//AND-ORING
				bool flag = 0;
				for(uint32_t i = 0; i < or_param; i++){
					if(and_flags[i] == 1){
						flag = 1;
						break;
					}
				}
				collision_count += flag;
            }
        }
		else{
			// Standard path (no allocation, just pointer access)
            const uint8_t* ptrOrg = (const uint8_t*)record.SeqASCIIOrg.c_str();
            const size_t lenOrg = record.OriginalLength;
            const uint8_t* ptrMut = (const uint8_t*)record.SeqASCIIMut.c_str();
            const size_t lenMut = record.MutatedLength;

			for(uint32_t hash_idx = 0; hash_idx < N_hash; hash_idx++){
				std::vector<uint32_t> and_flags = std::vector<uint32_t>(or_param, 0); 
				for(uint32_t or_i = 0; or_i < or_param; or_i++){
					and_flags[or_i] = 1;
					for(uint32_t and_i = 0; and_i < and_param; and_i++){
						andor_seeds[or_i * and_param + and_i] = HashSeed + hash_idx + (or_i * and_param + and_i)*1337;
						
						hash(ptrOrg, lenOrg, andor_seeds[or_i * and_param + and_i], &hash_val_org[or_i * and_param + and_i]);
						hash(ptrMut, lenMut, andor_seeds[or_i * and_param + and_i], &hash_val_mut[or_i * and_param + and_i]);
					}
					// ANDing
					for(uint32_t i = 0; i < and_param; i++){
						if(hash_val_org[or_i * and_param + i] != hash_val_mut[or_i * and_param + i]){
							and_flags[or_i] = 0;
							break;
						}
					}
				}

				//AND-ORING
				bool flag = 0;
				for(uint32_t i = 0; i < or_param; i++){
					if(and_flags[i] == 1){
						flag = 1;
						break;
					}
				}
				collision_count += flag;
			}
		}
        // --- OPTIMIZATION END ---

		// Compute average and stddev of collisions for this sequence pair.
		double avg_collision = static_cast<double>(collision_count) / static_cast<double>(N_hash);
		AverageCollision[rec_idx] = avg_collision;
	}
}


template <typename hashtype>
static bool LSHCollisionTestInnerInnerParallel(const HashInfo * hinfo, uint32_t N_seq, uint32_t N_hash, HashFn hash, seed_t HashSeed, common_params_struct &common_params, sim_bins_struct &sim_bins, std::ofstream &out_file){
	
	printf("Inside LSHCollisionTestANDORInnerInner\n");

	seed_t seed_offset = 42;	// Coz its the answer to everything!
	SequenceRecordsWithMetadataStruct sequenceRecordsforTest;
	sequenceRecordsforTest.OriginalSequenceLength = common_params.seqLen;
	sequenceRecordsforTest.DistanceClass = common_params.distanceClass;
	sequenceRecordsforTest.isBasesDrawnFromUniformDist = common_params.isBasesDrawnFromUniformDist;
	sequenceRecordsforTest.DatagenSeed = common_params.DatagenSeed + seed_offset;
	sequenceRecordsforTest.DataMutateSeed = common_params.DataMutateSeed + seed_offset;
	// sequenceRecordsforTest.tokenlength = common_params.tokenlength;
	sequenceRecordsforTest.KeyCount = N_seq;
	sequenceRecordsforTest.HashCount = N_hash;
	
	SequenceDataGenerator dataGenTest(&sequenceRecordsforTest);

	// For each of the sequence pair, draw a random mutation parameter from the bin statistics. Then store it in the mutation record. and mutate it.
	seed_t bin_sampling_seed = common_params.DataMutateSeed + 3*seed_offset;
	seed_t bin_params_sampling_seed = common_params.DataMutateSeed + 7*seed_offset;
	
	Rand rng_bin_sampler(bin_sampling_seed);
	Rand rng_bin_params_sampler(bin_params_sampling_seed);

	for(uint32_t idx = 0; idx < N_seq; idx++){
		uint32_t bin_fill_count = 0;
		int sampled_binid = -1;
		uint32_t attempts = 0;
		const uint32_t max_attempts = 1000;

		while(bin_fill_count == 0 && attempts < max_attempts){
			sampled_binid = rng_bin_sampler.rand_range(100);
			bin_fill_count = sim_bins.bin_fill_count[sampled_binid];
			attempts++;
		}
		
		if(bin_fill_count == 0){
			printf("Warning: Could not find non-empty bin after %u attempts\n", max_attempts);
			// sequenceRecordsforTest.Records[idx].snpRate = 1.0; // Assign a default value
			continue;  // Skip this sequence or handle error
		}
		
		uint32_t rand_param_idx = rng_bin_params_sampler.rand_range(bin_fill_count);
		double sampled_error_param = sim_bins.bin_error_parameters[sampled_binid][rand_param_idx];
		sequenceRecordsforTest.Records[idx].foundationalParameter = sampled_error_param;
	}

	if(g_mutation_model == MUTATION_MODEL_SIMPLE_SNP_ONLY){
		SequenceDataMutatorSubstitutionOnly dataMutTest(&sequenceRecordsforTest);
		printf("Completed mutation using simple SNP only model.\n");
	}
	else if(g_mutation_model == MUTATION_MODEL_GEOMETRIC_MUTATOR){
		SequenceDataMutatorGeometric dataMutTest(&sequenceRecordsforTest);
		printf("Completed mutation using geometric mutator model.\n");
	}
	
	// Print Similarity values
	out_file << ":5:";
	for (size_t i = 0; i < N_seq; i++) {
		if (i == N_seq - 1)
			out_file << sequenceRecordsforTest.Records[i].similarity << "\n";
		else
			out_file << sequenceRecordsforTest.Records[i].similarity << ",";
	}
	// Print param values
	out_file << ":6:";
	for (size_t i = 0; i < N_seq; i++) {
		if (i == N_seq - 1)
			out_file << sequenceRecordsforTest.Records[i].snpRate << "\n";
		else
			out_file << sequenceRecordsforTest.Records[i].snpRate << ",";
	}
	
	if(g_mutation_model == MUTATION_MODEL_GEOMETRIC_MUTATOR){	
		out_file << ":7:";
		for (size_t i = 0; i < N_seq; i++) {
			if (i == N_seq - 1)
				out_file << sequenceRecordsforTest.Records[i].delRate << "\n";
			else
				out_file << sequenceRecordsforTest.Records[i].delRate << ",";
		}
		out_file << ":8:";
		for (size_t i = 0; i < N_seq; i++) {
			if (i == N_seq - 1)
				out_file << sequenceRecordsforTest.Records[i].insmean << "\n";
			else
				out_file << sequenceRecordsforTest.Records[i].insmean << ",";
		}
		out_file << ":9:";
		for (size_t i = 0; i < N_seq; i++) {
			if (i == N_seq - 1)
				out_file << sequenceRecordsforTest.Records[i].stayRate << "\n";
			else
				out_file << sequenceRecordsforTest.Records[i].stayRate << ",";
		}
		out_file << ":14:";
		for (size_t i = 0; i < N_seq; i++) {
			if (i == N_seq - 1)
				out_file << sequenceRecordsforTest.Records[i].insRate << "\n";
			else
				out_file << sequenceRecordsforTest.Records[i].insRate << ",";
		}
	}

	//for each sequence pair, compute N_hash hashes and store them as, mean and stddev.
	const bool useUniverseOpt = hinfo->hasUniverseVectorOptimisation();
    const uint32_t tokenLen = common_params.tokenlength;

	// === PRE-COMPUTE ALL UNION VECTORS BEFORE PARALLELIZATION ===
    // This eliminates heap contention during the parallel phase
	std::vector<UnionBitVectorsStruct> precomputedVectors;
	if (useUniverseOpt) {
        printf("Pre-computing union vectors for %u sequences...\n", N_seq);
        auto precompute_start = std::chrono::high_resolution_clock::now();
        
        precomputedVectors.resize(N_seq);
        for (uint32_t i = 0; i < N_seq; i++) {
            precomputedVectors[i] = CreateUnionBitVectors(
                sequenceRecordsforTest.Records[i].SeqASCIIOrg,
                sequenceRecordsforTest.Records[i].SeqASCIIMut,
                tokenLen
            );
        }
        
        auto precompute_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = precompute_end - precompute_start;
        printf("Pre-computation took %.3f seconds.\n", elapsed.count());
    }
	// === END PRE-COMPUTE ALL UNION VECTORS BEFORE PARALLELIZATION ===

	alignas(64) std::vector<double> AverageCollision(N_seq, 0.0);
	
	std::vector<std::pair<uint32_t, uint32_t>> ANDOR_params = {{2,3}};//{{1,1},{2,1},{3,2},{1,2},{2,3},{2,2}}; //{{5,5}, {11,11}, {21,21}};
	for(auto andor_param : ANDOR_params){
		uint32_t and_param = andor_param.first;
		uint32_t or_param = andor_param.second;

		// std::vector<seed_t> andor_seeds = std::vector<seed_t>(and_param*or_param, 0);

		const std::vector<UnionBitVectorsStruct> *vecPtr = useUniverseOpt ? &precomputedVectors : nullptr;
		
		if (g_NCPU == 1) {
			printf("Starting collision computation with %u thread...\n", g_NCPU);
			
			auto start_seq = std::chrono::high_resolution_clock::now();
			// Single threaded fallback
			LSHCollisionTestInnerInnerWorker<hashtype>(
				hinfo, N_seq, N_hash, hash, HashSeed, 
				common_params, sequenceRecordsforTest,
				vecPtr,
				AverageCollision.data(), 
				0, N_seq, 
				and_param, or_param
			);
			auto end_seq = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed_seq = end_seq - start_seq;
			printf("Sequential collision computation took %.3f seconds.\n", elapsed_seq.count());
		}
		else{
			#if defined(HAVE_THREADS)
				printf("Starting collision computation with %u threads...\n", g_NCPU);
				std::vector<std::thread> threads(g_NCPU);
				int block_size = N_seq / g_NCPU;
				printf("Block size per thread: %d\n", block_size);

				auto start_par = std::chrono::high_resolution_clock::now();

				for(uint32_t i = 0; i < g_NCPU; i++){
					int start = i * block_size;
					int end = (i == g_NCPU - 1) ? N_seq : (i + 1) * block_size;

					threads[i] = std::thread(
						LSHCollisionTestInnerInnerWorker<hashtype>, 
						hinfo, N_seq, N_hash, hash, HashSeed, 
						std::ref(common_params), 
						std::ref(sequenceRecordsforTest), 
						vecPtr,
						AverageCollision.data(), 
						start, end,
						and_param, or_param
					);
				}

				for (auto& t : threads) {
					t.join();
				}
				
				auto end_par = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed_par = end_par - start_par;
				printf("Parallel collision computation took %.3f seconds.\n", elapsed_par.count());
			#endif
		}
		out_file << ":10:" << "AND," << "OR" << std::endl;
		out_file << ":11:" << and_param << "," << or_param << std::endl;
		// Print Average Collision values
		out_file << ":12:"; // Starting  line with collision curves
		for (size_t i = 0; i < N_seq; i++) {
			if (i == N_seq - 1)
				out_file << AverageCollision[i] << "\n";
			else
				out_file << AverageCollision[i] << ",";
		}
	}
	
	return true;
}

static sim_bins_struct LSHCollisionTestInnerAgg(uint32_t N_agg, common_params_struct &common_params, std::ofstream &out_file){
	
	printf("Inside get_params_aggregated_in_bins\n");

	SequenceRecordsWithMetadataStruct sequenceRecordsForAgg;

	sequenceRecordsForAgg.OriginalSequenceLength = common_params.seqLen;
	sequenceRecordsForAgg.DistanceClass = common_params.distanceClass;
	sequenceRecordsForAgg.isBasesDrawnFromUniformDist = common_params.isBasesDrawnFromUniformDist;
	sequenceRecordsForAgg.DatagenSeed = common_params.DatagenSeed;
	sequenceRecordsForAgg.DataMutateSeed = common_params.DataMutateSeed;
	// sequenceRecordsForAgg.tokenlength = common_params.tokenlength;
	sequenceRecordsForAgg.KeyCount = N_agg;

	SequenceDataGenerator dataGenAgg(&sequenceRecordsForAgg);

	sim_bins_struct sim_bins_agg;	//Bin to store the error parameters 

	Rand rng(sequenceRecordsForAgg.DataMutateSeed);

    std::vector<double> rand_error_param(N_agg, 0.0);
	std::vector<double> similarity_values(N_agg, 0.0);
	
	if(g_mutation_model == MUTATION_MODEL_SIMPLE_SNP_ONLY){
		SequenceDataMutatorSubstitutionOnly dataMutAgg(&sequenceRecordsForAgg, &rand_error_param);
	}
	else if(g_mutation_model == MUTATION_MODEL_GEOMETRIC_MUTATOR){
		printf("Using geometric mutator model for data mutation in aggregation phase.\n");
		SequenceDataMutatorGeometric dataMutAgg(&sequenceRecordsForAgg, &rand_error_param);
	}
	
	// Print Similarity values
	out_file << ":13:";
	for (size_t i = 0; i < N_agg; i++) {
		if (i == N_agg - 1)
			out_file << rand_error_param[i] << "\n";
		else
			out_file << rand_error_param[i] << ",";
	}
	
	// Extract similarity values
	for(uint32_t idx = 0; idx < N_agg; idx++){
		similarity_values[idx] = sequenceRecordsForAgg.Records[idx].similarity;
	}

	// Perform binning on similarity values
	for (size_t i = 0; i < N_agg; i++) {
		float sim_value = similarity_values[i];
		uint32_t bin_index = static_cast<uint32_t>((sim_value - sequenceRecordsForAgg.binstart) / sequenceRecordsForAgg.binsize);
		if (bin_index >= sequenceRecordsForAgg.bincount) {
			bin_index = sequenceRecordsForAgg.bincount - 1; // Clamp to last bin
		}
		
		uint32_t bin_fill_count = sim_bins_agg.bin_fill_count[bin_index];

		if(bin_fill_count == g_bincount_full){
			continue;	// To avoid excessive memory usage, we limit to x entries per bin.
		}
		
		sim_bins_agg.bin_error_parameters[bin_index][bin_fill_count] = rand_error_param[i];
		sim_bins_agg.bin_fill_count[bin_index]++;
	}

	//Compute mean and stddev for each bin
	for (size_t bin_idx = 0; bin_idx < sequenceRecordsForAgg.bincount; bin_idx++) {
		const auto & params = sim_bins_agg.bin_error_parameters[bin_idx];
		if (!params.empty()) {
			// Compute mean
			double sum = 0.0;
			for(uint32_t param_idx = 0; param_idx < sim_bins_agg.bin_fill_count[bin_idx]; param_idx++){
				sum += params[param_idx];
			}
			// printf("Bin %zu: Sum = %f, Count = %u\n", bin_idx, sum, sim_bins_agg.bin_fill_count[bin_idx]);
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
	// for (size_t bin_idx = 0; bin_idx < sequenceRecordsForAgg.bincount; bin_idx++) {
	// 	printf("Bin %zu: Fill Count = %u", bin_idx, sim_bins_agg.bin_fill_count[bin_idx]);
	// 	// Print mean and stddev
	// 	printf(", Mean = %0.2f, Stddev = %0.2f\n", sim_bins_agg.bin_error_parameters_mean[bin_idx], sim_bins_agg.bin_error_parameters_stddev[bin_idx]);
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
	// Now, I want to go through each bin, and if its fill count is less than g_bincount_full,
	// I want to fill the remaining entries with random samples from a normal distribution
	// defined by the mean and stddev of that bin.
	
	std::mt19937 gen(common_params.DataMutateSeed + 1337);  // Seeded RNG for reproducibility
	
	for (size_t bin_idx = 0; bin_idx < sequenceRecordsForAgg.bincount; bin_idx++) {
		uint32_t current_fill = sim_bins_agg.bin_fill_count[bin_idx];
		
		if (current_fill < g_bincount_full && current_fill > 0) {
			// Get the mean and stddev for this bin
			double mean = sim_bins_agg.bin_error_parameters_mean[bin_idx];
			double stddev = sim_bins_agg.bin_error_parameters_stddev[bin_idx];
			
			// If stddev is 0 or very small, use a small default to avoid degenerate distribution
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

			//Todo: Add the part where in angular similarity, we geniuinely have empty bins from 0 to 49


			// // Bin is completely empty - use bin center as mean with small stddev
			// double bin_center = sequenceRecordsForAgg.binstart + (bin_idx + 0.5) * sequenceRecordsForAgg.binsize;
			// double default_stddev = sequenceRecordsForAgg.binsize / 4.0;  // Quarter of bin width
			
			// std::normal_distribution<double> normal_dist(bin_center, default_stddev);
			
			// for (uint32_t fill_idx = 0; fill_idx < g_bincount_full; fill_idx++) {
			// 	double sampled_value = normal_dist(gen);
			// 	sampled_value = std::max(0.0, std::min(1.0, sampled_value));
				
			// 	sim_bins_agg.bin_error_parameters[bin_idx][fill_idx] = sampled_value;
			// 	sim_bins_agg.bin_fill_count[bin_idx]++;
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
	sequenceRecordsForAgg.Records.clear();
	sequenceRecordsForAgg.Records.shrink_to_fit();  // Actually releases the memory
	
	return sim_bins_agg;
}

template <typename hashtype>
static bool LSHCollisionTestInner( const HashInfo * hinfo, const seed_t baseSeed, const uint32_t seqLen, flags_t flags, std::ofstream &out_file) {

	bool result = true;	//TODO: Update this based on test results.

	const uint32_t tokenlength = GetTokenLength();  		// Get runtime token length

	HashFn hash = hinfo->hashFn(g_hashEndian);

	const seed_t hash_seed = hinfo->Seed(baseSeed);

	bool isBasesDrawnFromUniformDist = true;

	// File header
	out_file << ":1:LSH Collision Test Results\n";
	out_file << ":2:" << "Hashname," << "SequenceLength," << "TokenLength,"<< "Distance Metric," << "Mutation Model,"<< "Mutation Expression" << std::endl;
	out_file << ":3:" << hinfo->name << "," << seqLen << "," << tokenlength << "," << setDistanceClassForHashInfo(hinfo->hash_flags) << "," << g_mutation_model << "," << g_mutation_expression_type << std::endl;
	// if(std::string(hinfo->name) == "SubseqHash-64"){
	// 	out_file << ":4:" << g_subseqHash1_subseq_len << "," << g_subseqHash1_d << std::endl;
	// }

	seed_t DatagenSeed = baseSeed + g_GoldenRatio + 17;		// Seed for data generation
	seed_t DataMutateSeed = baseSeed + 29;	// Seed for data mutation
	seed_t HashSeed = hash_seed;			// Seed for hash family generation

	if (!REPORT(VERBOSE, flags)) {
		// printf("LSH Collision Test: Key Size = %3u bits (%2u bytes), Keys = %8u, Hashes per Key = %4zu\n",
        //    keybits, (unsigned)keybytes, keycount, hashcount);

		printf("Hash Function: %s\n", hinfo->name);
		printf("Hash Bits: %u\n", hinfo->bits);
    }

	//--------------------------------------------//
	common_params_struct common_params;
	common_params.seqLen = seqLen;
	common_params.DatagenSeed = DatagenSeed;
	common_params.DataMutateSeed = DataMutateSeed;
	common_params.tokenlength = tokenlength;
	common_params.isBasesDrawnFromUniformDist = isBasesDrawnFromUniformDist;
	common_params.distanceClass = setDistanceClassForHashInfo(hinfo->hash_flags);
	//--------------------------------------------//

	uint32_t N_agg = 0;
	sim_bins_struct sim_bins;

	uint32_t N_seq = 0;		// Number of sequences to generate for testing
	uint32_t N_hash = 0;	// Number of hashes to compute per sequence


	//--------------------------------------------//
	if(hinfo->isVerySlow()){
		printf("Hash %s is marked as very slow. Limiting test parameters for practicality.\n", hinfo->name);
		N_agg = 500000;	// Number of sequences to generate for testing
		// N_agg = 10000;	// Number of sequences to generate for testing
		sim_bins = LSHCollisionTestInnerAgg(N_agg, common_params, out_file);
		
		//print bin means and stddevs using	
		for (size_t bin_idx = 0; bin_idx < sim_bins.bin_error_parameters_mean.size(); bin_idx++) {
			printf("Bin %zu: Count %d, Mean = %0.2f, Stddev = %0.2f\n", bin_idx, sim_bins.bin_fill_count[bin_idx], sim_bins.bin_error_parameters_mean[bin_idx], sim_bins.bin_error_parameters_stddev[bin_idx]);
		}
		
		//--------------------------------------------//
		// N_seq = 1000;		// Number of sequences to generate for testing
		// N_hash = 1000;	// Number of hashes to compute per sequence
		N_seq = 5000;		// Number of sequences to generate for testing
		N_hash = 2000;	// Number of hashes to compute per sequence
		
		// N_seq = 5000;		// Number of sequences to generate for testing
		// N_hash = 500;	// Number of hashes to compute per sequence
	}
	else{
		N_agg = 500000;	// Number of sequences to generate for testing
		sim_bins = LSHCollisionTestInnerAgg(N_agg, common_params, out_file);
		
		//print bin means and stddevs using	
		for (size_t bin_idx = 0; bin_idx < sim_bins.bin_error_parameters_mean.size(); bin_idx++) {
			printf("Bin %zu: Count %d, Mean = %0.2f, Stddev = %0.2f\n", bin_idx, sim_bins.bin_fill_count[bin_idx], sim_bins.bin_error_parameters_mean[bin_idx], sim_bins.bin_error_parameters_stddev[bin_idx]);
		}
		
		//--------------------------------------------//
		N_seq = 5000;		// Number of sequences to generate for testing
		N_hash = 1000;	// Number of hashes to compute per sequence
		// N_seq = 1000;		// Number of sequences to generate for testing
		// N_hash = 100;	// Number of hashes to compute per sequence
	}
	
	// LSHCollisionTestInnerInner<hashtype>(hinfo, N_seq, N_hash, hash, HashSeed, common_params, sim_bins, out_file);
	LSHCollisionTestInnerInnerParallel<hashtype>(hinfo, N_seq, N_hash, hash, HashSeed, common_params, sim_bins, out_file);

	//--------------------------------------------//

    return result;	//TODO: For now, the result is always true. We need to add logic to find where the test fails.
}


//----------------------------------------------------------------------------//
template <typename hashtype>
bool LSHCollisionAndOrTest( const HashInfo * hinfo, bool extra, flags_t flags) {
	
	printf("[[[ LSH Collision Tests ]]]\n\n");
	if(extra){
		printf("Extra flag is set. Running extended tests where applicable.\n");
	}

	bool result = true;

	// Create a output file for storing the results.
	std::string filename = "../results/collisionResults_" + std::string(hinfo->name) + "ANDOR.csv";

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




	// Create a code for generating an output file name based on hash name.
	
	/*
		If the hash has tokenisation property, then we need to test for multiple token lengths
		Otherwise, we just use a token length of 0 (no tokenisation)
	*/
	std::vector<uint32_t> tokenlengths;
	if(hinfo->hasTokenisationProperty()){
		printf("Hash %s has tokenisation property. Testing multiple token lengths.\n", hinfo->name);
		tokenlengths = {13};//{4 ,7, 13, 21, 31, 33}; //create_tokens();
	}
	else{
		tokenlengths = {0}; // No tokenization
	}
	

	std::vector<uint32_t> sequenceLengths;

	if(hinfo->isSmallSequenceLength()){
		printf("Hash %s is marked as very slow. Limiting test parameters for practicality.\n", hinfo->name);
		sequenceLengths = {60}; //{20,30,40}; //{512};
	}
	else{
		sequenceLengths = {512}; //{16, 24, 32, 48, 64, 80, 96, 128, 256, 512, 1024, 2048, 4096, 8192};
	}

	seed_t baseSeed = g_GoldenRatio; // Base seed for reproducibility
	
	seed_t flagsSeedOffset = 101; // Offset to change the seed based on flags
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

				g_TokenLength = toklen;	// Set global token length for testing
			}

			baseSeed += flagsSeedOffset;
			printf("\nTesting hash: %s with keybits: %u (sequence length: %u), token length: %d\n", hinfo->name, keybits, seqLen, toklen);
			result &= LSHCollisionTestInner<hashtype>(hinfo, baseSeed, seqLen, flags, out_file);
		}
	}
		
    // Cleanup: Reset LSH global variables after test completion
    SetIsTestActive(false);

    printf("%s\n", result ? "" : g_failstr);
	out_file.close();
    return result;
}

INSTANTIATE(LSHCollisionAndOrTest, HASHTYPELIST);