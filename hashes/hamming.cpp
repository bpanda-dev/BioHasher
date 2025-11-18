/*
 * HAMMING LSH
 * Copyright (C) 2025 IISc
 *
 * A brief intro about Hamming LSH:
 * Hamming LSH is a locality-sensitive hashing technique designed for Hamming distance.
 * It works by randomly sampling bit positions from the input and checking if they are set (1).
 * The hash value is constructed from the sampled bits. Similar inputs (small Hamming distance)
 * will have similar hash values with high probability.
 *
 * The algorithm:
 * 1. Generate A random BASE position using a seed
 * 2. For each position, extract the BASE value from the input
 * 3. return the base as the hash value
 * 
 * This preserves Hamming distance: inputs that differ in few bits will produce
 * similar hash values, while inputs that differ in many bits will produce very different hashes.
 * 
 * ###YOURLICENSETEXT
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


//------------------------------------------------------------
REGISTER_FAMILY(HammingLSH,
   $.src_url    = "https://github.com/bpanda-dev/BioHasher",
   $.src_status = HashFamilyInfo::SRC_ACTIVE
);

// Standard 32-bit Hamming LSH
REGISTER_HASH(Hamming_32_encoded_seq,
   $.desc            = "Hamming LSH 32-bit - Random bit sampling for Hamming distance preservation",
   $.hash_flags      = FLAG_HASH_LOCAL_SENSITIVE | FLAG_HASH_HAMMING_DISTANCE,
   $.impl_flags      = 0,
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = hamming32_encoded_seq<false>,
   $.hashfn_bswap    = hamming32_encoded_seq<true>
);

//------------------------------------------------------------
