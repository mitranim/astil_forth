#pragma once

#ifdef CLANGD
#include "./intrin.c"
#endif

static const USED Sym INTRIN[] = {
  INTRIN_END,             // end
  INTRIN_FUN,             // fun:
  INTRIN_FUN_COMP,        // fun_comp:
  INTRIN_DEFINE_FUN,      // .define_fun
  INTRIN_DEFINE_FUN_COMP, // .define_fun_comp
  INTRIN_BRACKET_BEG,     // [
  INTRIN_BRACKET_END,     // ]
  INTRIN_RET_EXEC,        // .ret
  INTRIN_RET,             // .ret
  INTRIN_RECUR,           // .recur
  INTRIN_TRY,             // .try
  INTRIN_THROW,           // .throw
  INTRIN_CATCH,           // .catch
  INTRIN_THROWS,          // .throws
  INTRIN_COMP_ONLY,       // .comp_only
  INTRIN_PLAIN_CALL,      // .plain_call
  INTRIN_INTERP_ONLY,     // .interp_only
  INTRIN_INLINE,          // .inline
  INTRIN_REDEFINE,        // .redefine
  INTRIN_HERE_WRITE,      // .here_write
  INTRIN_HERE_EXEC,       // .here_exec
  INTRIN_COMP_INSTR,      // .comp_instr
  INTRIN_COMP_LOAD,       // .comp_load
  INTRIN_COMP_ALLOC_DATA, // .comp_alloc_data
  INTRIN_COMP_PAGE_ADDR,  // .comp_page_addr
  INTRIN_COMP_PAGE_LOAD,  // .comp_page_load
  INTRIN_COMP_CALL,       // .comp_call
  INTRIN_QUIT,            // .quit
  INTRIN_READ_CHAR,       // .read_char
  INTRIN_READ_UNTIL_CHAR, // .read_until_char
  INTRIN_READ_WORD,       // .read_word
  INTRIN_IMPORT,          // .use
  INTRIN_IMPORT_TICK,     // use'
  INTRIN_EXTERN_ADR,      // .comp_extern_adr
  INTRIN_EXTERN_FUN,      // .extern_fun
  INTRIN_FIND_WORD,       // .find_word
  INTRIN_INLINE_WORD,     // .inline_word
  INTRIN_CALL_XT,         // .call_xt
  INTRIN_COMP_LOCAL,      // .comp_local
  INTRIN_DEBUG_ON,        // .debug_on
  INTRIN_DEBUG_OFF,       // .debug_off
  INTRIN_DEBUG_FLUSH,     // .debug_flush
  INTRIN_DEBUG_THROW,     // .debug_throw
  INTRIN_DEBUG_STACK_LEN, // .debug_stack_len
  INTRIN_DEBUG_STACK,     // .debug_stack
  INTRIN_DEBUG_DEPTH,     // .debug_depth
  INTRIN_DEBUG_TOP_INT,   // .debug_top_int
  INTRIN_DEBUG_TOP_PTR,   // .debug_top_ptr
  INTRIN_DEBUG_TOP_STR,   // .debug_top_str
  INTRIN_DEBUG_MEM,       // .debug_mem
  INTRIN_DEBUG_WORD,      // .debug_word
  INTRIN_DEBUG_WORD_TICK, // debug'
  INTRIN_DEBUG_DIS,       // dis'
  INTRIN_DEBUG_SYNC_CODE, // .debug_sync_code
};
