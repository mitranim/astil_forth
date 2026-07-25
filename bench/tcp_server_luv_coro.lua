-- BOT-GENERATED

local uv = require("luv")

local READY = "R"
local DATA = "D"
local CLOSE = "Q"
local PORT = 19777

local server = uv.new_tcp()
assert(server:bind("127.0.0.1", PORT))

local function resume(thread, ...)
  assert(coroutine.resume(thread, ...))
end

local function write(client, bytes)
  local thread = coroutine.running()
  client:write(bytes, function(err) resume(thread, err) end)
  local err = coroutine.yield()
  assert(not err, err)
end

local function read(client)
  local thread = coroutine.running()
  client:read_start(function(err, bytes)
    client:read_stop()
    resume(thread, err, bytes)
  end)
  local err, bytes = coroutine.yield()
  assert(not err, err)
  return bytes
end

local function handle(client)
  write(client, READY)
  while true do
    local bytes = read(client)
    assert(bytes == DATA or bytes == CLOSE, "bad data byte")
    write(client, bytes)
    if bytes == CLOSE then
      client:close()
      return
    end
  end
end

assert(server:listen(128, function(err)
  assert(not err, err)
  local client = uv.new_tcp()
  assert(server:accept(client))
  resume(coroutine.create(handle), client)
end))

uv.run()
