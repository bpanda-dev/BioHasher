# Hash Template Generator for BioHasher

Filename : `createHashTemplate.py`

We provide an interactive command-line script that scaffolds a new C++ hash function file and registers it within the BioHasher build system. It walks you through **8 guided steps**, validates every input, generates a ready-to-compile `.cpp` file in the `hashes/` directory, and automatically updates `hashes/Hashsrc.cmake` so the new hash is included in the next build.

---
## Quick Start

```bash
# From the BioHasher root directory
python3 createHashTemplate.py
```

Follow the interactive prompts. At the end you will have:
1. A new file `hashes/<hashname>.cpp` containing a compilable template.
2. An updated `hashes/Hashsrc.cmake` with the new file registered.

Then build and verify:

```bash
mkdir -p build && cd build
cmake ..
make
./SMHasher3 --list | grep <YourHashName>
```

---
## Step-by-Step Walkthrough
### Step 1 — Hash Name
**Prompt:** `Hash name:`

The primary identifier for your hash function. This name is used for:
- The C++ template function name (e.g., `MyHash<bswap>(...)`)
- The `REGISTER_HASH(MyHash, ...)` macro call
- The output filename (`myhash.cpp`, automatically lowercased)

**Validation rules:**

| Rule               | Constraint                                                        |
| ------------------ | ----------------------------------------------------------------- |
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
| ------ | ------------------------- |
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
| ---------- | --------------------------------------------------------- |
| Format     | Must start with `http://` or `https://`                   |
| Length     | ≤ 500 characters                                          |
| Characters | No whitespace, angle brackets, or shell-unsafe characters |

**Default:** `https://github.com/username/repo_name` (press Enter to accept).
  
---
### Step 6 — Source Status
**Prompt:** `Select source status:`
Indicates the maturity level of the hash implementation. This is embedded in the `REGISTER_FAMILY` macro as `HashFamilyInfo::SRC_<STATUS>`.

| Choice      | Macro Value     | Meaning                                                     |
| ----------- | --------------- | ----------------------------------------------------------- |
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
| ------ | ---------------------------------------- |
| 1      | 32 bits **(default)**                    |
| 2      | 64 bits                                  |
| 3      | 128 bits                                 |
| 4      | 256 bits                                 |
| 5      | 512 bits                                 |
| 6      | Custom (must be a multiple of 8, ≤ 1024) |
**Multiple selection:** Separate choices with commas.

| Input       | Result                     |
| ----------- | -------------------------- |
| `1`         | 32-bit variant only        |
| `1,2`       | 32-bit and 64-bit variants |
| `32,64,128` | Direct bit values work too |

When multiple sizes are selected, the script generates:

- A **separate C++ function** for each size (suffixed: `MyHash_32`, `MyHash_64`, ...)
- A **separate `REGISTER_HASH`** block for each size
- A **single shared `REGISTER_FAMILY`** block

---

### Generated C++ File Structure

For a hash named `MyHash` with **32-bit and 64-bit** variants, the generated `hashes/myhash.cpp` looks like:

```cpp
/*
 * MyHash
 * Copyright (C) 2026 Your Name
 *
 * Licensed under the MIT License.
 */
#include "Platform.h"
#include "Hashlib.h"

//------------------------------------------------------------
// MyHash implementation

//------------------------------------------------------------
template <bool bswap>
static void MyHash_32( const void * in, const size_t len, const seed_t seed, void * out ) {
    // Output: 32 bits
    uint32_t hash = 0;
    // Copy the hash state to the output.
    PUT_U32<bswap>(hash, (uint8_t *)out, 0);
}

//------------------------------------------------------------
template <bool bswap>
static void MyHash_64( const void * in, const size_t len, const seed_t seed, void * out ) {
    // Output: 64 bits
    uint64_t hash = 0;
    // Copy the hash state to the output.
    PUT_U64<bswap>(hash, (uint8_t *)out, 0);
}

//------------------------------------------------------------
REGISTER_FAMILY(MyHash,
   $.src_url    = "https://github.com/you/your-hash",
   $.src_status = HashFamilyInfo::SRC_STABLEISH
 );

REGISTER_HASH(MyHash_32,
   $.desc            = "My custom hash (32-bit)",
   $.hash_flags      =
         0,
   $.impl_flags      =
         0,
   $.bits            = 32,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = MyHash_32<false>,
   $.hashfn_bswap    = MyHash_32<true>
 );

REGISTER_HASH(MyHash_64,
   $.desc            = "My custom hash (64-bit)",
   $.hash_flags      =
         0,
   $.impl_flags      =
         0,
   $.bits            = 64,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = MyHash_64<false>,
   $.hashfn_bswap    = MyHash_64<true>
 );
```
If you select a **single bit size**, the suffix is omitted (e.g., `MyHash` instead of `MyHash_32`).

---

### Non-Standard Bit Sizes

For bit sizes other than 32 or 64 (e.g., 128, 256, 512), the generator creates multiple `uint64_t` / `uint32_t` variables with warning comments prompting you to adapt the storage and copy mechanism:

```cpp
template <bool bswap>
static void MyHash_128( const void * in, const size_t len, const seed_t seed, void * out ) {
    // Output: 128 bits
    // WARNING: This is not a standard output size (32 or 64 bits).
    // Please change the datatype of hash value storage and
    // the memory copy mechanism below according to your implementation.
    //
    uint64_t hash0 = 0;  // bits 0 to 63
    uint64_t hash1 = 0;  // bits 64 to 127
    // Copy the hash state to the output.
    PUT_U64<bswap>(hash0, (uint8_t *)out, 0);
    PUT_U64<bswap>(hash1, (uint8_t *)out, 8);
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

| Field | Min Length | Max Length | Pattern / Constraints |
|---|---|---|---|
| Hash Name | 2 | 50 | `^[a-zA-Z][a-zA-Z0-9_]*$`, no C++ keywords |
| Author Name | 2 | 100 | `^[a-zA-Z][a-zA-Z\s\-'.]*$` |
| Family Name | 2 | 50 | Same as Hash Name |
| Repository URL | — | 500 | Must match `^https?://...` |
| Description | 5 | 200 | No unescaped double quotes |
| Bit Size | — | — | Positive integer, multiple of 8, ≤ 1024 |

Invalid inputs will display an error message and re-prompt.

---

## After Running the Script

Once the file is generated, follow these steps:

### 1. Implement Your Hash Logic

Open `hashes/<hashname>.cpp` and replace the placeholder code inside the template function with your actual hash implementation:

```cpp
template <bool bswap>
static void MyHash( const void * in, const size_t len, const seed_t seed, void * out ) {
    // TODO: Replace this with your hash logic
    uint32_t hash = 0;

    // Your implementation goes here...
    // 'in'   = pointer to input data
    // 'len'  = length of input in bytes
    // 'seed' = seed value for the hash

    PUT_U32<bswap>(hash, (uint8_t *)out, 0);
}
```

### 2. Build

```bash
cd build
cmake ..
make
```

### 3. Verify Registration

```bash
./SMHasher3 --list | grep MyHash
```

You should see your hash listed in the output.

<!-- ### 4. Update Verification Codes (Optional)

After your implementation is complete, run BioHasher's verification tests to generate the correct `verification_LE` and `verification_BE` values, then update them in the `REGISTER_HASH` block (they default to `0x0`). -->

---

## Example Session

```
============================================================
  BioHasher - New Hash File Generator
============================================================

[Step 1/8] Hash Name
----------------------------------------
Enter the name for your hash function.
Rules: Must start with a letter, only letters/numbers/underscores allowed.
Hash name: BioMinHash

[Step 2/8] Author Name
----------------------------------------
Enter your name for the copyright notice.
Author name: Jane Doe

[Step 3/8] License
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

[Step 4/8] Hash Family Name
----------------------------------------
Enter the hash family name (can be same as hash name).
Press Enter to use 'BioMinHash' as the family name.
Family name [BioMinHash]:

[Step 5/8] Repository URL
----------------------------------------
Enter the URL of your source code repository.
Repository URL [https://github.com/username/repo_name]: https://github.com/janedoe/biominhash

[Step 6/8] Source Status
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

[Step 7/8] Hash Description
----------------------------------------
Enter a brief description of your hash function.
Description: Biological MinHash for sequence similarity

[Step 8/8] Hash Output Size
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
  Output File:   /path/to/BioHasher/hashes/biominhash.cpp
============================================================

Create this file? [Y/n]: y

 Successfully created: /path/to/BioHasher/hashes/biominhash.cpp

 Updating Hashsrc.cmake...
   Added 'hashes/biominhash.cpp' to Hashsrc.cmake

 Next steps:
    1. To test if your BioMinHashHash function has been added to BioHasher:
    2. Build BioHasher using $mkdir build > $cd build > $make
    3. run BioHasher using, ./BioHasher --list | grep BioMinHash. It should list your hash.
    4. Implement the logic of your Hash in the BioMinHashHash function.
```

---

## Troubleshooting

| Problem | Cause | Solution |
|---|---|---|
| `Warning: Could not find .../Hashsrc.cmake` | Script not at repo root | Run from the BioHasher root directory |
| File already exists prompt | Hash name already taken | Choose a different name or confirm overwrite |
| Hash not showing in `--list` | CMake cache stale | Delete build and rebuild: `rm -rf build && mkdir build && cd build && cmake .. && make` |

---

## File Layout

```
BioHasher/
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
- For reference, you can also look at `hashes/EXAMPLE.cpp` and `hashes/EXAMPLE-mit.cpp` as manual template references that show the same structure the script generates.

<!-- - The `verification_LE` and `verification_BE` fields default to `0x0`. Update these after your implementation is complete and verified. -->