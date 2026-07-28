\ Copy of Gforth's `bubble.fs` with cosmetic changes.

\ .( Loading Bubble Sort benchmark...) cr

\ A classical benchmark of an O(n**2) algorithm; Bubble sort
\
\ Part of the programs gathered by John Hennessy for the MIPS
\ RISC project at Stanford. Translated to forth by Marty Fraeman
\ Johns Hopkins University/Applied Physics Laboratory.

\ MM forth2c doesn't have it !
: mybounds over + swap ;

\ 1 cells Constant cell

variable seed ( -- addr )

: initiate-seed ( -- )  74755 seed ! ;
: random  ( -- n )  seed @ 1309 * 13849 + 65535 and dup seed ! ;

32768 constant elements ( -- int)

align create ELEMS elements cells allot

: initiate-list { list len -- }
  initiate-seed
  list len cells + list do random i ! cell +loop
;

: verify-list { list len -- }
  list len 1- cells mybounds do
    i 2@ > abort" bubble-sort: not sorted"
  cell +loop
;

: bubble { list len -- }
  \ ." bubbling..." cr
  len 1 do
    list len i - cells mybounds do
      i 2@ > if i 2@ swap i 2! then
    cell +loop
  loop
;

: bubble-sort { list len -- }
  list len initiate-list
  list len bubble
  list len verify-list
;

: main ( -- )
  ELEMS elements bubble-sort
;
main
