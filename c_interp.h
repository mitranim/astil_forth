#pragma once
#include "./c_asm.h"
#include "./c_read.h"
#include "./c_sym.h"
#include "./lib/dict.h"
#include "./lib/stack.h"

/*
Every compiler is an interpreter.
Every Forth interpreter is a compiler.
Would be nice to have a name reflecting both.
*/
typedef struct {
  Sint_stack ints;    // Forth integer stack.
  Sym_stack  syms;    // Defined symbols.
  Sym_dict   words;   // Symbols by name.
  Asm asm;            // Assembler; one arch for now.
  Asm     asm_snap;   // Snapshot for rewinding.
  Str_set imports;    // Realpaths of already-imported files.
  Reader *reader;     // Each input file has its own parser state.
  Sym    *defining;   // Symbol being defined between `:` and `;`.
  bool    compiling;  // Turned on by `:`, can be turned off.
  bool    redefining; // Temporarily suppress "redefined" diagnostic.
} Interp;

static constexpr auto INTERP_INTS_FLOOR = offsetof(Interp, ints.floor);
static constexpr auto INTERP_INTS_TOP   = offsetof(Interp, ints.top);
