(
Vaguely similar to regular Forth testing words:

  https://forth-standard.org/standard/testsuite

Example tests:

  T{ 10 20    <T> 10 20    } \ OK
  T{ 10 20    <T> 10 20 30 } \ Fail
  T{ 10 20 30 <T> 10 20    } \ Fail
)

0 var: T_LEN_0 \ Stack length at `T{`.
0 var: T_LEN_1 \ Stack length at `<T>`.

: T_reset
  \ Reset to initial stack length.
  T_LEN_0 @ stack_trunc
  T_LEN_0 off!
  T_LEN_1 off!
;

: T{ stack_len T_LEN_0 ! ;
: <T> stack_len T_LEN_1 ! ;

: }T
  stack_len   to: len2 \ Length at `}T`.
  T_LEN_1 @   to: len1 \ Stack length at `<T>`.
  T_LEN_0 @   to: len0 \ Stack length at `T{`.
  len1 len0 - to: rel0 \ Relative length before `<T>`.
  len2 len1 - to: rel1 \ Relative length after `<T>`.

  \ Does the stack length match?
  rel0 rel1 =
  #if
    \ Does the content match?
    len0 to: ind0
    len1 to: ind1
    0    to: ind
    true to: equal

    #loop
      ind rel1 < #while
      ind len0 + pick0
      ind len1 + pick0
      <> #if false to: equal #end
      ind inc to: ind
    #end

    equal #if T_reset #ret #end
  #else
    rel0 rel1 [ 2 ] elogf" stack length mismatch: (%zd) <T> (%zd)" lf
  #end

  elog" stack content mismatch: T{ "

  \ Print cells before `<T>`.
  len0
  #loop
    dup len1 < #while
    dup pick0 [ 1 ] elogf" %zd "
    inc
  #end

  elog" <T> "

  \ Print cells after `<T>`.
  #loop
    dup len2 < #while
    dup pick0 [ 1 ] elogf" %zd "
    inc
  #end drop

  elog" }T" lf

  T_reset
  throw" test failure"
  unreachable
;
