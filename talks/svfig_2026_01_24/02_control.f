import' std:lang_s.f

\ Nicer control flow structures.

: main { val }
  val 0 =
  if
    " branch: 0" log lf
  else

  val 1 =
  elif
    " branch: 1" log lf
  else

  val 2 =
  elif
    " branch: 2" log lf
  else
    " branch: none" log lf
  end \ Terminates everything.
;

" run 0: " log 0 main
" run 1: " log 1 main
" run 2: " log 2 main
" run 3: " log 3 main
