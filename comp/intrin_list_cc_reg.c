#pragma once

#ifdef CLANGD
#include "./intrin.c"
#endif

static const USED Sym INTRIN[] = {
  // Specific to this CC.
  INTRIN_BRACE,              // {
  INTRIN_COMP_SIGNATURE_GET, // comp_signature_get
  INTRIN_COMP_SIGNATURE_SET, // comp_signature_set
  INTRIN_COMP_ARGS_VALID,    // comp_args_valid
  INTRIN_COMP_ARGS_GET,      // comp_args_get
  INTRIN_COMP_ARGS_SET,      // comp_args_set
  INTRIN_COMP_NEXT_ARG_REG,  // comp_next_arg_reg
  INTRIN_COMP_SCRATCH_REG,   // comp_scratch_reg
  INTRIN_COMP_CLOBBER,       // comp_clobber
  INTRIN_COMP_BARRIER,       // comp_barrier
  INTRIN_COMP_LOCAL_GET,     // comp_local_get
  INTRIN_COMP_LOCAL_SET,     // comp_local_set
  INTRIN_COMP_LOCAL_OFF,     // comp_local_off
  INTRIN_TRY_ALL,            // try_all
  INTRIN_GET_TRY_ALL,        // get_try_all
  INTRIN_ALLOCA,             // alloca
  INTRIN_COMPILE_EXECUTABLE, // compile_executable
  INTRIN_DEBUG_CTX,          // debug_ctx
  INTRIN_DEBUG_CTX_COMP,     // #debug_ctx
  INTRIN_DEBUG_ARG,          // debug_arg

  // Shared.
  INTRIN_COLON,            // :
  INTRIN_COLON_COLON,      // ::
  INTRIN_SEMICOLON,        // ;
  INTRIN_END,              // end
  INTRIN_FUN,              // fun:
  INTRIN_FUN_COMP,         // fun_comp:
  INTRIN_DEFINE_FUN,       // define_fun
  INTRIN_DEFINE_FUN_COMP,  // define_fun_comp
  INTRIN_BRACKET_BEG,      // [
  INTRIN_BRACKET_END,      // ]
  INTRIN_RET,              // #ret
  INTRIN_RECUR,            // #recur
  INTRIN_TRY,              // try
  INTRIN_THROW,            // throw
  INTRIN_CATCH,            // catch
  INTRIN_COMP_ONLY,        // comp_only
  INTRIN_INTERP_ONLY,      // interp_only
  INTRIN_INLINE,           // inline
  INTRIN_REDEFINE,         // redefine
  INTRIN_HERE_WRITE,       // here_write
  INTRIN_HERE_EXEC,        // here_exec
  INTRIN_COMP_INSTR,       // comp_instr
  INTRIN_COMP_LOAD,        // comp_load
  INTRIN_ALLOC_DATA,       // alloc_data
  INTRIN_COMP_PAGE_ADDR,   // comp_page_addr
  INTRIN_COMP_PAGE_LOAD,   // comp_page_load
  INTRIN_COMP_CALL,        // comp_call
  INTRIN_QUIT,             // quit
  INTRIN_CHAR,             // char
  INTRIN_PARSE,            // parse
  INTRIN_PARSE_WORD,       // parse_word
  INTRIN_IMPORT,           // import
  INTRIN_IMPORT_TICK,      // import'
  INTRIN_EXTERN_ADR,       // extern_adr
  INTRIN_EXTERN_FUN,       // extern_fun
  INTRIN_FIND_WORD,        // find_word
  INTRIN_INLINE_WORD,      // inline_word
  INTRIN_EXECUTE,          // execute
  INTRIN_COMP_LOCAL_NAMED, // comp_local_named
  INTRIN_COMP_LOCAl_ANON,  // comp_local_anon
  INTRIN_DEBUG_ON,         // debug_on
  INTRIN_DEBUG_OFF,        // debug_off
  INTRIN_DEBUG_FLUSH,      // debug_flush
  INTRIN_DEBUG_THROW,      // debug_throw
  INTRIN_DEBUG_STACK_LEN,  // debug_stack_len
  INTRIN_DEBUG_STACK,      // debug_stack
  INTRIN_DEBUG_DEPTH,      // debug_depth
  INTRIN_DEBUG_TOP_INT,    // debug_top_int
  INTRIN_DEBUG_TOP_PTR,    // debug_top_ptr
  INTRIN_DEBUG_TOP_STR,    // debug_top_str
  INTRIN_DEBUG_MEM,        // debug_mem
  INTRIN_DEBUG_WORD,       // debug_word
  INTRIN_DEBUG_WORD_TICK,  // debug'
  INTRIN_DEBUG_DIS,        // dis'
  INTRIN_DEBUG_SYNC_CODE,  // debug_sync_code
};
