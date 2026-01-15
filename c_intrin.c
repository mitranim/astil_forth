#pragma once
#include "./c_asm.c"
#include "./c_interp.h"
#include "./c_read.c"
#include "./c_read.h"
#include "./c_sym.c"
#include "./lib/dict.c"
#include "./lib/err.h"
#include "./lib/fmt.c"
#include "./lib/misc.h"
#include "./lib/stack.h"
#include "./lib/str.c"
#include "lib/stack.c"
#include <dlfcn.h>
#include <string.h>

/*
Intrinsic procedures necessary for "bootstrapping" the Forth system.
Should be kept to a minimum. Non-fundamental words should be written
in Forth; where regular language is insufficient, we use inline asm.

Intrinsics don't even provide conditionals or arithmetic.
*/

// TODO: avoid forward declarations.
Err interp_import(Interp *, const char *);
Err interp_call_sym(Interp *, const Sym *);
Err compile_call_sym(Interp *, Sym *);

static Err err_undefined_word(const char *name) {
  return errf("undefined word " FMT_QUOTED, name);
}

static Err interp_read_word(Interp *interp) {
  const auto read = interp->reader;
  try(read_skip_whitespace(read));
  try(read_word(read));
  IF_DEBUG(eprintf("[system] read word: " FMT_QUOTED "\n", read->word.buf));
  return nullptr;
}

static Err interp_read_sym(Interp *interp, Sym **out) {
  try(interp_read_word(interp));

  const auto name = interp->reader->word.buf;
  const auto sym  = dict_get(&interp->words, name);

  if (!sym) return err_undefined_word(name);
  if (out) *out = sym;
  return nullptr;
}

static Err err_current_sym_not_defining() {
  return err_str("unable to find current word: not inside a colon definition");
}

static Err current_sym(const Interp *interp, Sym **out) {
  const auto sym = interp->defining;
  if (!sym) return err_current_sym_not_defining();
  if (out) *out = sym;
  return nullptr;
}

static Err err_nested_definition(const Interp *interp) {
  Sym *sym;
  try(current_sym(interp, &sym));
  return errf("unexpected \":\" in definition of " FMT_QUOTED, sym->name.buf);
}

static Err err_wordlist_at_capacity(const char *name) {
  return errf("unable to create word " FMT_QUOTED ": wordlist at capacity", name);
}

static Err intrin_colon(Interp *interp) {
  if (interp->defining) return err_nested_definition(interp);

  const auto read = interp->reader;
  const auto asm  = &interp->asm;

  try(read_skip_whitespace(read));
  try(read_word(read));

  const auto name = read->word;
  IF_DEBUG(eprintf("[system] read word name: " FMT_QUOTED "\n", name.buf));

  const auto syms = &interp->syms;
  if (!stack_rem(syms)) return err_wordlist_at_capacity(name.buf);

  /*
  Add to the symbol list, but not to the symbol dict.
  During the colon, the new word is not yet visible.
  */
  const auto sym = stack_push(
    syms,
    (Sym){
      .type      = SYM_NORM,
      .name      = name,
      .immediate = is_name_immediate(&name),
    }
  );
  asm_sym_beg(asm, sym);

  interp->defining  = sym;
  interp->compiling = true;
  return nullptr;
}

static Err err_semi_not_compiling() {
  return err_str("unsupported use of \";\" when not compiling");
}

static Err intrin_semicolon(Interp *interp) {
  const auto asm = &interp->asm;

  Sym *sym;
  try(current_sym(interp, &sym));
  if (!interp->compiling) return err_semi_not_compiling();
  try(asm_sym_end(asm, sym));

  const auto dict = &interp->words;
  if (dict_has(dict, sym->name.buf) && !interp->redefining) {
    eprintf("[system] redefined word " FMT_QUOTED "\n", sym->name.buf);
  }
  dict_set(dict, sym->name.buf, sym);

  /*
  Prints instructions as hexpairs in the system's endian order,
  which is usually little endian. How to disassemble:

    echo <hex> | llvm-mc --disassemble --hex
  */
  IF_DEBUG({
    eprintf("[system] compiled word: " FMT_QUOTED "\n", sym->name.buf);
    asm_debug_print_sym_instrs(asm, sym, "[system]   ");

    // const auto read = interp->reader;
    // eprintf(
    //   "[system] [reader] position " FMT_UINT "; %s:" FMT_UINT ":" FMT_UINT "\n",
    //   READ_POS_ARGS(read)
    // );
  });

  interp->asm_snap   = interp->asm;
  interp->defining   = nullptr;
  interp->compiling  = false;
  interp->redefining = false;

  // const auto ints = &interp->ints;
  // if (stack_len(ints)) {
  //   eprintf(
  //     "[system] warning: " FMT_SINT " stack items after defining " FMT_QUOTED
  //     "; this usually indicates a bug\n",
  //     stack_len(ints),
  //     sym->name.buf
  //   );
  // }
  return nullptr;
}

static void intrin_bracket_beg(Interp *interp) { interp->compiling = false; }

static void intrin_bracket_end(Interp *interp) { interp->compiling = true; }

static Err intrin_immediate(Interp *interp) {
  Sym *sym;
  try(current_sym(interp, &sym));
  sym->immediate = true;
  return nullptr;
}

static Err intrin_comp_only(Interp *interp) {
  Sym *sym;
  try(current_sym(interp, &sym));
  sym->comp_only = true;
  return nullptr;
}

static Err intrin_not_comp_only(Interp *interp) {
  Sym *sym;
  try(current_sym(interp, &sym));
  sym->comp_only = false;
  return nullptr;
}

static Err intrin_inline(Interp *interp) {
  Sym *sym;
  try(current_sym(interp, &sym));
  sym->norm.inlinable = true;
  return nullptr;
}

static Err intrin_throws(Interp *interp) {
  Sym *sym;
  try(current_sym(interp, &sym));
  sym->throws = true;
  return nullptr;
}

static Err intrin_redefine(Interp *interp) {
  try(current_sym(interp, nullptr));
  interp->redefining = true;
  return nullptr;
}

/*
Caution: unlike in other Forth systems, `here` is not an executable address.
Executable code is copied to a different memory page when finalizing a word.
*/
static void intrin_here(Interp *interp) {
  stack_push(&interp->ints, (Sint)(asm_next_writable_instr(&interp->asm)));
}

static Err intrin_comp_instr(Interp *interp) {
  Sint val;
  try(int_stack_pop(&interp->ints, &val));
  try(asm_append_instr_from_int(&interp->asm, val));
  return nullptr;
}

static Err intrin_comp_load(Interp *interp) {
  Sym *sym;
  try(current_sym(interp, &sym));

  Sint imm;
  Sint reg;
  try(int_stack_pop(&interp->ints, &imm));
  try(int_stack_pop(&interp->ints, &reg));

  aver(reg >= 0 && reg < ASM_REG_LEN);

  bool has_load = true;
  asm_append_imm_to_reg(&interp->asm, (U8)reg, imm, &has_load);
  if (has_load) sym->norm.has_loads = true;
  return nullptr;
}

static Err interp_validate_buf_len(Sint len, Ind *out) {
  if (len > 0 && len < (Sint)IND_MAX) {
    *out = (Ind)len;
    return nullptr;
  }
  return errf("invalid data length: " FMT_SINT, len);
}

static Err interp_validate_buf_ptr(Sint ptr, const U8 **out) {
  /*
  Some systems deliberately ensure that virtual memory addresses
  are just out of range of `int32`. Would be nice to check if the
  pointer is somewhere in that range, but the assumption would be
  really non-portable.

  In a Forth implementation for OS-free microchips with a very small
  amount of real memory, valid data pointers could begin close to 0,
  depending on how data is organized. Since our implementation only
  supports 64-bit machines and requires an OS, we can at least assume
  addresses which are "decently large".
  */
  if (ptr > (1 << 14)) {
    *out = (U8 *)ptr;
    return nullptr;
  }
  return errf("suspiciously invalid-looking data pointer: %p", (U8 *)ptr);
}

// In Forth, like in C, data buffers are passed around as `(ptr, len)`.
static Err interp_pop_buf(Interp *interp, const U8 **out_buf, Ind *out_len) {
  const auto ints = &interp->ints;
  Sint       len_src;
  Sint       buf_src;

  try(int_stack_pop(ints, &len_src));
  try(int_stack_pop(ints, &buf_src));
  try(interp_validate_buf_len(len_src, out_len));
  try(interp_validate_buf_ptr(buf_src, out_buf));
  return nullptr;
}

/*
Stores constant data of arbitrary length, namely a string,
into the constant region, and compiles `( -- addr size )`.
*/
static Err intrin_comp_const(Interp *interp) {
  const auto ints = &interp->ints;

  Sint      reg;
  const U8 *buf;
  Ind       len;
  try(int_stack_pop(ints, &reg));
  try(asm_validate_reg(reg));
  try(interp_pop_buf(interp, &buf, &len));
  try(asm_alloc_const_append_load(&interp->asm, buf, len, (U8)reg));

  IF_DEBUG(eprintf(
    "[system] appended constant with address %p and length " FMT_UINT "\n",
    buf,
    (Uint)len
  ));

  Sym *sym;
  try(current_sym(interp, &sym));
  sym->norm.has_loads = true;
  return nullptr;
}

// ( C: size register -- addr )
static Err intrin_comp_static(Interp *interp) {
  const auto ints = &interp->ints;

  Sint reg;
  Sint len_src;
  Ind  len;
  try(int_stack_pop(ints, &reg));
  try(int_stack_pop(ints, &len_src));
  try(asm_validate_reg(reg));
  try(interp_validate_buf_len(len_src, &len));

  U8 *addr;
  try(asm_alloc_data_append_load(&interp->asm, len, (U8)reg, &addr));
  try(int_stack_push(&interp->ints, (Sint)addr));

  Sym *sym;
  try(current_sym(interp, &sym));
  sym->norm.has_loads = true;
  return nullptr;
}

static Err err_sym_out_bounds(const Sym *floor, const Sym *ceil, const Sym *val) {
  return errf(
    "expected a word address between %p and %p; got an out of bounds value: %p",
    floor,
    ceil,
    val
  );
}

static Err interp_validate_sym_ptr(Interp *interp, Sym *sym) {
  const auto syms = &interp->syms;
  if (sym >= syms->floor && sym < syms->ceil) return nullptr;
  return err_sym_out_bounds(syms->floor, syms->ceil, sym);
}

static Err interp_sym_by_ptr(Interp *interp, Sym **out) {
  Sint val;
  try(int_stack_pop(&interp->ints, &val));

  const auto sym = (Sym *)val;
  try(interp_validate_sym_ptr(interp, sym));

  IF_DEBUG(eprintf(
    "[system] found address of symbol " FMT_QUOTED ": %p\n", sym->name.buf, sym
  ));

  if (out) *out = sym;
  return nullptr;
}

static Err intrin_comp_call(Interp *interp) {
  Sint val;
  try(int_stack_pop(&interp->ints, &val));

  const auto sym = (Sym *)val;
  try(interp_validate_sym_ptr(interp, sym));

  IF_DEBUG(eprintf(
    "[system] found address of symbol " FMT_QUOTED ": %p\n", sym->name.buf, sym
  ));
  return compile_call_sym(interp, sym);
}

static const Err ERR_QUIT = "quit";

static Err intrin_quit(Interp *interp) {
  if (interp->defining) {
    return err_str("can't quit while defining a word");
  }
  return ERR_QUIT;
}

static Err intrin_ret(Interp *interp) {
  Sym *sym;
  try(current_sym(interp, &sym));
  sym->norm.has_rets = true;
  asm_append_ret(&interp->asm);
  return nullptr;
}

static Err intrin_recur(Interp *interp) {
  Sym *sym;
  try(current_sym(interp, &sym));
  set_add(&sym->callees, sym);
  set_add(&sym->callers, sym);
  asm_append_recur(&interp->asm);
  return nullptr;
}

static Err err_char_eof() {
  return err_str("EOF where a character was expected");
}

static Err intrin_char(Interp *interp) {
  const auto read = interp->reader;
  U8         byte;

  try(read_ascii_printable(read, &byte));
  if (!byte) return err_char_eof();
  try(int_stack_push(&interp->ints, byte));
  return nullptr;
}

// TODO: not fundamental, implement in Forth via intrinsic `char`.
static Err intrin_parse(Interp *interp) {
  const auto read = interp->reader;
  const auto ints = &interp->ints;

  // IF_DEBUG(eprintf(
  //   "[system] stack length before pop delim: " FMT_UINT "\n", stack_len(ints)
  // ));

  Sint delim;
  try(int_stack_pop(ints, &delim));

  // IF_DEBUG(eprintf(
  //   "[system] stack_len(ints) before parsing: " FMT_SINT "\n", stack_len(ints)
  // ));

  try(validate_char_ascii_printable(delim));
  try(read_skip_whitespace(read));
  try(read_until(read, (U8)delim));

  // IF_DEBUG(eprintf(
  //   "[system] parsed until delim " FMT_CHAR "; length: " FMT_UINT
  //   "; string: %s\n",
  //   (int)delim,
  //   read->buf.len,
  //   read->buf.buf
  // ));

  try(int_stack_push(ints, (Sint)read->buf.buf));
  try(int_stack_push(ints, (Sint)read->buf.len));

  // IF_DEBUG({
  //   eprintf(
  //     "[system] stack_len(ints) after parsing: " FMT_SINT "\n", stack_len(ints)
  //   );
  //   eprintf("[system] stack pointer after parsing: %p\n", ints->top);
  // });
  return nullptr;
}

// Technically not fundamental. TODO implement in Forth via intrinsic `char`.
static Err intrin_parse_word(Interp *interp) {
  try(interp_read_word(interp));

  const auto word = &interp->reader->word;
  const auto ints = &interp->ints;

  try(int_stack_push(ints, (Sint)word->buf));
  try(int_stack_push(ints, (Sint)word->len));
  return nullptr;
}

static Err intrin_import(Interp *interp) {
  const auto ints = &interp->ints;
  Sint       path;

  try(int_stack_pop(ints, nullptr)); // discard length
  try(int_stack_pop(ints, &path));
  try(interp_import(interp, (const char *)path));
  return nullptr;
}

static Err intrin_import_quote(Interp *interp) {
  const auto read = interp->reader;
  try(read_skip_whitespace(read));
  try(read_until(read, '"'));
  try(interp_import(interp, (const char *)read->buf.buf));
  return nullptr;
}

static Err intrin_import_tick(Interp *interp) {
  const auto read = interp->reader;
  try(read_skip_whitespace(read));
  try(read_word(read));
  try(interp_import(interp, (const char *)read->word.buf));
  return nullptr;
}

static Err intrin_read_extern(Interp *interp, void **out_addr) {
  try(interp_read_word(interp));
  const auto name = interp->reader->word.buf;

  IF_DEBUG(
    eprintf("[system] read name of extern symbol: " FMT_QUOTED "\n", name)
  );

  const auto addr = dlsym(RTLD_DEFAULT, name);

  if (!addr) {
    return errf(
      "unable to find extern symbol " FMT_QUOTED "; error: %s", name, dlerror()
    );
  }

  IF_DEBUG(eprintf(
    "[system] found address of extern symbol " FMT_QUOTED ": %p\n", name, addr
  ));

  if (out_addr) *out_addr = addr;
  return nullptr;
}

static Err intrin_extern_ptr(Interp *interp) {
  void *addr;
  try(intrin_read_extern(interp, &addr));

  const auto sym = stack_push(
    &interp->syms,
    (Sym){
      .type         = SYM_EXT_PTR,
      .name         = interp->reader->word,
      .ext_ptr.addr = addr,
    }
  );

  dict_set(&interp->words, sym->name.buf, sym);
  asm_register_dysym(&interp->asm, sym->name.buf, (U64)addr);
  return nullptr;
}

static Err intrin_extern_proc(Interp *interp) {
  void *addr;
  try(intrin_read_extern(interp, &addr));

  const auto ints = &interp->ints;
  Sint       out_len;
  Sint       inp_len;
  try(int_stack_pop(ints, &out_len));
  try(int_stack_pop(ints, &inp_len));

  if (inp_len < 0) return err_str("negative input parameter count");
  if (inp_len > ASM_PARAM_REG_LEN) return err_str("too many input parameters");
  if (out_len < 0) return err_str("negative output parameter count");
  if (out_len > 1) return err_str("too many output parameters");

  const auto sym = stack_push(
    &interp->syms,
    (Sym){
      .type             = SYM_EXT_PROC,
      .name             = interp->reader->word,
      .ext_proc.fun     = addr,
      .ext_proc.inp_len = (U8)inp_len,
      .ext_proc.out_len = (U8)out_len,
    }
  );

  dict_set(&interp->words, sym->name.buf, sym);
  asm_register_dysym(&interp->asm, sym->name.buf, (U64)addr);
  return nullptr;
}

static Err interp_find_word(Interp *interp, Sym **out) {
  Sint len;
  Sint buf;
  try(int_stack_pop(&interp->ints, &len));
  try(int_stack_pop(&interp->ints, &buf));

  const auto name = (const char *)buf;
  if (DEBUG) aver((Sint)strlen(name) == len);

  const auto sym = dict_get(&interp->words, name);
  if (!sym) return err_undefined_word(name);

  IF_DEBUG(
    eprintf("[system] found symbol " FMT_QUOTED " at address %p\n", name, sym)
  );

  *out = sym;
  return nullptr;
}

static Err intrin_find_word(Interp *interp) {
  Sym *sym;
  try(interp_find_word(interp, &sym));
  return int_stack_push(&interp->ints, (Sint)sym);
}

static Err err_inline_not_defining() {
  return err_str("unsupported use of \"inline\" outside a colon definition");
}

static Err interp_inline_sym(Interp *interp, Sym *sym) {
  return asm_inline_sym(&interp->asm, sym);
}

static Err intrin_inline_word(Interp *interp) {
  if (!interp->defining) return err_inline_not_defining();
  Sym *sym;
  try(interp_sym_by_ptr(interp, &sym));
  return interp_inline_sym(interp, sym);
}

static Err intrin_execute(Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, &sym));
  return interp_call_sym(interp, sym);
}

/*
Allocates a slot on the system stack for a local variable.
Does not append any instructions; the caller is expected
to follow this up with `intrin_comp_local_pop` and / or
`intrin_comp_local_push`.
*/
static Err intrin_comp_local_ind(Interp *interp) {
  const U8 *buf;
  Ind       len;
  try(interp_pop_buf(interp, &buf, &len));
  const auto ind = asm_local_get_or_make(&interp->asm, (const char *)buf, len);
  try(int_stack_push(&interp->ints, ind));
  return nullptr;
}

// Appends instructions for moving a value from the Forth stack to the local.
static Err intrin_comp_local_pop(Interp *interp) {
  Sint ind;
  try(int_stack_pop(&interp->ints, &ind));
  try(asm_append_local_pop(&interp->asm, ind));
  return nullptr;
}

// Appends instructions for copying a value from the local to the Forth stack.
static Err intrin_comp_local_push(Interp *interp) {
  Sint ind;
  try(int_stack_pop(&interp->ints, &ind));
  try(asm_append_local_push(&interp->asm, ind));
  return nullptr;
}

static void debug_flush(void *) {
  fflush(stdout);
  fflush(stderr);
}

static Err debug_throw() { return "debug_throw"; }

static void debug_stack(Interp *interp) {
  const auto ints = &interp->ints;
  const auto len  = stack_len(ints);

  if (!len) {
    eprintf("[debug] stack is empty (%p)\n", ints->floor);
    return;
  }

  eprintf("[debug] stack (%p) (" FMT_SINT "):\n", ints->floor, len);

  for (stack_range(auto, ptr, ints)) {
    eprintf(
      "[debug]   %p -- " FMT_SINT " " FMT_UINT_HEX "\n", ptr, *ptr, (Uint)*ptr
    );
  }
}

static void debug_depth(Interp *interp) {
  eprintf("[debug] stack depth: " FMT_SINT "\n", stack_len(&interp->ints));
}

static void debug_top_int(Interp *interp) {
  eprintf("[debug] stack top int: " FMT_SINT "\n", stack_head(&interp->ints));
}

static void debug_top_ptr(Interp *interp) {
  const auto ints = &interp->ints;

  if (stack_len(ints)) {
    eprintf("[debug] stack top ptr: %p\n", (void *)stack_head(ints));
  }
  else {
    eputs("[debug] stack top ptr: none (empty)");
  }
}

static void debug_top_str(Interp *interp) {
  eprintf(
    "[debug] stack top string: %s\n", (const char *)stack_head(&interp->ints)
  );
}

static Err debug_mem(Interp *interp) {
  Sint val;
  try(int_stack_pop(&interp->ints, &val));

  auto ptr = (const Uint *)val;
  eprintf("[debug] memory at address %p:\n", ptr);
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 0, *(ptr + 0));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 1, *(ptr + 1));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 2, *(ptr + 2));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 3, *(ptr + 3));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 4, *(ptr + 4));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 5, *(ptr + 5));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 6, *(ptr + 6));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 7, *(ptr + 7));
  return nullptr;
}

static Err debug_word(Interp *interp) {
  Sym *sym;
  try(interp_read_sym(interp, &sym));

  const auto asm  = &interp->asm;
  const auto name = sym->name.buf;

  switch (sym->type) {
    case SYM_NORM: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name: %s\n"
        "[debug]   type: normal\n"
        "[debug]   execution token: %p\n",
        name,
        sym
      );
      asm_debug_print_sym_instrs(asm, sym, "[debug]   ");
      IF_DEBUG({
        eprintf("[debug]   symbol: ");
        repr_struct(sym);
      });
      return nullptr;
    }

    case SYM_INTRIN: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name: %s\n"
        "[debug]   type: intrinsic\n"
        "[debug]   execution token: %p\n"
        "[debug]   address: %p\n",
        name,
        sym,
        sym->intrin.fun
      );
      return nullptr;
    }

    case SYM_EXT_PTR: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name: %s\n"
        "[debug]   type: external symbol\n"
        "[debug]   execution token: %p\n"
        "[debug]   address: %p\n",
        name,
        sym,
        sym->ext_ptr.addr
      );
      return nullptr;
    }

    case SYM_EXT_PROC: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name: %s\n"
        "[debug]   type: external procedure\n"
        "[debug]   address: %p\n",
        name,
        sym->ext_proc.fun
      );
      return nullptr;
    }

    default: unreachable();
  }
}

static Err debug_sync_code(Interp *interp) {
  try(asm_code_sync(&interp->asm));
  eprintf("[debug] synced code heaps\n");
  return nullptr;
}

// The field `.name.len` must be set separately (CBA to maintain here).
static constexpr Sym USED INTRIN[] = {
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = ":",
    .intrin.fun  = (void *)intrin_colon,
    .throws      = true,
    .immediate   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = ";",
    .intrin.fun  = (void *)intrin_semicolon,
    .throws      = true,
    .immediate   = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "[",
    .intrin.fun  = (void *)intrin_bracket_beg,
    .immediate   = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "]",
    .intrin.fun  = (void *)intrin_bracket_end,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "immediate",
    .intrin.fun  = (void *)intrin_immediate,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "comp_only",
    .intrin.fun  = (void *)intrin_comp_only,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "not_comp_only",
    .intrin.fun  = (void *)intrin_not_comp_only,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "inline",
    .intrin.fun  = (void *)intrin_inline,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "throws",
    .intrin.fun  = (void *)intrin_throws,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "redefine",
    .intrin.fun  = (void *)intrin_redefine,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "here",
    .intrin.fun  = (void *)intrin_here,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "comp_instr",
    .intrin.fun  = (void *)intrin_comp_instr,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "comp_load",
    .intrin.fun  = (void *)intrin_comp_load,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "comp_const",
    .intrin.fun  = (void *)intrin_comp_const,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "comp_static",
    .intrin.fun  = (void *)intrin_comp_static,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "comp_call",
    .intrin.fun  = (void *)intrin_comp_call,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "quit",
    .intrin.fun  = (void *)intrin_quit,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "#ret",
    .intrin.fun  = (void *)intrin_ret,
    .throws      = true,
    .immediate   = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "#recur",
    .intrin.fun  = (void *)intrin_recur,
    .throws      = true,
    .immediate   = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "char",
    .intrin.fun  = (void *)intrin_char,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "parse",
    .intrin.fun  = (void *)intrin_parse,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "parse_word",
    .intrin.fun  = (void *)intrin_parse_word,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "import",
    .intrin.fun  = (void *)intrin_import,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "import\"",
    .intrin.fun  = (void *)intrin_import_quote,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "import'",
    .intrin.fun  = (void *)intrin_import_tick,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "extern_ptr:",
    .intrin.fun  = (void *)intrin_extern_ptr,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "extern:",
    .intrin.fun  = (void *)intrin_extern_proc,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "find_word",
    .intrin.fun  = (void *)intrin_find_word,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "inline_word",
    .intrin.fun  = (void *)intrin_inline_word,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "execute",
    .intrin.fun  = (void *)intrin_execute,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "comp_local_ind",
    .intrin.fun  = (void *)intrin_comp_local_ind,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "comp_local_pop",
    .intrin.fun  = (void *)intrin_comp_local_pop,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "comp_local_push",
    .intrin.fun  = (void *)intrin_comp_local_push,
    .throws      = true,
    .comp_only   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug_flush",
    .intrin.fun  = (void *)debug_flush,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug_throw",
    .intrin.fun  = (void *)debug_throw,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug_stack",
    .intrin.fun  = (void *)debug_stack,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug_depth",
    .intrin.fun  = (void *)debug_depth,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug_top_int",
    .intrin.fun  = (void *)debug_top_int,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug_top_ptr",
    .intrin.fun  = (void *)debug_top_ptr,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug_top_str",
    .intrin.fun  = (void *)debug_top_str,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug_mem",
    .intrin.fun  = (void *)debug_mem,
    .throws      = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug'",
    .intrin.fun  = (void *)debug_word,
    .throws      = true,
    .immediate   = true,
    .interp_only = true,
  },
  (Sym){
    .type        = SYM_INTRIN,
    .name.buf    = "debug_sync_code",
    .intrin.fun  = (void *)debug_sync_code,
    .throws      = true,
    .interp_only = true,
  },
};
