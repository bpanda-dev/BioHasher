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
#include <algorithm>
#include <limits>
#include <fstream>
#include <iomanip>
#include <cmath>

#define two_bit_per_base 2 // 2 bits per base for A,C,G,T

static constexpr char bases[4] = {'A', 'C', 'T', 'G'};
static constexpr size_t NUM_BASES = std::size(bases);

typedef uint64_t  seed_t; // Seed type for random number generators

struct SequenceRecordUnit{
	uint32_t OriginalLength;
	uint32_t MutatedLength;

	std::string SeqASCIIOrg;
	std::string SeqASCIIMut;

	std::vector<uint8_t> SeqTwoBitOrg;
	std::vector<uint8_t> SeqTwoBitMut;

	// Distance metrics
	float HammingSimilarity;
	float HammingDissimilarity;
	uint32_t HammingDistance;

	float JaccardSimilarity;
	float JaccardDissimilarity;
	
	uint32_t EditDistance;

	uint32_t HashDistance;

	uint32_t LocalAlignmentScore;
	uint32_t GlobalAlignmentScore;

	std::string LocalCIGARString;
	std::string GlobalCIGARString;
};

struct BinStatisticsUnit {
    double binStart;
    double binEnd;
    uint32_t count;
    std::vector<uint32_t> recordIndices;  // Indices of records in this bin
    
    BinStatisticsUnit(double start, double end) 
        : binStart(start), binEnd(end), count(0) {}
};

struct SequenceRecordsWithMetadata{
	uint32_t KeyCount;                    // Number of sequence pairs
    uint32_t OriginalSequenceLength;      // Original length (before mutations)
	uint32_t DistanceClass;			// Distance class for the sequences (Hamming, Jaccard, etc.)
	seed_t DatagenSeed;                          // For reproducibility, we have base seed
	seed_t DataMutateSeed;                        // For reproducibility, we have mutation seed

	uint32_t HashCount;		// Number of hash functions to be used for LSH

	// Content of sequences
	double A_percentage;	// Percentage of A bases in the original sequence
	double C_percentage;	// Percentage of C bases in the original sequence
	double G_percentage;	// Percentage of G bases in the original sequence
	double T_percentage;	// Percentage of T bases in the original sequence

	// Mutation Parameters
	// Per-base probability for SNPs and small-scale indels.
    double snpRate;
    double smallIndelRate;

    // Minimal and maximal size for small indels.  Indels will be simulated uniformly in this range.  The range is
    // stored internally as [min, max) but given as [min, max] from the command line.
    int minSmallIndelSize;
    int maxSmallIndelSize;

    // Per-base probability for having a structural variation.
    double svIndelRate;
    double svInversionRate;
    double svTranslocationRate;
    double svDuplicationRate;

    // Minimal and maximal size for structural variations.  SVs will be simulated uniformly in this range.  The range is
    // stored internally as [min, max) but given as [min, max] from the command line.
    int minSVSize;
    int maxSVSize;

	float binsize;	// Bin size for distance metrics
	float binstart;	// Bin start
	float binend;		// Bin end
	uint32_t bincount;

	// Array of sequence records (AoS)
    std::vector<SequenceRecordUnit> Records;

	SequenceRecordsWithMetadata() : KeyCount(0), OriginalSequenceLength(0),DistanceClass(0), DatagenSeed(0), DataMutateSeed(0),
		A_percentage(0.25), C_percentage(0.25), G_percentage(0.25), T_percentage(0.25),
		snpRate(0.0), smallIndelRate(0.0), minSmallIndelSize(1), maxSmallIndelSize(1),
		svIndelRate(0.0), svInversionRate(0.0), svTranslocationRate(0.0), svDuplicationRate(0.0),
		minSVSize(50), maxSVSize(100), binsize(0.1f), binstart(0.0f), binend(1.0f), bincount(10) {}
};

class SequenceDataGenerator {
	// Please note that this class requires that the sequencelength be a multiple of 4, so that there is no padding in the sequence storage.

	private:
		std::mt19937 RandGen;
	public:
		// Constructor to initialize data generation parameters.
		SequenceDataGenerator(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata);

		// Filling the records.
		void FillRecords(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata);

		static void decodeSequencesToASCII(const std::vector<uint8_t>& seqTwoBit, std::string& seqASCII, uint32_t sequenceLength);

		void generateWithDistribution(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata);
};

class SequenceDataMutator {
	private:
		std::mt19937 RandGen;
		
		uint8_t GetBaseAtPosition(const std::vector<uint8_t>& seq, uint32_t pos);	// Helper: Get base at position
		void SetBaseAtPosition(std::vector<uint8_t>& seq, uint32_t pos, uint8_t base);	// Helper: Set base at position

		bool simulateSNP(SequenceRecordUnit &record, const uint32_t pos, std::mt19937 &randGen);
		
		bool simulateSmallIndel(SequenceRecordUnit &record, uint32_t pos, std::mt19937 &randGen, SequenceRecordsWithMetadata *sequenceRecordsWithMetadata);
		void simulateDeletion(SequenceRecordUnit &record, uint32_t pos, uint32_t deleteSize);
		void simulateInsertion(SequenceRecordUnit &record, uint32_t pos, uint32_t insertSize, std::mt19937 &randGen);

		// void simulateStructuralVariation(SequenceRecordUnit &record, const uint32_t pos, std::mt19937 &randGen);

	public:
		SequenceDataMutator(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata);

		void ApplyMutations(SequenceRecordsWithMetadata *sequence_records_with_metadata);
};

class SequenceDataMetrics {
public:
	
	SequenceDataMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata);

	// Hamming distance (ideally only for equal-length sequences, but if the sequence length differs, we can define it as the distance over the length of the shorter sequence)
	void ComputeHammingMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata);
	
	// Jaccard similarity using k-mers
	void ComputeJaccardMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata, uint32_t tokenSize);
	
	// Edit distance (Levenshtein distance)
	uint32_t computeEditDistance(const std::string& seq1, const std::string& seq2);	//Helper for edit distance
	void ComputeEditDistanceMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata);

	// Global alignment (Needleman-Wunsch)
	int computeGlobalAlignment(const std::string& seq1, const std::string& seq2, int match, int mismatch, int gap); // Helper for global alignment
	void ComputeGlobalAlignmentMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata, int match = 1, int mismatch = -1, int gap = -2);
	
	// Local alignment (Smith-Waterman)
	uint32_t computeLocalAlignment(const std::string& seq1, const std::string& seq2, int match, int mismatch, int gap);
	void ComputeLocalAlignmentMetrics(SequenceRecordsWithMetadata *sequenceRecordsWithMetadata, int match = 2, int mismatch = -1, int gap = -2);
	
	std::vector<BinStatisticsUnit> BinRecords(SequenceRecordsWithMetadata* sequenceRecordsWithMetadata, uint32_t metric, double binStart = 0.0, double binEnd = 1.0, double binSize = 0.1);

	void printBinStatistics(const std::vector<BinStatisticsUnit>& bins, uint32_t metric);

};




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
