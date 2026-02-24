#pragma once
#include "./err.c" // IWYU pragma: export
#include "./fmt.h" // IWYU pragma: export
#include "./num.h"
#include <stdio.h>

static thread_local char FMT_BUF[4096];
static char              FMT_64_BUF[65] = {};

/*
Integer bits high to low. To represent a signed number, simply cast it.
The returned buffer is only valid until the next call.

Clang explodes this 4-liner into more than a kilobyte of Arm64
vector instructions and lookup tables (combined) 🤯. Wild.
*/
static const char *uint64_to_bit_str(U64 num) {
  constexpr auto LEN = 64;
  for (Ind ind = 0; ind < LEN; ind++) {
    FMT_64_BUF[ind] = ((num >> (LEN - ind - 1)) & 1) ? '1' : '0';
  }
  return FMT_64_BUF;
}

/*
Integer bits low to high. Not the same as little endian.
To represent the bits of a signed number, simply cast it.
The returned buffer is only valid until the next call.
*/
static const char *uint64_to_bit_str_reverse(U64 num) {
  constexpr auto LEN = 64;
  for (Ind ind = 0; ind < LEN; ind++) {
    FMT_64_BUF[ind] = ((num >> ind) & 1) ? '1' : '0';
  }
  return FMT_64_BUF;
}

static const char *uint32_to_bit_str(U32 num) {
  constexpr auto LEN = 32;
  for (Ind ind = 0; ind < LEN; ind++) {
    FMT_64_BUF[ind] = ((num >> (LEN - ind - 1)) & 1) ? '1' : '0';
  }
  FMT_64_BUF[LEN] = '\0';
  return FMT_64_BUF;
}

static void print_byte_range_hex(const U8 *beg, const U8 *end) {
  while (beg && beg < end) printf("%0.2x", *beg++);
}

static void eprint_byte_range_hex(const U8 *beg, const U8 *end) {
  while (beg && beg < end) eprintf("%0.2x", *beg++);
}

static char *fmt_bytes_hex_into(char *buf, Uint buf_len, const U8 *src, Uint len) {
  if (!buf_len) return nullptr;

  static constexpr char digits[] = "0123456789abcdef";
  const auto            buf_cap  = (Ind)buf_len - 1;
  Ind                   buf_ind  = 0;

  for (Ind ind = 0; ind < len && buf_ind < buf_cap; ind++) {
    const auto byte = src[ind];
    buf[buf_ind++]  = digits[(byte >> 4) & 0b1111];
    buf[buf_ind++]  = digits[byte & 0b1111];
  }

  buf[buf_ind] = 0;
  return buf;
}

static char *fmt_bytes_hex(const U8 *src, Uint len) {
  return fmt_bytes_hex_into(FMT_BUF, arr_cap(FMT_BUF), src, len);
}

static char *fmt_bytes_into(char *buf, Uint buf_len, const U8 *src, Uint src_len) {
  aver(buf_len >= 6); // {...}\0

  const auto buf_lim = buf_len - 2; // }\0
  Ind        buf_ind = 0;
  Ind        src_ind = 0;

  buf[buf_ind++] = '{';

#define fmt_bytes_into_push(val)   \
  {                                \
    buf[buf_ind++] = val;          \
    if (buf_ind >= buf_lim) break; \
  }

  for (; src_ind < src_len; src_ind++) {
    if (buf_ind > 1) {
      fmt_bytes_into_push(',');
      fmt_bytes_into_push(' ');
    }

    static constexpr char digits[] = "0123456789abcdef";
    const auto            byte     = src[src_ind];

    fmt_bytes_into_push('0');
    fmt_bytes_into_push('x');
    fmt_bytes_into_push(digits[(byte >> 4) & 0b1111]);
    fmt_bytes_into_push(digits[byte & 0b1111]);
  }

  // Ran out of buffer.
  if (src_ind < src_len) {
    buf[buf_len - 5] = '.';
    buf[buf_len - 4] = '.';
    buf[buf_len - 3] = '.';
  }

  buf[buf_ind++] = '}';
  buf[buf_ind]   = '\0';
  return buf;
}

static char *fmt_bytes(const U8 *src, Uint len) {
  return fmt_bytes_into(FMT_BUF, arr_cap(FMT_BUF), src, len);
}

/* clang-format off */

static char *fmt_char                   (char                   val) {return spf("%c",         val);}
static char *fmt_char_unsigned          (char unsigned          val) {return spf("%x",         val);}
static char *fmt_char_signed            (char signed            val) {return spf("%d",         val);}
static char *fmt_int_short_unsigned     (int short unsigned     val) {return spf("0x%04hx",    val);}
static char *fmt_int_short_signed       (int short signed       val) {return spf("%hd",        val);}
static char *fmt_int_unsigned           (int unsigned           val) {return spf("0x%08x",     val);}
static char *fmt_int_signed             (int signed             val) {return spf("%d",         val);}
static char *fmt_int_long_unsigned      (int long unsigned      val) {return spf("0x%016lx",   val);}
static char *fmt_int_long_signed        (int long signed        val) {return spf("%ld",        val);}
static char *fmt_int_long_long_unsigned (int long long unsigned val) {return spf("0x%016llx",  val);}
static char *fmt_int_long_long_signed   (int long long signed   val) {return spf("%lld",       val);}
static char *fmt_float                  (float                  val) {return spf("%f", (double)val);}
static char *fmt_double                 (double                 val) {return spf("%f",         val);}
static char *fmt_double_long            (double long            val) {return spf("%Lf",        val);}
static char *fmt_ptr                    (void const *           val) {return spf("%p",         val);}
static char *fmt_chars                  (char const *           val) {return spf("\"%s\"",     val);}

/* clang-format on */

static char *fmt_chars_unsigned(const char unsigned *src, Uint len) {
  return fmt_bytes(src, len);
}

// TODO use `%d` format.
static char *fmt_chars_signed(const char signed *src, Uint len) {
  return fmt_bytes((const U8 *)src, len);
}

static const char *bool_str(bool val) { return val ? "true" : "false"; }

static Err bool_decode(const char *src, bool *out) {
  if (!src) {
    *out = false;
    return nullptr;
  }
  if (!strcmp(src, "true")) {
    *out = true;
    return nullptr;
  }
  if (!strcmp(src, "false")) {
    *out = false;
    return nullptr;
  }
  return errf("unable to decode " FMT_QUOTED " as boolean", src);
}

/*
int main(void) {
  printf("%s\n", uint64_to_bit_str(0b00000000000000001111111111111111LL));
  printf("%s\n", uint64_to_bit_str(0b11111111111111110000000000000000LL));

  printf(
    "%s\n",
    uint64_to_bit_str_reverse(0b00000000000000001111111111111111LL)
  );

  printf(
    "%s\n",
    uint64_to_bit_str_reverse(0b11111111111111110000000000000000LL)
  );
*/

/*
int main(void) {
  {
    U32 val = 0x01234567;
    printf("0x%s\n", fmt_bytes_hex((const U8 *)&val, sizeof(val)));
  }

  {
    U64 val = 0x0123456789ABCDEF;
    printf("0x%s\n", fmt_bytes_hex((const U8 *)&val, sizeof(val)));
  }

  {
    Sint val = 1;
    printf("0x%s\n", fmt_bytes_hex((const U8 *)&val, sizeof(val)));
  }
}
*/

/*
int main(void) {
  {
    U64 val = 0x0123456789abcdef;
    printf("%s\n", fmt_any(val));
  }

  {
    U64 val = 0x0123456789abcdef;
    printf("%s\n", fmt_any(&val));
  }

  {
    const auto val = "hello world";
    static_assert(is_ptr_or_arr(val));
    static_assert(is_ptr(val));
    static_assert(!is_arr(val));
    printf("%s\n", fmt_any(val));
  }

  {
    static constexpr char val[] = "hello world";
    static_assert(is_ptr_or_arr(val));
    static_assert(!is_ptr(val));
    static_assert(is_arr(val));
    printf("%s\n", fmt_any(val));
  }

  {
    static constexpr U8 val[16] = {
      0x38,
      0x30,
      0x33,
      0x66,
      0x63,
      0x62,
      0x31,
      0x31,
      0x62,
      0x61,
      0x36,
      0x31,
      0x34,
      0x30,
      0x63,
      0x63
    };

    static_assert(is_ptr_or_arr(val));
    static_assert(!is_ptr(val));
    static_assert(is_arr(val));

    printf("%s\n", fmt_any(val));
  }

  {
    static constexpr U8 val[16] = {
      0x38,
      0x30,
      0x33,
      0x66,
      0x63,
      0x62,
      0x31,
      0x31,
      0x62,
      0x61,
      0x36,
      0x31,
      0x34,
      0x30,
      0x63,
      0x63
    };

    static_assert(is_ptr_or_arr(val));
    static_assert(!is_ptr(val));
    static_assert(is_arr(val));
    static_assert(is_ptr(&val[0]));
    static_assert(!is_arr(&val[0]));

    printf("%s\n", fmt_any(&val[0]));
  }

  {
    struct {
      int val;
    } val = {.val = 0x12345678};

    printf("%s\n", fmt_any(&val));
  }

  {
    struct {
      int val;
    } val = {.val = 0x12345678};

    printf("%s\n", fmt_any(val));
  }

  {
    U16 val[2] = {0x1234, 0x5678};
    static_assert(is_ptr_or_arr(val));
    static_assert(!is_ptr(val));
    static_assert(is_arr(val));
    printf("%s\n", fmt_any(val));
  }
}
*/
