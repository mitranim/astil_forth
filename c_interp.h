#pragma once
#include "./c_comp.h"
#include "./c_read.h"
#include "./c_sym.h"
#include "./lib/dict.h"
#include "./lib/stack.h"

typedef struct {
  Ind  ints_len;
  Ind  syms_len;
  Comp comp;
} Interp_snap;

/*
Every compiler is an interpreter.
Every Forth interpreter is a compiler.
Would be nice to have a name reflecting both.
*/
typedef struct {
  Sint_stack  ints;      // Forth integer stack.
  Reader     *reader;    // Each input file has its own parser state.
  Sym_stack   syms;      // Defined symbols.
  Sym_dict    dict_exec; // Wordlist `WORDLIST_EXEC`.
  Sym_dict    dict_comp; // Wordlist `WORDLIST_COMP`.
  Str_set     imports;   // Realpaths of already-imported files.
  Comp        comp;      // Code and compilation context.
  Interp_snap snap;      // Stable snapshot for rewinding.
} Interp;

static constexpr auto INTERP_INTS_FLOOR = offsetof(Interp, ints.floor);
static constexpr auto INTERP_INTS_TOP   = offsetof(Interp, ints.top);
