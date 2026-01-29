/*
Cleaned-up, stripped-down, de-obfuscated, less-horrible version of the
exception-handling "server" code created via Mach Interface Generator.
*/

#pragma once
#include <Availability.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig.h>
#include <mach/mig_errors.h>
#include <mach/ndr.h>
#include <mach/notify.h>
#include <mach/port.h>
#include <mach/std_types.h>
#include <string.h>

/*
routine mach_exception_raise_state(
        exception_port : mach_port_t;
        exception      : exception_type_t;
        code           : mach_exception_data_t, const;
  inout flavor         : int;
        old_state      : thread_state_t, const;
    out new_state      : thread_state_t
);
*/

kern_return_t extern catch_mach_exception_raise_state(
  mach_port_t                 exception_port,
  exception_type_t            exception,
  const mach_exception_data_t code,
  mach_msg_type_number_t      code_len,
  int                        *flavor,
  const thread_state_t        state_prev,
  mach_msg_type_number_t      state_prev_len,
  thread_state_t              state_next,
  mach_msg_type_number_t     *state_next_len
);

#pragma pack(push, 4)

// Adapted from `__Request__mach_exception_raise_state_t`.
typedef struct {
  mach_msg_header_t      head;
  NDR_record_t           NDR;
  exception_type_t       exception;
  mach_msg_type_number_t code_len;
  int64_t                code_max[2];
  int                    flavor;
  mach_msg_type_number_t state_prev_len;
  natural_t              state_prev_max[THREAD_STATE_MAX];
} Mach_exc_inp_max;

// Adapted from `__Reply__mach_exception_raise_state_t`.
typedef struct {
  mig_reply_error_t      err;
  int                    flavor;
  mach_msg_type_number_t state_next_len;
  natural_t              state_next_max[THREAD_STATE_MAX];
} Mach_exc_out_max;

// Followed by `Mach_exc_inp_seg_0`.
typedef struct {
  mach_msg_header_t head;
  NDR_record_t      NDR;
} Mach_exc_inp_head;

// Args begin here; port is implied. Followed by `Mach_exc_inp_seg_1`.
typedef struct {
  exception_type_t       exception;
  mach_msg_type_number_t code_len;
  int64_t                code[] __attribute__((__counted_by__(code_len)));
} Mach_exc_inp_seg_0;

typedef struct {
  int                    flavor;
  mach_msg_type_number_t state_prev_len;
  natural_t              state_prev[];
} Mach_exc_inp_seg_1;

typedef struct {
  mig_reply_error_t      err;
  int                    flavor;
  mach_msg_type_number_t state_next_len;
  natural_t              state_next[];
} Mach_exc_out;

#pragma pack(pop)

[[maybe_unused]]
static constexpr auto MACH_EXC_MSG_MAX_SIZE = sizeof(union {
  Mach_exc_inp_max req;
  Mach_exc_out_max res;
});

/*
The Mach kernel sets this id on `catch_exception_raise_state` messages,
which can be received on ports initialized via `task_set_exception_ports`
with the `EXCEPTION_STATE` flag.
*/
static constexpr auto MACH_MSG_EXCEPTION_STATE = 2406;

// Adapted from `mach_exc_server` and `_Xmach_exception_raise_state`.
boolean_t mach_on_exception(mach_msg_header_t *inp, mach_msg_header_t *out) {
  const auto out_msg = (Mach_exc_out *)out;
  const auto id      = inp->msgh_id;
  const auto bits    = MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(inp->msgh_bits), 0);

  out_msg->err = (mig_reply_error_t){
    .Head =
      {
        .msgh_bits        = bits,
        .msgh_remote_port = inp->msgh_remote_port,
        .msgh_local_port  = MACH_PORT_NULL,
        .msgh_id          = id + 100,
      },
    .NDR = NDR_record,
  };

  if (id != MACH_MSG_EXCEPTION_STATE) {
    out_msg->err.RetCode        = MIG_BAD_ID;
    out_msg->err.Head.msgh_size = sizeof(mig_reply_error_t);
    return false;
  }

  /*
  Inline arrays are variable-sized, which makes it impossible to define a struct
  which represents a message containing an array. MIG produces a "fake" struct
  definition, where array fields have the maximum allowed size, and uses weird
  pointer arithmetic to adjust pointers in accordance with provided lengths.
  Using separate segments with trailing arrays seems clearer.
  */
  const auto ptr0 = (Mach_exc_inp_head *)inp + 1;
  const auto seg0 = (Mach_exc_inp_seg_0 *)ptr0;
  const auto ptr1 = (char *)(seg0 + 1) + seg0->code_len * sizeof(seg0->code[0]);
  const auto seg1 = (Mach_exc_inp_seg_1 *)ptr1;

  // TODO validate `bits`, `seg0->code_len`, total msg size.

  const auto err_code = catch_mach_exception_raise_state(
    inp->msgh_local_port,
    seg0->exception,
    seg0->code,
    seg0->code_len,
    &seg1->flavor,
    seg1->state_prev,
    seg1->state_prev_len,
    out_msg->state_next,
    &out_msg->state_next_len
  );

  if (err_code != KERN_SUCCESS) {
    out_msg->err.RetCode        = err_code;
    out_msg->err.Head.msgh_size = sizeof(mig_reply_error_t);
    return false;
  }

  const auto out_size = offsetof(Mach_exc_out, state_next) +
    out_msg->state_next_len * sizeof(out_msg->state_next[0]);

  out_msg->err.Head.msgh_size = (mach_msg_size_t)out_size;
  out_msg->flavor             = seg1->flavor;
  return true;
}
