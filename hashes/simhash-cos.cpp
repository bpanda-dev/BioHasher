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


//mean = 0.0, stddev = 1.0
static std::vector<double> GenerateNormalHyperplane(int n, seed_t seed) {
	// 1. Initialize the engine with a specific seed for reproducibility
	std::mt19937 gen(seed); 
	
	// 2. Define the distribution (Normal/Gaussian)
	double mean = 0.0;
	double std_dev = 1.0;
	std::normal_distribution<double> dist(mean, std_dev);
	
	// 3. Populate the array
	std::vector<double> data;
	data.reserve(n); // Pre-allocate memory for efficiency
	
	for (int i = 0; i < n; ++i) {
		data.push_back(dist(gen));
	}

	return data;
}


static void simHash32(const void* in , const size_t len, const seed_t seed, void* out) {

	// Input validation
	if (in == nullptr || out == nullptr || len == 0) {
		if (out) PUT_U32(0, (uint8_t *)out, 0);
		return;
	}

	const char* data = static_cast<const char*>(in);
	
	//print the data
	printf("Inside Hash: ");
	for(int i =0; i< (int)len; i++){
	      std::cout << data[i] << "seed: " << seed << std::endl;
	}
	printf("\n");

	auto hyperplane = GenerateNormalHyperplane(len, seed);

	double temp_hash = 0.0;
	for(int i = 0; i < static_cast<int>(len); i++) {
	if(data[i] == '0') continue; // Skip zero bytes to optimize. Note that the data is in character therefore there is a need for the quotes.
	else{
			temp_hash += static_cast<double>(hyperplane[i]);
	}
	}
	uint32_t hash = (temp_hash >= 0) ? 1 : 0;

	PUT_U32(hash, (uint8_t *)out, 0);
}


//------------------------------------------------------------
REGISTER_FAMILY(SimHash,
   $.src_url    = "###YOURREPOSITORYURL",
   $.src_status = HashFamilyInfo::SRC_UNKNOWN
 );

REGISTER_HASH(SimHash_Cos_32,
   $.desc            = "Simhash Cosine 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCALITY_SENSITIVE,
   $.impl_flags      = FLAG_IMPL_MULTIPLY | FLAG_IMPL_VERY_SLOW,        // Simhash is computationally expensive
   $.bits            = 32,
   $.hashfn			 = simHash32,
   $.parameterNames  = {"k"},
   $.parameterDescriptions = {"Kmer Length"},
   $.parameterValues = {TOKEN_LENGTH},
   $.similarity_name = "Cosine",
   $.similarityfn    = CosineSimilarity,
   $.check_equality_fn = check_equality<uint32_t>
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

