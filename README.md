# BioHasher : Genomic Hash Function Testing Framework

## Overview

BioHasher is a specialized Locality Sensitive Hash function testing framework designed for biological sequence analysis. It is built upon the hash function testing framework SMHasher3[https://gitlab.com/fwojcik/smhasher3]. BioHasher currently supports three tests Collision Curve Test, Collision Curve Test with AND-OR amplification, and Approximate Nearest Neighbour Test.


## Getting Started

To get started with BioHasher, follow these steps:

1. Clone the repository:

    ```bash
    git clone https://github.com/bpanda-dev/BioHasher.git
    ```

2. Build BioHasher:

   ```bash
    cd BioHasher
    mkdir build
    cd build
    cmake ..
    make -j$(nproc)
    # Perform a sample test run to perform collison analysis of an included hash function (OneBaseSamplingHash-32)
    ./SMHasher3 --test=LSHCollision OneBaseSamplingHash-32 --ncpu=16
    ```
   
    If everything works correctly, the above run should generate a file named `collisionResults_OneBaseSamplingHash-32.csv` in the `results` directory under `BioHasher`. This file contains the output of the collision test.

3. To plot the curves from the output csv file,

    ```bash
    python ../analysis/plot_collisioncurves.py ../results/collisionResults_SubseqHash-64.csv
    ```

    <!-- This will generate various versions of collision curves plots in the analysis directory. We explain about the type of plots in section [[?]]. -->

> Add the code for setting up environment.

## Python Analysis Environment

The analysis and plotting scripts (in `analysis/`) require a Python environment with several scientific packages. A conda environment file is provided for reproducibility.

### Prerequisites

- [Miniconda](https://docs.conda.io/en/latest/miniconda.html) or [Anaconda](https://www.anaconda.com/download)

### Setting up the environment

1. **Create the conda environment** from the provided `environment.yaml`:

   ```bash
   conda env create -f environment.yaml
   ```

2. **Activate the environment**:

   ```bash
   conda activate biohasher
   ```

3. **Verify** the installation:

   ```bash
   python -c "import matplotlib; import numpy; import pandas; import scipy; import plotly; import adjustText; print('All packages OK')"
   ```

4. **Deactivate** when done:

   ```bash
   conda deactivate
   ```

### Updating the environment

If `environment.yaml` is modified (e.g., a new dependency is added), update your existing environment with:

```bash
conda env update -f environment.yaml --prune
```


## Usage Guide for Novel Hash function testing

This section walks through the complete workflow of: **adding a new hash function**, **running tests**, and **generating plots** from the results.

---

### Part 1 — Adding a New Hash Function

There are two approaches to add a new hash function to BioHasher: using the **interactive template generator script**, or **manually** from the example files.

#### Option A: Using `createHashTemplate.py` (Recommended)

BioHasher ships with an interactive Python script that scaffolds a new hash `.cpp` file and registers it in the build system automatically. Run it from the repository root:

```bash
python3 createHashTemplate.py
```

The script walks you through **8 guided steps**:

| Step | Prompt          | What it sets                                                       |
| ---- | --------------- | ------------------------------------------------------------------ |
| 1    | Hash Name       | C++ function name, `REGISTER_HASH` identifier, and output filename |
| 2    | Author Name     | Copyright header                                                   |
| 3    | License         | License text in file header (MIT default, 8 options)               |
| 4    | Family Name     | `REGISTER_FAMILY(...)` grouping (defaults to hash name)            |
| 5    | Repository URL  | Source URL in family registration                                  |
| 6    | Source Status   | `SRC_UNKNOWN`, `SRC_FROZEN`, `SRC_STABLEISH`, or `SRC_ACTIVE`      |
| 7    | Description     | Human-readable description in `REGISTER_HASH`                      |
| 8    | Output Bit Size | 32, 64, 128, 256, 512, or custom (multiple allowed)                |

Every input is validated (naming rules, C++ keyword checks, URL format, etc.). The script **never exits on bad input** — it re-prompts until valid input is provided.

**What it generates:**

1. A compilable C++ template at `hashes/<hashname>.cpp` containing:

    - Copyright header with your chosen license
    - A `template <bool bswap>` hash function stub for each selected bit size
    - `REGISTER_FAMILY(...)` and `REGISTER_HASH(...)` macro blocks
    - Correct `PUT_U32` / `PUT_U64` output calls
2. An updated `hashes/Hashsrc.cmake` with the new file registered

> **Full documentation:** See [`createHashTemplate.md`](documents/createHashTemplate.md) for the complete reference including validation rules, example sessions, and troubleshooting.

#### Option B: Manual Creation

Copy one of the example templates and fill in the placeholders:

```bash
cp hashes/EXAMPLE-mit.cpp hashes/myhash.cpp # MIT license template
# or
cp hashes/EXAMPLE.cpp hashes/myhash.cpp # Generic template
```

Then manually:

1. Replace all `###YOUR...` placeholders in the file
2. Add `hashes/myhash.cpp` to the `set(HASH_SRC_FILES ...)` block in `hashes/Hashsrc.cmake`  

See [`hashes/README.addinghashes.md`](hashes/README.addinghashes.md) for the original SMHasher3 walkthrough.

#### After Creating the Template

Regardless of any of the methods used above:

**1. Implement your hash logic** — open `hashes/<hashname>.cpp` and replace the placeholder body:

```cpp

template <bool bswap>
static void MyHash( const void * in, const size_t len, const seed_t seed, void * out ) {
    // Your hash implementation goes here
    // 'in' = pointer to input data (2-bit encoded for genomic sequences)
    // 'len' = input length in bytes
    // 'seed' = seed value
    uint32_t hash = 0;
    // ... your logic ...
    PUT_U32<bswap>(hash, (uint8_t *)out, 0);
}
```

**2. Set the correct hash flags** in `REGISTER_HASH(...)`:

| Flag                                     | When to use                                           | FLAG TYPE |
| ---------------------------------------- | ----------------------------------------------------- | --------- |
| `FLAG_HASH_LOCALITY_SENSITIVE`           | Your hash is an LSH function                          | HASH      |
| `FLAG_HASH_TOKENISATION_PROPERTY`        | Your hash decomposes input into k-mers/tokens         | HASH      |
| `FLAG_HASH_HAMMING_SIMILARITY`           | Preserves Hamming distance                            | HASH      |
| `FLAG_HASH_JACCARD_SIMILARITY`           | Preserves Jaccard similarity                          | HASH      |
| `FLAG_HASH_COSINE_SIMILARITY`            | Preserves Cosine similarity                           | HASH      |
| `FLAG_HASH_ANGULAR_SIMILARITY`           | Preserves Angular similarity                          | HASH      |
| `FLAG_HASH_EDIT_SIMILARITY`              | Preserves Edit distance                               | HASH      |
| `FLAG_HASH_OVERLAPPING_TOKENS`           | Uses overlapping k-mers                               | HASH      |
| `FLAG_HASH_NONOVERLAPPING_TOKENS`        | Uses non-overlapping k-mers                           | HASH      |
| `FLAG_HASH_UNIVERSE_VECTOR_OPTIMISATION` | Enable pre-computed union bit vectors                 | HASH      |
| `FLAG_IMPL_SLOW`                         | Hash is computationally expensive                     | IMPL      |
| `FLAG_IMPL_VERY_SLOW`                    | Very slow (reduces LSH test parameters automatically) | IMPL      |
| `FLAG_IMPL_SMALL_SEQUENCE_LENGTH`        | Uses small sequences (40 bases instead of 512)        | IMPL      |

- **IMPL flags** are for controlling **test execution behaviour** — they tell the testing module how to adjust parameters (e.g., fewer iterations, shorter sequences) based on the computational cost of your hash implementation. They do not affect hash semantics.
- **HASH flags** are for declaring the **mathematical properties and similarity semantics** of your hash function. They tell the testing module which distance metric to use when measuring collision rates, how input is tokenised, and whether LSH-specific test logic should be activated.

#### Example: Adding Flags in `REGISTER_HASH(...)`

If your hash is a locality-sensitive MinHash that preserves Jaccard similarity using overlapping 5-mers, and it's computationally expensive:

```cpp
REGISTER_HASH(MyMinHash_64,
   $.desc            = "My custom MinHash (64-bit, Jaccard, overlapping k-mers)",
   $.hash_flags      = FLAG_HASH_LOCALITY_SENSITIVE | FLAG_HASH_TOKENISATION_PROPERTY | FLAG_HASH_JACCARD_SIMILARITY | FLAG_HASH_OVERLAPPING_TOKENS,
   $.impl_flags      = FLAG_IMPL_VERY_SLOW,
   // ... other registration fields ...
);
```
Note that we use ORing of the flags to combine them.

> **Tip:** If your hash is extremely slow (minutes per test), use `FLAG_IMPL_VERY_SLOW` — this reduces the number of sequence pairs to work on.

**3. Build and verify:**

```bash
cd build
cmake ..
make 

# Confirm your hash is registered
./SMHasher3 --list | grep MyHash
```

---

### Part 2 — Running Tests

#### CLI Reference

```bash
./SMHasher3 [options] [<hashname>]
```

**Key flags:**

| Flag                    | Description                                                   | Status in BioHasher |
| ----------------------- | ------------------------------------------------------------- | ------------------- |
| `--list`                | List all registered hashes with descriptions                  | Active              |
| `--listnames`           | List just hash names (useful for scripting)                   | Active              |
| `--tests`               | Print all valid test suite names                              | Active              |
| `--test=<name>[,...]`   | Enable **only** these tests (disables all others first)       | Active              |
| `--notest=<name>[,...]` | Disable specific tests                                        | Active              |
| `--ncpu=N`              | Number of threads for parallel tests                          | Active              |
| `--extra`               | Run extended/thorough testing                                 | Active              |
| `--verbose`             | Verbose output with more stats and diagrams                   | Not Active          |
| `--seed=<value>`        | Set hash default seed (hex or decimal)                        | Active              |
| `--randseed=<value>`    | Set base RNG seed for reproducibility                         | Active              |
| `--endian=<mode>`       | Force endianness (`default`, `native`, `big`, `little`, etc.) | Active              |
| `--exit-on-failure`     | Stop on first test failure                                    | Active              |
| `--version`             | Print version string                                          | Active              |

#### Test 1 — LSH Collision Test (`--test=LSHCollision`)

**What it does:** The collision test measures how faithfully a hash function preserves sequence similarity. It generates thousands of random genomic sequence pairs, mutates one copy at controlled rates, bins the pairs into 100 similarity buckets (0.00–1.00), and records the average hash-collision rate per bucket. The resulting *collision curve* should closely track the diagonal for an ideal LSH — deviations reveal where the hash over- or under-estimates similarity, helping you understand its quality before deploying it in a real pipeline.

```bash
# Run LSH collision test (multi-threaded recommended)
./SMHasher3 --test=LSHCollision SubSeqHash-64 --ncpu=16
```

**Output:** `results/collisionResults_<hashname>.csv`

**Test parameters** (automatically adjusted based on hash flags):

| Parameter                 | Normal Hash | `FLAG_IMPL_VERY_SLOW`                            |
| ------------------------- | ----------- | ------------------------------------------------ |
| Sequence pairs per bin    | 50,000      | 5,000                                            |
| Hash repetitions per pair | 2,000       | 2,000                                            |
| Sequence length           | 512 bases   | 512 (or 40 if `FLAG_IMPL_SMALL_SEQUENCE_LENGTH`) |

---

#### Test 2 — LSH Collision AND-OR Test (`--test=LSHCollisionAndOrTest`)

**What it does:** AND-OR amplification reshapes the collision probability's S-curve to give you finer control over the trade-off between false positives and false negatives. The **AND** parameter `b` raises the base collision probability to the `b`-th power (making the curve steeper — fewer false positives), while the **OR** parameter `r` takes `r` independent AND-bands and reports a collision if *any* band matches (recovering recall). This test sweeps over a grid of `(AND, OR)` pairs and records the amplified collision curve for each, so you can visually compare how different configurations sharpen or flatten the curve.

The `(AND, OR)` grid is configured in [`util/LSHGlobals.cpp`](util/LSHGlobals.cpp) via `g_ANN_start_B`, `g_ANN_MAX_B`, `g_ANN_start_R`, `g_ANN_MAX_R`.

```bash
./SMHasher3 --test=LSHCollisionAndOrTest SubSeqHash-64 --ncpu=16
```

**Output:** `results/collisionResults_<hashname>ANDOR.csv`

The output format is the same tagged-line CSV as the basic collision test, with additional `:10:` (AND/OR headers) and `:11:` (AND/OR values) lines for each parameter pair, followed by `:12:` collision rates.

> **Full documentation:** See [`ANDORtest.md`](documents/ANDORtest.md) for the complete reference including AND-OR basics, internal pipeline, pseudocode, all configurable parameters, and caveats.

---

#### Test 3 — Approximate Nearest Neighbour Test (`--test=LSHApproxNearestNeighbour`)

**What it does:** This test evaluates the hash function as an *LSH index* for nearest-neighbour search — the end-to-end use case most genomic LSH pipelines care about. It:

1. Generates a reference database of random sequences.
2. Samples query sequences from the reference, then mutates them to target 90–100% similarity.
3. Computes brute-force ground truth (the true nearest neighbour for each query).
4. For each `(b, r)` configuration, builds an LSH index with `r` tables (each using a `b`-hash band signature), inserts all reference sequences, queries with each mutated sequence, and compares results against ground truth.
5. Reports **Recall**, **Precision**, **FPR**, and **F1** averaged over multiple independent runs.

This helps you select the optimal `(b, r)` parameters for your application by directly measuring retrieval quality.

> **Full documentation:** See [`ANNtest.md`](documents/ANNtest.md) for the complete reference including the 5-phase pipeline internals, all configurable parameters, evaluation metrics, and caveats.

```bash
./SMHasher3 --test=LSHApproxNearestNeighbour SubSeqHash-64 --ncpu=16
```

**Output:** `results/ApproxNearestNeighbourResults_<hashname>.csv`

**Output format:**

```
:1: LSH Approx Nearest Neighbour Summary
:2: Hashname, SequenceLength, TokenLength, Distance Metric, Mutation Model, Mutation Expression
:3: <values>
:4.1:/:4.2:/:4.3: Hash-specific parameters
:5: b, r, Avg_Recall, Avg_Precision, Avg_FPR, Avg_F1_Score   (column headers)
:6: <one data row per (b,r) pair>
```

**Configuration** (all in [`util/LSHGlobals.cpp`](util/LSHGlobals.cpp)):

| Variable                       | Description                        | Default |
| ------------------------------ | ---------------------------------- | ------- |
| `g_ANN_start_B` / `g_ANN_MAX_B` | Range of `b` (hashes per table)  | 1–5     |
| `g_ANN_start_R` / `g_ANN_MAX_R` | Range of `r` (number of tables)  | 1–7     |
| `g_ANN_runs_for_avg`           | Independent runs to average over   | 3       |
| `g_sequenceLength_ANN`         | Length of reference/query sequences | —       |
| `g_Nseq_in_Database`           | Number of reference sequences      | —       |
| `g_numQueriesForApproxNNTest`  | Number of query sequences          | —       |

---

#### Configuring the Mutation Model

The mutation model is set at compile time in [`util/LSHGlobals.cpp`](util/LSHGlobals.cpp):

| Variable                     | Options                                                                                                                                                                            | Default         |
| ---------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------- |
| `g_mutation_model`           | `MUTATION_MODEL_SIMPLE_SNP_ONLY` (0), `MUTATION_MODEL_GEOMETRIC_MUTATOR` (1)                                                                                                       | `1` (Geometric) |
| `g_mutation_expression_type` | `MUTATION_EXPRESSION_BALANCED` (0), `MUTATION_EXPRESSION_SUB_ONLY` (1), `MUTATION_EXPRESSION_DEL_LITE` (2), `MUTATION_EXPRESSION_INS_LITE` (3), `MUTATION_EXPRESSION_SUB_LITE` (4) | `0` (Balanced)  |

> **Note:** If a hash has `FLAG_HASH_HAMMING_SIMILARITY`, the mutation model is automatically forced to `MUTATION_MODEL_SIMPLE_SNP_ONLY` at runtime (since the geometric mutator changes sequence lengths, which is incompatible with Hamming distance).

See [`MutationModels.md`](documents/MutationModels.md) for full documentation on what each mutation expression does.

After changing these values, rebuild:

```bash
cd build && make -j$(nproc)
```

#### Output File Format

Results are written to `results/collisionResults_<hashname>.csv` using tagged lines:

```md
:1: LSH Collision Test Results (header)
:2: Column names: Hashname, SequenceLength, TokenLength, DistanceMetric, MutationModel, MutationExpression
:3: Values for the above columns
:5: Similarity values (comma-separated floats)
:6: SNP rates per bin
:7: Deletion rates per bin (geometric mutator only)
:8: Insertion means per bin (geometric mutator only)
:9: Stay rates per bin (geometric mutator only)
:10: AND/OR parameter headers
:11: AND/OR parameter values (1,1 for basic test)
:12: Average collision rates per bin (comma-separated floats)
:13: Raw error parameters from aggregation
:14: Insertion rates (geometric mutator only)
```

For a particular Hash function, each run **appends** to the CSV if it already exists, so you can accumulate results across multiple token lengths or configurations.

---

### Part 3 — Generating Plots

#### 3a. Collision Curve Plots (`analysis/plot_collisioncurves.py`)

Works for both basic collision and AND-OR collision CSV files.

```bash
pip install pandas numpy matplotlib scipy
python analysis/plot_collisioncurves.py results/collisionResults_<hashname>.csv
```

All plots are saved in the current working directory.

| Filename Pattern              | Description                                 |
| ----------------------------- | ------------------------------------------- |
| `*_binaveraged.png`           | Bin-averaged collision curves (overlay)     |
| `*_snpcolor_multiplot.png`    | SNP-rate colored scatter subplots           |
| `*_boxplot_multiplot.png`     | Box plot subplots per similarity bin        |
| `*_monocolor_multiplot.png`   | Single-color scatter subplots               |
| `*_verificationCurves.png`    | Mutation parameter distributions (sanity check) |

#### 3b. ANN Result Plots (`analysis/plot_ANN.py`)

Visualises the Recall vs FPR trade-off across `(b, r)` configurations.

```bash
pip install matplotlib numpy adjustText
python analysis/plot_ANN.py results/ApproxNearestNeighbourResults_<hashname>.csv
```

Produces **6 plots** (3 linear + 3 log-scale):

| Filename Pattern                       | Description                                              |
| -------------------------------------- | -------------------------------------------------------- |
| `*_fpr_vs_recall.png` / `*_log.png`   | FPR vs Recall per `b` value, annotated with `r`          |
| `*_best_fpr_per_recall_bin.png` / `*_log.png` | Pareto-optimal (min FPR) per recall bin           |
| `*_all_points.png` / `*_log.png`      | All `(b,r)` configs as an annotated scatter              |

---

### Project Structure

```bash
BioHasher/
├── hashes/                             # Hash function implementations
│   ├── minhash.cpp                     # MinHash implementation
│   ├── simhash.cpp                     # SimHash implementation
│   └── subseqhash/                     # SubseqHash implementation
│       └── ssh1.cpp
├── tests/                              # Test implementations
│   ├── LSHCollisionTest.cpp            # Collision curve test
│   ├── LSHCollisionAndOrTest.cpp       # Collision curve test with AND-OR amplification
│   └── ApproxNearestNeighbourTest.cpp  # Approximate nearest neighbour test
├── analysis/                           # Plotting scripts
│   ├── plot_collisioncurves.py         # Collision curve plotting
│   └── plot_ANN.py                     # ANN result plotting
├── results/                            # Results
├── documents/                          # Documentation
│   ├── ANNtest.md                      # Approximate Nearest Neighbour Test
│   ├── Collisiontest.md                    # AND-OR Amplification Test
│   ├── createHashTemplate.md           # Adding a New Hash Function
│   ├── MutationModels.md               # Mutation Models
│   └── plot.md                         # Plotting Scripts
├── util/                               # Utilities & configuration
│   └── LSHGlobals.cpp                  # Test parameter globals
├── include/                            # Header files
└── README.md                           # This file
```

---

## License

This project builds upon the SMHasher3 framework. Please refer to individual source files for specific licensing information.

## References

- [SMHasher3](https://gitlab.com/fwojcik/smhasher3): Original hash testing framework BioHasher is based on.
- Hash functions in genomic sequence analysis - Ke Chen, Xiang Li, Qian Shi, Mingfu Shao, Paul Medvedev
- Shrivastava, Anshumali, and Ping Li. "In defense of minhash over simhash." Artificial intelligence and statistics. PMLR, 2014.
