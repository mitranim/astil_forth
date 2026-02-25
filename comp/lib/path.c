#pragma once
#include "./err.c"
#include "./fmt.h"
#include "./num.h"
#include "./str.c"
#include <string.h>

// Matches a protocol like `file:` or a volume like `C:`.
static bool path_has_volume(const char *path) {
  if (!path) return false;
  if (!is_char_alnum(path[0])) return false;

  char byte;
  while ((byte = *++path)) {
    if (byte == ':') return true;
    if (!is_char_alnum(byte)) return false;
  }
  return false;
}

static bool is_path_abs(const char *path) {
  return !!path && (path[0] == '/' || path_has_volume(path));
}

/*
Strictly relative paths look like this: `file.ext` or `dir/file.ext`.
They don't begin with any of:
- `vol:`
- `/`
- `./`
- `../`
*/
static bool is_path_strict_rel(const char *path) {
  if (str_has_prefix(path, "/")) return false;
  if (str_has_prefix(path, "./")) return false;
  if (str_has_prefix(path, "../")) return false;
  if (path_has_volume(path)) return false;
  return true;
}

/*
Lazy-ass code, doesn't try to "clean" the path by collapsing
redundant segments. The output should be passed to `realpath`
which canonicalizes the path for us, then freed.
*/
static char *path_join(const char *base, const char *suf, bool is_dir) {
  if (!suf) return base ? strdup(base) : nullptr;
  if (!base) return strdup(suf);
  if (is_path_abs(suf)) return strdup(suf);

  const auto inf     = is_dir ? "/" : "/../";
  const auto inf_len = is_dir ? arr_cap("/") : arr_cap("/../");
  const auto buf_len = (Ind)(strlen(base) + inf_len + strlen(suf));
  const auto buf_cap = buf_len + 1;
  const auto buf     = (char *)calloc(buf_cap, sizeof(char));

  auto rem = buf_len;
  auto ptr = buf;

  auto len = strlcpy(ptr, base, rem);
  ptr += len;
  aver(rem >= len);
  rem -= len;

  len = strlcpy(ptr, inf, rem);
  ptr += len;
  aver(rem >= len);
  rem -= len;

  len = strlcpy(ptr, suf, rem);
  ptr += len;
  aver(rem >= len);
  rem -= len;

  if (!rem) {
    eprintf(
      "internal error: out of buffer space when joining paths " FMT_QUOTED
      " and " FMT_QUOTED "\n",
      base,
      suf
    );
    abort();
  }

  return buf;
}

static bool is_path_stdin(const char *path) {
  return !strcmp(path, "-") || !strcmp(path, "/dev/stdin");
}

// Normalizes the path so the file can be opened,
// and returns a statically allocated string.
static const char *file_path_stdio(const char *path) {
  if (is_path_stdin(path)) {
    return "/dev/stdin";
  }
  if (!strcmp(path, "/dev/stdout")) {
    return "/dev/stdout";
  }
  if (!strcmp(path, "/dev/stderr")) {
    return "/dev/stderr";
  }
  return nullptr;
}
