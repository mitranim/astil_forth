#pragma once
#include "./bits.h"
#include "./err.c"
#include "./num.h"

// Is there a simpler way?
static Bits bits_span(U8 ceil) {
  aver(ceil <= BITS_CEIL);

  // OK for `0..63`, not OK (and not portable) for `64`.
  const auto shifted = (Bits)1 << (ceil & (BITS_CEIL - 1));

  // All-1 for `0..63`, all-0 for `64`.
  const auto mask = -(Bits)(ceil < BITS_CEIL);

  return (shifted & mask) - 1;
}

// Half-open: `[floor,ceil)`.
static Bits bits_range(U8 floor, U8 ceil) {
  return bits_span(ceil) ^ bits_span(floor);
}

static bool bits_full(Bits set) { return !~set; }

static U8 bits_len(Bits set) { return (U8)__builtin_popcountg(set); }

static bool bits_has(Bits set, U8 bit) {
  return (bit < BITS_CEIL) && (set & ((Bits)1 << bit));
}

static Bits bits_add(Bits set, U8 bit) {
  aver(bit < BITS_CEIL);
  return set | ((Bits)1 << bit);
}

static void bits_add_to(Bits *set, U8 bit) { *set = bits_add(*set, bit); }

static Bits bits_del(Bits set, U8 bit) {
  aver(bit < BITS_CEIL);
  return set & ~((Bits)1 << bit);
}

static void bits_del_from(Bits *set, U8 bit) { *set = bits_del(*set, bit); }

static bool bits_has_some(Bits set, Bits bits) { return set & bits; }

static bool bits_has_all(Bits set, Bits bits) { return (set & bits) == bits; }

static Bits bits_add_all(Bits set, Bits bits) { return set | bits; }

static void bits_add_all_to(Bits *set, Bits bits) {
  *set = bits_add_all(*set, bits);
}

static Bits bits_del_all(Bits set, Bits bits) { return set & ~bits; }

static void bits_del_all_from(Bits *set, Bits bits) {
  *set = bits_del_all(*set, bits);
}

static Bits bits_add_span(Bits set, U8 ceil) { return set | bits_span(ceil); }

static void bits_add_span_to(Bits *set, U8 ceil) {
  *set = bits_add_span(*set, ceil);
}

static Bits bits_del_span(Bits set, U8 ceil) { return set ^ bits_span(ceil); }

static void bits_del_span_from(Bits *set, U8 ceil) {
  *set = bits_del_span(*set, ceil);
}

/*
Returns index of lowest non-zero bit while zeroing it.
If set is empty, the returned index is out of bounds.
*/
ALLOW_OVERFLOW static U8 bits_pop_low(Bits *tar) {
  const auto set = *tar;
  const auto ind = (U8)__builtin_ctzg(set, BITS_CEIL);
  *tar           = set & (set - 1);
  return ind;
}

/*
Returns index of lowest zero bit while un-zeroing it.
If set is full, the returned index is out of bounds.

GCC has `__builtin_stdc_first_trailing_zero`, Clang doesn't.
*/
ALLOW_OVERFLOW static U8 bits_push_low(Bits *tar) {
  const auto set = *tar;
  const auto ind = (U8)__builtin_ctzg(~set, BITS_CEIL);
  *tar           = set | (set + 1);
  return ind;
}

static bool low_bits_zero(Uint val, Uint scale) {
  return !(val & ((1 << scale) - 1));
}

static bool high_bits_zero(Uint val, Uint scale) { return !(val >> scale); }

/*
#include <stdio.h>

int main(void) {
  Bits one   = 0b01010;
  Bits two   = 0b10101;
  Bits three = 0b00001;

  printf("%d\n", bits_has_some(one, two));
  printf("%d\n", bits_has_some(one, three));
  printf("%d\n", bits_has_some(two, three));
  printf("%llu\n", bits_del_all(three, one));
  printf("%llu\n", bits_del_all(three, two));
}
*/

/*
#include <stdio.h>

int main(void) {
  Bits set = 0b0111000111000;
  while (set) printf("pop: %d\n", bits_pop_low(&set));

  set = 0b0111000111000;
  while (!bits_full(set)) printf("push: %d\n", bits_push_low(&set));
}
*/

/*
#include "./fmt.c"
#include <stdio.h>

int main(void) {
  printf("%s\n", uint64_to_bit_str(bits_span(0)));
  printf("%s\n", uint64_to_bit_str(bits_span(5)));
  printf("%s\n", uint64_to_bit_str(bits_span(63)));
  printf("%s\n", uint64_to_bit_str(bits_span(64)));
}
*/

/*
#include "./fmt.c"
#include <stdio.h>

int main(void) {
  printf("[5  9):  %s\n", uint64_to_bit_str(bits_range(5, 9)));
  printf("[0  23): %s\n", uint64_to_bit_str(bits_range(0, 23)));
  printf("[0  31): %s\n", uint64_to_bit_str(bits_range(0, 31)));
  printf("[0  32): %s\n", uint64_to_bit_str(bits_range(0, 32)));
  printf("[0  40): %s\n", uint64_to_bit_str(bits_range(0, 40)));
  printf("[0  51): %s\n", uint64_to_bit_str(bits_range(0, 51)));
  printf("[51 62): %s\n", uint64_to_bit_str(bits_range(51, 62)));
  printf("[51 63): %s\n", uint64_to_bit_str(bits_range(51, 63)));
  printf("[51 64): %s\n", uint64_to_bit_str(bits_range(51, 64)));
}
*/
