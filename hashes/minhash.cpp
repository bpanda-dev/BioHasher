/*
 * MINHASH
 * Copyright (C) 2025 IISc
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

#include "../util/Random.h"

#include <vector>
#include <unordered_map>


void ExtractBitsMinhash(const uint8_t *src, size_t startBit, size_t bitLen, uint8_t *dst){
      // Allocate destination buffer
	memset(dst, 0, (bitLen + 7) / 8); // Clear destination buffer

	for (size_t i = 0; i < bitLen; ++i) {
		size_t srcBitIndex  = startBit + i;
		size_t src_byte_index = srcBitIndex / 8;
		size_t src_bit_offset = 7 - (srcBitIndex % 8);  // // LSB-first (0 = rightmost bit)

		uint8_t bit = (src[src_byte_index] >> src_bit_offset) & 1;

		size_t dst_byte_index = i / 8;
		size_t dst_bit_offset = 7 - (i % 8); // LSB-first in destination too

		dst[dst_byte_index] |= (bit << dst_bit_offset);
	}
}

//-----------------------------------------------------------------------------
#define _mersenne_prime uint64_t((uint64_t(1) << 61) - 1) // A large prime number: 2^61 - 1 : Mersenne prime

#define _max_hash_32 uint64_t(uint64_t(uint64_t(1) << 32) - 1)  // 2^32 - 1 : Maximum hash value for unsigned 32-bit hash functions
#define _hash_range_32 uint64_t(1 << 32)  // 2^32     : Total Number of buckets. 



// template <bool bswap>
// static void minhash32_encoded_seq(const void* in , const size_t len, const seed_t seed, void* out) {
      
//       // Use the inbuilt FNV1a hash function
//       const HashInfo * Fnv1aInfo = findHash("FNV-1a-32");  //Accessing the 32-bit FNV1a hash function from the minhash32
//       if (!Fnv1aInfo) {
//             // Fallback to another hash or throw error
//             fprintf(stderr, "FNV1a hash function not found!\n");
//             out = nullptr;
//             return;
//       }
//       seed_t SeedFNV =  5 + (37 * gGoldenRatio); 

//       const uint8_t* data = static_cast<const uint8_t*>(in);      // Input should be nucleotides in 2-bit format and multiple of 4 bases, i.e. 1 byte.

//       if(in==nullptr || len==0){
//             PUT_U32<bswap>(0, (uint8_t *)out, 0);
//             return;
//       }

//       // Each byte contains 4 bases (2 bits per base). Total Bases = len * 4
//       uint32_t BasesPerByte = 4;
//       uint32_t TotalBases = len * BasesPerByte; 

//       // Use runtime k-mer length if LSH test is active, otherwise use default
//       const uint32_t DefaultMinHashTokenLength = 13; 
//       uint32_t BasesInToken = IsLSHTestActive() ? GetLSHTokenLength() : DefaultMinHashTokenLength;
//       uint32_t BitsInToken = BasesInToken * 2; // in bits
//       uint32_t BytesInToken = (BitsInToken + 7) / 8; // in bytes, rounded up
      
//       if(TotalBases < BasesInToken){
//             // If the total bases are less than the token length, return max hash value
//             PUT_U32<bswap>(0, (uint8_t *)out, 0);
//             return;
//       }

//       // Single token storage.
//       std::vector<uint8_t> token(BytesInToken, 0); 

//       // Generate hash function parameters 'a' and 'b' using the seed.
//       const uint32_t BytesInInt = 4;
      
//       //------------------------------------------------------------
//       //Generate random number,
//       Rand r(seed, BytesInInt);
//       RandSeq rng = r.get_seq(SEQ_DIST_1, BytesInInt); 
//       uint32_t a = 0;
//       uint32_t b = 0;
//       rng.write((void*)&a, 0, 1);
//       rng.write((void*)&b, 0, 1);
//       if(a==b){
//             printf("Oye! the parameter values are equal!");
//       }
//       a = a + 1;
//       //------------------------------------------------------------

//       uint32_t MinHashValue = UINT32_MAX; // Initialize min hash value to maximum possible.
//       for(int i = 0; i < (int)(TotalBases - BasesInToken + 1); i++){
//             uint32_t initHash = 0;

//             std::fill(token.begin(), token.end(), 0);
//             uint32_t TokenStartBitPos = i * 2;

//             // Extract the k-mer (token) from the input data
//             ExtractBitsMinhash(data, TokenStartBitPos, BitsInToken, token.data());  //focus on this => check if this actually correct.

//             if(bswap){
//                   Fnv1aInfo->hashfn_bswap(token.data(), BytesInToken, SeedFNV, &initHash);
//             }
//             else{
//                   Fnv1aInfo->hashfn_native(token.data(), BytesInToken, SeedFNV, &initHash);
//             }

//             uint32_t TokenHash = 0;
//             TokenHash = (((((uint64_t)a * initHash) + b) % _mersenne_prime) & _max_hash_32); // <-- Applying the permutation hash function

//             if(TokenHash < MinHashValue){
//                   MinHashValue = TokenHash;
//             }
//       }

//       PUT_U32<bswap>(MinHashValue, (uint8_t*)out, 0);
// }
//------------------------------------------------------------

template <bool bswap>
static void minhash32_encoded_set(const void* in , const size_t len, const seed_t seed, void* out) {

      /*
            Note that here, the number of tokens is the length/(token size) i.e. non-overlapping tokens. 
            If there is padding, which we can detect from the metadata, we will add one more token.
            TODO: PADDING SUPPORT
      */
      
      // Use the inbuilt FNV1a hash function
      const HashInfo * Fnv1aInfo = findHash("FNV-1a-32");  //Accessing the 32-bit FNV1a hash function from the minhash32
      if (!Fnv1aInfo) {
            // Fallback to another hash or throw error
            fprintf(stderr, "FNV1a hash function not found!\n");
            out = nullptr;
            return;
      }
      seed_t SeedFNV =  5 + (37 * gGoldenRatio); 

      const uint8_t* data = static_cast<const uint8_t*>(in);      // Input should be nucleotides in 2-bit format and multiple of 4 bases, i.e. 1 byte.

      if(in==nullptr || len==0){
            PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }

      // Each byte contains 4 bases (2 bits per base). Total Bases = len * 4
      uint32_t BasesPerByte = 4;
      uint32_t TotalBases = len * BasesPerByte; 

      // Use runtime k-mer length if LSH test is active, otherwise use default
      const uint32_t DefaultMinHashTokenLength = 13; 
      uint32_t BasesInToken = IsLSHTestActive() ? GetLSHTokenLength() : DefaultMinHashTokenLength;
      uint32_t BitsInToken = BasesInToken * 2; // in bits
      uint32_t BytesInToken = (BitsInToken + 7) / 8; // in bytes, rounded up
      
      if(TotalBases < BasesInToken){
            // If the total bases are less than the token length, return max hash value
            PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }

      // Single token storage.
      std::vector<uint8_t> token(BytesInToken, 0); 

      // Generate hash function parameters 'a' and 'b' using the seed.
      const uint32_t BytesInInt = 4;
      
      //------------------------------------------------------------
      //Generate random number,
      Rand r(seed, BytesInInt);
      uint32_t a = uint32_t(r.rand_u64() & 0xFFFFFFFF);
      uint32_t b = uint32_t(r.rand_u64() & 0xFFFFFFFF);
      if(a==b){
            printf("Oye! the parameter values are equal!");
            // printf("Seed: %llu, a: %u, b: %u\n", seed, a, b);
      }
      // printf("Seed: %llu, a: %u, b: %u\n", seed, a, b);
      a = a + 1;  // so that a is never 0
      //------------------------------------------------------------

      uint32_t MinHashValue = UINT32_MAX; // Initialize min hash value to maximum possible.
      
      //For set input, we need to extract non overlapping tokens.
      int numTokens = (TotalBases + BasesInToken - 1) / BasesInToken;   // This will add padding if needed.
      // printf("Num tokens: %d\n", numTokens);
      for(int i = 0; i < numTokens; i++){
            //for each token
            uint32_t initHash = 0;

            std::fill(token.begin(), token.end(), 0);
            uint32_t TokenStartBitPos = i * BasesInToken * 2;     // <- The change is done here for set input. We are extracting non-overlapping tokens.

            // Extract the k-mer (token) from the input data
            ExtractBitsMinhash(data, TokenStartBitPos, BitsInToken, token.data());  //focus on this => check if this actually correct.

            if(bswap){
                  Fnv1aInfo->hashfn_bswap(token.data(), BytesInToken, SeedFNV, &initHash);
            }
            else{
                  Fnv1aInfo->hashfn_native(token.data(), BytesInToken, SeedFNV, &initHash);
            }

            uint32_t TokenHash = 0;
            TokenHash = (((((uint64_t)a * initHash) + b) % _mersenne_prime) & _max_hash_32); // <-- Applying the permutation hash function
            // printf("Token %d: Hash %u\n", i, TokenHash);
            if(TokenHash < MinHashValue){
                  MinHashValue = TokenHash;
            }
      }
      // printf("Final MinHash Value: %u\n", MinHashValue);
      PUT_U32<bswap>(MinHashValue, (uint8_t*)out, 0);
}


//------------------------------------------------------------
REGISTER_FAMILY(MinHash,
   $.src_url    = "https://github.com/bpanda-dev/BioHasher",
   $.src_status = HashFamilyInfo::SRC_ACTIVE
 );

// //The bitwise version is for the encoded form of the data. 
// REGISTER_HASH(MinHash_32_encoded_seq,
//    $.desc            = "MINHASH 32-bit version",
//    $.hash_flags      = FLAG_HASH_LOCAL_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_JACCARD_SIMILARITY | FLAG_HASH_NONOVERLAPPING_TOKENS,
//    $.impl_flags      = FLAG_IMPL_SLOW,
//    $.bits            = 32,
//    $.verification_LE = 0x0,
//    $.verification_BE = 0x0,
//    $.hashfn_native   = minhash32_encoded_seq<false>,
//    $.hashfn_bswap    = minhash32_encoded_seq<true>,
//    $.hashfn_varout_native = nullptr,
//    $.hashfn_varout_bswap  = nullptr
// );

//The bitwise version is for the encoded form of the data. 
REGISTER_HASH(MinHash_32_encoded_set,
   $.desc            = "MINHASH 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCAL_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_JACCARD_SIMILARITY | FLAG_HASH_OVERLAPPING_TOKENS,    // No tokenization property for set input, Set is already tokenized.
   $.impl_flags      = FLAG_IMPL_SLOW,
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = minhash32_encoded_set<false>,
   $.hashfn_bswap    = minhash32_encoded_set<true>,
   $.hashfn_varout_native = nullptr,
   $.hashfn_varout_bswap  = nullptr
);

//------------------------------------------------------------

