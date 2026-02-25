#pragma once
#include "./err.c" // IWYU pragma: export
#include "./fmt.h"
#include "./num.h"
#include "./str.h" // IWYU pragma: export
#include <string.h>

static Err err_str_buf_over(const char *src, Uint len, Ind cap) {
  return errf(
    "string " FMT_QUOTED " overlows buffer capacity; len = " FMT_UINT
    ", cap = " FMT_IND,
    src,
    len,
    cap
  );
}

static Err str_copy_impl(char *out, const char *src, Ind cap, Ind *out_len) {
  const auto len = strlcpy(out, src, cap);
  if (len >= cap) return err_str_buf_over(src, len, cap);
  *out_len = (Ind)len;
  return nullptr;
}

static char *str_alloc_copy(const char *src, Ind src_len) {
  if (!src || !src_len) return nullptr;
  const auto buf_len = src_len + 1;
  const auto out     = malloc(buf_len);
  const auto out_len = strlcpy(out, src, buf_len);
  aver(out_len == src_len);
  return out;
}

static Err err_word_len_mismatch(Word_str *word, Ind len) {
  return errf(
    "length mismatch for " FMT_QUOTED ": " FMT_IND " vs " FMT_IND,
    word->buf,
    word->len,
    len
  );
}

static Err valid_word(const char *buf, Ind len, Word_str *word) {
  try(str_copy(word, buf));
  if (word->len == len) return nullptr;
  return err_word_len_mismatch(word, len);
}

static bool is_char_alnum(char val) {
  return (
    (val >= '0' && val <= '9') || (val >= 'A' && val <= 'Z') ||
    (val >= 'a' && val <= 'z')
  );
}

static const char *str_without_prefix(const char *str, const char *pre) {
  const auto len = strlen(pre);
  if (strncmp(str, pre, len)) return nullptr;
  return str + len;
}
