#pragma once
#include "./arr.h"

#define FMT_QUOTED "\"%s\""
#define FMT_CHAR "\"%c\""

#define repr_struct(val) __builtin_dump_struct(val, fprintf, stderr)

#define eputs(val)       \
  {                      \
    fputs(val, stderr);  \
    fputc('\n', stderr); \
  }

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#define sprintbuf(buf, fmt, ...) \
  (snprintf((buf), arr_cap(buf), fmt __VA_OPT__(, ) __VA_ARGS__), (buf))

#define spf(...) sprintbuf(FMT_BUF, __VA_ARGS__)

/*
Usage:

  void Some_type_repr(Some_type val) {
    print_struct_beg(val, Some_type);
    print_struct_field(val, field0);
    print_struct_field(val, field1);
    print_struct_field(val, field2);
    print_struct_end();
  }

One can also use `__builtin_dump_struct`:

    __builtin_dump_struct(&some_struct, printf);

But custom printing allows specialization.
Our version also prints `char*` as string.
*/
#define print_struct_beg(val, typ)  \
  {                                 \
    static_assert_type(val, typ *); \
    puts("(" #typ "){");            \
  }

#define eprint_struct_beg(val, typ) \
  {                                 \
    static_assert_type(val, typ *); \
    eputs("(" #typ "){");           \
  }

#define print_struct_field(src, field) \
  printf("  ." #field " = %s,\n", fmt_any(src->field))

#define print_struct_field_hint(src, field, hint) \
  printf("  ." #field " = %s,%s\n", fmt_any(src->field), hint)

#define eprint_struct_field(src, field) \
  eprintf("  ." #field " = %s,\n", fmt_any(src->field))

#define eprint_struct_field_hint(src, field, hint) \
  eprintf("  ." #field " = %s,%s\n", fmt_any(src->field), hint)

#define print_struct_end() puts("};")
#define eprint_struct_end() eputs("};")

// For internal usage in fmting.
#define generic_val(typ, val) _Generic(val, typ: val, default: (typ){0})

// For internal usage in fmting.
#define generic_call(typ, fun, val, ...) \
  typ:                                   \
  fun(generic_val(typ, val) __VA_OPT__(, ) __VA_ARGS__)

// For internal usage in fmting.
#define generic_fmt_arr(typ, fun, val)                   \
  typ:                                                   \
  __builtin_choose_expr(                                 \
    is_ptr(val),                                         \
    fmt_ptr(to_ptr_or_arr(val)),                         \
    fun(generic_val(typ, val), sizeof(val) /* NOLINT */) \
  )

// For internal usage in fmting.
#define fmt_composite(val)                                               \
  __builtin_choose_expr(                                                 \
    is_arr(val),                                                         \
    fmt_bytes((const U8 *)to_ptr_or_arr(val), sizeof(val) /* NOLINT */), \
    ({                                                                   \
      const auto tmp = val;                                              \
      fmt_bytes((const U8 *)&tmp, sizeof(val) /* NOLINT */);             \
    })                                                                   \
  )

// For internal usage in fmting.
#define fmt_ptr_or_composite(val)                                \
  __builtin_choose_expr(                                         \
    is_ptr(val), fmt_ptr(to_ptr_or_arr(val)), fmt_composite(val) \
  )

/* clang-format off */
#define fmt_any(val)                                                           \
  _Generic(                                                                    \
    val,                                                                       \
    generic_call    (char,                   fmt_char,                   val), \
    generic_call    (char unsigned,          fmt_char_unsigned,          val), \
    generic_call    (char signed,            fmt_char_signed,            val), \
    generic_call    (int short unsigned,     fmt_int_short_unsigned,     val), \
    generic_call    (int short signed,       fmt_int_short_signed,       val), \
    generic_call    (int unsigned,           fmt_int_unsigned,           val), \
    generic_call    (int signed,             fmt_int_signed,             val), \
    generic_call    (int long unsigned,      fmt_int_long_unsigned,      val), \
    generic_call    (int long signed,        fmt_int_long_signed,        val), \
    generic_call    (int long long unsigned, fmt_int_long_long_unsigned, val), \
    generic_call    (int long long signed,   fmt_int_long_long_signed,   val), \
    generic_call    (float,                  fmt_float,                  val), \
    generic_call    (double,                 fmt_double,                 val), \
    generic_call    (double long,            fmt_double_long,            val), \
    generic_call    (char *,                 fmt_chars,                  val), \
    generic_call    (char const *,           fmt_chars,                  val), \
    generic_fmt_arr (char unsigned *,        fmt_chars_unsigned,         val), \
    generic_fmt_arr (char unsigned const *,  fmt_chars_unsigned,         val), \
    generic_fmt_arr (char signed *,          fmt_chars_signed,           val), \
    generic_fmt_arr (char signed const *,    fmt_chars_signed,           val), \
    default: fmt_ptr_or_composite(val)                                         \
  )
/* clang-format on */

#define fmt_any_as_hex_bytes(val)                 \
  ({                                              \
    const auto tmp = val;                         \
    fmt_bytes_hex((const U8 *)&tmp, sizeof(tmp)); \
  })
