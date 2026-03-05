/*
This file implements stack unwinding for the stack-CC version of the system.
When detecting stack underflow / overflow, we use this to return control to
the calling C code; this is relevant for the REPL mode, where otherwise the
whole program would crash. Currently supports only Arm64 + Mach.

We only need this for hardware-triggered "bad memory access" exceptions.
When modifying the Forth integer stack in the interpreter code, we check
bounds in software. Additionally, regular "exceptions" in Forth code use
local "gotos" and don't involve this at all.
*/
#pragma once
#include "./arch.h"
#include "./arch_arm64.c"
#include "./interp.h"
#include "./lib/fmt.c"
#include "./lib/mach_exc.c"
#include "./lib/misc.h"
#include "./mach_misc.h"
#include <mach/mach.h>
#include <stdio.h>

extern const Instr asm_call_forth_epilogue __asm__("asm_call_forth_epilogue");
extern const Instr asm_call_forth_trace __asm__("asm_call_forth_trace");

#define UNWIND_CTX_FMT "; frame " FMT_UINT " = %p, callee = %p"
#define UNWIND_CTX_INP frame_ind, frame, callee

/*
Reference:

  https://github.com/ARM-software/abi-aa/blob/c51addc3dc03e73a016a1e4edf25440bcac76431/aapcs64/aapcs64.rst#646the-frame-pointer
*/
typedef struct Frame_record {
  struct Frame_record *parent;
  const Instr         *caller;
} Frame_record;

static Err err_unwind_no_frame() {
  return err_str("unable to unwind: FP register is zero");
}

static Err err_unwind_empty_frame(const Frame_record *frame) {
  return errf("unable to unwind: frame record at address %p is empty", frame);
}

static Err err_unwind_internal_no_parent(
  const Frame_record *frame, Uint frame_ind, const void *callee
) {
  return errf(
    "unable to unwind: internal error: frame record does not refer to a parent frame" UNWIND_CTX_FMT,
    UNWIND_CTX_INP
  );
}

static Err err_unwind_internal_no_caller(
  const Frame_record *frame, Uint frame_ind, const void *callee
) {
  return errf(
    "unable to unwind: internal error: frame record does not have a caller instruction to return to" UNWIND_CTX_FMT,
    UNWIND_CTX_INP
  );
}

static Err err_unwind_internal_direction(
  const Frame_record *frame, Uint frame_ind, const void *callee
) {
  return errf(
    "unable to unwind: internal error: frame record refers to a parent frame at a lower address" UNWIND_CTX_FMT,
    UNWIND_CTX_INP
  );
}

static Err err_unwind_no_epilogue(const Frame_record *frame, const void *pc_new) {
  return errf(
    "unable to unwind: outer PC %p doesn't match epilogue address %p; frame: %p; parent: %p; caller: %p",
    pc_new,
    &asm_call_forth_epilogue,
    frame,
    frame->parent,
    frame->caller
  );
}

static Err err_unwind_invalid_stack(
  const Frame_record *frame, const void *sp_new, const void *pc_new
) {
  return errf(
    "unable to unwind: memory at the deduced SP %p has unexpected values, indicating invalid SP after unwinding; PC: %p; frame: %p; parent: %p; caller: %p",
    sp_new,
    pc_new,
    frame,
    frame->parent,
    frame->caller
  );
}

/*
For simplicity, this unwinder "cheats": it ignores each "catch" and returns
control directly to outer C code. It also assumes we don't support Go-style
deferred cleanup; invoking cleanup code during unwinding would increase the
complexity. The real use case of unwinding is REPL mode, where such nuances
are not important.

Assumptions:
- Every "catch" can be ignored.
- Deferred cleanup can be ignored.
- If the faulty PC is in Forth, the call chain contains `asm_call_forth`.
*/
static Err mach_unwind_thread(
  const Interp *interp, const char *msg, Thread_state *state
) {
  const auto code   = &interp->comp.code;
  auto       frame  = (Frame_record *)state->fp;
  const auto pc_old = (const Instr *)state->pc;
  auto       pc_new = pc_old;

  if (!frame) return err_unwind_no_frame();
  if (!frame->parent && !frame->caller) return err_unwind_empty_frame(frame);

  const auto leaf_lr = (const Instr *)state->lr;

  // Leaf procedure without its own frame record.
  if (comp_code_is_instr_ours(code, pc_new) && frame->caller != leaf_lr) {
    IF_DEBUG(eprintf(
      "[system] [unwind] frame -1: leaf procedure without a frame; PC: %p -> %p; LR: %p -> %p\n",
      pc_new,
      leaf_lr,
      (void *)state->lr,
      frame->caller
    ));

    /*
    Note: when doing this, we must also modify the LR;
    we do this unconditionally at the end of unwinding,
    using the last found frame.
    */
    pc_new = leaf_lr;
  }

  Uint frame_ind = 0;

  while (comp_code_is_instr_ours(code, pc_new)) {
    if (!frame->parent) {
      return err_unwind_internal_no_parent(frame, frame_ind, pc_new);
    }
    if (!frame->caller) {
      return err_unwind_internal_no_caller(frame, frame_ind, pc_new);
    }
    if (!(frame->parent > frame)) {
      return err_unwind_internal_direction(frame, frame_ind, pc_new);
    }

    IF_DEBUG({
      eprintf("[system] [unwind] frame " FMT_UINT " %p: ", frame_ind, frame);
      repr_struct(frame);
    });

    pc_new = frame->caller;
    frame  = frame->parent;
    frame_ind++;
  }

  IF_DEBUG({
    if (frame_ind) {
      eprintf(
        "[system] [unwind] final frame " FMT_UINT " %p: ", frame_ind, frame
      );
    }
    else {
      eprintf("[system] [unwind] frame %p: ", frame);
    }
    repr_struct(frame);
  });

  if (pc_new != &asm_call_forth_epilogue) {
    return err_unwind_no_epilogue(frame, pc_new);
  }

  pc_new = &asm_call_forth_trace;

  /*
  Finally, we need to restore the SP. Since we're returning
  to the trampoline, we can simply use its FP as the basis.
  */
  const auto sp_new = (const U64 *)frame - 4;

  // SYNC[asm_magic].
  const auto sp_ok = sp_new[1] == 0xABCD'FEED'ABCD'FACE;
  if (!sp_ok) {
    eprintf(
      "[system] [unwind] memory at SP %p:\n"
      "  %p: 0x%0.16llx 0x%0.16llx\n"
      "  %p: 0x%0.16llx 0x%0.16llx\n"
      "  %p: 0x%0.16llx 0x%0.16llx\n"
      "  %p: 0x%0.16llx 0x%0.16llx\n",
      sp_new,
      sp_new,
      sp_new[0],
      sp_new[1],
      sp_new + 2,
      sp_new[2],
      sp_new[3],
      sp_new + 4,
      sp_new[4],
      sp_new[5],
      sp_new + 6,
      sp_new[6],
      sp_new[7]
    );
    fflush(stderr);
    return err_unwind_invalid_stack(frame, sp_new, pc_new);
  }

  assign_cast(&state->fp, frame);
  assign_cast(&state->lr, frame->caller);
  assign_cast(&state->sp, sp_new);
  assign_cast(&state->pc, pc_new);
  assign_cast(&state->x[ASM_REG_ERR], msg); // Return error string to C.
  return nullptr;
}
