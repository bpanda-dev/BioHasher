# BioHasher - Genomic Hash Function Testing Framework

## Overview

BioHasher is a specialized hash function testing framework designed for biological sequence analysis. It extends the SMHasher3 framework with locality-sensitive hashing (LSH) tests optimized for genomic data processing and similarity detection.

## Usage Guide

This section walks through the complete workflow: **adding a new hash function**, **running tests**, and **generating plots** from the results.

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

> **Full documentation:** See [`createHashTemplate.README.md`](createHashTemplate.README.md) for the complete reference including validation rules, example sessions, and troubleshooting.

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

- **IMPL flags** are for controlling **test execution behaviour** — they tell the test harness how to adjust parameters (e.g., fewer iterations, shorter sequences) based on the computational cost of your hash implementation. They do not affect hash semantics.
- **HASH flags** are for declaring the **mathematical properties and similarity semantics** of your hash function — they tell the test harness which distance metric to use when measuring collision rates, how input is tokenised, and whether LSH-specific test logic should be activated.
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

#### Running LSH Collision Tests (BioHasher-Specific)

The LSH Collision test is **off by default** — you must explicitly enable it:

```bash
# Run LSH collision test for a SubseqHash hash
./SMHasher3 --test=LSHCollision SubseqHash-64

# Run with multi-threading (recommended for large tests)
./SMHasher3 --test=LSHCollision SubseqHash-64 --ncpu=16
./SMHasher3 --test=LSHCollision OneBaseSamplingHash-32 --ncpu=16

#Note: The and-or amplification documentation is underway.
# Run LSH AND-OR amplification collision test for a SubseqHash hash
#./SMHasher3 --test=LSHCollisionAndOrTest SimHash-Ang-32 --ncpu=16
#./SMHasher3 --test=LSHCollision SimHash-Ang-32 --ncpu=16

```

#### What the LSH Collision Test Does (A brief idea)

1. **Generates sequence pairs** — creates random genomic sequences and mutates them using the configured mutation model.
2. **Bins by similarity** — sorts the pairs into 100 similarity bins ($0.00–1.00$) based on the hash's distance metric (Hamming, Jaccard, Cosine, etc.)
3. **Computes collision rates** — for each bin, hashes both sequences N times with different seeds and measures the average collision rate
4. **Writes CSV results** — outputs to `results/collisionResults_<hashname>.csv`

**Test parameters** (automatically adjusted based on hash flags):

| Parameter                 | Normal Hash | `FLAG_IMPL_VERY_SLOW`                            |
| ------------------------- | ----------- | ------------------------------------------------ |
| Sequence pairs per bin    | 50,000      | 5,000                                            |
| Hash repetitions per pair | 2,000       | 2,000                                            |
| Sequence length           | 512 bases   | 512 (or 40 if `FLAG_IMPL_SMALL_SEQUENCE_LENGTH`) |

#### Configuring the Mutation Model

The mutation model is set at compile time in [`util/LSHGlobals.cpp`](util/LSHGlobals.cpp):

| Variable                     | Options                                                                                                                                                                            | Default         |
| ---------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------- |
| `g_mutation_model`           | `MUTATION_MODEL_SIMPLE_SNP_ONLY` (0), `MUTATION_MODEL_GEOMETRIC_MUTATOR` (1)                                                                                                       | `1` (Geometric) |
| `g_mutation_expression_type` | `MUTATION_EXPRESSION_BALANCED` (0), `MUTATION_EXPRESSION_SUB_ONLY` (1), `MUTATION_EXPRESSION_DEL_LITE` (2), `MUTATION_EXPRESSION_INS_LITE` (3), `MUTATION_EXPRESSION_SUB_LITE` (4) | `0` (Balanced)  |

> **Note:** If a hash has `FLAG_HASH_HAMMING_SIMILARITY`, the mutation model is automatically forced to `MUTATION_MODEL_SIMPLE_SNP_ONLY` at runtime (since the geometric mutator changes sequence lengths, which is incompatible with Hamming distance).

See [`MutationModels.md`](MutationModels.md) for full documentation on what each mutation expression does.

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

After running the LSH collision tests, use the Jupyter notebooks in [`analysis/python notebooks/`](analysis/python%20notebooks/) to parse the CSV results and generate plots.

#### Prerequisites

```bash
pip install pandas numpy matplotlib scipy
```

#### Available Notebooks

| Notebook                                                                           | Purpose                                                                  |
| ---------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| [`analysisplots.ipynb`](analysis/python%20notebooks/analysisplots.ipynb)           | **Main analysis notebook** — reads CSV results, generates all plot types |
| [`cleananalysisplots.ipynb`](analysis/python%20notebooks/cleananalysisplots.ipynb) | Cleaned/final version of analysis plots                                  |

| [`plots.ipynb`](analysis/python%20notebooks/plots.ipynb) | Additional plotting utilities and helper functions |

| [`lsh.ipynb`](analysis/python%20notebooks/lsh.ipynb) | Standalone LSH experiments using Python `datasketch` / `simhash` |

#### Step-by-Step Plotting Workflow

##### Step 1: Load the CSV data

The helper script [`analysis/python notebooks/plot.py`](analysis/python%20notebooks/plot.py) provides `read_collision_data_complete()` which parses the `:N:` tagged CSV format into a Pandas DataFrame:

Each row in the resulting DataFrame contains:

- `hashname`, `sequencelength`, `tokenlength` — test metadata

- `similarity_values` — array of similarity bin centers

- `collision_rates` — array of average collision rates per bin

- `snp_rate`, `del_rate`, `ins_mean`, `stay_rate` — mutation parameters per bin

- `AND_param`, `OR_param` — amplification parameters

##### Step 2: Generate bin-averaged collision curves (overlay plot)

This is the primary plot type — it shows average collision rate vs. similarity, with one curve per hash configuration:

##### Output Plot Files

All plots are saved to the `analysis/python notebooks/` directory:

| Filename Pattern              | Description                             |
| ----------------------------- | --------------------------------------- |
| `*_binaveraged.png`           | Bin-averaged collision curves (overlay) |
| `*_snpcolor_multiplot.png`    | SNP-rate colored scatter subplots       |
| `*_boxplot_multiplot.png`     | Box plot subplots per similarity bin    |
| `*_singlecolor_multiplot.png` | Single-color scatter subplots           |
| `*_barplot_multiplot.png`     | Bar plot subplots                       |

---

### File Structure

```bash
BioHasher/
├── hashes/                         # Hash function implementations
│ ├── minhash.cpp                       # MinHash implementation
│ ├── simhash.cpp                       # SimHash implementation
│ └── subseqhash/                       # SubseqHash implementation
│     └──  ssh1.cpp
├── tests/                          # Test implementations
│ ├── LSHCollisionTest.cpp              # Collision curve test
│ └── LSHCollisionAndOrTest.cpp         # Collision Curve test with AND-OR amplification
├── include/                        # Header files
└── README.md                       # This file
```

---

## Legacy SMHasher3 Information

This project builds upon SMHasher3, a tool for testing the quality of [hash functions](https://en.wikipedia.org/wiki/Hash_function) in terms of their distribution, collision, and performance properties.

### Original SMHasher3 Usage

- `./SMHasher3 --tests` will show all available test suites

- `./SMHasher3 --list` will show all available hashes and their descriptions

- `./SMHasher3 <hashname>` will test the given hash with the default set of test suites

## License

This project builds upon the SMHasher3 framework. Please refer to individual source files for specific licensing information.

## References

- **SMHasher3**: Original hash testing framework
- **Genomic Hashing**: Specialized applications in bioinformatics.
