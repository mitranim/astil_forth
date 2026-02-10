import' ../forth/lang_r.f

: fib { ind -- out }
  ind 1 <= if 1 ret end
  ind 1 - recur { prev }
  ind 2 - recur
  prev +
;

: main
  36 fib { val }

  \ log" fib(36): " val log_int lf
;
main
