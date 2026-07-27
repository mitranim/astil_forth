# BOT-GENERATED

import sys


CHUNK_SIZE = 64 * 1024


def copy(source, output):
    # Buffered streams make `shutil.copyfileobj` extremely slow on PyPy here.
    # Raw `FileIO` stays fast; this loop also handles partial writes.
    while chunk := source.read(CHUNK_SIZE):
        remaining = memoryview(chunk)
        while remaining:
            remaining = remaining[output.write(remaining):]


def main():
    stdin = sys.stdin.buffer.raw
    stdout = sys.stdout.buffer.raw

    if not sys.argv[1:]:
        copy(stdin, stdout)
    else:
        for name in sys.argv[1:]:
            if name == "-":
                copy(stdin, stdout)
            else:
                with open(name, "rb", buffering=0) as source:
                    copy(source, stdout)


if __name__ == "__main__":
    main()
