#pragma once
#include "./err.h"
#include "./fmt.h"
#include "./hash.c"
#include "./hash_table_common.h"
#include "./mem.c"
#include "./mem.h"
#include "./misc.h"
#include <stdlib.h>

// One bit per entry, with no tombstones: our hash tables are append-only.
static constexpr Uint HASH_TABLE_BITS_SIZE = sizeof((Hash_table){}.bits);
static constexpr Uint HASH_TABLE_BITS_CAP  = HASH_TABLE_BITS_SIZE * CHAR_BIT;
static constexpr Uint HASH_TABLE_INIT_CAP  = 4;

// Conforms to `Hash_fun`.
static Hash mem_hash(const void *key, Uint len) { return hash_bytes(key, len); }

// Conforms to `Eq_fun`.
static bool mem_eq(const void *one, const void *two, Uint len) {
  return !memcmp(one, two, len);
}

static void hash_table_deinit(void *tab) {
  const auto tar = (Hash_table *)tab;
  if (!tar) return;
  if (tar->bits) free(tar->bits);
  if (tar->keys) free(tar->keys);
  if (tar->vals) free(tar->vals);
  *tar = (Hash_table){};
}

static void hash_table_init(
  Hash_table *out, Ind cap, Uint key_size, Uint val_size
) {
  if (!cap) {
    *out = (Hash_table){};
    return;
  }

  // Power of 2, or 0. Assumed by `hash_table_loaded` and `hash_table_probe_ind`.
  IF_DEBUG(aver(!(cap & (cap - 1))));

  // Round-up divide; also min 1 unless cap is 0.
  auto bits_len = (cap + HASH_TABLE_BITS_CAP - 1) / HASH_TABLE_BITS_CAP;

  *out = (Hash_table){
    .cap  = cap,
    .bits = calloc(bits_len, HASH_TABLE_BITS_SIZE),
    .keys = memalloc(cap, key_size),
    .vals = memalloc(cap, val_size),
  };
}

// clang-format off

static bool hash_table_valid(const Hash_table *tab) {
  return (
    tab &&
    is_aligned(tab) &&
    is_aligned(&tab->len) &&
    is_aligned(&tab->cap) &&
    is_aligned(&tab->bits) &&
    is_aligned(&tab->keys) &&
    is_aligned(&tab->vals) &&
    ((
      !tab->len &&
      !tab->cap &&
      !tab->bits &&
      !tab->keys &&
      !tab->vals
    ) ||
    (
        tab->cap > 0 &&
        tab->len < tab->cap &&
        tab->bits &&
        tab->keys &&
        tab->vals
    ))
  );
}

// clang-format on

static bool hash_table_bits_get_at(const Uint *bits, Ind ind) {
  return bits[ind / HASH_TABLE_BITS_CAP] &
    ((Uint)1 << (ind % HASH_TABLE_BITS_CAP));
}

static void hash_table_bits_set_at(Uint *bits, Ind ind) {
  bits[ind / HASH_TABLE_BITS_CAP] |= ((Uint)1 << (ind % HASH_TABLE_BITS_CAP));
}

// For loops.
static Ind hash_table_next(const Hash_table *tab, Ind ind) {
  for (; ind < tab->cap; ind++) {
    if (hash_table_bits_get_at(tab->bits, ind)) return ind;
  }
  return INVALID_IND;
}

// For loops.
static Ind hash_table_head(const Hash_table *tab) {
  return hash_table_next(tab, 0);
}

// Load factor 50%.
static bool hash_table_loaded(const Hash_table *tab) {
  return (tab->len << 1) > tab->cap;
}

// Quadratic probing.
static Ind hash_table_probe(Ind iter) { return (iter * iter + iter) / 2; }

static Ind hash_table_probe_ind(const Hash_table *tab, Hash hash, Ind iter) {
  return (Ind)modulo2((hash + hash_table_probe(iter)), tab->cap);
}

static Ind hash_table_matching_ind(
  const Hash_table *tab,
  const void       *key,
  Uint              key_size,
  Hash_fun          hash_fun,
  Eq_fun            eq_fun
) {
  const auto hash = hash_fun(key, key_size);

  for (Ind iter = 0; iter < tab->cap; iter++) {
    const auto ind = hash_table_probe_ind(tab, hash, iter);
    if (!hash_table_bits_get_at(tab->bits, ind)) continue;

    const auto key_ptr = ptr_at(tab->keys, ind, key_size);
    if (!eq_fun(key_ptr, key, key_size)) continue; // Slot collision;

    return ind;
  }

  return INVALID_IND;
}

/*
Must have capacity. Immediately stores the key and updates the bits
and the length. The caller is responsible for updating the value.
*/
static Ind hash_table_available_ind(
  const Hash_table *tab,
  const void       *key,
  Uint              key_size,
  Hash_fun          hash_fun,
  Eq_fun            eq_fun,
  bool *new
) {
  aver(tab->cap > tab->len);
  const auto hash = hash_fun(key, key_size);

  for (Ind iter = 0; iter < tab->cap; iter++) {
    const auto ind     = hash_table_probe_ind(tab, hash, iter);
    const auto key_ptr = ptr_at(tab->keys, ind, key_size);

    if (!hash_table_bits_get_at(tab->bits, ind)) {
      memcpy(key_ptr, key, key_size);
      if (new) *new = true;
      return ind;
    }

    if (eq_fun(key_ptr, key, key_size)) {
      if (new) *new = false;
      return ind;
    }

    // Slot collision.
  }

  unreachable();
}

static bool hash_table_has(
  const Hash_table *tab,
  const void       *key,
  Uint              key_size,
  Hash_fun          hash_fun,
  Eq_fun            eq_fun
) {
  return hash_table_matching_ind(tab, key, key_size, hash_fun, eq_fun) !=
    INVALID_IND;
}

static void hash_table_rehash(
  Hash_table *prev,
  Ind         cap,
  Uint        key_size,
  Uint        val_size,
  Hash_fun    hash_fun,
  Eq_fun      eq_fun
) {
  Hash_table next;
  hash_table_init(&next, cap, key_size, val_size);

  for (Ind ind_prev = 0; ind_prev < prev->cap; ind_prev++) {
    if (!hash_table_bits_get_at(prev->bits, ind_prev)) continue;

    bool new;
    const auto key_prev = ptr_at(prev->keys, ind_prev, key_size);
    const auto val_prev = ptr_at(prev->vals, ind_prev, val_size);
    const auto ind_next = hash_table_available_ind(
      &next, key_prev, key_size, hash_fun, eq_fun, &new
    );
    const auto key_next = ptr_at(next.keys, ind_next, key_size);
    const auto val_next = ptr_at(next.vals, ind_next, val_size);
    aver(new);

    memcpy(key_next, key_prev, key_size);
    memcpy(val_next, val_prev, val_size);
    hash_table_bits_set_at(next.bits, ind_next);
    next.len++;
  }

  hash_table_deinit(prev);
  *prev = next;
}

static void *hash_table_set(
  Hash_table *tab,
  const void *key,
  const void *val,
  Uint        key_size,
  Uint        val_size,
  Hash_fun    hash_fun,
  Eq_fun      eq_fun
) {
  if (!tab->cap) {
    hash_table_init(tab, HASH_TABLE_INIT_CAP, key_size, val_size);
  }
  else if (hash_table_loaded(tab)) {
    hash_table_rehash(tab, tab->cap * 2, key_size, val_size, hash_fun, eq_fun);
  }

  bool new;
  const auto ind = hash_table_available_ind(
    tab, key, key_size, hash_fun, eq_fun, &new
  );

  if (new) {
    const auto key_ptr = ptr_at(tab->keys, ind, key_size);
    memcpy(key_ptr, key, key_size);
    hash_table_bits_set_at(tab->bits, ind);
    tab->len++;
  }

  const auto val_ptr = ptr_at(tab->vals, ind, val_size);
  memcpy(val_ptr, val, val_size);
  return val_ptr;
}

static void hash_table_eprint_repr(const Hash_table *tab) {
  eprintf(
    "(Hash_table){\n"
    "  .len  = " FMT_IND
    ",\n"
    "  .cap  = " FMT_IND
    ",\n"
    "  .bits = %p,\n"
    "  .keys = %p,\n"
    "  .vals = %p,\n"
    "}\n",
    tab->len,
    tab->cap,
    tab->bits,
    tab->keys,
    tab->vals
  );
}
