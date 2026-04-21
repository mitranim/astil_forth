/*
AOT-compiles a Mach-O executable from the interpreter state,
using JIT-compiled code, static data, and symbol dictionaries.

This works because JIT-compiled code uses PC-relative addressing of data and
external symbols. Code, data, and symbol addresses are placed in `Comp_heap`.
We translate its memory layout into virtual memory segments described in the
executable, which allows offsets hardcoded inside instructions to work as-is.
External addresses are located in a GOT section, patched by the OS dylinker.

See `../clib/mach_o.h` for many useful links.

This is supported only for the register-based calling convention,
where interpreter state and Forth stack are not (supposed to be)
used by compiled code reachable from "main". For stack-CC, we'd
need to setup the stack, which is easily doable via a dedicated
Mach-O section; maybe later.
*/
#pragma once
#include "../clib/err.h"
#include "../clib/io.c"
#include "../clib/mach_codesign.c"
#include "../clib/mach_misc.c"
#include "../clib/mach_o.h"
#include "../clib/mem.c"
#include "../clib/mem.h"
#include "../clib/set.c"
#include "./arch.c"
#include "./comp.c"
#include "./interp.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static Err err_sym_interp_only(const Sym *sym, const Sym *dep) {
  return errf(
    "unable to compile: symbol " FMT_QUOTED " used by " FMT_QUOTED
    " is interpreter-only",
    dep->name.buf,
    sym->name.buf
  );
}

static Err validate_callees_can_compile(Sym_set *visited, const Sym *sym) {
  for (set_range(Ind, ind, &sym->callees)) {
    const auto dep = sym->callees.vals[ind];
    if (set_has(visited, dep)) continue;
    set_add(visited, dep);
    if (dep->interp_only) return err_sym_interp_only(sym, dep);
    try(validate_callees_can_compile(visited, dep));
  }
  return nullptr;
}

static void encode_got_section(const Comp *comp, Buf *buf) {
  const auto cap = stack_len(&comp->code.externs.names);

  for (Ind ind = 0; ind < cap; ind++) {
    // 4-byte distance to next fixup; 0 = end of chain.
    const U64 next = ind == cap - 1 ? 0 : 2;

    buf_append(
      buf,
      (Mach_fixup_ptr_64_bind){
        .ordinal = ind,
        .next    = next,
        .bind    = 1,
      }
    );
  }
}

/*
Build data for the command `MLC_DYLD_CHAINED_FIXUPS`. This will be loaded into
the executable under the segment name `__LINKEDIT`; we call this linkedit data.

See the comment on `Mach_fixup_head` for the structure of linkedit data.

`got_vm_off` points to the beginning of the `__got` section, which contains
a list of `Mach_fixup_ptr_64_bind` entries; these are address-sized structs
which point to their `Mach_fixup_import` entries in linkedit data, and will
be replaced with addresses by the dylinker.
*/
static void encode_linkedit_section(
  const Comp *comp, U64 text_seg_vm_off, U64 got_vm_off, Buf *buf
) {
  const auto code          = &comp->code;
  const auto exts          = &code->externs;
  const auto imports_count = (Ind)stack_len(&exts->names);
  const auto base_len      = buf->len;

  constexpr U32 img_off = sizeof(Mach_fixup_head);
  constexpr U32 seg_off = img_off + sizeof(Mach_fixup_img_5);
  constexpr U32 imp_off = seg_off + sizeof(Mach_fixup_seg);
  const U32     sym_off = imp_off + sizeof(Mach_fixup_import) * imports_count;

  buf_append(
    buf,
    (Mach_fixup_head){
      .starts_offset  = img_off,
      .imports_offset = imp_off,
      .symbols_offset = sym_off,
      .imports_count  = imports_count,
      .imports_format = MFI_IMPORT,
    }
  );

  buf_append(
    buf,
    (Mach_fixup_img_5){
      .seg_count          = 5,
      .seg_info_offset[0] = 0,                 // __PAGEZERO   → no fixup
      .seg_info_offset[1] = 0,                 // __TEXT       → no fixup
      .seg_info_offset[2] = 0,                 // __DATA       → no fixup
      .seg_info_offset[3] = seg_off - img_off, // __DATA_CONST → __got
      .seg_info_offset[4] = 0,                 // __LINKEDIT   → no fixup
    }
  );

  buf_append(
    buf,
    (Mach_fixup_seg){
      .size           = sizeof(Mach_fixup_seg),
      .page_size      = MEM_PAGE,
      .pointer_format = MFP_64_OFFSET,
      .segment_offset = got_vm_off - text_seg_vm_off, // from cctools and dyld
      .page_count     = 1,
      .page_start[0]  = 0,
    }
  );

  {
    // Index in the names-blob pointed to by `Mach_fixup_head.symbols_offset`.
    // Index 0 is the initial `\0`. The names-blob is appended below.
    U32 str_pos = 1;

    for (stack_range(auto, name, &exts->names)) {
      buf_append(
        buf,
        (Mach_fixup_import){
          .lib_ordinal = 1, // 1-indexed: first dylib we load (libSystem).
          .name_offset = str_pos,
        }
      );

      // Next name will be offset by this one's `_` + name + `\0`. See below.
      str_pos += 1 + name->len + 1;
    }
  }

  aver(buf->len - base_len == sym_off);

  /*
  Symbol names. Each `Mach_fixup_import` appended above contains an offset of
  its name in this chunk. Each name is `_` + name + `\0`; the underscore is a
  Mach-O convention for non-asm symbols.
  */
  buf_append_byte(buf, '\0');
  for (stack_range(auto, name, &exts->names)) {
    buf_append_byte(buf, '_');
    buf_append_bytes(buf, (const U8 *)name->buf, name->len);
    buf_append_byte(buf, '\0');
  }
}

static Err compile_mach_executable(Interp *interp, Buf *buf, const Sym *main) {
  try(comp_validate_main(main));

  const auto comp = &interp->comp;
  const auto code = &comp->code;
  try(comp_code_sync(code));

  {
    defer(set_deinit) Sym_set visited = {};
    try(validate_callees_can_compile(&visited, main));
  }

  /*
  We build the image piece-by-piece, but some of the sizes and offsets have to
  be calculated in advance, because they're specified in the headers upfront.
  */

  // Must match the commands below.
  constexpr auto cmd_count = 11;

  // Must match the commands below.
  constexpr U32 cmd_size = 0 +                      //
    sizeof(Mach_load_cmd_seg) +                     // __PAGEZERO
    sizeof(Mach_load_cmd_seg) + sizeof(Mach_sect) + // __TEXT
    sizeof(Mach_load_cmd_seg) + sizeof(Mach_sect) + // __DATA
    sizeof(Mach_load_cmd_seg) + sizeof(Mach_sect) + // __DATA_CONST
    sizeof(Mach_load_cmd_seg) +                     // __LINKEDIT
    sizeof(Mach_load_cmd_dylinker) +                // MLC_LOAD_DYLINKER
    sizeof(Mach_cmd_dylib) +                        // MLC_LOAD_DYLIB
    sizeof(Mach_load_cmd_linkedit) +                // MLC_DYLD_CHAINED_FIXUPS
    sizeof(Mach_load_cmd_main) +                    // MLC_MAIN
    sizeof(Mach_load_cmd_bui_ver) +                 // MLC_BUILD_VERSION
    sizeof(Mach_load_cmd_linkedit) +                // MLC_CODE_SIGNATURE
    0;

  buf_append(
    buf,
    (Mach_head){
      .magic      = 0xFEEDFACF,
      .cputype    = MCT_ARM64,
      .filetype   = MFT_EXECUTE,
      .flags      = MHF_NOUNDEFS | MHF_DYLDLINK | MHF_TWOLEVEL | MHF_PIE,
      .ncmds      = cmd_count,
      .sizeofcmds = cmd_size,
    }
  );

  constexpr U64 pagezero_vm_size = (U64)UINT32_MAX + 1;

  buf_append(
    buf,
    (Mach_load_cmd_seg){
      .head.cmd     = MLC_SEGMENT_64,
      .head.cmdsize = sizeof(Mach_load_cmd_seg),
      .segname      = "__PAGEZERO",
      .vmaddr       = 0,
      .vmsize       = pagezero_vm_size,
      .flags        = MSF_FORBID,
    }
  );

  /*
  The `__TEXT` command is special: its content includes the Mach-O headers
  followed by the `__text` section. C compilers also include more sections,
  such as stubs, constant string content, unwind info; our case is simpler.
  See the comment on `Mach_load_cmd_seg`. This is why the `__TEXT` segment
  has `fileoff = 0`: it begins at the headers. Like every segment, offsets
  and sizes of `__TEXT` must have page alignment.

  Since `__text` is a section, not a segment, normally it doesn't need page
  alignment and may begin immediately after headers. Some compilers reserve
  a bit of extra space between headers and `__text` to make post-processing
  of the executable easier; inserting a code signature is a notable example.
  In our case, `__text` needs page alignment because of `Comp_heap` mapping:

    Mach-O headers        -> __TEXT (0x320)
    Comp_heap.exec.instrs -> __text (0x100000)
    Comp_heap.data        -> __data (0x40000)
    Comp_heap.externs     -> __got  (0x4000)

  Mach-O seems to require the body of each segment load command to be padded
  to memory page size in the file too, not just in virtual memory, likely for
  straightforward memory mapping. This comes with a side effect: the offset of
  each segment in the file is aligned to memory page size, which we test later.
  Judging by `clang` and `codesign`, either `__LINKEDIT` or the last segment
  may be exempt from this, but we align it anyway.
  */
  constexpr U32 text_seg_file_off = 0;
  constexpr U64 text_seg_vm_off   = pagezero_vm_size;
  constexpr U32 text_code_file_off = mem_align_page(sizeof(Mach_head) + cmd_size);
  constexpr U64 text_code_vm_off = text_seg_vm_off + text_code_file_off;

  const auto instrs              = &code->code_exec;
  const U32  text_code_real_size = instrs->len * sizeof(Instr);

  const U32 text_seg_size = mem_align_page(
    text_code_file_off + text_code_real_size
  );

  aver(is_aligned_to(text_seg_size, MEM_PAGE));
  aver(is_aligned_to(text_seg_vm_off, MEM_PAGE));
  aver(is_aligned_to(text_seg_size, MEM_PAGE));

  buf_append(
    buf,
    (Mach_load_cmd_seg){
      .head.cmd     = MLC_SEGMENT_64,
      .head.cmdsize = sizeof(Mach_load_cmd_seg) + sizeof(Mach_sect),
      .segname      = "__TEXT",
      .vmaddr       = text_seg_vm_off,
      .vmsize       = text_seg_size,
      .fileoff      = text_seg_file_off,
      .filesize     = text_seg_size,
      .maxprot      = MP_READ | MP_EXEC,
      .initprot     = MP_READ | MP_EXEC,
      .nsects       = 1,
    }
  );

  buf_append(
    buf,
    (Mach_sect){
      .sectname = "__text",
      .segname  = "__TEXT",
      .addr     = text_code_vm_off,
      .size     = text_code_real_size,
      .offset   = text_code_file_off,
      .align    = 2, // 2^2
      .flags    = MST_REGULAR | MSA_PURE_INSTRUCTIONS | MSA_SOME_INSTRUCTIONS,
    }
  );

  // See comments above about segment size and alignment.
  U32 file_off = text_seg_size;

  const auto data_file_off  = file_off;
  const auto data_real_size = code->data.len;
  const auto data_file_size = mem_align_page(data_real_size);
  const auto data_vm_size   = data_file_size;

  /*
  Preserving the relative offsets between code and data allows JIT code to be
  used in AOT. Offsets are "hardcoded" in instructions such as `adrp & ldr`.
  The same approach is used for the GOT section below.
  */
  constexpr auto data_vm_off = text_code_vm_off + offsetof(Comp_heap, data) -
    offsetof(Comp_heap, exec.instrs);

  aver(text_code_vm_off + text_seg_size <= data_vm_off);

  aver(is_aligned_to(data_file_off, MEM_PAGE));
  aver(is_aligned_to(data_file_size, MEM_PAGE));
  aver(is_aligned_to(data_vm_off, MEM_PAGE));
  aver(is_aligned_to(data_vm_size, MEM_PAGE));

  file_off += data_file_size;

  buf_append(
    buf,
    (Mach_load_cmd_seg){
      .head.cmd     = MLC_SEGMENT_64,
      .head.cmdsize = sizeof(Mach_load_cmd_seg) + sizeof(Mach_sect),
      .segname      = "__DATA",
      .vmaddr       = data_vm_off,
      .vmsize       = data_vm_size,
      .fileoff      = data_file_off,
      .filesize     = data_file_size,
      .maxprot      = MP_READ | MP_WRITE,
      .initprot     = MP_READ | MP_WRITE,
      .nsects       = 1,
    }
  );

  buf_append(
    buf,
    (Mach_sect){
      .sectname = "__data",
      .segname  = "__DATA",
      .addr     = data_vm_off,
      .size     = data_real_size,
      .offset   = data_file_off,
      .align    = 3, // 2^3
    }
  );

  const auto extern_count  = code->externs.addrs.len;
  const auto got_file_off  = file_off;
  const auto got_real_size = extern_count * sizeof(U64);
  const auto got_file_size = mem_align_page(got_real_size);
  const auto got_vm_size   = mem_align_page(got_file_size);

  constexpr auto got_vm_off = text_code_vm_off + offsetof(Comp_heap, externs) -
    offsetof(Comp_heap, exec.instrs);

  aver(data_vm_off + data_vm_size <= got_vm_off);

  aver(is_aligned_to(got_file_off, MEM_PAGE));
  aver(is_aligned_to(got_file_size, MEM_PAGE));
  aver(is_aligned_to(got_vm_off, MEM_PAGE));
  aver(is_aligned_to(got_vm_size, MEM_PAGE));

  file_off += got_file_size;

  buf_append(
    buf,
    (Mach_load_cmd_seg){
      .head.cmd     = MLC_SEGMENT_64,
      .head.cmdsize = sizeof(Mach_load_cmd_seg) + sizeof(Mach_sect),
      .segname      = "__DATA_CONST",
      .vmaddr       = got_vm_off,
      .vmsize       = got_vm_size,
      .fileoff      = got_file_off,
      .filesize     = got_file_size,
      .maxprot      = MP_READ | MP_WRITE,
      .initprot     = MP_READ | MP_WRITE,
      .nsects       = 1,
      .flags        = MSF_READ_ONLY,
    }
  );

  buf_append(
    buf,
    (Mach_sect){
      .sectname = "__got",
      .segname  = "__DATA_CONST",
      .addr     = got_vm_off,
      .size     = got_real_size,
      .offset   = got_file_off,
      .align    = 3, // 2^3
      .flags    = MST_NON_LAZY_SYMBOL_POINTERS,
    }
  );

  defer(buf_deinit) Buf linkedit = {};
  encode_linkedit_section(comp, text_seg_vm_off, got_vm_off, &linkedit);

  const auto linkedit_file_off  = file_off;
  const auto linkedit_real_size = linkedit.len;
  const auto linkedit_file_size = mem_align_page(linkedit_real_size);
  const auto linkedit_vm_off    = got_vm_off + got_vm_size;
  const auto linkedit_vm_size   = linkedit_file_size;

  file_off += linkedit_file_size;

  buf_append(
    buf,
    (Mach_load_cmd_seg){
      .head.cmd     = MLC_SEGMENT_64,
      .head.cmdsize = sizeof(Mach_load_cmd_seg),
      .segname      = "__LINKEDIT",
      .vmaddr       = linkedit_vm_off,
      .vmsize       = linkedit_vm_size,
      .fileoff      = linkedit_file_off,
      .filesize     = linkedit_real_size,
      .maxprot      = MP_READ,
      .initprot     = MP_READ,
    }
  );

  buf_append(
    buf,
    (Mach_load_cmd_dylinker){
      .head.cmd     = MLC_LOAD_DYLINKER,
      .head.cmdsize = sizeof(Mach_load_cmd_dylinker),
      .name_offset  = offsetof(Mach_load_cmd_dylinker, name),
      .name         = "/usr/lib/dyld",
    }
  );

  buf_append(
    buf,
    (Mach_cmd_dylib){
      .head.cmd     = MLC_LOAD_DYLIB,
      .head.cmdsize = sizeof(Mach_cmd_dylib),
      .name_offset  = offsetof(Mach_cmd_dylib, name),
      .timestamp    = 2,                    // Copied from Clang's output.
      .cur_ver      = mach_ver(1351, 0, 0), // Copied from Clang's output.
      .compat_ver   = mach_ver(1, 0, 0),    // Copied from Clang's output.
      .name         = "/usr/lib/libSystem.B.dylib",
    }
  );

  buf_append(
    buf,
    (Mach_load_cmd_linkedit){
      .head.cmd     = MLC_DYLD_CHAINED_FIXUPS,
      .head.cmdsize = sizeof(Mach_load_cmd_linkedit),
      .dataoff      = linkedit_file_off,
      .datasize     = linkedit_real_size,
    }
  );

  {
    const auto main_code_off = main->norm.spans.prologue * sizeof(Instr);
    const auto main_off      = text_code_file_off + main_code_off;

    buf_append(
      buf,
      (Mach_load_cmd_main){
        .head.cmd     = MLC_MAIN,
        .head.cmdsize = sizeof(Mach_load_cmd_main),
        .entryoff     = main_off,
      }
    );
  }

  // Seems optional.
  buf_append(
    buf,
    (Mach_load_cmd_bui_ver){
      .head.cmd     = MLC_BUILD_VERSION,
      .head.cmdsize = sizeof(Mach_load_cmd_bui_ver),
      .platform     = MBP_MACOS,
      .minos        = mach_ver(11, 0, 0), // Lowest version for Apple Silicon.
    }
  );

  constexpr char sig_id[]     = "f9de984d6c2b47dfbb5bb20441933fe3";
  constexpr auto sig_id_len   = arr_cap(sig_id) - 1;
  const auto     sig_file_off = file_off;
  const auto sig_file_size = mach_codesign_total_size(sig_file_off, sig_id_len);

  buf_append(
    buf,
    (Mach_load_cmd_linkedit){
      .head.cmd     = MLC_CODE_SIGNATURE,
      .head.cmdsize = sizeof(Mach_load_cmd_linkedit),
      .dataoff      = sig_file_off,
      .datasize     = sig_file_size,
    }
  );

  // Done writing headers; pad up to `__text` and write various sections.

  aver(buf->len == sizeof(Mach_head) + cmd_size);
  aver(sizeof(Mach_head) + cmd_size <= text_code_file_off);

  buf_zeropad_to(buf, text_code_file_off);
  aver(buf->len == text_code_file_off);

  aver(text_code_real_size == instrs->len * sizeof(Instr));
  buf_append_bytes(buf, (const U8 *)instrs->dat, text_code_real_size);
  aver(buf->len == text_code_file_off + text_code_real_size);

  aver(buf->len <= data_file_off);
  buf_zeropad_to(buf, data_file_off);
  aver(buf->len == data_file_off);

  aver(data_real_size == code->data.len);
  buf_append_bytes(buf, code->data.dat, data_real_size);
  aver(buf->len == data_file_off + data_real_size);

  aver(buf->len <= got_file_off);
  buf_zeropad_to(buf, got_file_off);
  aver(buf->len == got_file_off);

  encode_got_section(comp, buf);

  aver(buf->len <= linkedit_file_off);
  aver(linkedit_file_off == got_file_off + got_file_size);
  buf_zeropad_to(buf, linkedit_file_off);
  aver(buf->len == linkedit_file_off);

  buf_append_bytes(buf, linkedit.dat, linkedit.len);
  aver(buf->len == linkedit_file_off + linkedit.len);
  buf_zeropad_to(buf, sig_file_off);
  aver(buf->len == sig_file_off);

  // Ad-hoc signature.
  mach_codesign((Mach_codesign_cfg){
    .buf       = buf,
    .src       = buf->dat,
    .src_len   = buf->len,
    .id        = sig_id,
    .id_len    = sig_id_len,
    .text_off  = text_seg_file_off,
    .text_size = text_seg_size,
    .is_main   = true,
  });

  return nullptr;

  /*
  Also possible to codesign like this, but we prefer not shelling out:
  {
    char *const argv[] = {"codesign", "-s", "-", "-f", "out.exe", nullptr};
    pid_t       pid;
    int         status;
    try(spawn_with_stdin(argv, nullptr, 0, &pid));
    try(wait_pid(pid, &status));
    if (status) return errf("codesign exited with status %d", status);
  }
  */
}

static Err compile_mach_executable_to(
  Interp *interp, const char *path, const Sym *main
) {
  defer(buf_deinit) Buf buf = {};

  try(compile_mach_executable(interp, &buf, main));

  FILE *file;
  try(file_open(path, "w", &file));
  try(file_write(file, buf.dat, 1, buf.len));
  try_errno(fflush(file));
  try_errno(fchmod(fileno(file), 0755));
  try_errno(fclose(file));

  return nullptr;
}
