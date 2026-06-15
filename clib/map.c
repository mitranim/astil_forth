/*
Simple hash-based maps. Append-only, no deletion.

In this codebase:

- "dict" = "hash table with string keys"
- "map" = "hash table with opaque keys"

See `./dict.h`, `./dict.c`.
*/
#pragma once
#include "./map.h" // IWYU pragma: export
#include "./hash_table_common.c"

static void map_deinit(void *map) { hash_table_deinit(map); }

static bool map_valid(const Map *map) {
  return hash_table_valid((const Hash_table *)map);
}

static Ind map_ind_impl(const Map *map, const void *key, Ind key_size) {
  return hash_table_matching_ind(
    (const Hash_table *)map, key, key_size, mem_hash, mem_eq
  );
}

static bool map_has_impl(Map *map, const void *key, Ind key_size) {
  return hash_table_matching_ind(
    (const Hash_table *)map, key, key_size, mem_hash, mem_eq
  );
}

static void *map_set_impl(
  Map *map, const void *key, const void *val, Ind key_size, Ind val_size
) {
  return hash_table_set(
    (Hash_table *)map, key, val, key_size, val_size, mem_hash, mem_eq
  );
}

static void map_eprint_repr(const Map *map) {
  hash_table_eprint_repr((const Hash_table *)map);
}

/*
#include "./err.h"
#include <limits.h>
#include <stdio.h>

int main() {
  deferred(map_deinit) map_of(Sint, Uint) map = {};
  // map_eprint_repr((const Map *)&map);

  assert_fatal(map.len == 0);
  assert_fatal(map.cap == 0);

  assert_fatal(!map_has(&map, 0));
  assert_fatal(!map_has(&map, -11));
  assert_fatal(!map_has(&map, -22));

  assert_fatal(map_get(&map, 0) == 0);
  assert_fatal(map_get(&map, -11) == 0);
  assert_fatal(map_get(&map, -22) == 0);

  map_set(&map, -11, 10);
  // map_eprint_repr((const Map *)&map);
  assert_fatal(map.len == 1);
  assert_fatal(map.cap == HASH_TABLE_INIT_CAP);
  assert_fatal(!map_has(&map, 0));
  assert_fatal(map_has(&map, -11));
  assert_fatal(map_get(&map, -11) == 10);
  assert_fatal(!map_has(&map, -22));

  map_set(&map, -11, 20);
  // map_eprint_repr((const Map *)&map);
  assert_fatal(map.len == 1);
  assert_fatal(map.cap == HASH_TABLE_INIT_CAP);
  assert_fatal(!map_has(&map, 0));
  assert_fatal(map_get(&map, -11) == 20);
  assert_fatal(!map_has(&map, -22));

  map_set(&map, -22, 30);
  // map_eprint_repr((const Map *)&map);
  assert_fatal(map.len == 2);
  assert_fatal(map.cap == HASH_TABLE_INIT_CAP);
  assert_fatal(!map_has(&map, 0));
  assert_fatal(map_get(&map, -11) == 20);
  assert_fatal(map_get(&map, -22) == 30);
  assert_fatal(!map_has(&map, -33));

  map_set(&map, -22, 40);
  // map_eprint_repr((const Map *)&map);
  assert_fatal(map.len == 2);
  assert_fatal(map.cap == HASH_TABLE_INIT_CAP);
  assert_fatal(!map_has(&map, 0));
  assert_fatal(map_get(&map, -11) == 20);
  assert_fatal(map_get(&map, -22) == 40);
  assert_fatal(!map_has(&map, -33));

  map_set(&map, -33, 50);
  // map_eprint_repr((const Map *)&map);
  assert_fatal(map.len == 3);
  assert_fatal(map.cap == HASH_TABLE_INIT_CAP);
  assert_fatal(!map_has(&map, 0));
  assert_fatal(map_get(&map, -11) == 20);
  assert_fatal(map_get(&map, -22) == 40);
  assert_fatal(map_get(&map, -33) == 50);
  assert_fatal(!map_has(&map, -44));

  map_set(&map, -33, 60);
  // map_eprint_repr((const Map *)&map);
  assert_fatal(map.len == 3);
  assert_fatal(map.cap == HASH_TABLE_INIT_CAP * 2);
  assert_fatal(map_get(&map, -11) == 20);
  assert_fatal(map_get(&map, -22) == 40);
  assert_fatal(map_get(&map, -33) == 60);

  map_set(&map, -44, 70);
  // map_eprint_repr((const Map *)&map);
  assert_fatal(map.len == 4);
  assert_fatal(map.cap == HASH_TABLE_INIT_CAP * 2);
  assert_fatal(map_get(&map, -11) == 20);
  assert_fatal(map_get(&map, -22) == 40);
  assert_fatal(map_get(&map, -33) == 60);
  assert_fatal(map_get(&map, -44) == 70);

  map_set(&map, -44, 80);
  // map_eprint_repr((const Map *)&map);
  assert_fatal(map.len == 4);
  assert_fatal(map.cap == HASH_TABLE_INIT_CAP * 2);

  assert_fatal(map_get(&map, -11) == 20);
  assert_fatal(map_get(&map, -22) == 40);
  assert_fatal(map_get(&map, -33) == 60);
  assert_fatal(map_get(&map, -44) == 80);

  map_deinit(&map);

  assert_fatal(!map.keys);
  assert_fatal(!map.vals);
  assert_fatal(!map.bits);
  assert_fatal(!map.len);
  assert_fatal(!map.cap);
}
*/
