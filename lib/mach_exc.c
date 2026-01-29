#pragma once
#include "./err.c" // IWYU pragma: export
#include "./mach_exc_msg.c"
#include <mach/mach.h>
#include <pthread.h>
#include <stdlib.h>

/*
Some links:

  https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/Mach/Mach.html
  https://docs.darlinghq.org/internals/macos-specifics/mach-exceptions.html
  https://www.mikeash.com/pyblog/friday-qa-2013-01-11-mach-exception-handlers.html
*/

static mach_port_t mach_exc_port;

/*
Definitions:

  $(xcrun --sdk macosx --show-sdk-path)/usr/include/mach/kern_return.h
*/
static const char *mach_kern_code_name(int code) {
  switch (code) {
    case 0:     return "KERN_SUCCESS";
    case 1:     return "KERN_INVALID_ADDRESS";
    case 2:     return "KERN_PROTECTION_FAILURE";
    case 3:     return "KERN_NO_SPACE";
    case 4:     return "KERN_INVALID_ARGUMENT";
    case 5:     return "KERN_FAILURE";
    case 6:     return "KERN_RESOURCE_SHORTAGE";
    case 7:     return "KERN_NOT_RECEIVER";
    case 8:     return "KERN_NO_ACCESS";
    case 9:     return "KERN_MEMORY_FAILURE";
    case 10:    return "KERN_MEMORY_ERROR";
    case 11:    return "KERN_ALREADY_IN_SET";
    case 12:    return "KERN_NOT_IN_SET";
    case 13:    return "KERN_NAME_EXISTS";
    case 14:    return "KERN_ABORTED";
    case 15:    return "KERN_INVALID_NAME";
    case 16:    return "KERN_INVALID_TASK";
    case 17:    return "KERN_INVALID_RIGHT";
    case 18:    return "KERN_INVALID_VALUE";
    case 19:    return "KERN_UREFS_OVERFLOW";
    case 20:    return "KERN_INVALID_CAPABILITY";
    case 21:    return "KERN_RIGHT_EXISTS";
    case 22:    return "KERN_INVALID_HOST";
    case 23:    return "KERN_MEMORY_PRESENT";
    case 24:    return "KERN_MEMORY_DATA_MOVED";
    case 25:    return "KERN_MEMORY_RESTART_COPY";
    case 26:    return "KERN_INVALID_PROCESSOR_SET";
    case 27:    return "KERN_POLICY_LIMIT";
    case 28:    return "KERN_INVALID_POLICY";
    case 29:    return "KERN_INVALID_OBJECT";
    case 30:    return "KERN_ALREADY_WAITING";
    case 31:    return "KERN_DEFAULT_SET";
    case 32:    return "KERN_EXCEPTION_PROTECTED";
    case 33:    return "KERN_INVALID_LEDGER";
    case 34:    return "KERN_INVALID_MEMORY_CONTROL";
    case 35:    return "KERN_INVALID_SECURITY";
    case 36:    return "KERN_NOT_DEPRESSED";
    case 37:    return "KERN_TERMINATED";
    case 38:    return "KERN_LOCK_SET_DESTROYED";
    case 39:    return "KERN_LOCK_UNSTABLE";
    case 40:    return "KERN_LOCK_OWNED";
    case 41:    return "KERN_LOCK_OWNED_SELF";
    case 42:    return "KERN_SEMAPHORE_DESTROYED";
    case 43:    return "KERN_RPC_SERVER_TERMINATED";
    case 44:    return "KERN_RPC_TERMINATE_ORPHAN";
    case 45:    return "KERN_RPC_CONTINUE_ORPHAN";
    case 46:    return "KERN_NOT_SUPPORTED";
    case 47:    return "KERN_NODE_DOWN";
    case 48:    return "KERN_NOT_WAITING";
    case 49:    return "KERN_OPERATION_TIMED_OUT";
    case 50:    return "KERN_CODESIGN_ERROR";
    case 51:    return "KERN_POLICY_STATIC";
    case 52:    return "KERN_INSUFFICIENT_BUFFER_SIZE";
    case 53:    return "KERN_DENIED";
    case 54:    return "KERN_MISSING_KC";
    case 55:    return "KERN_INVALID_KC";
    case 56:    return "KERN_NOT_FOUND";
    case 0x100: return "KERN_RETURN_MAX";
    default:    return "<unknown>";
  }
}

static Err err_mach_port_alloc(kern_return_t code) {
  if (code == KERN_SUCCESS) return nullptr;
  return errf(
    "unable to allocate Mach exception port; error: %d %s",
    code,
    mach_kern_code_name(code)
  );
}

static Err err_mach_port_send(kern_return_t code) {
  if (code == KERN_SUCCESS) return nullptr;
  return errf(
    "unable to make Mach exception port sendable; error: %d %s",
    code,
    mach_kern_code_name(code)
  );
}

/*
Requires `catch_mach_exception_raise_state` to be defined in the current
translation unit. See the signature in `./mach_exc_msg.c`.
*/
static void *mach_on_exceptions(void *) {
  mach_msg_server(mach_on_exception, MACH_EXC_MSG_MAX_SIZE, mach_exc_port, 0);
  return nullptr;
}

static Err mach_exception_init(exception_mask_t exception_types) {
  const auto task_port = mach_task_self(); // Current process.

  try(err_mach_port_alloc(
    mach_port_allocate(task_port, MACH_PORT_RIGHT_RECEIVE, &mach_exc_port)
  ));

  try(err_mach_port_send(mach_port_insert_right(
    task_port, mach_exc_port, mach_exc_port, MACH_MSG_TYPE_MAKE_SEND
  )));

  /*
  `MACH_EXCEPTION_CODES` -> 64-bit mode.

  `EXCEPTION_STATE` -> on exceptions, send the message `exception_raise_state`
  which includes the state of the faulting thread; we use this for recovery.
  */
  const auto flags = MACH_EXCEPTION_CODES | EXCEPTION_STATE;

  const auto code = task_set_exception_ports(
    task_port,
    exception_types,
    mach_exc_port,
    (exception_behavior_t)flags,
    ARM_THREAD_STATE64
  );

  if (code == KERN_SUCCESS) return nullptr;

  return errf(
    "unable to enable Mach exception handling; code: %d %s",
    code,
    mach_kern_code_name(code)
  );
}

static Err mach_exception_server_init(pthread_t *out) {
  pthread_t tmp;
  if (!out) out = &tmp;
  return err_direct(pthread_create(out, nullptr, mach_on_exceptions, nullptr));
}

static Err mach_exception_server_deinit(pthread_t val) {
  return err_direct(pthread_kill(val, SIGTERM));
}
