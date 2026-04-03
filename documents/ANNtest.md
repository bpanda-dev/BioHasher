# Approximate Nearest Neighbour (ANN) Test — Detailed Reference

This document provides an in-depth explanation of the `LSHApproxNearestNeighbour` test, how it works internally, all configurable parameters, and important caveats.

**Source:** [`tests/ApproxNearestNeighbourTest.cpp`](../tests/ApproxNearestNeighbourTest.cpp)

---

## Purpose

The ANN test evaluates a hash function as an *LSH index* for nearest-neighbour search which is the end-to-end use case most genomic LSH pipelines care about. Rather than measuring collision probabilities in isolation (as the Collision tests do), it builds a full LSH index, queries it with mutated sequences, and compares retrieved candidates against brute-force ground truth.

The key output: for each `(b, r)` configuration, how well does the hash trade off **Recall** vs **FPR**?

```bash
./SMHasher3 --test=LSHApproxNearestNeighbour SubSeqHash-64 --ncpu=16
```

**Output:** `results/ApproxNearestNeighbourResults_<hashname>.csv`

---

## How It Works (5-Phase Pipeline)

```
Phase 1          Phase 2            Phase 3          Phase 4              Phase 5
Reference  →  Query Sampling  →  Aggregation  →  Ground Truth  →  LSH Index Build
Generation     & Mutation          Binning       (brute-force)     Query & Evaluate
                                                                  (repeat per (b,r))
```

### Phase 1 — Reference Database Generation

Generates `g_Nseq_in_Database` random genomic sequences (uniform base distribution) of length `g_sequenceLength_ANN`. These form the "database" that the LSH index will be populated with.

The sequences are also de-duplicated into an `unordered_map<string, vector<uint32_t>>` mapping each unique sequence to all its positions. This is important because:
- At short sequence lengths duplicates are possible. 
- When there is a strong skew in the probability distribution of nucleotides, repeated sequences are more likely to occur.
- The ground truth phase uses position-based matching, so duplicate handling ensures correctness.

### Phase 2 — Query Sampling & Mutation

1. **Sampling:** Randomly picks `g_numQueriesForApproxNNTest` sequences from the reference database (with replacement, using a seeded RNG).

2. **Error parameter assignment:** For each query, draws a random similarity bin from Phase 3's lookup table (targeting bins between `target_sim_low` (0.90) and `target_sim_high` (1.0)), then samples an error parameter from that bin.

3. **Mutation:** Applies the configured mutation model (`g_mutation_model` + `g_mutation_expression_type`) to produce a mutated version of each query. The result: each query's mutated sequence has ~90–100% similarity to its original reference sequence.

### Phase 3 — Aggregation Binning

This phase builds the error-parameter lookup table that Phase 2 needs. It runs *before* query mutation:

1. Generates `g_norm_N_agg_cases` (default: 500,000) random sequences and mutates them with uniformly-drawn error rates.
2. Sorts results into 100 similarity bins (0.00–1.00) based on the measured similarity after mutation.
3. For each bin, records:
   - The error parameters that produced sequences falling in that bin
   - Mean and stddev of those error parameters
4. **Partially-filled bins** (fewer than `g_bincount_full` = 1,000 entries) are topped up with Gaussian samples drawn from the bin's mean/stddev. Completely empty bins are left empty.

This calibration step decouples "desired similarity" from "required error parameter", which varies by mutation model and sequence length.

### Phase 4 — Brute-Force Ground Truth

For each mutated query, computes the true similarity against *every* reference sequence and picks the top-`TOP_K` nearest neighbours.

- **Distance metric** is determined automatically by the hash's `FLAG_HASH_*_SIMILARITY` flag:

  | Flag | Metric | Distance Class |
  |------|--------|----------------|
  | `FLAG_HASH_HAMMING_SIMILARITY` | Hamming Similarity | 1 |
  | `FLAG_HASH_JACCARD_SIMILARITY` | Jaccard Similarity | 2 |
  | `FLAG_HASH_COSINE_SIMILARITY` | Cosine Similarity | 3 |
  | `FLAG_HASH_ANGULAR_SIMILARITY` | Angular Similarity | 4 |
  | `FLAG_HASH_EDIT_SIMILARITY` | Edit Similarity | 5 |

- **TOP_K** is currently hardcoded to `1` — each query has exactly one ground-truth nearest neighbour.
- Similarities are sorted in descending order and the top entries are retained.

### Phase 5 — LSH Index Build, Query & Evaluate

For each `(b, r)` pair in the grid `[g_ANN_start_B..g_ANN_MAX_B] × [g_ANN_start_R..g_ANN_MAX_R]`, the following is repeated `g_ANN_runs_for_avg` times (with fresh random seeds each run):

#### 5a. Index Construction (`ANN_LSH_Index`)

The `ANN_LSH_Index` class implements the standard banded LSH scheme:

- **`r` hash tables** — each table is an `unordered_map<uint64_t, vector<uint32_t>>` (band signature → list of positions).
- **`b` hash functions per table** — each with an independent seed drawn from `SeedGenerator`.
- **Band signature computation:** For a given sequence and table, the `b` hash outputs are combined into a single `uint64_t` via a golden-ratio hash combiner (similar to `boost::hash_combine`):

  ```cpp
  // For each of b hashes:
  band_signature = seed ^ (hash_val + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
  ```

  This avoids the naive bitwise-AND approach, which biases signatures toward zero.

- **Insert:** Each reference sequence is inserted into all `r` tables by computing its band signature for each table.

#### 5b. Query

For each mutated query:
1. Compute its band signature for each of the `r` tables.
2. Look up matching buckets in each table.
3. Collect all candidate positions across all tables into a deduplicated set (this is the OR operation — a match in *any* table counts).

#### 5c. Evaluation Metrics

For each query, compare the LSH candidate set against the ground-truth set:

| Metric | Formula | Meaning |
|--------|---------|---------|
| **Recall** | TP / (TP + FN) | Fraction of true neighbours found |
| **Precision** | TP / (TP + FP) | Fraction of returned candidates that are true neighbours |
| **FPR** | FP / (N − \|ground truth\|) | Fraction of non-neighbours incorrectly returned |
| **F1** | 2 × (Recall × Precision) / (Recall + Precision) | Harmonic mean of Recall and Precision |

These are **averaged** across all queries for each run, then **averaged across all runs** for the final reported values.

---

## Configurable Parameters

All parameters are set at compile time in [`util/LSHGlobals.cpp`](../util/LSHGlobals.cpp). Rebuild after changing.

| Variable | Description | Default |
|----------|-------------|---------|
| `g_sequenceLength_ANN` | Length (in bases) of each reference and query sequence | `45` |
| `g_Nseq_in_Database` | Number of sequences in the reference database | `10,000` |
| `g_numQueriesForApproxNNTest` | Number of mutated query sequences | `100` |
| `g_ANN_runs_for_avg` | Independent runs to average metrics over | `3` |
| `g_ANN_start_B` / `g_ANN_MAX_B` | Range of `b` (hashes combined per band) to sweep | `1`–`5` |
| `g_ANN_start_R` / `g_ANN_MAX_R` | Range of `r` (number of independent tables) to sweep | `1`–`7` |
| `g_norm_N_agg_cases` | Aggregation-phase sample count (for bin calibration) | `500,000` |
| `g_tokenLengths_array[0]` | Token/k-mer length (for tokenised hashes only) | `13` |
| `g_mutation_model` | Mutation model (0 = SNP-only, 1 = Geometric) | `1` |
| `g_mutation_expression_type` | Mutation expression variant | `0` (Balanced) |

#### Parameters in the test source (`ApproxNearestNeighbourTest.cpp`)

These require editing the source code directly:

| Variable | Description | Default |
|----------|-------------|---------|
| `TOP_K` | Number of ground-truth nearest neighbours per query | `1` |
| `target_sim_low` | Lower bound of target similarity for query mutation | `0.90` |
| `target_sim_high` | Upper bound of target similarity for query mutation | `1.0` |

---

## Output Format

```
:1: LSH Approx Nearest Neighbour Summary
:2: Hashname, SequenceLength, TokenLength, Distance Metric, Mutation Model, Mutation Expression
:3: <values for above>
:4.1: Hash-specific parameter names
:4.2: Hash-specific parameter descriptions
:4.3: Hash-specific parameter values
:5: b, r, Avg_Recall, Avg_Precision, Avg_FPR, Avg_F1_Score   (column headers)
:6: <one data row per (b,r) pair — comma-separated values>
```

Each `(b, r)` pair produces exactly one `:6:` line. The file appends if it already exists, so results from multiple runs accumulate.

---

## Caveats & Notes

> [!IMPORTANT]
> **Ground truth is brute-force O(Q × N).** Each query is compared against every reference sequence. With the defaults (`Q=100`, `N=10,000`) this takes seconds, but scaling both to large values (e.g., `Q=10,000`, `N=1,000,000`) will be very slow. The ground truth phase is **single-threaded**.

> [!NOTE]
> **TOP_K is hardcoded to 1.** The test currently finds only the single nearest neighbour per query. This means Recall is binary (0 or 1) per query — either the true nearest neighbour was found or it wasn't. To evaluate top-k retrieval quality, modify `TOP_K` in `ApproxNearestNeighbourTest.cpp` and rebuild.

> [!NOTE]
> **Target similarity range is 90–100%.** Queries are mutated to be very similar to their source sequence. This is the regime where LSH-based search is most useful (finding near-duplicates). To test how the index performs on more distant sequences, lower `target_sim_low` in the test source.

> [!WARNING]
> **Band signature uses 64-bit output.** The `compute_band_signature` function always stores hash output into a `uint64_t`. If your hash produces fewer bits (e.g., 32-bit), the upper bits will be zero. This is functionally correct but may affect bucket distribution quality. TODO: This is important, I need to fix this. 

> [!NOTE]
> **Duplicate sequences in the reference.** At short sequence lengths (e.g., 16 bases long), random generation can produce duplicate sequences for a large `g_Nseq_in_Database`(number of sequences in the reference database). The test handles this via `uniqueKmerPositions` mapping, but the effective database size may be smaller than `g_Nseq_in_Database`.

> [!NOTE]
> **Mutation model is auto-overridden for Hamming hashes.** If the hash has `FLAG_HASH_HAMMING_SIMILARITY`, the mutation model is forced to `MUTATION_MODEL_SIMPLE_SNP_ONLY` at runtime, since the geometric mutator produces insertions/deletions that change sequence length — incompatible with Hamming distance.

---

## Plotting Results

See [`analysis/plot_lsh_results.py`](../analysis/plot_lsh_results.py) for visualisation:

```bash
pip install matplotlib numpy adjustText
python analysis/plot_lsh_results.py results/ApproxNearestNeighbourResults_<hashname>.csv
```

Produces 6 plots (3 linear + 3 log-scale):

| Plot | What it shows |
|------|---------------|
| `*_fpr_vs_recall.png` | FPR vs Recall per `b` value, each point annotated with `r` |
| `*_best_fpr_per_recall_bin.png` | Pareto-optimal (min FPR) configuration per recall bin |
| `*_all_points.png` | All `(b,r)` configs as an annotated scatter |

Each plot has a `*_log.png` variant with log-scaled Y-axis for inspecting low-FPR regimes.
