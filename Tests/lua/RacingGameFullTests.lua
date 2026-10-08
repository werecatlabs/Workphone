-- Run from the repository root with bin/windows/v145/x64/MD/RelWithDebInfo/luavm.exe.
local fixtureLibrary = dofile("Tests/lua/SampleVehicleAdvancedTests.lua")
local core = "bin/Media/Scripts/Lua/Game/Core/"
local racing = "bin/Media/Scripts/Lua/Game/Racing/Racing/"
function include(name)
    for _, folder in ipairs({core, "bin/Media/Scripts/Lua/UI/", racing, "bin/Media/Scripts/Lua/Game/Racing/", racing .. "Misc/", racing .. "Race/System/",
        racing .. "Race/UI/", racing .. "Race/Others/", racing .. "Race/Helpers/", racing .. "Vehicle/", racing .. "Vehicle/Input/"}) do
        local file = io.open(folder .. name, "r")
        if file then file:close(); return dofile(folder .. name) end
    end
    error("Missing include: " .. name)
end
function ColourF(...) return {...} end
IEvent = {ACTIVATE_HASH = 1, CLICK_HASH = 2}
State = {Edit=2, Play=3}
KeyCode.Return, KeyCode.C = 100, 101
local vec = getmetatable(Vector3F(0, 0, 0))
function vec.__add(a, b) return Vector3F(a.x+b.x, a.y+b.y, a.z+b.z) end
function vec.__mul(a, b) return Vector3F(a.x*b, a.y*b, a.z*b) end
include("RacingGameFull.lua")

local session = RaceSession.new()
session:begin("race", 1)
assert(session:update(2, 0, 100, true) == nil and session.state == "countdown")
session:pause(); session:update(20, 0, 100, true)
assert(session.countdown == 1 and session.elapsed == 0)
session:resume(); assert(session:update(1, 0, 100, true) == "go")
session:update(1, 99, 100, true); session:update(1, 0, 100, true)
assert(session.completedLaps == 0, "Shortcut must not award a lap")
for _, index in ipairs({10, 20, 25, 35, 45, 50, 60, 70, 75, 85, 95, 99, 0}) do
    session:update(1, index, 100, true)
end
assert(session.state == "finished" and session.completedLaps == 1)
local finishedTime = session.elapsed
session:update(100, 0, 100, true)
assert(session.elapsed == finishedTime)
session:begin("timeTrial", 1); session:update(3, 0, 100, true)
for lap = 1, 2 do
    for _, index in ipairs({10, 20, 25, 35, 45, 50, 60, 70, 75, 85, 95, 99, 0}) do session:update(1, index, 100, true) end
end
assert(session.state == "racing" and session.completedLaps == 2)
local oldBest = session.bestLapTime
session:pause(); session:update(1000, 0, 100, true); session:resume()
assert(session.bestLapTime == oldBest and session.elapsed == 26)
session:begin("race", 1); session:update(3, 0, 100, true)
for _, index in ipairs({99, 90, 80, 75, 65, 55, 50, 40, 30, 25, 15, 5, 0}) do session:update(1, index, 100, true) end
assert(session.completedLaps == 0, "Reverse travel cannot complete a lap")

local temporary = os.tmpname()
local records = RacingRecords.new(temporary)
records.settings = {seed=42, quality=2, laps=5}
assert(records:record(42, 90) and not records:record(42, 100) and records:record(42, 85))
assert(records:save())
local loaded = RacingRecords.new(temporary); assert(loaded:load())
assert(loaded.settings.seed == 42 and loaded.settings.laps == 5 and loaded:getBest(42) == 85)
local f = assert(io.open(temporary, "w")); f:write("seed=99999999999999\nquality=NaN\nlaps=0\nbest:7=99999999\nos.execute=evil\n"); f:close()
loaded = RacingRecords.new(temporary); loaded:load()
assert(loaded.settings.seed == 7 and loaded.settings.quality == 1 and loaded.settings.laps == 3 and loaded:getBest(7) == 0)
os.remove(temporary)

local sample, app, race, body, keys, destroyed, drawn, owner = fixtureLibrary.fixture()
local manager = app:getGameManager()
local function array(items) return {size=function() return #items end, at=function(_, i) return items[i+1] end} end
local function uiComponent()
    local value = {}
    for _, method in ipairs({"setPosition","setSize","setAnchor","setAnchorMin","setAnchorMax","setZOrder","setColour","setMaterialPath","setText",
        "setTextSize","setTextStr","setHorizontalAlignment","setVerticalAlignment","setNormalColour",
        "setHighlightedColour","setPressedColour","setDisabledColour","setEnabled"}) do
        value[method] = function(self, item) self[method .. "Value"] = item end
    end
    value.setZOrder = function(self, z, cascade)
        assert(type(cascade) == "boolean", "native setZOrder requires cascade")
        self.setZOrderValue = z
    end
    value.getProperties = function(self)
        return {setPropertyAsInt=function(_, name, item) self[name] = item end,
            setPropertyAsBool=function(_, name, item) self[name] = item end}
    end
    value.setProperties = function() end
    value.setState = function(self, state) self.state=state end
    value.getEvents = function() error("Native event arrays are not Lua-bound") end
    value.setClickHandler = function(self, component, functionName)
        self.listener = {component=component, fn=functionName, hash=IEvent.CLICK_HASH}
    end
    return value
end
local nextActorId = 0
local function enhance(actor)
    nextActorId = nextActorId + 1
    actor.instanceId = nextActorId
    function actor:getHandle() return {getInstanceId=function() return self.instanceId end} end
    actor.enabled = true
    function actor:getName() return self.name end
    function actor:getChildren() return array(self.children) end
    function actor:getNumChildren() return #self.children end
    function actor:getChildByIndex(index) return self.children[index + 1] end
    function actor:setEnabled(value, cascade)
        assert(type(cascade) == "boolean", "native actor setEnabled requires cascade")
        self.enabled = value
    end
    function actor:isEnabled() return self.enabled end
    local oldAddChild = actor.addChild
    function actor:addChild(child) child.parent = self; oldAddChild(self, child) end
    local oldTransform = actor.getWorldTransform
    function actor:getWorldTransform()
        local transform = oldTransform(self)
        transform.forward = function() return Vector3F(0, 0, -1) end
        return transform
    end
    local oldAdd = actor.addComponent
    function actor:addComponent(name)
        if name == "Layout" or name == "LayoutTransform" or name == "Image" or name == "Material" or name == "Text" or name == "Button" then
            local component = uiComponent()
            if name == "Text" then component.setTextSize = nil end -- Only IUIText exposes this.
            self.uiComponents = self.uiComponents or {}
            self.uiComponents[name] = component
            return component
        end
        return oldAdd(self, name)
    end
    return actor
end
enhance(owner)
local oldCreate, oldDestroy = manager.createActor, manager.destroyActor
function manager:createActor() return enhance(oldCreate(self)) end
function manager:destroyActor(actor, cascade)
    oldDestroy(self, actor, cascade)
    if actor.parent then
        for i, child in ipairs(actor.parent.children) do if child == actor then table.remove(actor.parent.children, i); break end end
    end
end
function manager:edit() app.edited = true end
function app:getProjectPath() return "." end
function app:isEditor() return true end
function app:setEditorCamera(value) self.editorCamera = value end
function app:getCameraManager() return {reset=function() self.cameraResets = (self.cameraResets or 0) + 1 end} end
function app:setPaused(value) self.paused = value end
local car = race:getCarController()
function car:setControls(...) race:setControls(...) end
function car:usePlayerControls() race:usePlayerControls() end
-- A real circuit tangent faces -Z; keep the fixture car aligned with it.
function race:getCircuitPosition(index) return Vector3F(0, 0, -index) end

local store = os.tmpname()
os.remove(store)
local realOpen = io.open
local realRename, realRemove = os.rename, os.remove
local prefix = "./RacingGameFull.records"
local function redirect(path) return path:sub(1,#prefix)==prefix and store..path:sub(#prefix+1) or path end
io.open = function(path, mode) return realOpen(redirect(path), mode) end
os.rename = function(from, to) return realRename(redirect(from),redirect(to)) end
os.remove = function(path) return realRemove(redirect(path)) end
local game = RacingGameFull({getActor=function() return owner end})
local inspector = {values={Seed="19", ["Appearance Quality"]=0, Laps=2}, buttons={}}
function inspector:setPropertyAsString(name, value) self.values[name] = value end
function inspector:setPropertyAsInt(name, value) self.values[name] = value end
function inspector:getPropertyAsString(name) return self.values[name] end
function inspector:getPropertyAsInt(name, default) return self.values[name] or default end
function inspector:hasProperty(name) return self.values[name] ~= nil end
function inspector:setButtonPressed(name, value) self.buttons[name] = value end
function inspector:isButtonPressed(name) return self.buttons[name] == true end
app.playing = false
game:getProperties(array({inspector}))
assert(inspector.buttons.Generate == false, "Inspector must expose Generate")
inspector.values.Seed, inspector.values["Appearance Quality"], inspector.values.Laps = "19", 0, 2
inspector.buttons.Generate = true
game:setProperties(array({inspector}))
assert(game.started and game.editPreview and not game.menu.visible and not app.playing and not app.paused)
assert(game.generatedSeed == 19 and game.generatedQuality == 0 and game.totalLaps == 2)
local previewRoot, previewChildren = game.generatedRoot, #owner.children
game:update()
assert(game.generatedRoot == previewRoot, "Edit updates must retain generated preview")
game:setProperties(array({inspector}))
assert(game.generatedRoot ~= previewRoot and #owner.children == previewChildren, "Regenerate must replace owned actors")
previewRoot = game.generatedRoot
app.playing = true; game:update()
assert(not game.editPreview and app.paused and game.menu.visible)
game:startRace()
assert(game.generatedRoot == previewRoot and game.gameState == "loading", "Play must reuse edit generation")
game:shutdown(); app.playing = false
game.seed, game.quality, game.totalLaps = 7, 1, 3
inspector.buttons.Generate = false
app.playing = true
local restoredRoots = {}
for _, name in ipairs({"RacingGameFull.Generated", "Racing.HUD", "__StartMenuGenerated"}) do
    local actor = manager:createActor()
    actor:setName(name); owner:addChild(actor)
    restoredRoots[#restoredRoots + 1] = actor
end
game:update()
for _, actor in ipairs(restoredRoots) do
    for _, child in ipairs(owner.children) do assert(child ~= actor, "Restored generated roots must be replaced") end
end
assert(game.gameState == "main" and game.menu.visible and not game.started)
assert(app.editorCamera == true, "The initial menu needs a renderable Editor viewport")
assert(#game.menu.view.generationWarnings == 0)
assert(game.menu.view.generatedRoot.uiComponents.Layout.flagNoInput == false,
    "Menu needs an interactive native canvas")
assert(game.hud.root.uiComponents.Layout.flagNoInput == true,
    "HUD must let native driving input through")
local function checkLayout(actor, parentWidth, parentHeight)
    local ui = actor.uiComponents
    if not ui or not ui.LayoutTransform then return end
    local layout = ui.LayoutTransform
    assert(layout.setAnchorValue.x == 0.5 and layout.setAnchorValue.y == 0.5,
        "Generated UI must explicitly center its pivot")
    local size, position = layout.setSizeValue, layout.setPositionValue
    if parentWidth then
        assert(math.abs(position.x) + size.x / 2 <= parentWidth / 2 + 0.001, actor.name .. " overflows horizontally")
        assert(math.abs(position.y) + size.y / 2 <= parentHeight / 2 + 0.001, actor.name .. " overflows vertically")
    end
    for _, child in ipairs(actor.children) do checkLayout(child, size.x, size.y) end
end
checkLayout(game.menu.view.generatedRoot, 1920, 1080)
checkLayout(game.hud.root, 1920, 1080)
local button = game.menu.view.buttons.Start
assert(button.listener.fn == "handleStartClicked", "Real menu listener must target full-game handler")
game:handleStartClicked()
assert(game.gameState == "main" and game.pendingAction, "UI callback must queue changes")
app.now = 1; game:update()
assert(game.gameState == "setup")
game.totalLaps = 1
game:startRace()
local root = game.generatedRoot
assert(app.editorCamera == false, "Race startup must select the generated game camera")
assert(game.started and game.gameState == "loading" and race.controls[2] == 1)
app.now = 2; game:update()
assert(game.gameState == "countdown")
app.now = 5; game:update()
assert(game.gameState == "racing" and race.playerControls)
app.now = 6; game:update()
local elapsed = game.manager.session.elapsed
keys[KeyCode.Escape] = true; app.now = 7; game:update()
assert(app.paused and game.gameState == "pause" and not app.quit)
keys[KeyCode.Escape] = false; app.now = 100; game:update()
assert(game.manager.session.elapsed == elapsed, "Pause must stop timing")
game:resumeRace(); assert(not app.paused and game.gameState == "racing")
app.now = 101; game:update()
assert(game.manager.session.elapsed == elapsed + 1)
for _, index in ipairs({10,20,25,35,45,50,60,70,75,85,95,99,0}) do
    race.index = index; game.vehicleActor.position = Vector3F(0, 0.42, -index)
    app.now = app.now + 1; game:update()
end
assert(game.gameState == "results" and app.paused and game.records:getBest(7) > 0)
game.manager.session.lapTimes = {12,13,14,15,16,17,18,19,20,21}
game:showResults()
checkLayout(game.menu.view.generatedRoot, 1920, 1080)
game:startRace()
assert(game.generatedRoot == root, "Retry should reuse expensive generated scene")
assert(game.hud.dashboard.setTextValue == "Preparing vehicle and circuit...", "Retry must clear previous telemetry")
local oldConfigured = race.isPhysicsConfigured
race.isPhysicsConfigured = function() return false end
app.now = app.now + 16; game:update()
assert(game.gameState == "main" and game.menu.visible and app.paused, "Loading timeout must return to a usable menu")
race.isPhysicsConfigured = oldConfigured
game:startRace()
root = game.generatedRoot
keys[KeyCode.Escape] = true; game:update()
assert(game.gameState == "main" and game.menu.visible, "Loading must be cancellable")
keys[KeyCode.Escape] = false
game:startRace()
game:pauseRace(); game:showMainMenu()
assert(game.gameState == "main" and app.paused)
game.mode = "timeTrial"; game:startRace()
assert(game.generatedRoot == root and game.manager.session.mode == "timeTrial")
game:nextCircuit(); game:startRace()
assert(game.generatedRoot ~= root and game.generatedSeed == 8, "New track must regenerate")
game:quitGame()
assert(not app.playing and not app.paused and app.edited and not app.quit)
assert(not game.gameInitialized and game.hud == nil and game.menu == nil)
local sampleClass = SampleVehicleAdvanced
include("RacingGameApplication.lua")
assert(SampleVehicleAdvanced == sampleClass, "Application entry point must not redefine the sample")
io.open = realOpen
os.rename, os.remove = realRename, realRemove
os.remove(store)
os.remove(store..".tmp"); os.remove(store..".bak")
print("RacingGameFull session, menu, controls, restart, results and persistence: PASS")
