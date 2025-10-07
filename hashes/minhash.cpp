/*
 * MINHASH
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
#define MINHASH_FUNCTIONS 32  // Number of hash functions

static int g_default_minhash_token_length = 13; // default token length. In our genomics application, this is typically referred to as a kmer.


// FNV1a 32/64 bit version.
template <typename hashT, bool bswap>
static void FNV1a( const void * in, const size_t len, const seed_t seed, void * out ) {
    const uint8_t * data = (const uint8_t *)in;
    const hashT     C1   = (sizeof(hashT) == 4) ? UINT32_C(2166136261) :
                                                  UINT64_C(0xcbf29ce484222325);
    const hashT     C2   = (sizeof(hashT) == 4) ? UINT32_C(  16777619) :
                                                  UINT64_C(0x100000001b3);
    
      // printf("Data Length: %zu\n", len);
      // printf("Data: ");
      // for (size_t i = 0; i < len; i++) {
      //         printf("%02x ", data[i]);
      // }
      // printf("\n");

      hashT h = (hashT)seed;


    h ^= C1;
    for (size_t i = 0; i < len; i++) {
        h ^= data[i];
        h *= C2;
    }

    h = COND_BSWAP(h, bswap);
    memcpy(out, &h, sizeof(h));
}

// uint32_t simple_hash(uint32_t key, uint32_t seed) {
//     key += seed;
//     key ^= key >> 16;
//     key *= 0x85ebca6b;
//     key ^= key >> 13;
//     key *= 0xc2b2ae35;
//     key ^= key >> 16;
//     return key;
// }


//------------------------------------------------------------
void extract_bits_minhash(const uint8_t *src,
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
static void minhash32_bitwise( const void * in, const size_t len, const seed_t seed, void * out, const size_t outlen=1) {
      
      // Input validation
      if (in == nullptr || out == nullptr || len == 0) {
            if (out) PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }

      const uint8_t* data = static_cast<const uint8_t*>(in);

      uint32_t bases_per_byte = 4; // 2 bits per base
      int total_bases = len * bases_per_byte; // Total number of bases
      
      // Use runtime k-mer length if LSH test is active, otherwise use default
      int bases_in_token = is_lsh_test_active() ? get_lsh_token_length() : g_default_minhash_token_length;
      uint32_t bits_in_token = bases_in_token * 2; // in bits
      uint32_t bytes_in_token = (bits_in_token + 7) / 8; // in bytes, rounded up

      if (total_bases < bases_in_token) {
            if (out) PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }
      
      // Use the defined MINHASH_FUNCTIONS constant!
      uint32_t signatures[MINHASH_FUNCTIONS];
      // Initialize all signatures to maximum
      for (int func = 0; func < MINHASH_FUNCTIONS; func++) {
            signatures[func] = UINT32_MAX;
      }

      std::vector<uint8_t> token(bytes_in_token);

      for(int i = 0; i < (total_bases - bases_in_token + 1); i++){
            std::fill(token.begin(), token.end(), 0);
            uint32_t bit_index = 2*i;      //start bit position of the token

            extract_bits_minhash(data, bit_index, bits_in_token, token.data());
                        
            // Hash this k-mer
            // Apply ALL hash functions to this k-mer
            for (int func = 0; func < MINHASH_FUNCTIONS; func++) {
                  uint32_t hash_seed = seed + func * 0x9e3779b9;  // Different seed per function

                  uint32_t token_hash = 0;

                  FNV1a<uint32_t, bswap>(token.data(), bytes_in_token, hash_seed, &token_hash);

                  if (token_hash < signatures[func]) {  //storing the minimum hash value
                      signatures[func] = token_hash;
                  }
            }
      }

      // TODO: future work will be on returning the entire signature array. 
    
      // Here I use an heuristic to xor the least three values into one for returning.
      // Find the 3 minimum signatures
      uint32_t min_signatures[3] = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
    
      for (int func = 0; func < MINHASH_FUNCTIONS; func++) {
            uint32_t sig = signatures[func];
            
            // Insert into sorted min array
            if (sig < min_signatures[0]) {
                  min_signatures[2] = min_signatures[1];
                  min_signatures[1] = min_signatures[0];
                  min_signatures[0] = sig;
            } else if (sig < min_signatures[1]) {
                  min_signatures[2] = min_signatures[1];
                  min_signatures[1] = sig;
            } else if (sig < min_signatures[2]) {
                  min_signatures[2] = sig;
            }
      }

      // XOR the 3 minimum signatures
      // uint32_t final_hash = min_signatures[0] ^ min_signatures[1] ^ min_signatures[2];
      
      uint32_t final_hash = min_signatures[0]; // For now, just return the smallest signature.

      PUT_U32<bswap>(final_hash, (uint8_t *)out, 0);
}


//------------------------------------------------------------
REGISTER_FAMILY(MinHash,
   $.src_url    = "###YOURREPOSITORYURL",
   $.src_status = HashFamilyInfo::SRC_UNKNOWN
 );

//The bitwise version is for the encoded form of the data. 
REGISTER_HASH(MinHash_32_bitwise,
   $.desc            = "MINHASH 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCAL_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_VARIABLE_OUTPUT_SIZE,
   $.impl_flags      = FLAG_IMPL_MULTIPLY | FLAG_IMPL_SLOW,        //FNV1a uses multiplication and  MinHash is computationally expensive
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
//    $.hashfn_native   = nullptr,
//    $.hashfn_bswap    = nullptr,
   $.hashfn_varout_native = minhash32_bitwise<false>,
   $.hashfn_varout_bswap  = minhash32_bitwise<true>
);

//------------------------------------------------------------

