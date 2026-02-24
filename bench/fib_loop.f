import' ../forth/lang.f

: fib { ind -- out }
  0 1 { prev next }
  ind for
    prev next + next { next prev }
  end
  next
;

: main
  1 16 lsl
  for 91 fib { -- } end

  \ log" fib(91): " 91 fib log_int lf
;
main
