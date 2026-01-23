#include "./c_interp.c"
#include "./lib/cli.c"
#include "./lib/err.h"
#include "./lib/time.c"
#include <stdio.h>
#include <string.h>

// #include "./c_mach_exec.c"

#ifdef NATIVE_CALL_CONV
static Err init_exception_handling() { return nullptr; }
#else
#include "./c_mach_exc.c"
#endif

static Err run(int argc, const char **argv) {
  bool timing = false;
  try(env_bool("DEBUG", &DEBUG));
  try(env_bool("TIMING", &timing));

  Interp interp = {};
  try(interp_init(&interp));
  try(init_exception_handling());

  for (int ind = 1; ind < argc; ind++) {
    Timing time = {.prefix = "[import] "};
    if (timing) timing_beg(&time);

    try(interp_import(&interp, argv[ind]));

    if (timing) timing_end(&time);
  }

  // try(compile_mach_executable(&interp));
  return nullptr;
}

int main(int argc, const char **argv) { try_main(run(argc, argv)); }

/*

regular compiler: (C/C++ & full intrinsic knowledge) + program text ->
  compiler thinks hard -> machine code

regular interpreter: (C/C++ & full intrinsic knowledge) + program text ->
  compiler thinks little -> VM code -> interpreter loop -> interpret VM code

regular JIT compiler: (C/C++ & full intrinsic knowledge) + program text ->
  compiler thinks hard -> machine code -> runs

our JIT compiler: (C & minimal intrinsics) + program text ->
  compiler thinks little -> program runs and assembles -> machine code -> runs

*/
