#pragma once
#include "../clib/num.h"
#include "../clib/str.h"

// SYNC[word_str].
typedef str_buf(128) Word_str;

typedef struct {
  Ind ind;
  Ind row;
  Ind col;
} Read_pos;

typedef struct {
  const char *src;
  Ind         len;
  Ind         pos;
  const char *path;
  bool        tty;
} Reader;

#define READ_POS_FMT "%s:" FMT_IND ":" FMT_IND

// TODO avoid double parsing.
#define READ_POS_ARGS(read) \
  (read)->path, reader_pos(read).row, reader_pos(read).col

typedef enum : U8 {
  CHAR_EOF = 1,
  CHAR_UNPRINTABLE,
  CHAR_WHITESPACE,
  CHAR_DECIMAL,
  CHAR_ARITH,
  CHAR_WORD,
} Char_kind;
