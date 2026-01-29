#pragma once
#include "./dict.h" // IWYU pragma: export
#include "./hash.c"
#include "./hash_table_common.c"
#include "./num.h"
#include <string.h>

// Used only for pointer arithmetic; ignored by hashing and equality.
static constexpr Uint DICT_KEY_SIZE = sizeof(char *);

// For dicts where keys are borrowed.
static void dict_deinit(void *dict) { hash_table_deinit(dict); }

// For dicts where keys are owned.
static void dict_deinit_with_keys(Dict *dict) {
  for (dict_range(Ind, ind, dict)) free(dict->keys[ind]);
  dict_deinit(dict);
}

static bool dict_valid(const Dict *dict) {
  return hash_table_valid((const Hash_table *)dict);
}

// For dicts where keys are borrowed.
static void dict_trunc(Dict *dict) { hash_table_trunc((Hash_table *)dict); }

// For dicts where keys are owned.
static void dict_trunc_with_keys(Dict *dict) {
  for (dict_range(Ind, ind, dict)) free(dict->keys[ind]);
  hash_table_trunc((Hash_table *)dict);
}

// Conforms to `Hash_fun`.
static Hash dict_key_hash(const void *key, Uint len) {
  (void)len;
  return hash_str(*(const char *const *)key);
}

// Conforms to `Eq_fun`.
static bool dict_key_eq(const void *one, const void *two, Uint len) {
  (void)len;
  return !strcmp(*(const char *const *)one, *(const char *const *)two);
}

static Ind dict_ind_impl(const Dict *dict, const char *key) {
  return hash_table_matching_ind(
    (const Hash_table *)dict, &key, DICT_KEY_SIZE, dict_key_hash, dict_key_eq
  );
}

static void *dict_set_impl(
  Dict *dict, const char *key, const void *val, Uint val_size
) {
  return hash_table_set(
    (Hash_table *)dict, &key, val, DICT_KEY_SIZE, val_size, dict_key_hash, dict_key_eq
  );
}

static void dict_eprint_repr(const Dict *dict) {
  hash_table_eprint_repr((const Hash_table *)dict);
}

/*
#include "./err.h"
#include <limits.h>
#include <stdio.h>

int main(void) {
  defer(dict_deinit) dict_of(Sint) dict = {};
  // dict_eprint_repr((Dict *)&dict);

  aver(dict.len == 0);
  aver(dict.cap == 0);

  aver(!dict_has(&dict, ""));
  aver(!dict_has(&dict, "one"));
  aver(!dict_has(&dict, "two"));

  aver(dict_get(&dict, "") == 0);
  aver(dict_get(&dict, "one") == 0);
  aver(dict_get(&dict, "two") == 0);

  dict_set(&dict, "one", 10);
  // dict_eprint_repr((Dict *)&dict);
  aver(dict.len == 1);
  aver(dict.cap == HASH_TABLE_INIT_CAP);
  aver(!dict_has(&dict, ""));
  aver(dict_has(&dict, "one"));
  aver(dict_get(&dict, "one") == 10);
  aver(!dict_has(&dict, "two"));

  dict_set(&dict, "one", 20);
  // dict_eprint_repr((Dict *)&dict);
  aver(dict.len == 1);
  aver(dict.cap == HASH_TABLE_INIT_CAP);
  aver(!dict_has(&dict, ""));
  aver(dict_get(&dict, "one") == 20);
  aver(!dict_has(&dict, "two"));

  dict_set(&dict, "two", 30);
  // dict_eprint_repr((Dict *)&dict);
  aver(dict.len == 2);
  aver(dict.cap == HASH_TABLE_INIT_CAP);
  aver(!dict_has(&dict, ""));
  aver(dict_get(&dict, "one") == 20);
  aver(dict_get(&dict, "two") == 30);
  aver(!dict_has(&dict, "three"));

  dict_set(&dict, "two", 40);
  // dict_eprint_repr((Dict *)&dict);
  aver(dict.len == 2);
  aver(dict.cap == HASH_TABLE_INIT_CAP);
  aver(!dict_has(&dict, ""));
  aver(dict_get(&dict, "one") == 20);
  aver(dict_get(&dict, "two") == 40);
  aver(!dict_has(&dict, "three"));

  dict_set(&dict, "three", 50);
  // dict_eprint_repr((Dict *)&dict);
  aver(dict.len == 3);
  aver(dict.cap == HASH_TABLE_INIT_CAP);
  aver(!dict_has(&dict, ""));
  aver(dict_get(&dict, "one") == 20);
  aver(dict_get(&dict, "two") == 40);
  aver(dict_get(&dict, "three") == 50);
  aver(!dict_has(&dict, "four"));

  dict_set(&dict, "three", 60);
  // dict_eprint_repr((Dict *)&dict);
  aver(dict.len == 3);
  aver(dict.cap == HASH_TABLE_INIT_CAP * 2);
  aver(dict_get(&dict, "one") == 20);
  aver(dict_get(&dict, "two") == 40);
  aver(dict_get(&dict, "three") == 60);

  dict_set(&dict, "four", 70);
  // dict_eprint_repr((Dict *)&dict);
  aver(dict.len == 4);
  aver(dict.cap == HASH_TABLE_INIT_CAP * 2);
  aver(dict_get(&dict, "one") == 20);
  aver(dict_get(&dict, "two") == 40);
  aver(dict_get(&dict, "three") == 60);
  aver(dict_get(&dict, "four") == 70);

  dict_set(&dict, "four", 80);
  // dict_eprint_repr((Dict *)&dict);
  aver(dict.len == 4);
  aver(dict.cap == HASH_TABLE_INIT_CAP * 2);

  aver(dict_get(&dict, "one") == 20);
  aver(dict_get(&dict, "two") == 40);
  aver(dict_get(&dict, "three") == 60);
  aver(dict_get(&dict, "four") == 80);

  aver(dict_get(&dict, strdup("")) == 0);
  aver(dict_get(&dict, strdup("one")) == 20);
  aver(dict_get(&dict, strdup("two")) == 40);
  aver(dict_get(&dict, strdup("three")) == 60);
  aver(dict_get(&dict, strdup("four")) == 80);

  dict_deinit(&dict);

  aver(!dict.keys);
  aver(!dict.vals);
  aver(!dict.bits);
  aver(!dict.len);
  aver(!dict.cap);
}
*/
