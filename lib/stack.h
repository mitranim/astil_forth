#pragma once
#include "./err.h"
#include "./misc.h"
#include "./num.h"
#include <assert.h>
#include <string.h>

// Stack with guards. Macros below assume the "empty ascending" style.
// SYNC[stack_floor_top].
typedef struct {
  Uint  bytelen; // Size of full region in bytes.
  void *cellar;  // Starting address of full region: `guard|stack|guard`.
  void *floor;   // Starting address of stack region.
  void *top;     // Address of "current" stack item, if any.
  void *ceil;    // Starting address of second guard, just above the stack.
} Stack;

#define stack_of(Elem) \
  struct {             \
    Uint  bytelen;     \
    Elem *cellar;      \
    Elem *floor;       \
    Elem *top;         \
    Elem *ceil;        \
  }

typedef struct {
  Uint len;
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

#define stack_cap(stack) ((stack)->ceil - (stack)->floor)
#define stack_len(stack) ((stack)->top - (stack)->floor)
#define stack_rem(stack) ((stack)->ceil - (stack)->top)

#define stack_val_type(stack) typeof((stack)->floor[0])
#define stack_val_size(stack) sizeof((stack)->floor[0])

#define stack_cap_bytes(stack) \
  ((Uint)((U8 *)(stack)->ceil - (U8 *)(stack)->floor))

#define stack_len_bytes(stack) \
  ((Uint)((U8 *)(stack)->top - (U8 *)(stack)->floor))

#define stack_head(stack) (stack)->top[-1]
#define stack_trunc(stack) (stack)->top = (stack)->floor

#define stack_push(stack, ...)         \
  ({                                   \
    const auto tmp_ptr = (stack)->top; \
    aver(tmp_ptr < (stack)->ceil);     \
    *tmp_ptr     = __VA_ARGS__;        \
    (stack)->top = tmp_ptr + 1;        \
    tmp_ptr;                           \
  })

#define stack_pop(stack)                  \
  ({                                      \
    assert((stack)->top < (stack)->ceil); \
    *(--(stack)->top);                    \
  })

#define stack_has_cap(stack) ((stack)->top < (stack)->ceil)

// Pushes the raw memory representation of the given value.
#define stack_push_raw(stack, ...)                                             \
  ({                                                                           \
    const auto     stack_ptr = stack;                                          \
    const auto     src_val   = __VA_ARGS__;                                    \
    const auto     out_ptr   = stack_ptr->top;                                 \
    constexpr auto val_size  = (Uint)sizeof(src_val);                          \
    constexpr auto elem_size = (Uint)sizeof(*stack_ptr->top);                  \
    static_assert(val_size >= elem_size && divisible_by(val_size, elem_size)); \
    memcpy(out_ptr, &src_val, val_size);                                       \
    (stack)->top += val_size / elem_size;                                      \
    out_ptr;                                                                   \
  })

// Pushes to the output stack/span everything from the source stack/span.
#define stack_push_from(out, src)                                  \
  ({                                                               \
    static_assert(sizeof(*(out)->top) == sizeof(*(src)->top));     \
    const auto len = stack_len(src);                               \
    if (len > 0) {                                                 \
      memcpy((out)->top, (src)->floor, len * sizeof(*(out)->top)); \
      (out)->top += len;                                           \
    }                                                              \
  })

#define stack_ind(stack, val) ((val) - (stack)->floor)

#define stack_range(type, name, stack) \
  type name = (stack)->floor;          \
  name < (stack)->top;                 \
  name++

#define stack_range_reverse(type, name, stack) \
  type name = (stack)->top - 1;                \
  name >= (stack)->floor;                      \
  name--
