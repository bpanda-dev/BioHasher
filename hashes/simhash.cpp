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
template <typename hashtype, bool bswap>
static hashtype simcompute(const std::vector<hashtype>& FeatureHashes){
      std::vector<int> bit_count_vector((sizeof(hashtype) * 8), 0);    // each position counts number of 1's - number of 0's. sizeof(hashtype) * 8 =>total bits

      //Compute frequency of each hash and store it in frequency map
      // std::unordered_map<hashtype, int> frequency_map;
      // for (const auto& h : FeatureHashes) {
      //     frequency_map[h]++;
      // }

      // Count the weighted number of 1's and 0's in each position.
	for (const auto& hash : FeatureHashes) {
		hashtype h = hash;
		// int weight = frequency_map[h];	// gives the weight for that hash
            int weight = 1;
		for (size_t i = 0; i < (sizeof(hashtype) * 8); ++i) {
			bit_count_vector[i] += (h & 1) ? weight : ((-1)*(weight));
			h >>= 1;   
		}
	}
      
      // Determine the final bits of the SimHash
      hashtype simhash = 0;
      for (size_t i = 0; i < (sizeof(hashtype) * 8); ++i) {
          if (bit_count_vector[i] > 0) {
              simhash |= (hashtype(1) << i);
          }
      }

      return simhash;
}


//------------------------------------------------------------
void ExtractBitsSimhash(const uint8_t *src,
                                  size_t start_bit,
                                  size_t bit_len, uint8_t *dst)
{
    // Allocate destination buffer
	memset(dst, 0, (bit_len + 7) / 8);

	for (size_t i = 0; i < bit_len; ++i) {
		size_t src_bit_index  = start_bit + i;
		size_t src_byte_index = src_bit_index / 8;
		size_t src_bit_offset = 7 - (src_bit_index % 8);  // // LSB-first (0 = rightmost bit)

		uint8_t bit = (src[src_byte_index] >> src_bit_offset) & 1;

		size_t dst_byte_index = i / 8;
		size_t dst_bit_offset = 7 - (i % 8); // LSB-first in destination too

		dst[dst_byte_index] |= (bit << dst_bit_offset);
	}
}


// // Copy `bit_len` bits starting at bit index `start_bit` in `src` into `dst`.
// // Bits are MSB-first inside bytes (same as your code).
// // dst is zero-initialized by this function; caller must allocate enough bytes.
// void ExtractBitsSimhash(const uint8_t* src, size_t start_bit, size_t bit_len, uint8_t* dst) {
//     if (bit_len == 0) return;

//     size_t dst_bytes = (bit_len + 7) / 8;
//     std::memset(dst, 0, dst_bytes);

//     size_t src_byte_index = start_bit / 8;
//     unsigned start_offset = static_cast<unsigned>(start_bit % 8);

//     // Fast path: byte-aligned start
//     if (start_offset == 0) {
//         size_t full_bytes = bit_len / 8;
//         if (full_bytes) std::memcpy(dst, src + src_byte_index, full_bytes);

//         unsigned tail_bits = static_cast<unsigned>(bit_len % 8);
//         if (tail_bits) {
//             // take next source byte and keep only the top `tail_bits`
//             uint8_t last = src[src_byte_index + full_bytes];
//             uint8_t mask = static_cast<uint8_t>(0xFF << (8 - tail_bits)); // top tail_bits set
//             dst[full_bytes] = last & mask;
//         }
//         return;
//     }

//     // General case: unaligned start — build each destination byte from two source bytes
//     // Combined formula: dest_byte = (src[b] << shift) | (src[b+1] >> (8 - shift))
//     // where b = (start_bit + 8*j) / 8 and shift = (start_bit + 8*j) % 8
//     for (size_t j = 0; j < dst_bytes; ++j) {
//         size_t bit_off = start_bit + 8 * j;
//         size_t idx = bit_off / 8;
//         unsigned shift = static_cast<unsigned>(bit_off % 8);

//         uint8_t b1 = src[idx];
//         // safe to read next byte even if it's past valid data if caller padded src; otherwise guard
//         uint8_t b2 = src[idx + 1];

//         uint8_t combined = static_cast<uint8_t>((uint8_t)(b1 << shift) | (uint8_t)(b2 >> (8 - shift)));

//         // If last byte is partial, mask off low bits (we keep top (bit_len%8) bits)
//         if (j == dst_bytes - 1) {
//             unsigned tail_bits = static_cast<unsigned>(bit_len % 8);
//             if (tail_bits) {
//                 uint8_t mask = static_cast<uint8_t>(0xFF << (8 - tail_bits));
//                 dst[j] = combined & mask;
//             } else {
//                 dst[j] = combined;
//             }
//         } else {
//             dst[j] = combined;
//         }
//     }
// }


//current limit: in should have nucleotides encoded in 2-bit format and multiple of 4 bases (1 byte). No padding allowed.
template <bool bswap>
static void simhash32_encoded( const void * in, const size_t len, const seed_t seed, void * out ) {
      // Input validation
      if (in == nullptr || out == nullptr || len == 0) {
            if (out) PUT_U32<bswap>(0, (uint8_t *)out, 0);
            return;
      }

      // Using the inbuilt FNV1a hash function
      const HashInfo * Fnv1aInfo = findHash("FNV-1a-32");  //Accessing the 32-bit FNV1a hash function from the minhash32
      if (!Fnv1aInfo) {
            // Fallback to another hash or throw error
            fprintf(stderr, "FNV1a hash function not found!\n");
            out = nullptr;
            return;
      }
      seed_t SeedFNV =  seed + (37 * gGoldenRatio); 

      const uint8_t* data = static_cast<const uint8_t*>(in);      // Input should be nucleotides in 2-bit format and multiple of 4 bases, i.e. 1 byte.

      
      uint32_t hash = 0;
     
      uint32_t BasesPerByte = 4; // 2 bits per base
      uint32_t TotalBases = len * BasesPerByte; // Total number of bases

      // Using runtime k-mer length if LSH test is active, otherwise use default
      const uint32_t DefaultSimHashTokenLength = 13; 
      uint32_t BasesInToken = IsLSHTestActive() ? GetLSHTokenLength() : DefaultSimHashTokenLength;
      uint32_t BitsInToken = BasesInToken * 2; // in bits
      uint32_t BytesInToken = (BitsInToken + 7) / 8; // in bytes, rounded up

      std::vector<uint32_t> FeatureHashes((TotalBases - BasesInToken + 1), 0);     // Store hashes of all tokens,

      std::vector<uint8_t> token(BytesInToken, 0); 
      uint32_t TokenHash = 0;

      for(int i = 0; i < static_cast<int>(TotalBases - BasesInToken + 1); i++){
            
            std::fill(token.begin(), token.end(), 0);
            
            uint32_t TokenStartBitPos = i * 2;      //start bit position of the token

            ExtractBitsSimhash(data, TokenStartBitPos, BitsInToken, token.data());  // Extract the k-mer (token) from the input data

            // Debug: print extracted token
            // printf("Token %d (bits %u-%u): ", i, BitIndex, BitIndex + BitsInToken - 1);
            // for (uint32_t j = 0; j < BytesInToken; j++) {
            //       printf("%02x", token[j]);
            // }
            // printf("\n");
            
            if(bswap){
                  Fnv1aInfo->hashfn_bswap(token.data(), BytesInToken, SeedFNV, &TokenHash);
            }
            else{
                  Fnv1aInfo->hashfn_native(token.data(), BytesInToken, SeedFNV, &TokenHash);
            }
            
            // printf(" -> hash: %08x\n", TokenHash);
            FeatureHashes[i] = TokenHash;
            TokenHash = 0;
      }

      // printf("Total tokens: %zu\n", FeatureHashes.size());
      // //print all feature hashes
      // for(size_t i=0; i < FeatureHashes.size(); i++){
      //     printf("%08x ", FeatureHashes[i]);
      // }
      // printf("\n");

      hash = simcompute<uint32_t, bswap>(FeatureHashes);
      // printf("Final SimHash: %08x\n", hash);

      PUT_U32<bswap>(hash, (uint8_t *)out, 0);
}


//------------------------------------------------------------
REGISTER_FAMILY(SimHash,
   $.src_url    = "###YOURREPOSITORYURL",
   $.src_status = HashFamilyInfo::SRC_UNKNOWN
 );

//The bitwise version is for the encoded form of the data. 
REGISTER_HASH(SimHash_32_encoded,
   $.desc            = "SIMHASH 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCAL_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_HAMMING_DISTANCE,
   $.impl_flags      = 
            FLAG_IMPL_MULTIPLY |   // FNV1a uses multiplication
            FLAG_IMPL_SLOW,        // Simhash is computationally expensive
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = simhash32_encoded<false>,
   $.hashfn_bswap    = simhash32_encoded<true>,
   $.hashfn_varout_native = nullptr,
   $.hashfn_varout_bswap  = nullptr
 );

//------------------------------------------------------------

