#pragma once
#include "../clib/dict.c"
#include "../clib/list.c"
#include "../clib/num.h"
#include "../clib/stack.c"
#include "../clib/str.c"
#include "./arch.c"
#include "./comp.h"
#include "./sym.c"

static Err err_current_sym_not_defining() {
  return err_str("unable to find current word: not inside a colon definition");
}

static Err comp_require_current_sym(const Comp *comp, Sym **out) {
  const auto sym = comp->ctx.sym;
  if (!sym) return err_current_sym_not_defining();
  if (out) *out = sym;
  return nullptr;
}

static Err err_internal_dup_local(const char *name) {
  return errf("internal error: duplicate local name " FMT_QUOTED, name);
}

static Err comp_local_add(Comp *comp, Word_str word, Local **out) {
  const auto dict = &comp->ctx.local_dict;
  const auto name = word.buf;
  if (dict_has(dict, name)) return err_internal_dup_local(name);

  const auto loc = stack_push(&comp->ctx.locals, (Local){.name = word});
  dict_set(dict, loc->name.buf, loc);
  if (out) *out = loc;
  return nullptr;
}

static Local *comp_local_get(Comp *comp, const char *name) {
  return dict_get(&comp->ctx.local_dict, name);
}

static Err comp_local_get_or_make(Comp *comp, Word_str name, Local **out) {
  auto loc = comp_local_get(comp, name.buf);
  if (loc) {
    if (out) *out = loc;
    return nullptr;
  }
  return comp_local_add(comp, name, out);
}

static Local *comp_local_anon(Comp *comp) {
  const auto ctx = &comp->ctx;
  const auto loc = stack_push(&ctx->locals, (Local){});
  str_fmt(&loc->name, "(anon_" FMT_IND ")", ctx->anon_locs++);
  return loc;
}

static Err err_local_invalid(Local *ptr) {
  return errf("%p is not a valid address of a local", ptr);
}

static Err comp_validate_local(Comp *comp, Sint num, Local **out) {
  const auto locs = &comp->ctx.locals;
  const auto ptr  = (Local *)num;
  if (!is_stack_elem(locs, ptr)) return err_local_invalid(ptr);
  if (out) *out = ptr;
  return nullptr;
}

static Err comp_inline_sym(
  Comp *comp, Sym *caller, const Sym *callee, bool catch
) {
  try(comp_require_current_sym(comp, nullptr));
  try(asm_inline_sym(comp, caller, callee, catch));
  return nullptr;
}

#ifndef CALL_CONV_STACK
#include "./comp_cc_reg.c"
#else
#include "./comp_cc_stack.c"
#endif

static bool instr_heap_valid(const Instr_heap *val) {
  return val && is_aligned(val) && is_aligned(&val->instrs);
}

// clang-format off

// Sanity check used in debugging.
static bool comp_heap_valid(const Comp_heap *val) {
  return (
    val &&
    is_aligned(val) &&
    is_aligned(&val->exec) &&
    instr_heap_valid(&val->exec) &&
    is_aligned(&val->externs) &&
    is_aligned(&val->intrins)
  );
}

// Sanity check used in debugging.
static bool comp_code_valid(const Comp_code *code) {
  return (
    code &&
    is_aligned(code) &&
    is_aligned(&code->write) &&
    is_aligned(&code->heap) &&
    is_aligned(&code->code_write) &&
    is_aligned(&code->code_exec) &&
    is_aligned(&code->data) &&
    is_aligned(&code->externs) &&
    is_aligned(&code->intrins) &&
    is_aligned(&code->valid_instr_len) &&
    instr_heap_valid(code->write) &&
    comp_heap_valid(code->heap) &&
    list_valid((const List *)&code->code_write) &&
    list_valid((const List *)&code->code_exec) &&
    list_valid((const List *)&code->data)
  );
}

// Sanity check used in debugging.
static bool comp_valid(const Comp *comp) {
  return (
    comp &&
    is_aligned(comp) &&
    is_aligned(&comp->code) &&
    is_aligned(&comp->ctx) &&
    comp_code_valid(&comp->code) &&
    comp_ctx_valid(&comp->ctx)
  );
}

// clang-format on

static Err instr_heap_deinit(Instr_heap **out) {
  if (!out || !*out) return nullptr;
  try(err_errno(munmap(*out, sizeof(**out))));
  *out = nullptr;
  return nullptr;
}

static Err instr_heap_init(Instr_heap **out) {
  const auto ptr = mem_map(sizeof(Instr_heap), 0);
  if (ptr == MAP_FAILED) return err_mmap();

  const auto base = (Instr_heap *)ptr;
  *out            = base;
  return mem_protect(base->instrs, sizeof(base->instrs), PROT_READ | PROT_WRITE);
}

static Err comp_heap_deinit(Comp_heap **out) {
  if (!out || !*out) return nullptr;
  try(err_errno(munmap(*out, sizeof(**out))));
  *out = nullptr;
  return nullptr;
}

static Err comp_heap_init(Comp_heap **out) {
  const auto ptr = mem_map(sizeof(Comp_heap), MAP_JIT);
  if (ptr == MAP_FAILED) return err_mmap();

  const auto heap = (Comp_heap *)ptr;
  *out            = heap;

  try(mprotect_jit(heap->exec.instrs, sizeof(heap->exec.instrs)));
  try(mem_protect(heap->data, sizeof(heap->data), PROT_READ | PROT_WRITE));
  try(mem_protect(heap->externs, sizeof(heap->externs), PROT_READ | PROT_WRITE));
  try(mem_protect(heap->intrins, sizeof(heap->intrins), PROT_READ | PROT_WRITE));
  return nullptr;
}

static Err comp_code_deinit(Comp_code *code) {
  dict_deinit(&code->externs.inds);
  dict_deinit(&code->intrins.inds);

  Err err = nullptr;
  err     = either(err, stack_deinit(&code->externs.names));
  err     = either(err, stack_deinit(&code->intrins.names));
  err     = either(err, instr_heap_deinit(&code->write));
  err     = either(err, comp_heap_deinit(&code->heap));
  *code   = (Comp_code){};
  return err;
}

static void comp_code_init_lists(Comp_code *code) {
  ptr_set(
    &code->code_write,
    {
      .dat = code->write->instrs,
      .len = 0,
      .cap = arr_cap(code->write->instrs),
    }
  );

  ptr_set(
    &code->code_exec,
    {
      .dat = code->heap->exec.instrs,
      .len = 0,
      .cap = arr_cap(code->heap->exec.instrs),
    }
  );

  ptr_set(
    &code->data,
    {
      .dat = code->heap->data,
      .len = 0,
      .cap = arr_cap(code->heap->data),
    }
  );

  ptr_set(
    &code->externs.addrs,
    {
      .dat = code->heap->externs,
      .len = 0,
      .cap = arr_cap(code->heap->externs),
    }
  );

  ptr_set(
    &code->intrins.addrs,
    {
      .dat = code->heap->intrins,
      .len = 0,
      .cap = arr_cap(code->heap->intrins),
    }
  );
}

static Err comp_code_init(Comp_code *code) {
  *code = (Comp_code){};

  try(instr_heap_init(&code->write));
  try(comp_heap_init(&code->heap));
  comp_code_init_lists(code);

  auto opt = (Stack_opt){.len = arr_cap(code->heap->externs)};
  try(stack_init(&code->externs.names, &opt));

  opt = (Stack_opt){.len = arr_cap(code->heap->intrins)};
  try(stack_init(&code->intrins.names, &opt));

  return nullptr;
}

static Err comp_init(Comp *comp) {
  try(comp_code_init(&comp->code));
  try(comp_ctx_init(&comp->ctx));
  return nullptr;
}

static Err comp_deinit(Comp *comp) {
  try(comp_ctx_deinit(&comp->ctx));
  try(comp_code_deinit(&comp->code));
  return nullptr;
}

static void comp_rewind(Comp *tar, Comp *snap) {
  comp_ctx_rewind(&tar->ctx, &snap->ctx);
}

/*
Used for registering dynamically-linked symbols, either intrinsic or external.
The provided address is used only in JIT-compiled code. In AOT-compiled code,
intrinsics are not used, and external symbols are patched by the OS dylinker.
*/
static void comp_register_dysym(Comp_syms *syms, const char *name, U64 addr) {
  const auto addrs = &syms->addrs;
  const auto names = &syms->names;
  const auto inds  = &syms->inds;

  auto got_ind = dict_get_or(inds, name, INVALID_IND);
  if (got_ind != INVALID_IND) return;

  got_ind = addrs->len;
  aver(stack_len(names) == got_ind);
  aver(inds->len == got_ind);

  const auto got_name = names->top;
  averr(str_copy(got_name, name));

  names->top++;
  list_push(addrs, addr);
  dict_set(inds, got_name->buf, got_ind);
}

static void *comp_find_extern(Comp *comp, const char *name) {
  const auto syms = &comp->code.externs;
  const auto inds = &syms->inds;
  const auto ind  = dict_get_or(inds, name, INVALID_IND);
  if (ind == INVALID_IND) return nullptr;
  return list_elem_ptr(&syms->addrs, ind);
}

static Err comp_append_call_sym(Comp *comp, Sym *callee) {
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  sym_auto_comp_only(caller, callee);
  sym_auto_interp_only(caller, callee);

  const auto ctx = &comp->ctx;
  if (callee->throws && !ctx->catches) caller->throws = true;

  const auto catch = callee->throws && ctx->catches;
  bool inlined     = false;

  switch (callee->type) {
    case SYM_NORM: {
      try(comp_append_call_norm(comp, callee, catch, &inlined));
      break;
    }
    case SYM_INTRIN: {
      try(comp_append_call_intrin(comp, callee, catch));
      break;
    }
    case SYM_EXTERN: {
      try(comp_append_call_extern(comp, callee));
      break;
    }
    default: unreachable();
  }

  if (!inlined) {
    set_add(&caller->callees, callee);
    set_add(&callee->callers, caller);
  }

  IF_DEBUG(eprintf(
    "[system] compiled call of symbol " FMT_QUOTED "\n", callee->name.buf
  ));
  return nullptr;
}

static void comp_sym_beg(Comp *comp, Sym *sym) {
  comp_ctx_trunc(&comp->ctx);
  comp->ctx.sym       = sym;
  comp->ctx.compiling = true;
  asm_sym_beg(comp, sym);
}

static void comp_sym_end(Comp *comp, Sym *sym) {
  asm_sym_end(comp, sym);
  sym_auto_inlinable(sym);

#ifndef CALL_CONV_STACK
  comp_warn_unused_locals(&comp->ctx);
#endif

  comp_ctx_trunc(&comp->ctx);
}

/*
Prints instructions as hexpairs in the system's endian order,
which is usually little endian. How to disassemble:

  echo <hex> | llvm-mc --disassemble --hex

Note that we also provide intrinsic `dis'` which prints disassembly.
*/
static void comp_debug_print_sym_instrs(
  const Comp *comp, const Sym *sym, const char *prefix
) {
  const auto spans = &sym->norm.spans;
  const auto code  = &comp->code;

  {
    const auto instrs = &code->code_write;
    const auto floor  = &instrs->dat[spans->prologue];
    const auto ceil   = &instrs->dat[spans->ceil];

    eprintf(
      "%swritable code address: %p\n"
      "%sinstructions in writable code heap (" FMT_SINT "):\n",
      prefix,
      floor,
      prefix,
      ceil - floor
    );
    fputs(prefix, stderr);
    eprint_byte_range_hex((U8 *)floor, (U8 *)ceil);
    fputc('\n', stderr);
  }

  if (comp_code_is_sym_ready(code, sym)) {
    const auto instrs = &code->code_exec;
    const auto floor  = &instrs->dat[spans->prologue];
    const auto ceil   = &instrs->dat[spans->ceil];

    eprintf(
      "%sexecutable code address: %p\n"
      "%sinstructions in executable code heap (" FMT_SINT "):\n",
      prefix,
      floor,
      prefix,
      ceil - floor
    );
    fputs(prefix, stderr);
    eprint_byte_range_hex((U8 *)floor, (U8 *)ceil);
    fputc('\n', stderr);
  }
}

static Err comp_sym_instr_range(
  Comp *comp, const Sym *sym, const Instr **floor, const Instr **ceil
) {
  const auto code   = &comp->code;
  const auto instrs = &code->code_exec;
  const auto spans  = &sym->norm.spans;

  try(comp_code_ensure_sym_ready(code, sym));
  aver(spans->ceil > spans->prologue);

  if (floor) *floor = &instrs->dat[spans->prologue];
  if (ceil) *ceil = &instrs->dat[spans->ceil];
  return nullptr;
}

static Err comp_append_ret(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  stack_push(
    &comp->ctx.asm_fix,
    (Asm_fixup){
      .type = ASM_FIX_RET,
      .ret  = asm_append_breakpoint(comp, ASM_CODE_RET),
    }
  );
  return nullptr;
}

static Err comp_append_recur(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  set_add(&sym->callees, sym);
  set_add(&sym->callers, sym);

  stack_push(
    &comp->ctx.asm_fix,
    (Asm_fixup){
      .type  = ASM_FIX_RECUR,
      .recur = asm_append_breakpoint(comp, ASM_CODE_RECUR),
    }
  );
  return nullptr;
}

/*
Takes an address and compiles instructions for obtaining that address using a
PC-relative offset. Intended for accessing non-code segments of `Comp_heap`,
namely `data`, `externs`, `intrins`.

TODO skip `adrp` when offset fits into `adr`.
*/
static Err comp_append_page_addr(Comp *comp, Uint adr, U8 reg) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  asm_append_page_addr(comp, reg, adr);
  sym->norm.has_loads = true;
  return nullptr;
}

// Counterpart of `comp_append_page_addr` which always loads.
static Err comp_append_page_load(Comp *comp, Uint adr, U8 reg) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  asm_append_page_load(comp, reg, adr);
  sym->norm.has_loads = true;
  return nullptr;
}

static Err err_out_of_space_data() {
  return err_str("unable to allocate static data: out of space");
}

// The resulting address is located at a significant distance from executable
// code, and needs to be accessed via the `adrp & add/ldr` idiom.
static Err comp_alloc_data(Comp *comp, const U8 *src, Ind len, const U8 **out) {
  const auto data = &comp->code.data;
  data->len       = __builtin_align_up(data->len, sizeof(void *));

  if (len > list_rem_bytes(data)) return err_out_of_space_data();

  const auto adr = list_next_ptr(data);
  data->len += len;

  if (src) memcpy(adr, src, len);
  if (out) *out = adr;

  IF_DEBUG(eprintf(
    "[system] allocated data region with address %p and length " FMT_IND "\n",
    adr,
    len
  ));
  return nullptr;
}

static const char *asm_fixup_fmt(Asm_fixup *fix) {
  static thread_local char BUF[4096];

  switch (fix->type) {
    case ASM_FIX_IMM: {
      const auto val = &fix->imm;
      return sprintbuf(
        BUF,
        "{type = imm, instr = %p, reg = %d, num = " FMT_SINT "}",
        val->instr,
        val->reg,
        val->num
      );
    }

    case ASM_FIX_RET: {
      return sprintbuf(BUF, "{type = ret, instr = %p}", fix->ret);
    }

    case ASM_FIX_TRY: {
      return sprintbuf(
        BUF,
        "{type = try, instr = %p, err_reg = %d}",
        fix->try.instr,
        fix->try.err_reg
      );
    }

    case ASM_FIX_THROW: {
      return sprintbuf(BUF, "{type = throw, instr = %p}", fix->throw);
    }

    case ASM_FIX_RECUR: {
      return sprintbuf(BUF, "{type = recur, instr = %p}", fix->recur);
    }

    default: unreachable();
  }
}

static Err comp_validate_main(const Sym *main) {
  if (!main) {
    return err_str("unable to build executable: missing entry point");
  }

  if (main->type != SYM_NORM) {
    return errf(
      "unable to build executable: entry word " FMT_QUOTED
      " is not a regular Forth word",
      main->name.buf
    );
  }

  if (main->throws) {
    return errf(
      "unable to build executable: entry point " FMT_QUOTED
      " throws;\n"
      "hint 0: handle all exceptions, log the errors, return 0 on success and non-zero on error;\n"
      "hint 1: use [ true catches ] at the beginning to auto-catch",
      main->name.buf
    );
  }

  if (main->out_len != 1) {
    return errf(
      "unable to build executable: entry point must return exactly one output (exit code); " FMT_QUOTED
      " returns %d outputs",
      main->name.buf,
      main->out_len
    );
  }

  return nullptr;
}
