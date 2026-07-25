// BOT-GENERATED

package main

import (
	"io"
	"net"
)

const ready = byte('R')
const data = byte('D')
const closeConnection = byte('Q')

func handle(conn net.Conn) {
	defer conn.Close()
	if _, err := conn.Write([]byte{ready}); err != nil {
		panic(err)
	}
	var buf [1]byte
	for {
		if _, err := io.ReadFull(conn, buf[:]); err != nil {
			panic(err)
		}
		if buf[0] != data && buf[0] != closeConnection {
			panic("bad data byte")
		}
		if _, err := conn.Write(buf[:]); err != nil {
			panic(err)
		}
		if buf[0] == closeConnection {
			return
		}
	}
}

func main() {
	listener, err := net.Listen("tcp4", "127.0.0.1:19777")
	if err != nil {
		panic(err)
	}

	for {
		conn, err := listener.Accept()
		if err != nil {
			panic(err)
		}
		go handle(conn)
	}
}
