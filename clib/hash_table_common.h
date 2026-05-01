#pragma once
#include "./hash_fnv.h"
#include "./num.h"
#include <limits.h>

/*
Opaque dict/map header. See `./dict.*` and `./map.*` files.
Must be binary-compatible with `Dict` and `Map`.

SYNC[hash_table_structure].
*/
typedef struct {
  Ind   len;
  Ind   cap;
  Uint *bits;
  void *keys;
  void *vals;
} Hash_table;

typedef Fnv_hash(Hash_fun)(const void *val, Ind len);
typedef bool(Eq_fun)(const void *one, const void *two, Ind len);

/*
Usage:

  for (hash_table_range(Ind, ind, tab)) {
    const auto key = tab->keys[ind];
    const auto val = tab->vals[ind];
  }
*/
#define hash_table_range(type, name, tab)                 \
  type name = hash_table_head((const Hash_table *)(tab)); \
  name < (tab)->cap;                                      \
  name = hash_table_next((const Hash_table *)(tab), name + 1)
