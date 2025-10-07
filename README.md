# BioHasher - Genomic Hash Function Testing Framework

## Overview

BioHasher is a specialized hash function testing framework designed for biological sequence analysis. It extends the SMHasher3 framework with locality-sensitive hashing (LSH) algorithms optimized for genomic data processing and similarity detection.

## Author

**bpanda-dev** - Implementation of MinHash and SimHash algorithms with framework extensions

## Key Features

### Genomic-Optimized Hash Functions
- **MinHash**: Locality-sensitive hashing for genomic similarity detection
- **SimHash**: Bit-wise similarity hashing for biological sequences
- **2-bit Encoding Support**: Efficient nucleotide sequence processing

### Advanced Testing Framework
- **LSH Collision Testing**: Specialized tests for locality-sensitive hash functions
- **Variable Output Support**: Hash functions with configurable output lengths
- **Tokenization-Aware Testing**: K-mer based sequence analysis

### Performance Optimized
- **Bit-level Operations**: Efficient processing of encoded sequences
- **Configurable Parameters**: Adjustable k-mer lengths and hash functions
- **Memory Efficient**: Optimized for large genomic datasets

## Implemented Algorithms

### MinHash Algorithm

**File**: [`hashes/minhash.cpp`](hashes/minhash.cpp)

MinHash is a locality-sensitive hashing technique that estimates Jaccard similarity between sets. This implementation is optimized for biological sequences.

#### Key Features:
- **Multiple Signatures**: Generates 32 hash signatures by default
- **Variable Output**: Supports returning 1 to N signatures
- **K-mer Tokenization**: Default k-mer length of 13 bases
- **Collision Resistance**: Different hash functions with unique seeds

#### Usage:
```cpp
// Set k-mer length
set_minhash_token_length(13);

// Generate multiple signatures
size_t num_signatures = 32;
std::vector<uint32_t> signatures(num_signatures);
minhash32_bitwise<false>(sequence, length, seed, signatures.data(), num_signatures);

// Generate single signature
uint32_t single_hash;
minhash32_bitwise<false>(sequence, length, seed, &single_hash, 1);
```

### SimHash Algorithm

**File**: [`hashes/simhash.cpp`](hashes/simhash.cpp)

SimHash produces similar hash values for similar inputs, making it ideal for near-duplicate detection and similarity clustering.

#### Key Features:
- **Bit Voting Mechanism**: Aggregates features through bit-wise voting
- **Hamming Distance Preservation**: Hash similarity correlates with sequence similarity
- **Configurable Tokens**: Default 11-base k-mers (22 bits)
- **FNV1a Based**: Uses FNV1a for individual token hashing

#### Usage:
```cpp
// Generate SimHash
uint32_t simhash_result;
simhash32_bitwise<false>(sequence, length, seed, &simhash_result);

// Calculate similarity (lower Hamming distance = higher similarity)
int hamming_distance = __builtin_popcount(hash1 ^ hash2);
```

## Input Format

### 2-Bit Nucleotide Encoding
- **A**: 00
- **C**: 01  
- **G**: 10
- **T**: 11

### Storage Format
- **Packing**: 4 bases per byte
- **Bit Order**: MSB-first
- **Constraints**: Must be multiple of 4 bases (byte-aligned)
- **No Padding**: Currently requires exact byte boundaries

### Example Encoding
```
Sequence: ACGT
Encoding: 00 01 10 11 = 0x1B (single byte)
```

## Framework Extensions

### New Hash Flags

- **`FLAG_HASH_LOCAL_SENSITIVE`**: Marks LSH algorithms
- **`FLAG_HASH_TOKENISATION_PROPERTY`**: Indicates k-mer tokenization
- **`FLAG_HASH_VARIABLE_OUTPUT_SIZE`**: Supports variable output lengths

### Variable Output Hash Functions

```cpp
// New function signature for variable output
typedef void (*HashFnVarOut)(const void* in, const size_t len, 
                             const seed_t seed, void* out, 
                             const size_t outlen);

// Conditional usage in tests
if (hinfo->hasVariableOutput()) {
    HashFnVarOut hashVarOut = hinfo->hashFnVarOut(g_hashEndian);
    hashVarOut(input, len, seed, output, output_length);
} else {
    HashFn hash = hinfo->hashFn(g_hashEndian);
    hash(input, len, seed, output);
}
```

## Building

### Requirements
- **C++11** or later
- **CMake** 3.10+
- **Standard Library** support for vectors and memory management

### Build Instructions
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)

# Run tests
./SMHasher3
```

### Build Targets
- **SMHasher3**: Main executable with all hash functions
- **SMHasher3Tests**: Test library
- **SMHasher3Hashlib**: Hash function library

## Testing

### LSH-Specific Tests

The framework includes specialized tests for locality-sensitive hash functions:

**File**: [`tests/LSHCollisionTest.cpp`](tests/LSHCollisionTest.cpp)

#### Test Features:
- **Similarity-Aware**: Tests collision rates at different similarity levels
- **Variable Output Support**: Handles both standard and multi-output hashes
- **Genomic-Specific**: Designed for biological sequence characteristics

#### Running LSH Tests:
```bash
# Run all tests
./SMHasher3

# Run specific hash function
./SMHasher3 MinHash-32-bitwise

# Run with specific test flags
./SMHasher3 --test=LSHCollision MinHash-32-bitwise
```

## Performance Characteristics

### MinHash Performance
- **Time Complexity**: O(n × k × m) where n=sequence length, k=k-mer length, m=hash functions
- **Space Complexity**: O(m) for signature storage
- **Marked**: `FLAG_IMPL_SLOW` due to computational complexity

### SimHash Performance  
- **Time Complexity**: O(n × k) where n=sequence length, k=k-mer length
- **Space Complexity**: O(n) for feature hash storage
- **Marked**: `FLAG_IMPL_SLOW` due to bit operations

## Applications

### Genomic Analysis
- **Sequence Similarity Detection**: Fast approximate matching
- **Database Search**: LSH-based genomic databases
- **Clustering**: Grouping related sequences
- **Duplicate Detection**: Finding near-duplicate sequences

### Phylogenetic Studies
- **Evolutionary Distance**: Measuring sequence relationships
- **Species Classification**: Taxonomic grouping
- **Comparative Genomics**: Cross-species analysis

### Clinical Applications
- **Variant Detection**: Identifying genomic variations
- **Pathogen Classification**: Rapid microbial identification
- **Drug Resistance**: Analyzing resistance patterns

## Configuration

### MinHash Configuration
```cpp
#define MINHASH_FUNCTIONS 32           // Number of hash functions
static int g_minhash_token_length = 13; // K-mer length

// Runtime configuration
void set_minhash_token_length(int length);
```

### SimHash Configuration
```cpp
// Token length (in bases)
uint32_t bases_in_token = 11;          // Default: 11 bases (22 bits)
uint32_t bases_per_byte = 4;           // 2 bits per base
```

## Hash Registration

### MinHash Registration
```cpp
REGISTER_HASH(MinHash_32_bitwise,
   $.desc            = "MINHASH 32-bit version",
   $.hash_flags      = FLAG_HASH_LOCAL_SENSITIVE | 
                       FLAG_HASH_TOKENISATION_PROPERTY | 
                       FLAG_HASH_VARIABLE_OUTPUT_SIZE,
   $.impl_flags      = FLAG_IMPL_MULTIPLY | FLAG_IMPL_SLOW,
   $.bits            = 32,
   $.hashfn_varout_native = minhash32_bitwise<false>,
   $.hashfn_varout_bswap  = minhash32_bitwise<true>
);
```

### SimHash Registration
```cpp
REGISTER_HASH(SimHash_32_bitwise,
   $.desc            = "SIMHASH 32-bit version", 
   $.hash_flags      = FLAG_HASH_LOCAL_SENSITIVE | 
                       FLAG_HASH_TOKENISATION_PROPERTY,
   $.impl_flags      = FLAG_IMPL_MULTIPLY | FLAG_IMPL_SLOW,
   $.bits            = 32,
   $.hashfn_native   = simhash32_bitwise<false>,
   $.hashfn_bswap    = simhash32_bitwise<true>
);
```



### File Structure
```
BioHasher/
├── hashes/           # Hash function implementations
│   ├── minhash.cpp   # MinHash implementation
│   └── simhash.cpp   # SimHash implementation
├── tests/            # Test implementations
│   └── LSHCollisionTest.cpp
├── include/          # Header files
└── README.md         # This file
```

## Legacy SMHasher3 Information

This project builds upon SMHasher3, a tool for testing the quality of [hash functions](https://en.wikipedia.org/wiki/Hash_function) in terms of their distribution, collision, and performance properties.

### Original SMHasher3 Usage
- `./SMHasher3 --tests` will show all available test suites
- `./SMHasher3 --list` will show all available hashes and their descriptions
- `./SMHasher3 <hashname>` will test the given hash with the default set of test suites

## License

This project builds upon the SMHasher3 framework. Please refer to individual source files for specific licensing information.

## References

- **MinHash**: Broder, A. Z. (1997). "On the resemblance and containment of documents"
- **SimHash**: Charikar, M. S. (2002). "Similarity estimation techniques from rounding algorithms"
- **SMHasher**: Original hash testing framework
- **Genomic Hashing**: Specialized applications in bioinformatics

