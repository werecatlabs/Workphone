-- Attach RacingGameFull to a Script component. Press Generate to preview in edit
-- mode, then enter Play with updateInPlayMode enabled to use the menus and race.
-- Circuit challenge and time trial reuse the vehicle sample, native controllers,
-- StartMenu and the reusable Game/Racing/Racing scripts.
if not SampleVehicleAdvanced then include("SampleVehicleAdvanced.lua") end
if not MenuManager then include("MenuManager.lua") end
if not RaceManager then include("RaceManager.lua") end
if not Statistics then include("Statistics.lua") end
if not WaypointCircuit then include("WaypointCircuit.lua") end
if not ProgressTracker then include("ProgressTracker.lua") end
if not RaceUI then include("RaceUI.lua") end
if not RaceCompletedUI then include("RaceCompletedUI.lua") end
if not PlayerData then include("PlayerData.lua") end
if not TimeTrialConfig then include("TimeTrialConfig.lua") end
if not PlayerCamera then include("PlayerCamera.lua") end
if not PlayerControl then include("PlayerControl.lua") end
if not RankManager then include("RankManager.lua") end
if not RaceManagerView then include("RaceManagerView.lua") end
class 'RacingGameFull' (SampleVehicleAdvanced)

local qualities = {"Preview", "Standard", "High", "Cinematic"}
local function item(label, callback) return {label, callback} end

function RacingGameFull:__init(component)
    SampleVehicleAdvanced.__init(self, component)
    self.quality, self.totalLaps, self.mode = 1, 3, "race"
    self.gameInitialized, self.gameState, self.keys = false, "main", {}
    self.manager = RaceManager(component)
    self.statistics = Statistics(component)
    self.circuit, self.progress = WaypointCircuit(component), ProgressTracker(component)
    self.results, self.playerCamera = RaceCompletedUI(component), PlayerCamera(component)
    self.playerControl, self.timeTrial = PlayerControl(component), TimeTrialConfig(component)
    self.ranks, self.raceView, self.managerView = RankManager(component), RaceView(component), RaceManagerView(component)
end

-- The full game uses RaceUI instead of the sample's debug-text overlay.
function RacingGameFull:drawText() end

function RacingGameFull:bindGeneratedScene()
    self.generatedSeed, self.generatedQuality = self.seed, self.quality
    self.generatedRoot:setName("RacingGameFull.Generated")
    self.circuit:setProceduralScene(self.raceScene)
    self.progress:bind(self.vehicleActor, self.circuit)
    self.playerControl:bind(self.car)
    self.playerCamera:bind(self.cameraController)
    self.raceView:bind(1, self.vehicleActor, self.statistics, self.progress)
    self.raceView.localPlayer, self.raceView.name = true, "You"
    self.managerView:bind(self.manager, self.ranks)
    self.managerView:JoinRace(self.raceView)
end

-- Inspector actions run directly in edit mode, without requiring script updates.
function RacingGameFull:generate()
    self.configured = true -- Keep the seed/quality selected in the Inspector.
    SampleVehicleAdvanced.generate(self)
    self:bindGeneratedScene()
    self.playerControl:enable(false)
    self.editPreview = not self.application:isPlaying()
    local ok, failure = pcall(self.initializeGame, self)
    if not ok then self:shutdown(); self.gameStartFailed = true; error(failure) end
    return true
end

function RacingGameFull:initializeGame()
    if self.gameInitialized then return end
    self.application = assert(IApplicationManager.instance())
    self.gameManager = assert(self.application:getGameManager())
    self.timer, self.input = self.application:getTimer(), self.application:getInputDeviceManager()
    local owner = assert(self:getActor())
    local project = self.application:getProjectPath()
    if project == "" then project = "." end
    self.playerData = PlayerData(self.component)
    self.records = self.playerData:open(project .. "/RacingGameFull.records")
    if not self.configured then
        self.seed, self.quality, self.totalLaps = self.records.settings.seed, self.records.settings.quality, self.records.settings.laps
    end
    self.menu = MenuManager(self.component)
    local game = self
    -- Use real time for menu debounce even while the race simulation is paused.
    self.menu.view.getTime = function() return game.timer:now() end
    self.hud = RaceUI(self.component)
    self.hud:generate(self.gameManager, owner)
    self.hud:show(false)
    self.gameInitialized, self.lastNow = true, self.timer:now()
    self:showMainMenu()
end

function RacingGameFull:savePreferences()
    self.records.settings = {seed = self.seed, quality = self.quality, laps = self.totalLaps}
    local ok, failure = self.playerData:save()
    self.saveError = not ok and ("Could not save records: " .. tostring(failure)) or nil
end

function RacingGameFull:defer(callback)
    return function()
        if not self.pendingAction then self.pendingAction = callback end
    end
end

function RacingGameFull:showMenu(state, title, subtitle, items, status)
    self.gameState = state
    self.hud:show(false)
    -- Queue callbacks: never destroy/rebuild UI actors inside a native click callback.
    local queued = {}
    for action, spec in pairs(items) do queued[action] = item(spec[1], self:defer(spec[2])) end
    self.menu:show(title, subtitle, queued, status)
end

function RacingGameFull:pauseSimulation()
    if self.started then self.playerControl:enable(false) end
    if not self.application:isPaused() then
        self.application:setPaused(true)
        self.ownsPause = true
    end
end

function RacingGameFull:unpauseSimulation()
    if self.ownsPause then self.application:setPaused(false); self.ownsPause = false end
end

function RacingGameFull:showMainMenu()
    if self.started and self.application:isPlaying() then self:pauseSimulation() end
    self:showMenu("main", "WORKPHONE RACING", "Procedural circuits. Your car. Your best lap.", {
        Start = item("Circuit challenge", function() self.mode = "race"; self:showSetup() end),
        Workshop = item("Time trial", function() self.mode = "timeTrial"; self:showSetup() end),
        Settings = item("Settings & controls", function() self.settingsReturn = "main"; self:showSettings() end),
        Exit = item(self.application:isEditor() and "Leave play mode" or "Quit game", function() self:quitGame() end)
    })
end

function RacingGameFull:setupSummary()
    return string.format("Track seed %d  |  %s\n%s  |  Track record %s", self.seed, qualities[self.quality + 1],
        self.mode == "timeTrial" and "Unlimited timed laps" or tostring(self.totalLaps) .. " lap challenge",
        RaceSession.formatTime(self.records:getBest(self.seed)))
end

function RacingGameFull:showSetup()
    self:showMenu("setup", self.mode == "timeTrial" and "TIME TRIAL" or "CIRCUIT CHALLENGE", self:setupSummary(), {
        Resume = item("Back", function() self:showMainMenu() end),
        Start = item("Start driving", function() self:startRace() end),
        Workshop = item("Laps: " .. self.totalLaps, function() self.totalLaps = self.totalLaps % 10 + 1; self:savePreferences(); self:showSetup() end),
        Settings = item("Quality: " .. qualities[self.quality + 1], function() self:cycleQuality(); self:showSetup() end),
        Exit = item("New circuit", function() self:nextCircuit(); self:showSetup() end)
    }, self.saveError)
end

function RacingGameFull:cycleQuality()
    self.quality = (self.quality + 1) % 4
    self:savePreferences()
end

function RacingGameFull:nextCircuit()
    self.seed = (self.seed + 1) % 4294967296
    self:savePreferences()
end

function RacingGameFull:showSettings()
    self:showMenu("settings", "SETTINGS", self:setupSummary() .. "\nChanges apply to the next race.", {
        Resume = item("Back", function() self:returnFromSettings() end),
        Start = item("Quality: " .. qualities[self.quality + 1], function() self:cycleQuality(); self:showSettings() end),
        Workshop = item("New circuit seed", function() self:nextCircuit(); self:showSettings() end),
        Settings = item("Laps: " .. self.totalLaps, function() self.totalLaps = self.totalLaps % 10 + 1; self:savePreferences(); self:showSettings() end),
        Exit = item("Driving controls", function() self:showControls() end)
    }, self.saveError)
end

function RacingGameFull:returnFromSettings()
    if self.settingsReturn == "pause" then self:showPauseMenu()
    elseif self.settingsReturn == "results" then self:showResults()
    else self:showMainMenu() end
end

function RacingGameFull:showControls()
    self:showMenu("controls", "DRIVING CONTROLS",
        "W / Up: throttle   S / Down: brake\nA / Left & D / Right: steer\nMouse wheel: zoom   C: camera\nR: restart run   Esc: pause",
        {Start = item("Back to settings", function() self:showSettings() end)})
end

function RacingGameFull:startRace()
    self:unpauseSimulation()
    -- Reuse the scene for retries; regenerate only when track/quality changed.
    if not self.started or self.generatedSeed ~= self.seed or self.generatedQuality ~= self.quality then
        self.rebuilding = true
        local ok, failure = pcall(SampleVehicleAdvanced.generate, self)
        self.rebuilding = false
        if not ok then error(failure) end
        self:bindGeneratedScene()
    end
    SampleVehicleAdvanced.reset(self)
    self.progress:RestartRace()
    self.playerControl:enable(false)
    if self.mode == "timeTrial" then self.timeTrial:apply(self.manager)
    else self.manager:InitializeRace("race", self.totalLaps) end
    self.raceView:RestartRace()
    self.raceSeed, self.lastNow = self.generatedSeed, self.timer:now()
    self.readyAt, self.gameState = self.lastNow + 0.35, "loading"
    self.menu:hide()
    self.hud:show(true)
    self.hud:setText("countdown", "GET READY")
    self.nextHudUpdate = 0
end

function RacingGameFull:pauseRace()
    if not self.manager:PauseRace() then return end
    self:pauseSimulation()
    self:showPauseMenu()
end

function RacingGameFull:showPauseMenu()
    self:showMenu("pause", "PAUSED", "Race time stops while this menu is open.", {
        Resume = item("Continue", function() self:resumeRace() end),
        Start = item("Restart run", function() self:startRace() end),
        Workshop = item(self.mode == "timeTrial" and "Finish time trial" or "Main menu", function()
            if self.mode == "timeTrial" then self.manager:EndRace(); self:finishRace() else self:showMainMenu() end
        end),
        Settings = item("Settings & controls", function() self.settingsReturn = "pause"; self:showSettings() end),
        Exit = item("Main menu", function() self:showMainMenu() end)
    })
end

function RacingGameFull:resumeRace()
    self:unpauseSimulation()
    self.manager:ResumeRace()
    self.gameState, self.lastNow = self.manager.session.state, self.timer:now()
    self.menu:hide(); self.hud:show(true)
    self.playerControl:enable(self.gameState == "racing")
end

function RacingGameFull:finishRace()
    self.records:record(self.raceSeed, self.manager.session.bestLapTime)
    self:savePreferences()
    self:pauseSimulation()
    self:showResults()
end

function RacingGameFull:showResults()
    self:showMenu("results", "SESSION COMPLETE",
        self.results:summary(self.manager.session, self.raceSeed, self.records:getBest(self.raceSeed)), {
        Start = item("Race again", function() self:startRace() end),
        Workshop = item("Main menu", function() self:showMainMenu() end),
        Settings = item("Settings", function() self.settingsReturn = "results"; self:showSettings() end),
        Exit = item(self.application:isEditor() and "Leave play mode" or "Quit game", function() self:quitGame() end)
    }, self.saveError)
end

function RacingGameFull:quitGame()
    self:savePreferences()
    self:unpauseSimulation()
    if self.application:isEditor() then
        self:shutdown()
        self.gameManager:edit()
        self.application:setPlaying(false)
    else self.application:setQuit(true) end
end

function RacingGameFull:readKeys()
    local pressed = {}
    for _, name in ipairs({"Up", "Down", "Return", "Escape", "R", "C"}) do
        local code = KeyCode[name]
        local down = code ~= nil and self:keyDown(code)
        pressed[name] = down and not self.keys[name]
        self.keys[name] = down
    end
    return pressed
end

function RacingGameFull:back()
    if self.gameState == "pause" then self:resumeRace()
    elseif self.gameState == "controls" then self:showSettings()
    elseif self.gameState == "settings" then self:returnFromSettings()
    elseif self.gameState ~= "main" then self:showMainMenu() end
end

function RacingGameFull:update()
    local application = IApplicationManager.instance()
    if not application or not application:isPlaying() then
        if self.gameInitialized and not self.editPreview then self:shutdown() end
        return
    end
    if self.editPreview then
        self.editPreview = false
        self:showMainMenu()
    end
    if self.gameStartFailed then return end
    local initialized, failure = pcall(self.initializeGame, self)
    if not initialized then
        self:shutdown(); self.gameStartFailed = true
        print("RacingGameFull startup failed: " .. tostring(failure))
        return
    end
    local now = self.timer:now()
    local dt = math.max(0, now - self.lastNow)
    self.lastNow = now
    local keys = self:readKeys()
    if self.pendingAction then
        local action = self.pendingAction
        self.pendingAction = nil
        local ok, failure = pcall(action)
        self.lastNow, dt = self.timer:now(), 0
        if not ok then
            self:pauseSimulation()
            self:showMainMenu()
            self.menu.view:setStatus("Unable to start: " .. tostring(failure), true)
        end
        return
    end
    if self.menu.visible then
        if keys.Up then self.menu:move(-1) end
        if keys.Down then self.menu:move(1) end
        if keys.Return then self.menu:activate() end
        if keys.Escape then self.pendingAction = function() self:back() end end
        return
    end
    if self.gameState == "loading" then
        if now >= self.readyAt and self.raceScene:isPhysicsConfigured() then self.gameState = "countdown" end
    else
        if keys.Escape then self:pauseRace(); return end
        if keys.R then self.pendingAction = function() self:startRace() end; return end
        if keys.C then self.playerCamera:toggle() end
        if not self.application:isPaused() then
            local progress = self.progress:sample()
            local event = self.manager:OnUpdate(dt, progress)
            self.gameState = self.manager.session.state
            if event == "go" then self.raceView.m_IsStarted = true; self.playerControl:enable(true) end
            if event == "lap" or event == "finish" then
                self.records:record(self.raceSeed, self.manager.session.lastLapTime)
                self:savePreferences()
            end
            if event == "finish" then
                self.statistics:updateSession(self.manager.session, progress, RacingSupport.length(self.rigidbody:getLinearVelocity()))
                self.ranks:refresh(); self:finishRace(); return
            end
            if now >= self.nextHudUpdate then
                self.nextHudUpdate = now + 0.1
                local speed = self.rigidbody:getLinearVelocity():length()
                self.statistics:updateSession(self.manager.session, progress, speed)
                self.ranks:refresh()
                local vehicle = self.car:getVehicleController()
                local drive = vehicle and vehicle:getDriveTrain()
                local properties = drive and drive:getProperties()
                local gear = properties and properties:getPropertyAsInt("Gear", 0) or 0
                local label = gear == 0 and "N" or gear == 1 and "R" or tostring(gear - 1)
                self.hud:updateSession(self.manager.session, self.statistics, progress, speed, label,
                    self.records:getBest(self.raceSeed), self.raceSeed)
            end
        end
    end
    self.raceScene:setView(self.cameraActor:getPosition(), self.camera:getFOV() * math.pi / 180,
        self.camera:getNearClipDistance(), 1)
end

-- StartMenu's native button listeners target this Script's methods.
function RacingGameFull:handleResumeClicked() if self.menu and self.menu.visible then self.menu.view:handleResumeClicked() end end
function RacingGameFull:handleStartClicked() if self.menu and self.menu.visible then self.menu.view:handleStartClicked() end end
function RacingGameFull:handleWorkshopClicked() if self.menu and self.menu.visible then self.menu.view:handleWorkshopClicked() end end
function RacingGameFull:handleSettingsClicked() if self.menu and self.menu.visible then self.menu.view:handleSettingsClicked() end end
function RacingGameFull:handleExitClicked() if self.menu and self.menu.visible then self.menu.view:handleExitClicked() end end

function RacingGameFull:getProperties(parameters)
    local p = parameters:at(0)
    p:setPropertyAsString("Seed", tostring(self.seed))
    p:setPropertyAsInt("Appearance Quality", self.quality)
    p:setPropertyAsInt("Laps", self.totalLaps)
    p:setButtonPressed("Generate", false)
end
function RacingGameFull:setProperties(parameters)
    local p = parameters:at(0)
    self.configured = p:hasProperty("Seed") or self.configured
    self:setGenerationOptions(tonumber(p:getPropertyAsString("Seed")) or self.seed,
        p:getPropertyAsInt("Appearance Quality", self.quality))
    self.totalLaps = math.max(1, math.min(10, p:getPropertyAsInt("Laps", self.totalLaps)))
    if self.gameInitialized then self:savePreferences() end
    if p:isButtonPressed("Generate") then self:generate() end
end

function RacingGameFull:shutdown()
    SampleVehicleAdvanced.shutdown(self)
    if self.rebuilding then return end
    self:unpauseSimulation()
    if self.hud then self.hud:destroy() end
    if self.menu then self.menu:destroy(self.gameManager) end
    self.managerView:shutdown()
    self.ranks:shutdown(); self.raceView:shutdown()
    self.playerControl:shutdown(); self.playerCamera:shutdown()
    self.hud, self.menu, self.pendingAction = nil, nil, nil
    self.gameInitialized, self.gameState, self.keys = false, "main", {}
    self.editPreview = false
    self.gameStartFailed = false
end
