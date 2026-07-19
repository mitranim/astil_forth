// BOT-ASSISTED

const CAP = 1 << 16
const RUNS = (1 << 27) / (CAP / 16)
const WANT = (1 << 27) * 9
const pat = Buffer.from("{a,b:c[d]e} \n\tfg")
const buf = new Uint8Array(CAP)
const dels = new Uint8Array(256)
const LBRACE = "{".charCodeAt(0)
const RBRACE = "}".charCodeAt(0)
const LBRACK = "[".charCodeAt(0)
const RBRACK = "]".charCodeAt(0)
const COLON = ":".charCodeAt(0)
const COMMA = ",".charCodeAt(0)
const SPACE = " ".charCodeAt(0)
const LF = "\n".charCodeAt(0)
const TAB = "\t".charCodeAt(0)

dels[LBRACE] = 1
dels[RBRACE] = 1
dels[LBRACK] = 1
dels[RBRACK] = 1
dels[COLON] = 1
dels[COMMA] = 1
dels[SPACE] = 1
dels[LF] = 1
dels[TAB] = 1

for (let ind = 0; ind < CAP; ind++) buf[ind] = pat[ind % pat.length]

function scan(buf, dels) {
  let out = 0
  for (let ind = 0; ind < buf.length; ind++) out += dels[buf[ind]]
  return out
}

let out = 0
for (let run = 0; run < RUNS; run++) out += scan(buf, dels)
if (out !== WANT) throw Error(`mismatch: expected ${WANT}; got ${out}`)
