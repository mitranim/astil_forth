#pragma once
#include "./err.h"
#include "./fmt.c"
#include "./mem.c"
#include "./mem.h"
#include "./num.h"
#include "./stack.h"
#include <assert.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <unistd.h>

static Err stack_deinit(void *stack) {
  const auto tar = (Stack *)stack;
  if (!tar) return nullptr;

  const auto addr = tar->cellar;
  if (addr) try_errno(munmap(addr, tar->bytelen));

  *tar = (Stack){};
  return nullptr;
}

static Err err_stack_no_len() {
  return err_str("unable to allocate stack: missing length");
}

/*
Stack is surrounded by inaccessible guards.
Underflow or overflow triggers a segfault.

  🚫🚫🚫🚫🔹🔹🔹🔹🔹🔹🔹🔹🚫🚫🚫🚫
    guard       stack        guard
*/
static Err stack_init_impl(void *out, Stack_opt *opt, Uint val_size) {
  const auto len = opt ? opt->len : 0;
  if (!len) return err_stack_no_len();

  const auto page_size = (Uint)sysconf(_SC_PAGESIZE);
  aver(page_size != (Uint)-1);

  const auto guard_size = page_size;
  const auto data_size  = val_size * len;
  const auto total_size = __builtin_align_up(
    (guard_size + data_size + guard_size), page_size
  );

  const auto cellar = mem_map(total_size, 0);
  if (cellar == MAP_FAILED) return err_mmap();

  const auto floor = (U8 *)cellar + guard_size;
  try(mprotect_mutable(floor, data_size));

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
  if (!stack_len(src)) return err_str("underflow of integer stack");
  const auto val = stack_pop(src);
  if (out) *out = val;
  return nullptr;
}

static Err int_stack_push(Sint_stack *tar, Sint val) {
  if (!stack_has_cap(tar)) return err_str("overflow of integer stack");
  stack_push(tar, val);
  return nullptr;
}

/*
#include <stdio.h>

Err run(void) {
  defer(stack_deinit) stack_of(Uint) stack = {};

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

int main(void) { try_main(run()); }
*/
