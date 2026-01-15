/*
Replica of Go slices. See example in `./list.c`.
Resizing relocates memory; pointers are transient.
TODO: minimize chances of variable name collision.
*/

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

#define list_rem_bytes(list)                               \
  ({                                                       \
    const auto tmp_list = list;                            \
    (tmp_list->cap - tmp_list->len) * list_val_size(list); \
  })

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
#define list_append(list, ...)                                \
  ({                                                          \
    const auto tmp_list = list;                               \
    list_reserve_more((List *)tmp_list, list_val_size(list)); \
    tmp_list->dat[tmp_list->len++] = __VA_ARGS__;             \
  })

/*
Appends the given value, allocating space as needed,
and returns a pointer to the newly appended entry.
*/
#define list_append_ptr(list, ...)                            \
  ({                                                          \
    const auto tmp_list = list;                               \
    list_reserve_more((List *)tmp_list, list_val_size(list)); \
    const auto tmp_ptr = &tmp_list->dat[tmp_list->len++];     \
    *tmp_ptr           = __VA_ARGS__;                         \
    tmp_ptr;                                                  \
  })

/*
Pops the last value, decrementing length.
Crashes the program if length is <= 0.
*/
#define list_pop(list)              \
  ({                                \
    const auto tmp_list = list;     \
    aver(tmp_list->len > 0);        \
    tmp_list->dat[tmp_list->len--]; \
  })

#define list_ptr_below(list, ind, max)     \
  ({                                       \
    const auto tmp_ind = ind;              \
    aver(tmp_ind >= 0 && tmp_ind < (max)); \
    &((list)->dat[tmp_ind]);               \
  })

/*
Returns a pointer to the next element.
Crashes if capacity is insufficient.
Does not initialize memory at location.
*/
#define list_next_ptr(list)                                 \
  ({                                                        \
    const auto tmp_list = list;                             \
    list_ptr_below(tmp_list, tmp_list->len, tmp_list->cap); \
  })

#define list_spare_ptr(list, ind)                 \
  ({                                              \
    const auto tmp_list = list;                   \
    list_ptr_below(tmp_list, ind, tmp_list->cap); \
  })

#define list_elem_ptr(list, ind)                  \
  ({                                              \
    const auto tmp_list = list;                   \
    list_ptr_below(tmp_list, ind, tmp_list->len); \
  })

// Returns element at index. Crashes if out of bounds.
#define list_elem(list, ind) (*list_elem_ptr(list, ind))

// Writes element at index. Crashes if out of bounds.
#define list_put(list, ind, val) (*list_elem_ptr(list, ind) = val)

// Returns pointer to first element. Crashes if list is empty.
#define list_head_ptr(list)                   \
  ({                                          \
    const auto tmp_list = list;               \
    aver(tmp_list->dat && tmp_list->len > 0); \
    tmp_list->dat;                            \
  })

// Returns first element. Crashes if out of bounds.
#define list_head(list) (*list_head_ptr(list))

// Returns pointer to last element. Crashes if list is empty.
#define list_last_ptr(list)                   \
  ({                                          \
    const auto tmp_list = list;               \
    aver(tmp_list->dat && tmp_list->len > 0); \
    &tmp_list->dat[tmp_list->len - 1];        \
  })

// Returns last element. Crashes if out of bounds.
#define list_last(list) (*list_last_ptr(list))

// For fixed-capacity lists. Appends without allocating more.
#define list_push(list, ...)                              \
  ({                                                      \
    const auto tmp_list = list;                           \
    aver(tmp_list->len < tmp_list->cap);                  \
    const auto tmp_ptr = &tmp_list->dat[tmp_list->len++]; \
    *tmp_ptr           = __VA_ARGS__;                     \
    tmp_ptr;                                              \
  })

// Like `list_push` but returns the element index.
#define list_push_ind(list, ...)              \
  ({                                          \
    const auto tmp_list = list;               \
    aver(tmp_list->len < tmp_list->cap);      \
    const auto tmp_ind     = tmp_list->len++; \
    tmp_list->dat[tmp_ind] = __VA_ARGS__;     \
    tmp_ind;                                  \
  })

// For fixed-capacity lists. Appends raw data without allocating more.
#define list_push_raw(list, src, len)                                 \
  list_push_raw_impl(                                                 \
    (List *)(list), list_val_size(list), src, len, __FILE__, __LINE__ \
  )

// For fixed-capacity lists. Appends raw value without allocating more.
#define list_push_raw_val(list, ...)                \
  ({                                                \
    const auto tmp_val = __VA_ARGS__;               \
    list_push_raw(list, &tmp_val, sizeof(tmp_val)); \
  })

#define is_list_elem(list, ptr)                                        \
  ({                                                                   \
    static_assert(sizeof(*ptr) == list_val_size(list));                \
    is_list_elem_impl((const List *)(list), list_val_size(list), ptr); \
  })

// Pointer just outside the allocated capacity.
#define list_ceil(list)                       \
  (typeof((list)->dat))(list_ceil_impl(       \
    (const List *)(list), list_val_size(list) \
  ))

#define list_val_type(list) typeof((list)->dat[0])
#define list_val_size(list) sizeof((list)->dat[0])

// Index of given list element, by pointer.
// Providing an invalid pointer is UB.
#define list_ind(list, val) ((Ind)((val) - (list)->dat))

static constexpr Uint LIST_INIT_CAP = 4;
