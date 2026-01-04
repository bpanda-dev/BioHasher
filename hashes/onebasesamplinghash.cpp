/*
 * One base sampling Hash Function for Hamming LSH
 * Copyright (C) 2025 IISc
 *
 * One-base sampling is a locality-sensitive hashing technique designed for Hamming distance.
 * It works by randomly sampling one base from the hash.
 * The hash value is sampled base. Similar inputs (small Hamming distance)
 * will have similar hash values with high probability.
 *
 */
#include "Platform.h"
#include "Hashlib.h"
#include "LSHGlobals.h"
#include "../util/Random.h"

#include <vector>
#include <cstdint>





















//-----------------------------------------------------------------------------
// Helper function to extract a single bit from a byte array
static inline uint8_t extractBit(const uint8_t* data, size_t bitPosition) {
    size_t byteIndex = bitPosition / 8;
    size_t bitOffset = bitPosition % 8;
    return (data[byteIndex] >> bitOffset) & 0x01;
}


//-----------------------------------------------------------------------------
// Hamming LSH for DNA sequences (2-bit encoded)
// This version is optimized for biological sequences

// Let |S| be the sequence length in bases.
// We will just use the seed and generate a random number between 0 and |S| - 1.
// Then we will extract the encoded base at that position and return that as the hash.
// Note that we are working on two bit encoded data. so our atomic unit is 2 bits.
// Also note that that length 'len' is in bytes. We need to divide it by two to get |S|.

template <bool bswap>
static void hamming32_encoded_seq(const void* in, const size_t len, const seed_t seed, void* out) {
    
    // Input validation
    if (in == nullptr || len == 0) {
        PUT_U32<bswap>(0, (uint8_t*)out, 0);
        return;
    }
    
    const uint8_t* data = static_cast<const uint8_t*>(in);
    
    // For DNA sequences: each byte contains 4 bases (2 bits per base)
    const uint32_t basesPerByte = 4;
    const uint32_t totalBases = len * basesPerByte;
    
    // Seed-based random number generator
    Rand r(seed, sizeof(uint32_t));
    uint32_t randomPosition = r.rand_range(totalBases);

    uint32_t hashValue = 0;
    
    // Extract the 2-bit encoded base at the random position
    uint32_t byteIndex = randomPosition / basesPerByte;
    uint32_t bitOffset = (randomPosition % basesPerByte) * 2;
    uint8_t baseValue = (data[byteIndex] >> bitOffset) & 0x03; // Extract 2 bits
    hashValue = baseValue & 0x00000003; // Store in hash value
    
    PUT_U32<bswap>(hashValue, (uint8_t*)out, 0);
}


// len is in bytes
template <bool bswap>
static void oneBaseSamplingHash_32(const void* in, const size_t len, const seed_t seed, void* out) {
    
    // Input validation
    if (in == nullptr || len == 0) {
        PUT_U32<bswap>(0, (uint8_t*)out, 0);
        return;
    }
    
    const uint8_t* data = static_cast<const uint8_t*>(in);

    const uint32_t totalBases = len;

    Rand r(seed, sizeof(uint32_t));
    uint32_t randomBase = r.rand_range(totalBases);

    // printf(">>> %d\n", randomBase);

    uint32_t hashValue = 0;
    
    // Extract the 8-bit base at the random position
    hashValue = static_cast<uint32_t>(data[randomBase]);
    
    PUT_U32<bswap>(hashValue, (uint8_t*)out, 0);
}


//------------------------------------------------------------
REGISTER_FAMILY(HammingLSH,
   $.src_url    = "https://github.com/bpanda-dev/BioHasher",
   $.src_status = HashFamilyInfo::SRC_UNKNOWN
);

// Standard 32-bit one-base sampling hashing
REGISTER_HASH(OneBaseSamplingHash_32,
   $.desc            = "One-Base Sampling 32-bit - Random base sampling for Hamming distance preservation",
   $.hash_flags      = FLAG_HASH_LOCALITY_SENSITIVE | FLAG_HASH_HAMMING_SIMILARITY,
   $.impl_flags      = 0,
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = oneBaseSamplingHash_32<false>,
   $.hashfn_bswap    = oneBaseSamplingHash_32<true>
);

// Standard 64-bit one-base sampling hashing
// REGISTER_HASH(OneBaseSamplingHash_64,
//    $.desc            = "One-Base Sampling 64-bit - Random base sampling for Hamming distance preservation",
//    $.hash_flags      = FLAG_HASH_LOCALITY_SENSITIVE, FLAG_HASH_HAMMING_SIMILARITY,
//    $.impl_flags      = 0,
//    $.bits            = 64,
//    $.verification_LE = 0x0,
//    $.verification_BE = 0x0,
//    $.hashfn_native   = oneBaseSamplingHash_64<false>,
//    $.hashfn_bswap    = oneBaseSamplingHash_64<true>
// );

//------------------------------------------------------------
