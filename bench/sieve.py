# BOT-TRANSLATED

def reset(flags):
  flags[:] = b"\x01" * len(flags)

def find_prime(flags):
  reset(flags)

  num = 0
  step = 3

  for ind in range(len(flags)):
    if flags[ind] != 0:
      ind1 = ind + step
      while ind1 < len(flags):
        flags[ind1] = 0
        ind1 += step
      num += 1

    step += 2

  return num

def run(length, runs):
  flags = bytearray(length)
  out = 0
  while runs > 0:
    out = find_prime(flags)
    runs -= 1
  if out != 1899:
    raise RuntimeError(f"mismatch: expected 1899; got {out}")

run(8192, 16384)
