// BOT-GENERATED

const READY = Uint8Array.of("R".charCodeAt(0))
const DATA = "D".charCodeAt(0)
const CLOSE = "Q".charCodeAt(0)
const PORT = 19777

function read(sock) {
  return new Promise(done => sock.data.resume = done)
}

async function handle(sock) {
  sock.write(READY)
  while (true) {
    const bytes = await read(sock)
    if (
      bytes.length !== 1 ||
      (bytes[0] !== DATA && bytes[0] !== CLOSE)
    ) throw Error("bad data byte")
    if (bytes[0] === CLOSE) {
      sock.end(bytes)
      return
    }
    sock.write(bytes)
  }
}

export const handlers = {
  open(sock) {
    sock.data = {}
    sock.data.task = handle(sock)
  },
  data(sock, bytes) {
    const resume = sock.data.resume
    sock.data.resume = undefined
    resume(bytes)
  },
  error(_sock, err) {throw err},
}

if (import.meta.main) {
  Bun.listen({
    hostname: "127.0.0.1",
    port: PORT,
    socket: handlers,
  })
}
