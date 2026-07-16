#pragma once
#include "./sym.h"
#include "../clib/set.c"
#include "./read_char.c"

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
    (sym->type != SYM_EXTERN || (sym->link_name && sym->link_name[0])) &&
    set_valid((const Set *)&sym->callees) &&
    set_valid((const Set *)&sym->callers)
  );
}

static Err err_declaration_call_like(const char *name) {
  return errf("invalid declaration " FMT_QUOTED "; use `.%s`", name, name);
}

static Err err_declaration_plain_call(const char *name) {
  return errf(
    "invalid declaration " FMT_QUOTED
    "; use an ident-like name or remove `.plain_call`",
    name
  );
}

static Err sym_validate_name(const Sym *sym) {
  const auto name = sym->name;
  if (is_num_begin((U8)name.buf[0], (U8)name.buf[1])) {
    return err_declaration_call_like(name.buf);
  }

  const auto ident_like = is_word_ident_like(name);
  if (sym->plain_call) {
    return ident_like ? nullptr : err_declaration_plain_call(name.buf);
  }
  return ident_like ? err_declaration_call_like(name.buf) : nullptr;
}

// clang-format on

static void sym_deinit(Sym *sym) {
  set_deinit(&sym->callees);
  set_deinit(&sym->callers);
}

static void sym_init_intrin(Sym *sym) {
  sym->type        = SYM_INTRIN;
  sym->name.len    = (Ind)strlen(sym->name.buf);
  sym->clobber     = ASM_REGS_VOLATILE;
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

static Err err_inline_not_leaf(const Sym *sym) {
  return errf(
    "unable to inline word " FMT_QUOTED ": not a leaf function", sym->name.buf
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
  if (!is_sym_leaf(sym)) return err_inline_not_leaf(sym);

  // Same as `err_inline_pc_rel`. Redundant check for safety.
  const auto spans = &sym->norm.spans;
  IF_DEBUG(try_assert(sym->norm.has_loads == (spans->data < spans->ceil)));
  if (spans->data < spans->ceil) return err_inline_has_data(sym);

  return nullptr;
}

static void sym_auto_inlinable(Sym *sym) {
  // SYNC[sym_inlinable].
  if (sym->type != SYM_NORM) return;
  if (sym->norm.has_loads) return;
  if (!is_sym_leaf(sym)) return;

  const auto spans = &sym->norm.spans;
  if (spans->data < spans->ceil) return;

  const auto len = spans->epi_err - spans->inner;
  if (len > ASM_INLINABLE_INSTR_LEN) return;

  sym->norm.inlinable = true;

  IF_DEBUG(
    eprintf("[system] symbol " FMT_QUOTED " is auto-inlinable\n", sym->name.buf)
  );
}

static const char *wordlist_name(Wordlist val) {
  switch (val) {
    case WORDLIST_EXEC: return "WORDLIST_EXEC";
    case WORDLIST_COMP: return "WORDLIST_COMP";
    default:            return "unknown wordlist";
  }
}

static void sym_register_call(Sym *caller, Sym *callee) {
  set_add(&caller->callees, callee);
  set_add(&callee->callers, caller);
}
