#pragma once
#include "./num.h"

typedef Uint Fnv_hash;

#define hash_val_impl(tmp, val)                    \
  ({                                               \
    const auto tmp = val;                          \
    fnv_hash_bytes((const U8 *)&tmp, sizeof(tmp)); \
  })

#define hash_val(...) hash_val_impl(UNIQ_IDENT, __VA_ARGS__)
