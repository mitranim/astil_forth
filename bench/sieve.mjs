// BOT-TRANSLATED (with tweaks).

function reset(flags) {flags.fill(1)}

function findPrime(flags) {
  reset(flags)

  let num = 0
  let step = 3

  for (let ind = 0; ind < flags.length; ind++) {
    if (flags[ind]) {
      for (let ind1 = ind + step; ind1 < flags.length; ind1 += step) {
        flags[ind1] = 0
      }
      num++
    }
    step += 2
  }

  return num
}

function run(flags, runs) {
  let out = 0
  for (let ind = 0; ind < runs; ind++) out = findPrime(flags)
  if (out !== 1899) throw Error(`mismatch: expected 1899; got ${out}`)
}

run(new Uint8Array(8192), 16384)
