#pragma once
#include "../lib/fmt.c"
#include "../lib/mach_misc.c"
#include "../lib/mach_o.h"

static void Mach_head_repr(Mach_head *val) {
  print_struct_beg(val, Mach_head);
  print_struct_field(val, magic);
  print_struct_field(val, cpu_type);
  print_struct_field(val, cpu_subtype);
  print_struct_field(val, file_type);
  print_struct_field(val, cmd_count);
  print_struct_field(val, cmd_size);
  print_struct_field(val, flags);
  print_struct_field(val, reserved);
  print_struct_end();
}

static void Mach_load_cmd_seg_repr(Mach_load_cmd_seg *val) {
  print_struct_beg(val, Mach_load_cmd_seg);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, segname);
  print_struct_field(val, vmaddr);
  print_struct_field(val, vmsize);
  print_struct_field(val, fileoff);
  print_struct_field(val, filesize);
  print_struct_field(val, maxprot);
  print_struct_field(val, initprot);
  print_struct_field(val, nsects);
  print_struct_field(val, flags);
  print_struct_end();
}

static void Mach_sect_repr(Mach_sect *val) {
  print_struct_beg(val, Mach_sect);
  print_struct_field(val, sectname);
  print_struct_field(val, segname);
  print_struct_field(val, addr);
  print_struct_field(val, size);
  print_struct_field(val, offset);
  print_struct_field(val, align);
  print_struct_field(val, reloff);
  print_struct_field(val, nreloc);
  print_struct_field(val, flags);
  print_struct_field(val, reserved1);
  print_struct_field(val, reserved2);
  print_struct_field(val, reserved3);
  print_struct_end();
}

static void Mach_load_cmd_linkedit_repr(Mach_load_cmd_linkedit *val) {
  print_struct_beg(val, Mach_load_cmd_linkedit);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, dataoff);
  print_struct_field(val, datasize);
  print_struct_end();
}

static void Mach_load_cmd_symtab_repr(Mach_load_cmd_symtab *val) {
  print_struct_beg(val, Mach_load_cmd_symtab);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, symoff);
  print_struct_field(val, nsyms);
  print_struct_field(val, stroff);
  print_struct_field(val, strsize);
  print_struct_end();
}

static void Mach_load_cmd_dysymtab_repr(Mach_load_cmd_dysymtab *val) {
  print_struct_beg(val, Mach_load_cmd_dysymtab);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, ilocalsym);
  print_struct_field(val, nlocalsym);
  print_struct_field(val, iextdefsym);
  print_struct_field(val, nextdefsym);
  print_struct_field(val, iundefsym);
  print_struct_field(val, nundefsym);
  print_struct_field(val, tocoff);
  print_struct_field(val, ntoc);
  print_struct_field(val, modtaboff);
  print_struct_field(val, nmodtab);
  print_struct_field(val, extrefsymoff);
  print_struct_field(val, nextrefsyms);
  print_struct_field(val, indirectsymoff);
  print_struct_field(val, nindirectsyms);
  print_struct_field(val, extreloff);
  print_struct_field(val, nextrel);
  print_struct_field(val, locreloff);
  print_struct_field(val, nlocrel);
  print_struct_end();
}

static void Mach_load_cmd_dyld_repr(Mach_load_cmd_dyld *val) {
  print_struct_beg(val, Mach_load_cmd_dyld);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, name_offset);
  print_struct_end();

  const auto str_len = val->head.cmdsize - val->name_offset;
  printf("Mach_load_cmd_dyld->name: %.*s\n", (int)str_len, val->name);
}

static void Mach_load_cmd_uuid_repr(Mach_load_cmd_uuid *val) {
  print_struct_beg(val, Mach_load_cmd_uuid);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, uuid);
  print_struct_end();
}

static void Mach_load_cmd_bui_ver_repr(Mach_load_cmd_bui_ver *val) {
  print_struct_beg(val, Mach_load_cmd_bui_ver);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, platform);
  printf(
    "  .minos = mach_ver_from_str(" FMT_QUOTED "),\n",
    mach_ver_to_str(val->minos)
  );
  printf(
    "  .sdk = mach_ver_from_str(" FMT_QUOTED "),\n", mach_ver_to_str(val->sdk)
  );
  print_struct_field(val, ntools);
  print_struct_end();
}

static void Mach_bui_ver_repr(Mach_bui_ver *val) {
  print_struct_beg(val, Mach_bui_ver);
  print_struct_field(val, tool);
  printf(
    "  .version = mach_ver_from_str(" FMT_QUOTED "),\n",
    mach_ver_to_str(val->version)
  );
  print_struct_end();
}

static void Mach_load_cmd_src_ver_repr(Mach_load_cmd_src_ver *val) {
  print_struct_beg(val, Mach_load_cmd_src_ver);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, version);
  print_struct_end();
}

static void Mach_load_cmd_main_repr(Mach_load_cmd_main *val) {
  print_struct_beg(val, Mach_load_cmd_main);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, entryoff);
  print_struct_field(val, stacksize);
  print_struct_end();
}

static void Mach_load_cmd_dylib_repr(Mach_load_cmd_dylib *val) {
  print_struct_beg(val, Mach_load_cmd_dylib);
  print_struct_field(val, head.cmd);
  print_struct_field(val, head.cmdsize);
  print_struct_field(val, head);
  print_struct_field(val, name_offset);
  print_struct_field(val, timestamp);
  printf(
    "  .cur_ver = mach_ver_from_str(" FMT_QUOTED "),\n",
    mach_ver_to_str(val->cur_ver)
  );
  printf(
    "  .compat_ver = mach_ver_from_str(" FMT_QUOTED "),\n",
    mach_ver_to_str(val->compat_ver)
  );
  print_struct_end();

  const auto str_len = val->head.cmdsize - val->name_offset;
  printf("Mach_load_cmd_dyld->name: %.*s\n", (int)str_len, val->name);
}
