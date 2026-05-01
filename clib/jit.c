#pragma once
#include "./err.c"
#include "./mem.c"
#include "./num.h"
#include <sys/mman.h>

#ifdef __APPLE__

#include "./err.h"
#include <libkern/OSCacheControl.h>
#include <pthread.h>

static Err mem_map_jit(Ind len, void **out) {
  const auto ptr = mem_map(len, MAP_JIT);
  if (ptr == MAP_FAILED) return err_mmap();
  *out = ptr;
  return nullptr;
}

// See the comment below.
//
// NOLINTBEGIN(clang-analyzer-security.MmapWriteExec)

/*
Should be used when initializing a JIT code heap, which must be memory-mapped
with the `MAP_JIT` flag. The last `pthread_jit_write_protect_np` determines
whether the region is executable or writable.

TODO clearer API.
*/
static Err mprotect_jit(void *addr, Ind len) {
  return err_errno(mprotect(addr, len, PROT_READ | PROT_WRITE | PROT_EXEC));
}

// NOLINTEND(clang-analyzer-security.MmapWriteExec)

// TODO consider using `pthread_jit_write_with_callback_np` instead.
static Err jit_before_write(void *addr, Ind len) {
  (void)addr;
  (void)len;
  pthread_jit_write_protect_np(false);
  return nullptr;
}

static Err jit_after_write(void *addr, Ind len) {
  pthread_jit_write_protect_np(true);
  sys_icache_invalidate(addr, len);
  return nullptr;
}

#else // __APPLE__

/*
The stuff below also seems to work on MacOS Apple Silicon,
so the `__APPLE__` case above might be unnecessary.
*/

#include "./err.c"
#include <sys/types.h>

static Err mem_map_jit(Ind len, void **out) {
  const auto ptr = mem_map(len, 0);
  if (ptr == MAP_FAILED) return err_mmap();
  *out = ptr;
  return nullptr;
}

static Err mprotect_jit(void *addr, Ind len) {
  return err_errno(mprotect(addr, len, PROT_READ | PROT_EXEC));
}

static Err jit_before_write(void *addr, Ind len) {
  try_errno(mprotect(addr, len, PROT_READ | PROT_WRITE));
  return nullptr;
}

static Err jit_after_write(void *addr, Ind len) {
  try(mprotect_jit(addr, len));
  __builtin___clear_cache(addr, (void *)((U8 *)addr + len));
  return nullptr;
}

#endif // __APPLE__
