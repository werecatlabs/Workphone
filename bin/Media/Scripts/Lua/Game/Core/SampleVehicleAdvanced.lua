-- Port of Samples/cpp/VehicleAdvanced, hosted by a Script/UserComponent.
-- Requires WPProcedural, WPVehiclePhysics and the ProceduralRaceScene Lua binding.
-- Attach this class to an actor and press Generate, or enter play mode to start.
-- ProceduralRaceScene owns generated assets and runs forces/reset/suspension on
-- the physics task. This script owns input, the camera rig, lap timing and HUD.

class 'SampleVehicleAdvanced' (BaseComponent)

local SPAWN_HEIGHT, CAMERA_DISTANCE, CAMERA_HEIGHT = 0.42, 8, 3
local DEBUG_TEXT_ID = 0x56454800
local function clamp(value, minimum, maximum)
    return math.max(minimum, math.min(maximum, value))
end
local function length(vector) return math.sqrt(vector:dotProduct(vector)) end
local function flat(vector) return Vector3F(vector.x, 0, vector.z) end
local function atan2(y, x)
    if math.atan2 then return math.atan2(y, x) end -- Lua 5.1/LuaJIT.
    return math.atan(y, x) -- Lua 5.4.
end

function SampleVehicleAdvanced:__init(component)
    BaseComponent.__init(self, component)
    self.seed, self.quality = 7, 2 -- Preview=0, Standard=1, High=2, Cinematic=3.
    self.started, self.startFailed, self.resetWasDown = false, false, false
    self.smokeTest, self.trackSmokeTest = false, false
    self.performanceTest = false
    self:resetRaceProgress()
    self:resetSmokeProgress()
end

function SampleVehicleAdvanced:__finalize()
    self:shutdown()
    BaseComponent.__finalize(self)
end

function SampleVehicleAdvanced:setGenerationOptions(seed, quality)
    self.seed = math.floor(clamp(seed, 0, 4294967295))
    self.quality = math.floor(clamp(quality, 0, 3))
end

function SampleVehicleAdvanced:resetRaceProgress()
    self.lastTrackIndex, self.nextCheckpoint, self.lap = 0, 1, 1
    self.lapStart = nil
    self.lastLapTime, self.bestLapTime = 0, 0
    self.nextDebugUpdate, self.nextDebugLog = 0, 0
end

function SampleVehicleAdvanced:resetSmokeProgress()
    self.smokePassed, self.smokePhase, self.smokeTime = false, 0, 0
    self.smokeFinished = false
    self.smokeReportStarted = false
    self.trackSmokeStartedAt = nil
    self.smokeStartPosition, self.smokeStartHeading = nil, nil
    self.smokeDriveSpeed = 0
    self.smokeMinHeight, self.smokeMaxHeight, self.smokePeakVerticalSpeed = math.huge, -math.huge, 0
end

function SampleVehicleAdvanced:getProperties(parameters)
    local properties = parameters:at(0)
    properties:setPropertyAsString("Seed", tostring(self.seed))
    properties:setPropertyAsInt("Appearance Quality", self.quality)
    properties:setPropertyAsBool("Smoke Test", self.smokeTest)
    properties:setPropertyAsBool("Track Smoke Test", self.trackSmokeTest)
    properties:setPropertyAsBool("Performance Test", self.performanceTest)
    properties:setButtonPressed("Generate", false)
    properties:setButtonPressed("Reset", false)
end

function SampleVehicleAdvanced:setProperties(parameters)
    local properties = parameters:at(0)
    self:setGenerationOptions(tonumber(properties:getPropertyAsString("Seed")) or self.seed,
        properties:getPropertyAsInt("Appearance Quality", self.quality))
    self:setSmokeTest(properties:getPropertyAsBool("Smoke Test", self.smokeTest))
    self:setTrackSmokeTest(properties:getPropertyAsBool("Track Smoke Test", self.trackSmokeTest))
    self.performanceTest = properties:getPropertyAsBool("Performance Test", self.performanceTest)
    if properties:isButtonPressed("Generate") then self:generate() end
    if properties:isButtonPressed("Reset") then self:reset() end
end

function SampleVehicleAdvanced:createActor(name)
    local actor = self.gameManager:createActor()
    assert(actor, "Cannot create " .. name)
    actor:setName(name)
    self.generatedRoot:addChild(actor)
    return actor
end

function SampleVehicleAdvanced:generate()
    self:shutdown()
    self.startFailed = false
    self.application = IApplicationManager.instance()
    assert(self.application, "SampleVehicleAdvanced requires an application manager")
    self.gameManager = self.application:getGameManager()
    self.timer = self.application:getTimer()
    self.input = self.application:getInputDeviceManager()
    local owner = self:getActor()
    assert(owner and self.gameManager and self.timer,
        "SampleVehicleAdvanced requires an owner actor, game manager and timer")
    local wasPlaying = self.application:isPlaying()
    self.application:setPlaying(false)

    local ok, failure = pcall(function()
        self.generatedRoot = self.gameManager:createActor()
        assert(self.generatedRoot, "Cannot create the generated scene root")
        self.generatedRoot:setName("SampleVehicleAdvanced.Generated")
        owner:addChild(self.generatedRoot)
        self.vehicleActor = self:createActor("Procedural Grand Prix")
        self.vehicleActor:setPosition(Vector3F(0, SPAWN_HEIGHT, 0))
        self.raceScene = self.vehicleActor:addComponent("ProceduralRaceScene")
        assert(self.raceScene, "ProceduralRaceScene is unavailable; rebuild Workphone and WPLua")
        self.raceScene:setSeed(self.seed)
        self.raceScene:setQuality(self.quality)
        assert(self.raceScene:regenerate(), self.raceScene:getGenerationError())
        self.car = self.raceScene:getCarController()
        self.rigidbody = self.vehicleActor:getComponent("Rigidbody")
        assert(self.car and self.rigidbody, "Generated vehicle has no controller or rigidbody")

        self.cameraActor = self:createActor("Vehicle Follow Camera")
        self:resetCamera()
        self.camera = self.cameraActor:addComponent("Camera")
        self.camera:setNearClipDistance(0.5)
        self.camera:setFarClipDistance(1000)
        self.camera:setActive(true)
        self.cameraController = self.cameraActor:addComponent("VehicleCameraController")
        self.cameraController:setTarget(self.vehicleActor)
        self.cameraController:setDistance(CAMERA_DISTANCE)
        self.cameraController:setHeight(CAMERA_HEIGHT)
        local scene = self.gameManager:getCurrentScene()
        assert(scene, "SampleVehicleAdvanced requires a current scene")
        scene:registerAllUpdates(self.vehicleActor)
        scene:registerAllUpdates(self.cameraActor)
        self:resetRaceProgress()
        self:resetSmokeProgress()
        self.started = true
        self.performanceStart, self.performanceLast = self.timer:now(), nil
        self.performanceFrames = {}
        self:drawText(DEBUG_TEXT_ID + 3, 0.02, "W/S: throttle/brake  A/D: steer  R: reset")
        self:drawText(DEBUG_TEXT_ID + 4, 0.06, "Arrows: drive  Mouse wheel: zoom  Esc: quit")
    end)
    self.application:setPlaying(wasPlaying)
    if not ok then
        self:shutdown()
        self.startFailed = true
        error("SampleVehicleAdvanced: " .. tostring(failure))
    end
    -- Generation occurs after the Editor's initial play transition. Queue the
    -- newly created actors for play as well, including the collision plane.
    if wasPlaying then self.gameManager:play() end
    return true
end

function SampleVehicleAdvanced:shutdown()
    if self.raceScene then self.raceScene:setControls(0, 0, 0) end
    if self.application then
        for id = DEBUG_TEXT_ID, DEBUG_TEXT_ID + 4 do self:drawText(id, 0, "") end
    end
    -- Only the generated subtree belongs to this sample.
    local root = self.generatedRoot
    self.generatedRoot, self.started = nil, false
    if root and self.gameManager then self.gameManager:destroyActor(root, true) end
    self.vehicleActor, self.cameraActor, self.cameraController, self.camera = nil, nil, nil, nil
    self.raceScene, self.car, self.rigidbody = nil, nil, nil
end

function SampleVehicleAdvanced:resetCamera()
    if self.cameraActor then
        self.cameraActor:setPosition(Vector3F(0, SPAWN_HEIGHT + CAMERA_HEIGHT, CAMERA_DISTANCE))
        self.cameraActor:lookAt(Vector3F(0, SPAWN_HEIGHT, 0), Vector3F(0, 1, 0))
        self.cameraActor:updateTransform()
    end
end

function SampleVehicleAdvanced:reset()
    if self.raceScene then self.raceScene:reset() end
    self:resetRaceProgress()
    self:resetCamera()
end

function SampleVehicleAdvanced:keyDown(key)
    return self.input and self.input:isKeyPressed(key) or false
end

function SampleVehicleAdvanced:updateControls()
    local throttle = (self:keyDown(KeyCode.W) or self:keyDown(KeyCode.Up)) and 1 or 0
    local brake = (self:keyDown(KeyCode.S) or self:keyDown(KeyCode.Down)) and 1 or 0
    local left = self:keyDown(KeyCode.A) or self:keyDown(KeyCode.Left)
    local right = self:keyDown(KeyCode.D) or self:keyDown(KeyCode.Right)
    local steering = (right and 1 or 0) - (left and 1 or 0)
    local speed = length(self.rigidbody:getLinearVelocity())
    local maxSteer, wheelbase = self.raceScene:getMaxSteeringAngle(), self.raceScene:getWheelbase()
    if self.smokeTest then
        throttle = (self.smokePhase == 1 or self.smokePhase == 2) and 1 or 0
        brake = self.smokePhase == 3 and 1 or 0
        steering = self.smokePhase == 2 and 0.35 or 0
    end
    -- Preserve full lock at low speed and cap lateral acceleration as speed increases.
    steering = steering * math.min(1, wheelbase * 8 / (math.max(speed * speed, 1) * maxSteer))
    if self.trackSmokeTest and self.raceScene:isPhysicsConfigured() then
        local now = self.timer:getTime()
        self.trackSmokeStartedAt = self.trackSmokeStartedAt or now
        local position = self.vehicleActor:getPosition()
        local nearest = self.raceScene:nearestCircuitSample(position)
        local count = self.raceScene:getCircuitSampleCount()
        local lookahead = math.max(8, speed * 0.75)
        local index = (nearest + math.floor(lookahead * count / self.raceScene:getCircuitLength())) % count
        local target = self.raceScene:getCircuitPosition(index)
        local localTarget = self.vehicleActor:getWorldTransform():inverseTransformPoint(target)
        local angle = atan2(2 * wheelbase * localTarget.x,
            math.max(localTarget.x * localTarget.x + localTarget.z * localTarget.z, 1))
        steering = clamp(angle / maxSteer, -1, 1)
        local wanted = clamp(7 / (1 + math.abs(angle) * 2), 3.5, 7)
        throttle, brake = clamp((wanted - speed) * 0.25, 0, 0.4), clamp((speed - wanted) * 0.12, 0, 0.4)
        local offset = length(flat(position - self.raceScene:getCircuitPosition(nearest)))
        if position.y < 0.1 or position.y > 1 or offset > 12 or now - self.trackSmokeStartedAt > 300 then
            self:finishSmokeTest(false, "Full circuit: car left the track or timed out")
        elseif self.lap > 1 then
            self:finishSmokeTest(true, "Full circuit")
        end
    end
    self.raceScene:setControls(throttle, brake, steering)
    local resetDown = self:keyDown(KeyCode.R)
    if resetDown and not self.resetWasDown then self:reset() end
    self.resetWasDown = resetDown
    if self:keyDown(KeyCode.Escape) then self.application:setQuit(true) end
end

function SampleVehicleAdvanced:drawText(id, y, text)
    local graphics = self.application:getGraphicsSystem()
    local debug = graphics and graphics:getDebug()
    if debug then debug:drawText(id, Vector2F(0.02, y), text, 0) end
end

function SampleVehicleAdvanced:updateDebugText()
    local now = self.timer:getTime()
    if now < self.nextDebugUpdate then return end
    self.nextDebugUpdate, self.lapStart = now + 0.1, self.lapStart or now
    local position = self.vehicleActor:getPosition()
    local index, count = self.raceScene:nearestCircuitSample(position), self.raceScene:getCircuitSampleCount()
    local offset = length(flat(position - self.raceScene:getCircuitPosition(index)))
    local onRoad = offset < 6.85
    if self.lastTrackIndex > count * 3 / 4 and index < count / 4 and self.nextCheckpoint == 4 and onRoad then
        self.lastLapTime = now - self.lapStart
        if self.bestLapTime == 0 or self.lastLapTime < self.bestLapTime then self.bestLapTime = self.lastLapTime end
        self.lap, self.lapStart, self.nextCheckpoint = self.lap + 1, now, 1
    end
    if math.floor(index * 4 / count) == self.nextCheckpoint and onRoad then
        self.nextCheckpoint = self.nextCheckpoint + 1
    end
    self.lastTrackIndex = index
    local vehicle = self.car:getVehicleController()
    local drive = vehicle and vehicle:getDriveTrain()
    local properties = drive and drive:getProperties()
    local gear = properties and properties:getPropertyAsInt("Gear", 0) or 0
    local rpm = properties and properties:getPropertyAsFloat("RPM", 0) or 0
    local gearLabel = gear == 0 and "N" or gear == 1 and "R" or tostring(gear - 1)
    local lines = {
        string.format("GP | %.1f km/h | G%s | %d rpm | Lap %d",
            length(self.rigidbody:getLinearVelocity()) * 3.6, gearLabel, math.floor(rpm), self.lap),
        string.format("Lap time %.1f s | Last %.1f s | Best %.1f s", now - self.lapStart,
            self.lastLapTime, self.bestLapTime),
        string.format("Circuit %.1f m | Seed %d%s", self.raceScene:getCircuitLength(), self.seed,
            offset > 11 and " | OFF TRACK" or "")
    }
    for i, line in ipairs(lines) do self:drawText(DEBUG_TEXT_ID + i - 1, 0.14 + (i - 1) * 0.04, line) end
    if now >= self.nextDebugLog then
        print(table.concat(lines, "\n"))
        self.nextDebugLog = now + 2
    end
end

function SampleVehicleAdvanced:setSmokeTest(enabled)
    if self.smokeTest ~= enabled then self:resetSmokeProgress() end
    self.smokeTest = enabled
    if enabled then self.trackSmokeTest = false end
end
function SampleVehicleAdvanced:setTrackSmokeTest(enabled)
    if self.trackSmokeTest ~= enabled then self:resetSmokeProgress() end
    self.trackSmokeTest = enabled
    if enabled then self.smokeTest = false end
end
function SampleVehicleAdvanced:smokeTestPassed() return self.smokePassed end
function SampleVehicleAdvanced:reportSmoke(message)
    print(message)
    local file = io.open("VehicleAdvancedLuaSmoke.log", self.smokeReportStarted and "a" or "w")
    self.smokeReportStarted = true
    if file then file:write(message, "\n"); file:close() end
end
function SampleVehicleAdvanced:finishSmokeTest(passed, description)
    self.smokeFinished = true
    self.smokePassed = passed
    self.raceScene:setControls(0, 0, 0)
    self:reportSmoke("VehicleAdvanced " .. description .. (passed and ": PASS" or ": FAIL"))
    self.application:setQuit(true)
end

function SampleVehicleAdvanced:updateSmokeTest()
    if self.smokeFinished then return end
    if self.timer:getTimeSinceLevelLoad() <= 3 then return end
    local body = self.rigidbody:getRigidDynamic()
    if not body then return end
    self.smokeTime = self.smokeTime + clamp(self.timer:getDeltaTime(), 0, 1 / 30)
    local transform = body:getTransform()
    local transformPosition = transform:getPosition()
    -- Keep an owned vector across phases; the transform accessor returns a reference.
    local position = Vector3F(transformPosition.x, transformPosition.y, transformPosition.z)
    local speed = length(body:getLinearVelocity())
    local settling = self.smokePhase == 0 or self.smokePhase == 4
    local duration = settling and 3 or self.smokePhase == 2 and 2 or 4
    if not settling or self.smokeTime > duration - 1 then
        self.smokeMinHeight = math.min(self.smokeMinHeight, position.y)
        self.smokeMaxHeight = math.max(self.smokeMaxHeight, position.y)
        self.smokePeakVerticalSpeed = math.max(self.smokePeakVerticalSpeed, math.abs(body:getLinearVelocity().y))
    end
    if self.smokeTime < duration then return end
    local stable = self.smokeMinHeight > 0 and self.smokeMaxHeight - self.smokeMinHeight < 0.15
        and self.smokePeakVerticalSpeed < 0.8
    local passed = false
    if self.smokePhase == 0 or self.smokePhase == 4 then
        passed = position.y > 0 and position.y < SPAWN_HEIGHT and speed < 2
        self.smokeStartPosition = position
    elseif self.smokePhase == 1 then
        passed = length(position - self.smokeStartPosition) > 1 and speed > 0.5
        self.smokeStartHeading = flat(transform:forward())
        self.smokeStartHeading:normalise()
    elseif self.smokePhase == 2 then
        local heading = flat(transform:forward())
        heading:normalise()
        passed = length(heading - self.smokeStartHeading) > 0.02
        self.smokeDriveSpeed = speed
    elseif self.smokePhase == 3 then
        passed = speed < self.smokeDriveSpeed
        self:reset() -- Applied on the next physics step.
    end
    passed = passed and stable
    self:reportSmoke(string.format("Vehicle smoke phase %d: %s | y=%.4f speed=%.4f height range=%.4f vertical speed=%.4f",
        self.smokePhase, passed and "PASS" or "FAIL", position.y, speed,
        self.smokeMaxHeight - self.smokeMinHeight, self.smokePeakVerticalSpeed))
    if not passed or self.smokePhase == 4 then
        self:finishSmokeTest(passed, "settle/drive/turn/brake/reset")
        return
    end
    self.smokePhase, self.smokeTime = self.smokePhase + 1, 0
    self.smokeMinHeight, self.smokeMaxHeight, self.smokePeakVerticalSpeed = math.huge, -math.huge, 0
end

-- Measure application-frame intervals after generation has warmed up, without
-- including mesh/texture upload time. Keep the scene and quality fixed for comparisons.
function SampleVehicleAdvanced:updatePerformanceTest()
    local now = self.timer:now()
    if now - self.performanceStart < 10 then return end
    if self.performanceLast then
        table.insert(self.performanceFrames, (now - self.performanceLast) * 1000)
    end
    self.performanceLast = now
    if now - self.performanceStart < 40 or #self.performanceFrames == 0 then return end
    local frames, sum = self.performanceFrames, 0
    for _, interval in ipairs(frames) do sum = sum + interval end
    table.sort(frames)
    local mean = sum / #frames
    local result = string.format("seed=%d quality=%d frames=%d mean=%.3f ms p95=%.3f ms updates=%.1f/s",
        self.seed, self.quality, #frames, mean, frames[math.ceil(#frames * 0.95)], 1000 / mean)
    print(result)
    local file = io.open("VehicleAdvancedLuaPerformance.log", "w")
    if file then
        file:write(result, "\n")
        local ok, failure = pcall(function()
            local profiler = self.application:getProfiler()
            if not profiler then return end
            for _, profile in ipairs(profiler:getProfiles()) do
                file:write(string.format("%s elapsed: %.3f ms\n", profile:getLabel(),
                    profile:getAverageTimeTaken() * 1000))
            end
        end)
        if not ok then file:write("Profiler unavailable: ", tostring(failure), "\n") end
        file:close()
    end
    self.performanceTest = false
    self.application:setQuit(true)
end

function SampleVehicleAdvanced:update()
    if not self.started then
        local application = IApplicationManager.instance()
        if not self.startFailed and application and application:isPlaying() then self:generate() end
        return
    end
    if not self.application:isPlaying() or self.application:isPaused() then
        self.raceScene:setControls(0, 0, 0)
        return
    end
    self:updateControls()
    if self.performanceTest then self:updatePerformanceTest() end
    self:updateDebugText()
    if self.smokeTest then self:updateSmokeTest() end
    self.raceScene:setView(self.cameraActor:getPosition(), self.camera:getFOV() * math.pi / 180,
        self.camera:getNearClipDistance(), 1)
end
