import' std:lang.f
import' std:internals.f
import' ./http_util.f

\ Simple example of an "HTTP" server which echoes requests
\ back to clients. HTTP in quotes because it only responds
\ with HTTP without parsing the request; the client can be
\ sending anything it wants.

128  let: MAX_CONN
4096 buf: READ_BUF

: write_opt { fd buf len } fd buf len write { -- } ;
: write_eol { fd }         fd CRLF 2 write_opt     ; \ \r\n

\ Oversimplified: doesn't read fully if there's too much, doesn't resize, etc.
: read_req { conn buf cap -- len }
  0 { len }

  loop
    conn buf cap read { wrote }
    wrote ifn len ret end

    wrote is_err if
      errno { code }
      code EINTR = if cont end
      code " unable to read request" os_err throw
    end

    len wrote + { len }
    leave
  end

  len
;

: respond_conn { conn }
  READ_BUF              { buf cap }
  conn buf cap read_req { len }

  " [srv] incoming request:" elog elf elf
  STDERR buf len write_opt elf

  conn s" HTTP/1.1 200 OK"   write_opt conn write_eol
  conn s" Connection: close" write_opt conn write_eol
  conn                                      write_eol
  conn s" request echo:"     write_opt conn write_eol conn write_eol
  conn buf len               write_opt conn write_eol

  " [srv] echoed request back to client" elog elf
;

: handle_conn { conn }
  \ Switch into "C mode" where exceptions are disabled.
  \ Words which "throw" really just return an error as the last output.
  [ true catches ]

  conn respond_conn { err }

  err if
    " [srv] unable to respond; error: %s" err elogf elf
  end

  " [srv] closing connection" elog elf elf

  conn close is_err if
    errno " [srv] unable to close connection" os_err elog elf
  end
;

: main
  " 2345"                  { port }
  instr' handle_conn       { handler }
  port MAX_CONN net_listen { sock }
  sock handler net_accept_loop
  sock sock_close \ Unreachable.
;
main
