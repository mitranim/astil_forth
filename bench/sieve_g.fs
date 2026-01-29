\ Slightly modified copy of Gforth's `sieve.fs`.

4096 constant RUNS
8192 constant LEN
create FLAGS LEN allot
FLAGS LEN + constant EFLAG

: find_prime ( -- num )
  FLAGS LEN 1 fill
  0 3
  EFLAG FLAGS do
    i c@ if
      dup i +
      dup EFLAG < if
        EFLAG swap do 0 i c! dup +loop
      else
        drop
      then
      swap 1+ swap
    then
    2 +
  loop
  drop
;

: main
  \ find_prime .

  RUNS 0 do
    find_prime drop
  loop
;
main
