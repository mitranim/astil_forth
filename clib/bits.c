// Fixed-size bitsets and other bit-fiddling.
#pragma once
#include "./bits.h"
#include "./err.c"
#include "./num.h"

// Is there a simpler way?
ALLOW_OVERFLOW static Bits bits_span(U8 ceil) {
  assert_fatal(ceil <= BITS_CEIL);

  /*
  Why all the casts: apparently, C "promotes" operands of bitwise operations
  from shorter types to SIGNED `int` THEN `clang-tidy` complains about using
  signed `int` in bitwise operations. What insanity.
  */
  const U8 shift = ceil & (U8)(BITS_CEIL - 1);

  // OK for `0..63`, not OK (and not portable) for `64`.
  const auto shifted = (Bits)1 << shift;

  // All-1 for `0..63`, all-0 for `64`.
  const auto mask = -(Bits)(ceil < BITS_CEIL);

  return (shifted & mask) - 1;
}

// Half-open: `[floor,ceil)`.
static Bits bits_range(U8 floor, U8 ceil) {
  return bits_span(ceil) ^ bits_span(floor);
}

static bool bits_full(Bits set) { return set == BITS_ALL; }

static U8 bits_len(Bits set) { return (U8)__builtin_popcountg(set); }

static bool bits_has(Bits set, U8 bit) {
  return (bit < BITS_CEIL) && (set & ((Bits)1 << bit));
}

static Bits bits_add(Bits set, U8 bit) {
  assert_fatal(bit < BITS_CEIL);
  return set | ((Bits)1 << bit);
}

static void bits_add_to(Bits *set, U8 bit) { *set = bits_add(*set, bit); }

static Bits bits_del(Bits set, U8 bit) {
  assert_fatal(bit < BITS_CEIL);
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

static Bits bits_del_span(Bits set, U8 ceil) {
  return bits_del_all(set, bits_span(ceil));
}

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
  return !(val & ((1u << scale) - 1));
}

static bool high_bits_zero(Uint val, Uint scale) { return !(val >> scale); }
