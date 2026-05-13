// Code is currently under construction. Changes have been made which needs addition of new seekable random number generator.
/* * SIMHASH
 * Copyright (C) 2026 IISc
 * ###YOURLICENSETEXT
 * */

#include "specifics.h"
#include "Hashlib.h"
#include "LSHGlobals.h"
#include "smhasher3Random.h"

#include "Mathmult.h"
#include <random>
#include <vector>
#include <unordered_map>
#include <iostream>


#define TOKEN_LENGTH 32


// NOTE: Only valid for 32 and 64 bit hash output.  
template<typename hashT>
bool check_equality(void* inp1, void* inp2){
      if(inp1 == nullptr || inp2 == nullptr){
            return false;
      }
      return (*static_cast<hashT*>(inp1) == *static_cast<hashT*>(inp2));
}

static std::set<std::string> get_kmers(const std::string& sequence, int k) {
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

static double JaccardSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t in1_len, const uint32_t in2_len) {
	
	uint32_t k = (uint32_t)TOKEN_LENGTH; // k-mer length

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


static double CosineSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t in1_len, const uint32_t in2_len) {
	
	uint32_t k = (uint32_t)TOKEN_LENGTH; // k-mer length

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

static double AngularSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t in1_len, const uint32_t in2_len) {
	double cosine_sim = CosineSimilarity(seq1, seq2, in1_len, in2_len);
	// Angular similarity = 1 - (angle / π) where angle = arccos(cosine_similarity)
	return 1 - (std::acos(cosine_sim) / M_PI);
}


// //mean = 0.0, stddev = 1.0
// static std::vector<double> GenerateNormalHyperplane(int n, seed_t seed) {
// 	// 1. Initialize the engine with a specific seed for reproducibility
// 	std::mt19937 gen(seed); 
	
// 	// 2. Define the distribution (Normal/Gaussian)
// 	double mean = 0.0;
// 	double std_dev = 1.0;
// 	std::normal_distribution<double> dist(mean, std_dev);
	
// 	// 3. Populate the array
// 	std::vector<double> data;
// 	data.reserve(n); // Pre-allocate memory for efficiency
	
// 	for (int i = 0; i < n; ++i) {
// 		data.push_back(dist(gen));
// 	}

// 	return data;
// }

// Convert a uint64_t to a uniform double in (0, 1).
// Uses the top 53 bits (full double mantissa precision).
// The +0.5 shift ensures the result is strictly > 0, which is
// required for the log() in Box-Muller.
static inline double u64_to_uniform01(uint64_t x) {
    return ((x >> 11) + 0.5) * (1.0 / (UINT64_C(1) << 53));
}


static void simHash32(const void* in , const size_t len, const seed_t seed, void* out) {

	// Input validation
	if (in == nullptr || out == nullptr || len == 0) {
		if (out) PUT_U32(0, (uint8_t *)out, 0);
		return;
	}

	const char* data = static_cast<const char*>(in);
	
	// Get kmers
	std::set<std::string> kmer_set = get_kmers(data, TOKEN_LENGTH);

	// Hash kmers using scattering hash function (e.g., MurmurHash3) to get a vector of hash values for the kmers.
	//We will use the inbuilt FNV1a hash function
	const HashInfo * Fnv1aInfo = findHash("FNV-1a-32");  //Accessing the 32-bit FNV1a hash function that is available in biolshasher.
	if (!Fnv1aInfo) {
		// Fallback to another hash or throw error. For now we will throw an error.
		fprintf(stderr, "FNV1a hash function not found!\n");
		out = nullptr;
		return;
	}
	seed_t SeedFNV = g_GoldenRatio;      //A constant seed for FNV1a hash function which will be used by all the hashes in this hash family.
	std::vector<uint32_t> kmer_hashes;
	for(const auto& kmer : kmer_set){
		uint32_t TokenHash = 0;
		Fnv1aInfo->hashfn(kmer.data(), TOKEN_LENGTH, SeedFNV, &TokenHash);
		TokenHash = TokenHash & 0xFFFFFFFF; // Ensure it's 32-bit
		kmer_hashes.push_back(TokenHash);
	}

	// For each kmer, generate a random number from a normal distribution and accumulate it for final hash.
	Rand rng(seed); // Using the seekable RNG for reproducibility
	
	double total_sum = 0;
	for(const auto& kmer_hash : kmer_hashes){
		
		rng.seek(kmer_hash);

		double u1 = u64_to_uniform01(rng.rand_u64());
    	double u2 = u64_to_uniform01(rng.rand_u64());

		double radius = sqrt(-2.0 * log(u1));
    	double angle  = 2.0 * M_PI * u2;

		total_sum += radius * cos(angle); // This is the random number from the normal distribution for this kmer
	}
	
	// if greater than 0, set bit to 1, else set to 0. This will give us the final SimHash value.
	uint32_t hash = (total_sum >= 0.0) ? 1 : 0;

	PUT_U32(hash, (uint8_t *)out, 0);
}


//------------------------------------------------------------
REGISTER_FAMILY(SimHash,
   $.src_url    = "###YOURREPOSITORYURL",
   $.src_status = HashFamilyInfo::SRC_UNKNOWN
 );

REGISTER_HASH(SimHash_Ang_32,
   $.desc            = "Simhash Angular 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCALITY_SENSITIVE,
   $.impl_flags      = FLAG_IMPL_MULTIPLY | FLAG_IMPL_VERY_SLOW,        // Simhash is computationally expensive
   $.bits            = 32,
   $.hashfn   = simHash32,
   $.parameterNames  = {"k"},
   $.parameterDescriptions = {"Kmer Length"},
   $.parameterValues = {TOKEN_LENGTH},
   $.similarity_name = "Angular",
   $.similarityfn    = AngularSimilarity,
   $.check_equality_fn = check_equality<uint32_t>
 );
//------------------------------------------------------------

