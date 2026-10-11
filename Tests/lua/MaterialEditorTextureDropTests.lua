-- Run from the repository root with luavm.exe.
-- Strict texture doubles reject the nil/base-resource values from the reported traceback.
function class(name)
    local value = {}
    _G[name] = value
    return function() end
end
BaseEditor = {}
StringUtil = {getHashS32 = function() return 123 end}
local checker = {texture = true, name = "checker"}
local dropped = {texture = true, name = "dropped"}
local mediaAvailable = true
local loadedPath, loadedDomain
local projectPath
local resources = {}
function resources:loadMediaTexture(path)
    loadedPath, loadedDomain = path, "media"
    if not mediaAvailable then return nil end
    if path == "checker.png" then return checker end
    if path == "Runtime/shared.png" then return dropped end
end
function resources:loadTexture(path)
    loadedPath, loadedDomain = path, "project"
    projectPath = path
    if path == "Textures/albedo.png" then return dropped end
end
function resources:loadResource() error("Texture slots must use typed texture loaders") end
IApplicationManager = {instance = function()
    return {getResourceDatabase = function() return resources end}
end}
local payload
package.preload.cjson = function()
    return {decode = function(value)
        if value == "bad-json" then error("invalid JSON") end
        return payload
    end}
end

dofile("bin/Media/Scripts/Lua/Editor/ResourceSelect.lua")
dofile("bin/Media/Scripts/Lua/Editor/MaterialEditor.lua")
local image = {calls = 0}
function image:setTexture(texture)
    assert(texture and texture.texture, "IUIImage:setTexture requires a typed nonnil texture")
    self.value = texture
    self.calls = self.calls + 1
end
local material = {saves = 0, assignments = 0}
function material:getTexture() return self.value end
function material:setTexture(texture, index)
    assert(texture and texture.texture)
    self.value, self.index = texture, index
    self.assignments = self.assignments + 1
end
function material:save() self.saves = self.saves + 1 end
local slot = setmetatable({index = 12, image = image}, {__index = ResourceSelect})
slot:setMaterial(material)
assert(image.value == checker and material.assignments == 0)
assert(loadedDomain == "media", "Placeholder must resolve from bundled media")

-- Reproduce the empty material slot with unavailable media from the traceback.
mediaAvailable = false
image.calls = 0
slot:setMaterial(material)
assert(image.calls == 0, "Missing placeholder must not reach native setTexture")
assert(slot:setTexture(nil) == false and material.assignments == 0)

local status = {}
function status:setText(text) self.text = text end
local editor = setmetatable({material = material, textureSlotIds = {[12] = slot}, statusText = status},
    {__index = MaterialEditor})
local function drop(data, encoded)
    payload = data
    editor:handleDrop({at = function() return encoded or "payload" end}, 12)
end
-- A valid project texture must still assign when the checker is unavailable.
drop({filePath = "Textures/albedo.png"})
assert(material.value == dropped and material.index == 12 and material.saves == 1)
assert(image.value == dropped and status.text:find("Texture assigned:", 1, true))
assert(projectPath == "Textures/albedo.png")

local assignments = material.assignments
drop({filePath = "missing.png"})
assert(material.assignments == assignments and material.value == dropped)
assert(status.text:find("Unable to load", 1, true))
drop({}, "bad-json")
drop("not an object")
drop({filePath = 42})
assert(material.assignments == assignments, "Malformed drops must preserve the slot")

mediaAvailable = true
drop({assetPath = "Runtime/shared.png", assetDomain = "media"})
assert(loadedDomain == "media" and material.assignments == assignments + 1)
slot:setMaterial(nil)
assert(slot:setTexture(dropped) == false, "Missing material must not report success")
material.value = nil
slot:setMaterial(material)
assert(slot:setTexture(nil) == true and material.value == checker)
print("PASS: material texture drops, missing media, typed assignment, failures and clearing")
