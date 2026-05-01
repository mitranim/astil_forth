/*
This file implements a Mach handler for "bad memory access" exceptions.
It detects underflow and overflow of the Forth integer stack, replacing
or augmenting cryptic segmentation faults with an actual error message.
When `DEBUG` is enabled, it also displays part of the execution context
of the faulting thread.

In the stack-CC version of the system, this also unwinds Forth frames in the
faulting thread, returning control to C-based interpreter code. This lets us
improve REPL UX; in stack-CC, stack underflow / overflow is very common, and
crashing the entire program every time would be very inconvenient.

In reg-CC, we don't unwind, because:
- The ABI has diverged.
- Compiler validates arity at compile time.
- Stack is used less often and underflow is rare.

If we wanted to convert stack underflow / overflow into an exception
that could be caught in program code, we would consider building CFI
tables which associate instruction addresses with systack frames and
catch handlers. But the complexity is not worth it.
*/
#pragma once
#include "../clib/cli.c"
#include "../clib/fmt.c"
#include "../clib/mach_exc.c"
#include "../clib/mach_misc.h"
#include "../clib/misc.h"
#include "./arch.h"
#include "./interp.c"
#include <mach/mach.h>
#include <stdio.h>

#ifdef CALL_CONV_STACK
#include "./mach_unwind.c"
#endif

#define SYS_REC_FMT "[system] [recovery] "

static void Mach_thread_state_repr(const Mach_thread_state *val) {
  eprint_struct_beg(val, const Mach_thread_state);

#ifdef CALL_CONV_STACK
  eprint_struct_field_hint(val, x[0], " // error register");
#else
  eprint_struct_field(val, x[0]);
#endif

  eprint_struct_field(val, x[1]);
  eprint_struct_field(val, x[2]);
  eprint_struct_field(val, x[3]);
  eprint_struct_field(val, x[4]);
  eprint_struct_field(val, x[5]);
  eprint_struct_field(val, x[6]);
  eprint_struct_field(val, x[7]);
  eprint_struct_field(val, x[8]);
  eprint_struct_field(val, x[9]);
  eprint_struct_field(val, x[10]);
  eprint_struct_field(val, x[11]);
  eprint_struct_field(val, x[12]);
  eprint_struct_field(val, x[13]);
  eprint_struct_field(val, x[14]);
  eprint_struct_field(val, x[15]);
  eprint_struct_field(val, x[16]);
  eprint_struct_field(val, x[17]);
  eprint_struct_field(val, x[18]);
  eprint_struct_field(val, x[19]);
  eprint_struct_field(val, x[20]);
  eprint_struct_field(val, x[21]);
  eprint_struct_field(val, x[22]);
  eprint_struct_field(val, x[23]);
  eprint_struct_field(val, x[24]);
  eprint_struct_field(val, x[25]);

#ifdef CALL_CONV_STACK

  // SYNC[asm_arm64_cc_stack_special_regs].
  eprint_struct_field_hint(val, x[26], " // stack floor register");
  eprint_struct_field_hint(val, x[27], " // stack top register");

#else // CALL_CONV_STACK

  eprint_struct_field(val, x[26]);
  eprint_struct_field(val, x[27]);

#endif // CALL_CONV_STACK

  // SYNC[asm_reg_interp].
  eprint_struct_field_hint(val, x[28], " // interpreter register");
  eprint_struct_field(val, fp);
  eprint_struct_field(val, lr);
  eprint_struct_field(val, sp);
  eprint_struct_field(val, pc);
  eprint_struct_field(val, cpsr);
  eprint_struct_field(val, pad);
  eprint_struct_end();
}

static void recovery_log_ctx(const Interp *interp) {
  if (!interp_valid(interp)) return;
  const auto read = interp->reader;
  if (!reader_valid(read)) return;
  eprintf(SYS_REC_FMT "position: " READ_POS_FMT "\n", READ_POS_ARGS(read));

  // backtrace_from_fp();
}

// NOLINTBEGIN(readability-non-const-parameter)
// NOLINTBEGIN(misc-misplaced-const)

/*
`EXC_MASK_BAD_ACCESS` is sometimes recoverable; when it's an overflow or
underflow of the Forth integer stack, we can assume that the thread state
is otherwise not corrupted. We then modify the thread state to convert this
to a Forth exception. When the thread resumes, the error bubbles up to C.

TODO: if we ever bother to support the x64 architecture, we should specify
`EXC_ARITHMETIC` for division by zero and convert it to a Forth exception.

When initing the exception port, it should be safe to specify unsupported
exception types; we simply print thread state and skip exception handling.
*/
kern_return_t catch_mach_exception_raise_state(
  mach_port_t                 exception_port,
  exception_type_t            exception,
  const mach_exception_data_t code,
  mach_msg_type_number_t      code_len,
  int                        *flavor,
  const thread_state_t        state_prev_ptr,
  mach_msg_type_number_t      state_prev_len,
  thread_state_t              state_next_ptr,
  mach_msg_type_number_t     *state_next_len
) {
  // NOLINTEND(misc-misplaced-const)
  // NOLINTEND(readability-non-const-parameter)

  (void)exception_port;

  constexpr auto state_chunk_size = sizeof(*(thread_state_t) nullptr);
  constexpr auto state_len = sizeof(Mach_thread_state) / state_chunk_size;
  static_assert(!(sizeof(Mach_thread_state) % state_chunk_size));

  if (code_len != 2) return MIG_BAD_ARGUMENTS;
  if (*flavor != ARM_THREAD_STATE64) return KERN_FAILURE;
  if (state_prev_len != state_len) return MIG_BAD_ARGUMENTS;

  const auto state  = (Mach_thread_state *)state_prev_ptr;
  const auto interp = (Interp *)state->x[ASM_REG_INTERP];

  if (DEBUG) {
    eprintf(SYS_REC_FMT "bad thread state: ");
    Mach_thread_state_repr(state);
    eprintf(SYS_REC_FMT "code[0]: %p\n", (void *)code[0]);
    eprintf(SYS_REC_FMT "code[1]: %p (bad address)\n", (void *)code[1]);
    eprintf(SYS_REC_FMT "interpreter address: %p\n", interp);

    /*
    Would be nice if this worked, but sadly `backtrace_from_fp` seems to look
    only for the current thread's frames.

    TODO: backtrace on our own. See `./mach_unwind.c`.

    void      *buf[256];
    const auto len = backtrace_from_fp((void *)state->fp, buf, arr_cap(buf));
    if (len) {
      eprintf(SYS_REC_FMT "backtrace:\n");
      backtrace_symbols_fd(buf, len, STDERR_FILENO);
    }
    */

    fflush(stderr);
  }

  if (!interp_valid(interp)) {
    IF_DEBUG({
      eprintf(
        SYS_REC_FMT
        "address %p recovered from register %d does not appear to reference valid interpreter state\n",
        interp,
        ASM_REG_INTERP
      );
      fflush(stderr);
    });
    return KERN_FAILURE;
  }

  if (exception != EXC_BAD_ACCESS) return KERN_FAILURE;

  const auto  comp      = &interp->comp;
  const auto  bad_addr  = (void *)code[1]; // undocumented
  const auto  ints      = &interp->ints;
  const void *ints_low  = ints->cellar;
  const void *ints_high = (const U8 *)ints_low + ints->bytelen;

  if (!(bad_addr >= ints_low && bad_addr < ints_high)) {
    eprintf(SYS_REC_FMT "bad access at address %p\n", bad_addr);
    eputs(SYS_REC_FMT "known valid address ranges:");
    eprintf(
      SYS_REC_FMT "  integer    stack:        [%p,%p)\n", ints->floor, ints->ceil
    );
    eprintf(
      SYS_REC_FMT "  writable   instructions: [%p,%p)\n",
      comp->code.code_write.dat,
      list_len_ceil(&comp->code.code_write)
    );
    eprintf(
      SYS_REC_FMT "  executable instructions: [%p,%p)\n",
      comp->code.code_exec.dat,
      list_len_ceil(&comp->code.code_exec)
    );
    recovery_log_ctx(interp);
    fflush(stderr);
    return KERN_FAILURE;
  }

  const auto under = bad_addr < (void *)ints->floor;
  const auto msg = under ? "integer stack underflow" : "integer stack overflow";

#ifdef CALL_CONV_STACK

  IF_DEBUG({
    eprintf(SYS_REC_FMT "detected %s\n", msg);
    recovery_log_ctx(interp);
    fflush(stderr);
  });

  const auto err = mach_unwind_thread(interp, msg, state);
  if (err) {
    eprintf(SYS_REC_FMT "%s\n", err);
    recovery_log_ctx(interp);
    fflush(stderr);
    return KERN_FAILURE;
  }

  const auto bad_pc = (void *)state->pc;
  if (bad_pc == bad_addr) return KERN_FAILURE;

  memcpy(state_next_ptr, state, sizeof(*state));
  *state_next_len = state_len;

  // IF_DEBUG({
  //   eprintf(SYS_REC_FMT "updated thread state: ");
  //   Mach_thread_state_repr((const Mach_thread_state *)state_next_ptr);
  //   fflush(stderr);
  // });

  return KERN_SUCCESS;

#else // CALL_CONV_STACK

  (void)state_next_ptr;
  (void)state_next_len;

  eprintf(SYS_REC_FMT "detected %s\n", msg);
  recovery_log_ctx(interp);
  fflush(stderr);

#endif // CALL_CONV_STACK

  return KERN_FAILURE;
}

static Err init_exception_handling(void) {
  bool recovery = true;
  try(env_bool("RECOVERY", &recovery));
  if (!recovery) return nullptr;

  const auto err = mach_exception_init(EXC_MASK_BAD_ACCESS | EXC_BAD_INSTRUCTION);

  // Delivering exceptions to Mach ports is optional...
  if (err) {
    eprintf("[system] %s\n", err);
    return nullptr;
  }

  // ...but if the OS agrees, the handling thread must actually run.
  try(mach_exception_server_init(nullptr));
  IF_DEBUG(eputs("[system] inited mach exception handling"));
  return nullptr;
}
