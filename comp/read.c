#pragma once
#include "./lib/err.c" // IWYU pragma: export
#include "./lib/fmt.h"
#include "./lib/mem.h"
#include "./read.h"
#include "./read_char_to_digit.h"
#include "./read_head_char_kind.h"
#include "./read_word_char_kind.h"
#include <execinfo.h>
#include <stdio.h>
#include <string.h>

static bool reader_valid(const Reader *read) {
  return read && is_aligned(read) && is_aligned(&read->file) && read->file;
}

static Err reader_err(Reader *read, Err err) {
  if (!err || !reader_valid(read) || !read->pos.ind) return err;

  return err_wrapf(
    "%s\n"
    "position: " READ_POS_FMT,
    err,
    READ_POS_ARGS(read)
  );
}

// TODO clearer name.
static U8 reader_unpeek(Reader *read) {
  const auto buf = &read->back;
  return buf->len ? buf->buf[buf->len-- - 1] : (U8)0;
}

static void reader_backtrack(Reader *read, U8 val) {
  str_push(&read->back, val);
}

static void reader_backtrack_word(Reader *read) {
  const auto word = &read->word;
  while (word->len) {
    reader_backtrack(read, word->buf[--word->len]);
  }
}

// 254 = Ctrl+D
// 255 = EOF
static U8 normalize_char(U8 val) { return val == 254 || val == 255 ? 0 : val; }

// For internal use by the reader. External code should
// use `read_ascii_printable` or `read_char`.
static U8 read_next_char(Reader *read) {
  const auto next = normalize_char((U8)fgetc(read->file));

  // IF_DEBUG(eprintf("[system] read char code: %d\n", next));
  // IF_DEBUG(eprintf("[system] read char: %c\n", next));

  /*
  Row/col counting is why we don't use `ungetc` for pushback:
  it would be impossible to tell if the next read character
  has already been read.

  Assumes pure ASCII. TODO support UTF-8.
  */

  const auto last = read->last;
  auto       pos  = read->pos;

  if (!pos.row) pos.row = 1;
  if (!pos.col) pos.col = 1;
  pos.ind++;

  if ((next == '\n' && last != '\r') || last == '\r') {
    pos.row++;
    pos.col = 1;
  }
  else {
    pos.col++;
  }

  if (WORD_CHAR_KIND[next] == CHAR_WORD) read->word_pos = pos;
  read->pos  = pos;
  read->last = next;
  return next;
}

static U8 read_char(Reader *read) {
  const auto next = reader_unpeek(read);
  if (next) return next;
  return read_next_char(read);
}

static Err err_unsupported_char(Sint code) {
  return errf("unsupported character code " FMT_SINT, code);
}

/*
static U8 reader_peek(Reader *read) {
  const auto buf = &read->back;
  if (!buf->len) reader_backtrack(read, read_next_char(read));
  return buf->buf[buf->len - 1];
}
*/

static Err validate_char_ascii_printable(Sint code) {
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

// Output is 0 on EOF.
static Err read_ascii_printable(Reader *read, U8 *out) {
  auto next = read_char(read);
  try(validate_char_ascii_printable(next));
  *out = next;
  return nullptr;
}

/*
static Err reader_peek_ascii_printable(Reader *read, U8 *out) {
  const auto next = reader_peek(read);
  try(validate_char_ascii_printable(next));
  *out = next;
  return nullptr;
}
*/

static Err err_not_digit(U8 val) {
  return errf(
    "unexpected non-digit character " FMT_CHAR " after numeric literal", val
  );
}

static Err err_digit_radix(U8 dig, U8 val, Sint radix) {
  return errf(
    "malformed numeric literal: digit %d from character " FMT_CHAR
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

static Err err_minus_unsigned(U8 radix) {
  return errf(
    "unsupported minus sign before an unsigned numeric literal in radix %d; only decimals may be signed",
    radix
  );
}

static Err read_num_internal(
  Reader *read, Sint num, S8 sign, U8 radix, bool is_signed, Sint *out
) {
  for (;;) {
    U8 head;
    try(read_ascii_printable(read, &head));

    // Cosmetic group separator.
    if (head == '_') continue;

    const auto dig = CHAR_TO_DIGIT[head];

    switch (dig) {
      // Unlike other Forths, we don't allow numeric literals to be immediately
      // followed by non-digit printable characters. Anything that begins with
      // a digit or `+`/`-` and a digit must be a number, not a word.
      case DIGIT_INVALID: {
        return err_not_digit(head);
      }

      case DIGIT_BREAK: {
        reader_backtrack(read, head);
        goto done;
      }

      default: {
        if (dig >= radix) {
          return err_digit_radix(dig, head, radix);
        }

        if (is_signed) {
          Sint next;
          if (__builtin_mul_overflow(num, radix, &next)) {
            return err_overflow(radix, "signed");
          }
          num = next + (dig * sign);
        }
        else {
          Uint prev = (Uint)num;
          Uint next = prev * (Uint)radix;
          if (__builtin_mul_overflow(prev, radix, &next)) {
            return err_overflow(radix, "unsigned");
          }
          num = (Sint)next + dig;
        }
        continue;
      }
    }
  }

done:
  *out = num;
  return nullptr;
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
  S8 sign = 1;
  U8 head;

  try(read_ascii_printable(read, &head));

  if (head == '-') {
    sign = -1;
    try(read_ascii_printable(read, &head));
  }
  else if (head == '+') {
    try(read_ascii_printable(read, &head));
  }

  if (HEAD_CHAR_KIND[head] != CHAR_DECIMAL) return err_not_digit(head);

  const auto num = (Sint)CHAR_TO_DIGIT[head];

  if (num == 0) {
    try(read_ascii_printable(read, &head));

    switch (head) {
      case 'b': {
        const auto radix = 2;
        if (sign < 0) return err_minus_unsigned(radix);
        return read_num_internal(read, num, 0, radix, false, out);
      }
      case 'o': {
        const auto radix = 8;
        if (sign < 0) return err_minus_unsigned(radix);
        return read_num_internal(read, num, 0, radix, false, out);
      }
      case 'x': {
        const auto radix = 16;
        if (sign < 0) return err_minus_unsigned(radix);
        return read_num_internal(read, num, 0, radix, false, out);
      }
      default: {
        reader_backtrack(read, head);
        break;
      }
    }
  }

  return read_num_internal(read, num * sign, sign, 10, true, out);
}

static Err err_word_len(const Reader *read, U8 len) {
  return errf(
    "word " FMT_QUOTED " exceeds max word length %d", read->word.buf, len
  );
}

static void read_skip_space(Reader *read) {
  const auto next = read_char(read);
  if (HEAD_CHAR_KIND[next] == CHAR_WHITESPACE) return;
  reader_backtrack(read, next);
}

static void read_skip_whitespace(Reader *read) {
  for (;;) {
    const auto next = read_char(read);
    if (HEAD_CHAR_KIND[next] == CHAR_WHITESPACE) continue;
    reader_backtrack(read, next);
    break;
  }
}

// Invalidates the earlier state of `read->word`.
static Err read_word(Reader *read) {
  read_skip_whitespace(read);

  const auto word = &read->word;
  str_trunc(word);

  for (;;) {
    U8 byte;
    try(read_ascii_printable(read, &byte));

    if (WORD_CHAR_KIND[byte] != CHAR_WORD) {
      reader_backtrack(read, byte);
      break;
    }

    if (str_push(word, byte)) {
      return err_word_len(read, str_cap(word) - 1);
    }
  }

  if (!word->len) return err_str("failed to read a word");
  str_terminate(word);
  return nullptr;
}

static Err err_read_eof(U8 delim) {
  return errf("unexpected EOF while looking for delimiter " FMT_CHAR, delim);
}

static Err err_read_buf_over(U8 delim) {
  return errf(
    "reader buffer overflow while looking for delimiter " FMT_CHAR, delim
  );
}

static Err read_until_char(Reader *read, U8 delim) {
  const auto buf = &read->buf;
  str_trunc(buf);

  for (;;) {
    auto next = read_char(read);

    if (next == delim) {
      str_terminate(buf);
      return nullptr;
    }

    if (!next) return err_read_eof(delim);

    if (str_push(buf, next)) {
      return err_read_buf_over(delim);
    }
  }

  return err_read_buf_over(delim);
}

// TODO: expose an intrinsic with this.
static Err read_until_word(Reader *read, const char *delim) {
  for (;;) {
    try(read_word(read));
    if (str_eq(&read->word, delim)) break;
  }
  return nullptr;
}
