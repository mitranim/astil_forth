# BOT-TRANSLATED

def reverse(str):
  low = 0
  high = len(str) - 1
  while low < high:
    str[low], str[high] = str[high], str[low]
    low += 1
    high -= 1

def run(str, runs):
  for _ in range(runs):
    reverse(str)
  # print("".join(str))

run(list("0123456789abcdef"), (1 << 22) + 1)
