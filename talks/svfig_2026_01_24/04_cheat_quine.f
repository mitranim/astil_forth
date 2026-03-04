import' std:lang_s.f

2 1 extern: fopen
1 1 extern: fgetc
1 0 extern: close

: main
  " talks/svfig_2026_01_24/04_cheat_quine.f" { path }
  " r"                                       { mode }
  path mode fopen                            { file }

  loop
    file fgetc { char }
    char 256 < while
    char putchar
  end

  file close
;
main
