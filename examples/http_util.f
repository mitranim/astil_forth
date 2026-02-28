import' std:lang.f
import' std:net.f
import' std:errno.f

\ Nice guide on libc networking: https://beej.us/guide/bgnet/html/

: sock_close { sock }
  sock close is_err if errno_elog" [srv] unable to close socket" end
;

: sock_bind_opt { info max_conn -- sock }
  info Addrinfo_family   @ { fam }
  info Addrinfo_socktype @ { type }
  info Addrinfo_protocol @ { prot }
  fam type prot socket     { sock }

  sock is_err if -1 ret end

  \ `setsockopt` requires non-nil option address.
  Cint alloca { opt }
  nil opt !cint

  sock SOL_SOCKET SO_REUSEADDR opt Cint setsockopt
  is_err if
    sock sock_close
    errno_elog" [srv] unable to `setsockopt`"
    -1 ret
  end

  sock info Addrinfo_addr @ info Addrinfo_addrlen @ bind
  is_err if
    sock sock_close
    errno_elog" [srv] unable to bind socket"
    -1 ret
  end

  sock max_conn listen
  is_err if
    sock sock_close
    errno_elog" [srv] unable to listen on socket"
    -1 ret
  end

  sock
;

\ Result is `Addrinfo *` and should be freed with `freeaddrinfo`.
: net_addr_info { port -- ainf }
  Addrinfo alloca { hint }
  Addrinfo    hint memzero
  AF_UNSPEC   hint Addrinfo_family   !
  SOCK_STREAM hint Addrinfo_socktype !
  AI_PASSIVE  hint Addrinfo_flags    !

  nil { ainf }
  nil port hint ref' ainf getaddrinfo { code }

  code if
    code gai_strerror { msg }
    port code msg throwf" [srv] unable to listen on port %s; code: %zd; msg; %s"
  end

  ainf
;

: net_listen { port max_conn -- sock }
  port net_addr_info { ainf }
  -1                 { sock }
  ainf               { next }

  loop
    next while
    next max_conn sock_bind_opt { sock }
    next Addrinfo_next @ { next }
    sock is_err ifn leave end
  end

  ainf freeaddrinfo

  sock is_err if
    port throwf" [srv] unable to listen on :%s"
  end

  port elogf" [srv] listening on http://localhost:%s" elf
  sock
;

: net_accept_loop { sock handler }
  Sockaddr_storage alloca { addr }
  Cuint            alloca { size }

  loop
    Sockaddr_storage addr memzero
    Sockaddr_storage size !cuint
    sock addr size accept { conn }

    conn is_err if
      errno { code }
      code EINTR = if cont end
      code strerror { msg }
      sock code msg elogf" [srv] unable to accept connection; code: %zd; msg: %s" elf
      cont
    end

    elogf" [srv] accepted connection" elf
    conn handler execute_raw
  end
;
