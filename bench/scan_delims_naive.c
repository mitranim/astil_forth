// BOT-ASSISTED

#include "../clib/num.h"
#include "./util.c"
#include <stdint.h>
#include <stdlib.h>

#define BLKS (1ull << 27)
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

static void init(Ind cap) {
  for (Ind ind = 0; ind < cap; ind++) buf[ind] = "{a,b:c[d]e} \n\tfg"[ind & 15];
}

__attribute__((noinline)) static Uint scan(
  const U8 *buf, Ind len, const U8 *dels
) {
  Uint out = 0;
  for (Ind ind = 0; ind < len; ind++) out += dels[buf[ind]];
  return out;
}

int main(void) {
  const Ind  cap  = (Ind)escape_u64(CAP);
  const Uint runs = (Uint)escape_u64(RUNS);

  init(cap);

  const U8 *const input = escape_ptr(buf);
  Uint            out   = 0;

  for (Uint run = 0; run < runs; run++) {
    out += escape_u64(scan(input, cap, dels));
  }
  if (out != WANT) abort();
}
