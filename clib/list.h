#pragma once
#include "./err.h"
#include "./num.h"
#include <string.h>

typedef struct {
  void *dat;
  Ind   len;
  Ind   cap;
} List;

/*
typedef struct {
  const Ind ind;
  const Ind len;
} Span;
*/

#define list_of(Elem) \
  struct {            \
    Elem *dat;        \
    Ind   len;        \
    Ind   cap;        \
  }

typedef list_of(Uint) Uint_list;
typedef list_of(Sint) Sint_list;
typedef list_of(Ind)  Ind_list;
typedef list_of(U8)   U8_list;
typedef list_of(S8)   S8_list;
typedef list_of(U16)  U16_list;
typedef list_of(S16)  S16_list;
typedef list_of(U32)  U32_list;
typedef list_of(S32)  S32_list;
typedef list_of(U64)  U64_list;
typedef list_of(S64)  S64_list;
typedef list_of(F32)  F32_list;
typedef list_of(F64)  F64_list;

#define list_len_bytes(list) (list)->len *list_val_size(list)

#define list_rem_bytes_inner(tmp, list)          \
  ({                                             \
    const auto tmp = list;                       \
    (tmp->cap - tmp->len) * list_val_size(list); \
  })

#define list_rem_bytes(...) list_rem_bytes_inner(UNIQ_IDENT, __VA_ARGS__)

// Reserves this much total capacity (not additional capacity).
#define list_reserve_total_cap(list, cap) \
  list_reserve_total_cap_impl((List *)(list), list_val_size(list), cap)

/*
Reserves at least this much spare capacity over the current length.
If the given list already has sufficient capacity, this is a nop.
If more space is needed, doubles the required capacity until done.
*/
#define list_reserve_spare_cap(list, cap) \
  list_reserve_spare_cap_impl((List *)(list), list_val_size(list), cap)

// Appends the given value, allocating space as needed.
#define list_append_inner(tmp, list, ...)               \
  ({                                                    \
    const auto tmp = list;                              \
    list_reserve_more((List *)tmp, list_val_size(tmp)); \
    tmp->dat[tmp->len++] = __VA_ARGS__;                 \
  })

#define list_append(...) list_append_inner(UNIQ_IDENT, __VA_ARGS__)

/*
Appends the given value, allocating space as needed,
and returns a pointer to the newly appended entry.
*/
#define list_append_ptr_inner(tmp_list, tmp_ptr, list, ...)       \
  ({                                                              \
    const auto tmp_list = list;                                   \
    list_reserve_more((List *)tmp_list, list_val_size(tmp_list)); \
    const auto tmp_ptr = &tmp_list->dat[tmp_list->len++];         \
    *tmp_ptr           = __VA_ARGS__;                             \
    tmp_ptr;                                                      \
  })

#define list_append_ptr(...) \
  list_append_ptr_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

/*
Pops the last value, decrementing length.
Crashes the program if length is <= 0.
*/
#define list_pop_inner(tmp, list) \
  ({                              \
    const auto tmp = list;        \
    aver(tmp->len > 0);           \
    tmp->dat[tmp->len--];         \
  })

#define list_pop(...) list_pop_inner(UNIQ_IDENT, __VA_ARGS__)

#define list_ptr_below_inner(tmp, list, ind, max) \
  ({                                              \
    const auto tmp = ind;                         \
    aver(tmp >= 0 && tmp < (max));                \
    &((list)->dat[tmp]);                          \
  })

#define list_ptr_below(...) list_ptr_below_inner(UNIQ_IDENT, __VA_ARGS__)

/*
Returns a pointer to the next element.
Crashes if capacity is insufficient.
Does not initialize memory at location.
*/
#define list_next_ptr_inner(tmp, list)       \
  ({                                         \
    const auto tmp = list;                   \
    list_ptr_below(tmp, tmp->len, tmp->cap); \
  })

#define list_next_ptr(...) list_next_ptr_inner(UNIQ_IDENT, __VA_ARGS__)

#define list_spare_ptr_inner(tmp, list, ind) \
  ({                                         \
    const auto tmp = list;                   \
    list_ptr_below(tmp, ind, tmp->cap);      \
  })

#define list_spare_ptr(...) list_spare_ptr_inner(UNIQ_IDENT, __VA_ARGS__)

#define list_elem_ptr_inner(tmp, list, ind) \
  ({                                        \
    const auto tmp = list;                  \
    list_ptr_below(tmp, ind, tmp->len);     \
  })

#define list_elem_ptr(...) list_elem_ptr_inner(UNIQ_IDENT, __VA_ARGS__)

// Returns element at index. Crashes if out of bounds.
#define list_elem(list, ind) (*list_elem_ptr(list, ind))

// Writes element at index. Crashes if out of bounds.
#define list_put(list, ind, val) (*list_elem_ptr(list, ind) = val)

// Returns pointer to first element. Crashes if list is empty.
#define list_head_ptr_inner(tmp, list) \
  ({                                   \
    const auto tmp = list;             \
    aver(tmp->dat && tmp->len > 0);    \
    tmp->dat;                          \
  })

#define list_head_ptr(...) list_head_ptr_inner(UNIQ_IDENT, __VA_ARGS__)

// Returns first element. Crashes if out of bounds.
#define list_head(list) (*list_head_ptr(list))

// Returns pointer to last element. Crashes if list is empty.
#define list_last_ptr_inner(tmp, list) \
  ({                                   \
    const auto tmp = list;             \
    aver(tmp->dat && tmp->len > 0);    \
    &tmp->dat[tmp->len - 1];           \
  })

#define list_last_ptr(...) list_last_ptr_inner(UNIQ_IDENT, __VA_ARGS__)

// Returns last element. Crashes if out of bounds.
#define list_last(list) (*list_last_ptr(list))

// For fixed-capacity lists. Appends without allocating more.
#define list_push_inner(tmp_list, tmp_ptr, list, ...)     \
  ({                                                      \
    const auto tmp_list = list;                           \
    aver(tmp_list->len < tmp_list->cap);                  \
    const auto tmp_ptr = &tmp_list->dat[tmp_list->len++]; \
    *tmp_ptr           = __VA_ARGS__;                     \
    tmp_ptr;                                              \
  })

#define list_push(...) list_push_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

// Like `list_push` but returns the element index.
#define list_push_ind_inner(tmp_list, tmp_ind, list, ...) \
  ({                                                      \
    const auto tmp_list = list;                           \
    aver(tmp_list->len < tmp_list->cap);                  \
    const auto tmp_ind     = tmp_list->len++;             \
    tmp_list->dat[tmp_ind] = __VA_ARGS__;                 \
    tmp_ind;                                              \
  })

#define list_push_ind(...) \
  list_push_ind_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

// For fixed-capacity lists. Appends raw data without allocating more.
#define list_push_raw(list, src, len)                                 \
  list_push_raw_impl(                                                 \
    (List *)(list), list_val_size(list), src, len, __FILE__, __LINE__ \
  )

// For fixed-capacity lists. Appends raw value without allocating more.
#define list_push_raw_val_inner(tmp, list, ...) \
  ({                                            \
    const auto tmp = __VA_ARGS__;               \
    list_push_raw(list, &tmp, sizeof(tmp));     \
  })

#define list_push_raw_val(...) list_push_raw_val_inner(UNIQ_IDENT, __VA_ARGS__)

#define is_list_elem(list, ptr)                                             \
  ({                                                                        \
    static_assert(sizeof(*ptr) == list_val_size(list));                     \
    is_list_elem_impl(                                                      \
      (const List *)(list), list_val_size(list), ptr, alignof(typeof(*ptr)) \
    );                                                                      \
  })

// Pointer just outside the current length.
// Writing to this address is UB.
#define list_len_ceil_inner(tmp, list) \
  ({                                   \
    const auto tmp = list;             \
    tmp->dat + tmp->len;               \
  })

#define list_len_ceil(...) list_len_ceil_inner(UNIQ_IDENT, __VA_ARGS__)

// Pointer just outside the allocated capacity.
#define list_cap_ceil(list)                   \
  (typeof((list)->dat))(list_ceil_impl(       \
    (const List *)(list), list_val_size(list) \
  ))

#define list_val_type(list) typeof((list)->dat[0])
#define list_val_size(list) (Ind)sizeof((list)->dat[0])

// Index of given list element, by pointer.
// If the element isn't in the list, index is -1.
#define list_ind_inner(tmp_list, tmp_floor, tmp_val, list, val)   \
  ({                                                              \
    const auto tmp_list  = list;                                  \
    const auto tmp_floor = tmp_list->dat;                         \
    const auto tmp_val   = val;                                   \
    (tmp_val >= tmp_floor) && (tmp_val < list_len_ceil(tmp_list)) \
      ? (Ind)(tmp_val - tmp_floor)                                \
      : INVALID_IND;                                              \
  })

#define list_ind(...) \
  list_ind_inner(UNIQ_IDENT, UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define list_trunc(list) ((list)->len = 0)

#define list_rewind_inner(tmp_next, tmp_prev, next, prev)                   \
  ({                                                                        \
    const auto tmp_next = next;                                             \
    const auto tmp_prev = prev;                                             \
    aver(tmp_next->cap >= tmp_prev->cap);                                   \
    memmove(                                                                \
      tmp_next->dat, tmp_prev->dat, tmp_prev->len * list_val_size(tmp_prev) \
    );                                                                      \
    tmp_next->len = tmp_prev->len;                                          \
  })

#define list_rewind(...) list_rewind_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

static constexpr Uint LIST_INIT_CAP = 4;
