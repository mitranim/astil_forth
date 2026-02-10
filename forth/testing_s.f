import' ./lang_s.f

\ Similar to the common Forth testing words:
\
\   https://forth-standard.org/standard/testsuite
\
\ Example tests:
\
\   T{ 10 20    <T> 10 20    } \ OK
\   T{ 10 20    <T> 10 20 30 } \ fail
\   T{ 10 20 30 <T> 10 20    } \ fail

0 var: T_LEN_0 \ Stack length at `T{`.
0 var: T_LEN_1 \ Stack length at `<T>`.

: T_reset
  T_LEN_0 @ stack_set_len
  T_LEN_0 off!
  T_LEN_1 off!
;

: T_stack_eq ( len0 len1 len -- equal )
  to: len
  to: len1
  to: len0

  len 0 +for: ind
    ind len0 + pick0 { one }
    ind len1 + pick0 { two }
    one two <> if false ret end
  end
  true
;

: T{  stack_len T_LEN_0 ! ;
: <T> stack_len T_LEN_1 ! ;

: }T
  stack_len   to: len2 \ Length at `}T`.
  T_LEN_1 @   to: len1 \ Stack length at `<T>`.
  T_LEN_0 @   to: len0 \ Stack length at `T{`.
  len1 len0 - to: rel0 \ Relative length before `<T>`.
  len2 len1 - to: rel1 \ Relative length after `<T>`.

  \ Does the stack length match?
  rel0 rel1 =
  if
    \ Does the content match?
    len0 len1 rel1 T_stack_eq
    if T_reset ret end
  else
    rel0 rel1 [ 2 ] elogf" stack length mismatch: (%zd) <T> (%zd)" elf
  end

  elog" stack content mismatch: T{ "
  len1 len0 +for: ind ind pick0 [ 1 ] elogf" %zd " end
  elog" <T> "
  len2 ind  +for: ind ind pick0 [ 1 ] elogf" %zd " end
  elog" }T" elf

  T_reset
  throw" test failure"
  unreachable
;
