import' ./f_lang.f
import' ./f_testing.f

: test_conditionals
  T{ false #if  10 #end                                                      <T>    }T
  T{ true  #if  10 #end                                                      <T> 10 }T
  T{ false #if  10 #else 20 #end                                             <T> 20 }T
  T{ true  #if  10 #else 20 #end                                             <T> 10 }T
  T{ false #ifn 10 #end                                                      <T> 10 }T
  T{ true  #ifn 10 #end                                                      <T>    }T
  T{ false #ifn 10 #else 20    #end                                          <T> 10 }T
  T{ true  #ifn 10 #else 20    #end                                          <T> 20 }T
  T{ true  #if  10 #else true  #elif  20 #end                                <T> 10 }T
  T{ false #if  10 #else true  #elif  20 #end                                <T> 20 }T
  T{ false #if  10 #else true  #elif  20 #else 30    #end                    <T> 20 }T
  T{ false #if  10 #else false #elif  20 #else 30    #end                    <T> 30 }T
  T{ true  #if  10 #else true  #elif  20 #else true  #elif  30 #end          <T> 10 }T
  T{ false #if  10 #else true  #elif  20 #else false #elif  30 #end          <T> 20 }T
  T{ false #if  10 #else false #elif  20 #else true  #elif  30 #end          <T> 30 }T
  T{ false #if  10 #else false #elif  20 #else false #elif  30 #end          <T>    }T
  T{ true  #if  10 #else true  #elif  20 #else true  #elif  30 #else 40 #end <T> 10 }T
  T{ false #if  10 #else true  #elif  20 #else false #elif  30 #else 40 #end <T> 20 }T
  T{ false #if  10 #else false #elif  20 #else true  #elif  30 #else 40 #end <T> 30 }T
  T{ false #if  10 #else false #elif  20 #else false #elif  30 #else 40 #end <T> 40 }T
  T{ false #ifn 10 #else false #elifn 20 #else false #elifn 30 #else 40 #end <T> 10 }T
  T{ true  #ifn 10 #else false #elifn 20 #else false #elifn 30 #else 40 #end <T> 20 }T
  T{ true  #ifn 10 #else true  #elifn 20 #else false #elifn 30 #else 40 #end <T> 30 }T
  T{ true  #ifn 10 #else true  #elifn 20 #else true  #elifn 30 #else 40 #end <T> 40 }T
;
test_conditionals

: test_recursion ( val -- )
  dup 13 <= #if #ret #end
  dec #recur
;
T{ 123 test_recursion <T> 13 }T

1 0 extern: sleep

: test_sleep_loop #loop log" sleeping" cr 1 sleep #end ;
\ test_sleep_loop

: test_loop_leave
  T{ #loop    #leave                        #end <T>    }T
  T{ #loop 10 #leave                        #end <T> 10 }T
  T{ #loop 10 #leave                        #end <T> 10 }T
  T{ #loop 10 #leave    #leave              #end <T> 10 }T
  T{ #loop 10 #leave    #leave    #leave    #end <T> 10 }T
  T{ #loop 10 #leave 20 #leave    #leave    #end <T> 10 }T
  T{ #loop 10 #leave 20 #leave 30 #leave    #end <T> 10 }T
  T{ #loop 10 #leave 20 #leave 30 #leave 40 #end <T> 10 }T
;
test_loop_leave

: test_loop_nested
  T{
    #loop
      10
      #loop
        20 #leave
      #end
      30
      #leave
    #end
    <T>
    10 20 30
  }T
;
test_loop_nested

: test_loop_while
  T{ #loop    false #while 10              #end <T>       }T
  T{ #loop 10 false #while 20              #end <T> 10    }T
  T{ #loop 10 false #while 20 true  #while #end <T> 10    }T
  T{ #loop 10 true  #while 20 false #while #end <T> 10 20 }T

  T{
    11
    #loop
      dup 5 > #while dup dec
    #end
    drop
    <T>
    11 10 9 8 7 6
  }T

  T{
    11
    #loop
      dup 5 > #while
      dup 7 > #while
      dup dec
    #end
    drop
    <T>
    11 10 9 8
  }T

  T{
    12 #loop
      dec
      dup #while
      dup #while
      dup #while
      dup
    #end
    <T>
    11 10 9 8 7 6 5 4 3 2 1 0
  }T
;
test_loop_while

: test_loop_while_leave
  T{ #loop    false #while 10 #leave    #end <T>       }T
  T{ #loop    true  #while 10 #leave    #end <T> 10    }T
  T{ #loop 10 false #while 20 #leave    #end <T> 10    }T
  T{ #loop 10 true  #while 20 #leave    #end <T> 10 20 }T
  T{ #loop 10 true  #while    #leave 20 #end <T> 10    }T
  T{ #loop #leave 10 true #while 20     #end <T>       }T
;
test_loop_while_leave

: test_loop_until
  T{ #loop                         false  #until <T>    }T
  T{ #loop 10                      false  #until <T> 10 }T
  T{ #loop 10 #leave               true   #until <T> 10 }T
  T{ #loop 10 #leave #leave        true   #until <T> 10 }T
  T{ #loop 10 #leave #leave #leave true   #until <T> 10 }T
  T{ #loop 10 #leave #leave #leave #leave #until <T> 10 }T

  T{
    12
    #loop dec dup dup 6 > #until
    drop
    <T>
    11 10 9 8 7 6
  }T

  T{
    12
    #loop
      dec dup
      dup 8 > #while
      dup 6 >
    #until
    drop
    <T>
    11 10 9 8
  }T

  T{
    12 #loop dup dec dup #until
    <T>
    12 11 10 9 8 7 6 5 4 3 2 1 0
  }T
;
test_loop_until

: test_loop_for_anon
  T{ 4 #for 10        #end <T> 10 10 10 10 }T
  T{ 4 #for 10 #leave #end <T> 10          }T
  T{ 4 #for #leave 10 #end <T>             }T

  T{
    0
    12 #for
      dup 5 > #if #leave #end
      dup inc
    #end
    drop
    <T>
    0 1 2 3 4 5
  }T

  T{
    0
    12 #for
      dup 6 < #while
      dup inc
    #end
    drop
    <T>
    0 1 2 3 4 5
  }T

  T{
    0
    12 #for
      dup 5 < #while
      dup 8 < #while
      dup inc
    #end
    <T>
    0 1 2 3 4 5
  }T

  T{
    0
    12 #for
      dup 8 < #while
      dup 5 < #while
      dup inc
    #end
    <T>
    0 1 2 3 4 5
  }T
;
test_loop_for_anon

: test_loop_for_minus
  T{ 12 -for: ind ind        #end <T> 11 10 9 8 7 6 5 4 3 2 1 0 }T
  T{ 12 -for: ind ind #leave #end <T> 11                        }T

  T{
    12 -for: ind
      ind 5 <= #if #leave #end
      ind
    #end
    <T>
    11 10 9 8 7 6
  }T

  T{
    12 -for: ind
      ind 5 > #while
      ind
    #end
    <T>
    11 10 9 8 7 6
  }T

  T{
    12 -for: ind
      ind 7 > #while
      ind 5 > #while
      ind
    #end
    <T>
    11 10 9 8
  }T

  T{
    12 -for: ind
      ind 5 > #while
      ind 7 > #while
      ind
    #end
    <T>
    11 10 9 8
  }T
;
test_loop_for_minus

: test_loop_for_plus
  T{ 12 -1  +for: ind ind        #end <T> -1 0 1 2 3 4 5 6 7 8 9 10 11 }T
  T{ 12 0   +for: ind ind        #end <T>    0 1 2 3 4 5 6 7 8 9 10 11 }T
  T{ 12 1   +for: ind ind        #end <T>      1 2 3 4 5 6 7 8 9 10 11 }T
  T{ 12 -1  +for: ind ind #leave #end <T> -1                           }T
  T{ 12 0   +for: ind ind #leave #end <T> 0                            }T
  T{ 12 1   +for: ind ind #leave #end <T> 1                            }T

  T{ 12 1 +for: ind true #while ind #leave #end <T> 1 }T
  T{ 12 1 +for: ind #leave true #while ind #end <T>   }T

  T{
    12 0 +for: ind
      ind 5 > #if #leave #end
      ind
    #end
    <T>
    0 1 2 3 4 5
  }T

  T{
    12 0 +for: ind
      ind 6 < #while
      ind
    #end
    <T>
    0 1 2 3 4 5
  }T

  T{
    12 0 +for: ind
      ind 8 < #while
      ind 6 < #while
      ind
    #end
    <T>
    0 1 2 3 4 5
  }T

  T{
    12 0 +for: ind
      ind 6 < #while
      ind 8 < #while
      ind
    #end
    <T>
    0 1 2 3 4 5
  }T
;
test_loop_for_plus

: test_plus_loop
  T{ 8 0 1 +loop: ind ind #end <T> 0 1 2 3 4 5 6 7 }T
  T{ 8 0 2 +loop: ind ind #end <T> 0 2 4 6         }T
  T{ 8 0 3 +loop: ind ind #end <T> 0 3 6           }T
;
test_plus_loop

: test_plus_loop_nested
  T{
    4 0 1
    +loop: one
      one
      6 one 1
      +loop: two
        two
      #end
    #end

    <T>

    0 0 1 2 3 4 5
      1 1 2 3 4 5
        2 2 3 4 5
          3 3 4 5
  }T
;
test_plus_loop_nested

: test_minus_loop
  T{ 8 0 1 -loop: ind ind #end <T> 7 6 5 4 3 2 1 0 }T
  T{ 8 0 2 -loop: ind ind #end <T> 6 4 2 0         }T
  T{ 8 0 3 -loop: ind ind #end <T> 5 2             }T
;
test_minus_loop

: test_sc 10 20 30 stack_len .sc ;
\ test_sc

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

T{ 10 20 30 swap_over <T> 20 10 30 }T

T{ 123 0 ?dup <T> 123     }T
T{ 234 1 ?dup <T> 234 234 }T

: test_varargs
  c" numbers (should be 10 20 30): %zd %zd %zd"
  10 20 30 [ 3 va- ] debug_stack printf [ -va ] cr
;
\ test_varargs

: test_fmt
  10 20 30 [ 3 ] logf" numbers: %zu %zu %zu" cr
;
\ test_fmt

: test_efmt
  10 20 30 [ 3 ] elogf" numbers: %zu %zu %zu" cr
;
\ test_efmt

4096 buf: STR_BUF

: test_str_fmt
  STR_BUF 10 20 30 [ 3 ] sf" numbers: %zu %zu %zu"
  STR_BUF drop puts
;
\ test_str_fmt

: test_sthrowf
  STR_BUF 10 20 30 [ 3 ] sthrowf" codes: %zu %zu %zu"
;
\ test_sthrowf

: test_throwf
  10 20 30 [ 3 ] throwf" codes: %zu %zu %zu"
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
  T{ 10 20    { one two }          one                <T> 10       }T
  T{ 10 20    { one two }          two                <T> 20       }T
  T{ 10 20    { one two }          one two            <T> 10 20    }T
  T{ 10 20    { one two }          two one            <T> 20 10    }T
  T{ 10 20    { one two }          two one one        <T> 20 10 10 }T
  T{ 10 20    { one two }          two one two        <T> 20 10 20 }T
  T{ 10 20    { one two }          one two one        <T> 10 20 10 }T
  T{ 10 20    { one two }          one one two        <T> 10 10 20 }T
  T{ 10 20    { one two -- }       one one two        <T> 10 10 20 }T
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

str" [test] ok" etype ecr eflush
