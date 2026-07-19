// BOT-ASSISTED

#include "../clib/num.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BLKS (1ull << 27)
#define CAP (1u << 16)
#define RUNS (BLKS / (CAP / 16))
#define WANT (BLKS * 9)

typedef char c8x16 __attribute__((vector_size(16)));
typedef U8   u8x16 __attribute__((vector_size(16)));
typedef U8   u8x8 __attribute__((vector_size(8)));
typedef U16  u16x8 __attribute__((vector_size(16)));
static U8    buf[CAP];

static const U8 *escape_ptr(const U8 *val) {
  register const U8 *x0 __asm__("x0") = val;
  __asm__ volatile("" : "+r"(x0) : : "memory");
  return x0;
}

static void init(void) {
  for (Ind ind = 0; ind < CAP; ind++) buf[ind] = "{a,b:c[d]e} \n\tfg"[ind & 15];
}

__attribute__((noinline)) static Uint scan(const U8 *buf, Ind len) {
  const u8x16 one  = (u8x16){1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  u16x8       low  = {};
  u16x8       high = {};
  for (Ind off = 0; off < len; off += 16) {
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
    low += __builtin_convertvector(
      __builtin_shufflevector(cnt, cnt, 0, 1, 2, 3, 4, 5, 6, 7), u16x8
    );
    high += __builtin_convertvector(
      __builtin_shufflevector(cnt, cnt, 8, 9, 10, 11, 12, 13, 14, 15), u16x8
    );
  }
  return (Uint)__builtin_reduce_add(low) + (Uint)__builtin_reduce_add(high);
}

int main(void) {
  init();
  Uint out = 0;
  for (Uint run = 0; run < RUNS; run++) out += scan(escape_ptr(buf), CAP);
  if (out != WANT) abort();
}
