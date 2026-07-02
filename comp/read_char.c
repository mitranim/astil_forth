#pragma once
#include "./read.h"

static bool is_char_end(U8 val) { return !val; }

static bool is_char_space(U8 val) {
  return val == ' ' || (val >= '\t' && val <= '\r');
}

static bool is_char_glyph(U8 val) { return val >= '!' && val <= '~'; }

static bool is_char_decimal(U8 val) { return val >= '0' && val <= '9'; }

static bool is_char_num_sign(U8 val) { return val == '+' || val == '-'; }

static bool is_num_begin(U8 val, U8 next) {
  return is_char_decimal(val) ||
    (is_char_num_sign(val) && is_char_decimal(next));
}

static bool is_char_letter_upper(U8 val) { return val >= 'A' && val <= 'Z'; }

static bool is_char_letter_lower(U8 val) { return val >= 'a' && val <= 'z'; }

static bool is_char_ident_beg(U8 val) {
  return is_char_letter_lower(val) || is_char_letter_upper(val) || val == '_';
}

static bool is_char_ident(U8 val) {
  return (
    is_char_letter_lower(val) || is_char_letter_upper(val) ||
    is_char_decimal(val) || val == '_'
  );
}

static bool is_word_ident_like(Word_str name) {
  if (!name.len) return false;
  if (!is_char_ident_beg((U8)name.buf[0])) return false;
  for (Ind ind = 1; ind < name.len; ind++) {
    if (!is_char_ident((U8)name.buf[ind])) return false;
  }
  return true;
}
