#pragma once
#include "./err.c" // IWYU pragma: export
#include "./fmt.c"
#include "./misc.h"
#include "./num.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/*
Usage:

  defer(fd_deinit) int file = -1;

The variable MUST be initialized to -1.
*/
static void fd_deinit(int *file) {
  if (!file) return;
  const auto val = *file;
  if (val < 0) return;
  close(val);
  *file = -1;
}

static Err err_file_unable_to_open(const char *path) {
  const auto msg = strerror(errno);

  if (!msg) {
    return errf("unable to open file " FMT_QUOTED, path, errno);
  }

  return errf(
    "unable to open file " FMT_QUOTED "; code: %d; msg: %s", path, errno, msg
  );
}

static Err fd_open(const char *path, int mode, int *out) {
  const auto val = open(path, mode);
  if (val < 0) return err_file_unable_to_open(path);
  *out = val;
  return nullptr;
}

static Err err_file_stat(const char *path) {
  const auto code = errno;
  return errf("unable to stat %s; code: %d; msg: %s", path, code, strerror(code));
}

static Err fd_stat(const char *path, int file, struct stat *out) {
  return fstat(file, out) ? err_file_stat(path) : nullptr;
}

static Err err_file_unable_to_read_errno(const char *path) {
  const auto code = errno;
  const auto msg  = strerror(code);
  return errf("unable to read %s; code: %d; message: %s", path, code, msg);
}

static Err err_file_read_mismatch(const char *path, Uint file_len, Uint read_len) {
  return errf(
    "mismatch when reading %s: expected to read " FMT_UINT
    " bytes, got " FMT_UINT " bytes\n",
    path,
    file_len,
    read_len
  );
}

/*
Usage:

  defer(file_deinit) FILE *file = nullptr;

The variable MUST be zero-initialized.
*/
static void file_deinit(FILE **var) { var_deinit(var, fclose); }

static Err file_open_fuzzy(const char *path, const char **resolved, FILE **out) {
  if (!strcmp(path, "-") || !strcmp(path, "/dev/stdin")) {
    if (resolved) *resolved = "/dev/stdin";
    *out = stdin;
    return nullptr;
  }

  if (!strcmp(path, "/dev/stdout")) {
    if (resolved) *resolved = "/dev/stdout";
    *out = stdout;
    return nullptr;
  }

  if (!strcmp(path, "/dev/stderr")) {
    if (resolved) *resolved = "/dev/stderr";
    *out = stderr;
    return nullptr;
  }

  const auto file = fopen(path, "r");
  if (file) {
    if (resolved) *resolved = path;
    *out = file;
    return nullptr;
  }

  return err_file_unable_to_open(path);
}

static Err file_open(const char *path, FILE **out) {
  const auto file = fopen(path, "r");
  if (!file) return err_file_unable_to_open(path);
  *out = file;
  return nullptr;
}

static Err file_read(const char *path, U8 **out_body, Uint *out_len) {
  defer(fd_deinit) int file = -1;
  try(fd_open(path, O_RDONLY, &file));

  struct stat info;
  try(fd_stat(path, file, &info));

  const auto file_len = (Uint)info.st_size;
  const auto buf_len  = file_len + 1;
  U8        *buf      = malloc(buf_len);
  const auto read_len = read(file, buf, file_len);

  if (read_len < 0) return err_file_unable_to_read_errno(path);

  if ((Uint)read_len != file_len) {
    return err_file_read_mismatch(path, file_len, (Uint)read_len);
  }

  buf[file_len] = '\0';
  *out_body     = buf;
  *out_len      = buf_len;
  return nullptr;
}

static Err file_read_text(const char *path, char **out_body, Uint *out_len) {
  return file_read(path, (U8 **)out_body, out_len);
}

static Err err_file_write(Uint exp, Uint act) {
  return errf(
    "stream write error: " FMT_UINT " objects written instead of " FMT_UINT,
    act,
    exp
  );
}

static Err file_stream_write(
  FILE *restrict file, const void *restrict vals, Uint val_size, Uint vals_len
) {
  const auto out = fwrite(vals, val_size, vals_len, file);
  if (out == vals_len) return nullptr;
  return err_file_write(vals_len, out);
}

// ultra lazy ass code
static char *path_join(const char *one, const char *two) {
  static constexpr Uint    LEN = 4096;
  static thread_local char BUF[LEN];

  Uint rem = LEN;
  auto ptr = BUF;

  auto len = strlcpy(ptr, one, rem);
  ptr += len;
  aver(rem >= len);
  rem -= len;

  len = strlcpy(ptr, "/../", rem);
  ptr += len;
  aver(rem >= len);
  rem -= len;

  len = strlcpy(ptr, two, rem);
  ptr += len;
  aver(rem >= len);
  rem -= len;

  if (!rem) {
    eprintf(
      "out of buffer space when joining paths " FMT_QUOTED " and " FMT_QUOTED
      "\n",
      one,
      two
    );
    abort();
  }

  return BUF;
}

/*
#include "./mem.c"

int main(void) {
  {
    defer(file_deinit) FILE *file = nullptr;
    file                          = stdin; // OK to "close"
  }

  {
    defer(str_deinit) char *body = nullptr;
    Uint                    len;

    try_main(file_read_text("./io.c", &body, &len));

    printf("read " FMT_UINT " bytes\n", len);
    puts(body);
  }
}
*/

/*
int main(void) {
  printf("%s\n", path_join("io.c", "blah.c"));
  printf("%s\n", realpath(path_join("io.c", "num.h"), nullptr));
  printf(
    "%s\n", realpath("/Users/m/code/m/forth/f_test.f/.././f_lang_s.f", nullptr)
  );
}
*/
