#pragma once
#include "./num.h"

// Opaque set header. Vals are treated as opaque byte chunks.
typedef struct {
  Ind   len;
  Ind   cap;
  Uint *bits;
  void *vals;
} Set;

#define set_of(Elem) \
  struct {           \
    Ind   len;       \
    Ind   cap;       \
    Uint *bits;      \
    Elem *vals;      \
  }

typedef set_of(Uint) Uint_set;
typedef set_of(Sint) Sint_set;
typedef set_of(Ind)  Ind_set;
typedef set_of(U8)   U8_set;
typedef set_of(S8)   S8_set;
typedef set_of(U16)  U16_set;
typedef set_of(S16)  S16_set;
typedef set_of(U32)  U32_set;
typedef set_of(S32)  S32_set;
typedef set_of(U64)  U64_set;
typedef set_of(S64)  S64_set;
typedef set_of(F32)  F32_set;
typedef set_of(F64)  F64_set;

#define set_init(set, cap) set_init_impl((Set *)(set), cap, set_val_size(set));

#define set_has_inner(tmp, set, val)                                \
  ({                                                                \
    const set_val_type(set) tmp = val;                              \
    set_matching_ind((const Set *)(set), &tmp, set_val_size(set)) < \
      INVALID_IND;                                                  \
  })

// NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)
#define set_has(...) set_has_inner(UNIQ_IDENT, __VA_ARGS__)
// NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)

#define set_add_inner(tmp, set, ...)                                    \
  ({                                                                    \
    const set_val_type(set) tmp = __VA_ARGS__;                          \
    (typeof(tmp) *)set_add_impl((Set *)(set), &tmp, set_val_size(set)); \
  })

// NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)

// Returned pointer is only valid until next set resize.
#define set_add(...) set_add_inner(UNIQ_IDENT, __VA_ARGS__)

// NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)

#define set_val_type(set) typeof((set)->vals[0])
#define set_val_size(set) (Ind)sizeof((set)->vals[0])
#define set_range hash_table_range
