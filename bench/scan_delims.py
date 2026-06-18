# BOT-GENERATED

import os

CAP = 1 << 16
REPS = (1 << 24) // (CAP // 16)
WANT = (1 << 24) * 9
PAT = b"{a,b:c[d]e} \n\tfg"
DELS = b"{}[]:, \n\t"


def main():
    dels = DELS
    buf = PAT * (CAP // len(PAT))
    out = 0
    for _ in range(REPS):
        for c in buf:
            out += c in dels
    if os.getenv("SCAN_DELIMS_PRINT"):
        print(out)
    raise SystemExit(0 if out == WANT else 1)


if __name__ == "__main__":
    main()
