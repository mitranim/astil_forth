#!/usr/bin/env python3

# BOT-GENERATED

import asyncio


READY = b"R"
DATA = b"D"
PORT = 19777


async def handle(
    reader: asyncio.StreamReader,
    writer: asyncio.StreamWriter,
) -> None:
    try:
        writer.write(READY)
        if await reader.readexactly(1) != DATA:
            raise RuntimeError("bad DATA byte")
        writer.write(DATA)
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
