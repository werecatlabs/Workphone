-- Run from the repository root with luavm.exe.
function class(name)
    _G[name] = {}
    return function() end
end
BaseEditor = {__init = function() end}
IEvent = {handleValueChanged = 1, handleSelection = 2}
function ColourF(r, g, b, a) return {r = r, g = g, b = b, a = a} end
function Vector2F(x, y) return {x = x, y = y} end

local function noOp() end
local control = {}
function control:setElementId(id) self.id = id end
function control:getElementId() return self.id end
function control:setText(text) self.text = text end
function control:getText() return self.text end
function control:setValue(value) self.value = value end
function control:getValue() return self.value end
function control:setColour(colour) self.colour = colour end
function control:getColour() return self.colour end
function control:setSelectedOption(option) self.option = option end
function control:getSelectedOption() return self.option end
function control:addTabItem() return setmetatable({}, {__index = control}) end
for _, method in ipairs({"addChild", "addOption", "setLabel", "setSameLine", "setSize", "setMinValue", "setMaxValue"}) do
    control[method] = noOp
end
for _, name in ipairs({"IUIWindow", "IUITabBar", "IUICollapsingHeader", "IUIText", "IUIButton", "IUILabelTogglePair",
    "IUILabelSliderPair", "IUITextEntry", "IUIDropdown", "IUIColourPicker", "Material", "IMaterial", "FileSelection"}) do
    _G[name] = {typeInfo = function() return name end}
end
function ResourceSelect()
    return {setSameLine = noOp, setLabel = noOp, setMaterial = noOp, handleEvent = noOp}
end

local selected
local selection = {
    size = function() return selected and 1 or 0 end,
    at = function(_, index) assert(index == 0); return selected end,
}
local manager = {
    getUI = function() return {addElement = function() return setmetatable({}, {__index = control}) end} end,
    getResourceDatabase = function() return {} end,
    getSelectionManager = function() return {getSelection = function() return selection end} end,
}
IApplicationManager = {instance = function() return manager end}

dofile("bin/Media/Scripts/Lua/Editor/MaterialEditor.lua")
local material = {}
function material:derived(typeInfo) return typeInfo == IMaterial.typeInfo() end
function material:getDiffuse() return self.diffuse end
function material:setDiffuse(colour) self.diffuse = colour end
function material:getMetallicColour() return self.metallic end
function material:setMetallicColour(colour) self.metallic = colour end
function material:save() self.saved = {diffuse = self.diffuse, metallic = self.metallic} end

local defaultMetallic = ColourF(1, 1, 1, 0)
local legacyAlbedo = ColourF(0.2, 0.3, 0.4, 1)
local legacy = setmetatable({diffuse = legacyAlbedo, metallic = defaultMetallic}, {__index = material})
local working = setmetatable({diffuse = legacyAlbedo, metallic = defaultMetallic}, {__index = material})
local registered = {}
local window = {
    getParentWindow = function() return setmetatable({}, {__index = control}) end,
    setDroppable = noOp,
    setHandleEvents = function(_, sender, enabled) registered[sender] = enabled end,
    isWindowVisible = function() return true end,
}
local editor = setmetatable({refreshShaderReloadStatus = noOp}, {__index = MaterialEditor})
editor:__init(window)
selected = working
editor:load()
assert(registered[editor.albedoColour] and registered[editor.metallicColour] and registered[editor.useMetallicColourToggle])

local function expectColour(actual, expected, message)
    assert(actual and actual.r == expected.r and actual.g == expected.g and actual.b == expected.b
        and actual.a == expected.a, message)
end
local function args(values)
    return {at = function(_, index) return values[index] end}
end
local function dispatch(sender, event, values)
    editor:handleEvent(args({[1] = event, [2] = args(values or {}), [3] = sender}))
end
local function changeColour(picker, colour)
    picker:setColour(colour)
    dispatch(picker, IEvent.handleValueChanged, {[0] = colour.r, colour.g, colour.b})
end
local function toggleOverride(enabled)
    editor.useMetallicColourToggle:setValue(enabled)
    dispatch(editor.useMetallicColourToggle, IEvent.handleValueChanged, {[1] = enabled})
end

assert(editor.useMetallicColourToggle:getValue() == false)
expectColour(editor.albedoColour:getColour(), legacyAlbedo, "Albedo picker should read diffuse colour")
expectColour(editor.metallicColour:getColour(), defaultMetallic, "Legacy materials should start with an inactive override")

local albedo = ColourF(0.7, 0.2, 0.3, 1)
local metallic = ColourF(0.1, 0.8, 0.6, 1)
changeColour(editor.albedoColour, albedo)
expectColour(working:getDiffuse(), albedo, "Albedo edit should update diffuse colour")
expectColour(working:getMetallicColour(), defaultMetallic, "Albedo edit should leave the metallic override inactive")
expectColour(editor.metallicColour:getColour(), defaultMetallic, "Albedo edit should leave the metallic picker alone")

changeColour(editor.metallicColour, metallic)
assert(editor.useMetallicColourToggle:getValue() == true)
expectColour(working:getMetallicColour(), metallic, "Metallic edit should set and activate the metallic override")
expectColour(working:getDiffuse(), albedo, "Metallic edit should leave diffuse colour alone")
expectColour(editor.albedoColour:getColour(), albedo, "Metallic edit should leave the albedo picker alone")

local newAlbedo = ColourF(0.4, 0.5, 0.9, 1)
changeColour(editor.albedoColour, newAlbedo)
dispatch(editor.applyButton, IEvent.handleSelection)
dispatch(editor.saveButton, IEvent.handleSelection)
expectColour(working.saved.diffuse, newAlbedo, "Apply and Save should keep the changed albedo")
expectColour(working.saved.metallic, metallic, "Apply and Save should keep the separate metallic override")

selected = legacy
editor:updateSelection()
assert(editor.useMetallicColourToggle:getValue() == false)
expectColour(editor.metallicColour:getColour(), defaultMetallic, "Reselecting a legacy material should reset the override")
dispatch(editor.saveButton, IEvent.handleSelection)
expectColour(legacy.saved.metallic, defaultMetallic, "Saving a legacy material should leave the override inactive")

selected = setmetatable(working.saved, {__index = material})
editor:updateSelection()
assert(editor.useMetallicColourToggle:getValue() == true)
expectColour(editor.albedoColour:getColour(), newAlbedo, "Reselection should load the saved albedo")
expectColour(editor.metallicColour:getColour(), metallic, "Reselection should load the saved metallic colour")

toggleOverride(false)
dispatch(editor.saveButton, IEvent.handleSelection)
expectColour(selected.saved.metallic, ColourF(metallic.r, metallic.g, metallic.b, 0), "Disabling the override should retain its colour")
toggleOverride(true)
expectColour(selected:getMetallicColour(), metallic, "Re-enabling the override should restore its authored colour")
expectColour(selected:getDiffuse(), newAlbedo, "Toggling the override should leave albedo alone")
print("PASS: Material Editor colour independence, override toggle, Apply/Save, and reselection")
