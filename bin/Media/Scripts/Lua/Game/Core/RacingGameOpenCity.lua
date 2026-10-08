-- Attach RacingGameOpenCity to a Script component; Generate previews the city.
-- Requires a build with the ProceduralRaceScene Open City properties and WPProcedural.
if not RacingGameFull then include("RacingGameFull.lua") end
class 'RacingGameOpenCity' (RacingGameFull)

local qualities = {"Preview", "Standard", "High", "Cinematic"}
local routes = {"Downtown loop", "Crosstown circuit", "City perimeter"}
local function item(label, callback) return {label, callback} end

function RacingGameOpenCity:__init(component)
    RacingGameFull.__init(self, component)
    self.generatedRootName = "RacingGameOpenCity.Generated"
    self.recordsFile = "RacingGameOpenCity.records"
    self.cityBlocks, self.cityRoute, self.mode = 8, 0, "freeDrive"
    -- Free driving still uses the shared loading/countdown/pause lifecycle, but
    -- never feeds movement into lap timing or awards records for exploration.
    local sample = self.progress.sample
    self.progress.sample = function(progress)
        if self.mode == "freeDrive" then
            return {index=0, count=self.circuit.numPoints, onRoad=false,
                offTrack=false, wrongWay=false, offset=0}
        end
        return sample(progress)
    end
end

function RacingGameOpenCity:configureRaceScene(scene)
    local p = scene:getProperties()
    assert(p:hasProperty("Open City"), "Rebuild Workphone: ProceduralRaceScene has no Open City support")
    p:setPropertyAsBool("Open City", true)
    p:setPropertyAsInt("City Blocks", self.cityBlocks)
    p:setPropertyAsInt("City Route", self.cityRoute)
    scene:setProperties(p)
end

function RacingGameOpenCity:bindGeneratedScene()
    RacingGameFull.bindGeneratedScene(self)
    self.generatedBlocks, self.generatedRoute = self.cityBlocks, self.cityRoute
    self.camera:setFarClipDistance(1400)
end

function RacingGameOpenCity:initializeGame()
    if self.gameInitialized then return end
    RacingGameFull.initializeGame(self)
    self.preferenceRecords = self.records
    local updateSession = self.hud.updateSession
    self.hud.updateSession = function(hud, session, statistics, progress, speed, gear, record)
        if self.mode ~= "freeDrive" then
            updateSession(hud, session, statistics, progress, speed, gear, record, self.generatedSeed)
            local target = (progress.index + 20) % progress.count
            local p = self.circuit:position(target)
            hud:setText("help", string.format("%s | Follow cyan arrows | Next marker X %.0f Z %.0f | R: restart  C: camera  Esc: pause",
                routes[self.generatedRoute + 1], p.x, p.z))
            return
        end
        local p = self.vehicleActor:getPosition()
        hud:setText("dashboard", string.format("OPEN CITY   |   %.0f km/h   |   Gear %s\nFree driving   |   City seed %d   |   Position X %.0f Z %.0f\nExplore the streets. Choose a city race from the pause menu.",
            speed * 3.6, gear, self.generatedSeed, p.x, p.z))
        hud:setText("countdown", session.state == "countdown" and tostring(math.ceil(session.countdown)) or "")
        hud:setText("help", "W/S or arrows: throttle/brake   A/D: steer   R: return to start   C: camera   Esc: pause")
    end
end

function RacingGameOpenCity:savePreferences()
    local preferences = self.preferenceRecords or self.records
    preferences.settings = {seed=self.seed, quality=self.quality, laps=self.totalLaps}
    local ok, failure = preferences:save()
    if ok and self.records ~= preferences then ok, failure = self.records:save() end
    self.saveError = not ok and ("Could not save city records: " .. tostring(failure)) or nil
end

function RacingGameOpenCity:startRace()
    if self.generatedBlocks ~= self.cityBlocks or self.generatedRoute ~= self.cityRoute then
        self.generatedQuality = nil
    end
    RacingGameFull.startRace(self)
    -- Separate files avoid collisions between any seed/size/route combinations.
    local project = self.application:getProjectPath()
    if project == "" then project = "." end
    local path = string.format("%s/RacingGameOpenCity.%d.%d.records", project, self.generatedBlocks, self.generatedRoute)
    if self.routeRecordsPath ~= path then
        self.records = RacingRecords.new(path)
        self.records:load()
        self.routeRecordsPath = path
    end
    self.raceSeed = self.generatedSeed
    self.hud:setText("dashboard", "Preparing city and vehicle...")
end

function RacingGameOpenCity:showMainMenu()
    if not self.started and self.application:isPlaying() then self.playerCamera:selectViewport(true) end
    if self.started and self.application:isPlaying() then self:pauseSimulation() end
    self:showMenu("main", "OPEN CITY RACING", "Explore a seeded city. Race its streets. Beat your best lap.", {
        Start = item("Free drive", function() self.mode = "freeDrive"; self:startRace() end),
        Workshop = item("City races", function() self.mode = "race"; self:showSetup() end),
        Settings = item("City settings & controls", function() self.settingsReturn = "main"; self:showSettings() end),
        Exit = item(self.application:isEditor() and "Leave play mode" or "Quit game", function() self:quitGame() end)
    })
end

function RacingGameOpenCity:setupSummary()
    return string.format("City seed %d | %d x %d blocks | %s\n%s | %d laps | Follow the cyan street arrows",
        self.seed, self.cityBlocks, self.cityBlocks, routes[self.cityRoute + 1],
        self.mode == "timeTrial" and "Time trial" or "Street circuit", self.totalLaps)
end

function RacingGameOpenCity:showSetup()
    self:showMenu("setup", "CITY RACES", self:setupSummary(), {
        Resume = item("Back", function() self:showMainMenu() end),
        Start = item("Start driving", function() self:startRace() end),
        Workshop = item(self.mode == "timeTrial" and "Mode: time trial" or "Mode: lap challenge", function()
            self.mode = self.mode == "timeTrial" and "race" or "timeTrial"; self:showSetup()
        end),
        Settings = item("Route: " .. routes[self.cityRoute + 1], function()
            self.cityRoute = (self.cityRoute + 1) % 3; self:showSetup()
        end),
        Exit = item("Laps: " .. self.totalLaps, function()
            self.totalLaps = self.totalLaps % 10 + 1; self:savePreferences(); self:showSetup()
        end)
    }, self.saveError)
end

function RacingGameOpenCity:showSettings()
    self:showMenu("settings", "CITY SETTINGS", self:setupSummary() .. "\nCity changes apply to the next drive.", {
        Resume = item("Back", function() self:returnFromSettings() end),
        Start = item("Quality: " .. qualities[self.quality + 1], function() self:cycleQuality(); self:showSettings() end),
        Workshop = item("City size: " .. self.cityBlocks .. " blocks", function()
            self.cityBlocks = self.cityBlocks == 10 and 6 or self.cityBlocks + 2; self:showSettings()
        end),
        Settings = item("New city seed", function() self:nextCircuit(); self:showSettings() end),
        Exit = item("Driving controls", function() self:showControls() end)
    }, self.saveError)
end

function RacingGameOpenCity:showPauseMenu()
    local free = self.mode == "freeDrive"
    self:showMenu("pause", "PAUSED", "Driving and race time stop while this menu is open.", {
        Resume = item("Continue driving", function() self:resumeRace() end),
        Start = item(free and "Return to city start" or "Restart route", function() self:startRace() end),
        Workshop = item(free and "City races" or self.mode == "timeTrial" and "Finish time trial" or "Free drive", function()
            if self.mode == "timeTrial" then self.manager:EndRace(); self:finishRace()
            elseif free then self.mode = "race"; self:showSetup()
            else self.mode = "freeDrive"; self:startRace() end
        end),
        Settings = item("Settings & controls", function() self.settingsReturn = "pause"; self:showSettings() end),
        Exit = item("Main menu", function() self:showMainMenu() end)
    })
end

function RacingGameOpenCity:showResults()
    self:showMenu("results", "CITY ROUTE COMPLETE", routes[self.generatedRoute + 1] .. "\n" ..
        self.results:summary(self.manager.session, self.generatedSeed, self.records:getBest(self.raceSeed)), {
        Start = item("Race again", function() self:startRace() end),
        Workshop = item("Free drive", function() self.mode = "freeDrive"; self:startRace() end),
        Settings = item("City races", function() self.mode = "race"; self:showSetup() end),
        Exit = item("Main menu", function() self:showMainMenu() end)
    }, self.saveError)
end

function RacingGameOpenCity:showControls()
    self:showMenu("controls", "CITY DRIVING CONTROLS",
        "W / Up: throttle   S / Down: brake\nA / Left & D / Right: steer\nMouse wheel: zoom   C: camera\nR: return to start / restart route   Esc: pause\nStreet races: follow cyan arrows in order. Shortcuts do not count.",
        {Start = item("Back to settings", function() self:showSettings() end)})
end

function RacingGameOpenCity:getProperties(parameters)
    RacingGameFull.getProperties(self, parameters)
    local p = parameters:at(0)
    p:setPropertyAsInt("City Blocks", self.cityBlocks)
    p:setPropertyAsInt("City Route", self.cityRoute)
end
function RacingGameOpenCity:setProperties(parameters)
    local p = parameters:at(0)
    self.cityBlocks = math.max(6, math.min(10, math.floor(p:getPropertyAsInt("City Blocks", self.cityBlocks) / 2) * 2))
    self.cityRoute = math.max(0, math.min(2, p:getPropertyAsInt("City Route", self.cityRoute)))
    RacingGameFull.setProperties(self, parameters)
end

function RacingGameOpenCity:shutdown()
    RacingGameFull.shutdown(self)
    if not self.rebuilding then
        self.preferenceRecords, self.routeRecordsPath = nil, nil
    end
end
