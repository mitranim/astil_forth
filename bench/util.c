#pragma once

#include <stdint.h>

static inline uint64_t escape_u64(uint64_t val) {
  register uint64_t x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0));
  return x0;
}

static inline unsigned __int128 escape_u128(unsigned __int128 val) {
  uint64_t low  = (uint64_t)val;
  uint64_t high = (uint64_t)(val >> 64);
  __asm__ volatile("" : "+r"(low), "+r"(high));
  return ((unsigned __int128)high << 64) | low;
}

static inline void *escape_ptr(void *val) {
  register void *x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0) : : "memory");
  return x0;
}
