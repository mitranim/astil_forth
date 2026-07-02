// BOT-TRANSLATED (with tweaks).

function pseudoRandom(seed) {return (seed * 1309 + 13849) & 65535}

function listInit(list) {
  let seed = 74755
  for (let ind = 0; ind < list.length; ind++) {
    list[ind] = seed = pseudoRandom(seed)
  }
}

function listDump(list) {
  let out = `{`
  for (const val of list) out += ` ` + val
  out += ` }`
  console.log(out)
}

function listVerify(list) {
  for (let ind = 0; ind < list.length - 1; ind++) {
    if (list[ind] > list[ind + 1]) throw Error("[bubble] not sorted")
  }
}

function bubble(list) {
  for (let ceil = list.length - 1; ceil > 0; ceil--) {
    for (let ind = 0; ind < ceil; ind++) {
      if (list[ind] > list[ind + 1]) {
        const tmp = list[ind]
        list[ind] = list[ind + 1]
        list[ind + 1] = tmp
      }
    }
  }
}

function bubbleSort(list) {
  listInit(list)
  bubble(list)
  // listDump(list)
  listVerify(list)
}

bubbleSort(Array(8192))
