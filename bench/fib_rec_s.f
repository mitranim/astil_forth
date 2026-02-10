import' ../forth/lang_s.f

: fib { ind -- out }
  ind 1 <= if 1 ret end
  ind 1 - recur
  ind 2 - recur
  +
;

: main
  36 fib

  \ log" fib(36): " log_int lf
;
main
