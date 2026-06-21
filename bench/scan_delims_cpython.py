import os

CAP = 1 << 16
RUNS = (1 << 24) // (CAP // 16)
WANT = (1 << 24) * 9
PAT = b"{a,b:c[d]e} \n\tfg"
DELS = b"{}[]:, \n\t"


def scan(buf: bytes, dels: bytes) -> int:
    out = 0
    for c in dels:
        out += buf.count(c)
    return out


def main():
    buf = PAT * (CAP // len(PAT))
    out = 0
    for _ in range(RUNS):
        out += scan(buf, DELS)
    if os.getenv("SCAN_DELIMS_PRINT"):
        print(out)
    if out != WANT:
        raise RuntimeError(f"mismatch: expected {WANT}; got {out}")


if __name__ == "__main__":
    main()
