#pragma once
#include "./err.c"
#include "./fmt.c"

static Err env_bool(const char *key, bool *out) {
  const auto val = getenv(key);
  if (!val) return nullptr; // Allow defaults.
  return bool_decode(val, out);
}

static bool is_cli_flag(const char *key) {
  return !!key && key[0] == '-' && !!key[1]; // `-` means stdio
}

static Err err_cli_key_over(const char *src, Ind len, Ind cap) {
  return errf(
    "unable to parse CLI arg " FMT_QUOTED ": key length " FMT_IND
    " exceeds maximum length " FMT_IND,
    src,
    len,
    cap - 1
  );
}

/*
Parses a CLI arg, which may be any of:

  val
  --key
  --key=
  --key=val

Space-separated key-val pairs like `--key val` are not supported
due to the ambiguities they create, particularly for booleans.
Combined `-abc` boolean flags are not supported either.
An input like `one=two` is considered to be a value.

The returned key, if any, includes all the leading `-`.
For `--key=val` inputs, the returned key is valid only
until the next call to `cli_key_val`.
*/
static Err cli_key_val(const char *src, const char **key, const char **val) {
  *key = nullptr;
  *val = nullptr;

  if (!src || !src[0]) return nullptr;

  if (!is_cli_flag(src)) {
    *val = src;
    return nullptr;
  }

  const auto sep = strchr(src, '=');

  if (!sep) {
    *key = src;
    return nullptr;
  }

  const auto key_len = sep - src;
  aver(key_len > 0);

  static char buf[256] = {};
  const auto  buf_cap  = arr_cap(buf);

  if ((Uint)key_len >= buf_cap) {
    return err_cli_key_over(src, (Ind)key_len, buf_cap);
  }

  const auto end = stpncpy(buf, src, key_len);
  aver(end > buf && end < (buf + buf_cap));

  *key = buf;
  *val = sep + 1;
  return nullptr;
}

/*
Note that unlike `cli_key_val`, this modifies the output `out_val`
only when a matching CLI flag is actually provided.
*/
static Err cli_bool_for(
  const char *name, const char *key, const char *val, bool *out_val, bool *ok
) {
  *ok = false;

  if (!key || strcmp(key, name)) return nullptr;

  if (!val || !val[0]) {
    *out_val = true;
    *ok      = true;
    return nullptr;
  }

  try(bool_decode(val, out_val));
  *ok = true;
  return nullptr;
}
