# BOT-ASSISTED

import sys


CAP = 65536
RUNS = 2048
OFFSET = 0xCBF29CE484222325
PRIME = 0x100000001B3
MASK = 0xFFFFFFFFFFFFFFFF
WANT = 0xB0A1EA8560222325
OFFSET_HI = 0xCBF29CE4
OFFSET_LO = 0x84222325
PRIME_LO = 0x1B3
BUF = (b"0123456789abcdef" * (CAP // 16))


def fnv1a64_int(hash: int, src: bytes) -> int:
    prime = PRIME
    mask = MASK
    for byte in src:
        hash ^= byte
        hash = (hash * prime) & mask
    return hash


def fnv1a64_u32(hi: int, lo: int, src: bytes) -> tuple[int, int]:
    prime = PRIME_LO
    for byte in src:
        lo = (lo ^ byte) & 0xFFFFFFFF
        carry = (((lo >> 16) * prime) + (((lo & 0xFFFF) * prime) >> 16)) >> 16
        hi = ((hi * prime) + carry + ((lo << 8) & 0xFFFFFFFF)) & 0xFFFFFFFF
        lo = (lo * prime) & 0xFFFFFFFF
    return hi, lo


def main() -> None:
    if sys.implementation.name == "pypy":
        hi = OFFSET_HI
        lo = OFFSET_LO
        for _ in range(RUNS):
            hi, lo = fnv1a64_u32(hi, lo, BUF)
        hash = (hi << 32) | lo
    else:
        hash = OFFSET
        for _ in range(RUNS):
            hash = fnv1a64_int(hash, BUF)
    if hash != WANT:
        raise RuntimeError(f"mismatch: expected {WANT:#x}; got {hash:#x}")


if __name__ == "__main__":
    main()
