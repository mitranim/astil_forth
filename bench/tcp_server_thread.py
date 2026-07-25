#!/usr/bin/env python3

# BOT-GENERATED

import socket
import sys
import threading


READY = b"R"
DATA = b"D"
CLOSE = b"Q"
PORT = 19777


def recv_byte(conn: socket.socket) -> bytes:
    data = conn.recv(1)
    if len(data) != 1:
        raise RuntimeError("short TCP read")
    return data


def handle(conn: socket.socket) -> None:
    with conn:
        conn.sendall(READY)
        while True:
            data = recv_byte(conn)
            if data not in (DATA, CLOSE):
                raise RuntimeError("bad data byte")
            conn.sendall(data)
            if data == CLOSE:
                return


def run() -> None:
    # Socket operations release the GIL. Avoid forced handoffs while thousands
    # of runnable handlers execute their tiny Python sections between them.
    sys.setswitchinterval(1.0)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(("127.0.0.1", PORT))
        listener.listen()
        while True:
            conn, _address = listener.accept()
            threading.Thread(target=handle, args=(conn,), daemon=True).start()


if __name__ == "__main__":
    run()
