// BOT-TRANSLATED

function reverse(str) {
  for (let low = 0, high = str.length - 1; low < high; low++, high--) {
    const tmp = str[low]
    str[low] = str[high]
    str[high] = tmp
  }
}

function run(str, runs) {
  while (runs-- > 0) reverse(str)
  // console.log(str.join(""))
}

run(Array.from("0123456789abcdef"), (1 << 22) + 1)
