// BOT-GENERATED

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

static constexpr uint8_t READY = 'R';
static constexpr uint8_t DATA  = 'D';
static constexpr int     PORT  = 19777;

static void *handle_connection(void *inp) {
  const int conn = (int)(intptr_t)inp;
  uint8_t   byte = READY;

  const bool ok = send(conn, &byte, 1, 0) == 1 &&
    recv(conn, &byte, 1, MSG_WAITALL) == 1 && byte == DATA &&
    send(conn, &byte, 1, 0) == 1;

  if (close(conn) || !ok) exit(1);
  return nullptr;
}

int main(void) {
  const int listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener < 0) {
    perror("socket");
    return 1;
  }
  const int reuse = 1;
  if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    perror("setsockopt");
    return 1;
  }
  const struct sockaddr_in address = {
    .sin_len    = sizeof(address),
    .sin_family = AF_INET,
    .sin_port   = htons(PORT),
    .sin_addr   = {.s_addr = htonl(INADDR_LOOPBACK)},
  };
  if (
    bind(listener, (const struct sockaddr *)&address, sizeof(address)) < 0 ||
    listen(listener, SOMAXCONN) < 0
  ) {
    perror("bind/listen");
    return 1;
  }

  pthread_attr_t attr;
  int            code = pthread_attr_init(&attr);
  if (!code) code = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (code) {
    errno = code;
    perror("pthread_attr");
    return 1;
  }

  for (;;) {
    const int conn = accept(listener, nullptr, nullptr);
    if (conn < 0) {
      perror("accept");
      return 1;
    }
    pthread_t thread;
    code = pthread_create(
      &thread, &attr, handle_connection, (void *)(intptr_t)conn
    );
    if (code) {
      errno = code;
      perror("pthread_create");
      close(conn);
      return 1;
    }
  }
}
