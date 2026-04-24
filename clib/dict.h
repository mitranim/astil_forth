#pragma once
#include "./hash_table_common.h"
#include "./num.h"
#include <assert.h>
#include <limits.h>

/*
Opaque dict header. Must be binary-compatible with `Hash_table`.

The implementation makes no assumption if the keys are owned or borrowed.
Consumer code decides this. See `dict_deinit` vs `dict_deinit_with_keys`.

SYNC[hash_table_structure].
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

// TODO fix `clang-format`.
typedef struct {
} Empty;

// Bit of a hack; `./lib/set.(c|h)` doesn't support `char*` keys.
typedef dict_of(Empty) Str_set;
static constexpr auto  EMPTY = (Empty){};

#define dict_init(dict, cap) \
  hash_table_init((Hash_table *)dict, cap, DICT_KEY_SIZE, dict_val_size(dict));

// Returns `INVALID_IND` when key is not present.
#define dict_ind(dict, key) dict_ind_impl((const Dict *)(dict), key)

#define dict_has(dict, key) (dict_ind(dict, key) != INVALID_IND)

#define dict_get_or_impl(tmp_dict, tmp_ind, dict, key, val)           \
  ({                                                                  \
    const auto tmp_dict = dict;                                       \
    const auto tmp_ind  = dict_ind_impl((const Dict *)tmp_dict, key); \
    (tmp_ind == INVALID_IND) ? val : tmp_dict->vals[tmp_ind];         \
  })

#define dict_get_or(...) dict_get_or_impl(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define dict_get(dict, key) dict_get_or(dict, key, (dict_val_type(dict)){0})

/*
Returned pointer is only valid until next dict resize.
Doesn't copy the key; the caller is responsible for its lifetime.
Dicts which own their keys must be freed with `dict_deinit_with_keys`.
*/
#define dict_set_inner(tmp, dict, key, ...)          \
  ({                                                 \
    const dict_val_type(dict) tmp = __VA_ARGS__;     \
    (typeof(tmp) *)dict_set_impl(                    \
      (Dict *)(dict), key, &tmp, dict_val_size(dict) \
    );                                               \
  })

#define dict_set(...) dict_set_inner(UNIQ_IDENT, __VA_ARGS__)

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

#define dict_rewind_impl(tmp_next, tmp_prev, next, prev)                     \
  ({                                                                         \
    const auto tmp_next = next;                                              \
    const auto tmp_prev = prev;                                              \
    aver(tmp_next->cap >= tmp_prev->cap);                                    \
    memcpy(                                                                  \
      tmp_next->bits, tmp_prev->bits, tmp_prev->len * sizeof(tmp_prev->bits) \
    );                                                                       \
    memcpy(                                                                  \
      tmp_next->keys, tmp_prev->keys, tmp_prev->len * sizeof(tmp_prev->keys) \
    );                                                                       \
    memcpy(                                                                  \
      tmp_next->vals, tmp_prev->vals, tmp_prev->len * sizeof(tmp_prev->vals) \
    );                                                                       \
    tmp_next->len = tmp_prev->len;                                           \
  })

#define dict_rewind(...) dict_rewind_impl(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)
