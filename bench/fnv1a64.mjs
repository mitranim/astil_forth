// BOT-ASSISTED

const CAP = 65536
const RUNS = 2048
const OFFSET = 0xcbf29ce484222325n
const PRIME_LO = 0x1b3
const WANT = 0xb0a1ea8560222325n
const buf = new Uint8Array(CAP)
const pat = Buffer.from("0123456789abcdef")

for (let ind = 0; ind < CAP; ind++) buf[ind] = pat[ind & 15]

function fnv1a64(hash, src) {
  let hi = Number((hash >> 32n) & 0xffffffffn)
  let lo = Number(hash & 0xffffffffn)
  for (let ind = 0; ind < src.length; ind++) {
    lo = (lo ^ src[ind]) >>> 0
    const carry = (Math.imul(lo >>> 16, PRIME_LO) + (Math.imul(lo & 0xffff, PRIME_LO) >>> 16)) >>> 16
    hi = (Math.imul(hi, PRIME_LO) + carry + ((lo << 8) >>> 0)) >>> 0
    lo = Math.imul(lo, PRIME_LO) >>> 0
  }
  return (BigInt(hi) << 32n) | BigInt(lo)
}

function fmt(hash) {
  return `0x${hash.toString(16).padStart(16, "0")}`
}

let hash = OFFSET
for (let rep = 0; rep < RUNS; rep++) hash = fnv1a64(hash, buf)

if (hash !== WANT) {
  throw Error(`mismatch: expected ${fmt(WANT)}; got ${fmt(hash)}`)
}
