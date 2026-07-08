// Simple hash-based sets. Append-only, no deletion. No string support.
#pragma once
#include "./set.h" // IWYU pragma: export
#include "./err.h"
#include "./fmt.h"
#include "./hash_table_common.c"
#include "./mem.h"
#include "./misc.h"
#include "./num.h"

static void set_deinit(void *set) {
  const auto tar = (Set *)set;
  if (!tar) return;
  free(tar->bits);
  free(tar->vals);
  *tar = (Set){};
}

static void set_init_impl(Set *set, Ind cap, Ind size) {
  *set = (Set){};
  if (!cap) return;

  // Power of 2, or 0. Assumed by `set_loaded` and `set_probe_ind`.
  IF_DEBUG(assert_fatal(!(cap & (cap - 1))));

  const auto bits_len = hash_table_bits_arr_len(cap);
  const auto bits     = calloc(bits_len, sizeof(set->bits[0]));
  const auto vals     = memalloc(cap, size);

  assert_fatal(bits);
  if (size) assert_fatal(vals);

  *set = (Set){
    .bits = bits,
    .vals = vals,
    .cap  = cap,
  };
}

// clang-format off

static bool set_valid(Set const *set) {
  return (
    set &&
    is_aligned(set) &&
    is_aligned(&set->len) &&
    is_aligned(&set->cap) &&
    is_aligned(&set->bits) &&
    is_aligned(&set->vals) &&
    ((
      !set->len &&
      !set->cap &&
      !set->bits &&
      !set->vals
    ) ||
    (
      set->cap > 0 &&
      set->len < set->cap &&
      set->bits
    ))
  );
}

// clang-format on

static bool set_loaded(const Set *set) {
  return hash_table_loaded((const Hash_table *)set);
}

static Ind set_matching_ind(const Set *set, const void *val, Ind size) {
  return hash_table_matching_ind(
    (const Hash_table *)set, val, size, mem_hash, mem_eq
  );
}

static Ind set_available_ind(
  const Set *set, const void *val, Ind size, bool *new
) {
  /*
  `Set.vals` lines up with `Hash_table.keys`. For a new slot, shared code
  copies `val` into `Set.vals`; callers must only update bits and length.
  */
  return hash_table_available_ind(
    (const Hash_table *)set, val, size, mem_hash, mem_eq, new
  );
}

static void set_rehash(Set *prev, Ind cap, Ind size) {
  Set next;
  set_init_impl(&next, cap, size);

  for (Ind ind_prev = 0; ind_prev < prev->cap; ind_prev++) {
    if (!hash_table_bits_get_at(prev->bits, ind_prev)) continue;

    bool       new;
    const auto val_prev = ptr_at(prev->vals, ind_prev, size);
    const auto ind_next = set_available_ind(&next, val_prev, size, &new);
    assert_fatal(new);
    hash_table_bits_set_at(next.bits, ind_next);
    next.len++;
  }

  set_deinit(prev);
  *prev = next;
}

static void *set_add_impl(Set *set, const void *val, Ind size) {
  if (!set->cap) {
    set_init_impl(set, HASH_TABLE_INIT_CAP, size);
  }
  else if (set_loaded(set)) {
    set_rehash(set, set->cap * 2, size);
  }

  bool       new;
  const auto ind = set_available_ind(set, val, size, &new);
  const auto out = ptr_at(set->vals, ind, size);

  if (new) {
    hash_table_bits_set_at(set->bits, ind);
    set->len++;
  }
  return out;
}

static void set_eprint_repr(Set *set) {
  eprintf(
    "(Set){\n"
    "  .len  = " FMT_IND
    ",\n"
    "  .cap  = " FMT_IND
    ",\n"
    "  .bits = %p,\n"
    "  .vals = %p,\n"
    "}\n",
    set->len,
    set->cap,
    set->bits,
    set->vals
  );
}

/*
#include "./err.h"
#include "./misc.h"
#include <limits.h>
#include <stdio.h>

int main() {
  deferred(set_deinit) set_of(Sint) set = {};
  // set_eprint_repr((Set *)&set);

  assert_fatal(set.len == 0);
  assert_fatal(set.cap == 0);
  assert_fatal(!set_has(&set, 0));
  assert_fatal(!set_has(&set, 10));
  assert_fatal(!set_has(&set, 20));

  for (span(3)) {
    set_add(&set, 10);
    // set_eprint_repr((Set *)&set);
    assert_fatal(set.len == 1);
    assert_fatal(set.cap == HASH_TABLE_INIT_CAP);
    assert_fatal(!set_has(&set, 0));
    assert_fatal(set_has(&set, 10));
    assert_fatal(!set_has(&set, 20));
  }

  for (span(3)) {
    set_add(&set, 20);
    // set_eprint_repr((Set *)&set);
    assert_fatal(set.len == 2);
    assert_fatal(set.cap == HASH_TABLE_INIT_CAP);
    assert_fatal(!set_has(&set, 0));
    assert_fatal(set_has(&set, 10));
    assert_fatal(set_has(&set, 20));
    assert_fatal(!set_has(&set, 30));
  }

  for (span(3)) {
    set_add(&set, 30);
    // set_eprint_repr((Set *)&set);
    assert_fatal(set.len == 3);
    assert_fatal(!set_has(&set, 0));
    assert_fatal(set_has(&set, 10));
    assert_fatal(set_has(&set, 20));
    assert_fatal(set_has(&set, 30));
    assert_fatal(!set_has(&set, 40));
  }
  assert_fatal(set.cap == HASH_TABLE_INIT_CAP * 2);

  for (span(3)) {
    set_add(&set, 40);
    // set_eprint_repr((Set *)&set);
    assert_fatal(set.len == 4);
    assert_fatal(set.cap == HASH_TABLE_INIT_CAP * 2);
    assert_fatal(!set_has(&set, 0));
    assert_fatal(set_has(&set, 10));
    assert_fatal(set_has(&set, 20));
    assert_fatal(set_has(&set, 30));
    assert_fatal(set_has(&set, 40));
    assert_fatal(!set_has(&set, 50));
  }

  for (span(3)) {
    set_add(&set, 50);
    // set_eprint_repr((Set *)&set);
    assert_fatal(set.len == 5);
    assert_fatal(!set_has(&set, 0));
    assert_fatal(set_has(&set, 10));
    assert_fatal(set_has(&set, 20));
    assert_fatal(set_has(&set, 30));
    assert_fatal(set_has(&set, 40));
    assert_fatal(set_has(&set, 50));
    assert_fatal(!set_has(&set, 60));
  }
  assert_fatal(set.cap == HASH_TABLE_INIT_CAP * 4);

  set_deinit(&set);
  assert_fatal(!set.vals);
  assert_fatal(!set.bits);
  assert_fatal(!set.len);
  assert_fatal(!set.cap);
}
*/
