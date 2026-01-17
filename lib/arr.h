// Macros for fixed-size arrays.
#pragma once
#include "./err.h"

#ifndef arr_cap
#define arr_cap(val) (sizeof(val) / sizeof(val[0]))
#endif

#define arr_ceil(arr) (&(arr)[arr_cap(arr)])

#define arr_copy(tar, src)                       \
  ({                                             \
    static_assert(arr_cap(src) == arr_cap(tar)); \
    memcpy(tar, src, arr_cap(src));              \
  })

#define arr_ptr_at(arr, ind)      \
  ({                              \
    const Ind tmp_ind = ind;      \
    aver(tmp_ind < arr_cap(arr)); \
    &(arr)[tmp_ind];              \
  })

/*
Caution: `ind` is for array elements (non-byte `sizeof`),
and `src` may be any value that fits within the capacity.
*/
#define arr_set(arr, ind, src)                               \
  ({                                                         \
    const auto tmp = src;                                    \
    arr_set_impl(arr, arr_cap(arr), ind, &tmp, sizeof(tmp)); \
  })

// Examples: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2299.htm
#define arr_clear(arr) memcpy(arr, (typeof(arr)){}, arr_cap(arr));

#define is_ptr_or_arr(val) (__builtin_classify_type(val) == 5)

#define is_of_type(val, typ) \
  __builtin_types_compatible_p(typeof(val), typeof(typ))

#define is_arr(val) (is_ptr_or_arr(val) && !is_ptr(val))

// False positive for unsized `T[]` fields in structs. OK for fixed sizes.
#define is_ptr(val) is_of_type(val, to_ptr_type(val))

/*
For weird macros.

  any[N] -> any*
  any*   -> any*
  any    -> void*
*/
#define to_ptr_type(val) \
  typeof(&*__builtin_choose_expr(is_ptr_or_arr(val), (val), (void *)nullptr))

#define to_ptr_or_arr(val) \
  __builtin_choose_expr(is_ptr_or_arr(val), val, nullptr)

// For fixed-size arrays only.
#define arr_range(type, name, arr) \
  type name = 0;                   \
  name < arr_cap(arr);             \
  name++
