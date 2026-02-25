import' ./lang.f
import' ./testing.f

\ This test suite for reg-CC is written differently from the test suite for
\ stack-CC, because of the various checks and restrictions in the compiler.
\ Control flow structures are not allowed to "return" or "push" any values.
\ The amount of arguments before and after `<T>` is checked at compile time
\ which means we can't write tests which repeatedly "push" values in a loop
\ then check the resulting list of values. We just check the final results.

T{       <T>       }T
T{ 10    <T> 10    }T
T{ 10 20 <T> 10 20 }T

: test_test
  T{       <T>       }T
  T{ 10    <T> 10    }T
  T{ 10 20 <T> 10 20 }T
;
test_test

: test_conditionals
  \ false if 10      end \ Must fail to compile.
  \ false if else 10 end \ Must fail to compile.

  T{ -1 { val } false if 10 { val } end val <T> -1 }T
  T{ -1 { val } true  if 10 { val } end val <T> 10 }T

  T{ false if 10 { val } else 20 { val } end val <T> 20 }T
  T{ true  if 10 { val } else 20 { val } end val <T> 10 }T

  T{ -1 { val } false ifn 10 { val } end val <T> 10 }T
  T{ -1 { val } true  ifn 10 { val } end val <T> -1 }T

  T{ false ifn 10 { val } else 20 { val } end val <T> 10 }T
  T{ true  ifn 10 { val } else 20 { val } end val <T> 20 }T

  T{
    -1 { val } false if 10 { val } else false elif 20 { val } end val
  <T>
    -1
  }T

  T{
    -1 { val }
    false if 10 { val } else false elif 20 { val } else 30 { val } end
    val
  <T>
    30
  }T

  T{
    -1 { val } false if 10 { val } else true elif 20 { val } end val
  <T>
    20
  }T

  T{
    -1 { val }
    false if 10 { val } else true elif 20 { val } else 30 { val } end
    val
  <T>
    20
  }T

  T{
    -1 { val } true if 10 { val } else false elif 20 { val } end val
  <T>
    10
  }T

  T{
    -1 { val }
    true if 10 { val } else false elif 20 { val } else 30 { val } end
    val
  <T>
    10
  }T

  T{
    -1 { val } false if 10 { val } else false elifn 20 { val } end val
  <T>
    20
  }T

  T{
    -1 { val }
    false if 10 { val } else false elifn 20 { val } else 30 { val } end
    val
  <T>
    20
  }T

  T{
    -1 { val } false if 10 { val } else true elifn 20 { val } end val
  <T>
    -1
  }T

  T{
    -1 { val }
    false if 10 { val } else true elifn 20 { val } else 30 { val } end
    val
  <T>
    30
  }T

  T{
    -1 { val } true if 10 { val } else false elifn 20 { val } end val
  <T>
    10
  }T

  T{
    -1 { val }
    true if 10 { val } else false elifn 20 { val } else 30 { val } end
    val
  <T>
    10
  }T
;
test_conditionals

: test_recursion { val -- val }
  val 13 <= if val ret end
  val dec recur
;
T{ 123 test_recursion <T> 13 }T

1 0 extern: sleep

: test_sleep_loop loop log" sleeping" lf 1 sleep end ;
\ test_sleep_loop

: test_loop_leave
  T{ -1 { val } loop            leave abort             end val <T> -1 }T
  T{ -1 { val } loop 10 { val } leave abort             end val <T> 10 }T
  T{ -1 { val } loop 10 { val } leave abort leave abort end val <T> 10 }T
;
test_loop_leave

: test_loop_nested
  T{
    -1 { one }
    -2 { two }
    -3 { three }
    loop
      10 { one }
      loop
        20 { two }
        leave
      end
      30 { three }
      leave
    end
    one two three
  <T>
    10 20 30
  }T
;
test_loop_nested

: test_loop_while
  T{
    -1 { val }
    loop false while 10 { val } end
    val
  <T>
    -1
  }T

  T{
    -1 { val }
    loop 10 { val } false while 20 { val } end
    val
  <T>
    10
  }T

  T{
    -1 { val }
    loop 10 { val } false while 20 { val } true while end
    val
  <T>
    10
  }T

  T{
    -1 { val }
    loop 10 { val } true while 20 { val } false while end
    val
  <T>
    20
  }T

  T{
    12 { val }
    loop
      val 5 > while
      val dec { val }
    end
    val
  <T>
    5
  }T

  T{
    12 { val }
    loop
      val 5 > while
      val 7 > while
      val dec { val }
    end
    val
  <T>
    7
  }T

  T{
    12 { val }
    loop
      val dec { val }
      val while
      val while
      val while
    end
    val
  <T>
    0
  }T
;
test_loop_while

: test_loop_while_leave
  T{ -1 { val } loop            false while 10 { val } leave end val <T> -1 }T
  T{ -1 { val } loop            true  while 10 { val } leave end val <T> 10 }T
  T{ -1 { val } loop 10 { val } false while 20 { val } leave end val <T> 10 }T
  T{ -1 { val } loop 10 { val } true  while 20 { val } leave end val <T> 20 }T
  T{ -1 { val } loop 10 { val } true  while leave 20 { val } end val <T> 10 }T
  T{ -1 { val } loop leave 10 { val } true  while 20 { val } end val <T> -1 }T
;
test_loop_while_leave

: test_loop_until
  T{ -1 { val } loop                              false until val <T> -1 }T
  T{ -1 { val } loop 10 { val }                   false until val <T> 10 }T
  T{ -1 { val } loop 10 { val } leave             true  until val <T> 10 }T
  T{ -1 { val } loop 10 { val } leave leave       true  until val <T> 10 }T
  T{ -1 { val } loop 10 { val } leave leave leave true  until val <T> 10 }T

  T{ 12 { val } loop val dec { val } val     until val <T> 0 }T
  T{ 12 { val } loop val dec { val } val 6 > until val <T> 6 }T

  T{
    12 { val }
    loop val dec { val } val 8 > while val 6 > until
    val
  <T>
    8
  }T
;
test_loop_until

: test_loop_for
  T{ -12 for end <T> }T
  T{ -1  for end <T> }T
  T{ 0   for end <T> }T

  T{ 0 { val } 12 for val inc { val }       end val <T> 12 }T
  T{ 0 { val } 12 for val inc { val } leave end val <T> 1  }T
  T{ 0 { val } 12 for leave val inc { val } end val <T> 0  }T

  T{
    0 { val }
    12 for
      val 5 > if leave end
      val inc { val }
    end
    val
  <T>
    6
  }T

  T{
    0 { val }
    12 for
      val 6 < while
      val inc { val }
    end
    val
  <T>
    6
  }T

  T{
    0 { val }
    12 for
      val 5 < while
      val 8 < while
      val inc { val }
    end
    val
  <T>
    5
  }T

  T{
    0 { val }
    12 for
      val 8 < while
      val 5 < while
      val inc { val }
    end
    val
  <T>
    5
  }T
;
test_loop_for

: test_loop_for_countdown
  T{ -12 -for: ind end <T> }T
  T{ -1  -for: ind end <T> }T
  T{ 0   -for: ind end <T> }T

  T{ -1 { ind } 12 -for: ind                       end ind <T> 0  }T
  T{ -1 { ind } 12 -for: ind            leave      end ind <T> 11 }T
  T{ -1 { ind } 12 -for: ind ind 5 <= if leave end end ind <T> 5  }T
  T{ -1 { ind } 12 -for: ind ind 5 >    while      end ind <T> 5  }T

  T{
    -1 { ind }
    12 -for: ind
      ind 7 > while
      ind 5 > while
    end
    ind
  <T>
    7
  }T

  T{
    -1 { ind }
    12 -for: ind
      ind 5 > while
      ind 7 > while
    end
    ind
  <T>
    7
  }T
;
test_loop_for_countdown

: test_loop_for_countup
  T{ -1 -1 +for: ind end ind <T> -1 }T
  T{ -1 -2 +for: ind end ind <T> -1 }T
  T{  0 -2 +for: ind end ind <T>  0 }T
  T{  0  0 +for: ind end ind <T>  0 }T
  T{  1  0 +for: ind end ind <T>  1 }T
  T{  2  0 +for: ind end ind <T>  2 }T
  T{ 12  0 +for: ind end ind <T> 12 }T

  T{ -1 -2 +for: ind       leave end ind <T> -2 }T
  T{ -1 -1 +for: ind       leave end ind <T> -1 }T
  T{  0  0 +for: ind       leave end ind <T>  0 }T
  T{  1  0 +for: ind       leave end ind <T>  0 }T
  T{  1  1 +for: ind       leave end ind <T>  1 }T
  T{  2  1 +for: ind       leave end ind <T>  1 }T
  T{  2  1 +for: ind false while end ind <T>  1 }T
  T{  2  1 +for: ind true  while end ind <T>  2 }T

  T{
    -123 { ind }
    12 0 +for: ind
      ind 5 > if leave end
    end
    ind
  <T>
    6
  }T

  T{
    -123 { ind }
    12 0 +for: ind
      ind 6 < while
    end
    ind
  <T>
    6
  }T

  T{
    -123 { ind }
    12 0 +for: ind
      ind 8 < while
      ind 6 < while
    end
    ind
  <T>
    6
  }T

  T{
    -123 { ind }
    12 0 +for: ind
      ind 6 < while
      ind 8 < while
    end
    ind
  <T>
    6
  }T
;
test_loop_for_countup

: test_loop_countup
  T{ 8 0 1 +loop: ind end ind <T> 8  }T
  T{ 8 0 2 +loop: ind end ind <T> 8  }T
  T{ 8 0 3 +loop: ind end ind <T> 9  }T
  T{ 8 0 4 +loop: ind end ind <T> 8  }T
  T{ 8 0 5 +loop: ind end ind <T> 10 }T
;
test_loop_countup

: test_loop_countup_nested
  T{
    4 0 1 +loop: one
      7 one 1 +loop: two end
    end
    one two
  <T>
    4 7
  }T
;
test_loop_countup_nested

: test_loop_countdown
  T{ 8 0 1 -loop: ind end ind <T> 0 }T
  T{ 8 0 2 -loop: ind end ind <T> 0 }T
  T{ 8 0 3 -loop: ind end ind <T> 2 }T
  T{ 8 0 4 -loop: ind end ind <T> 0 }T
  T{ 8 0 5 -loop: ind end ind <T> 3 }T
;
test_loop_countdown

: test_cont_loop
  -1 { ind }
  loop
    ind inc { ind }
    ind 7 < if cont end
    leave
  end
  T{ ind <T> 7 }T
;
test_cont_loop

: test_cont_for
  0 { ind }
  7 for
    ind inc { ind }
    cont
    leave
  end
  T{ ind <T> 7 }T
;
test_cont_for

\ Used internally by testing utils. The test is kinda cyclic.
123 var: VAR
T{ VAR @     <T> 123 }T
T{ 234 VAR ! <T>     }T
T{ VAR @     <T> 234 }T
T{ 345 VAR ! <T>     }T
T{ VAR @     <T> 345 }T

T{ 0    negate <T> 0    }T
T{ 123  negate <T> -123 }T
T{ -123 negate <T> 123  }T

\ `=` is used by the testing tools, so this test is almost meaningless.
T{  0    0   = <T> 1 }T
T{  0    123 = <T> 0 }T
T{  123  0   = <T> 0 }T
T{ -123  0   = <T> 0 }T
T{ -123  123 = <T> 0 }T
T{  123 -123 = <T> 0 }T
T{ -123 -123 = <T> 1 }T
T{  123  123 = <T> 1 }T

T{  0    0   <> <T> 0 }T
T{  0    123 <> <T> 1 }T
T{  123  0   <> <T> 1 }T
T{ -123  0   <> <T> 1 }T
T{ -123  123 <> <T> 1 }T
T{  123 -123 <> <T> 1 }T
T{ -123 -123 <> <T> 0 }T
T{  123  123 <> <T> 0 }T

T{  0    0   > <T> 0 }T
T{  0    123 > <T> 0 }T
T{  123  0   > <T> 1 }T
T{ -123  0   > <T> 0 }T
T{ -123  123 > <T> 0 }T
T{  123 -123 > <T> 1 }T
T{ -123 -123 > <T> 0 }T
T{  123  123 > <T> 0 }T

T{  0    0   < <T> 0 }T
T{  0    123 < <T> 1 }T
T{  123  0   < <T> 0 }T
T{ -123  0   < <T> 1 }T
T{ -123  123 < <T> 1 }T
T{  123 -123 < <T> 0 }T
T{ -123 -123 < <T> 0 }T
T{  123  123 < <T> 0 }T

T{  0    0    <= <T> 1 }T
T{  0    123  <= <T> 1 }T
T{  123  0    <= <T> 0 }T
T{ -123  0    <= <T> 1 }T
T{ -123  123  <= <T> 1 }T
T{  123 -123  <= <T> 0 }T
T{ -123 -123  <= <T> 1 }T
T{  123  123  <= <T> 1 }T

T{  0    0    >= <T> 1 }T
T{  0    123  >= <T> 0 }T
T{  123  0    >= <T> 1 }T
T{ -123  0    >= <T> 0 }T
T{ -123  123  >= <T> 0 }T
T{  123 -123  >= <T> 1 }T
T{ -123 -123  >= <T> 1 }T
T{  123  123  >= <T> 1 }T

T{ 234 123 - <T> 111   }T
T{ 234 123 + <T> 357   }T
T{ 234 123 * <T> 28782 }T
T{ 234 123 / <T> 1     }T

T{ 12 1 mod <T> 0 }T
T{ 12 2 mod <T> 0 }T
T{ 12 3 mod <T> 0 }T
T{ 12 4 mod <T> 0 }T
T{ 12 5 mod <T> 2 }T
T{ 12 6 mod <T> 0 }T
T{ 12 7 mod <T> 5 }T
T{ 12 8 mod <T> 4 }T
T{ 12 9 mod <T> 3 }T

T{ 12 1 /mod <T> 0 12 }T
T{ 12 2 /mod <T> 0 6  }T
T{ 12 3 /mod <T> 0 4  }T
T{ 12 4 /mod <T> 0 3  }T
T{ 12 5 /mod <T> 2 2  }T
T{ 12 6 /mod <T> 0 2  }T
T{ 12 7 /mod <T> 5 1  }T
T{ 12 8 /mod <T> 4 1  }T
T{ 12 9 /mod <T> 3 1  }T

T{    0 abs <T>   0 }T
T{ -123 abs <T> 123 }T
T{  123 abs <T> 123 }T

T{  123  234 min <T>  123 }T
T{  234  123 min <T>  123 }T
T{  0    123 min <T>  0   }T
T{  123  0   min <T>  0   }T
T{  0   -123 min <T> -123 }T
T{ -123  0   min <T> -123 }T
T{ -123  234 min <T> -123 }T
T{  234 -123 min <T> -123 }T

T{  123  234 max <T>  234 }T
T{  234  123 max <T>  234 }T
T{  0    123 max <T>  123 }T
T{  123  0   max <T>  123 }T
T{  0   -123 max <T>  0   }T
T{ -123  0   max <T>  0   }T
T{ -123  234 max <T>  234 }T
T{  234 -123 max <T>  234 }T

T{ nil            nil     cstr= <T> true  }T
T{ nil            c" one" cstr= <T> false }T
T{ c" one"        nil     cstr= <T> false }T
T{ c" one" strdup c" two" cstr= <T> false }T
T{ c" two" strdup c" one" cstr= <T> false }T
T{ c" one" strdup c" one" cstr= <T> true  }T

T{ nil            nil     cstr< <T> false }T
T{ nil            c" one" cstr< <T> true  }T
T{ c" one"        nil     cstr< <T> false }T
T{ c" one" strdup c" two" cstr< <T> true  }T
T{ c" two" strdup c" one" cstr< <T> false }T
T{ c" one" strdup c" one" cstr< <T> false }T

: int32_negative { -- val } [
  0b0_00_100101_00_0000000000000000_00000 comp_instr \ mov w0, -1
  1 comp_args_set
] ;

T{ int32_negative     <T> 0xffff_ffff           }T
T{ int32_negative int <T> 0xffff_ffff_ffff_ffff }T
T{ int32_negative int <T> -1                    }T

: test_varargs
  10 20 30 va{ c" numbers (should be 10 20 30): %zd %zd %zd" printf }va lf
;
\ test_varargs

: test_fmt
  10 20 30 logf" numbers: %zd %zd %zd" lf
;
\ test_fmt

: test_efmt
  10 20 30 elogf" numbers: %zd %zd %zd" lf
;
\ test_efmt

4096 buf: STR_BUF

\ TODO: port this when we need this.
\ : test_str_fmt
\   STR_BUF 10 20 30 sf" numbers: %zd %zd %zd"
\   STR_BUF drop puts
\ ;
\ test_str_fmt

\ TODO: port this when we need this.
\ : test_sthrowf
\   STR_BUF 10 20 30 [ 3 ] sthrowf" codes: %zd %zd %zd"
\ ;
\ test_sthrowf

: test_throwf
  10 20 30 throwf" codes: %zd %zd %zd"
;
\ test_throwf

: test_locals
  T{          { }                                     <T>          }T
  T{          { -- }                                  <T>          }T
  T{ 10       { }                                     <T> 10       }T
  T{ 10 20    { }                                     <T> 10 20    }T
  T{ 10       { }                  20                 <T> 10 20    }T
  T{          { }                  10 20              <T> 10 20    }T
  T{ 10       { -- }                                  <T>          }T
  T{ 10 20    { -- }                                  <T>          }T
  T{ 10       { one }                                 <T>          }T
  T{ 20       { one }              one                <T> 20       }T
  T{ 30       { one }              one one            <T> 30 30    }T
  T{ 40       { one -- }           one one            <T> 40 40    }T
  T{ 10       { -- one }                              <T>          }T
  T{ 10 20    { one two }          one two            <T> 10 20    }T
  T{ 10 20    { one two }                             <T>          }T
  T{ 10 20    { one two -- }                          <T>          }T
  T{ 30 40    { one two }          one                <T> 30       }T
  T{ 50 60    { one two -- }       one                <T> 50       }T
  T{ 10 20    { one two }          two                <T> 20       }T
  T{ 30 40    { one two }          one two            <T> 30 40    }T
  T{ 50 60    { one two }          two one            <T> 60 50    }T
  T{ 10 20    { one two }          two one one        <T> 20 10 10 }T
  T{ 30 40    { one two }          two one two        <T> 40 30 40 }T
  T{ 10 20    { one two }          one two one        <T> 10 20 10 }T
  T{ 30 40    { one two }          one one two        <T> 30 30 40 }T
  T{ 10 20    { one two -- }       one one two        <T> 10 10 20 }T
  T{ 30 40    { one --     }       one one            <T> 30 30    }T
  T{ 30 40    { one -- two }       one one            <T> 30 30    }T
  T{ 10 20    { one --     }       one one            <T> 10 10    }T
  T{ 10 20    { one -- two }       one one            <T> 10 10    }T
  T{ 10 20    { -- one two }                          <T>          }T
  T{ 30 40    { one two -- three } one one two        <T> 30 30 40 }T
  T{ 10 20 30 { one two three }    one two three      <T> 10 20 30 }T
  T{ 10 20 30 { one two three }    three one two      <T> 30 10 20 }T
  T{ 10 20 30 { one two three }    two three one      <T> 20 30 10 }T
  T{ 10 20 30 { -- one two three }                    <T>          }T
  T{ 40 50 60 { -- one two three }                    <T>          }T
  T{ 10 20 30 { -- }                                  <T>          }T
  T{ 10 20    { one two } one two { one two } one two <T> 10 20    }T
  T{ 10 20    { one two } 30 40   { one two } one two <T> 30 40    }T
  T{ 10 20    { one two } 30 40   { two one } one two <T> 40 30    }T
;
test_locals

: test_stack_primitives
  T{ stack_len <T> 0 }T

  10 >stack
  T{ stack_len <T> 1  }T
  T{ stack>    <T> 10 }T
  T{ stack_len <T> 0  }T

  20 >stack
  30 >stack
  T{ stack_len <T> 2  }T
  T{ stack>    <T> 30 }T
  T{ stack_len <T> 1  }T
  T{ stack>    <T> 20 }T
  T{ stack_len <T> 0  }T
;
test_stack_primitives

\ Since `>stack` and `stack>` are intended for moving values
\ between registers and the Forth data stack, using them in
\ interpreted code is meaningless, doesn't change anything.
T{ 10 >stack <T> 10 }T
T{ 20 stack> <T> 20 }T

: test_to_stack_variadic
  T{ stack_len <T> 0 }T
  >>stack
  T{ stack_len <T> 0 }T

  10 >>stack
  T{ stack_len <T> 1  }T
  T{ stack>    <T> 10 }T
  T{ stack_len <T> 0  }T

  20 30 >>stack
  T{ stack_len <T> 2  }T
  T{ stack>    <T> 30 }T
  T{ stack_len <T> 1  }T
  T{ stack>    <T> 20 }T
  T{ stack_len <T> 0  }T

  40 50 60 >>stack
  T{ stack_len <T> 3  }T
  T{ stack>    <T> 60 }T
  T{ stack_len <T> 2  }T
  T{ stack>    <T> 50 }T
  T{ stack_len <T> 1  }T
  T{ stack>    <T> 40 }T
  T{ stack_len <T> 0  }T
;
test_to_stack_variadic

: test_stack_braces
  T{ stack_len <T> 0 }T
  stack{ }
  T{ stack_len <T> 0 }T

  10 >>stack
  T{ stack_len <T> 1 }T
  stack{ one }
  T{ stack_len one <T> 0 10 }T

  10 20 >>stack
  T{ stack_len <T> 2 }T
  stack{ one two }
  T{ stack_len one two <T> 0 10 20 }T

  10 20 30 >>stack
  T{ stack_len <T> 3 }T
  stack{ one two three }
  T{ stack_len one two three <T> 0 10 20 30 }T
;
test_stack_braces

: test_alloca_cell
  Cell alloca { ptr }

  T{ ptr @ <T> ptr @ }T \ Should not segfault. The value is undefined.

  123 ptr !
  T{ ptr @ <T> 123 }T

  234 ptr !
  T{ ptr @ <T> 234 }T
;
test_alloca_cell

: test_alloca_align
  systack_ptr { SP }
  1  alloca   { adr0 }
  2  alloca   { adr1 }
  4  alloca   { adr2 }
  8  alloca   { adr3 }
  16 alloca   { adr4 }
  32 alloca   { adr5 }

  \ Compiler ensures natural alignment up to 16.
  T{ adr0 1  mod <T> 0 }T
  T{ adr1 2  mod <T> 0 }T
  T{ adr2 4  mod <T> 0 }T
  T{ adr3 8  mod <T> 0 }T
  T{ adr4 16 mod <T> 0 }T
  T{ adr5 16 mod <T> 0 }T

  T{ adr0 <T> SP 16 - }T
  T{ adr1 <T> SP 32 - }T
  T{ adr2 <T> SP 48 - }T
  T{ adr3 <T> SP 64 - }T
  T{ adr4 <T> SP 80 - }T
  T{ adr5 <T> SP 112 - }T
;
test_alloca_align

65536 let: SIZE_BIG

: test_alloca_big
  SIZE_BIG alloca { adr0 }
  SIZE_BIG alloca { adr1 }
  SIZE_BIG alloca { adr2 }

  T{ adr0 16 mod <T> 0 }T
  T{ adr1 16 mod <T> 0 }T
  T{ adr2 16 mod <T> 0 }T

  T{ adr0 adr1 - <T> SIZE_BIG }T
  T{ adr1 adr2 - <T> SIZE_BIG }T
;
test_alloca_big

: test_alloca_dynamic { size0 size1 }
  systack_ptr  { SP   }
  size0 alloca { adr0 }
  size1 alloca { adr1 }

  T{ SP   adr0 - <T> size0 }T
  T{ adr0 adr1 - <T> size1 }T

  adr0 @ { -- } \ Should not segfault.
  adr1 @ { -- } \ Should not segfault.
;
32 64 test_alloca_dynamic

1 1 arr: Arr0
2 1 arr: Arr1
3 1 arr: Arr2
1 2 arr: Arr3
1 3 arr: Arr4
2 3 arr: Arr5
3 3 arr: Arr6

T{ Arr0 <T> 1 }T
T{ Arr1 <T> 2 }T
T{ Arr2 <T> 3 }T
T{ Arr3 <T> 2 }T
T{ Arr4 <T> 3 }T
T{ Arr5 <T> 6 }T
T{ Arr6 <T> 9 }T

struct: Typ
  8 1 field: Typ_field0
  4 1 field: Typ_field1
end

\ Field alignment and padding needs to match C.
\ We test alignment for a lot more cases below.
T{        Typ        <T>    16  }T
T{    0   Typ_field0 <T>    0   }T
T{    0   Typ_field1 <T>    8   }T
T{    123 Typ_field0 <T>    123 }T
T{    234 Typ_field0 <T>    234 }T
T{    123 Typ_field1 <T>    131 }T
T{    234 Typ_field1 <T>    242 }T

Typ mem: ptr

T{ ptr Typ_field0 <T> ptr     }T
T{ ptr Typ_field1 <T> ptr 8 + }T

T{    ptr Typ_field0 @ <T> 0  }T
T{ 10 ptr Typ_field0 ! <T>    }T
T{    ptr Typ_field0 @ <T> 10 }T

T{    ptr Typ_field1 @ <T> 0  }T
T{ 20 ptr Typ_field1 ! <T>    }T
T{    ptr Typ_field1 @ <T> 20 }T

T{    ptr Typ_field0 @ <T> 10 }T
T{ 30 ptr Typ_field0 ! <T>    }T
T{    ptr Typ_field0 @ <T> 30 }T

T{    ptr Typ_field1 @ <T> 20 }T
T{ 40 ptr Typ_field1 ! <T>    }T
T{    ptr Typ_field1 @ <T> 40 }T

struct: Typ0 end
T{ Typ0           <T> 0   }T

struct: Typ1
  U8 1 field: Typ1_field
end

T{ Typ1           <T> 1   }T
T{ 123 Typ1_field <T> 123 }T

struct: Typ2
  U8  1 field: Typ2_field0
  U32 1 field: Typ2_field1
end

T{ Typ2            <T> 8   }T
T{ 123 Typ2_field0 <T> 123 }T
T{ 123 Typ2_field1 <T> 127 }T

struct: Typ3
  U32 1 field: Typ3_field0
  U8  1 field: Typ3_field1
end

T{ Typ3            <T> 8   }T
T{ 123 Typ3_field0 <T> 123 }T
T{ 123 Typ3_field1 <T> 127 }T

struct: Typ4
  U8  1 field: Typ4_field0
  U32 1 field: Typ4_field1
  U64 1 field: Typ4_field2
end

T{ Typ4            <T> 16  }T
T{ 123 Typ4_field0 <T> 123 }T
T{ 123 Typ4_field1 <T> 127 }T
T{ 123 Typ4_field2 <T> 131 }T

struct: Typ5
  U32 1 field: Typ5_field0
  U8  1 field: Typ5_field1
  U64 1 field: Typ5_field2
end

T{ Typ5            <T> 16  }T
T{ 123 Typ5_field0 <T> 123 }T
T{ 123 Typ5_field1 <T> 127 }T
T{ 123 Typ5_field2 <T> 131 }T

struct: Typ6
  U32 1 field: Typ6_field0
  U64 1 field: Typ6_field1
  U8  1 field: Typ6_field2
end

T{ Typ6            <T> 24  }T
T{ 123 Typ6_field0 <T> 123 }T
T{ 123 Typ6_field1 <T> 131 }T
T{ 123 Typ6_field2 <T> 139 }T

struct: Typ7
  U64 1 field: Typ7_field0
  U32 1 field: Typ7_field1
  U32 1 field: Typ7_field2
  U8  1 field: Typ7_field3
end

T{ Typ7            <T> 24  }T
T{ 123 Typ7_field0 <T> 123 }T
T{ 123 Typ7_field1 <T> 131 }T
T{ 123 Typ7_field2 <T> 135 }T
T{ 123 Typ7_field3 <T> 139 }T

struct: Typ8
  U8  1 field: Typ8_field0
  U8  1 field: Typ8_field1
  U8  1 field: Typ8_field2
  U32 1 field: Typ8_field3
end

T{ Typ8            <T> 8   }T
T{ 123 Typ8_field0 <T> 123 }T
T{ 123 Typ8_field1 <T> 124 }T
T{ 123 Typ8_field2 <T> 125 }T
T{ 123 Typ8_field3 <T> 127 }T

struct: Typ9
  U8 1   field: Typ9_field0
  U8 128 field: Typ9_field1
end

T{ Typ9            <T> 129 }T
T{ 123 Typ9_field0 <T> 123 }T
T{ 123 Typ9_field1 <T> 124 }T

struct: Typ10
  U8  1   field: Typ10_field0
  U8  2   field: Typ10_field1
  U32 3   field: Typ10_field2
  U8  128 field: Typ10_field3
end

T{ Typ10            <T> 144 }T
T{ 123 Typ10_field0 <T> 123 }T
T{ 123 Typ10_field1 <T> 124 }T \ followed by padding 1
T{ 123 Typ10_field2 <T> 127 }T \ not followed by padding
T{ 123 Typ10_field3 <T> 139 }T

struct: Typ11
  5   3 field: Typ11_field0
  U64 1 field: Typ11_field1
end

T{ Typ11            <T> 24  }T
T{ 123 Typ11_field0 <T> 123 }T
T{ 123 Typ11_field1 <T> 139 }T

: test_catch_invalid
  \ catch'  nop \ Must fail to compile: `nop` doesn't throw.
  \ catch'' nop \ Must fail to compile: `nop` not in `WORDLIST_COMP`.
;

: test_throw throw" test_err" ;

: test_catch_0_val { -- val }
  false if test_throw end
  123
;

: test_catch_0
  T{ test_catch_0_val          <T> 123     }T
  T{ catch' test_catch_0_val   <T> 123 nil }T
  T{ catch' test_throw { err } <T>         }T
  T{ c" test_err" err cstr=    <T> true    }T
;
test_catch_0

: test_catch_1_cond { one -- two }
  one if one 2 * ret end
  throw" test_err"
  unreachable
  nil
;

: test_catch_1
  T{ 123 test_catch_1_cond                    <T> 246     }T
  T{ 123 catch' test_catch_1_cond             <T> 246 nil }T
  T{ 0   catch' test_catch_1_cond { val err } <T>         }T
  T{ c" test_err" err cstr=                   <T> true    }T

  \ Accident of register allocation. We're not attached to this behavior.
  T{ val <T> err }T
;

: test_catch_2_cond { one two -- three four }
  one <>0 { ok0 }
  two <>0 { ok1 }

  ok0 ok1 and if
    one two + { three }
    two one - { four  }
    three four ret
  end

  throw" test_err"
  unreachable
  nil nil
;

: test_catch_2
  T{ 13 25 test_catch_2_cond        <T> 38 12     }T
  T{ 13 25 catch' test_catch_2_cond <T> 38 12 nil }T

  T{ 0  13 catch' test_catch_2_cond { one two err } <T>      }T
  T{ c" test_err" err cstr=                         <T> true }T

  \ Accident of register allocation. We're not attached to this behavior.
  T{ one two <T> err true }T

  T{ 12 0  catch' test_catch_2_cond { one two err } <T>      }T
  T{ c" test_err" err cstr=                         <T> true }T

  \ Accident of register allocation. We're not attached to this behavior.
  T{ one two <T> err 0 }T
;
test_catch_2

\ Procedures with `catches` never throw IMPLICITLY, but we allow to throw
\ EXPLICITLY. The compiler marks the procedure as "throwing", in addition
\ to "catching". Callers handle its exception as usual.
: test_catches_and_throws_fun [ true catches ]
  test_throw { err_ignore }
  throw" different_test_err"
;

\ Note: when testing `catches`, we have to replace `}T` with `}T`
\ because `}T` throws on test failures, and `catches` reveals those
\ exceptions as values. The code wouldn't compile without `try` due
\ to arity mismatch.
: test_catches_and_throws [ true catches ]
  test_catches_and_throws_fun { err }
  T{ c" different_test_err" err cstr= <T> true }T
;
test_catches_and_throws

: test_catches_0
  [ true catches ]

  test_throw   { err0 }
  c" test_err" { err1 }

  T{ err0 err1 =     <T> false }T
  T{ err0 err1 cstr= <T> true  }T
;
test_catches_0

: test_catches_1_fun { one -- two }
  one ifn test_throw end
  one 2 *
;

\ Compare with `test_catches_1` which is the actual test.
: test_catches_1_fun_throwing
  \ 0 test_catches_1_fun { -- } \ Must throw.

  T{ 123 test_catches_1_fun { val } <T>     }T
  T{ val                            <T> 246 }T
  T{ 123 test_catches_1_fun         <T> 246 }T
;
test_catches_1_fun_throwing

: test_catches_1
  [ true catches ]

  T{ 0 test_catches_1_fun { val err } <T>      }T
  T{ c" test_err" err cstr=           <T> true }T

  \ Accident of register allocation. We're not attached to this behavior.
  T{ val <T> err }T

  T{ 123 test_catches_1_fun { val err } <T>         }T
  T{ val err                            <T> 246 nil }T
  T{ 123 test_catches_1_fun             <T> 246 nil }T
;
test_catches_1

: test_catches_2_fun { one two -- three }
  one ifn test_throw end
  one two +
;

\ Compare with `test_catches_2` which is the actual test.
: test_catches_2_fun_throwing
  \ 0 123 test_catches_2_fun { -- } \ Must throw.

  T{ 11 22 test_catches_2_fun { val } <T>    }T
  T{ val                              <T> 33 }T
  T{ 11 22 test_catches_2_fun         <T> 33 }T

  T{ 11 0 test_catches_2_fun { val } <T>    }T
  T{ val                             <T> 11 }T
  T{ 11 0 test_catches_2_fun         <T> 11 }T
;
test_catches_2_fun_throwing

: test_catches_2
  [ true catches ]

  T{ 0 11 test_catches_2_fun { val err } <T>      }T
  T{ c" test_err" err cstr=              <T> true }T

  \ Accident of register allocation. We're not attached to this behavior.
  T{ val <T> err }T

  T{ 11 22 test_catches_2_fun { val err } <T>        }T
  T{ val err                              <T> 33 nil }T
  T{ 11 22 test_catches_2_fun             <T> 33 nil }T

  T{ 11 0 test_catches_2_fun { val err } <T>        }T
  T{ val err                             <T> 11 nil }T
  T{ 11 0 test_catches_2_fun             <T> 11 nil }T
;
test_catches_2

: test_catches_3_fun { one -- two three }
  one ifn test_throw end
  one 2 *    { two }
  one negate { three }
  two three
;

\ Compare with `test_catches_3` which is the actual test.
: test_catches_3_fun_throwing
  \ 0 test_catches_3_fun { -- } \ Must throw.

  T{ 123 negate <T> -123 }T

  T{ 123 test_catches_3_fun { one two } <T>          }T
  T{ one two                            <T> 246 -123 }T
  T{ 123 test_catches_3_fun             <T> 246 -123 }T
;
test_catches_3_fun_throwing

: test_catches_3
  [ true catches ]

  T{ 0 test_catches_3_fun { one two err } <T>      }T
  T{ c" test_err" err cstr=               <T> true }T

  \ Accident of register allocation. We're not attached to this behavior.
  T{ one <T> err }T
  T{ two <T> 0   }T

  T{ 123 test_catches_3_fun { one two err } <T>              }T
  T{ one two err                            <T> 246 -123 nil }T
  T{ 123 test_catches_3_fun                 <T> 246 -123 nil }T
;
test_catches_3

: test_catches_4_fun { one two -- three four }
  one ifn test_throw end
  one two + { three }
  two one - { four }
  three four
;

\ Compare with `test_catches_4` which is the actual test.
: test_catches_4_fun_throwing
  \ 0 123 test_catches_4_fun { -- } \ Must throw.

  T{ 13 25 test_catches_4_fun { one two } <T>       }T
  T{ one two                              <T> 38 12 }T
  T{ 13 25 test_catches_4_fun             <T> 38 12 }T

  T{ 13 0 test_catches_4_fun { one two } <T>        }T
  T{ one two                             <T> 13 -13 }T
  T{ 13 0 test_catches_4_fun             <T> 13 -13 }T
;
test_catches_4_fun_throwing

: test_catches_4
  [ true catches ]

  T{ 0 13 test_catches_4_fun { one two err } <T>      }T
  T{ c" test_err" err cstr=                  <T> true }T

  \ Accident of register allocation. We're not attached to this behavior.
  T{ one <T> err }T
  T{ two <T> 13  }T

  T{ 13 25 test_catches_4_fun { one two err } <T>           }T
  T{ one two err                              <T> 38 12 nil }T
  T{ 13 25 test_catches_4_fun                 <T> 38 12 nil }T

  T{ 13 0 test_catches_4_fun { one two err } <T>            }T
  T{ one two err                             <T> 13 -13 nil }T
  T{ 13 0 test_catches_4_fun                 <T> 13 -13 nil }T
;
test_catches_4

\ Must compile.
: test_max_input_regs { A B C D E F G H -- } ;

\ Must fail to compile.
\ : test_too_many_input_regs { A B C D E F G H I -- } ;

\ Must compile.
: test_max_output_regs { -- A B C D E F G H } 0 1 2 3 4 5 6 7 ;

\ Must fail to compile.
\ : test_too_many_output_regs { -- A B C D E F G H I } ;

\ Must compile.
: test_max_output_regs_throw { -- A B C D E F G }
  false if test_throw end
  0 1 2 3 4 5 6
;

\ Must fail to compile: error takes up 1 more reg.
\ : test_too_many_output_regs_throw { -- A B C D E F G H }
\   false if test_throw end
\   0 1 2 3 4 5 6 7
\ ;

log" [test] ok" lf
