#pragma once
#include "../clib/dict.h"
#include "../clib/stack.h"
#include "./comp.h"
#include "./read.h"
#include "./sym.h"

// SYNC[interp_snap_fields].
typedef struct {
  Comp      comp;
  Sint_span cells;
  Sym_stack syms;
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
Ambient execution context; address is kept in `ASM_REG_CTX`.

Minimum required roles:
- Indicate if interpreter is available and we're on its thread.
- Provide a memory arena. Caller owns and frees; callees "allocate" within.

The `Interp*` object begins with `Ctx`, and its `.self` is self-referencing:
the value is the self-address of `Interp*` / `Ctx*`. In all other contexts,
the value of `.self` must not self-reference, and is otherwise undefined.

SYNC[ctx_fields].
*/
typedef struct {
  void *self; // Self-address / `Interp*` in interpreter mode.
  U8   *top;  // Borrowed bump-memory top.
  U8   *ceil; // Borrowed bump-memory ceiling.
} Ctx;

/*
Every compiler is an interpreter.
Every Forth interpreter is a compiler.
Would be nice to have a name describing both.

SYNC[interp_fields].
*/
typedef struct {
  Ctx          ctx;       // Ambient Forth context; must be first.
  Sint_span    cells;     // Forth cell stack; views `Comp_heap.cells`.
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

static_assert(!offsetof(Interp, ctx));
static_assert(offsetof(Interp, cells) == sizeof(Ctx));

static constexpr auto CTX_CELLS_FLOOR = offsetof(Interp, cells.floor);
static_assert(CTX_CELLS_FLOOR == 40);

// SYNC[ctx_cells_top].
static constexpr auto CTX_CELLS_TOP = offsetof(Interp, cells.top);
static_assert(CTX_CELLS_TOP == 24);

// SYNC[interp_comp_code_write_off].
static constexpr auto INTERP_COMP_CODE_WRITE_OFF = offsetof(
  Interp, comp.code.write
);
static_assert(INTERP_COMP_CODE_WRITE_OFF == 208);

// SYNC[interp_data_off].
static constexpr auto INTERP_DATA_OFF = offsetof(Interp, comp.code.data);
static_assert(INTERP_DATA_OFF == 272);
