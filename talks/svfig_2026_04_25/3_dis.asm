/*
astil examples/aot_cli.af --build=out.exe

./out.exe

pagestuff -a out.exe
otool -hvl out.exe
make disasm file=out.exe && open local/out.s
*/

out.exe:  file format mach-o arm64
Mach header
      magic cputype cpusubtype  caps    filetype ncmds sizeofcmds      flags
MH_MAGIC_64   ARM64        ALL  0x00     EXECUTE    11        768   NOUNDEFS DYLDLINK TWOLEVEL PIE
Load command 0
      cmd LC_SEGMENT_64
  cmdsize 72
  segname __PAGEZERO
   vmaddr 0x0000000000000000
   vmsize 0x0000000100000000
  fileoff 0
 filesize 0
  maxprot ---
 initprot ---
   nsects 0
    flags (none)
Load command 1
      cmd LC_SEGMENT_64
  cmdsize 152
  segname __TEXT
   vmaddr 0x0000000100000000
   vmsize 0x0000000000010000
  fileoff 0
 filesize 65536
  maxprot r-x
 initprot r-x
   nsects 1
    flags (none)
Section
  sectname __text
   segname __TEXT
      addr 0x0000000100004000
      size 0x000000000000aacc
    offset 16384
     align 2^2 (4)
    reloff 0
    nreloc 0
      type S_REGULAR
attributes PURE_INSTRUCTIONS SOME_INSTRUCTIONS
 reserved1 0
 reserved2 0
Load command 2
      cmd LC_SEGMENT_64
  cmdsize 152
  segname __DATA
   vmaddr 0x000000010040c000
   vmsize 0x0000000000004000
  fileoff 65536
 filesize 16384
  maxprot rw-
 initprot rw-
   nsects 1
    flags (none)
Section
  sectname __data
   segname __DATA
      addr 0x000000010040c000
      size 0x000000000000259c
    offset 65536
     align 2^3 (8)
    reloff 0
    nreloc 0
      type S_REGULAR
attributes (none)
 reserved1 0
 reserved2 0
Load command 3
      cmd LC_SEGMENT_64
  cmdsize 152
  segname __DATA_CONST
   vmaddr 0x0000000100450000
   vmsize 0x0000000000004000
  fileoff 81920
 filesize 16384
  maxprot rw-
 initprot rw-
   nsects 1
    flags SG_READ_ONLY
Section
  sectname __got
   segname __DATA_CONST
      addr 0x0000000100450000
      size 0x0000000000000128
    offset 81920
     align 2^3 (8)
    reloff 0
    nreloc 0
      type S_NON_LAZY_SYMBOL_POINTERS
attributes (none)
 reserved1 0 (index into indirect symbol table)
 reserved2 0
Load command 4
      cmd LC_SEGMENT_64
  cmdsize 72
  segname __LINKEDIT
   vmaddr 0x0000000100454000
   vmsize 0x0000000000004000
  fileoff 98304
 filesize 552
  maxprot r--
 initprot r--
   nsects 0
    flags (none)
Load command 5
          cmd LC_LOAD_DYLINKER
      cmdsize 32
         name /usr/lib/dyld (offset 12)
Load command 6
          cmd LC_LOAD_DYLIB
      cmdsize 56
         name /usr/lib/libSystem.B.dylib (offset 24)
   time stamp 2 Thu Jan  1 01:00:02 1970
      current version 1351.0.0
compatibility version 1.0.0
Load command 7
      cmd LC_DYLD_CHAINED_FIXUPS
  cmdsize 16
  dataoff 98304
 datasize 552
Load command 8
       cmd LC_MAIN
   cmdsize 24
  entryoff 60040
 stacksize 0
Load command 9
       cmd LC_BUILD_VERSION
   cmdsize 24
  platform macos
       sdk n/a
     minos 11.0
    ntools 0
Load command 10
      cmd LC_CODE_SIGNATURE
  cmdsize 16
  dataoff 114688
 datasize 1037

Sections:
Idx Name          Size     VMA              Type
  0 __text        0000aacc 0000000100004000 TEXT
  1 __data        0000259c 000000010040c000 DATA
  2 __got         00000128 0000000100450000 DATA

SYMBOL TABLE:

DYNAMIC SYMBOL TABLE:

/*
Dump of machine code JIT-compiled from Forth.

"Just works" in AOT-compiled mode because our code is position-independent.
Instructions in `__text` use PC-relative offsets for addressing other pages
and sections, namely `__data` and `__got`. When creating an AOT executable,
we instruct the OS to use the same virtual memory offsets for the sections,
as the ones we use in JIT execution.
*/
Disassembly of section __TEXT,__text:
0000000100004000 <__text>:
// ... elided for brevity's sake ...
// ... tons of lines here ...

/*
Dump of compiler-managed data memory requested in Forth with `alloc_data`.
All changes to this memory made during interpretation are preserved here.
*/
Disassembly of section __DATA,__data:
000000010040c000 <__data>:
// ... elided for brevity's sake ...
// ... tons of lines here ...

/*
Addresses of external symbols patched by the dylinker when starting up.
*/
Disassembly of section __DATA_CONST,__got:
0000000100450000 <__got>:
// ... elided for brevity's sake ...
// ... tons of lines here ...
