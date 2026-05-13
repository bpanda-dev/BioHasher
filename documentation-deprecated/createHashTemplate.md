# Hash Template Generator for BioLSHasher
>Needs rework. Not to be used for now.

Filename : `createHashTemplate.py`

We provide an interactive command-line script that scaffolds a new C++ hash function file and registers it within the BioLSHasher build system. It walks you through **11 guided steps**, validates every input, generates a ready-to-compile `.cpp` file in the `hashes/` directory, and automatically updates `hashes/Hashsrc.cmake` so the new hash is included in the next build.

---

## Quick Start

```bash
# From the BioLSHasher root directory
python3 createHashTemplate.py
```

Follow the interactive prompts. At the end you will have:

1. A new file `hashes/<hashname>.cpp` containing a compilable template with your hash function stub, similarity function (built-in or custom stub), and all registration macros.
2. An updated `hashes/Hashsrc.cmake` with the new file registered.

Then build and verify:

```bash
mkdir -p build && cd build
cmake ..
make
./BioLSHasher --list | grep <YourHashName>
```

---

## Step-by-Step Walkthrough

### Step 1 — Hash Name

**Prompt:** `Hash name:`

The primary identifier for your hash function. This name is used for:

- The C++ function name (e.g., `MyHash(...)`)
- The `REGISTER_HASH(MyHash, ...)` macro call
- The output filename (`myhash.cpp`, automatically lowercased)

**Validation rules:**

| Rule               | Constraint                                                        |
|--------------------|-------------------------------------------------------------------|
| Length             | 2–50 characters                                                   |
| First character    | Must be a letter (`a-z`, `A-Z`)                                   |
| Allowed characters | Letters, digits, underscores only (`^[a-zA-Z][a-zA-Z0-9_]*$`)     |
| Reserved words     | Cannot be a C++ keyword (`if`, `class`, `void`, `template`, etc.) |

---

### Step 2 — Author Name

**Prompt:** `Author name:`

Used in the copyright header of the generated file:

```cpp
/*
* MyHash
* Copyright (C) 2026 <Your Name Here>
* ...
*/
```

---

### Step 3 — License

**Prompt:** `Enter choice [1-8] or press Enter for MIT License:`

Select the open-source license for the copyright header. Pressing **Enter** defaults to **MIT License**.

| Choice | License                   |
|--------|---------------------------|
| 1      | MIT License **(default)** |
| 2      | Apache 2.0                |
| 3      | BSD 2-Clause              |
| 4      | BSD 3-Clause              |
| 5      | GPL v2                    |
| 6      | GPL v3                    |
| 7      | Public Domain / CC0       |
| 8      | Custom                    |

If you select **8 (Custom)**, you will be prompted to enter free-form license text.

---

### Step 4 — Hash Family Name

**Prompt:** `Family name [<HashName>]:`

The family name groups related hash variants under a single `REGISTER_FAMILY(...)` block. Press **Enter** to reuse the hash name from Step 1.

This is useful when you have multiple bit-width variants (e.g., `MyHash_32`, `MyHash_64`) that belong to the same logical family `MyHash`.

**Validation:** Same rules as Hash Name.

---

### Step 5 — Repository URL

**Prompt:** `Repository URL [https://github.com/username/repo_name]:`

The URL of the source repository where the hash implementation originates. This is stored in the `REGISTER_FAMILY` macro:

```cpp
REGISTER_FAMILY(MyHash,
$.src_url = "https://github.com/you/your-hash",
$.src_status = HashFamilyInfo::SRC_STABLEISH
);
```

**Validation:**

| Rule       | Constraint                                                |
|------------|-----------------------------------------------------------|
| Format     | Must start with `http://` or `https://`                   |
| Length     | ≤ 500 characters                                          |
| Characters | No whitespace, angle brackets, or shell-unsafe characters |

**Default:** `https://github.com/username/repo_name` (press Enter to accept).
  
---

### Step 6 — Source Status

**Prompt:** `Select source status:`
Indicates the maturity level of the hash implementation. This is embedded in the `REGISTER_FAMILY` macro as `HashFamilyInfo::SRC_<STATUS>`.

| Choice      | Macro Value     | Meaning                                                     |
|-------------|-----------------|-------------------------------------------------------------|
| `UNKNOWN`   | `SRC_UNKNOWN`   | No information available about the source status            |
| `FROZEN`    | `SRC_FROZEN`    | Code is finalized; no further changes expected              |
| `STABLEISH` | `SRC_STABLEISH` | Code is stable but minor updates are possible **(default)** |
| `ACTIVE`    | `SRC_ACTIVE`    | Code is actively being developed                            |

You can enter the choice number (`1`–`4`) or type the status name directly (e.g., `ACTIVE`).

---

### Step 7 — Description

**Prompt:** `Description:`

A brief, human-readable description of your hash function. Useful to provide description to new users about the hash function. It appears in the `REGISTER_HASH` macro:

```cpp
$.desc = "Your description here",
```

---

### Step 8 — Hash Output Size (Bits)

**Prompt:** `Enter choice(s) [1-6] or press Enter for 32 bits:`

Select one or more output bit sizes. You can choose multiple variants in a single run.

| Choice | Bit size                                 |
|--------|------------------------------------------|
| 1      | 32 bits **(default)**                    |
| 2      | 64 bits                                  |
| 3      | 128 bits                                 |
| 4      | 256 bits                                 |
| 5      | 512 bits                                 |
| 6      | Custom (must be a multiple of 8, ≤ 1024) |

**Multiple selection:** Separate choices with commas.

| Input       | Result                     |
|-------------|----------------------------|
| `1`         | 32-bit variant only        |
| `1,2`       | 32-bit and 64-bit variants |
| `32,64,128` | Direct bit values work too |

When multiple sizes are selected, the script generates:

- A **separate C++ function** for each size (suffixed: `MyHash_32`, `MyHash_64`, ...)
- A **separate `REGISTER_HASH`** block for each size
- A **single shared `REGISTER_FAMILY`** block

---

### Step 9 — LSH Candidacy

**Prompt:** `Is this hash an LSH candidate to be tested for LSH properties? [Y/n]:`

BioLSHasher is a framework specifically designed for testing Locality-Sensitive Hashing (LSH) functions. All test suites in BioLSHasher evaluate LSH properties.

- **If you answer `Y` (default):** The hash is marked as an LSH candidate and `FLAG_HASH_LOCALITY_SENSITIVE` will be set in the generated `REGISTER_HASH` block.
- **If you answer `N`:** The script exits immediately with the following message:

```
============================================================
  BioLSHasher only contains tests related to Locality-Sensitive
  Hashing (LSH). Non-LSH hash functions are not supported by
  the current test infrastructure.

  If you believe your hash has LSH properties, please rerun
  this script and select 'Y' at the LSH candidacy step.
============================================================
```

> **Note:** This is the only step where the script can exit early. All other steps re-prompt on invalid input.

---

### Step 10 — Similarity Name

**Prompt:** `Similarity name:`

Enter the name of the similarity measure that your hash function preserves. BioLSHasher has built-in implementations for the following similarity metrics:

| Built-in Name | Description                                |
|---------------|--------------------------------------------|
| `Hamming`     | Fraction of matching positions (equal-length sequences) |
| `Jaccard`     | Set intersection over set union            |
| `Cosine`      | Cosine of the angle between frequency vectors |
| `Angular`     | Normalised angular distance                |
| `Edit`        | 1 − (edit distance / max length)           |

You may also enter a **custom similarity name** if your metric is not listed above (e.g., `MinHash`, `Containment`).

**Validation:**

| Rule               | Constraint                                                   |
|--------------------|--------------------------------------------------------------|
| Length             | 1–50 characters                                              |
| First character    | Must be a letter                                             |
| Allowed characters | Letters, digits, underscores only (`^[a-zA-Z][a-zA-Z0-9_]*$`) |

> **Tip:** For edit similarity, just write `Edit` — not `Edit Similarity` or `EditSimilarity`.

---

### Step 11 — Similarity Function

This step behaves differently depending on whether the similarity name from Step 10 is a built-in or custom metric.

**If built-in** (e.g., `Edit`, `Hamming`, etc.):

The script automatically sets the function name and informs you that the implementation will be included in the generated file:

```
'Edit' is a built-in similarity metric in BioLSHasher.
The implementation of 'EditSimilarity' will be included in your hash file.
  → Similarity function set to: EditSimilarity
```

No further input is needed. The full C++ implementation of the similarity function will be injected into your hash file.

**If custom:**

**Prompt:** `Similarity function name:`

You are asked to name your C++ similarity function. This function must follow the `SimilarityFn` signature:

```cpp
double FnName(const std::string& seq1, const std::string& seq2,
              const uint32_t in1_len, const uint32_t in2_len);
```

The script generates a stub function in the output file that you need to implement:

```cpp
//------------------------------------------------------------
// MyMetricSimilarity: Custom similarity function — implement your logic here.
// Must return a value in [0.0, 1.0] where 1.0 means identical.
double MyMetricSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t in1_len, const uint32_t in2_len) {
    // TODO: Implement your similarity computation.
    return 0.0;
}
```

**Validation:** Must be a valid C++ identifier (letters, digits, underscores; starts with a letter or underscore).

---

### Generated C++ File Structure

For a hash named `MyHash` with **32-bit and 64-bit** variants using **Edit** similarity (built-in), the generated `hashes/myhash.cpp` looks like:

```cpp
/*
 * MyHash
 * Copyright (C) 2026 Your Name
 *
 * Licensed under the MIT License.
 */
#include "specifics.h"
#include "Hashlib.h"
#include "LSHGlobals.h"

#include <vector>
#include <cassert>
#include <cmath>
#include <set>
#include <map>
//------------------------------------------------------------
// MyHash implementation

//------------------------------------------------------------
static void MyHash_32( const void * in, const size_t len, const seed_t seed, void * out ) {
    // Output: 32 bits
    uint32_t hash = 0;
    // Copy the hash state to the output.
    PUT_U32(hash, (uint8_t *)out, 0);
}

//------------------------------------------------------------
static void MyHash_64( const void * in, const size_t len, const seed_t seed, void * out ) {
    // Output: 64 bits
    uint64_t hash = 0;
    // Copy the hash state to the output.
    PUT_U64(hash, (uint8_t *)out, 0);
}

//------------------------------------------------------------
// Edit Similarity: 1 - (edit_distance / max_length)
double EditSimilarity(const std::string& seq1, const std::string& seq2, const uint32_t in1_len, const uint32_t in2_len) {
    // ... full implementation auto-generated ...
}

//------------------------------------------------------------
REGISTER_FAMILY(MyHash,
   $.src_url    = "https://github.com/you/your-hash",
   $.src_status = HashFamilyInfo::SRC_STABLEISH
 );

REGISTER_HASH(MyHash_32,
   $.desc            = "My custom hash (32-bit)",
   $.hash_flags      =
         FLAG_HASH_LOCALITY_SENSITIVE,
   $.impl_flags      =
         0,
   $.bits            = 32,
   $.hashfn          = MyHash_32,
   $.similarity_name = "Edit",
   $.similarityfn    = EditSimilarity
 );

REGISTER_HASH(MyHash_64,
   $.desc            = "My custom hash (64-bit)",
   $.hash_flags      =
         FLAG_HASH_LOCALITY_SENSITIVE,
   $.impl_flags      =
         0,
   $.bits            = 64,
   $.hashfn          = MyHash_64,
   $.similarity_name = "Edit",
   $.similarityfn    = EditSimilarity
 );
```

If you select a **single bit size**, the suffix is omitted (e.g., `MyHash` instead of `MyHash_32`).

---

### Non-Standard Bit Sizes

For bit sizes other than 32 or 64 (e.g., 128, 256, 512), the generator creates multiple `uint64_t` / `uint32_t` variables with warning comments prompting you to adapt the storage and copy mechanism:

```cpp
static void MyHash_128( const void * in, const size_t len, const seed_t seed, void * out ) {
    // Output: 128 bits
    // WARNING: This is not a standard output size (32 or 64 bits).
    // Please change the datatype of hash value storage and
    // the memory copy mechanism below according to your implementation.
    //
    uint64_t hash0 = 0;  // bits 0 to 63
    uint64_t hash1 = 0;  // bits 64 to 127
    // Copy the hash state to the output.
    PUT_U64(hash0, (uint8_t *)out, 0);
    PUT_U64(hash1, (uint8_t *)out, 8);
}
```

---

### Hashsrc.cmake Update

The script automatically appends the new file to `hashes/Hashsrc.cmake`:

```cmake
set(HASH_SRC_FILES
  ...
  hashes/myhash.cpp    # ← newly added
)
```

The entry is inserted just before the closing `)` of the `set()` command. If the entry already exists, it is skipped with a message.

---

## Validation Rules Summary

All inputs are validated before proceeding. Here is a summary:

| Field               | Min Length | Max Length | Pattern / Constraints                      |
|---------------------|------------|------------|---------------------------------------------|
| Hash Name           | 2          | 50         | `^[a-zA-Z][a-zA-Z0-9_]*$`, no C++ keywords |
| Author Name         | 2          | 100        | `^[a-zA-Z][a-zA-Z\s\-'.]*$`                |
| Family Name         | 2          | 50         | Same as Hash Name                           |
| Repository URL      | —          | 500        | Must match `^https?://...`                  |
| Description         | 5          | 200        | No unescaped double quotes                  |
| Bit Size            | —          | —          | Positive integer, multiple of 8, ≤ 1024     |
| Similarity Name     | 1          | 50         | `^[a-zA-Z][a-zA-Z0-9_]*$`                  |
| Similarity Function | 1          | 100        | Valid C++ identifier                        |

Invalid inputs will display an error message and re-prompt.

---

## After Running the Script

Once the file is generated, follow these steps:

### 1. Implement Your Hash Logic

Open `hashes/<hashname>.cpp` and replace the placeholder code inside the hash function with your actual hash implementation:

```cpp
static void MyHash( const void * in, const size_t len, const seed_t seed, void * out ) {
    // TODO: Replace this with your hash logic
    uint32_t hash = 0;

    // Your implementation goes here...
    // 'in'   = pointer to input data (genomic sequence, unencoded)
    // 'len'  = length of input in bytes
    // 'seed' = seed value for the hash

    PUT_U32(hash, (uint8_t *)out, 0);
}
```

### 2. Implement Custom Similarity (if applicable)

If you entered a custom similarity name in Step 10, the generated file contains a stub function that returns `0.0`. You need to implement your similarity logic there. The function must return a value in `[0.0, 1.0]` where `1.0` means identical.

If you chose a built-in similarity metric (Hamming, Jaccard, Cosine, Angular, or Edit), the full implementation is already included — no additional work needed.

### 3. Build

```bash
cd build
cmake ..
make
```

### 4. Verify Registration

```bash
./BioLSHasher --list | grep MyHash
```

You should see your hash listed in the output.

---

## Example Session

```bash
============================================================
  BioLSHasher - New Hash File Template Generator
============================================================

[Step 1/11] Hash Name
----------------------------------------
Enter the name for your hash function.
Rules: Must start with a letter, only letters/numbers/underscores allowed. No C++ keywords
Hash name: BioMinHash

[Step 2/11] Author Name
----------------------------------------
Enter your name for the copyright notice.
Author name: Jane Doe

[Step 3/11] License
----------------------------------------
Select a license for your hash implementation.

Available license options:
  1. MIT License
  2. Apache 2.0
  3. BSD 2-Clause
  4. BSD 3-Clause
  5. GPL v2
  6. GPL v3
  7. Public Domain
  8. Custom
Enter choice [1-8] or press Enter for MIT License: 

[Step 4/11] Hash Family Name
----------------------------------------
Enter the hash family name (can be same as hash name).
Press Enter to use 'BioMinHash' as the family name.
Family name [BioMinHash]:

[Step 5/11] Repository URL
----------------------------------------
Enter the URL of your source code repository.
Repository URL [https://github.com/username/repo_name]: https://github.com/janedoe/biominhash

[Step 6/11] Source Status
----------------------------------------
Select the source code status.
  UNKNOWN   - No Information available
  FROZEN    - Code is finalized, no changes expected
  STABLEISH - Code is stable but minor updates possible
  ACTIVE    - Code is actively being developed

Select source status:
Available options:
  1. UNKNOWN
  2. FROZEN
  3. STABLEISH (default)
  4. ACTIVE
Enter choice [1-4] or press Enter for default: 4

[Step 7/11] Hash Description
----------------------------------------
Enter a brief description of your hash function.
Description: Biological MinHash for sequence similarity

[Step 8/11] Hash Output Size
----------------------------------------
Select the hash output size in bits.

Common hash bit sizes:
  1. 32 bits (default)
  2. 64 bits
  3. 128 bits
  4. 256 bits
  5. 512 bits
  6. Custom
Enter choice(s) [1-6] or press Enter for 32 bits: 1,2
  Selected: 32 bits, 64 bits
  Confirm selection? [Y/n]: y

[Step 9/11] LSH Candidacy
----------------------------------------
BioLSHasher is a framework for evaluating Locality-Sensitive Hashing (LSH) functions.
All test suites in BioLSHasher are designed to test LSH properties.

Is this hash an LSH candidate to be tested for LSH properties? [Y/n]: Y
  Marked as LSH candidate. FLAG_HASH_LOCALITY_SENSITIVE will be set.

[Step 10/11] Similarity Name
----------------------------------------
Enter the name of the similarity measure that this hash function preserves.
Built-in options: Hamming, Jaccard, Cosine, Angular, Edit
You may also enter a custom similarity name if yours is not listed above.

  Note: If your hash function uses edit similarity, just write 'Edit'
        — not 'Edit Similarity' or 'EditSimilarity'.

Similarity name: Jaccard

[Step 11/11] Similarity Function
----------------------------------------
'Jaccard' is a built-in similarity metric in BioLSHasher.
The implementation of 'JaccardSimilarity' will be included in your hash file.
  → Similarity function set to: JaccardSimilarity

============================================================
  Summary - Please Review
============================================================
  Hash Name:     BioMinHash
  Family Name:   BioMinHash
  Author:        Jane Doe
  License:       Licensed under the MIT License...
  Repository:    https://github.com/janedoe/biominhash
  Source Status: ACTIVE
  Description:   Biological MinHash for sequence similarity
  Bit Sizes:     32, 64 (2 variants)
  LSH Candidate: Yes
  Similarity:    Jaccard
  Similarity Fn: JaccardSimilarity
  Output File:   /path/to/BioLSHasher/hashes/biominhash.cpp
============================================================

Create this file? [Y/n]: y

 Successfully created: /path/to/BioLSHasher/hashes/biominhash.cpp

 Updating Hashsrc.cmake...
   Added 'hashes/biominhash.cpp' to Hashsrc.cmake

 Next steps:
    1. To test if your BioMinHashHash function has been added to BioLSHasher:
    2. Build BioLSHasher using $mkdir build > $cd build > $make
    3. run BioLSHasher using, ./BioLSHasher --list | grep BioMinHash. It should list your hash.
    4. Implement the logic of your Hash in the BioMinHashHash function.
```

---

## Troubleshooting

| Problem                                     | Cause                   | Solution                                                                                |
|---------------------------------------------|-------------------------|-----------------------------------------------------------------------------------------|
| `Warning: Could not find .../Hashsrc.cmake` | Script not at repo root | Run from the BioLSHasher root directory                                                   |
| File already exists prompt                  | Hash name already taken | Choose a different name or confirm overwrite                                            |
| Hash not showing in `--list`                | CMake cache stale       | Delete build and rebuild: `rm -rf build && mkdir build && cd build && cmake .. && make` |
| Script exits at Step 9                      | Hash is not LSH         | BioLSHasher only tests LSH functions; rerun and answer `Y` if applicable                  |

---

## Relevant File Layout

```bash
BioLSHasher/
├── createHashTemplate.py          ← This script
├── hashes/
│   ├── Hashsrc.cmake              ← Build file list (auto-updated)
│   ├── EXAMPLE.cpp                ← Manual template reference
│   ├── EXAMPLE-mit.cpp            ← Manual template reference (MIT)
│   ├── myhash.cpp                 ← Generated output (example)
│   └── ...
└── build/
    └── ...
```

---

## Notes

- The script can be cancelled at any point with `Ctrl+C` — no files will be written until final confirmation.
- If you select a **single bit size**, the function and registration use the bare hash name (e.g., `MyHash`). If you select **multiple bit sizes**, each variant gets a suffix (e.g., `MyHash_32`, `MyHash_64`).
- If you choose a **built-in similarity metric**, the full C++ implementation is injected into the generated file along with the necessary `#include` headers. If you choose a **custom metric**, a stub function is generated that you must implement.
- The only early exit point is **Step 9 (LSH Candidacy)** — if you indicate the hash is not an LSH candidate, the script terminates because BioLSHasher's test suites only evaluate LSH properties.