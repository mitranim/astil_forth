#!/usr/bin/env python3

# BOT-GENERATED

import asyncio


READY = b"R"
DATA = b"D"
CLOSE = b"Q"
PORT = 19777


async def handle(
    reader: asyncio.StreamReader,
    writer: asyncio.StreamWriter,
) -> None:
    try:
        writer.write(READY)
        while True:
            data = await reader.readexactly(1)
            if data not in (DATA, CLOSE):
                raise RuntimeError("bad data byte")
            writer.write(data)
            if data == CLOSE:
                break
    finally:
        writer.close()
        await writer.wait_closed()


async def run() -> None:
    server = await asyncio.start_server(
        handle,
        "127.0.0.1",
        PORT,
    )
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(run())
