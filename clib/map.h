#pragma once
#include "./hash_table_common.h"
#include <assert.h>
#include <limits.h>

// Opaque map header. Keys and vals are treated as opaque byte chunks.
typedef Hash_table Map;

#define map_of(Key, Val) \
  struct {               \
    Ind   len;           \
    Ind   cap;           \
    Uint *bits;          \
    Key  *keys;          \
    Val  *vals;          \
  }

#define map_init(map, cap)                                         \
  hash_table_init(                                                 \
    (Hash_table *)(map), cap, map_key_size(map), map_val_size(map) \
  );

#define map_ind_impl(tmp, map, key)                            \
  ({                                                           \
    const map_key_type(map) tmp = key;                         \
    map_ind_impl((const Map *)(map), &tmp, map_key_size(map)); \
  })

// Returns `INVALID_IND` when key is not present.
#define map_ind(...) map_ind_impl(UNIQ_IDENT, __VA_ARGS__)

#define map_has(map, key) (map_ind(map, key) < INVALID_IND)

#define map_get_impl(tmp_key, tmp_map, tmp_ind, map, key) \
  ({                                                      \
    const map_key_type(map) tmp_key = key;                \
    const auto tmp_map              = map;                \
    const auto tmp_ind              = map_ind_impl(       \
      (const Map *)(map), &tmp_key, map_key_size(map)     \
    );                                                    \
    tmp_ind < INVALID_IND ? tmp_map->vals[tmp_ind]        \
                          : (map_val_type(tmp_map)){0};   \
  })

// TODO add `map_get_or` similar to `dict_get_or`.
#define map_get(...) \
  map_get_impl(UNIQ_IDENT, UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define map_set_impl(tmp_key, tmp_val, map, key, ...)                        \
  ({                                                                         \
    const map_key_type(map) tmp_key = key;                                   \
    const map_val_type(map) tmp_val = __VA_ARGS__;                           \
    (typeof(tmp_val) *)map_set_impl(                                         \
      (Map *)(map), &tmp_key, &tmp_val, map_key_size(map), map_val_size(map) \
    );                                                                       \
  })

// Returned pointer is only valid until next map resize.
#define map_set(...) map_set_impl(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define map_key_type(map) typeof((map)->keys[0])
#define map_key_size(map) sizeof((map)->keys[0])
#define map_val_type(map) typeof((map)->vals[0])
#define map_val_size(map) sizeof((map)->vals[0])

/*
Usage:

  for (map_range(Ind, ind, map)) {
    const auto key = map->keys[ind];
    const auto val = map->vals[ind];
  }
*/
#define map_range hash_table_range
