import' std:lang.f

\ ## Internal interpreter structures
\
\ Using structs, we can access interpreter-internal data
\ which is not exposed via dedicated intrinsics.

\ SYNC[word_str].
struct: Sym_word
  U32 1   field: Sym_word_len
  U8  128 field: Sym_word_buf
end

\ Word definition. This is also the execution token
\ returned by the ticks `'` and `''`.
\
\ Note: this struct definition is incomplete.
\
\ SYNC[sym_fields].
struct: Sym
  Cint     1 field: Sym_type
  Sym_word 1 field: Sym_name
  U8       1 field: Sym_wordlist
  Adr      1 field: Sym_instr \ Executable address.

  \ ... Other fields are omitted until we need them.
end

: comp_word_instr { wordlist } ( C: "name" -- ) ( E: -- instr )
  wordlist next_word comp_push
  compile' Sym_instr
  compile' @
;

\ Similar to the ticks `'` and `''`, but instead of returning an execution
\ token which is `Sym *`, this returns the address of the first executable
\ instruction of the next word.
\
\ Intended for callbacks passed to foreign callers
\ such as `pthread_create` or `qsort`.
\
\ Note: callbacks passed to foreign callers should use `[ true catches ]`
\ to reveal all exceptions as error values (and handle them). Otherwise,
\ if a callback prematurely terminates due to an exception, the error
\ will be silently ignored by the foreign caller.
:: instr'  ( C: "name" -- ) ( E: -- instr ) WORDLIST_EXEC comp_word_instr ;
:: instr'' ( C: "name" -- ) ( E: -- instr ) WORDLIST_COMP comp_word_instr ;
