# Approximate Nearest Neighbour (ANN) Test

This test evaluates a hash function as an LSH index for nearest-neighbour search — the actual end-to-end use case we care about. Instead of just checking collision probabilities, it builds a full LSH index, queries it with mutated sequences, and measures Recall vs FPR against brute-force ground truth using the **c-ANN** formulation.

```bash
./BioLSHasher --test=LSHApproxNearestNeighbour SubSeqHash-64
```

Output goes to `results/ApproxNearestNeighbourResults_<hashname>.csv`.

---

## How It Works

![c-ANN concept.png](c-ANN%20concept.png)

The test is a 5-phase pipeline:

### 1. Reference Database
Generates `g_Nseq_in_Database` (default: 10k) random genomic sequences. Sequence length is determined by the hash's `onlyShortSequenceLength()` flag — if set, uses `g_ShortSequenceLength` (45bp), otherwise `g_LongSequenceLength` (256bp). Duplicates are tracked via a position map so ground truth stays correct.

### 2. Query Generation
Samples `g_numQueriesForApproxNNTest` (default: 100) sequences from the reference DB, then mutates each one to produce a query with ~90–100% similarity to its source. The target similarity range is controlled by `target_sim_low` / `target_sim_high` in the test source.

### 3. Aggregation Binning
A calibration step that maps "desired similarity" → "required error parameter" for the current mutation model. It runs `g_NAggCases_ANN` (default: 500k) random mutations, bins results by measured similarity, and builds a lookup table. This decouples the similarity you *want* from the error rate you need to *set* — which varies by mutation model and sequence length.

### 4. Ground Truth (Brute Force, c-ANN)
For each mutated query, computes similarity against **every** reference sequence using the distance metric matching the hash's similarity flag (Hamming, Jaccard, Cosine, Angular, or Edit).

Instead of picking a fixed top-K, the test uses a **c-ANN** approach: it stores all reference entries with similarity ≥ `min(c_values) * target_sim_low`. At evaluation time for each `c` value, the true neighbour set is all entries with `similarity ≥ c * target_sim_low`. This lets us evaluate how the index performs across different approximation tolerances.

> This is O(Q × N) and single-threaded. Fine with defaults, slow at scale.

### 5. LSH Index & Evaluation
For each `(b, r, c)` triple — where `(b, r)` comes from the configured grid and `c` from `g_cANN_c_values` — the following is repeated `g_ANN_runs_for_avg` times:

1. **Build** — Creates `r` hash tables, each using `b` independently-seeded hash functions. Band signatures are combined via a golden-ratio hash combiner (not naive AND).
2. **Query** — For each mutated query, looks up candidates across all `r` tables and deduplicates (OR across tables).
3. **Evaluate** — For the given `c`, builds the true-neighbour set (all refs with `sim ≥ c * target_sim_low`), then computes Recall, Precision, FPR, and F1 against the LSH candidate set. Metrics are macro-averaged across queries, then averaged across runs.

---

## Key Parameters

Most parameters live in [`lib/LSHGlobals.cpp`](../lib/LSHGlobals.cpp) : Please rebuild after changing.

| Parameter | What it does | Default |
|-----------|-------------|---------|
| `g_ShortSequenceLength` | Sequence length for short-sequence hashes | 45 |
| `g_LongSequenceLength` | Sequence length for long-sequence hashes | 256 |
| `g_Nseq_in_Database` | Reference DB size | 10,000 |
| `g_numQueriesForApproxNNTest` | Number of query sequences | 100 |
| `g_NAggCases_ANN` | Aggregation-phase sample count (for bin calibration) | 500,000 |
| `g_ANN_runs_for_avg` | Runs to average metrics over | 3 |
| `g_ANN_start_B` / `g_ANN_MAX_B` | `b` (AND) range | 1–5 |
| `g_ANN_start_R` / `g_ANN_MAX_R` | `r` (OR) range | 1–7 |
| `g_cANN_c_values` | c-ANN approximation factors to sweep | {0.5, 0.6, 0.7, 0.8, 0.9, 0.95} |
| `g_mutation_model` | Mutation model (0=SNP-only, 1=Geometric) | 1 |

A couple of things are hardcoded in [`tests/ApproxNearestNeighbour.cpp`](../tests/ApproxNearestNeighbour.cpp):

| Parameter | What it does | Default |
|-----------|-------------|---------|
| `target_sim_low` / `target_sim_high` | Similarity range for mutations | 0.90 – 1.0 |

---

## Things to Know

- **Geometric mutations produce indels which break Hamming distance** : If a hash has hamming distance, please ensure that the mutation model only supports substitution mutation.

- **Short sequences can produce duplicates** — At short lengths or with skewed base distributions, the reference DB may have duplicate sequences. The test handles this, but effective DB size < `g_Nseq_in_Database`.

- **Band signatures are 64-bit** — If your hash outputs fewer bits (e.g., 32-bit), upper bits will be zero. Functionally correct but may affect bucket distribution. (Known issue, needs fixing.)

- **c-ANN approximation factor** — Lower `c` values define a more relaxed neighbourhood (more true neighbours), which generally increases recall but makes precision harder to interpret. The default sweep covers 0.5 to 0.95.

---

## Plotting

```bash
pip install matplotlib numpy adjustText
python analysis/plot_ANN.py results/ApproxNearestNeighbourResults_<hashname>.csv
```

Generates FPR-vs-Recall plots, Pareto-optimal configs, and full scatter plots (linear + log scale variants).

---

For the full technical deep-dive (aggregation bin internals, band signature math, output format spec), see the detailed reference in `docs/ANNtest.md`.
