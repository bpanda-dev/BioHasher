# LSH Collision AND-OR Test — Detailed Reference

This document provides an in-depth explanation of the `LSHCollisionAndOrTest`, how AND-OR amplification reshapes collision curves, the internal pipeline, all configurable parameters, and important caveats.

**Source:** [`tests/LSHCollision.cpp`](../tests/LSHCollision.cpp)

---

## Purpose

A single LSH hash function has a base collision probability `p(s)` that depends on similarity `s`. This curve is often too gradual — sequences at moderate similarity still collide too often (false positives), while highly similar sequences sometimes don't collide (false negatives). AND-OR amplification reshapes this curve to create a sharper threshold:

- **AND (parameter `b`):** Hash both sequences `b` times independently. Declare a collision only if *all* `b` hashes collide. This raises the effective collision probability to `p(s)^b`, suppressing false positives.
- **OR (parameter `r`):** Repeat the AND-band `r` times independently. Declare a collision if *any* of the `r` bands collides. This gives an effective probability of `1 - (1 - p(s)^b)^r`, recovering recall.

The test sweeps over a grid of `(AND, OR)` pairs and records the amplified collision curve for each, so you can visually compare how different configurations sharpen or flatten the S-curve.

```bash
./SMHasher3 --test=LSHCollisionAndOrTest SubSeqHash-64 --ncpu=16
```

**Output:** `results/collisionResults_<hashname>ANDOR.csv`

---

## How It Works (3-Phase Pipeline)

```
Phase 1              Phase 2                  Phase 3
Aggregation   ->   Sequence Generation   ->  AND-OR Collision
Binning            & Mutation                Measurement
                                             (repeat per (AND,OR) pair)
```

### Phase 1 — Aggregation Binning

Identical to the basic collision test's aggregation phase:

1. Generates `g_norm_N_agg_cases` (default: 500,000) random sequence pairs and mutates them with uniformly-drawn error rates.
2. Sorts results into 100 similarity bins (0.00–1.00).
3. For each bin, records the error parameters and computes mean/stddev.
4. Partially-filled bins are topped up with Gaussian samples from the bin's statistics.

This produces a lookup table that maps desired similarity → error parameter.

### Phase 2 — Sequence Generation & Mutation

For the main test phase:

1. Generates `N_seq` sequence pairs (default: 5,000 for normal hashes, 1,000 for very slow hashes).
2. For each pair, draws a random bin from the aggregation table, samples an error parameter, and mutates the sequence accordingly.
3. Records similarity values, SNP rates, and (for the geometric mutator) deletion rates, insertion means, stay rates, and insertion rates.

### Phase 3 — AND-OR Collision Measurement

This is the core of the test. For each `(AND, OR)` pair in the configured grid:

#### Seed Generation

Each of the `b × r` independent hash evaluations gets a deterministic seed:

```
seed[or_i][and_i] = HashSeed + hash_idx + (or_i * and_param + and_i) * 1337
```

#### AND-OR Logic (per sequence pair, per hash iteration)

```
For each hash_iteration (1 to N_hash):
    For each OR-band (1 to r):
        and_flag[or_i] = 1                      # assume collision
        For each AND-slot (1 to b):
            hash original sequence with seed[or_i][and_i]
            hash mutated  sequence with seed[or_i][and_i]
            if hashes differ:
                and_flag[or_i] = 0              # AND fails → no collision in this band
                break                           # early exit

    collision = OR over all and_flags           # any band matched?
    collision_count += collision

average_collision = collision_count / N_hash
```

#### Parallelisation

The collision computation is parallelised across sequence pairs using `g_NCPU` threads. Each thread processes a block of sequences independently (no shared mutable state — `AverageCollision` is indexed per-sequence).

#### Universe Vector Optimisation

For hashes with `FLAG_HASH_UNIVERSE_VECTOR_OPTIMISATION`, the test pre-computes union bit vectors for all sequence pairs *once* before the parallel phase. This prevents millions of redundant memory allocations during hashing and significantly improves performance through better cache utilisation.

---

## Output Format

The output file uses the same tagged-line CSV format as the basic collision test, with AND-OR specific additions:

```
:1:  LSH Collision Test Results (header)
:2:  Column names — Hashname, SequenceLength, TokenLength, Distance Metric, Mutation Model, Mutation Expression
:3:  Values for above columns
:4.1:/:4.2:/:4.3: Hash-specific parameters

:13: Raw error parameters from aggregation phase

:5:  Similarity values (comma-separated, one per sequence pair)
:6:  SNP rates per sequence pair
:7:  Deletion rates (geometric mutator only)
:8:  Insertion means (geometric mutator only)
:9:  Stay rates (geometric mutator only)
:14: Insertion rates (geometric mutator only)

--- Repeated per (AND, OR) pair: ---
:10: AND, OR                          (column headers)
:11: <and_value>, <or_value>          (parameter values)
:12: Average collision rates           (comma-separated, one per sequence pair)
```

The file **appends** if it already exists, so multiple runs accumulate.

---

## Configurable Parameters

### In `util/LSHGlobals.cpp` (compile-time)

| Variable                        | Description                                   | Default        |
|---------------------------------|-----------------------------------------------|----------------|
| `g_ANN_start_B` / `g_ANN_MAX_B` | Range of AND parameter to sweep               | `1`–`5`        |
| `g_ANN_start_R` / `g_ANN_MAX_R` | Range of OR parameter to sweep                | `1`–`7`        |
| `g_norm_N_agg_cases`            | Number of sequences for aggregation binning   | `500,000`      |
| `g_norm_N_seq`                  | Sequences per test (normal hashes)            | `5,000`        |
| `g_norm_N_hashes`               | Hash iterations per sequence pair (normal)    | `1,000`        |
| `g_slow_N_seq`                  | Sequences per test (very slow hashes)         | `1,000`        |
| `g_slow_N_hashes`               | Hash iterations per sequence pair (slow)      | `1,000`        |
| `g_sequenceLength_large`        | Sequence length for normal hashes             | `{512}`        |
| `g_sequenceLength_small`        | Sequence length for small-sequence hashes     | `{45}`         |
| `g_tokenLengths_array`          | Token lengths to test (tokenised hashes)      | `{13}`         |
| `g_mutation_model`              | Mutation model (0 = SNP-only, 1 = Geometric)  | `1`            |
| `g_mutation_expression_type`    | Mutation expression variant                   | `0` (Balanced) |
| `g_bincount_full`               | Max entries per similarity bin in aggregation | `1,000`        |

> [!NOTE]
> The AND/OR parameter grid reuses `g_ANN_start_B`/`g_ANN_MAX_B`/`g_ANN_start_R`/`g_ANN_MAX_R` — the same variables used by the ANN test. Changing them affects both tests.

### In `LSHCollision.cpp` (source-level)

| Variable          | Description                      | Default           |
|-------------------|----------------------------------|-------------------|
| `tokenlengths`    | Token lengths (tokenised hashes) | `{13}`            |
| `sequenceLengths` | Sequence lengths to test         | `{512}` or `{45}` |

---

## The Mathematics

For a base hash with collision probability `p` at a given similarity:

| Configuration           | Effective collision probability |
|-------------------------|---------------------------------|
| Base hash (AND=1, OR=1) | `p`                             |
| AND only (AND=b, OR=1)  | `p^b`                           |
| OR only (AND=1, OR=r)   | `1 - (1-p)^r`                   |
| AND-OR (AND=b, OR=r)    | `1 - (1 - p^b)^r`               |

The AND-OR combination creates an S-curve with a sharper transition. The "threshold" similarity (where the curve is steepest) is approximately at `p ≈ (1/b)^(1/b)`.

**Example:** With `AND=3, OR=5`:
- At `p = 0.9`: effective = `1 - (1 - 0.9³)⁵` = `1 - (1 - 0.729)⁵` ≈ `0.999` (high recall)
- At `p = 0.3`: effective = `1 - (1 - 0.3³)⁵` = `1 - (1 - 0.027)⁵` ≈ `0.129` (low FPR)

---

## Caveats & Notes

> [!IMPORTANT]
> **Computational cost grows as O(b × r × N_hash × N_seq).** Each `(AND, OR)` pair requires `b × r` hash evaluations per hash iteration per sequence pair. With default `b` up to 5, `r` up to 7, `N_hash = 1,000`, and `N_seq = 5,000`, that's up to 175 million hash evaluations per `(AND, OR)` pair. Use `--ncpu` to parallelise.

> [!NOTE]
> **Seed derivation is deterministic but not cryptographic.** Seeds are derived as `HashSeed + hash_idx + slot_index * 1337`. This gives independent-enough seeds for testing purposes but is not a cryptographically random scheme.

> [!NOTE]
> **Empty similarity bins are left empty.** If the aggregation phase produces no sequences in a particular similarity bin, that bin is skipped entirely during test-phase sampling. This can happen for extreme similarity values (very close to 0 or 1) depending on the mutation model.

> [!WARNING]
> **Mutation model is auto-overridden for Hamming hashes.** If the hash has `FLAG_HASH_HAMMING_SIMILARITY`, the mutation model is forced to `MUTATION_MODEL_SIMPLE_SNP_ONLY` at runtime, since the geometric mutator produces insertions/deletions that change sequence length — incompatible with Hamming distance.

> [!NOTE]
> **Sanity checks for tokenised hashes.** The test validates that: (1) token length ≤ sequence length, (2) enough tokens exist for meaningful analysis (≥ 20), and (3) the token space isn't too small relative to the number of tokens (which would cause high collision rates even for random data).

---

## Plotting Results

Use the same collision curve plotting script as the basic collision test:

```bash
pip install pandas numpy matplotlib scipy
python analysis/plot_collisioncurves.py results/collisionResults_<hashname>ANDOR.csv
```

The plots will show separate collision curves for each `(AND, OR)` configuration overlaid, making it easy to compare how different amplification settings reshape the S-curve.
