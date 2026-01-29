#pragma once
#include "./err.h"
#include "./fmt.h"
#include "./hash_table_common.c"
#include "./mem.h"
#include "./misc.h"
#include "./num.h"
#include "./set.h" // IWYU pragma: export

static void set_deinit(void *set) {
  const auto tar = (Set *)set;
  if (!tar) return;
  if (tar->bits) free(tar->bits);
  if (tar->vals) free(tar->vals);
  *tar = (Set){};
}

static void set_init_impl(Set *set, Ind cap, Uint size) {
  if (!cap) {
    *set = (Set){};
    return;
  }

  // Power of 2, or 0. Assumed by `set_loaded` and `set_probe_ind`.
  IF_DEBUG(aver(!(cap & (cap - 1))));

  // Round-up divide; also min 1 unless cap is 0.
  auto bits_len = (cap + HASH_TABLE_BITS_CAP - 1) / HASH_TABLE_BITS_CAP;

  *set = (Set){
    .bits = calloc(bits_len, sizeof(set->bits[0])),
    .vals = memalloc(cap, size),
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
      set->bits &&
      set->vals
    ))
  );
}

// clang-format on

static bool set_loaded(const Set *set) {
  return hash_table_loaded((const Hash_table *)set);
}

static Ind set_matching_ind(const Set *set, const void *val, Uint size) {
  return hash_table_matching_ind(
    (const Hash_table *)set, val, size, mem_hash, mem_eq
  );
}

static Ind set_available_ind(
  const Set *set, const void *val, Uint size, bool *new
) {
  return hash_table_available_ind(
    (const Hash_table *)set, val, size, mem_hash, mem_eq, new
  );
}

static void set_rehash(Set *prev, Ind cap, Uint size) {
  Set next;
  set_init_impl(&next, cap, size);

  for (Ind ind_prev = 0; ind_prev < prev->cap; ind_prev++) {
    if (!hash_table_bits_get_at(prev->bits, ind_prev)) continue;

    bool new;
    const auto val_prev = ptr_at(prev->vals, ind_prev, size);
    const auto ind_next = set_available_ind(&next, val_prev, size, &new);
    const auto val_next = ptr_at(next.vals, ind_next, size);

    memcpy(val_next, val_prev, size);
    hash_table_bits_set_at(next.bits, ind_next);
    next.len++;
  }

  set_deinit(prev);
  *prev = next;
}

static void *set_add_impl(Set *set, const void *val, Uint size) {
  if (!set->cap) {
    set_init_impl(set, HASH_TABLE_INIT_CAP, size);
  }
  else if (set_loaded(set)) {
    set_rehash(set, set->cap * 2, size);
  }

  bool new;
  const auto ind = set_available_ind(set, val, size, &new);
  const auto out = ptr_at(set->vals, ind, size);

  if (new) {
    memcpy(out, val, size);
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

int main(void) {
  defer(set_deinit) set_of(Sint) set = {};
  // set_eprint_repr((Set *)&set);

  aver(set.len == 0);
  aver(set.cap == 0);
  aver(!set_has(&set, 0));
  aver(!set_has(&set, 10));
  aver(!set_has(&set, 20));

  for (span(3)) {
    set_add(&set, 10);
    // set_eprint_repr((Set *)&set);
    aver(set.len == 1);
    aver(set.cap == HASH_TABLE_INIT_CAP);
    aver(!set_has(&set, 0));
    aver(set_has(&set, 10));
    aver(!set_has(&set, 20));
  }

  for (span(3)) {
    set_add(&set, 20);
    // set_eprint_repr((Set *)&set);
    aver(set.len == 2);
    aver(set.cap == HASH_TABLE_INIT_CAP);
    aver(!set_has(&set, 0));
    aver(set_has(&set, 10));
    aver(set_has(&set, 20));
    aver(!set_has(&set, 30));
  }

  for (span(3)) {
    set_add(&set, 30);
    // set_eprint_repr((Set *)&set);
    aver(set.len == 3);
    aver(!set_has(&set, 0));
    aver(set_has(&set, 10));
    aver(set_has(&set, 20));
    aver(set_has(&set, 30));
    aver(!set_has(&set, 40));
  }
  aver(set.cap == HASH_TABLE_INIT_CAP * 2);

  for (span(3)) {
    set_add(&set, 40);
    // set_eprint_repr((Set *)&set);
    aver(set.len == 4);
    aver(set.cap == HASH_TABLE_INIT_CAP * 2);
    aver(!set_has(&set, 0));
    aver(set_has(&set, 10));
    aver(set_has(&set, 20));
    aver(set_has(&set, 30));
    aver(set_has(&set, 40));
    aver(!set_has(&set, 50));
  }

  for (span(3)) {
    set_add(&set, 50);
    // set_eprint_repr((Set *)&set);
    aver(set.len == 5);
    aver(!set_has(&set, 0));
    aver(set_has(&set, 10));
    aver(set_has(&set, 20));
    aver(set_has(&set, 30));
    aver(set_has(&set, 40));
    aver(set_has(&set, 50));
    aver(!set_has(&set, 60));
  }
  aver(set.cap == HASH_TABLE_INIT_CAP * 4);

  set_deinit(&set);
  aver(!set.vals);
  aver(!set.bits);
  aver(!set.len);
  aver(!set.cap);
}
*/
