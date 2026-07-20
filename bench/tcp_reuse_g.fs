\ BOT-GENERATED

c-library socket_reuse
\c #include <sys/socket.h>
\c static int reuse_address(int fd) {
\c   int one = 1;
\c   return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
\c }
c-function reuse-address reuse_address n -- n
end-c-library
