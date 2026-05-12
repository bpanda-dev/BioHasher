# Mutation Model & Expression Configuration Guide

This document explains the different mutation models supported by the tool and explains the macros defined in `LSHGlobals.h` which are useful in configuring the models.

The current version of BioLSHasher contains, two mutation models, 

1. **Substitution Only mutation model** : Performs only point substitutions across the input sequence base on the mutation rate $e_{s} \in [0,1]$. BioLSHasher samples the $e_{s}$ uniformly from the domain $[0,1]$.
2. **Geometric Insertion mutation model** : Performs substitutions and deletions as point mutations(length 1) with the error rates $e_s$ and $e_d$, respectively, satisfying $e_s \ge 0,\ e_d < 1 \text{ and } e_s+e_d \le 1$. In contrast, for every position in sequence, insertion events are performed by sampling the length of the insertion from a geometric distribution with mean $\mu >= 0$. If the sampled insertion length at a position of the sequence is $0$, there is no insertion.


> **Note:** Hamming Similarity is only defined between sequences of equal length. Geometric Insertion mutation model may change the length of the sequence, therefore if a LSH hash is defined to be preserving **Hamming Similarity**, BioLSHasher will automatically force apply **Substitution Only** model at runtime in the test entry points (`LSHCollisionTest.cpp`, `LSHCollisionAndOrTest.cpp`).

The Geometric Insertion mutation model inherently contains three effective degrees of freedom. To streamline testing and avoid the complexity of exploring the entire parameter space, we reduce the degrees of freedom by coupling the mutation rates via fixed functional relationships(we call it mutation expressions).

## Baseline Parameter Derivation

The core parameter for all configurations is the **mean insertion length**, denoted as $\mu$.

1. We draw $\mu$ uniformly at random from the interval $[0, 0.03]$. 
2. This $\mu$ serves as the mean for the geometric distribution used to sample insertion lengths. 
3. We define the success probability $p$ for this distribution as: $p = \frac{1}{1+\mu}$
4. We then calculate a baseline reference rate, $e_i$, defined as the probability of generating an insertion of exactly length 1: $$e_i = (1-p) \cdot p$$
Using this baseline $e_i$, we define the substitution rate ($e_s$) and deletion rate ($e_d$) for five distinct experimental scenarios:

## Mutation Expressions:

1. **BALANCED**: Substitution, deletion, and length-1 insertion rates are all equal.  i.e. $e_s = e_i$ and $e_d = e_i$.
2. **DELETION-lite**: Deletion rate is $(\frac{1}{5})^{th}$ the $e_i$ and $e_s = e_i$. Sequences tend to grow slightly after mutation with new nucleotides.
3. **INSERTION-lite**: Both substitution and deletion rates are 5× higher i.e. $e_s = 5*e_i$ and $e_d = 5*e_i$, meaning insertions are relatively less impactful. Sequences tend to shrink.
4. **SUBSTITUTION-lite**: Substitutions occur at 1/5th of $e_i$ and $e_d = e_i$. 
5. **SUBSTITUTION-only**: Unlike the standard **Substitution Only** model where the rate is drawn from $[0, 1]$, here $e_s$ is constrained to the range $[0, (1-p)p]$ where $p = \frac{1}{1 + 0.03}$). *Note:* The sole purpose of $e_i$ here is to set the scalar value for $e_s$. For general use, we recommend the standard **Substitution Only** mutation model over this constrained version.

---

## Configuring

To set and configure the mutation models, we have defined various macros in `LSHGlobals.h` file. 

### Setting the Mutation Model to use

Mutation model to use can be set via the global variable  `g_mutation_model` in `LSHGlobals.cpp`.

| Macro                            | Value | Description                                                                                                                                                                                                                                                                                                 |
| -------------------------------- | ----- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| MUTATION_MODEL_SIMPLE_SNP_ONLY   | $0$   | **Substitution-only model.** Each position in the sequence is independently flipped to a different base with probability equal to `snpRate`. Sequence length is **never changed** (no insertions or deletions).                                                                                             |
| MUTATION_MODEL_GEOMETRIC_MUTATOR | $1$   | **Geometric mutation model.** At each position, a substitution, deletion, or stay event occurs based on probabilities ($e_s$,$e_d$). Additionally, insertions of geometrically-distributed length are drawn at every position. Sequence length **can change**. Implemented by SequenceDataMutatorGeometric. |

Edit the initializer of `g_mutation_model` in `LSHGlobals.cpp`:
```cpp
// ...
// Change to MUTATION_MODEL_SIMPLE_SNP_ONLY (0) or MUTATION_MODEL_GEOMETRIC_MUTATOR (1)
uint32_t g_mutation_model = MUTATION_MODEL_SIMPLE_SNP_ONLY;
// ...
```

---

### Setting the Mutation Expression to use (Only for Geometric Mutator)

When using `MUTATION_MODEL_GEOMETRIC_MUTATOR`, these macros control **the relationship** between the insertion mean length given by geometric mean parameter $\mu_i$ and the resulting substitution/deletion rates. They are set via the global constant `g_mutation_expression_type`.

| Macro                        | Value | Description                                                                                                                                                   |
| ---------------------------- | ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| MUTATION_EXPRESSION_BALANCED | $0$   | **Balanced.** Substitution, deletion, and length-1 insertion rates are all equal.                                                                             |
| MUTATION_EXPRESSION_SUB_ONLY | $1$   | **Substitution only.** No deletions occur; insertion lengths are forced to 0. Only substitutions happen alongside the geometric insertion framework.          |
| MUTATION_EXPRESSION_DEL_LITE | $2$   | **Deletion-lite.** Deletions occur at 1/5th the rate of substitutions. Sequences tend to grow slightly.                                                       |
| MUTATION_EXPRESSION_INS_LITE | $3$   | **Insertion-lite.** Both substitution and deletion rates are 5× higher, meaning insertions are relatively less impactful. Sequences mutate more aggressively. |
| MUTATION_EXPRESSION_SUB_LITE | $4$   | **Substitution-lite.** Substitutions occur at 1/5th the rate of deletions. Sequences tend to shrink.                                                          |

```cpp
// ...
// Change to any of: MUTATION_EXPRESSION_BALANCED (0), MUTATION_EXPRESSION_SUB_ONLY (1),
// MUTATION_EXPRESSION_DEL_LITE (2), MUTATION_EXPRESSION_INS_LITE (3), MUTATION_EXPRESSION_SUB_LITE (4)
const uint32_t g_mutation_expression_type = MUTATION_EXPRESSION_BALANCED;
// ...
```
---

## Quick Reference

| What to change                           | File             | Variable                     |
| ---------------------------------------- | ---------------- | ---------------------------- |
| Mutation model (SNP-only vs Geometric)   | `LSHGlobals.cpp` | `g_mutation_model`           |
| Mutation expression (rate relationships) | `LSHGlobals.cpp` | `g_mutation_expression_type` |

Both sets of macros are defined in `LSHGlobals.h`, initialized in `LSHGlobals.cpp` and consumed by the mutation engines in `BioDataGeneration.cpp`.