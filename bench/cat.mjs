// BOT-GENERATED

/*
Note: `Bun.write(out_file, src_file)` would be slow on MacOS.
It uses `fcopyfile`, which is meant for regular files on disk
and is disastrously slow for non-disk-files such as stdio and
`/dev/null` on the tested system (MacOS 15).
*/

const args = Bun.argv.slice(2)
const out = Bun.stdout.writer({highWaterMark: 64 * 1024})

async function copy(src) {
  for await (const chunk of src.stream()) { // Also closes `src`.
    out.write(chunk)
  }
}

if (args.length === 0) await copy(Bun.stdin)

for (const name of args) {
  const src = name === "-" ? Bun.stdin : Bun.file(name)
  await copy(src)
}

await out.end()
