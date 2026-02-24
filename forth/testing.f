import' ./lang.f

\ Similar to the common Forth testing words:
\
\   https://forth-standard.org/standard/testsuite
\
\ Example tests:
\
\   T{ 10 20    <T> 10 20    } \ OK
\   T{ 10 20    <T> 10 20 30 } \ fail
\   T{ 10 20 30 <T> 10 20    } \ fail

0 var: T_ST_LEN_0 \ Stack length at `T{`.
0 var: T_ST_LEN_1 \ Stack length at `<T>`.
0 var: T_ARGS_LEN \ Args length at `<T>` in compiled mode.

: T_reset
  T_ST_LEN_0 @ stack_set_len
  T_ST_LEN_0 off!
  T_ST_LEN_1 off!
;

: T_stack_eq { len0 len1 len -- equal }
  len 0 +for: ind
    ind len0 + stack_at@ { one }
    ind len1 + stack_at@ { two }
    one two <> if false ret end
  end
  true
;

\ ## Interpretation-mode test routines (top level)

: T{ stack_len T_ST_LEN_0 ! ;

: <T> stack_len T_ST_LEN_1 ! ;

\ 10 20   T{   30 40   <T>   50 60   }T
\         ^ len0 = 2   ^ len1 = 4    ^ len2 = 6
\                      ^ rel0 = 2    ^ rel1 = 2
: }T
  stack_len    { len2 } \ Length at `}T`.
  T_ST_LEN_1 @ { len1 } \ Stack length at `<T>`.
  T_ST_LEN_0 @ { len0 } \ Stack length at `T{`.
  len1 len0 -  { rel0 } \ Relative length before `<T>`.
  len2 len1 -  { rel1 } \ Relative length after `<T>`.

  \ Does the stack length match?
  rel0 rel1 =
  if
    \ Does the content match?
    len0 len1 rel1 T_stack_eq
    if T_reset ret end
  else
    rel0 rel1 elogf" [test] stack length mismatch: (%zd) <T> (%zd)" elf
  end

  elog" [test] stack content mismatch: T{ "
  len1 len0 +for: ind ind stack_at@ elogf" %zd " end
  elog" <T> "
  len2 ind  +for: ind ind stack_at@ elogf" %zd " end
  elog" }T" elf

  T_reset
  throw" test failure"
  unreachable
;

\ ## Compilation-mode test routines (inside words)

: T_end_comp { len } [ true catches ]
  stack_len  { len2 }
  len2 len - { len1 }
  len1 len - { len0 }

  len0 len1 len T_stack_eq
  if len0 stack_set_len ret end

  elog" [test] mismatch: T{ "
  len1 len0 +for: ind ind stack_at@ elogf" %zd " end
  elog" <T> "
  len2 ind  +for: ind ind stack_at@ elogf" %zd " end
  elog" }T" elf

  len0 stack_set_len
  throw" test failure"
  unreachable
;

:: T{
  T{
  c" when calling `T{`" 0 comp_args_valid
;

:: <T>
  <T>
  comp_args_get { len }
  len comp_args_to_stack
  0   comp_args_set
  len T_ARGS_LEN !
;

:: }T
  }T

  T_ARGS_LEN @ { len }
  T_reset
  c" when calling `}T`" len comp_args_valid

  len comp_args_to_stack
  0   comp_args_set
  len comp_push

  \ Disabling auto-catch just for this call allows test utils to work
  \ seamlessly when testing words which specify `[ true catches ]`.
  get_catches { ok }
  ok if false catches end
  compile' T_end_comp
  ok if true catches end
;
