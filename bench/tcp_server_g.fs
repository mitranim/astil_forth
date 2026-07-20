\ BOT-GENERATED

require unix/socket.fs
require tasker.fs
require tcp_reuse_g.fs

create tcp-byte 1 allot

19777 constant tcp-port
4096 constant worker-task-size

: create-reusable-server ( port -- listener )
  sockaddr-tmp 4 cells erase
  htonl PF_INET or sockaddr-tmp !
  PF_INET SOCK_STREAM 0 socket
  dup 0< abort" no free socket" >r
  r@ reuse-address 0<> abort" setsockopt failed"
  r@ sockaddr-tmp 16 bind 0= if r> exit then
  r> drop true abort" bind failed"
;

: nonblocking ( fd -- ) dup 3 0 fcntl 4 or 4 swap fcntl 0< abort" fcntl failed" ;

: retryable? ( errno -- flag ) dup 4 = swap dup 11 = swap 35 = or or ;

: recv-byte ( fd -- n )
  begin
    dup tcp-byte 1 0 recv dup 0<
  while
    drop errno retryable? 0= if -1 exit then
    pause
  repeat
  nip
;

: worker ( fd -- )
  >r
  [char] R tcp-byte c!
  r@ tcp-byte 1 0 send 1 <> abort" short READY write"
  r@ recv-byte
  1 <> tcp-byte c@ [char] D <> or abort" bad DATA byte"
  r@ tcp-byte 1 0 send 1 <> abort" short echo write"
  r> closesocket drop
;

: spawn-worker ( fd -- )
  worker-task-size NewTask >r 1 r> pass
  worker
;

: run ( -- )
  tcp-port create-reusable-server { listener }
  listener 128 listen
  listener nonblocking
  begin
    listener 0 0 accept() dup 0< if
      drop errno retryable? 0= abort" accept failed"
    else
      dup nonblocking
      spawn-worker
    then
    pause
  again
;

run
