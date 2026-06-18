// BOT-GENERATED

const CAP = 1 << 16;
const REPS = (1 << 24) / (CAP / 16);
const WANT = (1 << 24) * 9;
const pat = new Uint8Array([123, 97, 44, 98, 58, 99, 91, 100, 93, 101, 125, 32, 10, 9, 102, 103]);
const buf = new Uint8Array(CAP);
for (let i = 0; i < CAP; i++) buf[i] = pat[i & 15];

function isDelim(c) {
  return c === 123 || c === 125 || c === 91 || c === 93 || c === 58 ||
    c === 44 || c === 32 || c === 10 || c === 9;
}

let out = 0;
for (let i = 0; i < REPS; i++) {
  for (let j = 0; j < CAP; j++) out += isDelim(buf[j]);
}
if (process.env.SCAN_DELIMS_PRINT) console.log(out);
if (out !== WANT) process.exit(1);
