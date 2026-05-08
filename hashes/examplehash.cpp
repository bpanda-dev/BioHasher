/*
 * One base sampling Hash Function for Hamming LSH
 * Copyright (C) 2025 IISc
 *
 * One-base sampling is a locality-sensitive hashing technique designed for Hamming distance.
 * It works by randomly sampling one base from the input.
 * The hash value is sampled base. Similar inputs (small Hamming distance)
 * will have similar hash values with high probability.
 *
 */

#include "specifics.h"
#include "Hashlib.h"
#include <random>
#include <vector>
#include <cstdint>
#include "assertMsg.h"


static bool check_equality_32(void* inp1, void* inp2){
    uint32_t* data1 = static_cast<uint32_t*>(inp1);
    uint32_t* data2 = static_cast<uint32_t*>(inp2);

    if(data1 == nullptr || data2 == nullptr){
        return false;
    }
    return (*data1 == *data2);
}

// Test this out.
static inline uint64_t hash_seed_to_random(seed_t seed) {
    uint64_t z = (seed + 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static void exampleHash_32(const void* in, const size_t len, const seed_t seed, void* out) {
    // Input validation
    if (in == nullptr || len == 0) {
        PUT_U32(0, (uint8_t*)out, 0);
        return;
    }

    const uint8_t* data = static_cast<const uint8_t*>(in);

    const uint32_t totalBases = len;

    std::mt19937 rng(seed);
    std::uniform_int_distribution<uint32_t> dist(0, len - 1);

    uint32_t randomBase = dist(rng);
    // printf(">>> %d\n", randomBase);

    uint32_t hashValue = 0;
    
    // Extract the 8-bit base at the random position
    hashValue = static_cast<uint32_t>(data[randomBase]);
    
    PUT_U32(hashValue, (uint8_t*)out, 0);
}

static double HammingSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t in1_len, const uint32_t in2_len) {
    // Hamming distance only defined for equal-length sequences
    BIOHASHER_ASSERT(in1_len == in2_len, "For hamming, we need both the sequences to be of same length. "
                                 "\n Either use substitution-Only mutation model or"
                                 "\n Change the similarity function of the hash Family to one which supports unequal length input pair!");

    uint32_t length = in1_len;
    uint32_t similar = 0;
    // Count mismatches in overlapping region
    for (uint32_t i = 0; i < length; i++) {
        if (seq1[i] == seq2[i]) {
            similar++;
        }
    }
    return (static_cast<float>(similar) / static_cast<float>(length));
}

//------------------------------------------------------------
REGISTER_FAMILY(exampleLSH,
   $.src_url    = "",
   $.src_status = HashFamilyInfo::SRC_UNKNOWN
);

// Standard 32-bit one-base sampling hashing
REGISTER_HASH(exampleHash,
   $.desc           = "exampleHash with Hamming distance preservation",
   $.hash_flags     = FLAG_HASH_LOCALITY_SENSITIVE,
   $.impl_flags     = 0,
   $.bits           = 32,
   $.hashfn         = exampleHash_32,
   $.similarity_name = "Hamming",
   $.similarityfn   = HammingSimilarity,
   $.check_equality_fn = check_equality_32
);


//------------------------------------------------------------
