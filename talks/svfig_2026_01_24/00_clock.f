import' std:lang_s.f

1 0 extern: sleep
1 1 extern: time
27  let:    ESC

: now        nil time ;
: del_line   ESC emit " [1A" type ESC emit " [2K" type ;
: strike_beg ESC emit " [9m" type ;
: strike_end ESC emit " [0m" type ;

: log_time ( min sec -- )
  [ 2 ] logf" elapsed time (mins:secs): %0.2lld:%0.2lld"
;

: main
  now 0 0 false { start mins secs printed }

  loop
    \ Strike-out the previous entry.
    printed if
      del_line
      strike_beg mins secs log_time strike_end lf
    end

    now start -    { elapsed }
    elapsed 60 /   { mins }
    elapsed 60 mod { secs }
    true           { printed }
    mins secs [ 2 ] logf" elapsed time (mins:secs): %0.2lld:%0.2lld" lf
    1 sleep
  end
;
main
