#pragma once
#include "./read.h"
#include "../clib/err.c" // IWYU pragma: export
#include "../clib/fmt.h"
#include "../clib/mem.h"
#include "./read_char_to_digit.h"
#include "./read_head_char_kind.h"
#include "./read_word_char_kind.h"
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

static Err err_unsupported_char(Sint code) {
  return errf("unsupported character code " FMT_SINT, code);
}

// 0 is a special case; caller must check for it.
static Err validate_ascii_printable(Sint code) {
  if (!(code >= 0 && code < 256)) return err_unsupported_char(code);
  switch (HEAD_CHAR_KIND[code]) {
    case CHAR_WHITESPACE:  [[fallthrough]];
    case CHAR_DECIMAL:     [[fallthrough]];
    case CHAR_ARITH:       [[fallthrough]];
    case CHAR_WORD:        return nullptr;
    case CHAR_EOF:         return nullptr;
    case CHAR_UNPRINTABLE: [[fallthrough]];
    default:               return err_unsupported_char(code);
  }
}

static Err err_not_digit(U8 val) {
  return errf(
    "unexpected non-digit character " FMT_CHAR_QUOTED " after numeric literal",
    val
  );
}

static Err err_digit_radix(U8 dig, U8 val, Sint radix) {
  return errf(
    "malformed numeric literal: digit %d from character " FMT_CHAR_QUOTED
    " is outside radix " FMT_SINT,
    dig,
    val,
    radix
  );
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

static Err err_minus_unsigned(Sint radix) {
  return errf(
    "unsupported minus sign before an unsigned numeric literal in radix " FMT_SINT
    "; only decimals may be signed",
    radix
  );
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
  auto head = read_char_at(read, read->pos);

  S8 sign = 1;

  if (head == '-' || head == '+') {
    if (head == '-') sign = -1;
    head = read_char_at(read, ++read->pos);
    try(validate_ascii_printable(head));
  }
  else {
    IF_DEBUG(assert_fatal(HEAD_CHAR_KIND[head] == CHAR_DECIMAL));
  }

  read->pos++;

  if (HEAD_CHAR_KIND[head] != CHAR_DECIMAL) return err_not_digit(head);

  Sint num       = CHAR_TO_DIGIT[head];
  U8   radix     = 10;
  bool is_signed = true;

  if (!num) {
    head = read_char_at(read, read->pos);
    try(validate_ascii_printable(head));

    switch (head) {
      case 'b': {
        if (sign < 0) return err_minus_unsigned(radix);
        radix     = 2;
        is_signed = false;
        read->pos++;
        break;
      }
      case 'o': {
        if (sign < 0) return err_minus_unsigned(radix);
        radix     = 8;
        is_signed = false;
        read->pos++;
        break;
      }
      case 'x': {
        if (sign < 0) return err_minus_unsigned(radix);
        radix     = 16;
        is_signed = false;
        read->pos++;
        break;
      }
      default: break;
    }
  }

  num *= sign;

  for (;;) {
    const auto head = read_char_at(read, read->pos);
    try(validate_ascii_printable(head));

    // Cosmetic group separator.
    if (head == '_') {
      read->pos++;
      continue;
    }

    const auto dig = CHAR_TO_DIGIT[head];

    switch (dig) {
      case DIGIT_BREAK: goto done;

      // Unlike other Forths, we don't allow numeric literals to be immediately
      // followed by non-digit printable characters. Anything that begins with
      // a digit or `+`/`-` and a digit must be a number, not a word.
      case DIGIT_INVALID: return err_not_digit(head);

      default: {
        if (dig >= radix) {
          return err_digit_radix(dig, head, radix);
        }

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
  *out = num;
  return nullptr;
}

static void read_skip_whitespace(Reader *read) {
  for (;;) {
    const auto next = read_char_at(read, read->pos);
    if (HEAD_CHAR_KIND[next] != CHAR_WHITESPACE) break;
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
    if (WORD_CHAR_KIND[head] != CHAR_WORD) break;
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
