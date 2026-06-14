#pragma once
#include "./interp_internal.c"
#include "./mach_o.c"

// #ifdef CLANGD
// #include "./intrin.c"
// #endif

static Err err_redundant_param_skip(const char *name) {
  return errf("in " FMT_QUOTED ": redundant `--` without output params", name);
}

/*
The braces immediately following `: <name>` are used for parameters,
while the braces anywhere else are used for assignments.
*/
static Err interp_parse_params(Interp *interp) {
  const auto read = interp_reader(interp);
  const auto beg  = read->pos;

  Word_str word;
  try(read_valid_word(read, &word));

  if (!str_eq(&word, "{")) {
    read->pos = beg;
    return nullptr;
  }

  const auto comp = &interp->comp;

  for (;;) {
    try(read_valid_word(read, &word));
    if (str_eq(&word, "}")) return nullptr;
    if (str_eq(&word, "--")) break;
    try(comp_add_input_param(comp, word));
  }

  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  bool has_err = false;

  for (;;) {
    try(read_valid_word(read, &word));
    if (str_eq(&word, "}")) break;
    try(comp_add_output_param(comp, word, nullptr));
    has_err = str_eq(&word, "err");
  }

  if (has_err) {
    Sym *sym;
    try(interp_require_current_sym(interp, &sym));
    sym->has_err = true;
  }

  if (!sym->out_len) return err_redundant_param_skip(sym->name.buf);
  return nullptr;
}

static Err intrin_brace(Interp *interp) {
  const auto read = interp_reader(interp);
  const auto comp = &interp->comp;
  const auto ctx  = &comp->ctx;

  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  for (;;) {
    Word_str word;
    try(read_valid_word(read, &word));

    if (str_eq(&word, "--")) goto discard;
    if (str_eq(&word, "}")) goto validate;

    Local *loc;
    try(comp_local_get_or_make(comp, word, &loc));
    try(comp_append_local_set_next(comp, loc));
  }

discard:
  Word_str word;
  try(read_valid_word(read, &word));

  if (str_eq(&word, "}")) {
    if (comp->ctx.arg_low <= comp->ctx.arg_len) {
      comp->ctx.arg_low = comp->ctx.arg_len = 0;
      return nullptr;
    }

    return err_str(
      "redundant `--` in assignment without any remaining arguments to skip"
    );
  }

  return err_str("`--` in assignment should be followed by `}`");

validate:
  const auto low = ctx->arg_low;
  if (!low) return nullptr;

  const auto len = ctx->arg_len;
  assert(low < len);

  return errf(
    "in " FMT_QUOTED
    ": arity mismatch: assigned %d of %d prior values; hint: either assign to more locals, or use `--` inside braces to discard unused values",
    sym->name.buf,
    low,
    len
  );
}

static Err interp_valid_name(Sint buf, Sint len, Word_str *out) {
  try(interp_validate_buf_len(buf, len));
  try(valid_word((const char *)buf, (Ind)len, out));
  return nullptr;
}

static Err intrin_colon(Interp *interp) {
  try(interp_begin_definition(interp));
  Word_str name;
  try(interp_read_word(interp, &name));
  try(interp_word_begin(interp, WORDLIST_EXEC, name));
  try(interp_parse_params(interp));
  return nullptr;
}

static Err intrin_colon_colon(Interp *interp) {
  try(interp_begin_definition(interp));
  Word_str name;
  try(interp_read_word(interp, &name));
  try(interp_word_begin(interp, WORDLIST_COMP, name));
  try(interp_parse_params(interp));
  return nullptr;
}

static Err intrin_fun(Interp *interp) {
  try(intrin_colon(interp));
  try(interp_push_semicolon(interp));
  return nullptr;
}

static Err intrin_fun_comp(Interp *interp) {
  try(intrin_colon_colon(interp));
  try(interp_push_semicolon(interp));
  return nullptr;
}

static Err intrin_define_fun(Sint buf, Sint len, Interp *interp) {
  Word_str name;
  try(interp_valid_name(buf, len, &name));
  try(interp_word_begin(interp, WORDLIST_EXEC, name));
  return nullptr;
}

static Err intrin_define_fun_comp(Sint buf, Sint len, Interp *interp) {
  Word_str name;
  try(interp_valid_name(buf, len, &name));
  try(interp_word_begin(interp, WORDLIST_COMP, name));
  return nullptr;
}

static Err intrin_ret(Interp *interp) {
  const auto comp = &interp->comp;
  try(comp_before_append_ret(comp));
  try(comp_append_ret(comp));
  comp_clear_args(comp);
  return nullptr;
}

static Err intrin_recur(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));

  const auto comp = &interp->comp;

  try(comp_validate_recur_args(comp));
  try(comp_clobber_regs(comp, ASM_REGS_VOLATILE));
  try(comp_append_recur(comp));
  comp_args_set(comp, sym->out_len);
  if (comp->ctx.try_all && sym->has_err) try(comp_append_try(comp, sym));

  sym->norm.has_recur = true;
  return nullptr;
}

static Err intrin_try(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(comp_append_try(&interp->comp, sym));
  return nullptr;
}

static Err intrin_throw(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(comp_append_throw(&interp->comp, sym));
  return nullptr;
}

static Err intrin_catch(Sint wordlist, Interp *interp) {
  try(interp_catch(interp, (Wordlist)wordlist));
  return nullptr;
}

static void intrin_try_all(bool val, Interp *interp) {
  if (interp->comp.ctx.sym) {
    interp->comp.ctx.try_all = val;
    return;
  }
  if (interp->module) interp->module->try_all = val;
}

static void intrin_get_try_all(Interp *interp, bool *out) {
  if (interp->comp.ctx.sym) *out = interp->comp.ctx.try_all;
  else *out = interp->module->try_all;
}

static Err intrin_comp_only(bool val, Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->comp_only = val;
  return nullptr;
}

static Err intrin_interp_only(bool val, Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->interp_only = val;
  return nullptr;
}

/*
Caution: unlike most Forth systems, because of Apple's W^X restrictions,
we use two code heaps: writable and executable. Control structures often
patch up instructions in the writable heap during word compilation. When
a word is finalized, by copying its instructions to the executable heap,
any further modifications of its range in the writable heap become nops.
*/
static Err intrin_here_write(Interp *interp, Instr **out) {
  if (!out) return nullptr;
  *out = comp_code_next_writable_instr(&interp->comp.code);
  return nullptr;
}

/*
Returns an address in the executable heap corresponding to the next instruction
in the writable heap during word compilation. The returned address does NOT yet
contain a valid instruction, but may be stashed for later use.
*/
static Err intrin_here_exec(Interp *interp, Instr **out) {
  if (!out) return nullptr;
  *out = comp_code_next_prog_counter(&interp->comp.code);
  return nullptr;
}

static Err intrin_comp_instr(Instr instr, Interp *interp) {
  asm_append_instr(&interp->comp, instr);
  return nullptr;
}

// SYNC[comp_constant].
static Err intrin_comp_load(Sint imm, Sint reg, Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(asm_validate_reg(reg));

  bool has_load = true;
  try(comp_append_imm_to_reg(&interp->comp, (U8)reg, imm, &has_load));
  if (has_load) sym->norm.has_loads = true;
  return nullptr;
}

static Err intrin_alloc_data(Sint buf, Sint len, Interp *interp, const U8 **adr) {
  if (buf) try(interp_validate_data_ptr(buf));
  try(interp_validate_data_len(len));
  try(comp_alloc_data(&interp->comp, (const U8 *)buf, (Ind)len, adr));
  return nullptr;
}

/*
TODO: drop `intrin_comp_page_addr` and `intrin_comp_page_load`.
Program code now has both `here_write` and `here_exec`, and can
calculate the offsets on its own.
*/
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

static Err intrin_comp_call(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  return comp_append_call_sym(&interp->comp, sym);
}

static Err intrin_read_char(Interp *interp, Sint *out) {
  char byte;
  try(interp_char(interp, &byte));
  if (out) *out = byte;
  return nullptr;
}

static Err intrin_read_until_char(
  Sint delim, Interp *interp, const char **buf, Sint *len
) {
  Ind len_tmp;
  try(validate_char_ascii_printable(delim));
  try(interp_parse_until(interp, (U8)delim, buf, &len_tmp));
  if (len) *len = len_tmp;
  return nullptr;
}

// Technically not fundamental. TODO implement in Forth via intrinsic `char`.
static Err intrin_read_word(Interp *interp, const char **buf, Sint *len) {
  Ind len_tmp;
  try(interp_parse_word(interp, buf, &len_tmp));
  if (len) *len = len_tmp;
  return nullptr;
}

static Err intrin_import(Sint buf, Sint len, Interp *interp) {
  try(interp_validate_buf_len(buf, len));
  const auto path = str_alloc_copy((const char *)buf, (Ind)len);
  const auto err  = interp_import(interp, path);
  free(path);
  return err;
}

static Err intrin_comp_extern_adr(Sint buf, Sint len, Interp *interp) {
  Word_str name;
  try(interp_valid_name(buf, len, &name));
  try(interp_extern_adr(interp, name.buf));

  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->norm.has_loads = true;

  const auto comp = &interp->comp;
  U8         reg;
  try(comp_next_arg_reg(comp, &reg));
  asm_append_dysym_load(comp, name.buf, reg, &comp->code.externs);
  return nullptr;
}

static Err intrin_extern_fun(
  Sint buf, Sint len, Sint inp_len, Sint out_len, Interp *interp
) {
  Word_str name;
  try(interp_valid_name(buf, len, &name));
  try(interp_extern_fun(interp, name.buf, inp_len, out_len));
  return nullptr;
}

static Err intrin_find_word(
  Sint buf, Sint len, Wordlist wordlist, Interp *interp, const Sym **sym
) {
  Word_str name;
  try(interp_valid_name(buf, len, &name));
  try(interp_find_word(interp, name.buf, wordlist, sym));
  return nullptr;
}

static Err intrin_inline_word(Sint ptr, Interp *interp) {
  constexpr bool err_mode = false;
  Sym           *caller;
  Sym           *callee;

  try(interp_require_current_sym(interp, &caller));
  try(interp_sym_by_ptr(interp, ptr, &callee));
  try(comp_inline_sym(&interp->comp, caller, callee, err_mode));
  return nullptr;
}

static Err intrin_execute(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(interp_call_sym(interp, sym));
  return nullptr;
}

static Err intrin_comp_signature_get(
  Interp *interp, Sint *inp_len, Sint *out_len, Sint *has_err
) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  if (inp_len) *inp_len = (Sint)sym->inp_len;
  if (out_len) *out_len = (Sint)sym->out_len;
  if (has_err) *has_err = (Sint)sym->has_err;
  return nullptr;
}

/*
Used by "compiling" words to tell the compiler about the
input and output parameter counts. The compiler normally
infers the counts when parsing `{}` signatures which are
not available when meta-programming.
*/
static Err intrin_comp_signature_set(
  Sint inp_len, Sint out_len, Sint has_err, Interp *interp
) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(asm_validate_input_param_reg(inp_len));
  try(asm_validate_output_param_reg(out_len));
  if (has_err && !out_len) {
    return errf(
      "unable to set signature of " FMT_QUOTED
      ": has_err requires at least one output",
      sym->name.buf
    );
  }
  sym->inp_len = (U8)inp_len;
  sym->out_len = (U8)out_len;
  sym->has_err = !!has_err;
  return nullptr;
}

static Err intrin_comp_args_valid(Sint args, Sint action, Interp *interp) {
  try(interp_validate_data_ptr(action));
  try(comp_validate_args(&interp->comp, (const char *)action, args, args));
  return nullptr;
}

static Err intrin_comp_args_min(Sint args, Sint action, Interp *interp) {
  Sym *sym;
  try(comp_require_current_sym(&interp->comp, &sym));
  if (action) try(interp_validate_data_ptr(action));

  const auto ctx     = &interp->comp.ctx;
  const auto arg_low = ctx->arg_low;
  const auto arg_len = ctx->arg_len;
  const auto msg     = action ? (const char *)action : "invalid argument count";

  if (arg_low) {
    return err_args_partial(sym, msg, arg_low, arg_len);
  }
  if (arg_len < args) {
    return err_args_arity(sym, msg, args, args, arg_len);
  }
  return nullptr;
}

static Err intrin_comp_args_get(Interp *interp, Sint *arg_len) {
  if (arg_len) *arg_len = (Sint)interp->comp.ctx.arg_len;
  return nullptr;
}

static Err intrin_comp_args_set(Sint len, Interp *interp) {
  try(asm_validate_arg_reg(len));
  comp_args_set(&interp->comp, (U8)len);
  return nullptr;
}

/*
Used by words which compile other words. For example, "var" uses this to load
its data address into the next argument register, playing nicely with our
on-the-fly register allocation.
*/
static Err intrin_comp_next_arg_reg(Interp *interp, Sint *out) {
  U8 reg;
  try(comp_next_arg_reg(&interp->comp, &reg));
  if (out) *out = (Sint)reg;
  return nullptr;
}

// Caution: only ONE scratch reg is available at a time.
static Err intrin_comp_scratch_reg(Interp *interp, Sint *out) {
  U8 reg;
  try(comp_scratch_reg(&interp->comp, &reg));
  if (out) *out = (Sint)reg;
  return nullptr;
}

/*
Control constructs such as counted loops use this to emit a barrier AFTER
initializing their own locals or otherwise using the available arguments.
This validates that there are no unused arguments, and clobbers each temp
register-local association. Might eventually get a more general mechanism.
*/
static Err intrin_comp_barrier(Sint req, Sint action, Interp *interp) {
  if (action) try(interp_validate_data_ptr(action));
  try(comp_barrier(&interp->comp, (const char *)action, req));
  return nullptr;
}

static Err intrin_comp_clobber(Sint reg, Interp *interp) {
  try(asm_validate_reg(reg));
  try(comp_clobber_reg(&interp->comp, (U8)reg));
  return nullptr;
}

static Err intrin_comp_local_named(
  Sint buf, Sint len, Interp *interp, const Local **out
) {
  try(interp_validate_buf_len(buf, len));
  Local *loc;
  try(interp_get_local(interp, (const char *)buf, (Ind)len, &loc));
  if (out) *out = local_token(loc);
  return nullptr;
}

static Err intrin_comp_local_anon(Interp *interp, const Local **out) {
  const auto comp = &interp->comp;
  const auto loc  = comp_local_anon(comp);
  if (out) *out = local_token(loc);
  return nullptr;
}

static Err intrin_comp_local_get(Sint reg, Sint loc_ptr, Interp *interp) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(asm_validate_reg(reg));
  try(comp_validate_local(comp, loc_ptr, &loc));
  try(comp_append_local_get(comp, loc, (U8)reg));
  return nullptr;
}

static Err intrin_comp_local_set(Sint reg, Sint loc_ptr, Interp *interp) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(asm_validate_reg(reg));
  try(comp_validate_local(comp, loc_ptr, &loc));
  try(comp_append_local_set(comp, loc, (U8)reg));
  return nullptr;
}

static Err intrin_comp_local_off(Sint ptr, Interp *interp, Sint *out) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(comp_validate_local(comp, ptr, &loc));
  comp_local_evict(comp, loc);
  *out = loc->fp_off;
  return nullptr;
}

static Err intrin_alloca(Interp *interp) { return comp_alloca(&interp->comp); }

static Err intrin_compile_executable(Sint main, Sint path, Interp *interp) {
  try(interp_validate_data_ptr(path));
  Sym *sym;
  try(interp_sym_by_ptr(interp, main, &sym));
  try(compile_mach_executable_to(interp, (const char *)path, sym));
  return nullptr;
}

static Err debug_mem(Sint adr, Interp *) {
  debug_mem_at((const Uint *)adr);
  return nullptr;
}

static Err debug_word(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  interp_repr_sym(interp, sym);
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
    "[debug]   data stack len:     " FMT_SINT
    "\n"
    "[debug]   clobber:            %s\n"
    "[debug]   redefining:         %s\n"
    "[debug]   compiling:          %s\n"
    "[debug]   has_alloca:         %s\n"
    "[debug]   try_all:            %s\n",
    name,
    sym,
    inp_len,
    out_len,
    ctx->arg_len,
    ctx->arg_low,
    stack_len(&interp->ints),
    uint32_to_bit_str((U32)sym->clobber),
    bool_str(ctx->redefining),
    bool_str(ctx->compiling),
    bool_str(ctx->has_alloca),
    bool_str(ctx->try_all)
  );

  if (loc_len) {
    eprintf("[debug]   locals (" FMT_SINT "):", loc_len);
    for (stack_range(auto, loc, locs)) {
      eprintf(" %s", loc->name.buf);
    }
    fputc('\n', stderr);
  }

  {
    constexpr auto cap      = arr_cap(ctx->reg_vals);
    const auto     arr      = ctx->reg_vals;
    const auto     args     = ctx->arg_len;
    bool           has_vals = false;
    U8             last_val = 0;

    for (U8 ind = 0; ind < cap; ind++) {
      if (!arr[ind].type) continue;
      has_vals = true;
      last_val = ind;
    }

    if (has_vals || args) {
      eprintf("[debug]   values in argument registers:\n");

      const auto last_val_ind = (U8)(last_val + 1);
      U8         val_cap      = cap;
      val_cap                 = MIN(val_cap, last_val_ind);
      val_cap                 = MAX(val_cap, args);

      for (U8 ind = 0; ind < val_cap; ind++) {
        const auto val = arr[ind];

        switch (val.type) {
          case REG_VAL_IMM: {
            eprintf("[debug]     %d -- constant: " FMT_SINT "\n", ind, val.imm);
            break;
          }
          case REG_VAL_LOC: {
            const auto loc  = val.loc;
            const auto name = comp_local_name(loc);
            eprintf("[debug]     %d -- local: %s\n", ind, name);
            break;
          }
          default: {
            eprintf("[debug]     %d -- unknown\n", ind);
            break;
          }
        }
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
    "[debug] arg: " FMT_SINT " " FMT_UINT_HEX " 0b%s\n",
    val,
    (Uint)val,
    uint64_to_bit_str((Uint)val)
  );
}

// The "missing" fields are set in `sym_init_intrin`.

static const USED auto INTRIN_TRY_ALL = (Sym){
  .name.buf = "try_all",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_try_all,
  .inp_len  = 1,
};

static const USED auto INTRIN_GET_TRY_ALL = (Sym){
  .name.buf = "get_try_all",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_get_try_all,
  .out_len  = 1,
};

static const USED auto INTRIN_BRACE = (Sym){
  .name.buf  = "{",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_brace,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_SIGNATURE_GET = (Sym){
  .name.buf  = "comp_signature_get",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_signature_get,
  .out_len   = 4,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_SIGNATURE_SET = (Sym){
  .name.buf  = "comp_signature_set",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_signature_set,
  .inp_len   = 3,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_ARGS_VALID = (Sym){
  .name.buf  = "comp_args_valid",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_args_valid,
  .inp_len   = 2,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_ARGS_MIN = (Sym){
  .name.buf  = "comp_args_min",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_args_min,
  .inp_len   = 2,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_ARGS_GET = (Sym){
  .name.buf  = "comp_args_get",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_args_get,
  .out_len   = 2,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_ARGS_SET = (Sym){
  .name.buf  = "comp_args_set",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_args_set,
  .inp_len   = 1,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_NEXT_ARG_REG = (Sym){
  .name.buf  = "comp_next_arg_reg",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_next_arg_reg,
  .out_len   = 2,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_SCRATCH_REG = (Sym){
  .name.buf  = "comp_scratch_reg",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_scratch_reg,
  .out_len   = 2,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_BARRIER = (Sym){
  .name.buf  = "comp_barrier",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_barrier,
  .inp_len   = 2,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_CLOBBER = (Sym){
  .name.buf  = "comp_clobber",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_clobber,
  .inp_len   = 1,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_LOCAL_GET = (Sym){
  .name.buf  = "comp_local_get",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_local_get,
  .inp_len   = 2,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_LOCAL_SET = (Sym){
  .name.buf  = "comp_local_set",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_local_set,
  .inp_len   = 2,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_LOCAL_OFF = (Sym){
  .name.buf  = "comp_local_off",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_local_off,
  .inp_len   = 1,
  .out_len   = 2,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_ALLOCA = (Sym){
  .name.buf  = "alloca",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_alloca,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMPILE_EXECUTABLE = (Sym){
  .name.buf = "compile_executable",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_compile_executable,
  .inp_len  = 2,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_DEBUG_WORD = (Sym){
  .name.buf = "debug_word",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_word,
  .inp_len  = 1,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_DEBUG_CTX = (Sym){
  .name.buf  = "debug_ctx",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_debug_ctx,
  .comp_only = true,
};

static const USED auto INTRIN_DEBUG_CTX_COMP = (Sym){
  .name.buf  = "#debug_ctx",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_debug_ctx,
  .comp_only = true,
};

static const USED auto INTRIN_DEBUG_ARG = (Sym){
  .name.buf = "debug_arg",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_debug_arg,
  .inp_len  = 1,
};
