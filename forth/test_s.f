import' ./lang_s.f
import' ./testing_s.f

T{       <T>       }T
T{ 10    <T> 10    }T
T{ 10 20 <T> 10 20 }T

: test_conditionals
  T{ false if  10 end                                                 <T>    }T
  T{ true  if  10 end                                                 <T> 10 }T
  T{ false if  10 else 20 end                                         <T> 20 }T
  T{ true  if  10 else 20 end                                         <T> 10 }T
  T{ false ifn 10 end                                                 <T> 10 }T
  T{ true  ifn 10 end                                                 <T>    }T
  T{ false ifn 10 else 20    end                                      <T> 10 }T
  T{ true  ifn 10 else 20    end                                      <T> 20 }T
  T{ true  if  10 else true  elif  20 end                             <T> 10 }T
  T{ false if  10 else true  elif  20 end                             <T> 20 }T
  T{ false if  10 else true  elif  20 else 30    end                  <T> 20 }T
  T{ false if  10 else false elif  20 else 30    end                  <T> 30 }T
  T{ true  if  10 else true  elif  20 else true  elif  30 end         <T> 10 }T
  T{ false if  10 else true  elif  20 else false elif  30 end         <T> 20 }T
  T{ false if  10 else false elif  20 else true  elif  30 end         <T> 30 }T
  T{ false if  10 else false elif  20 else false elif  30 end         <T>    }T
  T{ true  if  10 else true  elif  20 else true  elif  30 else 40 end <T> 10 }T
  T{ false if  10 else true  elif  20 else false elif  30 else 40 end <T> 20 }T
  T{ false if  10 else false elif  20 else true  elif  30 else 40 end <T> 30 }T
  T{ false if  10 else false elif  20 else false elif  30 else 40 end <T> 40 }T
  T{ false ifn 10 else false elifn 20 else false elifn 30 else 40 end <T> 10 }T
  T{ true  ifn 10 else false elifn 20 else false elifn 30 else 40 end <T> 20 }T
  T{ true  ifn 10 else true  elifn 20 else false elifn 30 else 40 end <T> 30 }T
  T{ true  ifn 10 else true  elifn 20 else true  elifn 30 else 40 end <T> 40 }T
;
test_conditionals

: test_recursion ( val -- val )
  dup 13 <= if ret end
  dec recur
;
T{ 123 test_recursion <T> 13 }T

1 0 extern: sleep

: test_sleep_loop loop log" sleeping" lf 1 sleep end ;
\ test_sleep_loop

: test_loop_leave
  T{ loop    leave                      end <T>    }T
  T{ loop 10 leave                      end <T> 10 }T
  T{ loop 10 leave                      end <T> 10 }T
  T{ loop 10 leave    leave             end <T> 10 }T
  T{ loop 10 leave    leave    leave    end <T> 10 }T
  T{ loop 10 leave 20 leave    leave    end <T> 10 }T
  T{ loop 10 leave 20 leave 30 leave    end <T> 10 }T
  T{ loop 10 leave 20 leave 30 leave 40 end <T> 10 }T
;
test_loop_leave

: test_loop_nested
  T{
    loop
      10
      loop
        20 leave
      end
      30
      leave
    end
    <T>
    10 20 30
  }T
;
test_loop_nested

: test_loop_while
  T{ loop    false while 10             end <T>       }T
  T{ loop 10 false while 20             end <T> 10    }T
  T{ loop 10 false while 20 true  while end <T> 10    }T
  T{ loop 10 true  while 20 false while end <T> 10 20 }T

  T{
    11
    loop
      dup 5 > while dup dec
    end
    drop
    <T>
    11 10 9 8 7 6
  }T

  T{
    11
    loop
      dup 5 > while
      dup 7 > while
      dup dec
    end
    drop
    <T>
    11 10 9 8
  }T

  T{
    12 loop
      dec
      dup while
      dup while
      dup while
      dup
    end
    <T>
    11 10 9 8 7 6 5 4 3 2 1 0
  }T
;
test_loop_while

: test_loop_while_leave
  T{ loop    false while 10 leave    end <T>       }T
  T{ loop    true  while 10 leave    end <T> 10    }T
  T{ loop 10 false while 20 leave    end <T> 10    }T
  T{ loop 10 true  while 20 leave    end <T> 10 20 }T
  T{ loop 10 true  while    leave 20 end <T> 10    }T
  T{ loop leave 10 true while 20     end <T>       }T
;
test_loop_while_leave

: test_loop_until
  T{ loop                      false until <T>    }T
  T{ loop 10                   false until <T> 10 }T
  T{ loop 10 leave             true  until <T> 10 }T
  T{ loop 10 leave leave       true  until <T> 10 }T
  T{ loop 10 leave leave leave true  until <T> 10 }T
  T{ loop 10 leave leave leave leave until <T> 10 }T

  T{
    12
    loop dec dup dup 6 > until
    drop
    <T>
    11 10 9 8 7 6
  }T

  T{
    12
    loop
      dec dup
      dup 8 > while
      dup 6 >
    until
    drop
    <T>
    11 10 9 8
  }T

  T{
    12 loop dup dec dup until
    <T>
    12 11 10 9 8 7 6 5 4 3 2 1 0
  }T
;
test_loop_until

: test_loop_for
  T{ -12 for          end <T>             }T
  T{ -1  for          end <T>             }T
  T{ 0   for          end <T>             }T
  T{ 4   for 10       end <T> 10 10 10 10 }T
  T{ 4   for 10 leave end <T> 10          }T
  T{ 4   for leave 10 end <T>             }T

  T{
    0
    12 for
      dup 5 > if leave end
      dup inc
    end
    drop
    <T>
    0 1 2 3 4 5
  }T

  T{
    0
    12 for
      dup 6 < while
      dup inc
    end
    drop
    <T>
    0 1 2 3 4 5
  }T

  T{
    0
    12 for
      dup 5 < while
      dup 8 < while
      dup inc
    end
    <T>
    0 1 2 3 4 5
  }T

  T{
    0
    12 for
      dup 8 < while
      dup 5 < while
      dup inc
    end
    <T>
    0 1 2 3 4 5
  }T
;
test_loop_for

: test_loop_for_countdown
  T{ -12 -for: ind ind       end <T>                           }T
  T{ -1  -for: ind ind       end <T>                           }T
  T{ 0   -for: ind ind       end <T>                           }T
  T{ 12  -for: ind ind       end <T> 11 10 9 8 7 6 5 4 3 2 1 0 }T
  T{ 12  -for: ind ind leave end <T> 11                        }T

  T{
    12 -for: ind
      ind 5 <= if leave end
      ind
    end
    <T>
    11 10 9 8 7 6
  }T

  T{
    12 -for: ind
      ind 5 > while
      ind
    end
    <T>
    11 10 9 8 7 6
  }T

  T{
    12 -for: ind
      ind 7 > while
      ind 5 > while
      ind
    end
    <T>
    11 10 9 8
  }T

  T{
    12 -for: ind
      ind 5 > while
      ind 7 > while
      ind
    end
    <T>
    11 10 9 8
  }T
;
test_loop_for_countdown

: test_loop_for_countup
  T{ 12 -1  +for: ind ind       end <T> -1 0 1 2 3 4 5 6 7 8 9 10 11 }T
  T{ 12 0   +for: ind ind       end <T>    0 1 2 3 4 5 6 7 8 9 10 11 }T
  T{ 12 1   +for: ind ind       end <T>      1 2 3 4 5 6 7 8 9 10 11 }T
  T{ 12 -1  +for: ind ind leave end <T> -1                           }T
  T{ 12 0   +for: ind ind leave end <T> 0                            }T
  T{ 12 1   +for: ind ind leave end <T> 1                            }T

  T{ 12 1 +for: ind true while ind leave end <T> 1 }T
  T{ 12 1 +for: ind leave true while ind end <T>   }T

  T{
    12 0 +for: ind
      ind 5 > if leave end
      ind
    end
    <T>
    0 1 2 3 4 5
  }T

  T{
    12 0 +for: ind
      ind 6 < while
      ind
    end
    <T>
    0 1 2 3 4 5
  }T

  T{
    12 0 +for: ind
      ind 8 < while
      ind 6 < while
      ind
    end
    <T>
    0 1 2 3 4 5
  }T

  T{
    12 0 +for: ind
      ind 6 < while
      ind 8 < while
      ind
    end
    <T>
    0 1 2 3 4 5
  }T
;
test_loop_for_countup

: test_loop_countup
  T{ 8 0 1 +loop: ind ind end ind <T> 0 1 2 3 4 5 6 7 8 }T
  T{ 8 0 2 +loop: ind ind end ind <T> 0 2 4 6         8 }T
  T{ 8 0 3 +loop: ind ind end ind <T> 0 3 6           9 }T
  T{ 8 0 4 +loop: ind ind end ind <T> 0 4             8 }T
  T{ 8 0 5 +loop: ind ind end ind <T> 0 5            10 }T
;
test_loop_countup

: test_loop_countup_nested
  T{
    4 0 1
    +loop: one
      one
      6 one 1
      +loop: two
        two
      end
    end

    <T>

    0 0 1 2 3 4 5
      1 1 2 3 4 5
        2 2 3 4 5
          3 3 4 5
  }T
;
test_loop_countup_nested

: test_loop_countdown
  T{ 8 0 1 -loop: ind ind end <T> 7 6 5 4 3 2 1 0 }T
  T{ 8 0 2 -loop: ind ind end <T> 6 4 2 0         }T
  T{ 8 0 3 -loop: ind ind end <T> 5 2             }T
;
test_loop_countdown

: test_cont_loop
  T{
    -1
    loop
      inc dup
      dup 7 < if cont end
      leave
    end
    drop
    <T>
    0 1 2 3 4 5 6 7
  }T
;
test_cont_loop

: test_cont_for
  T{
    -1
    7 for
      inc dup
      cont
      leave
    end
    drop
    <T>
    0 1 2 3 4 5 6
  }T
;
test_cont_for

: test_sc 10 20 30 stack_len .sc ;
\ test_sc

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

T{ 10 20 30 swap_over <T> 20 10 30 }T

T{ 123 0 ?dup <T> 123     }T
T{ 234 1 ?dup <T> 234 234 }T

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

: test_varargs
  c" numbers (should be 10 20 30): %zd %zd %zd"
  10 20 30 [ 3 ] va{ debug_stack printf }va lf
;
\ test_varargs

: test_fmt
  10 20 30 [ 3 ] logf" numbers: %zd %zd %zd" lf
;
\ test_fmt

: test_efmt
  10 20 30 [ 3 ] elogf" numbers: %zd %zd %zd" lf
;
\ test_efmt

4096 buf: STR_BUF

: test_str_fmt
  STR_BUF 10 20 30 [ 3 ] sf" numbers: %zd %zd %zd"
  STR_BUF drop puts
;
\ test_str_fmt

: test_sthrowf
  STR_BUF 10 20 30 [ 3 ] sthrowf" codes: %zd %zd %zd"
;
\ test_sthrowf

: test_throwf
  10 20 30 [ 3 ] throwf" codes: %zd %zd %zd"
;
\ test_throwf

: test_to
  T{ 10 to: one <T>       }T
  T{ one        <T> 10    }T
  T{ one one    <T> 10 10 }T
  T{ 20 to: one <T>       }T
  T{ one        <T> 20    }T
  T{ one one    <T> 20 20 }T
  T{ 30 to: two <T>       }T
  T{ two        <T> 30    }T
  T{ one        <T> 20    }T
  T{ one two    <T> 20 30 }T
  T{ two one    <T> 30 20 }T
;
test_to

: test_locals
  T{          { }                                     <T>          }T
  T{          { -- }                                  <T>          }T
  T{ 10       { }                                     <T> 10       }T
  T{ 10 20    { }                                     <T> 10 20    }T
  T{ 10       { }                  20                 <T> 10 20    }T
  T{          { }                  10 20              <T> 10 20    }T
  T{ 10       { one }                                 <T>          }T
  T{ 10       { one }              one                <T> 10       }T
  T{ 10       { one }              one one            <T> 10 10    }T
  T{ 10       { one -- }           one one            <T> 10 10    }T
  T{ 10       { -- one }                              <T> 10       }T
  T{ 10 20    { one }                                 <T> 10       }T
  T{ 10 20    { one }              one                <T> 10 20    }T
  T{ 10 20    { one }              one one            <T> 10 20 20 }T
  T{ 10 20    { one -- }           one one            <T> 10 20 20 }T
  T{ 10 20    { one two }                             <T>          }T
  T{ 10 20    { one two -- }                          <T>          }T
  T{ 10 20    { one two }          one                <T> 10       }T
  T{ 10 20    { one two }          two                <T> 20       }T
  T{ 10 20    { one two }          one two            <T> 10 20    }T
  T{ 10 20    { one two }          two one            <T> 20 10    }T
  T{ 10 20    { one two }          two one one        <T> 20 10 10 }T
  T{ 10 20    { one two }          two one two        <T> 20 10 20 }T
  T{ 10 20    { one two }          one two one        <T> 10 20 10 }T
  T{ 10 20    { one two }          one one two        <T> 10 10 20 }T
  T{ 10 20    { one two -- }       one one two        <T> 10 10 20 }T
  T{ 10 20    { one --     }       one one            <T> 10 20 20 }T
  T{ 10 20    { one -- two }       one one            <T> 10 20 20 }T
  T{ 10 20    { one -- two }       one one            <T> 10 20 20 }T
  T{ 10 20    { -- one two }                          <T> 10 20    }T
  T{ 10 20    { one two -- three } one one two        <T> 10 10 20 }T
  T{ 10 20 30 { one two three }    one two three      <T> 10 20 30 }T
  T{ 10 20 30 { one two three }    three one two      <T> 30 10 20 }T
  T{ 10 20 30 { one two three }    two three one      <T> 20 30 10 }T
  T{ 10 20 30 { -- one two three }                    <T> 10 20 30 }T
  T{ 10 20    { one two } one two { one two } one two <T> 10 20    }T
  T{ 10 20    { one two } 30 40   { one two } one two <T> 30 40    }T
  T{ 10 20    { one two } 30 40   { two one } one two <T> 40 30    }T
;
test_locals

: test_local_ref
  ref: val0 ref: val1 ref: val2 { adr0 adr1 adr2 }
  sysstack_frame_ptr { FP }

  T{ FP   <T> sysstack_frame_ptr }T
  T{ adr0 <T> FP 16 +            }T
  T{ adr1 <T> FP 24 +            }T
  T{ adr2 <T> FP 32 +            }T

  12 23 34 { val0 val1 val2 }
  T{ val0   val1   val2   <T> 12 23 34 }T
  T{ adr0 @ adr1 @ adr2 @ <T> 12 23 34 }T

  -34 -23 -12 { val0 val1 val2 }
  T{ val0   val1   val2   <T> -34 -23 -12 }T
  T{ adr0 @ adr1 @ adr2 @ <T> -34 -23 -12 }T

  21 adr0 ! 32 adr1 ! 43 adr2 !
  T{ val0   val1   val2   <T> 21 32 43 }T
  T{ adr0 @ adr1 @ adr2 @ <T> 21 32 43 }T
;
test_local_ref

: test_catch_invalid
  \ catch'  nop \ Must fail to compile: `nop` doesn't throw.
  \ catch'' nop \ Must fail to compile: `nop` not in `WORDLIST_COMP`.
;

: test_throw throw" test_err" ;

: test_catch_0_val
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

: test_catch_1_cond ( one -- two )
  dup if 2 * ret end
  drop
  throw" test_err"
;

: test_catch_1
  T{ 123 test_catch_1_cond                <T> 246     }T
  T{ 123 catch' test_catch_1_cond         <T> 246 nil }T
  T{ 0   catch' test_catch_1_cond { err } <T>         }T
  T{ c" test_err" err cstr=               <T> true    }T
;
test_catch_1

: test_catch_2_cond ( one two -- three four )
  dup2 <>0 swap <>0 and if * dup ret end
  drop2
  throw" test_err"
;

: test_catch_2
  T{ 11 22 test_catch_2_cond                <T> 242 242     }T
  T{ 11 22 catch' test_catch_2_cond         <T> 242 242 nil }T

  T{ 0  22 catch' test_catch_2_cond { err } <T>             }T
  T{ c" test_err" err cstr=                 <T> true        }T

  T{ 11 0  catch' test_catch_2_cond { err } <T>             }T
  T{ c" test_err" err cstr=                 <T> true        }T
;
test_catch_2

\ Procedures with `catches` never throw IMPLICITLY, but we allow to throw
\ EXPLICITLY. The compiler marks the procedure as "throwing", in addition
\ to "catching". Callers handle its exception as usual.
: test_catches_and_throws_fun [ true catches ]
  test_throw { err_ignore }
  throw" different_test_err"
;

\ Note: in stack-CC, when using `catches`, EVERY call to EVERY word
\ pushes an error to the stack. When the callee doesn't throw, the
\ error is nil. This way, we don't have to remember which words do
\ and do not throw, but we have to remember to use `try`.
: test_catches_and_throws [ true catches ]
  test_catches_and_throws_fun { err }
  T{ c" different_test_err" err cstr= try <T> 1 }T
;
test_catches_and_throws

: test_catches_0
  [ true catches ]

  test_throw   { err0 }
  c" test_err" { err1 }

  T{ err0 err1     = try <T> 0 }T
  T{ err0 err1 cstr= try <T> 1 }T
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

  \ Note: when the callee throws, it fails to push a return value
  \ to the stack. We can't `{ val err }` because of stack underflow.
  \ After catching, the stack is in a partially undefined state: the
  \ error is definitely there, but what else was pushed, is unknown.
  \ This is a problem with stack-CC in general. Compare reg-CC tests.
  T{ 0 test_catches_1_fun { err } <T>   }T
  T{ c" test_err" err cstr= try   <T> 1 }T

  T{ 123 test_catches_1_fun { val err } <T>       }T
  T{ val err                            <T> 246 0 }T
  T{ 123 test_catches_1_fun             <T> 246 0 }T
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

  T{ 0 11 test_catches_2_fun { err } <T>   }T
  T{ c" test_err" err cstr= try      <T> 1 }T

  T{ 11 22 test_catches_2_fun { val err } <T>      }T
  T{ val err                              <T> 33 0 }T
  T{ 11 22 test_catches_2_fun             <T> 33 0 }T

  T{ 11 0 test_catches_2_fun { val err } <T>      }T
  T{ val err                             <T> 11 0 }T
  T{ 11 0 test_catches_2_fun             <T> 11 0 }T
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

  T{ 0 test_catches_3_fun { err } <T>   }T
  T{ c" test_err" err cstr= try   <T> 1 }T

  T{ 123 test_catches_3_fun { one two err } <T>            }T
  T{ one two err                            <T> 246 -123 0 }T
  T{ 123 test_catches_3_fun                 <T> 246 -123 0 }T
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

  T{ 0 13 test_catches_4_fun { err } <T>   }T
  T{ c" test_err" err cstr= try      <T> 1 }T

  T{ 13 25 test_catches_4_fun { one two err } <T>         }T
  T{ one two err                              <T> 38 12 0 }T
  T{ 13 25 test_catches_4_fun                 <T> 38 12 0 }T

  T{ 13 0 test_catches_4_fun { one two err } <T>          }T
  T{ one two err                             <T> 13 -13 0 }T
  T{ 13 0 test_catches_4_fun                 <T> 13 -13 0 }T
;
test_catches_4

log" [test] ok" lf
