-- Shared language contract for the optional LuaJIT VM. Engine bindings are
-- verified separately by the production-host tests, not inferred from this test.
assert(jit and jit.version_num >= 20100)
local source = "local function nested() error('backend failure') end; nested()"
local chunk = assert(loadstring(source, "@Tests/Backend.lua"))
local ok, diagnostic = xpcall(chunk, debug.traceback)
assert(not ok and diagnostic:find("Tests/Backend.lua", 1, true))
assert(diagnostic:find("stack traceback", 1, true))
local task = coroutine.create(function() coroutine.yield(17); return 23 end)
local resumed, value = coroutine.resume(task)
assert(resumed and value == 17)
resumed, value = coroutine.resume(task)
assert(resumed and value == 23 and coroutine.status(task) == "dead")
local collected = false
do
    local proxy = newproxy(true)
    getmetatable(proxy).__gc = function() collected = true end
end
collectgarbage("collect")
assert(collected)
jit.off(); assert(not jit.status())
jit.on(); assert(jit.status())
print("LuaJIT backend language contracts: PASS (" .. jit.version .. ")")
