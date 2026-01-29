import' ./lang_r.f
import' ./testing_r.f

\ This test suite for reg-CC is written differently from the test suite for
\ stack-CC, because of the various checks and restrictions in the compiler.
\ Control flow structures are not allowed to "return" or "push" any values.
\ The amount of arguments before and after `<T>` is checked at compile time
\ which means we can't write tests which repeatedly "push" values in a loop
\ then check the resulting list of values. We just check the final results.

T{       <T>       }T
T{ 10    <T> 10    }T
T{ 10 20 <T> 10 20 }T

: test_conditionals
  \ false #if 10       #end \ Must fail to compile.
  \ false #if #else 10 #end \ Must fail to compile.

  T{ -1 { val } false #if 10 { val } #end val <T> -1 }T
  T{ -1 { val } true  #if 10 { val } #end val <T> 10 }T

  T{ false #if 10 { val } #else 20 { val } #end val <T> 20 }T
  T{ true  #if 10 { val } #else 20 { val } #end val <T> 10 }T

  T{ -1 { val } false #ifn 10 { val } #end val <T> 10 }T
  T{ -1 { val } true  #ifn 10 { val } #end val <T> -1 }T

  T{ false #ifn 10 { val } #else 20 { val } #end val <T> 10 }T
  T{ true  #ifn 10 { val } #else 20 { val } #end val <T> 20 }T

  T{
    -1 { val } false #if 10 { val } #else false #elif 20 { val } #end val
  <T>
    -1
  }T

  T{
    -1 { val }
    false #if 10 { val } #else false #elif 20 { val } #else 30 { val } #end
    val
  <T>
    30
  }T

  T{
    -1 { val } false #if 10 { val } #else true #elif 20 { val } #end val
  <T>
    20
  }T

  T{
    -1 { val }
    false #if 10 { val } #else true #elif 20 { val } #else 30 { val } #end
    val
  <T>
    20
  }T

  T{
    -1 { val } true #if 10 { val } #else false #elif 20 { val } #end val
  <T>
    10
  }T

  T{
    -1 { val }
    true #if 10 { val } #else false #elif 20 { val } #else 30 { val } #end
    val
  <T>
    10
  }T

  T{
    -1 { val } false #if 10 { val } #else false #elifn 20 { val } #end val
  <T>
    20
  }T

  T{
    -1 { val }
    false #if 10 { val } #else false #elifn 20 { val } #else 30 { val } #end
    val
  <T>
    20
  }T

  T{
    -1 { val } false #if 10 { val } #else true #elifn 20 { val } #end val
  <T>
    -1
  }T

  T{
    -1 { val }
    false #if 10 { val } #else true #elifn 20 { val } #else 30 { val } #end
    val
  <T>
    30
  }T

  T{
    -1 { val } true #if 10 { val } #else false #elifn 20 { val } #end val
  <T>
    10
  }T

  T{
    -1 { val }
    true #if 10 { val } #else false #elifn 20 { val } #else 30 { val } #end
    val
  <T>
    10
  }T
;
test_conditionals

: test_recursion { val -- val }
  val 13 <= #if val #ret #end
  val dec #recur
;
T{ 123 test_recursion <T> 13 }T

1 0 extern: sleep

: test_sleep_loop #loop log" sleeping" lf 1 sleep #end ;
\ test_sleep_loop

: test_loop_leave
  T{ -1 { val } #loop            #leave abort              #end val <T> -1 }T
  T{ -1 { val } #loop 10 { val } #leave abort              #end val <T> 10 }T
  T{ -1 { val } #loop 10 { val } #leave abort #leave abort #end val <T> 10 }T
;
test_loop_leave

: test_loop_nested
  T{
    -1 { one }
    -2 { two }
    -3 { three }
    #loop
      10 { one }
      #loop
        20 { two }
        #leave
      #end
      30 { three }
      #leave
    #end
    one two three
  <T>
    10 20 30
  }T
;
test_loop_nested

: test_loop_while
  T{
    -1 { val }
    #loop false #while 10 { val } #end
    val
  <T>
    -1
  }T

  T{
    -1 { val }
    #loop 10 { val } false #while 20 { val } #end
    val
  <T>
    10
  }T

  T{
    -1 { val }
    #loop 10 { val } false #while 20 { val } true #while #end
    val
  <T>
    10
  }T

  T{
    -1 { val }
    #loop 10 { val } true #while 20 { val } false #while #end
    val
  <T>
    20
  }T

  T{
    12 { val }
    #loop
      val 5 > #while
      val dec { val }
    #end
    val
  <T>
    5
  }T

  T{
    12 { val }
    #loop
      val 5 > #while
      val 7 > #while
      val dec { val }
    #end
    val
  <T>
    7
  }T

  T{
    12 { val }
    #loop
      val dec { val }
      val #while
      val #while
      val #while
    #end
    val
  <T>
    0
  }T
;
test_loop_while

: test_loop_while_leave
  T{ -1 { val } #loop            false #while 10 { val } #leave #end val <T> -1 }T
  T{ -1 { val } #loop            true  #while 10 { val } #leave #end val <T> 10 }T
  T{ -1 { val } #loop 10 { val } false #while 20 { val } #leave #end val <T> 10 }T
  T{ -1 { val } #loop 10 { val } true  #while 20 { val } #leave #end val <T> 20 }T
  T{ -1 { val } #loop 10 { val } true  #while #leave 20 { val } #end val <T> 10 }T
  T{ -1 { val } #loop #leave 10 { val } true  #while 20 { val } #end val <T> -1 }T
;
test_loop_while_leave

: test_loop_until
  T{ -1 { val } #loop                                 false  #until val <T> -1 }T
  T{ -1 { val } #loop 10 { val }                      false  #until val <T> 10 }T
  T{ -1 { val } #loop 10 { val } #leave               true   #until val <T> 10 }T
  T{ -1 { val } #loop 10 { val } #leave #leave        true   #until val <T> 10 }T
  T{ -1 { val } #loop 10 { val } #leave #leave #leave true   #until val <T> 10 }T

  T{ 12 { val } #loop val dec { val } val     #until val <T> 0 }T
  T{ 12 { val } #loop val dec { val } val 6 > #until val <T> 6 }T

  T{
    12 { val }
    #loop val dec { val } val 8 > #while val 6 > #until
    val
  <T>
    8
  }T
;
test_loop_until

: test_loop_for
  T{ -12 #for #end <T> }T
  T{ -1  #for #end <T> }T
  T{ 0   #for #end <T> }T

  T{ 0 { val } 12 #for val inc { val }        #end val <T> 12 }T
  T{ 0 { val } 12 #for val inc { val } #leave #end val <T> 1  }T
  T{ 0 { val } 12 #for #leave val inc { val } #end val <T> 0  }T

  T{
    0 { val }
    12 #for
      val 5 > #if #leave #end
      val inc { val }
    #end
    val
  <T>
    6
  }T

  T{
    0 { val }
    12 #for
      val 6 < #while
      val inc { val }
    #end
    val
  <T>
    6
  }T

  T{
    0 { val }
    12 #for
      val 5 < #while
      val 8 < #while
      val inc { val }
    #end
    val
  <T>
    5
  }T

  T{
    0 { val }
    12 #for
      val 8 < #while
      val 5 < #while
      val inc { val }
    #end
    val
  <T>
    5
  }T
;
test_loop_for

: test_loop_for_countdown
  T{ -12 -for: ind #end <T> }T
  T{ -1  -for: ind #end <T> }T
  T{ 0   -for: ind #end <T> }T

  T{ -1 { ind } 12 -for: ind                          #end ind <T> 0  }T
  T{ -1 { ind } 12 -for: ind              #leave      #end ind <T> 11 }T
  T{ -1 { ind } 12 -for: ind ind 5 <= #if #leave #end #end ind <T> 5  }T
  T{ -1 { ind } 12 -for: ind ind 5 >      #while      #end ind <T> 5  }T

  T{
    -1 { ind }
    12 -for: ind
      ind 7 > #while
      ind 5 > #while
    #end
    ind
  <T>
    7
  }T

  T{
    -1 { ind }
    12 -for: ind
      ind 5 > #while
      ind 7 > #while
    #end
    ind
  <T>
    7
  }T
;
test_loop_for_countdown

: test_loop_for_countup
  T{ -1 -1 +for: ind #end ind <T> -1 }T
  T{ -1 -2 +for: ind #end ind <T> -1 }T
  T{  0 -2 +for: ind #end ind <T>  0 }T
  T{  0  0 +for: ind #end ind <T>  0 }T
  T{  1  0 +for: ind #end ind <T>  1 }T
  T{  2  0 +for: ind #end ind <T>  2 }T
  T{ 12  0 +for: ind #end ind <T> 12 }T

  T{ -1 -2 +for: ind       #leave #end ind <T> -2 }T
  T{ -1 -1 +for: ind       #leave #end ind <T> -1 }T
  T{  0  0 +for: ind       #leave #end ind <T>  0 }T
  T{  1  0 +for: ind       #leave #end ind <T>  0 }T
  T{  1  1 +for: ind       #leave #end ind <T>  1 }T
  T{  2  1 +for: ind       #leave #end ind <T>  1 }T
  T{  2  1 +for: ind false #while #end ind <T>  1 }T
  T{  2  1 +for: ind true  #while #end ind <T>  2 }T

  T{
    -123 { ind }
    12 0 +for: ind
      ind 5 > #if #leave #end
    #end
    ind
  <T>
    6
  }T

  T{
    -123 { ind }
    12 0 +for: ind
      ind 6 < #while
    #end
    ind
  <T>
    6
  }T

  T{
    -123 { ind }
    12 0 +for: ind
      ind 8 < #while
      ind 6 < #while
    #end
    ind
  <T>
    6
  }T

  T{
    -123 { ind }
    12 0 +for: ind
      ind 6 < #while
      ind 8 < #while
    #end
    ind
  <T>
    6
  }T
;
test_loop_for_countup

: test_loop_countup
  T{ 8 0 1 +loop: ind #end ind <T> 8  }T
  T{ 8 0 2 +loop: ind #end ind <T> 8  }T
  T{ 8 0 3 +loop: ind #end ind <T> 9  }T
  T{ 8 0 4 +loop: ind #end ind <T> 8  }T
  T{ 8 0 5 +loop: ind #end ind <T> 10 }T
;
test_loop_countup

: test_loop_countup_nested
  T{
    4 0 1 +loop: one
      7 one 1 +loop: two #end
    #end
    one two
  <T>
    4 7
  }T
;
test_loop_countup_nested

: test_loop_countdown
  T{ 8 0 1 -loop: ind #end ind <T> 0 }T
  T{ 8 0 2 -loop: ind #end ind <T> 0 }T
  T{ 8 0 3 -loop: ind #end ind <T> 2 }T
  T{ 8 0 4 -loop: ind #end ind <T> 0 }T
  T{ 8 0 5 -loop: ind #end ind <T> 3 }T
;
test_loop_countdown

\ Used internally by testing utils. The test is kinda cyclic.
123 var: VAR
T{ VAR @     <T> 123 }T
T{ 234 VAR ! <T>     }T
T{ VAR @     <T> 234 }T
T{ 345 VAR ! <T>     }T
T{ VAR @     <T> 345 }T

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

log" [test] ok" lf
