import' std:lang_s.f

: fib { ind -- out }
  ind 1 <= if 1 ret end
  ind 1 - recur
  ind 2 - recur
  +
;

: main
  36 fib

  \ " fib(36): " log log_int lf
;
main
