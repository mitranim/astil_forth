/*
Links:

- https://github.com/apple-oss-distributions/xnu/tree/main/EXTERNAL_HEADERS/mach-o/loader.h
- https://github.com/apple-oss-distributions/cctools/blob/main/include/mach-o/loader.h
- https://github.com/apple-oss-distributions/cctools/blob/main/include/mach/machine-cctools.h
- https://web.archive.org/web/20140911185310/https://developer.apple.com/library/mac/documentation/DeveloperTools/Conceptual/MachORuntime/Reference/reference.html
- https://en.wikipedia.org/wiki/Mach-O
*/

#pragma once
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

// `mach_header_64` in Apple cctools.
typedef struct {
  U32            magic;
  Mach_cpu_type  cpu_type;
  U32            cpu_subtype;
  Mach_file_type file_type;
  U32            cmd_count;
  U32            cmd_size;
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
  MSF_HIGHVM              = 0x01,
  MSF_FVMLIB              = 0x02,
  MSF_NORELOC             = 0x04,
  MSF_PROTECTED_VERSION_1 = 0x08,
  MSF_READ_ONLY           = 0x10,
} Mach_load_cmd_seg_flag;

// `segment_command_64` in Apple cctools.
typedef struct {
  Mach_load_cmd_head     head;
  char                   segname[16];
  U64                    vmaddr;
  U64                    vmsize;
  U64                    fileoff;
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

// `section_64` in Apple cctools.
typedef struct {
  char           sectname[16];
  char           segname[16];
  U64            addr;
  U64            size;
  U32            offset;
  U32            align;
  U32            reloff;
  U32            nreloc;
  Mach_sect_flag flags;
  U32            reserved1;
  U32            reserved2;
  U32            reserved3;
} Mach_sect;

/*
`linkedit_data_command` in Apple cctools.

Command types:
- LC_ATOM_INFO
- LC_CODE_SIGNATURE
- LC_DATA_IN_CODE
- LC_DYLD_CHAINED_FIXUPS
- LC_DYLD_EXPORTS_TRIE
- LC_DYLIB_CODE_SIGN_DRS
- LC_FUNCTION_STARTS
- LC_FUNCTION_VARIANT_FIXUPS
- LC_FUNCTION_VARIANTS
- LC_LINKER_OPTIMIZATION_HINT
- LC_SEGMENT_SPLIT_INFO
*/
typedef struct {
  Mach_load_cmd_head head;
  U32                dataoff;
  U32                datasize;
} Mach_load_cmd_linkedit;

// `symtab_command` in Apple cctools. See `<nlist.h>` and `<stab.h>`.
typedef struct {
  Mach_load_cmd_head head;    // MLC_SYMTAB
  U32                symoff;  /* symbol table offset */
  U32                nsyms;   /* number of symbol table entries */
  U32                stroff;  /* string table offset */
  U32                strsize; /* string table size in bytes */
} Mach_load_cmd_symtab;

// `dysymtab_command` in Apple cctools.
typedef struct {
  Mach_load_cmd_head head; // MLC_DYSYMTAB

  /*
   * The symbols indicated by symoff and nsyms of the LC_SYMTAB load command
   * are grouped into the following three groups:
   *    local symbols (further grouped by the module they are from)
   *    defined external symbols (further grouped by the module they are from)
   *    undefined symbols
   *
   * The local symbols are used only for debugging.  The dynamic binding
   * process may have to use them to indicate to the debugger the local
   * symbols for a module that is being bound.
   *
   * The last two groups are used by the dynamic binding process to do the
   * binding (indirectly through the module table and the reference symbol
   * table when this is a dynamically linked shared library file).
   */
  U32 ilocalsym; /* index to local symbols */
  U32 nlocalsym; /* number of local symbols */

  U32 iextdefsym; /* index to externally defined symbols */
  U32 nextdefsym; /* number of externally defined symbols */

  U32 iundefsym; /* index to undefined symbols */
  U32 nundefsym; /* number of undefined symbols */

  /*
   * For the for the dynamic binding process to find which module a symbol
   * is defined in the table of contents is used (analogous to the ranlib
   * structure in an archive) which maps defined external symbols to modules
   * they are defined in.  This exists only in a dynamically linked shared
   * library file.  For executable and object modules the defined external
   * symbols are sorted by name and is use as the table of contents.
   */
  U32 tocoff; /* file offset to table of contents */
  U32 ntoc;   /* number of entries in table of contents */

  /*
   * To support dynamic binding of "modules" (whole object files) the symbol
   * table must reflect the modules that the file was created from.  This is
   * done by having a module table that has indexes and counts into the merged
   * tables for each module.  The module structure that these two entries
   * refer to is described below.  This exists only in a dynamically linked
   * shared library file.  For executable and object modules the file only
   * contains one module so everything in the file belongs to the module.
   */
  U32 modtaboff; /* file offset to module table */
  U32 nmodtab;   /* number of module table entries */

  /*
   * To support dynamic module binding the module structure for each module
   * indicates the external references (defined and undefined) each module
   * makes.  For each module there is an offset and a count into the
   * reference symbol table for the symbols that the module references.
   * This exists only in a dynamically linked shared library file.  For
   * executable and object modules the defined external symbols and the
   * undefined external symbols indicates the external references.
   */
  U32 extrefsymoff; /* offset to referenced symbol table */
  U32 nextrefsyms;  /* number of referenced symbol table entries */

  /*
   * The sections that contain "symbol pointers" and "routine stubs" have
   * indexes and (implied counts based on the size of the section and fixed
   * size of the entry) into the "indirect symbol" table for each pointer
   * and stub.  For every section of these two types the index into the
   * indirect symbol table is stored in the section header in the field
   * reserved1.  An indirect symbol table entry is simply a 32bit index into
   * the symbol table to the symbol that the pointer or stub is referring to.
   * The indirect symbol table is ordered to match the entries in the section.
   */
  U32 indirectsymoff; /* file offset to the indirect symbol table */
  U32 nindirectsyms;  /* number of indirect symbol table entries */

  /*
   * To support relocating an individual module in a library file quickly the
   * external relocation entries for each module in the library need to be
   * accessed efficiently.  Since the relocation entries can't be accessed
   * through the section headers for a library file they are separated into
   * groups of local and external entries further grouped by module.  In this
   * case the presents of this load command who's extreloff, nextrel,
   * locreloff and nlocrel fields are non-zero indicates that the relocation
   * entries of non-merged sections are not referenced through the section
   * structures (and the reloff and nreloc fields in the section headers are
   * set to zero).
   *
   * Since the relocation entries are not accessed through the section headers
   * this requires the r_address field to be something other than a section
   * offset to identify the item to be relocated.  In this case r_address is
   * set to the offset from the vmaddr of the first LC_SEGMENT command.
   * For MH_SPLIT_SEGS images r_address is set to the the offset from the
   * vmaddr of the first read-write LC_SEGMENT command.
   *
   * The relocation entries are grouped by module and the module table
   * entries have indexes and counts into them for the group of external
   * relocation entries for that the module.
   *
   * For sections that are merged across modules there must not be any
   * remaining external relocation entries for them (for merged sections
   * remaining relocation entries must be local).
   */
  U32 extreloff; /* offset to external relocation entries */
  U32 nextrel;   /* number of external relocation entries */

  /*
   * All the local relocation entries are grouped together (they are not
   * grouped by their module since they are only used if the object is moved
   * from it staticly link edited address).
   */
  U32 locreloff; /* offset to local relocation entries */
  U32 nlocrel;   /* number of local relocation entries */
} Mach_load_cmd_dysymtab;

/*
`dylinker_command` in Apple cctools.

Command must be immediately followed by an inline string which is the dylinker
path name. The string's size is included into `.head.cmdsize` and the whole
thing must be padded to 4 bytes with zeros.
*/
typedef struct {
  Mach_load_cmd_head head;        // MLC_LOAD_DYLINKER
  U32                name_offset; // sizeof(Mach_load_cmd_dyld)
  char               name[];      // Follows the command.
} Mach_load_cmd_dyld;

// `dylib_command` in Apple cctools. Must be followed by name string.
typedef struct {
  Mach_load_cmd_head head;        // MLC_LOAD_DYLIB
  U32                name_offset; // sizeof(Mach_load_cmd_dylib)
  U32                timestamp;   // Seems skippable.
  U32                cur_ver;     // Dylib ver.
  U32                compat_ver;
  char               name[]; // Follows the command.
} Mach_load_cmd_dylib;

// `uuid_command` in Apple cctools.
typedef struct {
  Mach_load_cmd_head head; // MLC_UUID
  U8                 uuid[16];
} Mach_load_cmd_uuid;

/*
`build_version_command` in Apple cctools.

`.cmdsize` includes `ntools * sizeof(Mach_bui_ver)`.
*/
typedef struct {
  Mach_load_cmd_head head; // MLC_BUILD_VERSION
  U32                platform;
  U32                minos; // as `mach_ver`
  U32                sdk;   // as `mach_ver`
  U32                ntools;
} Mach_load_cmd_bui_ver;

// Known values.
typedef enum : U32 {
  MBT_CLANG = 0x01,
  MBT_SWIFT = 0x02,
  MBT_LD    = 0x03,
  MBT_LLD   = 0x04,
} Mach_bui_tool;

// `build_tool_version` in Apple cctools.
typedef struct {
  Mach_bui_tool tool;
  U32           version;
} Mach_bui_ver;

// `source_version_command` in Apple cctools.
typedef struct {
  Mach_load_cmd_head head; // MLC_SOURCE_VERSION
  U64                version;
} Mach_load_cmd_src_ver;

// `entry_point_command` in Apple cctools.
typedef struct {
  Mach_load_cmd_head head;      // MLC_MAIN
  U64                entryoff;  // Offset in "text" section.
  U64                stacksize; // Optional.
} Mach_load_cmd_main;
