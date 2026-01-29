/*
In this codebase:

- "dict" = "hash table with string keys"
- "map" = "hash table with opaque keys"

See `./dict.h`, `./dict.c`.
*/

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

// Returns -1 when key is not present.
#define map_ind(map, key)                                 \
  ({                                                      \
    const map_key_type(map) tmp_key = key;                \
    const auto tmp_size             = map_key_size(map);  \
    map_ind_impl((const Map *)(map), &tmp_key, tmp_size); \
  })

#define map_has(map, key) (map_ind(map, key) < (Ind) - 1)

#define map_get(map, key)                                                      \
  ({                                                                           \
    const map_key_type(map) tmp_key = key;                                     \
    const auto tmp_size             = map_key_size(map);                       \
    const auto tmp_map              = map;                                     \
    const auto tmp_ind = map_ind_impl((const Map *)(map), &tmp_key, tmp_size); \
    const auto tmp_got = tmp_ind < (Ind) - 1;                                  \
    tmp_got ? tmp_map->vals[tmp_ind] : (map_val_type(tmp_map)){0};             \
  })

// Returned pointer is only valid until next map resize.
#define map_set(map, key, ...)                                       \
  ({                                                                 \
    const map_key_type(map) tmp_key = (key);                         \
    const map_val_type(map) tmp_val = (__VA_ARGS__);                 \
    const auto tmp_key_size         = map_key_size(map);             \
    const auto tmp_val_size         = map_val_size(map);             \
    const auto tmp_map              = (map);                         \
    const auto tmp_ptr              = map_set_impl(                  \
      (Map *)tmp_map, &tmp_key, &tmp_val, tmp_key_size, tmp_val_size \
    );                                                               \
    (typeof(tmp_val) *)tmp_ptr;                                      \
  })

#define map_key_type(map) typeof((map)->keys[0])
#define map_key_size(map) sizeof((map)->keys[0])
#define map_val_type(map) typeof((map)->vals[0])
#define map_val_size(map) sizeof((map)->vals[0])
#define map_range hash_table_range
