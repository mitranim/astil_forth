#pragma once
#include "./err.c"
#include "./fmt.c"

static Err env_bool(const char *key, bool *out) {
  const auto val = getenv(key);
  if (!val) return nullptr; // Allow defaults.
  return decode_bool(val, out);
}
