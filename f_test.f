import' ./f_lang.f
import' ./f_testing.f

: test_conditionals
  T{ false #if  10          #end <T>    }T
  T{ true  #if  10          #end <T> 10 }T
  T{ false #if  10 #else 20 #end <T> 20 }T
  T{ true  #if  10 #else 20 #end <T> 10 }T
  T{ false #ifn 10          #end <T> 10 }T
  T{ true  #ifn 10          #end <T>    }T
  T{ false #ifn 10 #else 20 #end <T> 10 }T
  T{ true  #ifn 10 #else 20 #end <T> 20 }T
;
test_conditionals

1 0 extern: sleep

: sleep_loop #begin log" sleeping" cr 1 sleep #again ;
: test_again sleep_loop ;
\ test_again

: countdown_repeat ( val -- )
  #begin
    debug_top_int
    dup #while
    dup #while
    dup #while
    dec
  #repeat #end #end
  debug_top_int
  drop
;

: test_repeat 12 countdown_repeat ;
\ test_repeat

: countdown_until ( val -- )
  #begin
    debug_top_int
    dec
    dup
  #until
  debug_top_int
  drop
;

: test_until 12 countdown_until ;
\ test_until

: countup_do_loop ( max -- )
  0 #do>
    debug_top_int
  #loop+
  debug_stack
  drop2
;

: test_do_loop 12 countup_do_loop ;
\ test_do_loop

\ `#loop+` patches `#while`; `#end` patches `#do>`.
: countup_do_loop_while ( max -- )
  0 #do>
    dup 6 < #while
    debug_top_int
  #loop+ #end
  debug_stack
  drop2
;

: test_do_loop_while 12 countup_do_loop_while ;
\ test_do_loop_while

: test_sc 10 20 30 depth .sc ;

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

T{ 10 20 30 swap_over <T> 20 10 30 }T

T{ 123 0 ?dup <T> 123     }T
T{ 234 1 ?dup <T> 234 234 }T

123 var_tmp: VAR_TMP
T{ VAR_TMP @     <T> 123 }T
T{ 234 VAR_TMP ! <T>     }T
T{ VAR_TMP @     <T> 234 }T
T{ 345 VAR_TMP ! <T>     }T
T{ VAR_TMP @     <T> 345 }T

123 var: VAR
T{ VAR @     <T> 123 }T
T{ 234 VAR ! <T>     }T
T{ VAR @     <T> 234 }T
T{ 345 VAR ! <T>     }T
T{ VAR @     <T> 345 }T

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
  c" numbers (should be 10 20 30): %zd %zd %zd"
  10 20 30 [ 3 va- ] debug_stack printf [ -va ] cr
;

: test_fmt
  10 20 30 [ 3 ] f" numbers: %zu %zu %zu" cr
;

: test_efmt
  10 20 30 [ 3 ] ef" numbers: %zu %zu %zu" cr
;

4096 buf_tmp: STR_BUF

: test_str_fmt
  STR_BUF 10 20 30 [ 3 ] sf" numbers: %zu %zu %zu"
  STR_BUF drop puts
;

: test_sthrowf
  STR_BUF 10 20 30 [ 3 ] sthrowf" codes: %zu %zu %zu"
;

: test_throwf
  10 20 30 [ 3 ] throwf" codes: %zu %zu %zu"
;

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
  T{          { }                                <T>          }T
  T{          { -- }                             <T>          }T
  T{ 10       { }                                <T> 10       }T
  T{ 10 20    { }                                <T> 10 20    }T
  T{ 10       { }                  20            <T> 10 20    }T
  T{          { }                  10 20         <T> 10 20    }T
  T{ 10       { one }                            <T>          }T
  T{ 10       { one }              one           <T> 10       }T
  T{ 10       { one }              one one       <T> 10 10    }T
  T{ 10       { one -- }           one one       <T> 10 10    }T
  T{ 10       { -- one }                         <T> 10       }T
  T{ 10 20    { one }                            <T> 10       }T
  T{ 10 20    { one }              one           <T> 10 20    }T
  T{ 10 20    { one }              one one       <T> 10 20 20 }T
  T{ 10 20    { one -- }           one one       <T> 10 20 20 }T
  T{ 10 20    { one two }                        <T>          }T
  T{ 10 20    { one two }          one           <T> 10       }T
  T{ 10 20    { one two }          two           <T> 20       }T
  T{ 10 20    { one two }          one two       <T> 10 20    }T
  T{ 10 20    { one two }          two one       <T> 20 10    }T
  T{ 10 20    { one two }          two one one   <T> 20 10 10 }T
  T{ 10 20    { one two }          two one two   <T> 20 10 20 }T
  T{ 10 20    { one two }          one two one   <T> 10 20 10 }T
  T{ 10 20    { one two }          one one two   <T> 10 10 20 }T
  T{ 10 20    { one two -- }       one one two   <T> 10 10 20 }T
  T{ 10 20    { one -- two }       one one       <T> 10 20 20 }T
  T{ 10 20    { one -- two }       one one       <T> 10 20 20 }T
  T{ 10 20    { -- one two }                     <T> 10 20    }T
  T{ 10 20    { one two -- three } one one two   <T> 10 10 20 }T
  T{ 10 20 30 { one two three }    one two three <T> 10 20 30 }T
  T{ 10 20 30 { one two three }    three one two <T> 30 10 20 }T
  T{ 10 20 30 { one two three }    two three one <T> 20 30 10 }T
  T{ 10 20 30 { -- one two three }               <T> 10 20 30 }T
;
test_locals

: test_recursion ( val -- )
  dup 13 <= #if #ret #end
  dec #recur
;
T{ 123 test_recursion <T> 13 }T

str" [test] ok" type cr flush eflush
