#pragma once
#include "./err.c"
#include "./num.h"
#include <sys/mman.h>

#ifdef __APPLE__

#include "./err.h"
#include <libkern/OSCacheControl.h>
#include <pthread.h>

static Err mprotect_jit(void *addr, Uint len) {
  return err_errno(mprotect(addr, len, PROT_READ | PROT_WRITE | PROT_EXEC));
}

// TODO consider using `pthread_jit_write_with_callback_np` instead.
static Err jit_before_write(void *addr, Uint len) {
  (void)addr;
  (void)len;
  pthread_jit_write_protect_np(false);
  return nullptr;
}

static Err jit_after_write(void *addr, Uint len) {
  pthread_jit_write_protect_np(true);
  sys_icache_invalidate(addr, len);
  return nullptr;
}

#else

/*
The stuff below also seems to work on MacOS Apple Silicon,
so the above might be unnecessary.
*/

#include "./err.c"
#include <sys/types.h>

static Err mprotect_jit(void *addr, Uint len) {
  return err_errno(mprotect(addr, len, PROT_READ | PROT_EXEC));
}

static Err jit_before_write(void *addr, Uint len) {
  try_errno(mprotect(addr, len, PROT_READ | PROT_WRITE));
  return nullptr;
}

static Err jit_after_write(void *addr, Uint len) {
  try(mprotect_jit(addr, len));
  __builtin___clear_cache(addr, (void *)((U8 *)addr + len));
  return nullptr;
}

#endif
