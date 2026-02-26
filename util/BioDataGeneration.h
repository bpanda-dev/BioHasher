//
// Created by Bikram Kumar Panda on 23/11/25.
//

// #ifndef SEQUENCEGENERATION_DATAGENERATION_H
// #define SEQUENCEGENERATION_DATAGENERATION_H
//
// Created by Bikram Kumar Panda on 23/11/25.
//

#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <bitset>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <set>
#include "LSHGlobals.h"

//Not used
#define two_bit_per_base 2 // 2 bits per base for A,C,G,T

static constexpr char bases[4] = {'A', 'C', 'T', 'G'};
static constexpr size_t NUM_BASES = std::size(bases);

typedef uint64_t  seed_t; // Seed type for random number generators

struct UnionBitVectorsStruct {
    std::vector<char> vec_a;
    std::vector<char> vec_b;
    std::vector<std::string> universe;
};

// Store k-mers with their positions for nearest neighbour lookups
struct KmerEntry {
	std::string kmer;
	uint32_t position;  // Start position in the reference sequence
};

struct NearestKmerEntry {
	std::string kmer;
	uint32_t position;     // Position in the reference sequence
	double similarity;     // Similarity to the query
};

double ComputeHammingSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t length);
double ComputeJaccardSimilarity(const std::string& seq1, const std::string& seq2, int k);
double ComputeCosineSimilarity(const std::string& seq1, const std::string& seq2, int k);
double ComputeAngularSimilarity(const std::string& seq1, const std::string& seq2, int k);
double ComputeEditSimilarity(const std::string& seq1, const std::string& seq2);


UnionBitVectorsStruct CreateUnionBitVectors(const std::string& seq1, const std::string& seq2, int k);

struct SequenceRecordUnit{
	uint32_t OriginalLength;
	uint32_t MutatedLength;

	std::string SeqASCIIOrg;
	std::string SeqASCIIMut;

	std::vector<uint8_t> SeqTwoBitOrg;	//Not used
	std::vector<uint8_t> SeqTwoBitMut;	//Not used

	double similarity;

	uint32_t LocalAlignmentScore;
	uint32_t GlobalAlignmentScore;

	std::string LocalCIGARString;
	std::string GlobalCIGARString;

	// Mutation Parameters
    double foundationalParameter;
	double snpRate;
	double delRate;
	double stayRate;
	double insmean;
	double insRate; //For 1 length.
};

struct sim_bins_struct{
	std::vector<uint32_t> bin_fill_count = std::vector<uint32_t>(g_bincount_full, 0); // Count of entries in each bin
	std::vector<std::vector<double>> bin_error_parameters = std::vector<std::vector<double>>(100, std::vector<double>(g_bincount_full));
	std::vector<double> bin_error_parameters_mean = std::vector<double>(100, 0);
	std::vector<double> bin_error_parameters_stddev = std::vector<double>(100, 0);
};

struct SequenceRecordsWithMetadataStruct{
	uint32_t KeyCount;                    // Number of sequence pairs
    uint32_t OriginalSequenceLength;      // Original length (before mutations)
	uint32_t DistanceClass;			// Distance class for the sequences (Hamming, Jaccard, etc.)
	
	seed_t DatagenSeed;                          // For reproducibility, we have base seed
	seed_t DataMutateSeed;                        // For reproducibility, we have mutation seed
	seed_t HashSeed;		// Seed for hash family generation
	
	uint32_t HashCount;		// Number of hash functions to be used for LSH

	// Content of sequences
	bool isBasesDrawnFromUniformDist;
	double A_percentage;	// Percentage of A bases in the original sequence
	double C_percentage;	// Percentage of C bases in the original sequence
	double G_percentage;	// Percentage of G bases in the original sequence
	double T_percentage;	// Percentage of T bases in the original sequence

	float binsize = 0.01f;	// Bin size for distance metrics
	float binstart = 0.0f;	// Bin start
	float binend = 1.0f;		// Bin end
	uint32_t bincount = 100;	// Number of bins	// This is similarity bins

	// Array of sequence records (AoS)
    std::vector<SequenceRecordUnit> Records;

	SequenceRecordsWithMetadataStruct() : KeyCount(0), OriginalSequenceLength(0),DistanceClass(0), DatagenSeed(0), DataMutateSeed(0),
		A_percentage(0.25), C_percentage(0.25), G_percentage(0.25), T_percentage(0.25),
		binsize(0.01f), binstart(0.0f), binend(1.0f), bincount(100) {}
};


class SeedGenerator {
private:
    std::mt19937_64 rng;  // 64-bit Mersenne Twister
    std::uniform_int_distribution<seed_t> dist;

public:
    explicit SeedGenerator(seed_t baseSeed) : rng(baseSeed), dist(0, UINT64_MAX) {}

    seed_t nextSeed() {
        return dist(rng);
    }
};

class Randbin {
private:
    std::mt19937 rng;  // Mersenne Twister 19937 generator
    std::uniform_int_distribution<uint32_t> dist;

public:
    explicit Randbin(seed_t seed) : rng(seed), dist(0, UINT32_MAX) {}

    // Generate a random number in the range [0, range)
    uint32_t rand_range(uint32_t range) {
        return dist(rng) % range;
    }
	
	// Generate a uniform random double in [low, high)
    double rand_custom_range(double low, double high) {
        std::uniform_real_distribution<double> real_dist(low, high);
        return real_dist(rng);
    }
};


class SequenceDataGenerator {
	public:
		// Constructor to initialise and generate random sequences.
		SequenceDataGenerator(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata);
		
		
		static void decodeSequencesToASCII(const std::vector<uint8_t>& seqTwoBit, std::string& seqASCII, uint32_t sequenceLength);
};

class SequenceDataMutatorSubstitutionOnly{
	private:
		// uint8_t GetBaseAtPosition(const std::vector<uint8_t>& seq, uint32_t pos);	// Helper: Get base at position
		// void SetBaseAtPosition(std::vector<uint8_t>& seq, uint32_t pos, uint8_t base);	// Helper: Set base at position
		// bool simulateSNP(SequenceRecordUnit &record, const uint32_t pos, std::mt19937 &randGen);
	public:
		SequenceDataMutatorSubstitutionOnly(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, std::vector<double> *rand_error_param);
		SequenceDataMutatorSubstitutionOnly(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata);
};

class SequenceDataMutatorGeometric{
	private:
		// uint8_t GetBaseAtPosition(const std::vector<uint8_t>& seq, uint32_t pos);	// Helper: Get base at position
		// void SetBaseAtPosition(std::vector<uint8_t>& seq, uint32_t pos, uint8_t base);	// Helper: Set base at position
		// bool simulateSNP(SequenceRecordUnit &record, const uint32_t pos, std::mt19937 &randGen);
	public:
		SequenceDataMutatorGeometric(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, std::vector<double> *rand_error_param);
		SequenceDataMutatorGeometric(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata);
};



// template <typename hashtype>
// class HashTableIndex{
// 	private:
// 		// Key: The LSH Hash Value (uint64_t)
// 		// Value: Vector of starting positions in string S
// 		std::unordered_map<uint64_t, std::vector<size_t>> hashTable;
	
// 		HashFn hash;
// 		seed_t hash_seed;
	
// 		int windowSize;
	
// 	public:
// 		HashTableIndex(int windowSize, const HashInfo * hinfo): 
// 			windowSize(windowSize), 
// 			hash(hinfo->hashFn(g_hashEndian)), 
// 			hash_seed(hinfo->Seed(baseSeed)){}

// 		void buildIndex(const std::string& S, seed_t seed) {
// 			if (S.length() < (size_t)windowSize) return;

// 			for (size_t i = 0; i <= S.length() - windowSize; ++i) {
// 				std::string kmer = S.substr(i, windowSize);
// 				hashtype hash_val;
// 				hash(kmer.c_str(), kmer.length(), seed, &hash_val);
// 				hashTable[hash_val].push_back(i);
// 			}
// 		}

// 		const std::vector<size_t>* getNeighbors(const std::string& queryKmer, seed_t seed) {
// 			hashtype hash_val;
// 			hash(queryKmer.c_str(), queryKmer.length(), seed, &hash_val);
// 			auto it = hashTable.find(hash_val);
// 			if (it != hashTable.end()) {
// 				return &(it->second);
// 			}
// 			return new std::vector<size_t>(); // Return an empty vector instead of nullptr
// 		}

// 		void printBucketStats() const {
// 			std::cout << "--- Hash Table Stats ---" << std::endl;
// 			std::cout << "Number of unique LSH buckets: " << hashTable.size() << std::endl;
// 		}

// 		void printHashTable() const {
// 			std::cout << "--- Hash Table Contents ---" << std::endl;
// 			for (const auto& pair : hashTable) {
// 				uint64_t hash = pair.first;
// 				const std::vector<size_t>& positions = pair.second;

// 				std::cout << "Hash: " << hash << " -> Positions: [";
// 				for (size_t i = 0; i < positions.size(); ++i) {
// 					std::cout << positions[i];
// 					if (i < positions.size() - 1) {
// 						std::cout << ", ";
// 					}
// 				}
// 				std::cout << "]" << std::endl;
// 			}
// 		}
// }


// class SequenceDataMutator {
// 	private:
// 		std::mt19937 RandGen;
		
// 		uint8_t GetBaseAtPosition(const std::vector<uint8_t>& seq, uint32_t pos);	// Helper: Get base at position
// 		void SetBaseAtPosition(std::vector<uint8_t>& seq, uint32_t pos, uint8_t base);	// Helper: Set base at position

// 		bool simulateSNP(SequenceRecordUnit &record, const uint32_t pos, std::mt19937 &randGen);
		
// 		bool simulateSmallIndel(SequenceRecordUnit &record, uint32_t pos, std::mt19937 &randGen, SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata);
// 		void simulateDeletion(SequenceRecordUnit &record, uint32_t pos, uint32_t deleteSize);
// 		void simulateInsertion(SequenceRecordUnit &record, uint32_t pos, uint32_t insertSize, std::mt19937 &randGen);

// 		// void simulateStructuralVariation(SequenceRecordUnit &record, const uint32_t pos, std::mt19937 &randGen);

// 	public:
// 		SequenceDataMutator(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata);

// 		void ApplyMutations(SequenceRecordsWithMetadataStruct *sequence_records_with_metadata);
// };

// class SequenceDataMetrics {
// public:
	
// 	SequenceDataMetrics(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata);

// 	// Hamming distance (ideally only for equal-length sequences, but if the sequence length differs, we can define it as the distance over the length of the shorter sequence)
// 	void ComputeHammingMetrics(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata);
	
// 	// Jaccard similarity using k-mers
// 	void ComputeJaccardMetrics(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, uint32_t tokenSize);
	
// 	// Edit distance (Levenshtein distance)
// 	uint32_t computeEditDistance(const std::string& seq1, const std::string& seq2);	//Helper for edit distance
// 	void ComputeEditDistanceMetrics(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata);
// 	// Global alignment (Needleman-Wunsch)
// 	int computeGlobalAlignment(const std::string& seq1, const std::string& seq2, int match, int mismatch, int gap); // Helper for global alignment
// 	void ComputeGlobalAlignmentMetrics(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, int match = 1, int mismatch = -1, int gap = -2);
	
// 	// Local alignment (Smith-Waterman)
// 	uint32_t computeLocalAlignment(const std::string& seq1, const std::string& seq2, int match, int mismatch, int gap);
// 	void ComputeLocalAlignmentMetrics(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, int match = 2, int mismatch = -1, int gap = -2);
	
// 	std::vector<BinStatisticsUnit> BinRecords(SequenceRecordsWithMetadataStruct *sequenceRecordsWithMetadata, uint32_t metric, double binStart = 0.0, double binEnd = 1.0, double binSize = 0.1);

// 	void printBinStatistics(const std::vector<BinStatisticsUnit>& bins, uint32_t metric);

// };




// class SequenceDataAnalyzer {
// public:

	

//     // // Export bins to file
//     // static void exportBinsToFile(
//     //     const std::vector<BinStatistics>& bins,
//     //     const std::string& filename
//     // );
    
//     // // Export all values to file (for histogram plotting)
//     // static void exportValuesToFile(
//     //     SequenceRecordsWithMetadata* sequenceRecordsWithMetadata,
//     //     const std::string& filename,
//     //     const std::string& metricName // = "NormalizedHammingDistance"
//     // );
    
//     // // Print bin statistics
    
// };

// #endif //SEQUENCEGENERATION_DATAGENERATION_H
