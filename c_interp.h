#pragma once
#include "./c_asm.h"
#include "./c_read.h"
#include "./c_sym.h"
#include "./lib/stack.h"

/*
Every compiler is an interpreter.
Every Forth interpreter is a compiler.
Would be nice to have a name reflecting both.
*/
typedef struct {
  Sint_stack ints;        // Forth integer stack.
  Sym_stack  syms;        // Defined symbols.
  Sym_dict   dict;        // Symbols by name.
  Asm asm;                // Assembler; one arch for now.
  Asm     asm_snap;       // Snapshot for rewinding.
  Reader *reader;         // Text parser.
  Sym    *defining;       // Symbol being defined between `:` and `;`.
  bool    compiling  : 1; // Turned on by `:`, can be turned off.
  bool    redefining : 1; // Temporarily suppress "redefined" diagnostic.
} Interp;

static constexpr auto INTERP_INTS_FLOOR = offsetof(Interp, ints.floor);
static constexpr auto INTERP_INTS_TOP   = offsetof(Interp, ints.top);
