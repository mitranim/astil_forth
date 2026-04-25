#pragma once
#include "./err.h" // IWYU pragma: export
#include "./fmt.h"
#include <errno.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void backtrace_capture() {
  BT_BUF_LEN = backtrace(BT_BUF, arr_cap(BT_BUF));
}

static void backtrace_print() {
  const auto len = BT_BUF_LEN;
  if (!len) return;

  backtrace_symbols_fd(BT_BUF, len, STDERR_FILENO);

  constexpr int cap = arr_cap(BT_BUF);
  if (!DEBUG || len < cap) return;

  eprintf("[debug] backtrace may have been truncated (capacity = %d)\n", cap);
}

[[noreturn]]
static void abort_traced(void) {
  backtrace_capture();
  backtrace_print();
  abort();
}

[[noreturn]]
static void aver_fatal(const char *expr, const char *file, int line) {
  fprintf(stderr, "assertion failed: (%s); %s:%d\n", expr, file, line);
  abort_traced();
}

[[noreturn]]
static void fatal_expr(
  const char *msg, const char *expr, const char *file, int line
) {
  fprintf(
    stderr,
    "error in expression: %s\n"
    "error message: %s\n"
    "%s:%d\n",
    expr,
    msg,
    file,
    line
  );
  abort_traced();
}

[[noreturn]]
static void fatal(const char *msg, const char *file, int line) {
  fprintf(
    stderr,
    "error: %s\n"
    "%s:%d\n",
    msg,
    file,
    line
  );
  abort_traced();
}

static inline void discard_err(Err val) { (void)val; }

static Err err_from_errno(
  int err, const char *expr, const char *fun, const char *file, int line
) {
  return errf(
    "error in expression %s\n"
    "code: %d; message: %s\n"
    "function: %s; file: %s; line: %d\n",
    expr,
    err,
    strerror(err),
    fun,
    file,
    line
  );
}
