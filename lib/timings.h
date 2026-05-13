#include <time.h>
#include <sys/time.h>
#include "HashInfo.h"

#define NSEC_PER_SEC 1000000000ULL


FORCE_INLINE static uint64_t monotonic_clock(void) {
  struct timespec ts;
  uint64_t result;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    return -10;

  result = ts.tv_sec * NSEC_PER_SEC;
  result += ts.tv_nsec;

  return result;
}


static FORCE_INLINE void cycle_timer_init() {
}

static FORCE_INLINE uint64_t cycle_timer_start() {
    return monotonic_clock();
}

static FORCE_INLINE uint64_t cycle_timer_end() {
    return monotonic_clock();
}


