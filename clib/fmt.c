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
  constexpr U8 LEN = 64;
  for (Ind ind = 0; ind < LEN; ind++) {
    FMT_64_BUF[ind] = ((num >> (LEN - ind - 1)) & 1u) ? '1' : '0';
  }
  return FMT_64_BUF;
}

/*
Integer bits low to high. Not the same as little endian.
To represent the bits of a signed number, just cast it.
The returned buffer is only valid until the next call.
*/
static const char *uint64_to_bit_str_reverse(U64 num) {
  constexpr U8 LEN = 64;
  for (Ind ind = 0; ind < LEN; ind++) {
    FMT_64_BUF[ind] = ((num >> ind) & 1u) ? '1' : '0';
  }
  return FMT_64_BUF;
}

static const char *uint32_to_bit_str(U32 num) {
  constexpr U8 LEN = 32;
  for (Ind ind = 0; ind < LEN; ind++) {
    FMT_64_BUF[ind] = ((num >> (LEN - ind - 1)) & 1u) ? '1' : '0';
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

static char *fmt_bytes_hex_into(char *buf, Ind cap, const U8 *src, Ind len) {
  if (!cap) return nullptr;

  // Want enough space for `...\0`.
  if (cap < 4 || !len) {
    buf[0] = '\0';
    return buf;
  }

  const auto buf_ceil = buf + cap;
  const auto buf_end  = buf_ceil - 1; // Reserve `\0`.
  const auto src_ceil = src + len;

  auto buf_top = buf;
  auto src_top = src;

  while (src_top < src_ceil) {
    const auto rem = buf_ceil - buf_top;
    if (rem < 2) break;

    static constexpr char digits[] = "0123456789abcdef";
    const auto            byte     = *src_top++;

    /*
    Why all the casts: apparently, C "promotes" operands of bitwise operations
    from shorter types to SIGNED `int` THEN `clang-tidy` complains about using
    signed `int` in bitwise operations. What insanity.
    */
    const U8 high = (U8)(byte >> (U8)4) & (U8)0b1111;

    *buf_top++ = digits[high];
    *buf_top++ = digits[byte & (U8)0b1111];
  }

  // Ran out of buffer.
  if (src_top < src_ceil) {
    buf_end[-1] = '.';
    buf_end[-2] = '.';
    buf_end[-3] = '.';
    buf_top     = buf_end;
  }

  *buf_top = '\0';
  return buf;
}

static char *fmt_bytes_hex(const U8 *src, Ind len) {
  return fmt_bytes_hex_into(FMT_BUF, arr_cap(FMT_BUF), src, len);
}

static char *fmt_bytes_into(char *buf, Ind cap, const U8 *src, Ind src_len) {
  // Want enough space for `{...}\0` and `{0xNN}\0`.
  if (cap < 8) return nullptr;

  if (!src_len) {
    // NOLINTBEGIN(clang-analyzer-security.insecureAPI.strcpy)
    strcpy(buf, "{}");
    // NOLINTEND(clang-analyzer-security.insecureAPI.strcpy)
    return buf;
  }

  const auto src_ceil   = src + src_len;
  const auto buf_ceil   = buf + cap;
  const auto buf_end    = buf_ceil - 1; // Reserve `\0`.
  const auto inner_ceil = buf_ceil - 2; // Reserve `}\0`.

  auto buf_top = buf;
  auto src_top = src;

  *buf_top++ = '{';

  const auto inner_beg = buf_top;

  while (src_top < src_ceil) {
    const bool sep = buf_top > inner_beg;
    const auto rem = inner_ceil - buf_top;
    const auto req = sep ? 6 : 4; // `0xNN` and maybe `, `

    if (rem < req) break;

    if (sep) {
      *buf_top++ = ',';
      *buf_top++ = ' ';
    }

    static constexpr char digits[] = "0123456789abcdef";

    // False positive.
    //
    // NOLINTBEGIN(clang-analyzer-core.uninitialized.Assign)

    const auto byte = *src_top++;

    // NOLINTEND(clang-analyzer-core.uninitialized.Assign)

    /*
    Why all the casts: apparently, C "promotes" operands of bitwise operations
    from shorter types to SIGNED `int` THEN `clang-tidy` complains about using
    signed `int` in bitwise operations. What insanity.
    */
    const U8 high = (U8)(byte >> (U8)4) & (U8)0b1111;

    *buf_top++ = '0';
    *buf_top++ = 'x';
    *buf_top++ = digits[high];
    *buf_top++ = digits[byte & (U8)0b1111];
  }

  aver(buf_top <= inner_ceil);

  // Ran out of buffer for inner content.
  if (src_top < src_ceil) {
    buf_top -= 4;
    aver(buf_top >= inner_beg);
    *buf_top++ = '.';
    *buf_top++ = '.';
    *buf_top++ = '.';
  }

  aver(buf_top < buf_end);
  *buf_top++ = '}';

  aver(buf_top <= buf_end);
  *buf_top = '\0';

  return buf;
}

static char *fmt_bytes(const U8 *src, Ind len) {
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

static char *fmt_chars_unsigned(const char unsigned *src, Ind len) {
  return fmt_bytes(src, len);
}

// TODO use `%d` format.
static char *fmt_chars_signed(const char signed *src, Ind len) {
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
