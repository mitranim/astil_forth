#include "./c_interp.c"
#include "./c_mach_exc.c" // IWYU pragma: export
#include "./lib/cli.c"
#include "./lib/err.h"
#include "./lib/mach_exc.c"
#include <stdio.h>
#include <string.h>

// #include "./c_mach_exec.c"

static Err init_exception_handling() {
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

static Err run(int argc, const char **argv) {
  bool recovery = true;
  try(env_bool("DEBUG", &DEBUG));
  try(env_bool("RECOVERY", &recovery));

  Interp interp = {};

  try(interp_init(&interp));
  if (recovery) try(init_exception_handling());

  for (int ind = 1; ind < argc; ind++) {
    try(interp_include(&interp, argv[ind]));
  }

  // try(compile_mach_executable(&interp));
  return nullptr;
}

int main(int argc, const char **argv) { try_main(run(argc, argv)); }

/*

regular compiler: (C/C++ & full intrinsic knowledge) + program text ->
  compiler thinks hard -> machine code

regular interpreter: (C/C++ & full intrinsic knowledge) + program text ->
  compiler thinks little -> VM bytecode -> interpreter loop -> interpret bytecode

regular JIT compiler: (C/C++ & full intrinsic knowledge) + program text ->
  compiler thinks hard -> machine code -> runs

our JIT compiler: (C & minimal intrinsics) + program text ->
  compiler thinks little -> program runs and assembles -> machine code -> runs

*/
