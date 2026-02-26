#include "LSHGlobals.h"
#include "Platform.h"
#include "Hashinfo.h"

/*
Currently the num_signatures variable is not used anywhere. It can be useful in future
if we want to test different number of signatures in Minhash or other LSH functions.
*/


// Global variables for runtime communication
const uint32_t g_bincount_full = 1000;

uint32_t g_TokenLength = 0;     // Default token/kmer length

uint32_t g_window_length = 0;		// Default window length for winnowing (if applicable)


// uint32_t gNumSignatures = 32;   // Default number of signatures	
bool g_IsTestActive = false;     // Whether test is currently running

const uint32_t g_GoldenRatio = 0x9e3779b1;	

// Change to MUTATION_MODEL_SIMPLE_SNP_ONLY (0) or MUTATION_MODEL_GEOMETRIC_MUTATOR (1)
uint32_t g_mutation_model = MUTATION_MODEL_GEOMETRIC_MUTATOR; //MUTATION_MODEL_SIMPLE_SNP_ONLY;//MUTATION_MODEL_GEOMETRIC_MUTATOR; // MUTATION_MODEL_SIMPLE_SNP_ONLY; // 0: Simple SNP only, 1: Geometric Mutator   // Change the mutation model here.
const uint32_t g_mutation_expression_type = MUTATION_EXPRESSION_BALANCED;//MUTATION_EXPRESSION_BALANCED;	// Change the mutation expression type here as needed.

// Setter functions (called by LSHCollision test)
void SetTokenLength(uint32_t length) {
    g_TokenLength = length;
}

void SetWindowLength(uint32_t length) {
    g_window_length = length;
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

uint32_t GetWindowLength() {
    return g_window_length;
}

// uint32_t GetNumSignatures() {
//     return g_NumSignatures;
// }

bool IsTestActive() {
    return g_IsTestActive;
}



void mutation_expression(double g_mean, uint32_t expression_type, double *P_sub_out, double *P_del_out){
    // Different mutation expressions can be implemented here.
    
    double insertion_of_len_1 = (1-(1/(1+g_mean)))*(1/(1+g_mean));  // Probability of insertion being of length 1 drawn from a geometric distribution with mean g_mean.

    if(expression_type == MUTATION_EXPRESSION_BALANCED){
        // Balanced expression: P_del = P_sub = Pr[insertion of length 1]
        *P_sub_out = insertion_of_len_1;
        *P_del_out = insertion_of_len_1;
    }
    else if(expression_type == MUTATION_EXPRESSION_SUB_ONLY){
        // SUB ONLY expression: P_del = 0, P_sub = Pr[insertion of length 1]
        *P_sub_out = insertion_of_len_1;
        *P_del_out = 0.0;
    }
    else if(expression_type == MUTATION_EXPRESSION_DEL_LITE){	
        // Deletion lite: P_del = (P_sub)/5, P_sub = Pr[insertion of length 1]
        *P_sub_out = insertion_of_len_1;
        *P_del_out = insertion_of_len_1 / 2.0;
    }
    else if(expression_type == MUTATION_EXPRESSION_INS_LITE){
        // INS lite expression: P_del = P_sub = Pr[insertion of length 1] * 5
        *P_sub_out = insertion_of_len_1*2.0;
        *P_del_out = insertion_of_len_1*2.0;
    }
    else if(expression_type == MUTATION_EXPRESSION_SUB_LITE){
        // SUB lite expression: P_del = Pr[insertion of length 1] , P_sub = Pr[insertion of length 1]/5
        *P_sub_out = insertion_of_len_1/2.0;
        *P_del_out = insertion_of_len_1;
    }
    else{
        // Default to balanced if unknown expression type is provided.
        // Balanced expression: P_del = P_sub = Pr[insertion of length 1]
        *P_sub_out = insertion_of_len_1;
        *P_del_out = insertion_of_len_1;
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