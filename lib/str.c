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
