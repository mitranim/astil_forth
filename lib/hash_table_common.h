#pragma once
#include "./hash.h"
#include "./num.h"
#include <limits.h>

/*
Common code for dicts, maps, and sometimes sets. In this codebase:
- "dict" = "hash table with string keys"
- "map" = "hash table with opaque keys"
*/

/*
Opaque dict/map header. See `./dict.*` and `./map.*` files.
Must be binary-compatible with `Dict` and `Map`.
*/
typedef struct {
  Ind   len;
  Ind   cap;
  Uint *bits;
  void *keys;
  void *vals;
} Hash_table;

typedef Hash(Hash_fun)(const void *val, Uint len);
typedef bool(Eq_fun)(const void *one, const void *two, Uint len);

#define hash_table_range(type, name, tab)                 \
  type name = hash_table_head((const Hash_table *)(tab)); \
  name < (tab)->cap;                                      \
  name = hash_table_next((const Hash_table *)(tab), name + 1)
