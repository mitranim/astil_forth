\ BOT-GENERATED

64 1024 * constant buffer-size
create copy-buffer buffer-size allot

: copy-input ( fileid -- )
  begin
    dup copy-buffer buffer-size rot read-file throw
    dup
  while
    copy-buffer swap stdout write-file throw
  repeat
  2drop
;

: copy-arg { arg arg-len -- }
  arg arg-len s" -" compare 0= if
    stdin copy-input
    exit
  then

  arg arg-len r/o bin open-file throw { input }
  input copy-input
  input close-file throw
;

variable copied

: main ( -- )
  false copied !

  begin
    next-arg 2dup or
  while
    copy-arg
    true copied !
  repeat
  2drop

  copied @ 0= if
    stdin copy-input
  then
  stdout flush-file throw
;

main bye
