#!/usr/bin/env python3

# BOT-GENERATED

import resource
import socket
import time


READY = b"R"
DATA = b"D"
PORT = 19777
CONNECTIONS = 4096
FD_HEADROOM = 256


def ensure_fd_capacity() -> None:
    required = CONNECTIONS + FD_HEADROOM
    soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
    if soft >= required:
        return
    if hard < required:
        raise RuntimeError(
            f"TCP benchmark needs {required} open files; hard limit is {hard}"
        )
    resource.setrlimit(resource.RLIMIT_NOFILE, (required, hard))


def recv_byte(sock: socket.socket) -> bytes:
    data = sock.recv(1)
    if len(data) != 1:
        raise RuntimeError("short TCP read")
    return data


def drive() -> None:
    clients: list[socket.socket] = []
    try:
        for index in range(CONNECTIONS):
            while True:
                client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                try:
                    client.connect(("127.0.0.1", PORT))
                    clients.append(client)
                    break
                except ConnectionRefusedError:
                    client.close()
                    if index:
                        raise
                    time.sleep(0.001)
                except BaseException:
                    client.close()
                    raise

        for client in clients:
            if recv_byte(client) != READY:
                raise RuntimeError("bad TCP READY byte")
        for client in clients:
            client.sendall(DATA)
        for client in clients:
            if recv_byte(client) != DATA:
                raise RuntimeError("bad TCP echo")
        for client in clients:
            if client.recv(1) != b"":
                raise RuntimeError("expected TCP EOF")
    finally:
        for client in clients:
            client.close()
