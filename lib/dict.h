/*
In this codebase:

- "dict" = "hash table with string keys"
- "map" = "hash table with opaque keys"

See `./map.h`, `./map.c`.
*/

#pragma once
#include "./hash_table_common.h"
#include "./num.h"
#include <assert.h>
#include <limits.h>

/*
Opaque dict header. Must be binary-compatible with `Hash_table`.

The implementation makes no assumption if the keys are owned or borrowed.
Consumer code decides this. See `dict_deinit` vs `dict_deinit_with_keys`.
*/
typedef struct {
  Ind    len;
  Ind    cap;
  Uint  *bits;
  char **keys;
  void  *vals;
} Dict;

#define dict_of(Elem) \
  struct {            \
    Ind    len;       \
    Ind    cap;       \
    Uint  *bits;      \
    char **keys;      \
    Elem  *vals;      \
  }

typedef dict_of(Uint) Uint_dict;
typedef dict_of(Sint) Sint_dict;
typedef dict_of(Ind)  Ind_dict;
typedef dict_of(U8)   U8_dict;
typedef dict_of(S8)   S8_dict;
typedef dict_of(U16)  U16_dict;
typedef dict_of(S16)  S16_dict;
typedef dict_of(U32)  U32_dict;
typedef dict_of(S32)  S32_dict;
typedef dict_of(U64)  U64_dict;
typedef dict_of(S64)  S64_dict;
typedef dict_of(F32)  F32_dict;
typedef dict_of(F64)  F64_dict;

#define dict_init(dict, cap) \
  hash_table_init((Hash_table *)dict, cap, DICT_KEY_SIZE, dict_val_size(dict));

// Returns -1 when key is not present.
#define dict_ind(dict, key) dict_ind_impl((const Dict *)(dict), key)

#define dict_has(dict, key) (dict_ind(dict, key) < (Ind) - 1)

#define dict_get(dict, key)                                           \
  ({                                                                  \
    const auto tmp_dict = dict;                                       \
    const auto tmp_ind  = dict_ind_impl((const Dict *)tmp_dict, key); \
    const auto tmp_got  = tmp_ind < (Ind) - 1;                        \
    tmp_got ? tmp_dict->vals[tmp_ind] : (dict_val_type(tmp_dict)){0}; \
  })

// Returned pointer is only valid until next dict resize.
#define dict_set(dict, key, ...)                             \
  ({                                                         \
    const dict_val_type(dict) tmp_val = __VA_ARGS__;         \
    const auto tmp_dict               = dict;                \
    const auto tmp_val_size           = dict_val_size(dict); \
    const auto tmp_ptr                = dict_set_impl(       \
      (Dict *)tmp_dict, key, &tmp_val, tmp_val_size          \
    );                                                       \
    (typeof(tmp_val) *)tmp_ptr;                              \
  })

#define dict_val_type(dict) typeof((dict)->vals[0])
#define dict_val_size(dict) sizeof((dict)->vals[0])

/*
Usage:

  for (dict_range(Ind, ind, dict)) {
    const auto key = dict->keys[ind];
    const auto val = dict->vals[ind];
  }
*/
#define dict_range hash_table_range
