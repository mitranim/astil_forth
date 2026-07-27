-- BOT-GENERATED

local chunk_size = 64 * 1024

local function copy(input)
  while true do
    local chunk, err = input:read(chunk_size)
    if chunk == nil then
      if err ~= nil then error(err, 0) end
      return
    end
    assert(io.stdout:write(chunk))
  end
end

if #arg == 0 then
  copy(io.stdin)
else
  for index = 1, #arg do
    local name = arg[index]
    if name == "-" then
      copy(io.stdin)
    else
      local input = assert(io.open(name, "rb"))
      copy(input)
      assert(input:close())
    end
  end
end

assert(io.stdout:flush())
