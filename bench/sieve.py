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
  while runs > 0:
    find_prime(flags)
    runs -= 1
  # print(find_prime(flags))

run(8192, 4096)
