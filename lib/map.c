#pragma once
#include "./hash_table_common.c"
#include "./map.h" // IWYU pragma: export

static void map_deinit(void *map) { hash_table_deinit(map); }

static bool map_valid(const Hash_table *map) {
  return hash_table_valid((const Hash_table *)map);
}

static Ind map_ind_impl(const Map *map, const void *key, Uint key_size) {
  return hash_table_matching_ind(
    (const Hash_table *)map, key, key_size, mem_hash, mem_eq
  );
}

static bool map_has_impl(Map *map, const void *key, Uint key_size) {
  return hash_table_matching_ind(
    (const Hash_table *)map, key, key_size, mem_hash, mem_eq
  );
}

static void *map_set_impl(
  Map *map, const void *key, const void *val, Uint key_size, Uint val_size
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

int main(void) {
  defer(map_deinit) map_of(Sint, Uint) map = {};
  // map_eprint_repr((const Map *)&map);

  aver(map.len == 0);
  aver(map.cap == 0);

  aver(!map_has(&map, 0));
  aver(!map_has(&map, -11));
  aver(!map_has(&map, -22));

  aver(map_get(&map, 0) == 0);
  aver(map_get(&map, -11) == 0);
  aver(map_get(&map, -22) == 0);

  map_set(&map, -11, 10);
  // map_eprint_repr((const Map *)&map);
  aver(map.len == 1);
  aver(map.cap == HASH_TABLE_INIT_CAP);
  aver(!map_has(&map, 0));
  aver(map_has(&map, -11));
  aver(map_get(&map, -11) == 10);
  aver(!map_has(&map, -22));

  map_set(&map, -11, 20);
  // map_eprint_repr((const Map *)&map);
  aver(map.len == 1);
  aver(map.cap == HASH_TABLE_INIT_CAP);
  aver(!map_has(&map, 0));
  aver(map_get(&map, -11) == 20);
  aver(!map_has(&map, -22));

  map_set(&map, -22, 30);
  // map_eprint_repr((const Map *)&map);
  aver(map.len == 2);
  aver(map.cap == HASH_TABLE_INIT_CAP);
  aver(!map_has(&map, 0));
  aver(map_get(&map, -11) == 20);
  aver(map_get(&map, -22) == 30);
  aver(!map_has(&map, -33));

  map_set(&map, -22, 40);
  // map_eprint_repr((const Map *)&map);
  aver(map.len == 2);
  aver(map.cap == HASH_TABLE_INIT_CAP);
  aver(!map_has(&map, 0));
  aver(map_get(&map, -11) == 20);
  aver(map_get(&map, -22) == 40);
  aver(!map_has(&map, -33));

  map_set(&map, -33, 50);
  // map_eprint_repr((const Map *)&map);
  aver(map.len == 3);
  aver(map.cap == HASH_TABLE_INIT_CAP);
  aver(!map_has(&map, 0));
  aver(map_get(&map, -11) == 20);
  aver(map_get(&map, -22) == 40);
  aver(map_get(&map, -33) == 50);
  aver(!map_has(&map, -44));

  map_set(&map, -33, 60);
  // map_eprint_repr((const Map *)&map);
  aver(map.len == 3);
  aver(map.cap == HASH_TABLE_INIT_CAP * 2);
  aver(map_get(&map, -11) == 20);
  aver(map_get(&map, -22) == 40);
  aver(map_get(&map, -33) == 60);

  map_set(&map, -44, 70);
  // map_eprint_repr((const Map *)&map);
  aver(map.len == 4);
  aver(map.cap == HASH_TABLE_INIT_CAP * 2);
  aver(map_get(&map, -11) == 20);
  aver(map_get(&map, -22) == 40);
  aver(map_get(&map, -33) == 60);
  aver(map_get(&map, -44) == 70);

  map_set(&map, -44, 80);
  // map_eprint_repr((const Map *)&map);
  aver(map.len == 4);
  aver(map.cap == HASH_TABLE_INIT_CAP * 2);

  aver(map_get(&map, -11) == 20);
  aver(map_get(&map, -22) == 40);
  aver(map_get(&map, -33) == 60);
  aver(map_get(&map, -44) == 80);

  map_deinit(&map);

  aver(!map.keys);
  aver(!map.vals);
  aver(!map.bits);
  aver(!map.len);
  aver(!map.cap);
}
*/
