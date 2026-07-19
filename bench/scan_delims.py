# BOT-ASSISTED

CAP = 1 << 16
RUNS = (1 << 27) // (CAP // 16)
WANT = (1 << 27) * 9
PAT = b"{a,b:c[d]e} \n\tfg"
DELS = bytes(1 if c in b"{}[]:, \n\t" else 0 for c in range(256))


def scan(buf: bytes, dels: bytes) -> int:
    out = 0
    for c in buf:
        out += dels[c]
    return out


def main():
    buf = PAT * (CAP // len(PAT))
    out = 0
    for _ in range(RUNS):
        out += scan(buf, DELS)
    if out != WANT:
        raise RuntimeError(f"mismatch: expected {WANT}; got {out}")


if __name__ == "__main__":
    main()
