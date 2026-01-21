#pragma once
#include "./c_interp_internal.c"
#include "lib/fmt.h"

static Err interp_brace_assignment(Interp *interp) {
  const auto read = interp->reader;
  const auto comp = &interp->comp;

  for (;;) {
    try(read_word(read));
    const auto word = read->word;

    if (str_eq(&word, "}")) return nullptr;
    if (str_eq(&word, "--")) break;

    Local *loc;
    try(comp_local_get_or_make(comp, word, &loc));
    try(comp_append_local_set(comp, loc));
  }

  // Treat the remaining arguments as consumed, and the remaining words after
  // `--` as comments. This allows to use `--` to discard unused values, and
  // makes it syntactically consistent with parameter lists.
  comp->ctx.arg_low = comp->ctx.arg_len = 0;
  for (;;) {
    try(read_word(read));
    if (str_eq(&read->word, "}")) return nullptr;
  }
  return nullptr;
}

static Err interp_brace_params(Interp *interp) {
  const auto read     = interp->reader;
  const auto comp     = &interp->comp;
  comp->ctx.proc_body = true;

  for (;;) {
    try(read_word(read));
    const auto word = read->word;
    if (str_eq(&word, "}")) return nullptr;
    if (str_eq(&word, "--")) break;
    try(comp_add_input_param(comp, word));
  }

  for (;;) {
    try(read_word(read));
    const auto word = read->word;
    if (str_eq(&word, "}")) return nullptr;
    try(comp_add_output_param(comp, word));
  }
  return nullptr;
}

static Err intrin_brace(Interp *interp) {
  const auto comp = &interp->comp;
  if (!comp->ctx.proc_body) return interp_brace_params(interp);
  return interp_brace_assignment(interp);
}

static Err intrin_ret(Interp *interp) {
  const auto comp = &interp->comp;
  try(comp_validate_ret_args(comp));
  try(comp_append_ret(comp));
  return nullptr;
}

static Err intrin_recur(Interp *interp) {
  const auto comp = &interp->comp;
  try(comp_validate_recur_args(comp));
  try(comp_append_recur(comp));
  return nullptr;
}

static Err intrin_comp_instr(Instr instr, Interp *interp) {
  asm_append_instr(&interp->comp, instr);
  return nullptr;
}

static Err intrin_comp_load(Sint reg, Sint imm, Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(asm_validate_reg(reg));

  bool has_load = true;
  asm_append_imm_to_reg(&interp->comp, (U8)reg, imm, &has_load);
  if (has_load) sym->norm.has_loads = true;
  return nullptr;
}

static Err interp_validate_buf_ptr(Sint val) {
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
  if (val > (1 << 14)) return nullptr;
  return errf("suspiciously invalid-looking data pointer: %p", (U8 *)val);
}

static Err interp_validate_buf_len(Sint val) {
  if (val > 0 && val < (Sint)IND_MAX) return nullptr;
  return errf("invalid data length: " FMT_SINT, val);
}

// TODO: either provide a way to align, or auto-align to `sizeof(void*)`.
static Err intrin_comp_const(Sint buf, Sint len, Sint reg, Interp *interp) {
  try(interp_validate_buf_ptr(buf));
  try(interp_validate_buf_len(len));
  try(asm_validate_reg(reg));
  try(interp_comp_const(interp, (const U8 *)buf, (Ind)len, (U8)reg));
  return nullptr;
}

static Err intrin_comp_static(Sint len, Sint reg, Interp *interp) {
  try(interp_validate_buf_len(len));
  try(asm_validate_reg(reg));
  try(interp_comp_static(interp, (Ind)len, (U8)reg));
  return nullptr;
}

static Err interp_sym_by_ptr(Interp *interp, Sint ptr, Sym **out) {
  const auto sym = (Sym *)ptr;
  try(interp_validate_sym_ptr(interp, sym));

  IF_DEBUG(eprintf(
    "[system] found address of symbol " FMT_QUOTED ": %p\n", sym->name.buf, sym
  ));

  if (out) *out = sym;
  return nullptr;
}

static Err intrin_comp_call(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  return comp_append_call_sym(&interp->comp, sym);
}

static Err intrin_parse(Sint delim, Interp *interp) {
  try(validate_char_ascii_printable(delim));
  try(interp_parse_until(interp, (U8)delim));
  return nullptr;
}

static Err intrin_import(Sint buf, Sint len, Interp *interp) {
  try(interp_validate_buf_ptr(buf));
  try(interp_validate_buf_len(len));

  const auto path = (const char *)buf;
  if (DEBUG) aver((Sint)strlen(path) == len);
  try(interp_import(interp, path)) return nullptr;
}

static Err intrin_extern_proc(Sint out_len, Sint inp_len, Interp *interp) {
  try(interp_extern_proc(interp, inp_len, out_len));
  return nullptr;
}

static Err intrin_find_word(Sint buf, Sint len, Interp *interp) {
  try(interp_validate_buf_ptr(buf));
  try(interp_validate_buf_len(len));
  try(interp_find_word(interp, (const char *)buf, (Ind)len));
  return nullptr;
}

static Err intrin_inline_word(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(comp_inline_sym(&interp->comp, sym));
  return nullptr;
}

static Err intrin_execute(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(interp_call_sym(interp, sym));
  return nullptr;
}

static Err intrin_comp_next_inp_reg(Interp *interp) {
  U8 reg;
  try(comp_next_valid_arg_reg(&interp->comp, &reg));
  try(int_stack_push(&interp->ints, reg));
  return nullptr;
}

static Err intrin_comp_next_out_reg(Interp *interp) {
  // Warning: would need adjustment for the x64 arch.
  // See the comments in `./c_comp_cc_reg.c`.
  U8 reg;
  try(comp_next_valid_arg_reg(&interp->comp, &reg));
  try(int_stack_push(&interp->ints, reg));
  return nullptr;
}

static Err intrin_get_local(Sint buf, Sint len, Interp *interp) {
  Local *loc;
  try(interp_validate_buf_ptr(buf));
  try(interp_validate_buf_len(len));
  try(interp_get_local(interp, (const char *)buf, (Ind)len, &loc));

  const auto tok = local_token(loc);
  try(int_stack_push(&interp->ints, (Sint)tok));
  return nullptr;
}

// FIXME use in `f_lang_cc_reg.f` instead of custom asm.
static Err intrin_comp_local_get(Sint ptr, Interp *interp) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(comp_validate_local(comp, ptr, &loc));
  try(comp_append_local_get(comp, loc));
  return nullptr;
}

// FIXME use in `f_lang_cc_reg.f` instead of custom asm.
static Err intrin_comp_local_set(Sint ptr, Interp *interp) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(comp_validate_local(comp, ptr, &loc));
  try(comp_append_local_set(comp, loc));
  return nullptr;
}

static void intrin_debug_ctx(Interp *interp) {
  const auto ctx     = &interp->comp.ctx;
  const auto locs    = &ctx->locals;
  const auto sym     = ctx->sym;
  const auto name    = sym ? sym->name.buf : nullptr;
  const auto inp_len = sym ? sym->inp_len : 0;
  const auto out_len = sym ? sym->out_len : 0;
  const auto loc_len = stack_len(locs);

  eprintf(
    "[debug] compilation context:\n"
    "[debug]   current word:      " FMT_QUOTED
    " at %p\n"
    "[debug]   input param count:  %d\n"
    "[debug]   output param count: %d\n"
    "[debug]   arguments total:    %d\n"
    "[debug]   arguments consumed: %d\n",
    name,
    sym,
    inp_len,
    out_len,
    ctx->arg_len,
    ctx->arg_low
  );

  if (loc_len) {
    eprintf("[debug]   locals (" FMT_SINT "):", loc_len);
    for (stack_range(auto, loc, locs)) {
      eprintf(" %s", loc->name.buf);
    }
    fputc('\n', stderr);
  }

  {
    const auto arr = ctx->loc_regs;
    const auto cap = arr_cap(ctx->loc_regs);
    U8         len = 0;

    for (U8 ind = 0; ind < cap; ind++) {
      if (arr[ind]) len++;
    }

    if (len) {
      eprintf("[debug]   locals in param registers (%d):\n", len);

      for (U8 ind = 0; ind < cap; ind++) {
        const auto loc = arr[ind];
        if (!loc) continue;
        eprintf("[debug]     %d -- %s\n", ind, loc->name.buf);
      }
    }
  }

  {
    const auto fixup = &ctx->fixup;
    const auto len   = stack_len(fixup);
    if (len) {
      eprintf("[debug]   fixups (" FMT_SINT "):\n", len);
      for (stack_range(auto, fix, fixup)) {
        eprintf("[debug]     %s\n", comp_fixup_fmt(fix));
      }
    }
  }

  IF_DEBUG(repr_struct(ctx));
}

/*
Control constructs such as counted loops must use this to emit a barrier AFTER
initializing their own locals or otherwise using the available arguments.

  #if ->
    use x0 for condition check
    emit clobber barrier
    <body>

  +loop: ->
    create locals; use "set" with x0 x1 x2
    emit clobber barrier
    begin loop
    use "get" with locals
*/
static Err intrin_comp_clobber_barrier(Interp *interp) {
  const auto comp = &interp->comp;
  for (U8 reg = 0; reg < arr_cap(comp->ctx.loc_regs); reg++) {
    try(comp_clobber_reg(comp, reg));
  }
  return nullptr;
}

// The "missing" fields are set in `sym_init_intrin`.

static constexpr USED auto INTRIN_BRACE = (Sym){
  .name.buf  = "{",
  .intrin    = (void *)intrin_brace,
  .throws    = true,
  .immediate = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_NEXT_INP_REG = (Sym){
  .name.buf  = "comp_next_inp_reg",
  .intrin    = (void *)intrin_comp_next_inp_reg,
  .out_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_NEXT_OUT_REG = (Sym){
  .name.buf  = "comp_next_out_reg",
  .intrin    = (void *)intrin_comp_next_out_reg,
  .out_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_CLOBBER_BARRIER = (Sym){
  .name.buf  = "comp_clobber_barrier",
  .intrin    = (void *)intrin_comp_clobber_barrier,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_LOCAL_GET = (Sym){
  .name.buf  = "comp_local_get",
  .intrin    = (void *)intrin_comp_local_get,
  .inp_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_LOCAL_SET = (Sym){
  .name.buf  = "comp_local_set",
  .intrin    = (void *)intrin_comp_local_set,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_DEBUG_CTX = (Sym){
  .name.buf  = "#debug_ctx",
  .intrin    = (void *)intrin_debug_ctx,
  .immediate = true,
  .comp_only = true,
};
