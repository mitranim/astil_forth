import' std:lang_s.f

\ Nicer loops.

: main
  42 0 1
  +loop: ind
    ind 5  < while
    ind 8  < while
    ind 12 < while
    ind [ 1 ] logf" index: %zd" lf
  end \ Terminates everything.

  loop
    leave
    leave
    leave
    leave
    leave
  end \ Terminates everything.
;
main
