#pragma once
#include "./c_read.h"
#include "./c_read_char_to_digit.h"
#include "./c_read_head_char_kind.h"
#include "./c_read_word_char_kind.h"
#include "./lib/err.c" // IWYU pragma: export
#include "./lib/fmt.h"
#include "./lib/mem.h"
#include <execinfo.h>
#include <stdio.h>
#include <string.h>

static void reader_init(Reader *read) {
  read->tty = isatty(fileno(read->file));
}

static bool reader_valid(const Reader *read) {
  return read && is_aligned(read) && is_aligned(&read->file) && read->file;
}

static Err reader_err(Reader *read, Err err) {
  if (!err || !reader_valid(read) || !read->pos.ind) return err;

  return err_wrapf(
    "%s\n"
    "reader context: " READ_POS_FMT,
    err,
    READ_POS_ARGS(read)
  );
}

static void read_char(Reader *read, char *out) {
  char next = read->next;

  if (next) {
    read->next = 0;
    *out       = next;
    return;
  }

  next = (char)fgetc(read->file);

  // IF_DEBUG(eprintf("[system] read char code: %d\n", next));
  // IF_DEBUG(eprintf("[system] read char: %c\n", next));

  /*
  Row/col counting is why we don't use `ungetc` for pushback:
  it would be impossible to tell if the next read character
  has already been read.

  Assumes pure ASCII. TODO support UTF-8.
  */

  const char last = read->last;
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
  *out       = next;
}

static void reader_backtrack(Reader *read, char val) {
  aver(!read->next);
  read->next = val;
}

static Err err_unsupported_char(Sint code) {
  return errf("unsupported character code " FMT_SINT, code);
}

static Err validate_char_ascii_printable(Sint code) {
  if (!(code >= 0 && code < 256)) return err_unsupported_char(code);
  switch (HEAD_CHAR_KIND[code]) {
    case CHAR_WHITESPACE:  [[fallthrough]];
    case CHAR_DECIMAL:     [[fallthrough]];
    case CHAR_ARITH:       [[fallthrough]];
    case CHAR_WORD:        return nullptr;
    case CHAR_EOF:         [[fallthrough]];
    case CHAR_UNPRINTABLE: [[fallthrough]];
    default:               return err_unsupported_char(code);
  }
}

static Err read_ascii_printable(Reader *read, U8 *out) {
  char next;
  read_char(read, &next);
  if (next == 255) next = 0; // Ctrl+D = EOF.
  *out = next;
  return nullptr;
}

// static Err err_num_sign(Read *read, Sint radix) {
//   return errf(
//     "syntax error: unexpected negative numeric literal in radix " FMT_SINT "; only decimal literals may be negative; other radixes are treated as unsigned",
//     radix
//   );
// }

static Err err_not_digit(char val) {
  return errf(
    "unexpected non-digit character " FMT_CHAR " after numeric literal", val
  );
}

static Err err_digit_radix(U8 dig, char val, Sint radix) {
  return errf(
    "malformed numeric literal: digit %d from character " FMT_CHAR
    " is outside radix " FMT_SINT,
    dig,
    val,
    radix
  );
}

static Err err_overflow(Sint radix) {
  return errf(
    "overflow of numeric literal (size: " FMT_UINT " bits; radix: " FMT_SINT ")",
    sizeof(Sint),
    radix
  );
}

/*
Assumes `head` is a decimal digit.
Supports only integers and doesn't check overflow.
*/
static Err read_num(Reader *read, U8 head, Sint sign, Sint *out) {
  Sint radix = 10;
  Sint num   = sign * (Sint)CHAR_TO_DIGIT[head];

  if (num == 0) {
    try(read_ascii_printable(read, &head));

    switch (head) {
      case 'b': {
        radix = 2;
        break;
      }
      case 'o': {
        radix = 8;
        break;
      }
      case 'x': {
        radix = 16;
        break;
      }
      default: {
        reader_backtrack(read, head);
        break;
      }
    }
  }

  for (;;) {
    try(read_ascii_printable(read, &head));

    // Cosmetic digit group separator.
    if (head == '_') continue;

    const auto dig = CHAR_TO_DIGIT[head];

    switch (dig) {
      case DIGIT_INVALID: return err_not_digit(head);

      case DIGIT_BREAK: {
        reader_backtrack(read, head);
        goto done;
      }

      default: {
        if (dig >= radix) return err_digit_radix(dig, head, radix);
        Sint next;
        if (__builtin_mul_overflow(num, radix, &next)) {
          return err_overflow(radix);
        }
        num = next + (dig * sign);
        continue;
      }
    }
  }

done:
  *out = num;
  return nullptr;
}

static Err err_word_len(const Reader *read, U8 len) {
  return errf(
    "word " FMT_QUOTED " exceeds max word length %d", read->word.buf, len
  );
}

static Err read_skip_whitespace(Reader *read) {
  U8 next;

  for (;;) {
    try(read_ascii_printable(read, &next));
    if (HEAD_CHAR_KIND[next] == CHAR_WHITESPACE) continue;
    reader_backtrack(read, next);
    return nullptr;
  }
}

// Invalidates the earlier state of `read->word`.
static Err read_word_after(Reader *read, U8 head) {
  const auto word = &read->word;
  str_trunc(word);
  if (head) word->buf[word->len++] = head;

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

  str_terminate(word);
  return nullptr;
}

static Err read_word(Reader *read) {
  try(read_word_after(read, 0));
  if (read->word.len) return nullptr;
  return err_str("failed to read a word");
}

static Err err_read_eof(U8 delim) {
  return errf("unexpected EOF while looking for delimiter " FMT_CHAR, delim);
}

static Err err_read_buf_over(U8 delim) {
  return errf(
    "reader buffer overflow while looking for delimiter " FMT_CHAR, delim
  );
}

static Err read_until(Reader *read, U8 delim) {
  const auto buf = &read->buf;
  str_trunc(buf);

  for (;;) {
    char next;
    read_char(read, &next);

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
