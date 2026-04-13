//
// Created by Bikram Kumar Panda on 13/04/26.
//

#include "similarities.h"


double HammingSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t in1_len, const uint32_t in2_len) {
    // Hamming distance only defined for equal-length sequences
    assert(in1_len == in2_len && "For hamming, we need both the sequences to be of same length.");

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

double ComputeJaccardSimilarity(const std::string& seq1, const std::string& seq2, int k) {
    // 1. Tokenize sequences into sets of k-mers
    std::set<std::string> set1 = get_kmers(seq1, k);
    std::set<std::string> set2 = get_kmers(seq2, k);

	// printf("Computing Jaccard Similarity for k=%d\n", k);

	// printf("Set1 size: %zu, Set2 size: %zu\n", set1.size(), set2.size());
	// //print the sets for debugging
	// // --- Printing Section ---
    // std::cout << "--- K-mer Analysis (k=" << k << ") ---" << std::endl;
    // std::cout << "Set1 (" << set1.size() << " unique): ";
    // for (const auto& kmer : set1) std::cout << kmer << " ";

    // std::cout << "\nSet2 (" << set2.size() << " unique): ";
    // for (const auto& kmer : set2) std::cout << kmer << " ";
    // std::cout << "\n" << std::endl;


    if (set1.empty() || set2.empty()) return 0.0;

    // 2. Find the intersection
    std::vector<std::string> intersect;
    std::set_intersection(set1.begin(), set1.end(),
                          set2.begin(), set2.end(),
                          std::back_inserter(intersect));

    // 3. Use the Inclusion-Exclusion principle for the Union size
    // |A ∪ B| = |A| + |B| - |A ∩ B|
    size_t intersection_size = intersect.size();
    size_t union_size = set1.size() + set2.size() - intersection_size;

    return static_cast<double>(intersection_size) / union_size;
}

double ComputeCosineSimilarity(const std::string& seq1, const std::string& seq2, int k) {
	// 1. Tokenize sequences into sets of k-mers
	std::set<std::string> set1 = get_kmers(seq1, k);
	std::set<std::string> set2 = get_kmers(seq2, k);

	if (set1.empty() || set2.empty()) return 0.0;

	// 2. Find the intersection
	std::vector<std::string> intersect;
	std::set_intersection(set1.begin(), set1.end(),
						  set2.begin(), set2.end(),
						  std::back_inserter(intersect));

	size_t intersection_size = intersect.size();

	// Cosine similarity = |A ∩ B| / sqrt(|A| * |B|)
	return static_cast<double>(intersection_size) /
		   std::sqrt(static_cast<double>(set1.size()) * static_cast<double>(set2.size()));
}

double ComputeAngularSimilarity(const std::string& seq1, const std::string& seq2, int k) {
	double cosine_sim = ComputeCosineSimilarity(seq1, seq2, k);
	// Angular similarity = 1 - (angle / π) where angle = arccos(cosine_similarity)
	return 1 - (std::acos(cosine_sim) / M_PI);
}

UnionBitVectorsStruct CreateUnionBitVectors(const std::string& seq1, const std::string& seq2, int k) {
	// Tokenize sequences into sets of k-mers
	std::set<std::string> kmer_set_a = get_kmers(seq1, k);
	std::set<std::string> kmer_set_b = get_kmers(seq2, k);

	if (kmer_set_a.empty() || kmer_set_b.empty()) return {std::vector<char>(), std::vector<char>(), std::vector<std::string>()};	//return empty vectors if either set is empty

    // 1. Create the Universe (Union of both sets)
    // std::set_union requires sorted ranges. std::set is already sorted.
    std::vector<std::string> union_kmers;
    std::set_union(kmer_set_a.begin(), kmer_set_a.end(),
                   kmer_set_b.begin(), kmer_set_b.end(),
                   std::back_inserter(union_kmers));

    // 2. Pre-allocate vectors
    size_t n = union_kmers.size();
	std::vector<char> vec_a(n, '0');
	std::vector<char> vec_b(n, '0');

    // 3. Fill the vectors
    for (size_t i = 0; i < n; ++i) {
        const std::string& kmer = union_kmers[i];

        // Use set::count to check for existence (O(log N))
        if (kmer_set_a.count(kmer)) {
            vec_a[i] = '1';
        }
        if (kmer_set_b.count(kmer)) {
            vec_b[i] = '1';
        }
    }

    return {std::move(vec_a), std::move(vec_b), std::vector<std::string>()};
}

int min1(int x,int y,int z){
	int min_xy = (x<y)? x:y;
    if(min_xy>z)
        return z;
    else
        return min_xy;
}

// double ComputeEditSimilarity(const std::string& seq1, const std::string& seq2) {
//
// 	//write your code here
// 	const int n = seq1.size();
// 	const int m = seq2.size();
//
// 	std::vector<std::vector<int>> D(n + 1, std::vector<int>(m + 1, 0));
//
// 	// Initialize base cases
//     for (int i = 0; i <= n; i++) {
//         D[i][0] = static_cast<int>(i);
//     }
//     for (int j = 0; j <= m; j++) {
//         D[0][j] = static_cast<int>(j);
//     }
//
// 	// Fill DP table
//     for (int i = 1; i <= n; i++) {
//         for (int j = 1; j <= m; j++) {
//             int insert_cost = D[i][j - 1] + 1;
//             int delete_cost = D[i - 1][j] + 1;
//             int match_cost  = D[i - 1][j - 1] + (seq1[i - 1] == seq2[j - 1] ? 0 : 1);
//
//             D[i][j] = std::min({insert_cost, delete_cost, match_cost});
//         }
//     }
//
// 	int edit_dist = D[n][m];
// 	double max_len = static_cast<double>(std::max(n, m));
//
//     return 1.0 - (static_cast<double>(edit_dist) / max_len);
// }
