/*
"MCS_*" constants are short for "Mach-O code signature".
See `./mach_codesign.c` for many informative links.
Struct definitions were adapted from LLVM's:

  https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/BinaryFormat/MachO.h
*/
#pragma once
#include "./mem.h"
#include "./num.h"

static constexpr U8  MCS_PAGE_BITS = 12;
static constexpr U16 MCS_PAGE_SIZE = (U16)1 << MCS_PAGE_BITS;

// CSMAGIC_EMBEDDED_SIGNATURE
static constexpr U32 MCS_MAGIC_EMBEDDED_SIGNATURE = 0xFADE0CC0;

// CSMAGIC_EMBEDDED_SIGNATURE
static constexpr U32 MCS_MAGIC_CODE_DIRECTORY = 0xFADE0C02;

// CS_SUPPORTSEXECSEG
static constexpr U32 MCS_SUPPORTS_EXEC_SEG = 0x20400;

// CSSLOT_CODEDIRECTORY
static constexpr U8 MCS_SLOT_CODE_DIRECTORY = 0;

// CS_EXECSEG_MAIN_BINARY
static constexpr U8 MCS_EXEC_SEG_MAIN_BINARY = 1;

// LLVM: `CodeSignAttrs`.
typedef enum : U32 {
  CS_ADHOC         = 0x00000002, // CS_ADHOC
  CS_LINKER_SIGNED = 0x00020000, // CS_LINKER_SIGNED
} Mach_codesign_attrs;

static constexpr U8 MCS_HASH_TYPE = 2;  // CS_HASHTYPE_SHA256
static constexpr U8 MCS_HASH_SIZE = 32; // CS_SHA256_LEN

/*
`CS_SuperBlob` in LLVM's `MachO.h`.

Since the word "blob" is not useful when describing actually structured data,
we replace it with more descriptive terms here and in the implementation code.

Structure of code signature data:

  [0]          Mach_codesign_head     (12)
  [12]         Mach_codesign_index[N] (8 * N)
  [12 + 8 * N] Mach_codesign_dir[N]   (88 * N)
  [...]        id                     (...)
  [...]        hash[]                 (...)

We need only 1 of each header struct:

  [0]   Mach_codesign_head  (12)
  [12]  Mach_codesign_index (20)
  [32]  Mach_codesign_dir   (88)
  [120] id                  (...)
  [...] hash[]              (...)

`.length` is the total size of ALL code signature data, beginning with this
struct and including all other header structs, identity strings, and hashes.
It's equal to the `.datasize` of the `MLC_CODE_SIGNATURE` load command.
*/
typedef struct {
  U32 magic; // MCS_MAGIC_EMBEDDED_SIGNATURE
  U32 length;
  U32 count; // 1
} Mach_codesign_head;

/*
`CS_BlobIndex` in LLVM's `MachO.h`. Points to `Mach_codesign_dir`.
This is considered part of the "super blob" above; offset is from
the start of the entire codesign data segment.
*/
typedef struct {
  U32 type;
  U32 offset;
} Mach_codesign_index;

/*
`CS_CodeDirectory` in LLVM's `MachO.h`.

The field comments are verbatim from LLVM, but they need some explanation.

All offsets here are from the start of this struct, and data
indicated by these offsets does not extend past `.length`.

"CodeDirectory blob" refers to the following, laid out subsequently:

  Mach_codesign_dir -- this struct
  id                     -- ident string
  hash[]                 -- one hash per 4 KiB data chunk

`.length` is the total size of the above. In our case, `.length` is equal to the
remainder of the code signature segment as indicated by the `MLC_CODE_SIGNATURE`
load command.

`.n_code_slots` is how many data chunks are hashed,
and how many hashes are in this "blob".

`.code_limit` is the size of all hashed data. The word "code" is misleading
because this starts at file offset 0 and includes all Mach-O headers and the
entire rest of the file up to the code signature data. We align this size to
`MEM_PAGE`, so it should be equal to `.n_code_slots * MCS_PAGE_SIZE`.

`.exec_seg_base` and so on are slightly misleading because they refer to the
segment `__TEXT`, which begins with Mach-O headers, followed by executable
instructions. The entire segment including headers is considered "executable".
*/
typedef struct {
  U32 magic;           /* magic number (CSMAGIC_CODEDIRECTORY) */
  U32 length;          /* total length of CodeDirectory blob */
  U32 version;         /* compatibility version */
  U32 flags;           /* setup and mode flags */
  U32 hash_offset;     /* offset of hash slot element at index zero */
  U32 ident_offset;    /* offset of identifier string */
  U32 n_special_slots; /* number of special hash slots */
  U32 n_code_slots;    /* number of ordinary (code) hash slots */
  U32 code_limit;      /* limit to main image signature range */
  U8  hash_size;       /* size of each hash in bytes */
  U8  hash_type;       /* type of hash (cdHashType* constants) */
  U8  platform;        /* platform identifier; zero if not platform binary */
  U8  page_size;       /* log2(page size in bytes); 0 => infinite */
  U32 pad2;            /* unused (must be zero) */

  /* Version 0x20100 */
  U32 scatter_offset; /* offset of optional scatter vector */

  /* Version 0x20200 */
  U32 team_offset; /* offset of optional team identifier */

  /* Version 0x20300 */
  U32 pad3;         /* unused (must be zero) */
  U64 code_limit64; /* limit to main image signature range, 64 bits */

  /* Version 0x20400 */
  U64 exec_seg_base;  /* offset of executable segment */
  U64 exec_seg_limit; /* limit of executable segment */
  U64 exec_seg_flags; /* executable segment flags */
} Mach_codesign_dir;

// Our internal interface.
typedef struct {
  Buf        *buf;     // Output buffer.
  const U8   *src;     // Start of Mach-O executable.
  Ind         src_len; // `Mach_codesign_dir.code_limit`
  const char *id;
  Ind         id_len;
  Ind         text_off;  // File offset of `__TEXT` segment (0).
  Ind         text_size; // File size of `__TEXT` segment.
  bool        is_main;
} Mach_codesign_cfg;
