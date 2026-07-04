#pragma once
#include "../clib/dict.h"
#include "../clib/stack.h"
#include "./comp.h"
#include "./read.h"
#include "./sym.h"

// SYNC[interp_snap_fields].
typedef struct {
  Comp       comp;
  Sint_stack ints;
  Sym_stack  syms;
} Interp_snap;

/*
Per-module state, created for each import.

SYNC[interp_module_fields].
*/
typedef struct {
  Reader reader;
  bool   slop;
} Module_ctx;

/*
Every compiler is an interpreter.
Every Forth interpreter is a compiler.
Would be nice to have a name describing both.

The Forth code which uses the register-based calling convention
accesses the data stack via the interpreter object, rather than
via a dedicated register, and hardcodes the field offsets which
must be kept in sync.

SYNC[interp_fields].
*/
typedef struct {
  Sint_stack   ints;      // Forth integer stack.
  Uint         argc;      // Interpreter CLI arg count.
  const char **argv;      // Interpreter CLI args.
  Sym_dict     dict_exec; // Wordlist `WORDLIST_EXEC`.
  Sym_dict     dict_comp; // Wordlist `WORDLIST_COMP`.
  Sym_stack    syms;      // Defined symbols.
  Module_ctx  *module;    // Context of the foremost module being read.
  Str_set      imports;   // Realpaths of already-imported files.
  Comp         comp;      // Code and compilation context.
  Interp_snap  snap;      // Stable snapshot for rewinding.
  bool         welcomed;  // Already printed REPL help.
  bool         slop;      // Disable validation of sloppy code in reg-CC.
} Interp;

static constexpr auto INTERP_INTS_FLOOR = offsetof(Interp, ints.floor);
static constexpr auto INTERP_INTS_TOP   = offsetof(Interp, ints.top);
