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
  reverse(str)
  if (str.join("") !== "fedcba9876543210") {
    throw Error("reverse-string output mismatch")
  }
}

run(Array.from("0123456789abcdef"), 1 << 25)
