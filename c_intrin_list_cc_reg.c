#pragma once

#ifdef CLANGD
#include "./c_intrin.c"
#endif

static constexpr USED Sym INTRIN[] = {
  // Specific to this CC.
  INTRIN_BRACE,                // {
  INTRIN_COMP_WORD_SIG,        // comp_word_sig
  INTRIN_COMP_NEXT_ARG,        // comp_next_arg
  INTRIN_COMP_NEXT_ARG_REG,    // comp_next_arg_reg
  INTRIN_COMP_CLOBBER_BARRIER, // comp_clobber_barrier
  INTRIN_COMP_LOCAL_GET,       // comp_local_get
  INTRIN_COMP_LOCAL_SET,       // comp_local_set
  INTRIN_DEBUG_CTX,            // debug_ctx
  INTRIN_DEBUG_CTX_IMM,        // #debug_ctx

  // Shared.
  INTRIN_COLON,           // :
  INTRIN_SEMICOLON,       // ;
  INTRIN_BRACKET_BEG,     // [
  INTRIN_BRACKET_END,     // ]
  INTRIN_RET,             // #ret
  INTRIN_RECUR,           // #recur
  INTRIN_IMMEDIATE,       // immediate
  INTRIN_COMP_ONLY,       // comp_only
  INTRIN_NOT_COMP_ONLY,   // not_comp_only
  INTRIN_INLINE,          // inline
  INTRIN_THROWS,          // throws
  INTRIN_REDEFINE,        // redefine
  INTRIN_HERE,            // here
  INTRIN_COMP_INSTR,      // comp_instr
  INTRIN_COMP_LOAD,       // comp_load
  INTRIN_ALLOC_DATA,      // alloc_data
  INTRIN_COMP_PAGE_ADDR,  // comp_page_addr
  INTRIN_COMP_CALL,       // comp_call
  INTRIN_QUIT,            // quit
  INTRIN_CHAR,            // char
  INTRIN_PARSE,           // parse
  INTRIN_PARSE_WORD,      // parse_word
  INTRIN_IMPORT,          // import
  INTRIN_IMPORT_QUOTE,    // import\"
  INTRIN_IMPORT_TICK,     // import'
  INTRIN_EXTERN_PTR,      // extern_ptr:
  INTRIN_EXTERN_PROC,     // extern:
  INTRIN_FIND_WORD,       // find_word
  INTRIN_INLINE_WORD,     // inline_word
  INTRIN_EXECUTE,         // execute
  INTRIN_GET_LOCAL,       // get_local
  INTRIN_ANON_LOCAL,      // anon_local
  INTRIN_DEBUG_FLUSH,     // debug_flush
  INTRIN_DEBUG_THROW,     // debug_throw
  INTRIN_DEBUG_STACK_LEN, // debug_stack_len
  INTRIN_DEBUG_STACK,     // debug_stack
  INTRIN_DEBUG_DEPTH,     // debug_depth
  INTRIN_DEBUG_TOP_INT,   // debug_top_int
  INTRIN_DEBUG_TOP_PTR,   // debug_top_ptr
  INTRIN_DEBUG_TOP_STR,   // debug_top_str
  INTRIN_DEBUG_MEM,       // debug_mem
  INTRIN_DEBUG_WORD,      // debug'
  INTRIN_DEBUG_SYNC_CODE, // debug_sync_code
};
