/*
Some tools for dealing with C strings in general,
and with our `str_buf` buffers in particular.
*/
#pragma once
#include "./str.h" // IWYU pragma: export
#include "./err.c" // IWYU pragma: export
#include "./fmt.h"
#include "./num.h"
#include <string.h>

static Err err_str_buf_over(const char *src, Uint len, Ind cap) {
  return errf(
    "string (length " FMT_UINT ") exceeds target buffer capacity " FMT_IND
    "; source: %s",
    len,
    cap,
    src
  );
}

/*
The name is a bit ambiguous with `str_set`, which is
a questionable tool anyway. TODO disambiguate better.
*/
static Err cstr_set(char *out_buf, Ind out_cap, const char *src, Ind src_len) {
  aver(out_cap);
  if (src_len > out_cap) return err_str_buf_over(src, src_len, out_cap);
  memcpy(out_buf, src, src_len);
  out_buf[src_len] = '\0';
  return nullptr;
}

static Err str_set_impl(char *out_buf, Ind *out_len, const char *src, Ind cap) {
  aver(cap);
  const auto len = strlcpy(out_buf, src, cap);

  if (len < cap) {
    *out_len = (Ind)len;
    return nullptr;
  }

  *out_len = cap - 1;
  return err_str_buf_over(src, len, cap);
}

static bool str_eq_impl(const char *buf, Ind len, const char *tar) {
  Ind ind = 0;
  while (ind < len && tar[ind] == buf[ind]) ind++;
  return ind == len && tar[ind] == '\0';
}

static char *str_alloc_copy(const char *src, Ind len) {
  if (!src || !len) return nullptr;

  char *out = malloc(len + 1);
  if (!out) return nullptr;

  memcpy(out, src, len);
  out[len] = '\0';
  return out;
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

static bool streq(const char *one, const char *two) {
  return (!one && !two) || (one && two && !strcmp(one, two));
}
