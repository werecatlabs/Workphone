-- Run from repository root using luavm.exe.
function class(name)
    _G[name] = {}
    return function(base) setmetatable(_G[name], {__index = base}) end
end
BaseEditor = {__init = function() end}
dofile("bin/Media/Scripts/Lua/Editor/ProceduralTextureEditor.lua")

local editor = setmetatable({}, {__index = ProceduralTextureEditor})
editor:__init(nil)
editor.setStatus = function(self, message) self.status = message end
editor.clearParamPanel = function() end
local calls = {}
editor.generateNoiseNode = function(_, node) calls[#calls+1] = node.id; return "noise" end
editor.generateNormalNode = function(_, node, source)
    calls[#calls+1] = node.id; assert(source == "noise"); return "normal"
end
editor.generateColorCorrectNode = function(_, node, source)
    calls[#calls+1] = node.id; assert(source == "normal"); return "colour"
end
local noise = editor:addNode("noise")
local normal = editor:addNode("normal")
local correction = editor:addNode("colorcorrect")
-- Force a hash layout differing from the creation order.
local nodes = editor.nodes
editor.nodes = {[correction]=nodes[correction], [noise]=nodes[noise], [normal]=nodes[normal]}
assert(editor:generate() and editor.generatedTexture == "colour")
assert(table.concat(calls, ",") == "1,2,3")
local previous = editor.generatedTexture
editor.generateNormalNode = function() return nil end
assert(not editor:generate() and editor.generatedTexture == previous)
editor.generateNormalNode = function() error("native failure") end
assert(not editor:generate() and editor.generatedTexture == previous)
local unsupported = editor:addNode("atlas")
calls = {}
assert(not editor:generate() and #calls == 0 and editor.generatedTexture == previous)
editor.nodes[unsupported] = nil

local recipe = editor:exportRecipe()
assert(recipe.version == 1 and #recipe.nodes == 3 and recipe.nodes[1].id == 1)
recipe.nodes[1].params.noiseSeed = 99
assert(editor.nodes[1].params.noiseSeed ~= 99, "Export must detach mutable settings")
assert(editor:importRecipe(recipe))
assert(editor.nodes[1].params.noiseSeed == 99 and editor.nextNodeId == 4)
assert(editor.generatedTexture == previous, "Import preserves last-good output until generation")
recipe.nodes[1].params.noiseSeed = 100
assert(editor.nodes[1].params.noiseSeed == 99, "Import must detach mutable settings")
local savedNodes = editor.nodes
recipe.nodes[2].id = 1
assert(not editor:importRecipe(recipe) and editor.nodes == savedNodes)
recipe = editor:exportRecipe()
recipe.state.outputResolution = -1
assert(not editor:importRecipe(recipe) and editor.nodes == savedNodes)
recipe = editor:exportRecipe()
recipe.nodes[1].params.noiseOctaves = 0
assert(not editor:importRecipe(recipe) and editor.nodes == savedNodes)
recipe = editor:exportRecipe()
recipe.nodes[1].params.noiseFrequency = 0/0
assert(not editor:importRecipe(recipe) and editor.nodes == savedNodes)
recipe = editor:exportRecipe()
recipe.version = 999
assert(not editor:importRecipe(recipe) and editor.nodes == savedNodes)
assert(not editor:importRecipe({}) and editor.nodes == savedNodes)
assert(not editor:loadRecipe("missing-procedural-texture-test-file.json"))
-- Production LuaManager preloads cjson; standalone luavm uses a codec fixture.
-- Exercise real filesystem persistence separately from recipe validation.
local savedRecipe
package.preload.cjson = function() return {
    encode = function(value) savedRecipe = value; return '{"version":1,"fixture":true}' end,
    decode = function(value)
        assert(value == '{"version":1,"fixture":true}', "Malformed JSON must fail")
        return savedRecipe
    end,
} end
local path = os.tmpname()
assert(editor:saveRecipe(path))
editor.nodes = {}
assert(editor:loadRecipe(path) and #editor:orderedNodeIds() == 3)
local file = assert(io.open(path, "wb")); file:write("malformed"); file:close()
local intact = editor.nodes
assert(not editor:loadRecipe(path) and editor.nodes == intact)
os.remove(path)
print("Procedural texture editor chain and recipe contracts passed")
