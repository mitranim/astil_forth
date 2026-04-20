#pragma once
#include "./num.h"

static constexpr U8 SHA256_SIZE = 32;

typedef struct {
  U8   data[64];
  U32  datalen;
  Uint bitlen;
  U32  state[8];
} Sha256_ctx;
