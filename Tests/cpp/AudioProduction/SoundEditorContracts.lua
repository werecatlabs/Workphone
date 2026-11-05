-- Headless tool logic fixture; native bindings and audible output are separate gates.
function class(name)
    return function(_) _G[name] = {} end
end
BaseEditor = {}
dofile(arg[1] .. "/bin/Media/Scripts/Lua/Editor/SoundEditor.lua")
local function manager()
    local m = {created = 0, destroyed = 0, allowPlayback = true}
    function m:createSound(path, loop)
        self.created = self.created + 1
        if path == "missing.wav" then return nil end
        local sound = {loaded = true, playing = false, owner = self, loop = loop}
        function sound:isLoaded() return self.loaded end
        function sound:play() self.playing = self.owner.allowPlayback end
        function sound:isPlaying() return self.playing end
        function sound:stop() self.playing = false end
        function sound:setVolume(value) self.volume = value end
        function sound:setLoop(value) self.loop = value end
        return sound
    end
    function m:destroySound(sound)
        assert(sound.owner == self)
        self.destroyed = self.destroyed + 1
        sound.loaded = false
    end
    function m:addSound2() error("obsolete API") end
    function m:addSound3() error("obsolete API") end
    function m:setVolume(value) self.volume = value end
    function m:setMute(value) self.mute = value end
    return m
end
local currentManager = manager()
local originalManager = currentManager
local editor = setmetatable({_settings = {eventPath = "shared.wav"}, _loadedBanks = {}}, {__index = SoundEditor})
function editor:_getSoundManager() return currentManager end
function editor:_getResourceDatabase() error("preview must not use shared catalog playback") end
function editor:_syncFromControls() end
function editor:_setStatus(status, _) self.status = status end
function editor:_setResult(results, key, value) if results then results[key] = value end end
local first = assert(editor:_loadSound("shared.wav"))
assert(editor:_loadSound("shared.wav") == first and currentManager.created == 1)
local results = {}
editor:_performAction("playEvent", results)
assert(editor._isPlaying and first.playing and results.playing == true)
-- Selection retires the previous preview, including when the project manager changes.
currentManager = manager()
editor._settings.eventPath = "next.wav"
editor:_applyLiveSetting("eventPath", "next.wav", results)
assert(originalManager.destroyed == 1 and not first.loaded and not first.playing)
assert(editor._currentSound == nil)
currentManager.allowPlayback = false
editor:_performAction("playEvent", results)
assert(not editor._isPlaying and results.playing == false)
assert(editor.status:find("did not start"))
editor:_performAction("resumeEvent", results)
assert(not editor._isPlaying and editor.status:find("did not resume"))
editor:_performAction("loadBank", results)
assert(results.success == false and results.bankLoaded == false and next(editor._loadedBanks) == nil)
assert(editor.status:find("unavailable"))
editor._settings.bus = 1
editor._settings.busVolume = 0.2
editor._settings.busMute = true
editor:_applyLiveSetting("busMute", true, results)
assert(currentManager.mute == nil and currentManager.volume == nil)
assert(results.success == false and editor.status:find("unavailable"))
editor._settings.bus = 0
assert(editor:_applyManagerSettings())
assert(currentManager.mute == true and currentManager.volume == 0.2)
editor:unload()
assert(currentManager.destroyed == 1 and editor._currentSound == nil)
editor:unload()
assert(currentManager.destroyed == 1)
assert(editor:_loadSound("missing.wav") == nil)
editor:__finalize()
assert(currentManager.destroyed == 1)
print("SoundEditor preview ownership, failure status and unsupported-bank contracts passed")

-- Run the real window construction and passive restore paths against an already
-- configured audio manager. UI setters emit callbacks to exercise sync guards.
local startupManager = {volume = 0.37, mute = true, volumeWrites = 0, muteWrites = 0}
function startupManager:getVolume() return self.volume end
function startupManager:isMute() return self.mute end
function startupManager:setVolume(value) self.volume = value; self.volumeWrites = self.volumeWrites + 1 end
function startupManager:setMute(value) self.mute = value; self.muteWrites = self.muteWrites + 1 end
local loadedEditor
local function notify(control)
    if loadedEditor then loadedEditor:_handleControlChanged(control, {}) end
end
local function element()
    local control = {children = {}, options = {}}
    function control:addChild(child) self.children[#self.children + 1] = child; child.parent = self end
    function control:removeChild(child) child.parent = nil end
    function control:getParent() return self.parent end
    function control:destroyAllChildren() self.children = {} end
    function control:setLabel(value) self.label = value end
    function control:setSize(value) self.size = value end
    function control:setSameLine(value) self.sameLine = value end
    function control:setMinValue(value) self.minimum = value end
    function control:setMaxValue(value) self.maximum = value end
    function control:setVisible(value) self.visible = value end
    function control:setValue(value) self.value = value; notify(self) end
    function control:getValue() return self.value end
    function control:setText(value) self.text = value; notify(self) end
    function control:getText() return self.text or "" end
    function control:addOption(value) self.options[#self.options + 1] = value end
    function control:setSelectedOption(value) self.selected = value; notify(self) end
    function control:getSelectedOption() return self.selected or 0 end
    function control:addTabItem() local tab = element(); self:addChild(tab); return tab end
    return control
end
for _, name in ipairs({"IUIWindow", "IUIDropdown", "IUIButton", "IUIImage", "IUIText", "IUICheckbox",
    "IUITabBar", "IUICollapsingHeader", "IUILabelSliderPair", "IUITextEntry", "IUILabelTogglePair"}) do
    _G[name] = {typeInfo = function() return 1 end}
end
Vector2F = function(x, y) return {x = x, y = y} end
local ui = {addElement = function() return element() end, removeElement = function() end}
local application = {getUI = function() return ui end, getSoundManager = function() return startupManager end}
IApplicationManager = {instance = function() return application end}
local parent = element()
local window = {getParentWindow = function() return parent end, getDebugWindow = function() end,
    setHandleEvents = function() end}
loadedEditor = setmetatable({}, {__index = SoundEditor})
loadedEditor:__init(window)
loadedEditor:load()
assert(startupManager.volume == 0.37 and startupManager.mute == true)
assert(startupManager.volumeWrites == 0 and startupManager.muteWrites == 0)
assert(loadedEditor.busVolumeSlider:getValue() == 0.37 and loadedEditor.busMuteCheckbox:getValue() == true)

local stale = {values = {busVolume = 1.0, busMute = false}}
function stale:hasProperty(key) return self.values[key] ~= nil end
function stale:getPropertyAsFloat(key) return self.values[key] end
function stale:getPropertyAsBool(key) return self.values[key] end
loadedEditor:setProperties({at = function() return stale end})
assert(startupManager.volume == 0.37 and startupManager.mute == true)
assert(startupManager.volumeWrites == 0 and startupManager.muteWrites == 0)
assert(loadedEditor.busVolumeSlider:getValue() == 0.37 and loadedEditor.busMuteCheckbox:getValue() == true)

-- Explicit edits still reach the existing manager API, one setting at a time.
loadedEditor.busVolumeSlider:setValue(0.2)
assert(startupManager.volume == 0.2 and startupManager.mute == true)
assert(startupManager.volumeWrites == 1 and startupManager.muteWrites == 0)
startupManager.volume = 0.58 -- A different tool changed the project master gain.
loadedEditor.busMuteCheckbox:setValue(false)
assert(startupManager.volume == 0.58 and startupManager.mute == false)
assert(startupManager.volumeWrites == 1 and startupManager.muteWrites == 1)
loadedEditor.volumeSlider:setValue(0.65)
assert(startupManager.volumeWrites == 1 and startupManager.muteWrites == 1)
startupManager.volume = 0.44
loadedEditor:show()
assert(loadedEditor.busVolumeSlider:getValue() == 0.44)
assert(startupManager.volumeWrites == 1 and startupManager.muteWrites == 1)
local snapshot = {values = {}}
function snapshot:setPropertyAsBool(key, value) self.values[key] = value end
snapshot.setPropertyAsFloat = snapshot.setPropertyAsBool
snapshot.setPropertyAsString = snapshot.setPropertyAsBool
loadedEditor:getProperties({at = function() return snapshot end})
assert(snapshot.values.busVolume == 0.44 and snapshot.values.busMute == false)
assert(startupManager.volumeWrites == 1 and startupManager.muteWrites == 1)
loadedEditor:unload()
print("SoundEditor startup/restore preserves project master gain and mute; explicit mixer edits remain live")
