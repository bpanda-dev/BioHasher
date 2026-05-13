/*
 * seekable_rng_test.cpp
 *
 * Minimal example demonstrating O(1) seekable random number generation
 * using the existing Rand class from smhasher3Random.h.
 *
 * Compile (from the lib/ directory):
 *   g++ -std=c++17 -O2 -DBARE_RNG \
 *       -include type_traits -include vector \
 *       -o seekable_rng_test \
 *       seekable_rng_test.cpp smhasher3Random.cpp specifics.cpp
 *
	// g++ -std=c++17 -O2 -DBARE_RNG \  
    // -include type_traits -include vector \
    // -o seekable_rng_test \
    // seekable_rng_test.cpp smhasher3Random.cpp specifics.cpp
 * Usage:
 *   ./seekable_rng_test <seed> <index>
 */
//  g++ -std=c++17 -O2 -DBARE_RNG \
//  	-include type_traits -include vector \
//  	-o seekable_rng \
//  	seekable_rng.cpp smhasher3Random.cpp specifics.cpp

#include "../lib/smhasher3Random.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: %s <seed> <index>\n\n", argv[0]);
    printf("  seed   - 64-bit seed value (decimal)\n");
    printf("  index  - 0-based index of the random number to retrieve\n\n");
    printf("Example: %s 42 1000000\n", argv[0]);
    return 1;
  }

  const uint64_t seed = strtoull(argv[1], nullptr, 10);
  const uint64_t index = strtoull(argv[2], nullptr, 10);

  printf("=== Seekable RNG Demo (Threefry-4x64 CBRNG) ===\n\n");
  printf("  Seed  : %llu\n", (unsigned long long)seed);
  printf("  Index : %llu\n\n", (unsigned long long)index);

  // ---- Method 1: O(1) direct seek ----
  auto t0 = std::chrono::high_resolution_clock::now();

  Rand rng1(seed);
  rng1.seek(index); // O(1) — sets counter directly
  uint64_t seeked_value = rng1.rand_u64();

  auto t1 = std::chrono::high_resolution_clock::now();
  double seek_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

  printf("[O(1) Seek]  rng(seed=%llu, index=%llu) = 0x%016llX\n",
         (unsigned long long)seed, (unsigned long long)index,
         (unsigned long long)seeked_value);
  printf("             Time: %.3f µs\n\n", seek_us);

  // ---- Method 2: Sequential iteration (verification) ----
  if (index <= 10'000'000) {
    auto t2 = std::chrono::high_resolution_clock::now();

    Rand rng2(seed);
    uint64_t sequential_value = 0;
    for (uint64_t i = 0; i <= index; i++) {
      sequential_value = rng2.rand_u64();
    }

    auto t3 = std::chrono::high_resolution_clock::now();
    double seq_us = std::chrono::duration<double, std::micro>(t3 - t2).count();

    printf("[Sequential] rng(seed=%llu, index=%llu) = 0x%016llX\n",
           (unsigned long long)seed, (unsigned long long)index,
           (unsigned long long)sequential_value);
    printf("             Time: %.3f µs\n\n", seq_us);

    if (seeked_value == sequential_value) {
      printf("MATCH — O(1) seek produces the same value as sequential "
             "generation.\n");
    } else {
      printf("MISMATCH — Something is wrong!\n");
      return 1;
    }

    if (seek_us > 0) {
      printf("Speedup: %.1fx faster via seek\n", seq_us / seek_us);
    }
  } else {
    printf("[Sequential] Skipped (index > 10M).\n");
  }

  // ---- Show a few values around the requested index ----
  printf("\n--- Values around index %llu ---\n", (unsigned long long)index);
  Rand rng3(seed);
  uint64_t start = (index >= 3) ? index - 3 : 0;
  for (uint64_t i = start; i <= index + 3; i++) {
    rng3.seek(i);
    uint64_t val = rng3.rand_u64();
    const char *marker = (i == index) ? " <-- requested" : "";
    printf("  [%llu] = 0x%016llX%s\n", (unsigned long long)i,
           (unsigned long long)val, marker);
  }

  return 0;
}
