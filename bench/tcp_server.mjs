// BOT-GENERATED

const READY = new Uint8Array([`R`.charCodeAt(0)])
const DATA = `D`.charCodeAt(0)
const PORT = 19777

Bun.listen({
  hostname: "127.0.0.1",
  port: PORT,
  socket: {
    open(socket) {
      socket.write(READY)
    },
    data(socket, bytes) {
      if (bytes.length !== 1 || bytes[0] !== DATA) {
        throw Error("bad DATA byte")
      }
      socket.end(bytes)
    },
    error(_socket, error) {
      throw error
    },
  },
})
