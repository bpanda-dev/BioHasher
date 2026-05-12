/*
 * ###YOURHASHNAME
 * Copyright (C) 2022 ###YOURNAME
 *
 * ###YOURLICENSETEXT
 */
#include "specifics.h"
#include "Hashlib.h"
#include "LSHGlobals.h"

#include "assertMsg.h"

// XXX Your hash filename MUST end in .cpp, and it MUST start with a
// lowercase letter!
//
// XXX Don't forget to add your new filename to the list in
// hashes/Hashsrc.cmake, keeping the list sorted by size!


//------------------------------------------------------------
// ####YOURHASHPARAMETERS
// Hash function parameters (e.g., k-mer size 'k' for minimizers).
// Add any parameters here and include them in REGISTER_HASH() below —
// they will be embedded in the hash family metadata and propagated to
// output CSV files for easy cross-run comparison. 

// Here we show an example of two parameters, 
// but you can have as many as you need, and they can be of any type (e.g., int, float, double, etc.).
#define PARAMETER_1 10
#define PARAMETER_2 20
//------------------------------------------------------------


//------------------------------------------------------------
// ###YOURHASHEQUALITYCHECKER: Logic to decide when two hash 
// outputs are equal. The current default implementation, assumes
// the outputs are single values of a certain type (e.g., uint32_t), 
// and just compares those. It can be char, int, int32, float, double, int64, etc. 
// If your hash outputs are more complex (e.g., a struct), 
// you can implement your custom equality checking logic here.
static bool check_equality(void* inp1, void* inp2){
    if (inp1 == nullptr || inp2 == nullptr) return false;
    return std::memcmp(inp1, inp2, 4) == 0;  // 4 bytes for 32-bit
}
//------------------------------------------------------------


//------------------------------------------------------------
// ###YOURHASHCODE
template <bool bswap>
static void ###YOURHASHNAMEHash( const void * in, const size_t len, const seed_t seed, void * out ) {
    uint32_t hash = 0;
    PUT_U32<bswap>(hash, (uint8_t *)out, 0);
}
//------------------------------------------------------------


//------------------------------------------------------------
// ####YOUR HASH'S SIMILARITY
// Hamming Similarity: fraction of matching positions (equal-length sequences only)
static double HammingSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t in1_len, const uint32_t in2_len) {
    BIOLSHASHER_ASSERT(in1_len == in2_len, "Hamming similarity requires equal-length sequences.");
    uint32_t similar = 0;
    for (uint32_t i = 0; i < in1_len; i++) {
        if (seq1[i] == seq2[i]) similar++;
    }
    return (static_cast<double>(similar) / static_cast<double>(in1_len));
}

//------------------------------------------------------------
REGISTER_FAMILY(###YOURHASHFAMILYNAME);

 // An instance where the output of the hash is 32 bits.
REGISTER_HASH(###YOURHASHNAME,
   $.desc            = "###YOURHASHDESCRIPTION",
   $.hash_flags      = FLAG_HASH_LOCALITY_SENSITIVE,
   $.impl_flags      = 0,
   $.bits            = 32,
   $.hashfn	         = ###YOURHASHNAMEHash,
   $.parameterNames  = {"x", "y"},
   $.parameterDescriptions  = {"Parameter 1 'x'", "Parameter 2 'y'"},
   $.parameterValues = {PARAMETER_1, PARAMETER_2},
   $.similarity_name = "Hamming",
   $.similarityfn   = HammingSimilarity,
   $.check_equality_fn = check_equality
 );
