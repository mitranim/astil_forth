import' ../../forth/lang_s.f

1 1 extern: strdup
2 1 extern: fopen
1 1 extern: fgetc

: main
  c" presentation/04_cheat_quine.f" strdup { path }
  c" r"           { mode }
  path mode fopen { file }

  #loop
    file fgetc { char }
    char 256 < #while
    char putchar
  #end

  path free
;
main
