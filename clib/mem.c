/*
Memory-related stuff.

We tend to express memory sizes with `Ind` which is `U32` on 64-bit systems.
This limits allocation size to 4 GiB which currently seems to be sufficient.
This is mainly because we express memory offsets with `Ind`; using the same
type avoids widening conversions, simplifying arithmetic. See `Ind` for the
explanation of using a shorter-than-pointer integer type for memory offsets.
*/
#pragma once
#include "./err.c"
#include "./mem.h" // IWYU pragma: export
#include "./misc.h"
#include "./num.c"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

/*
Usage example:

  deferred(mem_deinit) void * some_val = some_func();
*/
static void mem_deinit(void **var) { var_deinit(var, free); }

/*
Usage example:

  deferred(chars_deinit) char * some_val = some_func();
*/
static void chars_deinit(char **var) { mem_deinit((void **)var); }

/*
Usage example:

  deferred(bytes_deinit) U8 * some_val = some_func();
*/
static void bytes_deinit(U8 **var) { mem_deinit((void **)var); }

[[noreturn]]
static void abort_mem_mul_over(Ind len, Ind size) {
  fprintf(
    stderr, "requested memory size overflow: " FMT_IND " * " FMT_IND, len, size
  );
  abort_traced();
}

// Overflow-safe counterpart of `len * size`.
static Ind mem_size(Ind len, Ind size) {
  Ind out;
  if (!__builtin_mul_overflow(len, size, &out)) return out;
  abort_mem_mul_over(len, size);
}

// Overflow-safe counterpart of `malloc(len * size)`.
static void *memalloc(Ind len, Ind size) { return malloc(mem_size(len, size)); }

static void *ptr_at(void *src, Ind ind, Ind size) {
  return (U8 *)src + mem_size(ind, size);
}

static Err err_mmap(void) {
  const auto code = errno;
  return errf(
    "unable to map memory; code: %d; message: %s", code, strerror(code)
  );
}

/*
Doesn't take pflags because we always use guards and `mprotect` inner parts.
The caller needs to handle the failure case:

  const auto ptr = mem_map(some_size, some_mflags);
  if (ptr == MAP_FAILED) return err_mmap();
*/
static void *mem_map(Ind len, int mflag) {
  // NOLINTBEGIN(hicpp-signed-bitwise)
  mflag |= MAP_ANON | MAP_PRIVATE;
  // NOLINTEND(hicpp-signed-bitwise)

  const auto fdes = -1;
  const auto off  = 0;
  return mmap(nullptr, len, PROT_NONE, mflag, fdes, off);
}

static Err mem_protect(void *addr, Ind len, int pflag) {
  return err_errno(mprotect(addr, len, pflag));
}

static void *ptr_align(void *val) {
  return __builtin_align_up(val, sizeof(val));
}

// Caller must reserve enough space. Assumes current system is little-endian.
static void encode_bigend_U32(U8 *ptr, U32 src) {
  ptr[0] = (U8)(src >> 24u);
  ptr[1] = (U8)(src >> 16u);
  ptr[2] = (U8)(src >> 8u);
  ptr[3] = (U8)src;
}

// Caller must reserve enough space. Assumes current system is little-endian.
static void encode_bigend_U64(U8 *ptr, U64 src) {
  ptr[0] = (U8)(src >> 56u);
  ptr[1] = (U8)(src >> 48u);
  ptr[2] = (U8)(src >> 40u);
  ptr[3] = (U8)(src >> 32u);
  ptr[4] = (U8)(src >> 24u);
  ptr[5] = (U8)(src >> 16u);
  ptr[6] = (U8)(src >> 8u);
  ptr[7] = (U8)src;
}

/*
Usage:

  deferred(buf_deinit) Buf buf = {};
  // ...

Value must be zero-initialized via `{}`.

The procedure is idempotent and may be called explicitly
before the deferred invocation, any amount of times.
*/
static void buf_deinit(Buf *buf) {
  if (!buf) return;
  free(buf->dat);
  buf->dat = nullptr;
  buf->len = 0;
  buf->cap = 0;
}

static U8 *buf_top(Buf *buf) { return buf->dat + buf->len; }

/*
Reserves at least this much extra capacity over the current capacity. Due to
pow2 rounding, capacity may be increased by more than the requested amount.
*/
static void buf_reserve(Buf *buf, Ind more) {
  const auto goal = add(buf->len, more);
  if (buf->cap >= goal) return;

  const Ind cap = round_up_pow2_Ind(goal);
  aver(cap);
  aver(cap >= goal);

  const auto dat = realloc(buf->dat, cap);
  aver(dat);

  buf->dat = dat;
  buf->cap = cap;
}

static void buf_append_byte(Buf *buf, U8 val) {
  buf_reserve(buf, 1);
  buf->dat[buf->len++] = val;
}

static void buf_append_bytes(Buf *buf, const U8 *src, Ind len) {
  buf_reserve(buf, len);
  memcpy(buf_top(buf), src, len);
  buf->len += len;
}

/*
Increases length by exactly this many zeroed bytes, adding capacity as needed.
Can be combined with `sizeof` to reserve space for values of arbitrary types,
which can be patched later with `buf_load` and `buf_store` by storing offset.
Avoid reinterpreting pointers due to UB potential.
*/
static void buf_zeropad(Buf *buf, Ind len) {
  if (!len) return;
  buf_reserve(buf, len);
  memset(buf_top(buf), 0, len);
  buf->len += len;
}

// Ensures at least this much TOTAL length, padding with zeros if needed.
static void buf_zeropad_to(Buf *buf, Ind len) {
  if (buf->len >= len) return;
  buf_zeropad(buf, len - buf->len);
}

// Assumes current system is little-endian.
static void buf_append_bigend_U32(Buf *buf, U32 src) {
  buf_reserve(buf, sizeof(src));
  encode_bigend_U32(buf_top(buf), src);
  buf->len += sizeof(src);
}

// Assumes current system is little-endian.
static void buf_append_bigend_U64(Buf *buf, U64 src) {
  buf_reserve(buf, sizeof(src));
  encode_bigend_U64(buf_top(buf), src);
  buf->len += sizeof(src);
}

static Err err_mpage_not_aligned(const void *addr) {
  return errf(
    "unable to deinit mpage with address %p: not aligned to page size" FMT_IND,
    addr,
    MEM_PAGE
  );
}

static Err err_mpage_no_magic(const void *addr) {
  return errf(
    "unable to deinit mpage with address %p: missing magic header", addr
  );
}

/*
Metadata for allocations via `mpage_init`; used by `mpage_deinit`.
Stored like this:

  [Mpage | guard | data | guard]
  ^ mmap           ^ mprotect
  ^ munmap

Guard size is `MEM_PAGE` so we don't bother storing it.
We could use `getpagesize` but it only adds complexity.
*/
typedef struct {
  U64 head[2]; // MPAGE_MAGIC
  Ind size;
} Mpage;

static constexpr U64 MPAGE_MAGIC[] = {
  0x0123456789'ABCDE'Full, // 🍔
  0xFED'CBA'9876543210ull, // 🐖🦥
};

/*
Usage:

  deferred(mpage_deinit) void* page = nullptr;
  try(mpage_init(&page, size));

Deinit is idempotent and may be invoked explicitly
before the deferred invocation, any amount of times.
*/
static Err mpage_deinit(void **page) {
  if (!page) return nullptr;

  const auto addr = *page;
  if (!addr) {
    *page = nullptr;
    return nullptr;
  }

  if (!__builtin_is_aligned(addr, MEM_PAGE)) {
    return err_mpage_not_aligned(addr);
  }

  const auto base = (Mpage *)((U8 *)addr - MEM_PAGE - MEM_PAGE);

  if (base->head[0] != MPAGE_MAGIC[0] || base->head[1] != MPAGE_MAGIC[1]) {
    return err_mpage_no_magic(addr);
  }

  const auto err = err_errno(munmap(base, base->size));
  *page          = nullptr;
  return err;
}

/*
Simple custom allocation with partial protection against underflow and overflow.
We sandwich the data area between guards which trigger pagefaults when accessed.
Due to OS / CPU limitations, we align the size of the data region to `MEM_PAGE`.
If the requested capacity is not pre-aligned, the extra memory is not protected.

Also see `./stack.c` and `./stack.h` for a more specialized analogue,
intended for generic stacks of values of any one type.
*/
static Err mpage_init(void **page, Ind cap) {
  const auto data_size = __builtin_align_up(cap, MEM_PAGE);
  const auto full_size = MEM_PAGE + MEM_PAGE + data_size + MEM_PAGE;
  const auto base      = mem_map(full_size, 0);

  if (base == MAP_FAILED) return err_mmap();

  const auto meta = (Mpage *)base;
  const auto data = (void *)((U8 *)base + MEM_PAGE + MEM_PAGE);

  try(mem_protect(meta, MEM_PAGE, PROT_READ | PROT_WRITE));
  try(mem_protect(data, data_size, PROT_READ | PROT_WRITE));

  *meta = (Mpage){
    .head[0] = MPAGE_MAGIC[0],
    .head[1] = MPAGE_MAGIC[1],
    .size    = full_size,
  };

  try(mem_protect(meta, MEM_PAGE, PROT_READ));
  *page = data;
  return nullptr;
}

/*
#include "./misc.h"

int main(int argc, const char *argv[]) {
  deferred(chars_deinit) char *empty = nullptr;
  deferred(chars_deinit) char *val   = strdup(argv[argc - 1]);
  puts(val);
}
*/

/*
int main(void) {
  deferred(buf_deinit) Buf buf = {};

  buf_append(&buf, 1234);
  printf("buf.len: " FMT_IND "\n", buf.len);

  buf_append(&buf, 2345);
  printf("buf.len: " FMT_IND "\n", buf.len);
}
*/

/*
int main(void) {
  deferred(mpage_deinit) void *page;
  try_main(mpage_init(&page, 0x10000));

  static constexpr char msg[] = "test";
  const auto            buf   = (U8 *)page;

  memcpy(buf, msg, sizeof(msg));

  printf("page: %p\n", page);
  printf("buf:  %s\n", buf);

  // printf("buf[-1]:      \n", buf[-1]);      // fault
  // printf("buf[0x10000]: \n", buf[0x10000]); // fault
}
*/
