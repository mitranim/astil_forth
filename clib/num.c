#pragma once
#include "./err.c"
#include "./num.h"
#include <limits.h>

/*
Used be defined as a generic macro, but the `clang-tidy` analyzer likes
to complain about this code A LOT and macros are more annoying to debug.

Clang doesn't seem to support `__builtin_stdc_bit_ceil` yet.

Accidentally, this limits some of our memory allocations
to 2 GiB, which is more than sufficient for now.

0 -> 0
1 -> 1
2 -> 2
3 -> 4
...
*/
static Ind round_up_pow2_Ind(Ind src) {
  if (!src) return 0;

  constexpr U8 bit_cap = sizeof(src) * CHAR_BIT;

  // Overflow: no higher power of 2. `(Ind)1 << (Ind)32` is UB.
  if (src > ((Ind)1 << (U8)(bit_cap - 1))) return 0;

  const U8 bit_len = bit_cap - (U8)__builtin_clzg((src - (Ind)1), bit_cap);

  return (Ind)1 << bit_len;
}

/*
#include <stdio.h>

int main(void) {
  for (Ind ind = 0; ind < 21; ind++) {
    printf(
      "round_up_pow2_Ind(" FMT_IND "): " FMT_IND "\n", ind, round_up_pow2_Ind(ind)
    );
  }
}
*/
