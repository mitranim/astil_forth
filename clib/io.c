#pragma once
#include "./err.c"
#include "./fmt.c"
#include "./misc.h"
#include "./num.h"
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

/*
Usage:

  deferred(fd_deinit) int file = -1;

The variable MUST be initialized to -1.

Alternatively, use `defer`:

  int file;
  try(fd_open(path, mode, &file));
  defer close(file);
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

static Err fd_read_all(const char *path, int file, U8 *buf, Uint len) {
  Uint off = 0;

  while (off < len) {
    const auto read_len = read(file, buf + off, len - off);

    if (read_len > 0) {
      off += (Uint)read_len;
      continue;
    }

    if (read_len == 0) return err_file_read_mismatch(path, len, off);

    if (errno == EINTR) continue;
    return err_file_unable_to_read_errno(path);
  }

  return nullptr;
}

/*
Usage:

  deferred(file_deinit) FILE *file = nullptr;

The variable MUST be zero-initialized.

Alternatively, use `defer`:

  FILE *file;
  try(file_open(path, mode, &file));
  defer fclose(file);
*/
static void file_deinit(FILE **var) { var_deinit(var, fclose); }

static Err file_open(const char *path, const char *mode, FILE **out) {
  const auto file = fopen(path, mode);
  if (!file) return err_file_unable_to_open(path);
  *out = file;
  return nullptr;
}

static Err err_file_not_regular(const char *path) {
  return errf("unable to read %s: not a regular file", path);
}

static Err file_read(const char *path, U8 **out_body, Uint *out_len) {
  deferred(fd_deinit) int file = -1;
  try(fd_open(path, O_RDONLY, &file));

  struct stat info;
  try(fd_stat(path, file, &info));

  if (!S_ISREG(info.st_mode)) return err_file_not_regular(path);
  aver(info.st_size >= 0);

  const auto file_len = (Uint)info.st_size;
  const auto buf_len  = add(file_len, 1);
  U8        *buf      = malloc(buf_len);
  const auto err      = fd_read_all(path, file, buf, file_len);

  if (err) {
    free(buf);
    return err;
  }

  buf[file_len] = '\0';
  *out_body     = buf;
  *out_len      = file_len;
  return nullptr;
}

static Err file_read_text(const char *path, char **out_body, Uint *out_len) {
  return file_read(path, (U8 **)out_body, out_len);
}

static Err err_file_unable_to_read_stream(const char *path) {
  const auto code = errno;
  return errf(
    "unable to read stream %s; code: %d; message: %s", path, code, strerror(code)
  );
}

static Err stream_append(
  const char *path, char **out_buf, Uint *out_len, const char *src, Uint src_len
) {
  const auto len = *out_len;
  const auto cap = len + src_len + 1;
  char      *buf = realloc(*out_buf, cap);

  if (!buf) {
    return errf("unable to allocate " FMT_UINT " bytes for %s", cap, path);
  }

  memcpy(buf + len, src, src_len);
  buf[len + src_len] = '\0';
  *out_buf           = buf;
  *out_len           = len + src_len;
  return nullptr;
}

static Err file_stream_read_text(
  const char *path, FILE *file, char **out_body, Uint *out_len
) {
  deferred(chars_deinit) char *body = nullptr;
  Uint                         len  = 0;
  char                         buf[4096];

  for (;;) {
    const auto read_len = fread(buf, 1, sizeof(buf), file);
    if (read_len) try(stream_append(path, &body, &len, buf, read_len));
    if (read_len == sizeof(buf)) continue;
    if (ferror(file)) return err_file_unable_to_read_stream(path);
    break;
  }

  if (!body) {
    body = calloc(1, 1);
    if (!body) return errf("unable to allocate 1 byte for %s", path);
  }

  *out_body = body;
  *out_len  = len;
  body      = nullptr;
  return nullptr;
}

static Err fd_read_available_text(
  const char *path, int file, char **out_body, Uint *out_len
) {
  deferred(chars_deinit) char *body = nullptr;
  Uint                         len  = 0;
  char                         buf[4096];

  for (;;) {
    const auto read_len = read(file, buf, sizeof(buf));
    if (!read_len) break;

    if (read_len < 0) {
      if (errno == EINTR) continue;
      return err_file_unable_to_read_stream(path);
    }

    try(stream_append(path, &body, &len, buf, (Uint)read_len));

    struct pollfd poll_fd = {.fd = file, .events = POLLIN};
    int           ready;

    do ready = poll(&poll_fd, 1, 0);
    while (ready < 0 && errno == EINTR);

    if (ready < 0) return err_file_unable_to_read_stream(path);

    const auto events = (Uint)(U16)poll_fd.revents;

    if (events & (Uint)(POLLERR | POLLNVAL)) {
      return err_file_unable_to_read_stream(path);
    }
    if (events & (Uint)POLLIN) continue;
    break;
  }

  if (!body) {
    body = calloc(1, 1);
    if (!body) return errf("unable to allocate 1 byte for %s", path);
  }

  *out_body = body;
  *out_len  = len;
  body      = nullptr;
  return nullptr;
}

static Err err_file_write(Uint exp, Uint act) {
  return errf(
    "stream write error: " FMT_UINT " objects written instead of " FMT_UINT,
    act,
    exp
  );
}

static Err file_write(
  FILE *restrict file, const void *restrict vals, Uint val_size, Uint vals_len
) {
  const auto out = fwrite(vals, val_size, vals_len, file);
  if (out == vals_len) return nullptr;
  return err_file_write(vals_len, out);
}

static Err write_all(int file, const U8 *buf, Uint len, int *out_err) {
  while (len) {
    auto wrote = write(file, buf, len);

    if (wrote > 0) {
      const auto val = (Uint)wrote;
      aver(val <= len);
      buf += val;
      len -= val;
      continue;
    }

    const auto err = errno;
    if (err == EINTR) continue;
    if (err == EPIPE) return nullptr; // File closed early.

    if (out_err) *out_err = err;
    const auto msg = strerror(err);

    if (!msg) {
      return errf("unable to fully write to file %d; code: %d", file, err);
    }
    return errf(
      "unable to fully write to file %d; code: %d; msg: %s", file, err, msg
    );
  }
  return nullptr;
}

static void file_purge(FILE *file) {
  if (!file) return;
  fpurge(file);
  int fdes = fileno(file);
  if (isatty(fdes)) tcflush(fdes, TCIFLUSH);
  clearerr(file);
}

/*
#include "./mem.c"

int main() {
  {
    deferred(file_deinit) FILE *file = nullptr;
    file                          = stdin; // OK to "close"
  }

  {
    deferred(chars_deinit) char *body = nullptr;
    Uint                         len;

    try_main(file_read_text("./io.c", &body, &len));

    printf("read " FMT_UINT " bytes\n", len);
    puts(body);
  }
}
*/
