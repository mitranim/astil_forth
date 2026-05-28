# BOT-TRANSLATED

def pseudo_random(seed):
  return (seed * 1309 + 13849) & 65535

def list_init(list):
  seed = 74755
  for ind in range(len(list)):
    seed = pseudo_random(seed)
    list[ind] = seed

def list_verify(list):
  for ind in range(len(list) - 1):
    if list[ind] > list[ind + 1]:
      raise RuntimeError("[bubble] not sorted")

def bubble(list):
  ceil = len(list) - 1
  while ceil > 0:
    for ind in range(ceil):
      if list[ind] > list[ind + 1]:
        list[ind], list[ind + 1] = list[ind + 1], list[ind]
    ceil -= 1

def bubble_sort(length):
  list = [0] * length
  list_init(list)
  bubble(list)
  list_verify(list)
  # print(list)

bubble_sort(8192)
