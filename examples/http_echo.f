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
      errno EINTR = if cont end
      errno c" unable to read request" os_throw
    end

    len wrote + { len }
    leave
  end

  len
;

: respond_conn { conn }
  READ_BUF              { buf cap }
  conn buf cap read_req { len }

  elogf" [srv] incoming request:" elf elf
  STDERR buf len write_opt elf

  conn " HTTP/1.1 200 OK"   write_opt conn write_eol
  conn " Connection: close" write_opt conn write_eol
  conn                                     write_eol
  conn " request echo:"     write_opt conn write_eol conn write_eol
  conn buf len              write_opt conn write_eol

  elogf" [srv] echoed request back to client" elf
;

: handle_conn { conn }
  \ Switch into "C mode" where exceptions are disabled.
  \ Words which "throw" really just return an error as the last output.
  [ true catches ]

  conn respond_conn { err }

  err if
    err elogf" [srv] unable to respond; error: %s"
  end

  elogf" [srv] closing connection" elf elf
  conn close is_err if errno_elog" [srv] unable to close connection" end
;

: main
  c" 2345"                 { port }
  instr' handle_conn       { handler }
  port MAX_CONN net_listen { sock }
  sock handler net_accept_loop
  sock sock_close \ Unreachable.
;
main
