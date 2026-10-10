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
