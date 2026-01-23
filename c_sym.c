#pragma once
#include "./c_sym.h"
#include "./lib/set.c"

// clang-format off

// Sanity check used in debugging.
static bool sym_valid(const Sym *sym) {
  return (
    sym &&
    is_aligned(sym) &&
    is_aligned(&sym->name) &&
    is_aligned(&sym->callees) &&
    is_aligned(&sym->callers) &&
    sym->name.len &&
    set_valid((const Set *)&sym->callees) &&
    set_valid((const Set *)&sym->callers)
  );
}

// clang-format on

static void sym_deinit(Sym *sym) {
  set_deinit(&sym->callees);
  set_deinit(&sym->callers);
}

static void sym_init_intrin(Sym *sym) {
  sym->type        = SYM_INTRIN;
  sym->name.len    = (Ind)strlen(sym->name.buf);
  sym->clobber     = ARCH_VOLATILE_REGS;
  sym->interp_only = true;
}

static bool is_sym_leaf(const Sym *sym) { return !sym->callees.len; }

static void sym_auto_comp_only(Sym *caller, const Sym *callee) {
  if (caller->comp_only) return;
  if (!callee->comp_only) return;

  caller->comp_only = true;

  IF_DEBUG(eprintf(
    "[system] word " FMT_QUOTED " uses compile-only word " FMT_QUOTED
    ", marking as compile-only\n",
    caller->name.buf,
    callee->name.buf
  ));
}

// Redundant with the caller-callee graph. TODO consider deduping.
static void sym_auto_interp_only(Sym *caller, const Sym *callee) {
  if (caller->interp_only) return;
  if (!callee->interp_only) return;

  caller->interp_only = true;

  IF_DEBUG(eprintf(
    "[system] word " FMT_QUOTED " uses interpreter-only word " FMT_QUOTED
    ", marking as interpreter-only\n",
    caller->name.buf,
    callee->name.buf
  ));
}

static void sym_auto_throws(Sym *caller, const Sym *callee) {
  if (callee->throws) caller->throws = true;
}

static Err err_inline_not_norm(const Sym *sym) {
  return errf(
    "unable to inline " FMT_QUOTED ": not a regular Forth word", sym->name.buf
  );
}

static Err err_inline_pc_rel(const Sym *sym) {
  return errf(
    "unable to inline word " FMT_QUOTED
    ": contains operations relative to the program counter (instruction address)",
    sym->name.buf
  );
}

static Err err_inline_early_ret(const Sym *sym) {
  return errf(
    "unable to inline word " FMT_QUOTED ": contains early returns", sym->name.buf
  );
}

static Err err_inline_not_leaf(const Sym *sym) {
  return errf(
    "unable to inline word " FMT_QUOTED ": not a leaf procedure", sym->name.buf
  );
}

static Err err_inline_has_data(const Sym *sym) {
  return errf(
    "unable to inline word " FMT_QUOTED ": loads local immediate values",
    sym->name.buf
  );
}

// SYNC[sym_inlinable].
static Err validate_sym_inlinable(const Sym *sym) {
  if (sym->type != SYM_NORM) return err_inline_not_norm(sym);
  if (sym->norm.has_loads) return err_inline_pc_rel(sym);
  if (sym->norm.has_rets) return err_inline_early_ret(sym);
  if (!is_sym_leaf(sym)) return err_inline_not_leaf(sym);

  // Same as `err_inline_pc_rel`. Redundant check for safety.
  const auto spans = &sym->norm.spans;
  IF_DEBUG(aver(sym->norm.has_loads == (spans->data < spans->ceil)));
  if (spans->data < spans->ceil) return err_inline_has_data(sym);

  return nullptr;
}

static void sym_auto_inlinable(Sym *sym) {
  // SYNC[sym_inlinable].
  if (sym->type != SYM_NORM) return;
  if (sym->norm.has_loads) return;
  if (sym->norm.has_rets) return;
  if (!is_sym_leaf(sym)) return;

  const auto spans = &sym->norm.spans;
  if (spans->data < spans->ceil) return;

  const auto len = spans->epilogue - spans->inner;
  if (len > ARCH_INLINABLE_INSTR_LEN) return;

  sym->norm.inlinable = true;

  IF_DEBUG(
    eprintf("[system] symbol " FMT_QUOTED " is auto-inlinable\n", sym->name.buf)
  );
}
