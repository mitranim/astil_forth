/*
Links:

- https://github.com/apple-oss-distributions/xnu/blob/main/EXTERNAL_HEADERS/mach-o/loader.h
- https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/mach_loader.c
- https://github.com/apple-oss-distributions/cctools/blob/main/include/mach-o/loader.h
- https://github.com/apple-oss-distributions/cctools/blob/main/otool/fixup-chains.h
- https://github.com/apple-oss-distributions/cctools/blob/main/include/mach/machine-cctools.h
- https://blog.xpnsec.com/building-a-mach-o-memory-loader-part-1/
- https://www.emergetools.com/blog/posts/iOS15LaunchTime
- https://en.wikipedia.org/wiki/Mach-O
- (outdated) https://owenbrooks.org/blog/constructing-a-valid-macho/
- (outdated) https://web.archive.org/web/20140911185310/https://developer.apple.com/library/mac/documentation/DeveloperTools/Conceptual/MachORuntime/Reference/reference.html
- (outdated) https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/Articles/MachOOverview.html

Commands:

  otool -hvl out.exe
  otool -hvl -chained_fixups out.exe
  pagestuff -a out.exe
  codesign -dvvv out.exe
  llvm-objdump --disassemble-all --headers --private-headers --reloc --dynamic-reloc --syms --dynamic-syms out.exe
*/

#pragma once
#include "./misc.h"
#include "./num.h"

// #include <nlist.h>
// #include <stab.h>

static constexpr U32 MACH_CPU_ARCH_ABI64 = 1 << 24;

typedef enum : U32 {
  MCT_X64   = 0x07 | MACH_CPU_ARCH_ABI64,
  MCT_ARM64 = 0x0C | MACH_CPU_ARCH_ABI64,
} Mach_cpu_type;

typedef enum : U32 {
  MFT_OBJECT      = 0x01,
  MFT_EXECUTE     = 0x02,
  MFT_FVMLIB      = 0x03,
  MFT_CORE        = 0x04,
  MFT_PRELOAD     = 0x05,
  MFT_DYLIB       = 0x06,
  MFT_DYLINKER    = 0x07,
  MFT_BUNDLE      = 0x08,
  MFT_DYLIB_STUB  = 0x09,
  MFT_DSYM        = 0x0A,
  MFT_KEXT_BUNDLE = 0x0B,
  MFT_FILESET     = 0x0C,
} Mach_file_type;

typedef enum : U32 {
  MHF_NOUNDEFS                      = 1 << 0,
  MHF_INCRLINK                      = 1 << 1,
  MHF_DYLDLINK                      = 1 << 2,
  MHF_BINDATLOAD                    = 1 << 3,
  MHF_PREBOUND                      = 1 << 4,
  MHF_SPLIT_SEGS                    = 1 << 5,
  MHF_LAZY_INIT                     = 1 << 6,
  MHF_TWOLEVEL                      = 1 << 7,
  MHF_FORCE_FLAT                    = 1 << 8,
  MHF_NOMULTIDEFS                   = 1 << 9,
  MHF_NOFIXPREBINDING               = 1 << 10,
  MHF_PREBINDABLE                   = 1 << 11,
  MHF_ALLMODSBOUND                  = 1 << 12,
  MHF_SUBSECTIONS_VIA_SYMBOLS       = 1 << 13,
  MHF_CANONICAL                     = 1 << 14,
  MHF_WEAK_DEFINES                  = 1 << 15,
  MHF_BINDS_TO_WEAK                 = 1 << 16,
  MHF_ALLOW_STACK_EXECUTION         = 1 << 17,
  MHF_ROOT_SAFE                     = 1 << 18,
  MHF_SETUID_SAFE                   = 1 << 19,
  MHF_NO_REEXPORTED_DYLIBS          = 1 << 20,
  MHF_PIE                           = 1 << 21,
  MHF_DEAD_STRIPPABLE_DYLIB         = 1 << 22,
  MHF_HAS_TLV_DESCRIPTORS           = 1 << 23,
  MHF_NO_HEAP_EXECUTION             = 1 << 24,
  MHF_APP_EXTENSION_SAFE            = 1 << 25,
  MHF_NLIST_OUTOFSYNC_WITH_DYLDINFO = 1 << 26,
  MHF_SIM_SUPPORT                   = 1 << 27,
  MHF_DYLIB_IN_CACHE                = (U32)1 << 31,
} Mach_head_flag;

typedef enum : U32 {
  MLC_REQ_DYLD                 = (U32)1 << 31,
  MLC_SEGMENT                  = 0x01,
  MLC_SYMTAB                   = 0x02,
  MLC_SYMSEG                   = 0x03,
  MLC_THREAD                   = 0x04,
  MLC_UNIXTHREAD               = 0x05,
  MLC_LOADFVMLIB               = 0x06,
  MLC_IDFVMLIB                 = 0x07,
  MLC_IDENT                    = 0x08,
  MLC_FVMFILE                  = 0x09,
  MLC_PREPAGE                  = 0x0A,
  MLC_DYSYMTAB                 = 0x0B,
  MLC_LOAD_DYLIB               = 0x0C,
  MLC_ID_DYLIB                 = 0x0D,
  MLC_LOAD_DYLINKER            = 0x0E,
  MLC_ID_DYLINKER              = 0x0F,
  MLC_PREBOUND_DYLIB           = 0x10,
  MLC_ROUTINES                 = 0x11,
  MLC_SUB_FRAMEWORK            = 0x12,
  MLC_SUB_UMBRELLA             = 0x13,
  MLC_SUB_CLIENT               = 0x14,
  MLC_SUB_LIBRARY              = 0x15,
  MLC_TWOLEVEL_HINTS           = 0x16,
  MLC_PREBIND_CKSUM            = 0x17,
  MLC_LOAD_WEAK_DYLIB          = (0x18 | MLC_REQ_DYLD),
  MLC_SEGMENT_64               = 0x19,
  MLC_ROUTINES_64              = 0x1A,
  MLC_UUID                     = 0x1B,
  MLC_RPATH                    = (0x1C | MLC_REQ_DYLD),
  MLC_CODE_SIGNATURE           = 0x1D,
  MLC_SEGMENT_SPLIT_INFO       = 0x1E,
  MLC_REEXPORT_DYLIB           = (0x1F | MLC_REQ_DYLD),
  MLC_LAZY_LOAD_DYLIB          = 0x20,
  MLC_ENCRYPTION_INFO          = 0x21,
  MLC_DYLD_INFO                = 0x22,
  MLC_DYLD_INFO_ONLY           = (0x22 | MLC_REQ_DYLD),
  MLC_LOAD_UPWARD_DYLIB        = (0x23 | MLC_REQ_DYLD),
  MLC_VERSION_MIN_MACOSX       = 0x24,
  MLC_VERSION_MIN_IPHONEOS     = 0x25,
  MLC_FUNCTION_STARTS          = 0x26,
  MLC_DYLD_ENVIRONMENT         = 0x27,
  MLC_MAIN                     = (0x28 | MLC_REQ_DYLD),
  MLC_DATA_IN_CODE             = 0x29,
  MLC_SOURCE_VERSION           = 0x2A,
  MLC_DYLIB_CODE_SIGN_DRS      = 0x2B,
  MLC_ENCRYPTION_INFO_64       = 0x2C,
  MLC_LINKER_OPTION            = 0x2D,
  MLC_LINKER_OPTIMIZATION_HINT = 0x2E,
  MLC_VERSION_MIN_TVOS         = 0x2F,
  MLC_VERSION_MIN_WATCHOS      = 0x30,
  MLC_NOTE                     = 0x31,
  MLC_BUILD_VERSION            = 0x32,
  MLC_DYLD_EXPORTS_TRIE        = (0x33 | MLC_REQ_DYLD),
  MLC_DYLD_CHAINED_FIXUPS      = (0x34 | MLC_REQ_DYLD),
  MLC_FILESET_ENTRY            = (0x35 | MLC_REQ_DYLD),
  MLC_ATOM_INFO                = 0x36,
  MLC_FUNCTION_VARIANTS        = 0x37,
  MLC_FUNCTION_VARIANT_FIXUPS  = 0x38,
  MLC_TARGET_TRIPLE            = 0x39,
} Mach_load_cmd_type;

// `mach_header_64` in Apple `cctools`.
typedef struct {
  U32            magic;
  Mach_cpu_type  cputype;
  U32            cpusubtype;
  Mach_file_type filetype;
  U32            ncmds;
  U32            sizeofcmds;
  U32            flags;
  U32            reserved;
} Mach_head; // 64-bit

typedef enum : U32 {
  MP_NONE  = 0,
  MP_READ  = 1 << 0,
  MP_WRITE = 1 << 1,
  MP_EXEC  = 1 << 2,
} Mach_prot;

typedef struct {
  Mach_load_cmd_type cmd;
  U32                cmdsize; // in bytes
} Mach_load_cmd_head;

typedef enum : U32 {
  MSF_FORBID              = 0x00,
  MSF_HIGHVM              = 0x01,
  MSF_FVMLIB              = 0x02,
  MSF_NORELOC             = 0x04,
  MSF_PROTECTED_VERSION_1 = 0x08,
  MSF_READ_ONLY           = 0x10,
} Mach_load_cmd_seg_flag;

/*
`segment_command_64` in Apple `cctools`.

`fileoff` is the offset of this load command's body from the start of the Mach-O
file, and `filesize` is the body's size within the file. For load commands with
one or several sections, the `Mach_sect.offset` of the first section usually
matches `Mach_load_cmd_seg.fileoff`.

`vmaddr` is where the body starts in virtual memory, and `vmsize` is the size
of the virtual memory segment, often larger than `filesize`. Virtual memory is
chunked into "pages" of platform-dependent size (see `MEM_PAGE`). `vmaddr` has
to be aligned to memory page size, and `vmsize` has to be padded up to it.

The `__TEXT` command is special: it's obligated to reserve space for Mach-O
headers before its `__text` section. Its `fileoff` is 0, meaning: it begins
at the start of the Mach-O file. `filesize` includes `Mach_head.sizeofcmds`
and the size of the `__text` section. The `__text` section's `offset` does
not match `fileoff`; it's the offset of the executable instruction payload
following after the headers. This rule doesn't apply to `MH_OBJECT` files.
*/
typedef struct {
  Mach_load_cmd_head     head;
  char                   segname[16];
  U64                    vmaddr;
  U64                    vmsize;
  U64                    fileoff; // From start of entire Mach-O file.
  U64                    filesize;
  U32                    maxprot;
  U32                    initprot;
  U32                    nsects;
  Mach_load_cmd_seg_flag flags;
} Mach_load_cmd_seg;

typedef enum : U32 {
  // Types; mutually exclusive: only one can be present.
  MST_REGULAR                             = 0x00,
  MST_ZEROFILL                            = 0x01,
  MST_CSTRING_LITERALS                    = 0x02,
  MST_4BYTE_LITERALS                      = 0x03,
  MST_8BYTE_LITERALS                      = 0x04,
  MST_LITERAL_POINTERS                    = 0x05,
  MST_NON_LAZY_SYMBOL_POINTERS            = 0x06,
  MST_LAZY_SYMBOL_POINTERS                = 0x07,
  MST_SYMBOL_STUBS                        = 0x08,
  MST_MOD_INIT_FUNC_POINTERS              = 0x09,
  MST_MOD_TERM_FUNC_POINTERS              = 0x0A,
  MST_COALESCED                           = 0x0B,
  MST_GB_ZEROFILL                         = 0x0C,
  MST_INTERPOSING                         = 0x0D,
  MST_16BYTE_LITERALS                     = 0x0E,
  MST_DTRACE_DOF                          = 0x0F,
  MST_LAZY_DYLIB_SYMBOL_POINTERS          = 0x10,
  MST_THREAD_LOCAL_REGULAR                = 0x11,
  MST_THREAD_LOCAL_ZEROFILL               = 0x12,
  MST_THREAD_LOCAL_VARIABLES              = 0x13,
  MST_THREAD_LOCAL_VARIABLE_POINTERS      = 0x14,
  MST_THREAD_LOCAL_INIT_FUNCTION_POINTERS = 0x15,
  MST_INIT_FUNC_OFFSETS                   = 0x16,

  // Attributes; can be combined.
  MSA_PURE_INSTRUCTIONS   = 0x80000000,
  MSA_NO_TOC              = 0x40000000,
  MSA_STRIP_STATIC_SYMS   = 0x20000000,
  MSA_NO_DEAD_STRIP       = 0x10000000,
  MSA_LIVE_SUPPORT        = 0x08000000,
  MSA_SELF_MODIFYING_CODE = 0x04000000,
  MSA_DEBUG               = 0x02000000,
  MSA_SOME_INSTRUCTIONS   = 0x00000400,
  MSA_EXT_RELOC           = 0x00000200,
  MSA_LOC_RELOC           = 0x00000100,
} Mach_sect_flag;

// `section_64` in Apple `cctools`.
typedef struct {
  char sectname[16];
  char segname[16];
  U64  addr;
  U64  size;
  U32  offset; // From start of entire Mach-O file.
  U32  align;  // Expressed as power of 2; value of 2 = align to 4 bytes.
  U32  reloff;
  U32  nreloc;
  U32  flags;
  U32  reserved1;
  U32  reserved2;
  U32  reserved3;
} Mach_sect;

/*
`linkedit_data_command` in Apple `cctools`.

Command types (see `Mach_load_cmd_type`):
- MLC_ATOM_INFO
- MLC_CODE_SIGNATURE
- MLC_DATA_IN_CODE
- MLC_DYLD_CHAINED_FIXUPS
- MLC_DYLD_EXPORTS_TRIE
- MLC_DYLIB_CODE_SIGN_DRS
- MLC_FUNCTION_STARTS
- MLC_FUNCTION_VARIANT_FIXUPS
- MLC_FUNCTION_VARIANTS
- MLC_LINKER_OPTIMIZATION_HINT
- MLC_SEGMENT_SPLIT_INFO
*/
typedef struct {
  Mach_load_cmd_head head;
  U32                dataoff;
  U32                datasize;
} Mach_load_cmd_linkedit;

// `symtab_command` in Apple `cctools`.
// See `<nlist.h>` and `<stab.h>`.
typedef struct {
  Mach_load_cmd_head head;    // MLC_SYMTAB
  U32                symoff;  /* symbol table offset */
  U32                nsyms;   /* number of symbol table entries */
  U32                stroff;  /* string table offset */
  U32                strsize; /* string table size in bytes */
} Mach_load_cmd_symtab;

// `dysymtab_command` in Apple `cctools`.
typedef struct {
  Mach_load_cmd_head head;         // MLC_DYSYMTAB
  U32                ilocalsym;    // index to local symbols
  U32                nlocalsym;    // number of local symbols
  U32                iextdefsym;   // index to externally defined symbols
  U32                nextdefsym;   // number of externally defined symbols
  U32                iundefsym;    // index to undefined symbols
  U32                nundefsym;    // number of undefined symbols
  U32                tocoff;       // file offset to table of contents
  U32                ntoc;         // number of entries in table of contents
  U32                modtaboff;    // file offset to module table
  U32                nmodtab;      // number of module table entries
  U32                extrefsymoff; // offset to referenced symbol table
  U32                nextrefsyms;  // number of referenced symbol table entries
  U32                indirectsymoff; // file offset to the indirect symbol table
  U32                nindirectsyms;  // number of indirect symbol table entries
  U32                extreloff;      // offset to external relocation entries
  U32                nextrel;        // number of external relocation entries
  U32                locreloff;      // offset to local relocation entries
  U32                nlocrel;        // number of local relocation entries
} Mach_load_cmd_dysymtab;

/*
`dylinker_command` in Apple `cctools`.

Command must be immediately followed by an inline string which is the dylinker
path name. The string's size is included into `.head.cmdsize` and the whole
thing must be padded to 4 bytes with zeros.
*/
typedef struct {
  Mach_load_cmd_head head;        // MLC_LOAD_DYLINKER
  U32                name_offset; // sizeof(Mach_load_cmd_dyld)
  char               name[];      // Follows the command.
} Mach_load_cmd_dyld;

// Fixed-size variant of `Mach_load_cmd_dyld`, easier to encode.
typedef struct {
  Mach_load_cmd_head head;
  U32                name_offset;
  char               name[sizeof("/usr/lib/dyld")];
  char               pad[6];
} Mach_load_cmd_dylinker;

static_assert(divisible_by(sizeof(Mach_load_cmd_dylinker), 8));

// `dylib_command` in Apple `cctools`. Must be followed by name string.
typedef struct {
  Mach_load_cmd_head head;        // MLC_LOAD_DYLIB
  U32                name_offset; // sizeof(Mach_load_cmd_dylib)
  U32                timestamp;   // Seems skippable.
  U32                cur_ver;     // Dylib ver.
  U32                compat_ver;
  char               name[]; // Follows the command.
} Mach_load_cmd_dylib;

// Fixed-size variant of `Mach_load_cmd_dylib`, easier to encode.
typedef struct {
  Mach_load_cmd_head head;
  U32                name_offset;
  U32                timestamp;
  U32                cur_ver;
  U32                compat_ver;
  char               name[sizeof("/usr/lib/libSystem.B.dylib")];
  char               pad[5];
} Mach_cmd_dylib;

static_assert(divisible_by(sizeof(Mach_cmd_dylib), 8));

// `uuid_command` in Apple `cctools`.
typedef struct {
  Mach_load_cmd_head head; // MLC_UUID
  U8                 uuid[16];
} Mach_load_cmd_uuid;

// Values for `Mach_load_cmd_bui_ver.platform`.
typedef enum : U32 {
  MBP_UNKNOWN              = 0,          // PLATFORM_UNKNOWN
  MBP_MACOS                = 1,          // PLATFORM_MACOS
  MBP_IOS                  = 2,          // PLATFORM_IOS
  MBP_TVOS                 = 3,          // PLATFORM_TVOS
  MBP_WATCHOS              = 4,          // PLATFORM_WATCHOS
  MBP_BRIDGEOS             = 5,          // PLATFORM_BRIDGEOS
  MBP_MACCATALYST          = 6,          // PLATFORM_MACCATALYST
  MBP_IOS_SIM              = 7,          // PLATFORM_IOS_SIM
  MBP_TVOS_SIM             = 8,          // PLATFORM_TVOS_SIM
  MBP_WATCHOS_SIM          = 9,          // PLATFORM_WATCHOS_SIM
  MBP_DRIVERKIT            = 10,         // PLATFORM_DRIVERKIT
  MBP_VISIONOS             = 11,         // PLATFORM_VISIONOS
  MBP_VISIONOS_SIM         = 12,         // PLATFORM_VISIONOS_SIM
  MBP_FIRMWARE             = 13,         // PLATFORM_FIRMWARE
  MBP_SEPOS                = 14,         // PLATFORM_SEPOS
  MBP_MACOS_EXCLAVECORE    = 15,         // PLATFORM_MACOS_EXCLAVECORE
  MBP_MACOS_EXCLAVEKIT     = 16,         // PLATFORM_MACOS_EXCLAVEKIT
  MBP_IOS_EXCLAVECORE      = 17,         // PLATFORM_IOS_EXCLAVECORE
  MBP_IOS_EXCLAVEKIT       = 18,         // PLATFORM_IOS_EXCLAVEKIT
  MBP_TVOS_EXCLAVECORE     = 19,         // PLATFORM_TVOS_EXCLAVECORE
  MBP_TVOS_EXCLAVEKIT      = 20,         // PLATFORM_TVOS_EXCLAVEKIT
  MBP_WATCHOS_EXCLAVECORE  = 21,         // PLATFORM_WATCHOS_EXCLAVECORE
  MBP_WATCHOS_EXCLAVEKIT   = 22,         // PLATFORM_WATCHOS_EXCLAVEKIT
  MBP_VISIONOS_EXCLAVECORE = 23,         // PLATFORM_VISIONOS_EXCLAVECORE
  MBP_VISIONOS_EXCLAVEKIT  = 24,         // PLATFORM_VISIONOS_EXCLAVEKIT
  MBP_ANY                  = 0xFFFFFFFF, // PLATFORM_ANY
} Mach_bui_plat;

/*
`build_version_command` in Apple `cctools`.

`.cmdsize` includes `ntools * sizeof(Mach_bui_ver)`.
*/
typedef struct {
  Mach_load_cmd_head head; // MLC_BUILD_VERSION
  Mach_bui_plat      platform;
  U32                minos; // as `mach_ver`
  U32                sdk;   // as `mach_ver`
  U32                ntools;
} Mach_load_cmd_bui_ver;

/*
Values for `Mach_bui_ver.tool`. GPU tools are omitted.
See `#define TOOL_*` in Apple's `loader.h`.
*/
typedef enum : U32 {
  MBT_CLANG = 1,
  MBT_SWIFT = 2,
  MBT_LD    = 3,
  MBT_LLD   = 4,
} Mach_bui_tool;

// `build_tool_version` in Apple `cctools`.
typedef struct {
  Mach_bui_tool tool;
  U32           version;
} Mach_bui_ver;

// `source_version_command` in Apple `cctools`.
typedef struct {
  Mach_load_cmd_head head; // MLC_SOURCE_VERSION
  U64                version;
} Mach_load_cmd_src_ver;

// `entry_point_command` in Apple `cctools`.
typedef struct {
  Mach_load_cmd_head head;      // MLC_MAIN
  U64                entryoff;  // Offset in "text" section.
  U64                stacksize; // Optional.
} Mach_load_cmd_main;

// Values for `Mach_fixup_head.imports_format`.
typedef enum : U32 {
  MFI_IMPORT          = 1, // DYLD_CHAINED_IMPORT
  MFI_IMPORT_ADDEND   = 2, // DYLD_CHAINED_IMPORT_ADDEND
  MFI_IMPORT_ADDEND64 = 3, // DYLD_CHAINED_IMPORT_ADDEND64
} Mach_fixup_import_fmt;

/*
`dyld_chained_fixups_header` in Apple `cctools` `fixup-chains.h`.

Describes the body of `LC_DYLD_CHAINED_FIXUPS`, loaded via the segment named
`__LINKEDIT`; the body is also called "chain_data" in cctools; we may refer
to this as "linkedit data". Each offset in `Mach_fixup_head` is from the start
of linkedit data, which also begins with this struct.

Structure of linkedit data in the general case;
see comment on `Mach_fixup_img` for more notes:

  [0]                  Mach_fixup_head     (28)
  [28 = starts_offset] Mach_fixup_img      (4)     -- seg_count = A
  [32]                 seg_info_offset[]   (4*A)   -- where each `Mach_fixup_seg[]`
  [...]                Mach_fixup_seg[]    (24*B)  -- where actual fixups
  [imports_offset]     Mach_fixup_import[] (4*C)   -- one fixup per symbol
  [symbols_offset]     name[]              (...*C) -- one name per symbol

The actual fixup locations, such as `__got` section, are filled up with
address-sized `Mach_fixup_ptr_64_bind` structs, which point to their
`Mach_fixup_import` entries in this data, and to each other in a chain;
the dylinker rewrites them with addresses.

The count of the following entries should generally match. It's possible
to contrive an executable where the count doesn't match, for example by
duplicating some of the entries, but there's no point:

- `Mach_fixup_import[]`
- `name[]`
- `Mach_fixup_ptr_64_bind[]` in sections such as `__got`

When compiling, we use a fixed amount of segments,
and only the `__got` section needs fixups, so we can
specialize the linkedit structure for convenience:

  [0]                   Mach_fixup_head  (28)
  [28 = starts_offset]  Mach_fixup_img_5 (24) -- where `Mach_fixup_seg` for `__got`
  [52]                  Mach_fixup_seg   (24) -- where `__got`
  [76 = imports_offset] Mach_fixup_import[]
  [symbols_offset]      name[]
*/
typedef struct {
  U32                   fixups_version; // 0
  U32                   starts_offset;  // sizeof(Mach_fixup_head)
  U32                   imports_offset; // start of `Mach_fixup_import[]`
  U32                   symbols_offset; // start of symbol strings
  U32                   imports_count;  // number of imported symbol names
  Mach_fixup_import_fmt imports_format; // DYLD_CHAINED_IMPORT*
  U32                   symbols_format; // 0 = uncompressed, 1 = zlib
} Mach_fixup_head;

/*
`dyld_chained_starts_in_image` in Apple `cctools` `fixup-chains.h`.
Placed immediately after `Mach_fixup_head`.

`.seg_count` must match the count of `MLC_SEGMENT_64` load commands in the
executable file. `.seg_info_offset` after the struct must contain exactly
that many entries, matching the order of the segment load commands.

For segments which need fixups, the corresponding `.seg_info_offset[N]` holds
the offset of a `Mach_fixup_seg` struct, which describes where to find its
fixups; the offset is from the start of `Mach_fixup_img`. For those without
fixups, the offset is 0, and no extra data is emitted.
*/
typedef struct {
  U32 seg_count;
  U32 seg_info_offset[]; // One per segment.
} Mach_fixup_img;

/*
Fixed-size specialized variant of `Mach_fixup_img` for easier encoding.
Used by `../comp/mach_o.c`; segment count must match how many segments
are created by all our load commands.

Segments:

    __PAGEZERO   -- offset 0
    __TEXT       -- offset 0
    __DATA       -- offset 0
    __DATA_CONST -- need fixup
    __LINKEDIT   -- offset 0

The `__DATA_CONST` offset is from the start of this struct.
*/
typedef struct {
  U32 seg_count;          // 5
  U32 seg_info_offset[5]; // `__got` gets fixups, other segments get offset 0.
} Mach_fixup_img_5;

/*
Values for `Mach_fixup_seg.page_start`.

- MFS_NONE  -- used in `Mach_fixup_seg.page_start[]` to denote a page with no fixups.
- MFS_MULTI -- used in `Mach_fixup_seg.page_start[]` to denote a page which has multiple starts.
- MFS_LAST  -- used in `Mach_fixup_seg.chain_starts[]` to denote last start in list for page.
*/
typedef enum : U16 {
  MFS_NONE  = 0xFFFF, // DYLD_CHAINED_PTR_START_NONE
  MFS_MULTI = 0x8000, // DYLD_CHAINED_PTR_START_MULTI
  MFS_LAST  = 0x8000, // DYLD_CHAINED_PTR_START_LAST
} Mach_fixup_start;

// Values for `Mach_fixup_seg.pointer_format`.
typedef enum : U16 {
  MFP_ARM64E              = 1,  // DYLD_CHAINED_PTR_ARM64E
  MFP_64                  = 2,  // DYLD_CHAINED_PTR_64
  MFP_32                  = 3,  // DYLD_CHAINED_PTR_32
  MFP_32_CACHE            = 4,  // DYLD_CHAINED_PTR_32_CACHE
  MFP_32_FIRMWARE         = 5,  // DYLD_CHAINED_PTR_32_FIRMWARE
  MFP_64_OFFSET           = 6,  // DYLD_CHAINED_PTR_64_OFFSET
  MFP_ARM64E_OFFSET       = 7,  // DYLD_CHAINED_PTR_ARM64E_OFFSET
  MFP_ARM64E_KERNEL       = 7,  // DYLD_CHAINED_PTR_ARM64E_KERNEL
  MFP_64_KERNEL_CACHE     = 8,  // DYLD_CHAINED_PTR_64_KERNEL_CACHE
  MFP_ARM64E_USERLAND     = 9,  // DYLD_CHAINED_PTR_ARM64E_USERLAND
  MFP_ARM64E_FIRMWARE     = 10, // DYLD_CHAINED_PTR_ARM64E_FIRMWARE
  MFP_X86_64_KERNEL_CACHE = 11, // DYLD_CHAINED_PTR_X86_64_KERNEL_CACHE
  MFP_ARM64E_USERLAND24   = 12, // DYLD_CHAINED_PTR_ARM64E_USERLAND24
} Mach_fixup_ptr;

/*
`dyld_chained_starts_in_segment` in Apple `cctools` `fixup-chains.h`.
Per-segment chain-starts descriptor for a single page.
*/
typedef struct {
  U32              size;              // sizeof(Mach_fixup_seg)
  U16              page_size;         // 0x1000 or 0x4000 (`MEM_PAGE`)
  Mach_fixup_ptr   pointer_format;    // DYLD_CHAINED_PTR_*
  U64              segment_offset;    // from `vmaddr` of `__TEXT`
  U32              max_valid_pointer; // 0 for 64-bit
  U16              page_count;        // 1
  Mach_fixup_start page_start[1]; // Offset of first fixup at `segment_offset`.

  // U16 chain_starts[1]; // Unused: only for 32-bit formats.
} Mach_fixup_seg;

/*
`dyld_chained_import` in Apple `cctools`.
Entry for `DYLD_CHAINED_IMPORT` / `MFI_IMPORT`.
One per extern symbol referenced from GOT.

`.lib_ordinal` is a 1-based index of the `MLC_LOAD_DYLIB` load command
which loads the symbol's dynamic library; in our case it's usually 1,
meaning the first and only dylib (libSystem).

`.name_offset` is a byte index of the start of the symbol's name
in linkedit data pointed to by `Mach_fixup_head.symbols_offset`.
*/
typedef struct {
  U32 lib_ordinal : 8;
  U32 weak_import : 1; // 0
  U32 name_offset : 23;
} Mach_fixup_import;

static_assert(sizeof(Mach_fixup_import) == 4);

/*
`dyld_chained_ptr_64_bind` in Apple `cctools`.

Bind fixup for `DYLD_CHAINED_PTR_64` / `MFP_64`.
Used for linking a single external symbol.

These pointer-sized structs fill up the `__got` section. Each structs takes
the eventual place of an external address. The dylinker interprets the data
and replaces it with actual extern addresses.

`.ordinal` is the index / offset of this symbol in `Mach_fixup_import[]` in
linkedit data. We also keep the order of entries consistent between linkedit
data and `__got`.

See `Mach_fixup_head` for the description of the linkedit data.
The start of the `Mach_fixup_import[]` list is indicated with
`Mach_fixup_head.imports_offset`.
*/
typedef struct {
  U64 ordinal  : 24; // Index into imports table.
  U64 addend   : 8;  // 0
  U64 reserved : 19; // 0
  U64 next     : 12; // 4-byte distance to next fixup; 0 = end of chain.
  U64 bind     : 1;  // 0 = rebase, 1 = bind.
} Mach_fixup_ptr_64_bind;

static_assert(sizeof(Mach_fixup_ptr_64_bind) == 8);
static_assert(sizeof(Mach_fixup_ptr_64_bind) == sizeof(U64));
