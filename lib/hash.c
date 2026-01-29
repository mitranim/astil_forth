#pragma once
#include "./hash.h"
#include "./num.h"
#include <string.h>

/*
Simplest hash function that came up in search:

  - https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
  - https://www.ietf.org/archive/id/draft-eastlake-fnv-21.html

FNV-1a; assumes little endian.
*/

#define ALLOW_OVERFLOW __attribute__((no_sanitize("integer")))

ALLOW_OVERFLOW static Hash hash_str(const char *src) {
  Hash out = 0xCBF29CE484222325ull;
  Hash val;
  while ((val = *src++)) {
    out ^= val;
    out *= 0x00000100000001B3;
  }
  return out;
}

ALLOW_OVERFLOW static Hash hash_bytes(const U8 *src, Uint len) {
  Hash out = 0xCBF29CE484222325ull;
  for (Ind ind = 0; ind < len; ind++) {
    out ^= src[ind];
    out *= 0x00000100000001B3;
  }
  return out;
}

/*
Only needed for big endian.

static Hash reverse_bytes(Hash src) {
  Hash out = src & 0xff;
  for (Ind ind = (64 / 8) - 1; ind > 0; ind--) {
    src >>= 8;
    out = (out << 8) + (src & 0xff);
  }
  return out;
}
*/

/*
#include <stdio.h>

int main(void) {
  printf(FMT_UINT "\n", hash_str(":"));
  printf(FMT_UINT "\n", hash_str(strdup(";")));
  printf(FMT_UINT "\n", hash_str(";"));
  printf(FMT_UINT "\n", hash_str(strdup(";")));
  printf(FMT_UINT "\n", hash_val((U32)123));
  printf(FMT_UINT "\n", hash_val((U32)123));
  printf(FMT_UINT "\n", hash_val((U64)123));
  printf(FMT_UINT "\n", hash_val((U64)123));
  printf(FMT_UINT "\n", hash_val((U32)234));
  printf(FMT_UINT "\n", hash_val((U32)234));
  printf(FMT_UINT "\n", hash_val((U64)234));
  printf(FMT_UINT "\n", hash_val((U64)234));
  printf(FMT_UINT "\n", hash_val((void *)234));
  printf(FMT_UINT "\n", hash_val((void *)234));
}
*/
