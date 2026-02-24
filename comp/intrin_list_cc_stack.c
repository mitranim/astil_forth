#pragma once

#ifdef CLANGD
#include "./intrin.c"
#endif

static constexpr USED Sym INTRIN[] = {
  INTRIN_COLON,             // :
  INTRIN_COLON_COLON,       // ::
  INTRIN_COLON_NAMED,       // colon_named
  INTRIN_COLON_COLON_NAMED, // colon_colon_named
  INTRIN_SEMICOLON,         // ;
  INTRIN_BRACKET_BEG,       // [
  INTRIN_BRACKET_END,       // ]
  INTRIN_RET,               // #ret
  INTRIN_RECUR,             // #recur
  INTRIN_CATCH,             // catch
  INTRIN_THROW,             // throw
  INTRIN_THROWS,            // throws
  INTRIN_COMP_ONLY,         // comp_only
  INTRIN_INLINE,            // inline
  INTRIN_REDEFINE,          // redefine
  INTRIN_HERE,              // here
  INTRIN_COMP_INSTR,        // comp_instr
  INTRIN_COMP_LOAD,         // comp_load
  INTRIN_ALLOC_DATA,        // alloc_data
  INTRIN_COMP_PAGE_ADDR,    // comp_page_addr
  INTRIN_COMP_PAGE_LOAD,    // comp_page_load
  INTRIN_COMP_CALL,         // comp_call
  INTRIN_QUIT,              // quit
  INTRIN_CHAR,              // char
  INTRIN_PARSE,             // parse
  INTRIN_PARSE_WORD,        // parse_word
  INTRIN_IMPORT,            // import
  INTRIN_IMPORT_QUOTE,      // import\"
  INTRIN_IMPORT_TICK,       // import'
  INTRIN_EXTERN_GOT,        // extern_got
  INTRIN_EXTERN_PROC,       // extern:
  INTRIN_FIND_WORD,         // find_word
  INTRIN_INLINE_WORD,       // inline_word
  INTRIN_EXECUTE,           // execute
  INTRIN_COMP_NAMED_LOCAL,  // comp_named_local
  INTRIN_COMP_ANON_LOCAL,   // comp_anon_local
  INTRIN_DEBUG_ON,          // debug_on
  INTRIN_DEBUG_OFF,         // debug_off
  INTRIN_DEBUG_FLUSH,       // debug_flush
  INTRIN_DEBUG_THROW,       // debug_throw
  INTRIN_DEBUG_STACK_LEN,   // debug_stack_len
  INTRIN_DEBUG_STACK,       // debug_stack
  INTRIN_DEBUG_DEPTH,       // debug_depth
  INTRIN_DEBUG_TOP_INT,     // debug_top_int
  INTRIN_DEBUG_TOP_PTR,     // debug_top_ptr
  INTRIN_DEBUG_TOP_STR,     // debug_top_str
  INTRIN_DEBUG_MEM,         // debug_mem
  INTRIN_DEBUG_WORD,        // debug'
  INTRIN_DEBUG_DIS,         // dis'
  INTRIN_DEBUG_SYNC_CODE,   // debug_sync_code
};
