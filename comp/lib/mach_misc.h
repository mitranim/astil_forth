#pragma once
#include "./num.h"
#include <assert.h>
#include <mach/mach.h>

// Darwin-specific. TODO better segregation of CPU stuff vs OS stuff.
typedef struct {
  U64 x[29];
  U64 fp;
  U64 lr;
  U64 sp;
  U64 pc;
  U32 cpsr;
  U32 pad;
} Mach_thread_state;

/*
Internal sanity check. Our `Mach_thread_state` redefines `arm_thread_state64_t`
using same-sized but differently-typed integers for better repr printing.
*/
static_assert(sizeof(Mach_thread_state) == sizeof(arm_thread_state64_t));
