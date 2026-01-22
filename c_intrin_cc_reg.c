#pragma once
#include "./c_interp_internal.c"
#include "lib/fmt.h"

/*
The braces immediately following `: <name>` are used for parameters,
while the braces anywhere else are used for assignments.
*/
static Err interp_parse_params(Interp *interp) {
  const auto read = interp->reader;

  try(read_word(read));
  const auto word = read->word;

  if (!str_eq(&word, "{")) {
    reader_backtrack_word(read);
    return nullptr;
  }

  const auto comp = &interp->comp;

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
    try(comp_add_output_param(comp, word, nullptr));
  }
  return nullptr;
}

static Err intrin_brace(Interp *interp) {
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

static Err interp_validate_data_ptr(Sint val) {
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

static Err interp_validate_data_len(Sint val) {
  if (val > 0 && val < (Sint)IND_MAX) return nullptr;
  return errf("invalid data length: " FMT_SINT, val);
}

static Err interp_valid_name(Sint buf, Sint len, Word_str *out) {
  try(interp_validate_data_ptr(buf));
  try(interp_validate_data_len(len));
  try(valid_word((const char *)buf, (Ind)len, out));
  return nullptr;
}

static Err intrin_colon(Interp *interp) {
  try(interp_begin_definition(interp));
  try(interp_word_begin(interp, WORDLIST_EXEC, interp->reader->word));
  try(interp_parse_params(interp));
  return nullptr;
}

static Err intrin_colon_colon(Interp *interp) {
  try(interp_begin_definition(interp));
  try(interp_word_begin(interp, WORDLIST_COMP, interp->reader->word));
  try(interp_parse_params(interp));
  return nullptr;
}

static Err intrin_colon_named(Sint buf, Sint len, Interp *interp) {
  Word_str name;
  try(interp_valid_name(buf, len, &name));
  try(interp_word_begin(interp, WORDLIST_EXEC, name));
  return nullptr;
}

static Err intrin_colon_colon_named(Sint buf, Sint len, Interp *interp) {
  Word_str name;
  try(interp_valid_name(buf, len, &name));
  try(interp_word_begin(interp, WORDLIST_COMP, name));
  return nullptr;
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

static Err intrin_comp_load(Sint imm, Sint reg, Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(asm_validate_reg(reg));

  bool has_load = true;
  asm_append_imm_to_reg(&interp->comp, (U8)reg, imm, &has_load);
  if (has_load) sym->norm.has_loads = true;
  return nullptr;
}

static Err intrin_alloc_data(Sint buf, Sint len, Interp *interp) {
  if (buf) {
    try(interp_validate_data_ptr(buf));
  }
  try(interp_validate_data_len(len));

  const U8 *adr;
  try(comp_alloc_data(&interp->comp, (const U8 *)buf, (Ind)len, &adr));
  try(int_stack_push(&interp->ints, (Sint)adr));
  return nullptr;
}

static Err intrin_comp_page_addr(Sint adr, Sint reg, Interp *interp) {
  try(interp_validate_data_ptr(adr));
  try(asm_validate_reg(reg));
  try(comp_append_page_addr(&interp->comp, (Uint)adr, (U8)reg));
  return nullptr;
}

static Err intrin_comp_page_load(Sint adr, Sint reg, Interp *interp) {
  try(interp_validate_data_ptr(adr));
  try(asm_validate_reg(reg));
  try(comp_append_page_load(&interp->comp, (Uint)adr, (U8)reg));
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
  try(interp_validate_data_ptr(buf));
  try(interp_validate_data_len(len));

  const auto path = (const char *)buf;
  if (DEBUG) aver((Sint)strlen(path) == len);
  try(interp_import(interp, path)) return nullptr;
}

// See comment on `interp_extern_got` for explanation.
static Err intrin_extern_got(Sint name, Sint len, Interp *interp) {
  try(interp_validate_data_ptr(name));
  try(interp_validate_data_len(len));
  try(interp_extern_got(interp, (const char *)name, (Ind)len));
  return nullptr;
}

static Err intrin_extern_proc(Sint inp_len, Sint out_len, Interp *interp) {
  try(interp_extern_proc(interp, inp_len, out_len));
  return nullptr;
}

static Err intrin_find_word(Sint buf, Sint len, Sint wordlist, Interp *interp) {
  try(interp_validate_data_ptr(buf));
  try(interp_validate_data_len(len));
  try(interp_find_word(interp, (const char *)buf, (Ind)len, (Wordlist)wordlist));
  return nullptr;
}

static Err intrin_inline_word(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(comp_inline_sym(&interp->comp, sym));
  return nullptr;
}

/*
TODO: convert to compile-time with the following semantics:
- Verify that at least one argument is available.
- Compile `blr <reg>` where `<reg>` is the latest arg.
- Compile a "try" (check the error reg).
- Assume no output and reset args to 0.
*/
static Err intrin_execute(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(interp_call_sym(interp, sym));
  return nullptr;
}

/*
Used by "compiling" words to tell the compiler about the
input and output parameter counts. The compiler normally
infers the counts when parsing `{}` signatures which are
not available when meta-programming.
*/
static Err intrin_comp_word_sig(Sint inp_len, Sint out_len, Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(arch_validate_input_param_reg(inp_len));
  try(arch_validate_output_param_reg(inp_len));
  sym->inp_len = (U8)inp_len;
  sym->out_len = (U8)out_len;
  return nullptr;
}

/*
This is slightly abstract: an "arg" may eventually become either an input
or an output. This assumes that we can decide the reg immediately, which
holds on _some_ architectures under _some_ assumptions, but does not hold
in the general case. See the comments in `./c_comp_cc_reg.c`.
*/
static Err intrin_comp_next_arg(Interp *interp) {
  try(comp_next_valid_arg_reg(&interp->comp, nullptr));
  return nullptr;
}

static Err intrin_comp_next_arg_reg(Interp *interp) {
  U8 reg;
  try(comp_next_valid_arg_reg(&interp->comp, &reg));
  try(int_stack_push(&interp->ints, reg));
  return nullptr;
}

static Err intrin_get_local(Sint buf, Sint len, Interp *interp) {
  Local *loc;
  try(interp_validate_data_ptr(buf));
  try(interp_validate_data_len(len));
  try(interp_get_local(interp, (const char *)buf, (Ind)len, &loc));

  const auto tok = local_token(loc);
  try(int_stack_push(&interp->ints, (Sint)tok));
  return nullptr;
}

static Err intrin_anon_local(Interp *interp) {
  const auto comp = &interp->comp;
  const auto loc  = comp_local_anon(comp);
  const auto tok  = local_token(loc);
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
    "[debug]   arguments:          %d\n"
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
    const auto coll = &ctx->asm_fix;
    const auto len  = stack_len(coll);
    if (len) {
      eprintf("[debug]   asm fixups (" FMT_SINT "):\n", len);
      for (stack_range(auto, val, coll)) {
        eprintf("[debug]     %s\n", asm_fixup_fmt(val));
      }
    }
  }

  {
    const auto coll = &ctx->loc_fix;
    const auto len  = stack_len(coll);
    if (len) {
      eprintf("[debug]   local fixups (" FMT_SINT "):\n", len);
      for (stack_range(auto, val, coll)) {
        eprintf("[debug]     %s\n", loc_fixup_fmt(val));
      }
    }
  }

  IF_DEBUG(repr_struct(ctx));
}

static void intrin_debug_arg(Sint val, Interp *) {
  eprintf(
    "[debug] arg: " FMT_SINT " " FMT_UINT_HEX " %s\n",
    val,
    (Uint)val,
    uint64_to_bit_str((Uint)val)
  );
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

static constexpr USED auto INTRIN_COLON_NAMED = (Sym){
  .name.buf = "colon",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_colon_named,
  .inp_len  = 2,
  .throws   = true,
};

static constexpr USED auto INTRIN_COLON_COLON_NAMED = (Sym){
  .name.buf = "colon_colon",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_colon_colon_named,
  .inp_len  = 2,
  .throws   = true,
};

static constexpr USED auto INTRIN_BRACE = (Sym){
  .name.buf  = "{",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_brace,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_NEXT_ARG = (Sym){
  .name.buf  = "comp_next_arg",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_next_arg,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_NEXT_ARG_REG = (Sym){
  .name.buf  = "comp_next_arg_reg",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_next_arg_reg,
  .out_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_WORD_SIG = (Sym){
  .name.buf  = "comp_word_sig",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_word_sig,
  .inp_len   = 2,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_CLOBBER_BARRIER = (Sym){
  .name.buf  = "comp_clobber_barrier",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_clobber_barrier,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_LOCAL_GET = (Sym){
  .name.buf  = "comp_local_get",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_local_get,
  .inp_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_LOCAL_SET = (Sym){
  .name.buf  = "comp_local_set",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_local_set,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_DEBUG_CTX = (Sym){
  .name.buf  = "debug_ctx",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_debug_ctx,
  .comp_only = true,
};

static constexpr USED auto INTRIN_DEBUG_CTX_IMM = (Sym){
  .name.buf  = "#debug_ctx",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_debug_ctx,
  .comp_only = true,
};

static constexpr USED auto INTRIN_DEBUG_ARG = (Sym){
  .name.buf = "debug_arg",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_debug_arg,
  .inp_len  = 1,
};
