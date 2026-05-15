#pragma once
#include "./err.h"
#include "./misc.h"
#include "./num.h"
#include <assert.h>
#include <string.h>

/*
Stack with guards. Macros below assume the "empty ascending" style.

Field ordering matters; some code hardcodes the offsets.

`bytelen` is pointer-sized `Uint` for ABI reasons;
this allows for easier interop with external code.
However, we generally limit allocation size to the
size representable with `Ind` (subject to change).

SYNC[stack_field_offsets].
*/
typedef struct {
  Uint  bytelen; // Size of full region in bytes.
  void *cellar;  // Starting address of full region: `guard|stack|guard`.
  void *floor;   // Starting address of stack region.
  void *top;     // Address of "current" stack item, if any.
  void *ceil;    // Starting address of second guard, just above the stack.
} Stack;

// SYNC[stack_field_offsets].
#define stack_of(Elem) \
  struct {             \
    Uint  bytelen;     \
    Elem *cellar;      \
    Elem *floor;       \
    Elem *top;         \
    Elem *ceil;        \
  }

typedef struct {
  Ind len;
} Stack_opt;

#define span_of(Elem) \
  struct {            \
    Elem *floor;      \
    Elem *top;        \
    Elem *ceil;       \
  }

typedef stack_of(Uint) Uint_stack;
typedef stack_of(Sint) Sint_stack;
typedef stack_of(U8)   U8_stack;
typedef stack_of(S8)   S8_stack;
typedef stack_of(U16)  U16_stack;
typedef stack_of(S16)  S16_stack;
typedef stack_of(U32)  U32_stack;
typedef stack_of(S32)  S32_stack;
typedef stack_of(U64)  U64_stack;
typedef stack_of(S64)  S64_stack;
typedef stack_of(F32)  F32_stack;
typedef stack_of(F64)  F64_stack;

typedef span_of(Uint) Uint_span;
typedef span_of(Sint) Sint_span;
typedef span_of(U8)   U8_span;
typedef span_of(S8)   S8_span;
typedef span_of(U16)  U16_span;
typedef span_of(S16)  S16_span;
typedef span_of(U32)  U32_span;
typedef span_of(S32)  S32_span;
typedef span_of(U64)  U64_span;
typedef span_of(S64)  S64_span;
typedef span_of(F32)  F32_span;
typedef span_of(F64)  F64_span;

#define stack_init(stack, opt) \
  stack_init_impl(stack, opt, stack_val_size(stack))

#define stack_cap(stack) ((Sint)((stack)->ceil - (stack)->floor))
#define stack_len(stack) ((Sint)((stack)->top - (stack)->floor))
#define stack_rem(stack) ((Sint)((stack)->ceil - (stack)->top))

#define stack_val_type(stack) typeof((stack)->floor[0])
#define stack_val_size(stack) (Ind)sizeof((stack)->floor[0])

#define stack_cap_bytes(stack) \
  ((Uint)((U8 *)(stack)->ceil - (U8 *)(stack)->floor))

#define stack_len_bytes(stack) \
  ((Uint)((U8 *)(stack)->top - (U8 *)(stack)->floor))

#define stack_head(stack) ((stack)->top[-1])
#define stack_trunc(stack) ((stack)->top = (stack)->floor)
#define stack_trunc_to(stack, len) ((stack)->top = (stack)->floor + len)

#define stack_push_inner(tmp, stack, ...) \
  ({                                      \
    const auto tmp = (stack)->top;        \
    aver(tmp < (stack)->ceil);            \
    *tmp         = __VA_ARGS__;           \
    (stack)->top = tmp + 1;               \
    tmp;                                  \
  })

#define stack_push(...) stack_push_inner(UNIQ_IDENT, __VA_ARGS__)

#define stack_pop(stack)                  \
  ({                                      \
    assert((stack)->top < (stack)->ceil); \
    *(--(stack)->top);                    \
  })

#define stack_push_raw_inner(                                          \
  tmp_stack, tmp_val, tmp_out, tmp_val_size, tmp_elem_size, stack, ... \
)                                                                      \
  ({                                                                   \
    const auto     tmp_stack     = stack;                              \
    const auto     tmp_val       = __VA_ARGS__;                        \
    const auto     tmp_out       = tmp_stack->top;                     \
    constexpr auto tmp_val_size  = (Uint)sizeof(tmp_val);              \
    constexpr auto tmp_elem_size = (Uint)sizeof(*tmp_stack->top);      \
    static_assert(                                                     \
      tmp_val_size >= tmp_elem_size &&                                 \
      divisible_by(tmp_val_size, tmp_elem_size)                        \
    );                                                                 \
    memcpy(tmp_out, &tmp_val, tmp_val_size);                           \
    tmp_stack->top += tmp_val_size / tmp_elem_size;                    \
    tmp_out;                                                           \
  })

// Pushes the raw memory representation of the given value.
#define stack_push_raw(...)                                                 \
  stack_push_raw_impl(                                                      \
    UNIQ_IDENT, UNIQ_IDENT, UNIQ_IDENT, UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__ \
  )

#define stack_push_from_inner(tmp, out, src)                            \
  ({                                                                    \
    static_assert(sizeof(*(out)->top) == sizeof(*(src)->top));          \
    const auto tmp = stack_len(src);                                    \
    if (tmp > 0) {                                                      \
      memcpy((out)->top, (src)->floor, tmp * (Ind)sizeof(*(out)->top)); \
      (out)->top += tmp;                                                \
    }                                                                   \
  })

// Pushes to the output stack/span everything from the source stack/span.
#define stack_push_from(...) stack_push_from_inner(UNIQ_IDENT, __VA_ARGS__)

// Index of given stack element, by pointer.
// Providing an invalid pointer is UB.
#define stack_ind(stack, val) ((Ind)((val) - (stack)->floor))

#define stack_rewind_inner(tmp_next, tmp_prev, next, prev) \
  ({                                                       \
    const auto tmp_next = next;                            \
    const auto tmp_prev = prev;                            \
    aver(tmp_next->floor == tmp_prev->floor);              \
    tmp_next->top = tmp_prev->top;                         \
  })

#define stack_rewind(...) \
  stack_rewind_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define is_stack_elem_inner(tmp_stack, tmp_ptr, stack, ptr)   \
  ({                                                          \
    static_assert(sizeof(*ptr) == stack_val_size(stack));     \
    const auto tmp_stack = stack;                             \
    const auto tmp_ptr   = ptr;                               \
    is_aligned(tmp_ptr) &&                                    \
      tmp_ptr >= tmp_stack->floor &&tmp_ptr < tmp_stack->top; \
  })

#define is_stack_elem(...) \
  is_stack_elem_inner(UNIQ_IDENT, UNIQ_IDENT, __VA_ARGS__)

#define stack_range(type, name, stack) \
  type name = (stack)->floor;          \
  name < (stack)->top;                 \
  name++

#define stack_range_reverse(type, name, stack) \
  type name = (stack)->top - 1;                \
  name >= (stack)->floor;                      \
  name--
