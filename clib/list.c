/*
Replica of Go slices. See example below (`main`).
Resizing relocates memory; pointers are transient.
*/
#pragma once
#include "./err.c"
#include "./list.h" // IWYU pragma: export
#include "./mem.h"
#include "./misc.h"

/*
Frees the data and zeroes all fields. Usage:

  deferred(list_deinit) list_of(int) some_list = {};
*/
static void list_deinit(void *src) {
  const auto tar = (List *)src;
  if (!tar) return;
  free(tar->dat);
  *tar = (List){};
}

static void list_reserve_total_cap_impl(List *tar, Ind size, Ind cap) {
  if (cap <= tar->cap) return;

  const auto next = mul(cap, size);
  const auto dat  = realloc(tar->dat, next);
  aver(dat);

  tar->dat = dat;
  tar->cap = cap;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)

static void list_reserve_spare_cap_impl(List *tar, Ind size, Ind more) {
  const auto goal = tar->len + more;
  auto       next = tar->cap;
  while (next < goal) next *= 2;
  list_reserve_total_cap_impl(tar, size, next);
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static void list_reserve_more(List *tar, Ind size) {
  if (tar->len < tar->cap) return;
  auto cap = tar->cap * 2;
  if (!cap) cap = LIST_INIT_CAP;
  list_reserve_total_cap_impl(tar, size, cap);
}

static void list_push_raw_impl(
  List *tar, Ind size, const void *src, Uint len, const char *file, int line
) {
  if (!len) return;
  aver(divisible_by(len, size));

  const auto rem = (tar->cap - tar->len) * size;
  if (!(rem >= len)) {
    fatal("unable to push raw data: out of capacity", file, line);
  }

  const auto off = mul(tar->len, size);
  memcpy(((U8 *)tar->dat + off), src, len);
  tar->len += len / size;
}

// clang-format off

static bool list_valid(const List *list) {
  return (
    list &&
    is_aligned(list) &&
    is_aligned(&list->dat) &&
    is_aligned(&list->len) &&
    is_aligned(&list->cap) &&
    (list->len <= list->cap) &&
    (!list->dat || list->cap)
  );
}

// clang-format on

static void *list_ceil_impl(const List *list, Ind size) {
  return (U8 *)list->dat + mul(list->cap, size);
}

static bool is_list_elem_impl(
  const List *list, Ind size, const void *ptr, Uint align
) {
  return (
    list && list->len && __builtin_is_aligned(ptr, align) &&
    (ptr >= list->dat) && (ptr < list_ceil_impl(list, size))
  );
}

// [[noreturn]]
// void fatal_list_at_capacity(const char *expr, const char *file, int line) {
//   fatal_expr("unable to push element into list: capacity full", expr, file, line);
// }

/*
#include "./misc.h"
#include "./num.h"
#include <stdio.h>

int main(void) {
  deferred(list_deinit) list_of(Uint) list = {};

  // list_head(&list); // crash

  list_append(&list, 123);
  list_append(&list, 234);
  list_append(&list, 345);
  list_append(&list, 456);
  list_append(&list, 567);
  list_append(&list, 678);

  printf("list.len:            " FMT_IND "\n", list.len);
  printf("list.cap:            " FMT_IND "\n", list.cap);
  printf("list_elem(&list, 0): " FMT_UINT "\n", list_elem(&list, 0));
  printf("list_elem(&list, 1): " FMT_UINT "\n", list_elem(&list, 1));
  printf("list_head(&list):    " FMT_UINT "\n", list_head(&list));
  printf("list_last(&list):    " FMT_UINT "\n", list_last(&list));
}
*/
