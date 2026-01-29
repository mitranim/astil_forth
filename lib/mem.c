#pragma once
#include "./err.c"
#include "./misc.h"
#include "./num.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

static void mem_deinit(void **var) { var_deinit(var, free); }

static void str_deinit(char **var) { mem_deinit((void **)var); }

static void bytes_deinit(U8 **var) { mem_deinit((void **)var); }

[[noreturn]]
static void abort_mem_mul_over(Uint len, Uint size) {
  fprintf(
    stderr, "requested memory size overflow: " FMT_UINT " * " FMT_UINT, len, size
  );
  abort_traced();
}

// Overflow-safe counterpart of `len * size`.
static Uint mem_size(Uint len, Uint size) {
  Uint out;
  if (!__builtin_mul_overflow(len, size, &out)) return out;
  abort_mem_mul_over(len, size);
}

// Overflow-safe counterpart of `malloc(len * size)`.
static void *memalloc(Uint len, Uint size) {
  return malloc(mem_size(len, size));
}

static void *ptr_at(void *src, Ind ind, Uint size) {
  return (U8 *)src + mem_size(ind, size);
}

static Err err_mmap() {
  const auto code = errno;
  return errf(
    "unable to map memory; code: %d; message: %s", code, strerror(code)
  );
}

// Doesn't take pflags because we always use guards and `mprotect` inner parts.
static void *mem_map(Uint len, int mflag) {
  mflag |= MAP_ANON | MAP_PRIVATE;
  const auto fd  = -1;
  const auto off = 0;
  return mmap(nullptr, len, PROT_NONE, mflag, fd, off);
}

static Err mprotect_mutable(void *addr, Uint len) {
  return err_errno(mprotect(addr, len, PROT_READ | PROT_WRITE));
}

/*
#include "./misc.h"

int main(int argc, const char **argv) {
  defer(str_deinit) char *empty = nullptr;
  defer(str_deinit) char *val   = strdup(argv[argc - 1]);
  puts(val);
}
*/
