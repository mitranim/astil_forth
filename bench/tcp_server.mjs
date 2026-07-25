// BOT-GENERATED

const READY = new Uint8Array([`R`.charCodeAt(0)])
const DATA = `D`.charCodeAt(0)
const CLOSE = `Q`.charCodeAt(0)
const PORT = 19777

Bun.listen({
  hostname: "127.0.0.1",
  port: PORT,
  socket: {
    open(sock) {
      sock.write(READY)
    },
    data(sock, bytes) {
      if (
        bytes.length !== 1 ||
        (bytes[0] !== DATA && bytes[0] !== CLOSE)
      ) throw Error("bad data byte")
      if (bytes[0] === CLOSE) sock.end(bytes)
      else sock.write(bytes)
    },
    error(_sock, err) {throw err},
  },
})
