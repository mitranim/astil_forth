-- BOT-GENERATED

local uv = require("luv")

local READY = "R"
local DATA = "D"
local CLOSE = "Q"
local PORT = 19777

local server = uv.new_tcp()
assert(server:bind("127.0.0.1", PORT))

assert(server:listen(128, function(err)
  assert(not err, err)

  local client = uv.new_tcp()

  assert(server:accept(client))
  assert(client:write(READY))

  client:read_start(function(read_error, bytes)
    assert(not read_error, read_error)
    assert(bytes == DATA or bytes == CLOSE, "bad data byte")

    client:write(bytes, function(write_error)
      assert(not write_error, write_error)
      if bytes == CLOSE then
        client:read_stop()
        client:close()
      end
    end)
  end)
end))

uv.run()
