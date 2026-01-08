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
 * 
 * ###YOURLICENSETEXT
 */

#include "Platform.h"
#include "Hashlib.h"
#include "LSHGlobals.h"

#include "../util/Random.h"

#include <vector>
#include <unordered_map>



void ExtractTokensMinhash(const uint8_t *src, size_t startBase, size_t baseLen, uint8_t *dst){
      // Extracts a k-mer (token) from the source sequence starting at startBase position
      // and copies it to the destination buffer.
      // src: Pointer to the source sequence (1-byte per base representation)
      // startBase: Starting position in bases
      // baseLen: Length of the k-mer in bases
      // dst: Pointer to the destination buffer

      size_t startByte = startBase; // Since each base is represented by 1 byte in this context
      for(size_t i = 0; i < baseLen; i++){
            dst[i] = src[startByte + i];
      }
}

//-----------------------------------------------------------------------------
#define _mersenne_prime uint64_t((uint64_t(1) << 61) - 1) // A large prime number: 2^61 - 1 : Mersenne prime

#define _max_hash_32 uint64_t(uint64_t(uint64_t(1) << 32) - 1)  // 2^32 - 1 : Maximum hash value for unsigned 32-bit hash functions
#define _hash_range_32 uint64_t(1 << 32)  // 2^32     : Total Number of buckets. 





template <bool bswap>
static void minHash32(const void* in , const size_t len, const seed_t seed, void* out) {

      //We will use the inbuilt FNV1a hash function
      const HashInfo * Fnv1aInfo = findHash("FNV-1a-32");  //Accessing the 32-bit FNV1a hash function that is available in smhasher3.
      if (!Fnv1aInfo) {
            // Fallback to another hash or throw error. For now we will throw an error.
            fprintf(stderr, "FNV1a hash function not found!\n");
            out = nullptr;
            return;
      }
      seed_t SeedFNV = g_GoldenRatio;      //A constant seed for FNV1a hash function which will be used by all the hashes in this hash family.

      const uint8_t* data = static_cast<const uint8_t*>(in);      // input is a neucleotide sequence in char format (ASCII) and length is in bytes.

      if(in==nullptr || len==0){
            PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }

      // Use runtime k-mer length if LSH test is active, otherwise throw an error
      const uint32_t DefaultMinHashTokenLength = 0; 
      uint32_t BasesInToken = IsTestActive() ? GetTokenLength() : DefaultMinHashTokenLength;
      uint32_t BytesInToken = BasesInToken; 
      
      if(len < BasesInToken){
            // If the total bases are less than the token length, return max hash value
            // this is a redundant check but just to be safe. One is there in LSH collision test too.
            PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }

      std::vector<uint8_t> token(BytesInToken, 0); // Token storage.

      //Generate random number,
      Rand r(seed, 4);
      RandSeq rng = r.get_seq(SEQ_DIST_1, 4); 
      uint32_t a = 0;
      uint32_t b = 0;
      rng.write((void*)&a, 1, 1);
      rng.write((void*)&b, 0, 1);
      if(a==b){
            printf("Oye! the parameter values are equal!");
      }
      if(a==0){
            printf("Oye! the parameter 'a' is zero!");
      }

      uint32_t MinHashValue = UINT32_MAX; // Initialize min hash value to maximum possible.
      for(int i = 0; i < (int)(len - BasesInToken + 1); i++){
            uint32_t initHash = 0;

            std::fill(token.begin(), token.end(), 0);
            uint32_t TokenStartPos = i;

            // Extract the k-mer (token) from the input data
            ExtractTokensMinhash(data, TokenStartPos, BasesInToken, token.data());  //focus on this => check if this actually correct.

            if(bswap){
                  Fnv1aInfo->hashfn_bswap(token.data(), BasesInToken, SeedFNV, &initHash);
            }
            else{
                  Fnv1aInfo->hashfn_native(token.data(), BasesInToken, SeedFNV, &initHash);
            }

            uint32_t TokenHash = 0;
            TokenHash = (((((uint64_t)a * initHash) + b) % _mersenne_prime) & _max_hash_32); // <-- Applying the permutation hash function

            if(TokenHash < MinHashValue){
                  MinHashValue = TokenHash;
            }
      }

      PUT_U32<bswap>(MinHashValue, (uint8_t*)out, 0);
}
//------------------------------------------------------------

REGISTER_FAMILY(MinHash,
   $.src_url    = "https://github.com/bpanda-dev/BioHasher",
   $.src_status = HashFamilyInfo::SRC_ACTIVE
 );

REGISTER_HASH(MinHash_32,
   $.desc            = "MinHash 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCALITY_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_JACCARD_SIMILARITY,
   $.impl_flags      = FLAG_IMPL_SLOW,
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = minHash32<false>,
   $.hashfn_bswap    = minHash32<true>
);


//------------------------------------------------------------

