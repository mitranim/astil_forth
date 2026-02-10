import' ../src_f/lang_s.f

\ Nicer control flow structures.

: main { val }
  val 0 =
  #if
    log" branch: 0" lf
  #else

  val 1 =
  #elif
    log" branch: 1" lf
  #else

  val 2 =
  #elif
    log" branch: 2" lf
  #else
    log" branch: none" lf
  #end \ Terminates everything.
;

log" run 0: " 0 main
log" run 1: " 1 main
log" run 2: " 2 main
log" run 3: " 3 main
