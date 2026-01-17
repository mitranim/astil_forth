#pragma once
#include "./lib/num.h"
#include "./lib/str.h"
#include <stdio.h>

typedef str_buf(4096) Read_buf;

typedef struct {
  Ind ind;
  Ind row;
  Ind col;
} Read_pos;

typedef struct {
  FILE    *file;
  Path_str file_path;
  Read_pos pos;
  Read_pos word_pos; // Last non-whitespace.
  Word_str word;     // Used for word names.
  Read_buf buf;      // Used for arbitrary text.
  Read_buf back;     // Backtrack buffer.
  U8       last;     // Last read char.
  bool     tty;      // True if file is interactive teletype.
} Reader;

#define READ_POS_FMT "position " FMT_IND "; %s:" FMT_IND ":" FMT_IND

#define READ_POS_ARGS(read)                                          \
  (read)->word_pos.ind, (read)->file_path.buf, (read)->word_pos.row, \
    (read)->word_pos.col

typedef enum : U8 {
  CHAR_EOF = 1,
  CHAR_UNPRINTABLE,
  CHAR_WHITESPACE,
  CHAR_DECIMAL,
  CHAR_ARITH,
  CHAR_WORD,
} Char_kind;
