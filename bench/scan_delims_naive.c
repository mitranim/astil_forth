// BOT-ASSISTED

#include "../clib/num.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BLKS (1ull << 24)
#define CAP (1u << 16)
#define RUNS (BLKS / (CAP / 16))
#define WANT (BLKS * 9)

static U8 buf[CAP];

static constexpr U8 dels[256] = {
  ['{']  = 1,
  ['}']  = 1,
  ['[']  = 1,
  [']']  = 1,
  [':']  = 1,
  [',']  = 1,
  [' ']  = 1,
  ['\n'] = 1,
  ['\t'] = 1,
};

static const U8 *escape_ptr(const U8 *val) {
  register const U8 *x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0) : : "memory");
  return x0;
}

static void init(void) {
  for (Ind ind = 0; ind < CAP; ind++) buf[ind] = "{a,b:c[d]e} \n\tfg"[ind & 15];
}

__attribute__((noinline)) static Uint scan(
  const U8 *buf, Ind len, const U8 *dels
) {
  Uint out = 0;
  for (Ind ind = 0; ind < len; ind++) out += dels[buf[ind]];
  return out;
}

int main(void) {
  init();
  Uint out = 0;
  for (Uint run = 0; run < RUNS; run++) out += scan(escape_ptr(buf), CAP, dels);
  if (getenv("SCAN_DELIMS_PRINT")) printf(FMT_UINT "\n", out);
  return out != WANT;
}
