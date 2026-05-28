-- BOT-TRANSLATED (with tweaks).

local function reset(flags, len)
  for ind = 1, len do
    flags[ind] = 1
  end
end

local function find_prime(flags, len)
  reset(flags, len)

  local num = 0
  local step = 3

  for ind = 1, len do
    if flags[ind] ~= 0 then
      for ind1 = ind + step, len, step do
        flags[ind1] = 0
      end

      num = num + 1
    end

    step = step + 2
  end

  return num
end

local function run(len, runs)
  local flags = {}
  for _ = 1, runs do
    find_prime(flags, len)
  end
  -- print(find_prime(flags, len))
end

run(8192, 4096)
