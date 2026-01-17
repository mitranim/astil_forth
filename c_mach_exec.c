/*
Unfinished sketch for building a Mach-O executable
from a running (JIT-compiled) program.

TODO:
- Map `Asm_heap` to Mach-O segments and sections.
  - Compactify by eliding all unused memory.
    Virtual memory addressing lets us do that.
  - Guards are automatic; just leave unmapped holes.
- Build symbol table:
  - Our symbols.
  - Extern symbols.
- Add a section for the Forth stack.
  - Prepend a prologue which sets up the stack registers.
*/

#pragma once
#include "./c_interp.h"
#include "./lib/dict.c"
#include "./lib/err.h"
#include "./lib/io.c"
#include "./lib/mach_misc.c"
#include "./lib/mach_o.h"
#include "./lib/mem.c"
#include "./lib/misc.h"
#include "./lib/num.h"
#include "./lib/set.c"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslimits.h>

/*
Links:

- https://github.com/apple-oss-distributions/xnu/tree/main/EXTERNAL_HEADERS/mach-o/loader.h
- https://en.wikipedia.org/wiki/Mach-O
*/

// static Err compile_exec_sym(FILE *out, Sym const *sym) {}

static Err err_sym_interp_only(const Sym *sym, const Sym *dep) {
  return errf(
    "unable to compile: symbol " FMT_QUOTED " used by " FMT_QUOTED
    " is interpreter-only",
    dep->name.buf,
    sym->name.buf
  );
}

static Err validate_callees_can_compile(Sym_set *visited, Sym *sym) {
  for (set_range(Ind, ind, &sym->callees)) {
    const auto dep = sym->callees.vals[ind];

    if (set_has(visited, dep)) continue;
    set_add(visited, dep);

    if (dep->interp_only) return err_sym_interp_only(sym, dep);
    try(validate_callees_can_compile(visited, dep));
  }
  return nullptr;
}

static Err compile_mach_executable(Interp *interp) {
  const auto dict = &interp->words;
  const auto main = dict_get(dict, "main");

  if (!main) {
    return err_str("unable to build executable: word \"main\" is not defined");
  }

  // const auto syms = &interp->syms;
  const auto exec     = &interp->asm.exec;
  const auto exec_len = stack_len(exec);

  defer(set_deinit) Sym_set visited = {};
  try(validate_callees_can_compile(&visited, main));

  defer(bytes_deinit) U8 *buf = nullptr;
  Uint                    len;
  const auto              str = open_memstream((char **)&buf, &len);

  {
    const auto head = (Mach_head){
      .magic     = 0xFEEDFACF,
      .cpu_type  = MCT_ARM64,
      .file_type = MFT_EXECUTE,
      .flags     = MHF_NOUNDEFS | MHF_DYLDLINK | MHF_TWOLEVEL | MHF_PIE |
        MHF_HAS_TLV_DESCRIPTORS,
      .cmd_count = 1,
      .cmd_size  = sizeof(Mach_load_cmd_seg), // FIXME there's more
    };
    try(file_stream_write(str, &head, sizeof(head), 1));
  }

  // Common trick; catches many invalid pointer derefs.
  {
    const auto cmd = (Mach_load_cmd_seg){
      .head.cmd     = MLC_SEGMENT_64,
      .head.cmdsize = sizeof(Mach_load_cmd_seg),
      .segname      = "__PAGEZERO",
      .vmaddr       = 0,
      .vmsize       = UINT32_MAX,
      // .flags        = 0, // access = segfault
    };
    try(file_stream_write(str, &cmd, sizeof(cmd), 1));
  }

  {
    const Uint            BASE_ADDR = (U64)UINT32_MAX + 1;
    static constexpr auto MACH_RX   = MP_READ | MP_EXEC;

    const auto cmd = (Mach_load_cmd_seg){
      .head.cmd     = MLC_SEGMENT_64,
      .head.cmdsize = sizeof(Mach_load_cmd_seg) + sizeof(Mach_sect),
      .segname      = "__TEXT",
      .vmaddr       = BASE_ADDR,

      // .vmsize = {0},
      // .fileoff = {0},
      // .filesize = {0},

      .maxprot  = MACH_RX,
      .initprot = MACH_RX,
      .nsects   = 1,
    };
    try(file_stream_write(str, &cmd, sizeof(cmd), 1));
  }

  {
    const auto sect = (Mach_sect){
      .sectname = "__text",
      .segname  = "__TEXT",
      // .addr     = {0},
      // .size     = {0},
      // .offset   = {0},
      // .align    = {0},
      // .reloff   = {0},
      // .nreloc   = {0},
      .flags = MST_REGULAR | MSA_SOME_INSTRUCTIONS,
    };
    try(file_stream_write(str, &sect, sizeof(sect), 1));
  }

  {
    const auto main_off = (U8 *)main->norm.exec.floor - (U8 *)exec->floor;
    aver(main_off >= 0);

    const auto cmd = (Mach_load_cmd_main){
      .head.cmd     = MLC_MAIN,
      .head.cmdsize = sizeof(Mach_load_cmd_main),
      .entryoff     = (U64)main_off,
    };
    try(file_stream_write(str, &cmd, sizeof(cmd), 1));
  }

  {
    static constexpr char name[16] = "/usr/lib/dyld";
    const auto            size     = sizeof(Mach_load_cmd_dyld);

    const auto cmd = (Mach_load_cmd_dyld){
      .head.cmd     = MLC_LOAD_DYLINKER,
      .head.cmdsize = size + sizeof(name),
      .name_offset  = size,
    };

    try(file_stream_write(str, &cmd, sizeof(cmd), 1));
    try(file_stream_write(str, name, sizeof(name), 1));
  }

  {
    static constexpr char name[32] = "/usr/lib/libSystem.B.dylib";
    const auto            size     = sizeof(Mach_load_cmd_dylib);

    const auto cmd = (Mach_load_cmd_dylib){
      .head.cmd     = MLC_LOAD_DYLIB,
      .head.cmdsize = size + sizeof(name),
      .name_offset  = size,
      .cur_ver      = mach_ver(1351, 0, 0),
      .compat_ver   = mach_ver(1, 0, 0),
    };

    try(file_stream_write(str, &cmd, sizeof(cmd), 1));
    try(file_stream_write(str, name, sizeof(name), 1));
  }

  /*
  This dump includes "dead" code used only during interpretation / compilation.
  Removing it would require us to relocate all PC-relative offsets in compiled
  code. Easy enough but requires additional book-keeping. Stay simple for now.
  */
  try(file_stream_write(str, exec->floor, stack_val_size(exec), exec_len));

  /*
  Now missing:
  - Linker info for extern procedure calls.
  - Handle error string returned by `main`, if any.
    - Print string and exit with non-0.
  - `exit` syscall.
  */

  /*
  for (set_range(auto, dep, &main->deps)) {
    if (set_has(&visited, dep)) continue;
    set_add(&visited, dep);
    try(compile_exec_sym(str, *dep));
  }
  */

  try_errno(fclose(str));
  return nullptr;
}
