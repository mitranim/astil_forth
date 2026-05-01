/*
Modified from:

- https://github.com/B-Con/crypto-algorithms/blob/master/sha256.h
- https://github.com/B-Con/crypto-algorithms/blob/master/sha256.c

Credit: Brad Conte (brad AT bradconte.com).
Original files are in public domain.

Changes:

- Only unsigned bitwise ops.
- Sensible integer names.
- Avoid scope pollution.
- Drop unused macros.
*/
#pragma once
#include "./hash_sha.h"
#include <string.h> // `memset`

// NOLINTBEGIN(readability-identifier-length)

ALLOW_OVERFLOW static void sha256_transform(Sha256_ctx *ctx, const U8 data[]) {
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32u - (b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2u) ^ ROTRIGHT(x, 13u) ^ ROTRIGHT(x, 22u))
#define EP1(x) (ROTRIGHT(x, 6u) ^ ROTRIGHT(x, 11u) ^ ROTRIGHT(x, 25u))
#define SIG0(x) (ROTRIGHT(x, 7u) ^ ROTRIGHT(x, 18u) ^ ((x) >> 3u))
#define SIG1(x) (ROTRIGHT(x, 17u) ^ ROTRIGHT(x, 19u) ^ ((x) >> 10u))

  U32 m[64];

  for (U32 i = 0, j = 0; i < 16; i += 1, j += 4) {
    m[i] = ((U32)data[j] << 24u) | ((U32)data[j + 1] << 16u) |
      ((U32)data[j + 2] << 8u) | (data[j + 3]);
  }
  for (U32 i = 16; i < 64; ++i) {
    m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
  }

  U32 a = ctx->state[0];
  U32 b = ctx->state[1];
  U32 c = ctx->state[2];
  U32 d = ctx->state[3];
  U32 e = ctx->state[4];
  U32 f = ctx->state[5];
  U32 g = ctx->state[6];
  U32 h = ctx->state[7];

  static constexpr U32 SHA256_CONST[64] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1,
    0x923F82A4, 0xAB1C5ED5, 0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174, 0xE49B69C1, 0xEFBE4786,
    0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147,
    0x06CA6351, 0x14292967, 0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85, 0xA2BFE8A1, 0xA81A664B,
    0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A,
    0x5B9CCA4F, 0x682E6FF3, 0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
  };

  for (U32 i = 0; i < 64; ++i) {
    U32 t1 = h + EP1(e) + CH(e, f, g) + SHA256_CONST[i] + m[i];
    U32 t2 = EP0(a) + MAJ(a, b, c);
    h      = g;
    g      = f;
    f      = e;
    e      = d + t1;
    d      = c;
    c      = b;
    b      = a;
    a      = t1 + t2;
  }

  ctx->state[0] += a;
  ctx->state[1] += b;
  ctx->state[2] += c;
  ctx->state[3] += d;
  ctx->state[4] += e;
  ctx->state[5] += f;
  ctx->state[6] += g;
  ctx->state[7] += h;

#undef ROTRIGHT
#undef CH
#undef MAJ
#undef EP0
#undef EP1
#undef SIG0
#undef SIG1
}

static void sha256_init(Sha256_ctx *ctx) {
  ctx->datalen  = 0;
  ctx->bitlen   = 0;
  ctx->state[0] = 0x6A09E667;
  ctx->state[1] = 0xBB67AE85;
  ctx->state[2] = 0x3C6EF372;
  ctx->state[3] = 0xA54FF53A;
  ctx->state[4] = 0x510E527F;
  ctx->state[5] = 0x9B05688C;
  ctx->state[6] = 0x1F83D9AB;
  ctx->state[7] = 0x5BE0CD19;
}

static void sha256_update(Sha256_ctx *ctx, const U8 data[], Ind len) {
  for (Ind ind = 0; ind < len; ind++) {
    ctx->data[ctx->datalen] = data[ind];
    ctx->datalen++;
    if (ctx->datalen == 64) {
      sha256_transform(ctx, ctx->data);
      ctx->bitlen += 512;
      ctx->datalen = 0;
    }
  }
}

static void sha256_final(Sha256_ctx *ctx, U8 hash[]) {
  U32 i = ctx->datalen;

  // Pad whatever data is left in the buffer.
  if (ctx->datalen < 56) {
    ctx->data[i++] = 0x80;
    while (i < 56) ctx->data[i++] = 0x00;
  }
  else {
    ctx->data[i++] = 0x80;
    while (i < 64) ctx->data[i++] = 0x00;
    sha256_transform(ctx, ctx->data);
    memset(ctx->data, 0, 56);
  }

  // Append to the padding the total message's length in bits and transform.
  ctx->bitlen += (typeof(ctx->bitlen))ctx->datalen * 8;
  ctx->data[63] = (U8)(ctx->bitlen);
  ctx->data[62] = (U8)(ctx->bitlen >> 8u);
  ctx->data[61] = (U8)(ctx->bitlen >> 16u);
  ctx->data[60] = (U8)(ctx->bitlen >> 24u);
  ctx->data[59] = (U8)(ctx->bitlen >> 32u);
  ctx->data[58] = (U8)(ctx->bitlen >> 40u);
  ctx->data[57] = (U8)(ctx->bitlen >> 48u);
  ctx->data[56] = (U8)(ctx->bitlen >> 56u);
  sha256_transform(ctx, ctx->data);

  // Since this implementation uses little endian `U8` ordering,
  // and SHA uses big endian, reverse all the bytes when copying
  // the final state to the output hash.
  for (U8 i = 0; i < 4; ++i) {
    hash[i]      = (ctx->state[0] >> (24u - (i * 8))) & 0x000000FFu;
    hash[i + 4]  = (ctx->state[1] >> (24u - (i * 8))) & 0x000000FFu;
    hash[i + 8]  = (ctx->state[2] >> (24u - (i * 8))) & 0x000000FFu;
    hash[i + 12] = (ctx->state[3] >> (24u - (i * 8))) & 0x000000FFu;
    hash[i + 16] = (ctx->state[4] >> (24u - (i * 8))) & 0x000000FFu;
    hash[i + 20] = (ctx->state[5] >> (24u - (i * 8))) & 0x000000FFu;
    hash[i + 24] = (ctx->state[6] >> (24u - (i * 8))) & 0x000000FFu;
    hash[i + 28] = (ctx->state[7] >> (24u - (i * 8))) & 0x000000FFu;
  }
}

// Out-buffer must have `SHA256_SIZE` capacity.
static void sha256(U8 *out, const U8 *src, Ind len) {
  Sha256_ctx ctx;
  sha256_init(&ctx);
  sha256_update(&ctx, src, len);
  sha256_final(&ctx, out);
}

#define sha256_any_inner(tmp, src, out)         \
  ({                                            \
    const auto tmp = src;                       \
    sha256(out, (const U8 *)&tmp, sizeof(tmp)); \
  })

#define sha256_any(...) sha256_any_inner(UNIQ_IDENT, __VA_ARGS__)

// NOLINTEND(readability-identifier-length)

/*
#include "./fmt.c"
#include <stdio.h>

int main(int argc, const char **argv) {
  U8 buf[SHA256_SIZE];

  for (Ind ind = 0; ind < 8; ind++) {
    sha256_any(ind, buf);
    printf("ind:         " FMT_IND "\n", ind);
    printf("ind as hex:  %s\n", fmt_bytes_hex((const U8 *)&ind, sizeof(ind)));
    printf("sha256(ind): %s\n", fmt_bytes_hex(buf, arr_cap(buf)));
  }
}
*/
