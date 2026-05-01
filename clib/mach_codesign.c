/*
Ad-hoc signing of Mach-O executables.

Links:

- https://github.com/golang/go/blob/master/src/cmd/internal/codesign/codesign.go
- https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/BinaryFormat/MachO.h
- https://github.com/llvm/llvm-project/blob/main/llvm/lib/ObjCopy/MachO/MachOWriter.cpp
- https://oliviagallucci.com/the-anatomy-of-a-mach-o-structure-code-signing-and-pac/
- https://owenbrooks.org/blog/constructing-a-valid-macho/
- https://developer.apple.com/library/archive/technotes/tn2206/_index.html
*/
#pragma once
#include "./hash_sha.c"
#include "./mach_codesign.h"
#include "./mem.c"
#include "./misc.h"
#include "./num.h"

// NOLINTBEGIN(bugprone-easily-swappable-parameters)

/*
The provided offset is where the code signature data begins in the Mach-O file.
The signature will contain hashes of everything in the file before this offset.

SYNC[mach_codesign_total_size].
*/
static U32 mach_codesign_total_size(U32 file_off, U8 id_len) {
  // NOLINTEND(bugprone-easily-swappable-parameters)

  aver(is_aligned_to(file_off, MCS_PAGE_SIZE));

  U32 out = sizeof(Mach_codesign_head) + sizeof(Mach_codesign_index) +
    sizeof(Mach_codesign_dir);

  out = add(out, id_len);
  out = add(out, 1);
  out = add(out, mul((file_off / MCS_PAGE_SIZE), MCS_HASH_SIZE));
  return out;
}

// SYNC[mach_codesign_total_size].
static void mach_codesign(Mach_codesign_cfg cfg) {
  constexpr U8 index_off = sizeof(Mach_codesign_head);
  constexpr U8 dir_off   = index_off + sizeof(Mach_codesign_index);

  // Unlike the prior offsets, these are from the start of `Mach_codesign_dir`.
  constexpr U8 id_off     = sizeof(Mach_codesign_dir);
  const U32    hash_off   = id_off + cfg.id_len + 1;
  const U32    hash_count = divide_round_up(cfg.src_len, MCS_PAGE_SIZE);

  const U32  dir_size   = hash_off + (hash_count * MCS_HASH_SIZE);
  const U32  total_size = dir_off + dir_size;
  const auto buf        = cfg.buf;

  {
    const auto src = (Mach_codesign_head){
      .magic  = MCS_MAGIC_EMBEDDED_SIGNATURE,
      .length = total_size,
      .count  = 1,
    };

    const auto beg = buf->len;
    buf_append_bigend_U32(buf, src.magic);
    buf_append_bigend_U32(buf, src.length);
    buf_append_bigend_U32(buf, src.count);
    aver(buf->len - beg == sizeof(src));
  }

  {
    const auto src = (Mach_codesign_index){
      .type   = MCS_SLOT_CODE_DIRECTORY,
      .offset = dir_off,
    };

    const auto beg = buf->len;
    buf_append_bigend_U32(buf, src.type);
    buf_append_bigend_U32(buf, src.offset);
    aver(buf->len - beg == sizeof(src));
  }

  {
    const auto src = (Mach_codesign_dir){
      .magic          = MCS_MAGIC_CODE_DIRECTORY,
      .length         = dir_size,
      .version        = MCS_SUPPORTS_EXEC_SEG,
      .flags          = CS_ADHOC | CS_LINKER_SIGNED,
      .hash_offset    = hash_off,
      .ident_offset   = id_off,
      .n_code_slots   = hash_count,
      .code_limit     = cfg.src_len,
      .hash_size      = MCS_HASH_SIZE,
      .hash_type      = MCS_HASH_TYPE,
      .page_size      = MCS_PAGE_BITS, // not `MCS_PAGE_SIZE`!
      .exec_seg_base  = cfg.text_off,
      .exec_seg_limit = cfg.text_size,
      .exec_seg_flags = cfg.is_main ? MCS_EXEC_SEG_MAIN_BINARY : 0,
    };

    const auto beg = buf->len;
    buf_append_bigend_U32(buf, src.magic);
    buf_append_bigend_U32(buf, src.length);
    buf_append_bigend_U32(buf, src.version);
    buf_append_bigend_U32(buf, src.flags);
    buf_append_bigend_U32(buf, src.hash_offset);
    buf_append_bigend_U32(buf, src.ident_offset);
    buf_append_bigend_U32(buf, src.n_special_slots);
    buf_append_bigend_U32(buf, src.n_code_slots);
    buf_append_bigend_U32(buf, src.code_limit);
    buf_append_byte(buf, src.hash_size);
    buf_append_byte(buf, src.hash_type);
    buf_append_byte(buf, src.platform);
    buf_append_byte(buf, src.page_size);
    buf_append_bigend_U32(buf, src.pad2);
    buf_append_bigend_U32(buf, src.scatter_offset);
    buf_append_bigend_U32(buf, src.team_offset);
    buf_append_bigend_U32(buf, src.pad3);
    buf_append_bigend_U64(buf, src.code_limit64);
    buf_append_bigend_U64(buf, src.exec_seg_base);
    buf_append_bigend_U64(buf, src.exec_seg_limit);
    buf_append_bigend_U64(buf, src.exec_seg_flags);
    aver(buf->len - beg == sizeof(src));
  }

  buf_append_bytes(buf, (const U8 *)cfg.id, cfg.id_len);
  buf_append_byte(buf, '\0');

  {
    // An internal restriction for peace of mind.
    aver(divisible_by(cfg.src_len, MCS_PAGE_SIZE));
    static_assert(MCS_HASH_SIZE == SHA256_SIZE);

    const U8 *page = cfg.src;
    const U8 *ceil = page + cfg.src_len;

    // Each 4 KiB page gets its own hash.
    for (; page < ceil; page += MCS_PAGE_SIZE) {
      buf_reserve(buf, MCS_HASH_SIZE);
      sha256(buf_top(buf), page, MCS_PAGE_SIZE);
      buf->len += MCS_HASH_SIZE;
    }
  }
}
