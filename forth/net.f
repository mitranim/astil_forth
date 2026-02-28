import' ./lang.f
import' ./io.f

\ ## Libc network IO
\
\ Incomplete declarations of `libc` network interfaces
\ translated from C definitions on MacOS 15.3 (24D60).
\
\ Procedures which return -1 on error should usually
\ be followed by the test `int_err` or `try_errno"`:
\
\   some_word try_errno" unable to blah"

1      let: AI_PASSIVE   \ Any host IP is fine.
0      let: AF_UNSPEC    \ Both IPv4 and IPv6.
1      let: SOCK_STREAM  \ TCP.
0xffff let: SOL_SOCKET   \ Tells `setsockopt` to configure socket itself.
4      let: SO_REUSEADDR \ Enables local address reuse.

\ Flags for `recv`.
1       let: MSG_OOB
1 1 lsl let: MSG_PEEK
1 6 lsl let: MSG_WAITALL

4 1 extern: getaddrinfo
1 0 extern: freeaddrinfo
1 1 extern: gai_strerror
5 1 extern: setsockopt
3 1 extern: socket
3 1 extern: bind
2 1 extern: listen
3 1 extern: accept
4 1 extern: send
4 1 extern: recv

struct: Sockaddr
  U8 1  field: sa_len
  U8 1  field: sa_family
  U8 14 field: sa_data
end

struct: Addrinfo
  Cint  1 field: ai_flags
  Cint  1 field: ai_family
  Cint  1 field: ai_socktype
  Cint  1 field: ai_protocol
  Cuint 1 field: ai_addrlen
  Cstr  1 field: ai_canonname
  Adr   1 field: ai_addr      \ Sockaddr *
  Adr   1 field: ai_next      \ Addrinfo *
end

U8 128 arr: Sockaddr_storage
