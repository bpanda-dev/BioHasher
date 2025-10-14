/*
 * SIMHASH
 * Copyright (C) 2022 IISc
 *
 * ###YOURLICENSETEXT
 */
#include "Platform.h"
#include "Hashlib.h"
#include "LSHGlobals.h"

#include "Mathmult.h"

#include <vector>
#include <unordered_map>

//------------------------------------------------------------
template <typename hashT, bool bswap>
static hashT compute(const std::vector<hashT>& feature_hashes){
      std::vector<int> bit_count_vector((sizeof(hashT) * 8), 0);    // each position counts number of 1's - number of 0's. sizeof(hashT) * 8 =>total bits

      //Compute frequency of each hash and store it in frequency map
      std::unordered_map<hashT, int> frequency_map;
      for (const auto& h : feature_hashes) {
          frequency_map[h]++;
      }

      // Count the weighted number of 1's and 0's in each position.
	for (const auto& hash : feature_hashes) {
		hashT h = hash;
		int weight = frequency_map[h];	// gives the weight for that hash
            // int weight = 1;
		for (size_t i = 0; i < (sizeof(hashT) * 8); ++i) {
			bit_count_vector[i] += (h & 1) ? weight : ((-1)*(weight));
			h >>= 1;   
		}
	}
      
      // Determine the final bits of the SimHash
      hashT simhash = 0;
      for (size_t i = 0; i < (sizeof(hashT) * 8); ++i) {
          if (bit_count_vector[i] > 0) {
              simhash |= (hashT(1) << i);
          }
      }

      return simhash;
}


//------------------------------------------------------------
void extract_bits(const uint8_t *src,
                                  size_t start_bit,
                                  size_t bit_len, uint8_t *dst)
{
    // Allocate destination buffer
	memset(dst, 0, (bit_len + 7) / 8);

	for (size_t i = 0; i < bit_len; ++i) {
		size_t src_bit_index  = start_bit + i;
		size_t src_byte_index = src_bit_index / 8;
		size_t src_bit_offset = (src_bit_index % 8);  // // LSB-first (0 = rightmost bit)

		uint8_t bit = (src[src_byte_index] >> src_bit_offset) & 1;

		size_t dst_byte_index = i / 8;
		size_t dst_bit_offset = (i % 8); // LSB-first in destination too

		dst[dst_byte_index] |= (bit << dst_bit_offset);
	}
}

//current limit: in should have nucleotides encoded in 2-bit format and multiple of 4 bases (1 byte). No padding allowed for now.
template <bool bswap>
static void simhash32_bitwise( const void * in, const size_t len, const seed_t seed, void * out ) {
      
      // Input validation
      if (in == nullptr || out == nullptr || len == 0) {
            if (out) PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }

      uint32_t hash = 0;
      const uint8_t *data = (const uint8_t *)in;

    //   uint32_t bases_per_byte = 4; // 2 bits per base
    //   uint32_t total_bases = len * bases_per_byte; // Total number of bases
      
    //   // Use runtime k-mer length if LSH test is active, otherwise use default
    //   uint32_t bases_in_token = is_lsh_test_active() ? get_lsh_token_length() : 11;
    //   uint32_t bits_in_token = bases_in_token * 2; // in bits
    //   uint32_t bytes_in_token = (bits_in_token + 7) / 8; // in bytes, rounded up

    //   std::vector<uint32_t> feature_hashes;     // Store hashes of all tokens, uint32_t for 32-bit FNV-1a

    //   // uint8_t *token = new uint8_t[bytes_in_token];
    //   uint8_t token[bytes_in_token];
    //   uint32_t token_hash = 0;

    //   for(int i = 0; i < (total_bases - bases_in_token + 1); i++){
    //         memset(token, 0, bytes_in_token);
    //         uint32_t bit_index = 2*i;      //start bit position of the token

    //         extract_bits(data, bit_index, bits_in_token, token);

    //         // Debug: print extracted token
    //         // printf("Token %d (bits %u-%u): ", i, bit_index, bit_index + bits_in_token - 1);
    //         // for (uint32_t j = 0; j < bytes_in_token; j++) {
    //         //       printf("%02x", token[j]);
    //         // }
    //         // printf("\n");
            
            
    //         FNV1a<uint32_t, bswap>(token, bytes_in_token, seed, &token_hash);
    //         // MurmurHash3_32<bswap>(token, bytes_in_token, seed, &token_hash);
            
    //         // printf(" -> hash: %08x\n", token_hash);
    //         feature_hashes.push_back(token_hash);
    //         token_hash = 0;
    //   }

    //   // delete[] token;   // Free the allocated memory

    //   // printf("Total tokens: %zu\n", feature_hashes.size());
    //   // //print all feature hashes
    //   // for(size_t i=0; i < feature_hashes.size(); i++){
    //   //     printf("%08x ", feature_hashes[i]);
    //   // }
    //   // printf("\n");

    //   hash = compute<uint32_t, bswap>(feature_hashes);
    //   // printf("Final SimHash: %08x\n", hash);

      PUT_U32<bswap>(hash, (uint8_t *)out, 0);
}


//------------------------------------------------------------
REGISTER_FAMILY(SimHash,
   $.src_url    = "###YOURREPOSITORYURL",
   $.src_status = HashFamilyInfo::SRC_UNKNOWN
 );

//The bitwise version is for the encoded form of the data. 
REGISTER_HASH(SimHash_32_bitwise,
   $.desc            = "SIMHASH 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCAL_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_HAMMING_DISTANCE,
   $.impl_flags      = 
            FLAG_IMPL_MULTIPLY |   // FNV1a uses multiplication
            FLAG_IMPL_SLOW,        // Simhash is computationally expensive
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = simhash32_bitwise<false>,
   $.hashfn_bswap    = simhash32_bitwise<true>
 );

// The charwise version is for the non-encoded form of the data.
//  REGISTER_HASH(SimHash_32_charwise,
//    $.desc            = "SIMHASH 32-bit version",
//    $.hash_flags      =
//          0,
//    $.impl_flags      =
//          0,
//    $.bits            = 32,
//    $.verification_LE = 0x0,
//    $.verification_BE = 0x0,
//    $.hashfn_native   = simhash32_charwise<false>,
//    $.hashfn_bswap    = simhash32_charwise<true>
//  );

// REGISTER_HASH(SimHash_64,
//    $.desc            = "SIMHASH 64-bit version",
//    $.hash_flags      =
//          0,
//    $.impl_flags      =
//          0,
//    $.bits            = 64,
//    $.verification_LE = 0x0,
//    $.verification_BE = 0x0,
//    $.hashfn_native   = simhash64<false>,
//    $.hashfn_bswap    = simhash64<true>
//  );

//  REGISTER_HASH(SimHash_96,
//    $.desc            = "SIMHASH 96-bit version",
//    $.hash_flags      =
//          0,
//    $.impl_flags      =
//          0,
//    $.bits            = 96,
//    $.verification_LE = 0x0,
//    $.verification_BE = 0x0,
//    $.hashfn_native   = simhash64<false>,
//    $.hashfn_bswap    = simhash64<true>
//  );

//  REGISTER_HASH(Hash_SimHash_128,
//    $.desc            = "SIMHASH 128-bit version",
//    $.hash_flags      =
//          0,
//    $.impl_flags      =
//          0,
//    $.bits            = 128,
//    $.verification_LE = 0x0,
//    $.verification_BE = 0x0,
//    $.hashfn_native   = simhash64<false>,
//    $.hashfn_bswap    = simhash64<true>
//  );

//------------------------------------------------------------

