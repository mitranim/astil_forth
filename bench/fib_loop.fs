: fib ( depth -- out )
  0 1 ( prev next )
  rot 0 ?do swap over + loop
  nip
;

: main
  0
  1 22 lshift 0 ?do drop 91 fib loop
  7540113804746346429 <>
  abort" iterative Fibonacci output mismatch"
;
main
