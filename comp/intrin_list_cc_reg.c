#pragma once

#ifdef CLANGD
#include "./intrin.c"
#endif

static const USED Sym INTRIN[] = {
  // Specific to this CC.
  INTRIN_BRACE,                      // {
  INTRIN_COMP_SIGNATURE_GET,         // comp_signature_get
  INTRIN_COMP_SIGNATURE_SET,         // comp_signature_set
  INTRIN_COMP_ARGS_VALID,            // comp_args_valid
  INTRIN_COMP_ARGS_MIN,              // comp_args_min
  INTRIN_COMP_ARGS_GET,              // comp_args_get
  INTRIN_COMP_ARGS_SET,              // comp_args_set
  INTRIN_COMP_ARGS_FOLD,             // comp_args_fold
  INTRIN_COMP_ALLOC_NEXT_REG,        // comp_alloc_next_reg
  INTRIN_COMP_REALLOC_REG,           // comp_realloc_reg
  INTRIN_COMP_BARRIER,               // comp_barrier
  INTRIN_COMP_PUSH_FROM_LOCAL,       // comp_push_from_local
  INTRIN_COMP_POP_INTO_LOCAL,        // comp_pop_into_local
  INTRIN_COMP_ASSIGN_LOCAL_FROM_REG, // comp_assign_local_from_reg
  INTRIN_TRY_ALL,                    // try_all
  INTRIN_ALLOCA,                     // alloca
  INTRIN_COMPILE_EXECUTABLE,         // compile_executable
  INTRIN_DEBUG_CTX,                  // debug_ctx
  INTRIN_DEBUG_CTX_COMP,             // #debug_ctx
  INTRIN_DEBUG_ARG,                  // debug_arg

  // Shared.
  INTRIN_END,             // end
  INTRIN_FUN,             // fun:
  INTRIN_FUN_COMP,        // fun_comp:
  INTRIN_DEFINE_FUN,      // define_fun
  INTRIN_DEFINE_FUN_COMP, // define_fun_comp
  INTRIN_BRACKET_BEG,     // [
  INTRIN_BRACKET_END,     // ]
  INTRIN_RET_EXEC,        // ret
  INTRIN_RET,             // #ret
  INTRIN_RECUR,           // #recur
  INTRIN_TRY,             // try
  INTRIN_THROW,           // throw
  INTRIN_CATCH,           // catch
  INTRIN_COMP_ONLY,       // comp_only
  INTRIN_INTERP_ONLY,     // interp_only
  INTRIN_INLINE,          // inline
  INTRIN_REDEFINE,        // redefine
  INTRIN_HERE_WRITE,      // here_write
  INTRIN_HERE_EXEC,       // here_exec
  INTRIN_COMP_INSTR,      // comp_instr
  INTRIN_COMP_LOAD,       // comp_load
  INTRIN_ALLOC_DATA,      // alloc_data
  INTRIN_COMP_PAGE_ADDR,  // comp_page_addr
  INTRIN_COMP_PAGE_LOAD,  // comp_page_load
  INTRIN_COMP_CALL,       // comp_call
  INTRIN_QUIT,            // quit
  INTRIN_READ_CHAR,       // read_char
  INTRIN_READ_UNTIL_CHAR, // read_until_char
  INTRIN_READ_WORD,       // read_word
  INTRIN_IMPORT,          // import
  INTRIN_IMPORT_TICK,     // import'
  INTRIN_EXTERN_ADR,      // extern_adr
  INTRIN_EXTERN_FUN,      // extern_fun
  INTRIN_FIND_WORD,       // find_word
  INTRIN_INLINE_WORD,     // inline_word
  INTRIN_CALL_XT,         // call_xt
  INTRIN_COMP_LOCAL,      // comp_local
  INTRIN_DEBUG_ON,        // debug_on
  INTRIN_DEBUG_OFF,       // debug_off
  INTRIN_DEBUG_FLUSH,     // debug_flush
  INTRIN_DEBUG_THROW,     // debug_throw
  INTRIN_DEBUG_STACK_LEN, // debug_stack_len
  INTRIN_DEBUG_STACK,     // debug_stack
  INTRIN_DEBUG_DEPTH,     // debug_depth
  INTRIN_DEBUG_TOP_INT,   // debug_top_int
  INTRIN_DEBUG_TOP_PTR,   // debug_top_ptr
  INTRIN_DEBUG_TOP_STR,   // debug_top_str
  INTRIN_DEBUG_MEM,       // debug_mem
  INTRIN_DEBUG_WORD,      // debug_word
  INTRIN_DEBUG_WORD_TICK, // debug'
  INTRIN_DEBUG_DIS,       // dis'
  INTRIN_DEBUG_SYNC_CODE, // debug_sync_code
};
