-- Run from the repository root with luavm.exe Tests/lua/SampleVehicleAdvancedTests.lua.
-- The real Lua port is exercised against a small scene/input fixture.
function class(name)
    _G[name] = _G[name] or {}
    return function(base)
        local type = {}
        type.__index = type
        setmetatable(type, {__index = base, __call = function(_, ...)
            local instance = setmetatable({}, type)
            instance:__init(...)
            return instance
        end})
        _G[name] = type
    end
end

local vector = {}
vector.__index = vector
function Vector3F(x, y, z) return setmetatable({x=x, y=y, z=z}, vector) end
function Vector2F(x, y) return {x=x, y=y} end
function vector:dotProduct(other) return self.x*other.x + self.y*other.y + self.z*other.z end
function vector:normalise()
    local magnitude = math.sqrt(self:dotProduct(self))
    if magnitude > 0 then self.x, self.y, self.z = self.x/magnitude, self.y/magnitude, self.z/magnitude end
end
function vector.__sub(a, b) return Vector3F(a.x-b.x, a.y-b.y, a.z-b.z) end
KeyCode = {}
for i, name in ipairs({"W", "Up", "S", "Down", "A", "Left", "D", "Right", "R", "Escape"}) do KeyCode[name] = i end

dofile("bin/Media/Scripts/Lua/Game/Core/BaseComponent.lua")
dofile("bin/Media/Scripts/Lua/Game/Core/SampleVehicleAdvanced.lua")
assert(Application == nil, "Loading the sample must not replace Application")

local function fixture()
    local keys, drawn, destroyed = {}, {}, {}
    local app = {playing=false, paused=false, now=0, sceneTime=0}
    local race = {controls={}, index=0, generated=true}
    local body = {velocity=Vector3F(0, 0, 0)}
    function body:getLinearVelocity() return self.velocity end
    function body:getRigidDynamic() return nil end
    function race:setControls(...) self.controls = {...} end
    function race:reset() self.resets = (self.resets or 0) + 1 end
    function race:getWheelbase() return 2.8 end
    function race:getMaxSteeringAngle() return 0.5 end
    function race:nearestCircuitSample() return self.index end
    function race:getCircuitPosition() return Vector3F(0, 0, 0) end
    function race:getCircuitSampleCount() return 100 end
    function race:getCircuitLength() return 1000 end
    function race:isPhysicsConfigured() return true end
    function race:setSeed(seed) self.seed = seed end
    function race:setQuality(quality) self.quality = quality end
    function race:regenerate() return self.generated end
    function race:getGenerationError() return "Missing WPProcedural" end
    local car = {}
    function car:getVehicleController() return nil end
    function race:getCarController() return car end
    function race:setView() end
    local camera = {}
    function camera:setNearClipDistance() end
    function camera:setFarClipDistance() end
    function camera:setActive() end
    function camera:setTarget() end
    function camera:setDistance() end
    function camera:setHeight() end
    function camera:getFOV() return 45 end
    function camera:getNearClipDistance() return 0.5 end
    local function actor()
        local a = {position=Vector3F(0, 0.3, 0), children={}}
        function a:setName(name) self.name = name end
        function a:setSmoothMotion(value) self.smoothMotion = value end
        function a:addChild(child) table.insert(self.children, child) end
        function a:setPosition(position) self.position = position end
        function a:getPosition() return self.position end
        function a:getWorldTransform()
            return {inverseTransformPoint=function(_, position) return position - self.position end}
        end
        function a:lookAt() end
        function a:updateTransform() end
        function a:addComponent(name) return name == "ProceduralRaceScene" and race or camera end
        function a:getComponent() return body end
        return a
    end
    local owner = actor()
    local manager = {}
    function manager:play() app.playTransitions = (app.playTransitions or 0) + 1 end
    function manager:createActor() return actor() end
    function manager:destroyActor(root, cascade) assert(cascade); table.insert(destroyed, root) end
    function manager:getCurrentScene() return {registerAllUpdates=function() end} end
    function app:getGameManager() return manager end
    function app:getTimer() return {
        now=function() return app.now end,
        getTime=function() return app.now end,
        getTimeSinceLevelLoad=function() return app.sceneTime end,
        getDeltaTime=function() return 1/60 end
    } end
    function app:getInputDeviceManager() return {isKeyPressed=function(_, key) return keys[key] or false end} end
    function app:getGraphicsSystem() return {setProperties=function(_, props)
            if props.reset_render_statistics then app.statisticsReset = true end
            if props.request_render_statistics then app.statisticsRequested = true end
        end,
        getProperties=function() return {setPropertyAsBool=function(props, key, value) props[key] = value end,
            getProperty=function(_, key, default)
                if key == "render_statistics" and app.statisticsReady then return "Render statistics fixture" end
                if key == "render_counters" and app.statisticsReady then return "Render counters fixture" end
                return default
            end} end,
        getDebug=function() return {
        drawText=function(_, id, _, text) drawn[id] = text end
    } end} end
    function app:isPlaying() return self.playing end
    function app:setPlaying(value) self.playing = value end
    function app:isPaused() return self.paused end
    function app:setQuit(value) self.quit = value end
    function app:getProfiler() return nil end
    IApplicationManager = {instance=function() return app end}
    local sample = SampleVehicleAdvanced({getActor=function() return owner end})
    return sample, app, race, body, keys, destroyed, drawn, owner
end

local sample, app, race, body, keys, destroyed, drawn, owner = fixture()
sample:update()
assert(not sample.started, "Edit mode must not auto-generate")
app.playing = true
sample:update()
assert(sample.started and app.playing and race.seed == 7 and race.quality == 2)
assert(app.playTransitions == 1, "Actors generated during play must enter play state")
assert(sample.vehicleActor.position.y == 0.42 and sample.cameraActor.position.z == 8)
assert(sample.cameraActor.smoothMotion, "Follow camera must use the render smoothing path")
assert(drawn[0x56454803] and drawn[0x56454804], "Driving instructions must be displayed")

keys[KeyCode.Up], keys[KeyCode.Left] = true, true
sample:updateControls()
assert(race.controls[1] == 1 and race.controls[2] == 0 and race.controls[3] == -1)
keys[KeyCode.Right] = true
sample:updateControls()
assert(race.controls[3] == 0, "Opposing steering keys cancel")
keys[KeyCode.Right] = false
body.velocity = Vector3F(30, 0, 0)
sample:updateControls()
assert(math.abs(race.controls[3]) < 0.1, "Steering must be limited at speed")
keys[KeyCode.Down] = true
sample:updateControls()
assert(race.controls[2] == 1)
keys[KeyCode.R] = true
sample:updateControls(); sample:updateControls()
assert(race.resets == 1, "A held reset key must reset only once")
keys[KeyCode.R] = false; sample:updateControls()
keys[KeyCode.R] = true; sample:updateControls()
assert(race.resets == 2)
keys[KeyCode.Escape] = true; sample:updateControls()
assert(app.quit)
app.paused = true; sample:update()
assert(race.controls[1] == 0 and race.controls[2] == 0 and race.controls[3] == 0)
app.paused, app.quit = false, false

sample:resetRaceProgress()
sample.vehicleActor.position = Vector3F(0, 0.3, 0)
for _, index in ipairs({0, 25, 50, 75, 99, 0}) do
    race.index = index; app.now = app.now + 1; sample:updateDebugText()
end
assert(sample.lap == 2 and sample.lastLapTime == 5 and sample.bestLapTime == 5)
sample:resetRaceProgress()
race.index = 99; app.now = app.now + 1; sample:updateDebugText()
race.index = 0; app.now = app.now + 1; sample:updateDebugText()
assert(sample.lap == 1, "Crossing the line without checkpoints must not complete a lap")
sample:resetRaceProgress()
sample.vehicleActor.position = Vector3F(10, 0.3, 0)
race.index = 25; app.now = app.now + 1; sample:updateDebugText()
assert(sample.nextCheckpoint == 1, "Off-road checkpoints must not count")

sample:setSmokeTest(true); sample.smokePhase = 2; sample:updateControls()
assert(race.controls[1] == 1 and race.controls[3] > 0)
sample:setTrackSmokeTest(true)
assert(sample.trackSmokeTest and not sample.smokeTest, "Smoke modes are mutually exclusive")
sample:generate()
assert(#destroyed == 1 and sample.started, "Regeneration replaces the owned subtree")
sample:shutdown(); sample:shutdown()
assert(#destroyed == 2 and not sample.started, "Cleanup is idempotent")
assert(drawn[0x56454800] == "" and drawn[0x56454804] == "", "Cleanup clears the sample HUD")

sample, app, race, body, keys, destroyed = fixture()
sample:setTrackSmokeTest(true)
sample:generate()
app.playing, app.sceneTime = true, 1000
sample:updateControls()
assert(not app.quit, "A long edit session must not time out a newly started track test")
sample:shutdown()

sample, app, race, body = fixture()
sample:generate()
sample:setSmokeTest(true)
app.sceneTime = 10
local reports = {}
function sample:reportSmoke(message) table.insert(reports, message) end
function body:getRigidDynamic() return self end
local temporaryPosition = Vector3F(0, 0.404, 0)
function body:getTransform() return {getPosition=function() return temporaryPosition end} end
sample.smokeTime = 2.99
sample:updateSmokeTest()
assert(sample.smokePhase == 1 and reports[1]:find("PASS"), "Settled car must advance the smoke test")
temporaryPosition.x = 100
assert(sample.smokeStartPosition.x == 0, "Smoke phases must keep an owned position copy")
sample:finishSmokeTest(true, "fixture")
sample:updateSmokeTest()
assert(#reports == 2 and app.quit, "Completed smoke tests must report and quit only once")
sample:shutdown()

sample, app = fixture()
sample:generate()
sample.performanceTest = true
local benchmarkOutput = ""
local originalOpen = io.open
io.open = function(path, mode)
    assert(path == "VehicleAdvancedLuaPerformance.log" and mode == "w")
    return {
        write=function(_, ...) benchmarkOutput = benchmarkOutput .. table.concat({...}) end,
        close=function() end
    }
end
app.now = 9
sample:updatePerformanceTest()
assert(#sample.performanceFrames == 0 and not app.quit, "Benchmark must exclude warmup")
for i = 0, 3000 do
    app.now = 10 + i / 100
    sample:updatePerformanceTest()
end
assert(app.statisticsReset and app.statisticsRequested and not app.quit,
    "Benchmark must request an asynchronous snapshot and await the render thread")
app.statisticsReady = true
app.now = 40.01
sample:updatePerformanceTest()
io.open = originalOpen
assert(app.quit and not sample.performanceTest, "Benchmark must finish and exit")
assert(benchmarkOutput:find("mean=10.000 ms p95=10.000 ms updates=100.0/s", 1, true),
    "Benchmark must use elapsed wall time for application update intervals")
assert(benchmarkOutput:find("Render statistics fixture\nRender counters fixture", 1, true),
    "Benchmark must preserve timing and counter fields without sampling the snapshot wait")
sample:shutdown()

sample, app, race, body, keys, destroyed = fixture()
app.playing, race.generated = true, false
local ok, message = pcall(function() sample:generate() end)
assert(not ok and message:find("Missing WPProcedural") and app.playing)
assert(sample.startFailed and not sample.started and #destroyed == 1)
sample:update()
assert(#destroyed == 1, "Failed generation must not retry every frame")
print("SampleVehicleAdvanced Lua behavior: PASS")
