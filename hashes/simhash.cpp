/* * SIMHASH
 * Copyright (C) 2022 IISc
 * ###YOURLICENSETEXT
 * */

#include "Platform.h"
#include "Hashlib.h"
#include "LSHGlobals.h"

#include "Mathmult.h"
#include <random>
#include <vector>
#include <unordered_map>
#include <iostream>

//mean = 0.0, stddev = 1.0
std::vector<double> GenerateNormalHyperplane(int n, seed_t seed) {
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



template <bool bswap>
static void simHash32(const void* in , const size_t len, const seed_t seed, void* out) {

      // Input validation
      if (in == nullptr || out == nullptr || len == 0) {
            if (out) PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }

      const char* data = static_cast<const char*>(in);
      
      // //print the data
      // printf("Inside Hash: ");
      // for(int i =0; i< (int)len; i++){
      //       std::cout << data[i] << " ";
      // }
      // printf("\n");

      auto hyperplane = GenerateNormalHyperplane(len, seed);

      double temp_hash = 0.0;
      for(int i = 0; i < static_cast<int>(len); i++) {
            if(data[i] == '0') continue; // Skip zero bytes to optimize. Note that the data is in character therefore there is a need for the quotes.
            else{
                  temp_hash += static_cast<double>(hyperplane[i]);
            }
      }
      uint32_t hash = (temp_hash >= 0) ? 1 : 0;

      PUT_U32<bswap>(hash, (uint8_t *)out, 0);
}


//------------------------------------------------------------
REGISTER_FAMILY(SimHash,
   $.src_url    = "###YOURREPOSITORYURL",
   $.src_status = HashFamilyInfo::SRC_UNKNOWN
 );

REGISTER_HASH(SimHash_Cos_32,
   $.desc            = "Simhash Cosine 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCALITY_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_COSINE_SIMILARITY | FLAG_HASH_UNIVERSE_VECTOR_OPTIMISATION,
   $.impl_flags      = FLAG_IMPL_MULTIPLY | FLAG_IMPL_VERY_SLOW,        // Simhash is computationally expensive
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = simHash32<false>,
   $.hashfn_bswap    = simHash32<true>
 );

REGISTER_HASH(SimHash_Ang_32,
   $.desc            = "Simhash Angular 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCALITY_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_ANGULAR_SIMILARITY | FLAG_HASH_UNIVERSE_VECTOR_OPTIMISATION,
   $.impl_flags      = FLAG_IMPL_MULTIPLY | FLAG_IMPL_VERY_SLOW,        // Simhash is computationally expensive
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = simHash32<false>,
   $.hashfn_bswap    = simHash32<true>
 );
//------------------------------------------------------------

