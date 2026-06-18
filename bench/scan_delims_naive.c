// BOT-GENERATED

#include "../clib/num.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BLKS (1ull << 24)
#define CAP (1u << 16)
#define REPS (BLKS / (CAP / 16))
#define WANT (BLKS * 9)

static U8 buf[CAP];

static void init(void) {
  const U8 src[16] = {
    '{', 'a', ',', 'b', ':', 'c', '[', 'd', ']', 'e', '}', ' ', '\n', '\t', 'f', 'g'
  };
  for (Ind idx = 0; idx < CAP; idx++) buf[idx] = src[idx & 15];
}

static Ind is_delim(U8 chr) {
  return chr == '{' || chr == '}' || chr == '[' || chr == ']' || chr == ':' ||
    chr == ',' || chr == ' ' || chr == '\n' || chr == '\t';
}

__attribute__((noinline)) static Uint scan(Uint reps) {
  Uint out = 0;
  for (Uint rep = 0; rep < reps; rep++) {
    for (Ind idx = 0; idx < CAP; idx++) out += is_delim(buf[idx]);
  }
  return out;
}

int main(void) {
  init();
  const Uint out = scan(REPS);
  if (getenv("SCAN_DELIMS_PRINT")) printf(FMT_UINT "\n", out);
  return out != WANT;
}
