#pragma once
#include "./c_interp_internal.c"

// #ifdef CLANGD
// #include "./c_intrin.c"
// #endif

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
    try(comp_append_local_set_next(comp, loc));
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
  comp_clear_args(comp);
  return nullptr;
}

static Err intrin_recur(Interp *interp) {
  const auto comp = &interp->comp;
  try(comp_validate_recur_args(comp));
  try(comp_clobber_regs(comp, ARCH_VOLATILE_REGS));
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
  try(arch_validate_reg(reg));

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
  try(arch_validate_reg(reg));
  try(comp_append_page_addr(&interp->comp, (Uint)adr, (U8)reg));
  return nullptr;
}

static Err intrin_comp_page_load(Sint adr, Sint reg, Interp *interp) {
  try(interp_validate_data_ptr(adr));
  try(arch_validate_reg(reg));
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
  IF_DEBUG(aver((Sint)strlen(path) == len));
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
static Err intrin_comp_signature(Sint inp_len, Sint out_len, Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(arch_validate_input_param_reg(inp_len));
  try(arch_validate_output_param_reg(inp_len));
  sym->inp_len = (U8)inp_len;
  sym->out_len = (U8)out_len;
  return nullptr;
}

static Err intrin_comp_args_valid(Sint action, Sint args, Interp *interp) {
  try(interp_validate_data_ptr(action));
  try(comp_validate_args(&interp->comp, (const char *)action, args));
  return nullptr;
}

static Err intrin_comp_args_get(Interp *interp) {
  try(int_stack_push(&interp->ints, interp->comp.ctx.arg_len));
  return nullptr;
}

static Err intrin_comp_args_set(Sint len, Interp *interp) {
  try(arch_validate_input_param_reg(len));
  interp->comp.ctx.arg_len = (U8)len;
  return nullptr;
}

/*
Used by words which compile other words. For example, "var" uses this to load
its data address into the next argument register, playing nicely with our
on-the-fly register allocation.
*/
static Err intrin_comp_next_arg_reg(Interp *interp) {
  U8 reg;
  try(comp_next_arg_reg(&interp->comp, &reg));
  try(int_stack_push(&interp->ints, reg));
  return nullptr;
}

static Err intrin_comp_scratch_reg(Interp *interp) {
  U8 reg;
  try(comp_scratch_reg(&interp->comp, &reg));
  try(int_stack_push(&interp->ints, reg));
  return nullptr;
}

/*
Control constructs such as counted loops use this to emit a barrier AFTER
initializing their own locals or otherwise using the available arguments.
This validates that there are no unused arguments, and clobbers each temp
register-local association. Might eventually get a more general mechanism.
*/
static Err intrin_comp_barrier(Interp *interp) {
  try(comp_barrier(&interp->comp));
  return nullptr;
}

static Err err_invalid_clobber_mask(Sint bits) {
  return errf("invalid clobber mask %s", uint64_to_bit_str((Uint)bits));
}

static Err intrin_comp_clobber(Sint bits, Interp *interp) {
  const auto rem = bits_del_all((Uint)bits, ARCH_VOLATILE_REGS);
  if (rem) return err_invalid_clobber_mask(bits);
  return comp_clobber_regs(&interp->comp, (Bits)bits);
}

static Err intrin_comp_named_local(Sint buf, Sint len, Interp *interp) {
  Local *loc;
  try(interp_validate_data_ptr(buf));
  try(interp_validate_data_len(len));
  try(interp_get_local(interp, (const char *)buf, (Ind)len, &loc));

  const auto tok = local_token(loc);
  try(int_stack_push(&interp->ints, (Sint)tok));
  return nullptr;
}

static Err intrin_comp_anon_local(Interp *interp) {
  const auto comp = &interp->comp;
  const auto loc  = comp_local_anon(comp);
  const auto tok  = local_token(loc);
  try(int_stack_push(&interp->ints, (Sint)tok));
  return nullptr;
}

static Err intrin_comp_local_free(Sint ptr, Interp *interp) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(comp_validate_local(comp, ptr, &loc));

  IF_DEBUG(eprintf(
    "[system] freeing local " FMT_QUOTED
    "; prior temp registers: %s; value is stable: %d",
    loc->name.buf,
    comp_local_fmt_reg_bits(comp, loc),
    loc->stable
  ));

  comp_local_regs_clear(comp, loc);
  return nullptr;
}

static Err intrin_comp_local_get(Sint reg, Sint loc_ptr, Interp *interp) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(arch_validate_reg(reg));
  try(comp_validate_local(comp, loc_ptr, &loc));
  try(comp_append_local_get(comp, loc, (U8)reg));
  return nullptr;
}

static Err intrin_comp_local_set(Sint reg, Sint loc_ptr, Interp *interp) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(arch_validate_reg(reg));
  try(comp_validate_local(comp, loc_ptr, &loc));
  try(comp_append_local_set(comp, loc, (U8)reg));
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
    "[debug]   arguments consumed: %d\n"
    "[debug]   data stack len:     " FMT_SINT "\n",
    name,
    sym,
    inp_len,
    out_len,
    ctx->arg_len,
    ctx->arg_low,
    stack_len(&interp->ints)
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

        const auto name = loc->name.len ? loc->name.buf : "(anonymous)";
        eprintf("[debug]     %d -- %s\n", ind, name);
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

static Err debug_mem(Sint adr, Interp *interp) {
  debug_mem_at((const Uint *)adr);
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

static constexpr USED auto INTRIN_COMP_SIGNATURE = (Sym){
  .name.buf  = "comp_signature",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_signature,
  .inp_len   = 2,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_ARGS_VALID = (Sym){
  .name.buf  = "comp_args_valid",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_args_valid,
  .inp_len   = 2,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_ARGS_GET = (Sym){
  .name.buf  = "comp_args_get",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_args_get,
  .out_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_ARGS_SET = (Sym){
  .name.buf  = "comp_args_set",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_args_set,
  .inp_len   = 1,
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

static constexpr USED auto INTRIN_COMP_SCRATCH_REG = (Sym){
  .name.buf  = "comp_scratch_reg",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_scratch_reg,
  .out_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_BARRIER = (Sym){
  .name.buf  = "comp_barrier",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_barrier,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_CLOBBER = (Sym){
  .name.buf  = "comp_clobber",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_clobber,
  .inp_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_LOCAL_GET = (Sym){
  .name.buf  = "comp_local_get",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_local_get,
  .inp_len   = 2,
  .throws    = true,
  .comp_only = true,
};

static constexpr USED auto INTRIN_COMP_LOCAL_SET = (Sym){
  .name.buf  = "comp_local_set",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_local_set,
  .inp_len   = 2,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_COMP_LOCAL_FREE = (Sym){
  .name.buf  = "comp_local_free",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_local_free,
  .inp_len   = 1,
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
