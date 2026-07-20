#pragma once
#include "./comp.c"
#include "./intrin.c"
#include "./read.c"

// clang-format off

/*
Used in unwinding and debugging.
TODO: could use a magic number.
*/
static bool interp_valid(const Interp *interp) {
  return (
    interp &&
    is_aligned(interp) &&
    // Must precede reads beyond `Ctx`; x28 may hold a regular runtime context.
    interp->ctx.self == interp &&
    is_aligned(&interp->ctx) &&
    is_aligned(&interp->ctx.top) &&
    is_aligned(&interp->ctx.ceil) &&
    is_aligned(&interp->comp) &&
    is_aligned(&interp->ints) &&
    is_aligned(&interp->syms) &&
    is_aligned(&interp->dict_exec) &&
    is_aligned(&interp->dict_comp) &&
    is_aligned(&interp->module) &&
    is_aligned(&interp->snap) &&
    comp_valid(&interp->comp) &&
    reader_valid(interp_reader_const(interp)) &&
    interp->ctx.top &&
    interp->ctx.ceil &&
    interp->ctx.top <= interp->ctx.ceil &&
    stack_valid((const Stack *)&interp->ints) &&
    stack_valid((const Stack *)&interp->syms) &&
    dict_valid((const Dict *)&interp->dict_exec) &&
    dict_valid((const Dict *)&interp->dict_comp)
  );
}

// clang-format on

static Err interp_deinit(Interp *interp) {
  if (!interp) return nullptr;

  const auto syms = &interp->syms;

  if (syms->floor) { // Bot-insisted redundant guard for C UB semantics.
    for (stack_range(auto, sym, syms)) sym_deinit(sym);
  }
  dict_deinit(&interp->dict_exec);
  dict_deinit(&interp->dict_comp);
  dict_deinit_with_keys((Dict *)&interp->imports);

  Err err = nullptr;
  err     = either(err, comp_deinit(&interp->comp));
  err     = either(err, stack_deinit(&interp->syms));
  err     = either(err, stack_deinit(&interp->ints));

  *interp = (Interp){};
  return err;
}

static Err interp_init_syms(Interp *interp) {
  static constexpr Ind intrin_len = (Ind)arr_cap(INTRIN);

  /*
  We have two wordlists (exec and comp), but currently restrict intrinsic
  words to just one namespace due to limitations of `comp_register_dysym`.
  */
  deferred(dict_deinit) Str_set names = {};
  dict_init(&names, round_up_pow2_Ind(intrin_len * 2));

  for (auto intrin = INTRIN; intrin < arr_ceil(INTRIN); intrin++) {
    const auto prev_len = names.len;
    dict_set(&names, intrin->name.buf, EMPTY);
    if (names.len == prev_len) {
      return errf("duplicate intrinsic name " FMT_QUOTED, intrin->name.buf);
    }
  }

  const auto syms = &interp->syms;
  const auto comp = &interp->comp;

  // Hidden XT for standard Forth `;`.
  // Used by defining words that push an XT for `end` to pop and call.
  sym_init_intrin(stack_push(
    syms,
    (Sym){
      .name.buf  = ";",
      .wordlist  = WORDLIST_COMP,
      .intrin    = (void *)intrin_semicolon,
      .out_len   = 1,
      .has_err   = true,
      .comp_only = true,
    }
  ));

  for (auto intrin = INTRIN; intrin < arr_ceil(INTRIN); intrin++) {
    const auto sym = stack_push(syms, *intrin);

    Sym_dict *dict;
    try(interp_wordlist(interp, sym->wordlist, &dict));

    sym_init_intrin(sym);
    try(sym_validate_name(sym));
    IF_DEBUG(try_assert(!dict_has(dict, sym->name.buf)));
    dict_set(dict, sym->name.buf, sym);

    const auto syms = &comp->code.intrins;
    comp_register_dysym(syms, sym->name.buf, (U64)sym->intrin);
  }

  IF_DEBUG({
    const auto syms = &comp->code.intrins;

    try_assert(syms->addrs.len == intrin_len);
    try_assert(stack_len(&syms->names) == intrin_len);
    try_assert(syms->inds.len == intrin_len);
  });
  return nullptr;
}

static Err interp_init(Interp *interp) {
  *interp       = (Interp){};
  Stack_opt opt = {.len = 4096};

  try(stack_init(&interp->ints, &opt));
  try(stack_init(&interp->syms, &opt));
  try(comp_init(&interp->comp));
  try(interp_init_syms(interp)); // Requires `comp_init` first.

  const auto heap = interp->comp.code.heap;

  ptr_set(
    &interp->ctx,
    {.self = interp, .top = heap->arena, .ceil = arr_ceil(heap->arena)}
  );

  try(interp_snapshot(interp));

  IF_DEBUG({
    eprintf("[system] interpreter: %p\n", interp);
    eprintf("[system] integer stack floor: %p\n", interp->ints.floor);
    eprintf(
      "[system] instruction floor (writable): %p\n",
      interp->comp.code.code_write.dat
    );
    eprintf(
      "[system] instruction floor (executable): %p\n",
      interp->comp.code.code_exec.dat
    );
  });
  return nullptr;
}
