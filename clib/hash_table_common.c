/*
Common code for dicts, maps, and sometimes sets. In this codebase:
- "dict" = "hash table with string keys"
- "map" = "hash table with opaque keys"
*/
#pragma once
#include "./hash_table_common.h"
#include "./err.h"
#include "./fmt.h"
#include "./hash_fnv.c"
#include "./mem.c"
#include "./mem.h"
#include "./misc.h"
#include <stdlib.h>

// One bit per entry, with no tombstones: our hash tables are append-only.
static constexpr U8 HASH_TABLE_BITS_SIZE = sizeof((Hash_table){}.bits);
static constexpr U8 HASH_TABLE_BITS_CAP  = HASH_TABLE_BITS_SIZE * CHAR_BIT;
static constexpr U8 HASH_TABLE_INIT_CAP  = 4;

// Conforms to `Hash_fun`.
static Fnv_hash mem_hash(const void *key, Ind len) {
  return fnv_hash_bytes(key, len);
}

// Conforms to `Eq_fun`.
static bool mem_eq(const void *one, const void *two, Ind len) {
  if (!len) return true;
  return !memcmp(one, two, len);
}

static void hash_table_deinit(void *tab) {
  const auto tar = (Hash_table *)tab;
  if (!tar) return;
  free(tar->bits);
  free(tar->keys);
  free(tar->vals);
  *tar = (Hash_table){};
}

/*
How many chunks of bits correspond to this capacity.
`cap` 0 = len 0, otherwise min 1.
*/
static Ind hash_table_bits_arr_len(Ind cap) {
  return divide_round_up(cap, HASH_TABLE_BITS_CAP);
}

static void hash_table_init(Hash_table *out, Ind cap, Ind key_size, Ind val_size) {
  *out = (Hash_table){};
  if (!cap) return;

  // Power of 2, or 0. Assumed by `hash_table_loaded` and `hash_table_probe_ind`.
  IF_DEBUG(assert_fatal(!(cap & (cap - 1))));

  auto bits_len = hash_table_bits_arr_len(cap);

  const auto bits = calloc(bits_len, HASH_TABLE_BITS_SIZE);
  const auto keys = memalloc(cap, key_size);
  const auto vals = memalloc(cap, val_size);

  assert_fatal(bits);
  if (key_size) assert_fatal(keys);
  if (val_size) assert_fatal(vals);

  *out = (Hash_table){
    .cap  = cap,
    .bits = bits,
    .keys = keys,
    .vals = vals,
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
        tab->bits
    ))
  );
}

// clang-format on

// Truncates the whole table while preserving the allocated capacity.
static void hash_table_trunc(Hash_table *tab) {
  auto bits_len = hash_table_bits_arr_len(tab->cap);
  for (Ind ind = 0; ind < bits_len; ind++) {
    tab->bits[ind] = 0;
  }
  tab->len = 0;
}

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

// Load factor 50%. Assumes `cap` is power of 2.
static bool hash_table_loaded(const Hash_table *tab) {
  return (tab->len << 1u) > tab->cap;
}

// Quadratic probing. Assumes `cap` is power of 2.
static Ind hash_table_probe(Ind iter) { return ((iter * iter) + iter) / 2; }

static Ind hash_table_probe_ind(const Hash_table *tab, Fnv_hash hash, Ind iter) {
  return (Ind)modulo2((hash + hash_table_probe(iter)), tab->cap);
}

static Ind hash_table_matching_ind(
  const Hash_table *tab,
  const void       *key,
  Ind               key_size,
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
Must have capacity. If key is new, stores it and reports `new`.
The caller is responsible for updating bits, length, and value.
*/
static Ind hash_table_available_ind(
  const Hash_table *tab,
  const void       *key,
  Ind               key_size,
  Hash_fun          hash_fun,
  Eq_fun            eq_fun,
  bool             *new
) {
  assert_fatal(tab->cap > tab->len);
  const auto hash = hash_fun(key, key_size);

  for (Ind iter = 0; iter < tab->cap; iter++) {
    const auto ind     = hash_table_probe_ind(tab, hash, iter);
    const auto key_ptr = ptr_at(tab->keys, ind, key_size);

    if (!hash_table_bits_get_at(tab->bits, ind)) {
      if (key_size) memcpy(key_ptr, key, key_size);
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
  Ind               key_size,
  Hash_fun          hash_fun,
  Eq_fun            eq_fun
) {
  return hash_table_matching_ind(tab, key, key_size, hash_fun, eq_fun) !=
    INVALID_IND;
}

static void hash_table_rehash(
  Hash_table *prev,
  Ind         cap,
  Ind         key_size,
  Ind         val_size,
  Hash_fun    hash_fun,
  Eq_fun      eq_fun
) {
  Hash_table next;
  hash_table_init(&next, cap, key_size, val_size);

  for (Ind ind_prev = 0; ind_prev < prev->cap; ind_prev++) {
    if (!hash_table_bits_get_at(prev->bits, ind_prev)) continue;

    bool       new;
    const auto key_prev = ptr_at(prev->keys, ind_prev, key_size);
    const auto ind_next = hash_table_available_ind(
      &next, key_prev, key_size, hash_fun, eq_fun, &new
    );
    assert_fatal(new);

    // `hash_table_available_ind` already copied the key.
    // Only non-zero-sized values are allocated or copied; see `Str_set`.
    if (val_size) {
      const auto val_prev = ptr_at(prev->vals, ind_prev, val_size);
      const auto val_next = ptr_at(next.vals, ind_next, val_size);
      memcpy(val_next, val_prev, val_size);
    }

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
  Ind         key_size,
  Ind         val_size,
  Hash_fun    hash_fun,
  Eq_fun      eq_fun
) {
  if (!tab->cap) {
    hash_table_init(tab, HASH_TABLE_INIT_CAP, key_size, val_size);
  }
  else if (hash_table_loaded(tab)) {
    hash_table_rehash(tab, tab->cap * 2, key_size, val_size, hash_fun, eq_fun);
  }

  bool       new;
  const auto ind = hash_table_available_ind(
    tab, key, key_size, hash_fun, eq_fun, &new
  );

  if (new) {
    // `hash_table_available_ind` already copied the key.
    hash_table_bits_set_at(tab->bits, ind);
    tab->len++;
  }

  // Only non-zero-sized values are allocated or copied; see `Str_set`.
  if (!val_size) return nullptr;

  const auto out = ptr_at(tab->vals, ind, val_size);
  memcpy(out, val, val_size);
  return out;
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
