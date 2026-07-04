#pragma once
#include "./read.h"
#include "../clib/err.c" // IWYU pragma: export
#include "../clib/fmt.h"
#include "../clib/mem.h"
#include "./read_char.c"
#include <execinfo.h>
#include <string.h>

static bool reader_valid(const Reader *read) {
  return read && is_aligned(read) && is_aligned(&read->src) && read->src;
}

static bool reader_has_more(const Reader *read) {
  return read->pos < read->len;
}

static Read_pos reader_pos(const Reader *read) {
  Read_pos pos = {
    .ind = read ? read->pos : 0,
    .row = 1,
    .col = 1,
  };

  if (!reader_valid(read)) return pos;

  U8 last = 0;
  for (Ind ind = 0; ind < read->pos && ind < read->len; ind++) {
    const auto next = (U8)read->src[ind];
    if ((next == '\n' && last != '\r') || last == '\r') {
      pos.row++;
      pos.col = 1;
    }
    else {
      pos.col++;
    }
    last = next;
  }

  return pos;
}

static Err reader_err(Reader *read, Err err) {
  if (!err || !reader_valid(read) || !read->pos) return err;

  return err_wrapf(
    "%s\n"
    "position: " READ_POS_FMT,
    err,
    READ_POS_ARGS(read)
  );
}

// 254 = Ctrl+D
// 255 = EOF
static U8 normalize_char(U8 val) { return val == 254 || val == 255 ? 0 : val; }

static U8 read_char_at(const Reader *read, Ind pos) {
  return pos < read->len ? normalize_char((U8)read->src[pos]) : 0u;
}

static constexpr U8 DIGIT_INVALID = 255;
static constexpr U8 DIGIT_BREAK   = 254;

static U8 char_to_digit(U8 val) {
  if (is_char_end(val)) return DIGIT_BREAK;
  if (is_char_space(val)) return DIGIT_BREAK;
  if (is_char_decimal(val)) return val - '0';
  if (is_char_letter_upper(val)) return 10 + val - 'A';
  if (is_char_letter_lower(val)) return 10 + val - 'a';
  return DIGIT_INVALID;
}

static Err err_unsupported_char(Sint code) {
  return errf("unsupported character code " FMT_SINT, code);
}

static Err validate_ascii_printable(Sint code) {
  if (!(code >= 0 && code < 256)) return err_unsupported_char(code);
  const auto val = (U8)code;
  if (is_char_end(val)) return nullptr;
  if (is_char_space(val)) return nullptr;
  if (is_char_glyph(val)) return nullptr;
  return err_unsupported_char(code);
}

static Err err_num_not_terminated(U8 val) {
  return errf(
    "numeric literal must be terminated with whitespace, got " FMT_CHAR_QUOTED,
    val
  );
}

static Err err_num_digit_radix(const char *prefix, U8 dig, U8 val, Sint radix) {
  return errf(
    "%s: digit %d from character " FMT_CHAR_QUOTED " is outside radix " FMT_SINT,
    prefix,
    dig,
    val,
    radix
  );
}

static Err err_num_not_terminated_digit(U8 dig, U8 val, Sint radix) {
  return err_num_digit_radix(
    "numeric literal must be terminated with whitespace", dig, val, radix
  );
}

static Err err_digit_radix(U8 dig, U8 val, Sint radix) {
  return err_num_digit_radix("malformed numeric literal", dig, val, radix);
}

static Err err_overflow(U8 radix, const char *mode) {
  return errf(
    "overflow of numeric literal (size: " FMT_UINT
    " bytes; radix: %d; mode: %s)",
    sizeof(Sint),
    radix,
    mode
  );
}

static Err err_minus_unsigned(const char *src, Ind len) {
  return errf(
    "unsupported minus sign in `%.*s`; only decimals may be signed", len, src
  );
}

static Err err_radix_prefix_no_digit(const char *src, Ind len) {
  return errf("unexpected bare `%.*s`; requires at least one digit", len, src);
}

/*
Supported formats:

- Binary:  0b10100101
- Octal:   0o123467
- Decimal: 123 +234 -345
- Hex:     0x123456789abcdef

Binary, octal, and hex are treated as unsigned during parsing,
and the preceding minus is forbidden for them, as it's unclear
what its semantics should be: either `~num + 1` as with signed
multiplication by -1, or simply set the sign bit to 1.
*/
static Err read_num(Reader *read, Sint *out) {
  const auto beg  = read->pos;
  auto       head = read_char_at(read, read->pos);

  S8 sign = 1;

  if (head == '-' || head == '+') {
    if (head == '-') sign = -1;
    head = read_char_at(read, ++read->pos);
    try(validate_ascii_printable(head));
  }
  else {
    IF_DEBUG(try_assert(is_char_decimal(head)));
  }

  read->pos++;

  if (!is_char_decimal(head)) return err_num_not_terminated(head);

  Sint num       = char_to_digit(head);
  U8   radix     = 10;
  bool has_digit = false;

  if (!num) {
    head = read_char_at(read, read->pos);
    try(validate_ascii_printable(head));

    switch (head) {
      case 'b': radix = 2; break;
      case 'o': radix = 8; break;
      case 'x': radix = 16; break;
      default:  break;
    }

    if (radix != 10) {
      if (sign < 0) {
        return err_minus_unsigned(read->src + beg, read->pos - beg + 1);
      }
      read->pos++;
    }
  }

  const bool is_signed = radix == 10;
  num *= sign;

  for (;;) {
    const auto head = read_char_at(read, read->pos);
    try(validate_ascii_printable(head));

    // Cosmetic group separator.
    if (head == '_') {
      read->pos++;
      continue;
    }

    const auto dig = char_to_digit(head);

    switch (dig) {
      case DIGIT_BREAK: goto done;

      // Numeric syntax owns the whole whitespace-delimited token.
      case DIGIT_INVALID: return err_num_not_terminated(head);

      default: {
        if (is_signed && dig >= 10) {
          return err_num_not_terminated_digit(dig, head, radix);
        }

        if (dig >= radix) {
          return err_digit_radix(dig, head, radix);
        }

        has_digit = true;

        if (is_signed) {
          Sint next;
          if (__builtin_mul_overflow(num, radix, &next)) {
            return err_overflow(radix, "signed");
          }
          num = next + (Sint)(dig * sign);
        }
        else {
          Uint prev = (Uint)num;
          Uint next = prev * (Uint)radix;
          if (__builtin_mul_overflow(prev, radix, &next)) {
            return err_overflow(radix, "unsigned");
          }
          num = (Sint)next + dig;
        }
        read->pos++;
      }
    }
  }

done:
  if (radix != 10 && !has_digit) {
    return err_radix_prefix_no_digit(read->src + beg, read->pos - beg);
  }
  *out = num;
  return nullptr;
}

static void read_skip_whitespace(Reader *read) {
  for (;;) {
    const auto next = read_char_at(read, read->pos);
    if (!is_char_space(next)) break;
    read->pos++;
  }
}

static Err err_word_len(Ind len, Ind cap) {
  return errf(
    "word length " FMT_IND " exceeds max word length " FMT_IND, len, cap - 1
  );
}

static Err valid_word(const char *src, Ind len, Word_str *out_word) {
  const auto cap = str_cap(out_word);
  if (len >= cap) return err_word_len(len, cap);
  try(cstr_set(out_word->buf, str_cap(out_word), src, len));
  out_word->len = len;
  return nullptr;
}

static Err read_word(Reader *read, const char **out_buf, Ind *out_len) {
  read_skip_whitespace(read);

  const auto beg = read->pos;
  for (;;) {
    const auto head = read_char_at(read, read->pos);
    try(validate_ascii_printable(head));
    if (!is_char_glyph(head)) break;
    read->pos++;
  }

  const auto len = read->pos - beg;
  if (!len) return err_str("failed to read a word");

  if (out_buf) *out_buf = read->src + beg;
  if (out_len) *out_len = len;
  return nullptr;
}

static Err read_valid_word(Reader *read, Word_str *out) {
  const char *buf;
  Ind         len;
  try(read_word(read, &buf, &len));
  return valid_word(buf, len, out);
}

static Err err_read_eof(U8 delim) {
  return errf(
    "unexpected EOF while looking for delimiter " FMT_CHAR_QUOTED, delim
  );
}

static Err read_until_char(
  Reader *read, U8 delim, const char **out_buf, Ind *out_len
) {
  const auto beg = read->pos;

  for (;;) {
    const auto next = read_char_at(read, read->pos);
    if (!next) return err_read_eof(delim);
    if (next == delim) break;
    read->pos++;
  }

  if (out_buf) *out_buf = read->src + beg;
  if (out_len) *out_len = read->pos - beg;
  return nullptr;
}
