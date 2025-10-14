//------------------------------------------------------------
// Seed USAGE:
// seed_t seedA = seed + (13 * gGoldenRatio); // Different seed for ParamA
// seed_t seedB = seed + (23 * gGoldenRatio); // Different seed for ParamB
// seed_t SeedFNV = seed + (37 * gGoldenRatio);  // Change the seed for use in FNV1a
//------------------------------------------------------------


/*
 * MINHASH
 * Copyright (C) 2022 IISc
 *
 * A brief intro about minhash:
 * MinHash is a locality-sensitive hashing (LSH) technique used to estimate the similarity between
 * two sets. It is particularly useful for large datasets where exact similarity computation can be
 * computationally expensive. The core idea of MinHash is to create a compact representation (signature)
 * of each set, such that the similarity between the sets can be approximated by comparing their signatures.
 *
 * Given a set of words, we first hash it to convert it into a numeric representation. (in our bitwise case, it is already in numeric format)
 * We then apply k independent hash functions (each simulating a random permutation of the universe). 
 * For each hash function, we compute the minimum hash value across all tokens in the set. 
 * The vector of these minima is the MinHash signature.
 * 
 * ###YOURLICENSETEXT
 */
#include "Platform.h"
#include "Hashlib.h"
#include "LSHGlobals.h"

#include "Mathmult.h"
#include "../util/Random.h"

#include <vector>
#include <unordered_map>

//------------------------------------------------------------
//Ensure that the number of permutations is not more than 2^32 for 32-bit hashes and 2^64 for 64-bit hashes. Though probably this will never happen,
//we added this note for clarity.
// #define MINHASH_NUM_PERMUTATIONS 32  // Number of hash functions
#define _mersenne_prime uint64_t((uint64_t(1) << 61) - 1) // A large prime number: 2^61 - 1 : Mersenne prime

// The _max_hash and _hash_range are defined to return a 32 bit version of the hash. _max_hash is actually used as a mask to limit the hash value to 32 bits.
#define _max_hash uint64_t(uint64_t(uint64_t(1) << 32) - 1)  // 2^32 - 1 : Maximum hash value for unsigned 32-bit hash functions
#define _hash_range uint64_t(1 << 32)  // 2^32     : Total Number of buckets. 


uint32_t DefaultMinHashTokenLength = 13; // Default token/kmer length for MinHash

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
static void minhash32_bitwise( const void * in, const size_t len, const seed_t seed, void * out, size_t outlen) {
      
      //outlen depends on the number of permutations. 
      uint32_t MINHASH_NUM_PERMUTATIONS = (uint32_t)outlen;

      printf("MinHash 32-bit: Number of permutations = %u\n", MINHASH_NUM_PERMUTATIONS);
      // Use the inbuilt FNV1a hash function
      const HashInfo * Fnv1aInfo = findHash("FNV-1a-32");  //Accessing the 32-bit FNV1a hash function from the minhash32
      if (!Fnv1aInfo) {
            // Fallback to another hash or throw error
            fprintf(stderr, "FNV1a hash function not found!\n");
            out = nullptr;
            return;
      }

      // Input validation: if invalid. return 0
      if (in == nullptr || out == nullptr || len == 0) {
            if (out) PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }

      const uint8_t* data = static_cast<const uint8_t*>(in);

      // Specific to encoded nucleotide data. (2bit version). No support for N.
      uint32_t BasesPerByte = 4; // 2 bits per base
      uint32_t TotalBases = len * BasesPerByte; // Total number of bases
      
      // Use runtime k-mer length if LSH test is active, otherwise use default
      uint32_t BasesInToken = IsLSHTestActive() ? GetLSHTokenLength() : DefaultMinHashTokenLength;
      uint32_t BitsInToken = BasesInToken * 2; // in bits
      uint32_t BytesInToken = (BitsInToken + 7) / 8; // in bytes, rounded up
      
      if (TotalBases < BasesInToken) { //check
            // Not enough bases to form a single token
            fprintf(stderr, "Input length too short for the specified token length!\n");
            if (out) PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }
      
      uint32_t signatures[MINHASH_NUM_PERMUTATIONS];

      // Initialize all signatures to maximum
      for (int func = 0; func < MINHASH_NUM_PERMUTATIONS; func++) {
            signatures[func] = UINT32_MAX;
      }

      std::vector<uint8_t> token(BytesInToken); // storage for a single token.

      // Parameters for use in the permutation hash functions : h(x) = (a * x + b) mod p. Here p is a large prime number (Mersenne prime).
      // We generate random a and b values for each permutation
      std::vector<uint32_t> ParamA(MINHASH_NUM_PERMUTATIONS);
      std::vector<uint32_t> ParamB(MINHASH_NUM_PERMUTATIONS);
      uint32_t BytesInInt = 4;

      seed_t seedA = seed + (13 * gGoldenRatio); // Different seed for ParamA
      seed_t seedB = seed + (23 * gGoldenRatio); // Different seed for ParamB
      Rand rA( seedA, BytesInInt );
      Rand rB( seedB, BytesInInt );
      RandSeq rsA = rA.get_seq(SEQ_DIST_1, BytesInInt);
      RandSeq rsB = rB.get_seq(SEQ_DIST_1, BytesInInt);
      rsA.write(&ParamA[0], 1, MINHASH_NUM_PERMUTATIONS);
      rsB.write(&ParamB[0], 0, MINHASH_NUM_PERMUTATIONS);

      // print ParamA and ParamB
      // printf("%d:%d\n", ParamA[0], ParamB[0]);
      // for(int i = 0; i < (int)MINHASH_NUM_PERMUTATIONS; i++){ 
      //       printf("ParamA[%d] = %u, ParamB[%d] = %u\n", i, ParamA[i], i, ParamB[i]);
      // }

      seed_t SeedFNV = seed + (37 * gGoldenRatio);  // Change the seed for use in FNV1a

      for(int i = 0; i < (int)(TotalBases - BasesInToken + 1); i++){
            
            std::fill(token.begin(), token.end(), 0); //reset the token
            uint32_t BitIndex = 2*i;      //start bit position of the token. each base is 2 bits. 

            extract_bits_minhash(data, BitIndex, BitsInToken, token.data());  //Extract the token bits. we call this as kmer in genomics literature.

            // First, we hash this k-mer to convert it into a random numeric representation of same bit length. Since, here we have fixed 
            // same input bit length of token, we directly compute the multiple hash functions(i.e permutations), 
            // we will compute the minimum hash value across all tokens in the set. 
            // Logic: for each token, we apply all the permutations one by one. Once we have the permutation hash value for the token, 
            // we compare it with the current minimum for that permutation and update if it's smaller. 
            // After processing all tokens, the array of minimum values for each permutation forms the MinHash signature.
            for (uint32_t func = 0; func < MINHASH_NUM_PERMUTATIONS; func++) {
                  uint32_t initHash = 0;
                  if(bswap){
                        Fnv1aInfo->hashfn_bswap(token.data(), BytesInToken, SeedFNV, &initHash);
                  }
                  else{
                        Fnv1aInfo->hashfn_native(token.data(), BytesInToken, SeedFNV, &initHash);
                  }
                  
                  // Now the initHash has 32 bits. We will apply the permutation hash functions to this initHash output.    
                  uint32_t TokenHash = 0;
                  TokenHash = (((((uint64_t)ParamA[func] * initHash) + ParamB[func]) % _mersenne_prime) & _max_hash); // <-- Applying the permutation hash function

                  if (TokenHash < signatures[func]) {  //storing the minimum hash value
                      signatures[func] = TokenHash;
                  }
            }
      }

      //print signatures
      for (int func = 0; func < MINHASH_NUM_PERMUTATIONS; func++) {
            printf("Signature[%d] = %u\n", func, signatures[func]);
      }

      // Copy the signatures to output vector.
      // Copy signatures to output buffer
      size_t copy_count = (size_t)MINHASH_NUM_PERMUTATIONS;
      for (size_t i = 0; i < copy_count; i++) {
            PUT_U32<bswap>(signatures[i], (uint8_t*)out, i * sizeof(uint32_t));
      }
}


//------------------------------------------------------------
REGISTER_FAMILY(MinHash,
   $.src_url    = "###YOURREPOSITORYURL",
   $.src_status = HashFamilyInfo::SRC_UNKNOWN
 );

//The bitwise version is for the encoded form of the data. 
REGISTER_HASH(MinHash_32_bitwise,
   $.desc            = "MINHASH 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCAL_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_VARIABLE_OUTPUT_SIZE | FLAG_HASH_JACCARD_SIMILARITY,
   $.impl_flags      = FLAG_IMPL_MULTIPLY | FLAG_IMPL_SLOW,        //FNV1a uses multiplication and  MinHash is computationally expensive
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = nullptr,
   $.hashfn_bswap    = nullptr,
   $.hashfn_varout_native = minhash32_bitwise<false>,
   $.hashfn_varout_bswap  = minhash32_bitwise<true>
);

//------------------------------------------------------------

