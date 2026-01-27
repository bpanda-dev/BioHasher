#include "LSHGlobals.h"
#include "Platform.h"
#include "Hashinfo.h"

/*
Currently the num_signatures variable is not used anywhere. It can be useful in future
if we want to test different number of signatures in Minhash or other LSH functions.
*/


// Global variables for runtime communication
const uint32_t g_bincount_full = 4000;

uint32_t g_TokenLength = 0;     // Default token/kmer length
// uint32_t gNumSignatures = 32;   // Default number of signatures	
bool g_IsTestActive = false;     // Whether test is currently running

const uint32_t g_GoldenRatio = 0x9e3779b1;	


uint32_t g_mutation_model = MUTATION_MODEL_SIMPLE_SNP_ONLY; // MUTATION_MODEL_SIMPLE_SNP_ONLY; // 0: Simple SNP only, 1: Geometric Mutator   // Change the mutation model here.
const uint32_t g_mutation_expression_type = MUTATION_EXPRESSION_DEL_EQUAL_SUB;
const double g_InsertionMean = 0.05;

const uint32_t g_subseqHash1_subseq_len = 12; // Default subsequence length for SubseqHash1
const uint32_t g_subseqHash1_d = 11;           // Default 'p' value for SubseqHash1

// Setter functions (called by LSHCollision test)
void SetTokenLength(uint32_t length) {
    g_TokenLength = length;
}

// void SetNumSignatures(uint32_t num) {
//     gNumSignatures = num;
// }

void SetIsTestActive(bool active) {
    g_IsTestActive = active;
}

// Getter functions (called by hash functions)
uint32_t GetTokenLength() {
    return g_TokenLength;
}

// uint32_t GetNumSignatures() {
//     return g_NumSignatures;
// }

bool IsTestActive() {
    return g_IsTestActive;
}



double mutation_expression(double P_sub, uint32_t expression_type){
    // Different mutation expressions can be implemented here.
    // For now, we implement a simple linear relationship.
    
    if(expression_type == MUTATION_EXPRESSION_DEL_EQUAL_SUB){	// Linear: P_del = P_sub
        return P_sub;
    }
    else if(expression_type == MUTATION_EXPRESSION_DEL_HALF_SUB){	// Quadratic: P_del = (P_sub)/2
        return P_sub * 0.05;
    }
    else if(expression_type == MUTATION_EXPRESSION_DEL_DOUBLE_SUB){	// Quadratic: P_del = (P_sub)*2
        return P_sub * 2.0;
    }
    else if(expression_type == MUTATION_EXPRESSION_DEL_ZERO){	// Quadratic: P_del = 0
        return 0.0;
    }
    else{
        return 0.0;
    }
}


bool is_valid_mutation_parameters(double P_sub, double P_del){
    
    if(P_sub >= 0.0  && P_del < 1.0 && (P_sub + P_del) < 1.0){
        return true;
    }
    else{
        return false;
    }
}


uint32_t setDistanceClassForHashInfo(const uint64_t hash_flags) {
	// Determine the distance class based on the hash function's properties
	if (hash_flags & FLAG_HASH_HAMMING_SIMILARITY) {
		return 1U; // Hamming distance
	} 
	else if (hash_flags & FLAG_HASH_JACCARD_SIMILARITY) {
		return 2U; // Jaccard distance
	}
	else if(hash_flags & FLAG_HASH_COSINE_SIMILARITY){
		return 3U; // Cosine similarity
	}
	else if(hash_flags & FLAG_HASH_ANGULAR_SIMILARITY){
		return 4U; // angular similarity
	}
	else if(hash_flags & FLAG_HASH_EDIT_SIMILARITY){
		return 5U; // Edit similarity
	}
	else {
		return 0U; // Default or unknown or distance not supported.
	}
}