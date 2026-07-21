/*
Alternative to `./list.c`. Stacks are created with fixed capacity,
with no support for resizing, which allows stable element pointers.
Each stack is surrounded with guards which trigger a segfault when
touched. This allows relatively fearless `*ptr++`-style operations.
*/
#pragma once
#include "./stack.h"
#include "./err.h"
#include "./fmt.c"
#include "./mem.c"
#include "./mem.h"
#include "./num.h"
#include <assert.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <unistd.h>

static Err stack_deinit(void *stack) {
  const auto tar = (Stack *)stack;
  if (!tar) return nullptr;

  const auto addr = tar->cellar;
  const auto err  = addr ? err_errno(munmap(addr, tar->bytelen)) : nullptr;

  *tar = (Stack){};
  return err;
}

static Err err_stack_no_len() {
  return err_str("unable to allocate stack: missing length");
}

static Err err_no_page_size(int size) {
  return errf(
    "unable to obtain page size via `getpagesize`; got unexpected value %d", size
  );
}

/*
Stack is surrounded by inaccessible guards.
Underflow or overflow triggers a segfault.

  🚫🚫🚫🚫🔹🔹🔹🔹🔹🔹🔹🔹🚫🚫🚫🚫
  guard    stack            guard
*/
static Err stack_init_impl(void *out, Stack_opt *opt, Ind val_size) {
  *(Stack *)out = (Stack){};

  const auto len = opt ? opt->len : 0;
  if (!len) return err_stack_no_len();

  const int page = getpagesize();
  if (!(page >= 0 && page < INT_MAX)) return err_no_page_size(page);

  const auto page_size  = (Ind)page;
  const auto data_size  = __builtin_align_up(MUL(val_size, len), page_size);
  const auto total_size = page_size + data_size + page_size;

  const auto cellar = mem_map(total_size, 0);
  if (cellar == MAP_FAILED) return err_mmap();

  const auto floor = (U8 *)cellar + page_size;
  const auto err   = mem_protect(floor, data_size, PROT_READ | PROT_WRITE);
  if (err) {
    munmap(cellar, total_size);
    return err;
  }

  *(Stack *)out = (Stack){
    .bytelen = total_size,
    .cellar  = cellar,
    .floor   = floor,
    .top     = floor, // Empty ascending.
    .ceil    = floor + data_size,
  };
  return nullptr;
}

// clang-format off

static bool stack_valid(Stack const *stack) {
  return (
    stack &&
    is_aligned(stack) &&
    is_aligned(&stack->bytelen) &&
    is_aligned(&stack->cellar) &&
    is_aligned(&stack->floor) &&
    is_aligned(&stack->top) &&
    is_aligned(&stack->ceil) &&
    stack->bytelen &&
    stack->cellar &&
    stack->cellar < stack->floor &&
    stack->floor &&
    stack->floor < stack->ceil &&
    stack->top &&
    stack->ceil
  );
}

// clang-format on

static bool span_valid(const Span *span) {
  return span && span->floor && span->floor <= span->top &&
    span->top <= span->ceil;
}

static void stack_rewind_impl(const Stack *prev, Stack *next) {
  assert_fatal(next->floor == prev->floor);
  next->top = prev->top;
}

static void Stack_repr(Stack *val) {
  print_struct_beg(val, Stack);
  print_struct_field(val, bytelen);
  print_struct_field(val, cellar);
  print_struct_field(val, floor);
  print_struct_field(val, top);
  print_struct_field(val, ceil);
  print_struct_end();
}

// Placed here when I was too tired to properly organize.
// TODO consider where this should be moved.
static Err int_stack_pop(Sint_stack *src, Sint *out) {
  if (stack_len(src) <= 0) return err_str("integer stack underflow");
  const auto val = stack_pop(src);
  if (out) *out = val;
  return nullptr;
}

static Err int_stack_push(Sint_stack *tar, Sint val) {
  if (stack_rem(tar) <= 0) return err_str("integer stack overflow");
  stack_push(tar, val);
  return nullptr;
}

static void stack_valid_count_type_test() {
  static_assert_type(stack_cap_valid((U8_span *)nullptr), Ind);
  static_assert_type(stack_len_valid((U8_span *)nullptr), Ind);
  static_assert_type(stack_rem_valid((U8_span *)nullptr), Ind);
}

/*
#include <stdio.h>

static Err run() {
  deferred(stack_deinit) stack_of(Uint) stack = {};

  Stack_opt opt = {.len = 8};
  try(stack_init(&stack, &opt));

  printf("cellar: %p\n", (void *)stack.cellar);
  printf("top:    %p\n", (void *)stack.top);
  printf("ceil:   %p\n", (void *)stack.ceil);
  printf("len:    " FMT_UINT "\n", stack_len(&stack));

  // stack.cellar[0] = 123; // segfault
  // stack.ceil[0]   = 234; // segfault
  // stack.top[-1]   = 123; // segfault

  *stack.top++ = 123; // ok
  *stack.top++ = 234; // ok

  printf("len:    " FMT_UINT "\n", stack_len(&stack));
  printf("head:   " FMT_UINT "\n", stack.top[-1]);
  printf("head:   " FMT_UINT "\n", stack_head(&stack));

  *--stack.top = 345; // ok
  *--stack.top = 456; // ok

  // *--stack.top = 567; // segfault

  printf("len:    " FMT_UINT "\n", stack_len(&stack));
  return nullptr;
}

int main() { try_main(run()); }
*/
