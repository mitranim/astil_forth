// BOT-GENERATED

#include "../clib/num.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLKS (1ull << 24)
#define CAP (1u << 16)
#define REPS (BLKS / (CAP / 16))
#define WANT (BLKS * 9)

typedef char c8x16 __attribute__((vector_size(16)));
typedef U8   u8x16 __attribute__((vector_size(16)));
static U8    buf[CAP];

static void init(void) {
  const U8 pat[16] = {
    '{', 'a', ',', 'b', ':', 'c', '[', 'd', ']', 'e', '}', ' ', '\n', '\t', 'f', 'g'
  };
  for (Ind idx = 0; idx < CAP; idx++) buf[idx] = pat[idx & 15];
}

__attribute__((noinline)) static Uint scan(Uint reps) {
  const u8x16 one = (u8x16){1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  Uint        out = 0;
  for (Uint rep = 0; rep < reps; rep++) {
    for (Ind off = 0; off < CAP; off += 16) {
      u8x16 vec;
      memcpy(&vec, buf + off, sizeof(vec));
      c8x16 msk = (vec == (U8)'{');
      msk |= (vec == '}');
      msk |= (vec == '[');
      msk |= (vec == ']');
      msk |= (vec == ':');
      msk |= (vec == ',');
      msk |= (vec == ' ');
      msk |= (vec == '\n');
      msk |= (vec == '\t');
      u8x16 cnt = (u8x16)msk & one;
      out += cnt[0] + cnt[1] + cnt[2] + cnt[3] + cnt[4] + cnt[5] + cnt[6] +
        cnt[7] + cnt[8] + cnt[9] + cnt[10] + cnt[11] + cnt[12] + cnt[13] +
        cnt[14] + cnt[15];
    }
  }
  return out;
}

int main(void) {
  init();
  const Uint out = scan(REPS);
  if (getenv("SCAN_DELIMS_PRINT")) printf(FMT_UINT "\n", out);
  return out != WANT;
}
