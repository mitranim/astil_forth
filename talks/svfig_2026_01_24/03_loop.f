import' std:lang_s.f

\ Nicer loops.

: main
  42 0 1
  +loop: ind
    ind 5  < while
    ind 8  < while
    ind 12 < while
    " index: %zd" ind [ 1 ] logf lf
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
