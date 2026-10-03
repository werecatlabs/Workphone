--Race_UI.cs handles displaying all UI in the race.

class 'RaceUI' (BaseComponent)

-- Create Persistent singleton
RaceUI.instance = nil

-- The C# version has a nested class RaceStandings. 
-- In Lua, we'll just use a table with the same structure:
-- { pos = GUIText, name = GUIText, time = GUIText }

function RaceUI:__init()
    RaceUI.instance = self
	
    self.player = nil

    -- Panels
    self.racePanel = nil
    self.pausePanel = nil
    self.race_completePanel = nil
    self.race_ko_Panel = nil
    self.raceSessionStartPanel = nil

    -- Texts
    self.rank = nil
    self.lap = nil
    self.currentLapTime = nil
    self.previousLapTime = nil
    self.bestLapTime = nil
    self.totalTime = nil
    self.currentSpeed = nil
    self.currentGear = nil
    self.countdown = nil
    self.raceInfo = nil
    self.wrongwayText = nil
    
    self.raceStandings = {} -- A list of RaceStandings-like tables
    self.menuScene = "Menu"

    -- Speedometer
    self.needle = nil
    self.minNeedleAngle = -20.0
    self.maxNeedleAngle = 160.0
    self.rotationMultiplier = 0.85
    self.needleRotation = 0

    self.raceInfos = {}
end

function RaceUI:Start()
    -- In a real scenario, you would subscribe to engine events here.
    -- e.g. EventManager:Subscribe("SceneLoaded", self, self.SceneWasLoaded)
    --      EventManager:Subscribe("SceneUnloaded", self, self.SceneWasUnloaded)

    for i, v in ipairs(self.raceStandings) do
        v.pos:SetText("")
        v.name:SetText("")
        v.time:SetText("")
    end

    if self.raceInfo then
        self.raceInfo:SetText("")
    end
end

function RaceUI:OnDestroy()
    -- Unsubscribe from events here
    -- e.g. EventManager:Unsubscribe("SceneLoaded", self, self.SceneWasLoaded)
end

function RaceUI:SceneWasLoaded(scene, loadSceneMode)
    self.raceInfos = {}
end

function RaceUI:SceneWasUnloaded(scene)
    self.raceInfos = {}
end

function RaceUI:HideAll()
    if self.racePanel then self.racePanel:SetActive(false) end
    if self.pausePanel then self.pausePanel:SetActive(false) end
    if self.race_completePanel then self.race_completePanel:SetActive(false) end
    if self.race_ko_Panel then self.race_ko_Panel:SetActive(false) end
    if self.raceSessionStartPanel then self.raceSessionStartPanel:SetActive(false) end
end


function RaceUI:Update()
    self:UpdateUI()
    self:VehicleGUI()

    if RaceManager.instance.viewingReplay then
        self:UpdateReplayUI()
    end
end


function RaceUI:UpdateUI()
    pcall(function()
        local applicationManager = ApplicationManager.instance
        if not applicationManager or applicationManager.isQuitting then
            return
        end

        if applicationManager.applicationState ~= SaracenTypes.ApplicationState.FB_STATE_FLIGHT or not applicationManager.isRaceMode then
            self:HideAll()
            return
        end

        if not self.player then
            local pilot = applicationManager.pilotObject
            if pilot then
                self.player = pilot:GetComponent("Statistics")
            end
        end

        if self.player then
            local raceType = RaceManager.instance._raceType
            
            if raceType == RaceManager.RaceType.Circuit or 
               raceType == RaceManager.RaceType.LapKnockout or
               raceType == RaceManager.RaceType.SpeedTrap then
                self:DefaultUI()
            elseif raceType == RaceManager.RaceType.TimeTrial then
                self:TimeTrialUI()
            elseif raceType == RaceManager.RaceType.Checkpoints then
                self:CheckpointRaceUI()
            elseif raceType == RaceManager.RaceType.Elimination then
                self:EliminationRaceUI()
            end

            self:HandlePanelActivation()

            local ui = UIManager.instance
            if ui and ui.isFullscreenDialogDisplayed then
                self:HideAll()
            end
        end

        if not RaceManager.instance.raceCompleted then
            self:UpdateInRaceStandings()
        else
            self:UpdateEndRaceStandings()
        end
    end)
end

function RaceUI:EliminationRaceUI()
end

function RaceUI:CheckpointRaceUI()
end

function RaceUI:TimeTrialUI()
    if self.rank then
        self.rank:SetText("Pos " .. self.player.rank .. "/" .. RankManager.instance.currentRacers)
    end

    if self.lap then
        self.lap:SetText("Lap " .. self.player.lap)
    end

    if self.currentLapTime then
        self.currentLapTime:SetText("Current " .. self.player.currentLapTime)
    end

    if self.totalTime then
        self.totalTime:SetText("Total " .. self.player.totalRaceTime)
    end

    if self.previousLapTime then
        if self.player.prevLapTime and self.player.prevLapTime ~= "" then
            self.previousLapTime:SetText("Last " .. self.player.prevLapTime)
        else
            self.previousLapTime:SetText("Last --:--:--")
        end
    end

    if self.bestLapTime then
        local applicationManager = ApplicationManager.instance
        local scene = applicationManager.selectedScenery
        if scene then
            local sceneName = scene.label
            -- Assuming a global PlayerPrefs object similar to Unity's
            if PlayerPrefs:HasKey("BestTime" .. sceneName) then
                self.bestLapTime:SetText("Best " .. PlayerPrefs:GetString("BestTime" .. sceneName))
            else
                self.bestLapTime:SetText("Best --:--:--")
            end
        end
    end

    if self.wrongwayText then
        if self.player:GetComponent("Statistics").goingWrongway then
            self.wrongwayText:SetText("Wrong Way!")
        else
            self.wrongwayText:SetText("")
        end
    end
end

function RaceUI:UpdateEndRaceStandings()
end

function RaceUI:UpdateInRaceStandings()
end

function RaceUI:UpdateReplayUI()
end

function RaceUI:VehicleGUI()
end

function RaceUI:DefaultUI()
    pcall(function()
        if not self.player then return end

        -- The speedometer logic from C# is commented out, but would be translated like this:
        --[[
        local playerCarController = self.player:GetComponent("Car_Controller")
        if playerCarController then
            if self.currentSpeed then
                self.currentSpeed:SetText(playerCarController.currentSpeed .. " mph")
                self.currentGear:SetText("Gear " .. playerCarController.currentGear)
            end

            if self.needle then
                local fraction = playerCarController.currentSpeed / self.maxNeedleAngle
                self.needleRotation = math.lerp(self.minNeedleAngle, self.maxNeedleAngle, (fraction * self.rotationMultiplier))
                self.needle:SetEulerAngles(Vector3.new(self.needle.eulerAngles.x, self.needle.eulerAngles.y, -self.needleRotation))
            end
        end
        --]]

        if self.rank then
            if self.player.finishedRace then
                self.rank:SetText("Pos " .. self.player.finishRank .. "/" .. RankManager.instance.currentRacers)
            else
                self.rank:SetText("Pos " .. self.player.rank .. "/" .. RankManager.instance.currentRacers)
            end
        end

        if self.lap then
            self.lap:SetText("Lap " .. self.player.lap .. "/" .. RaceManager.instance.totalLaps)
        end

        if self.currentLapTime then
            self.currentLapTime:SetText("Current " .. self.player.currentLapTime)
        end

        if self.totalTime then
            self.totalTime:SetText("Total " .. self.player.totalRaceTime)
        end

        local playerStatistics = self.player
        if self.previousLapTime then
            if playerStatistics and playerStatistics.prevLapTime ~= "" then
                self.previousLapTime:SetText("Last " .. playerStatistics.prevLapTime)
            else
                self.previousLapTime:SetText("Last --:--:--")
            end
        end

        if self.bestLapTime then
            local applicationManager = ApplicationManager.instance
            local scene = applicationManager.selectedScenery
            if scene then
                local sceneName = scene.label
                if PlayerPrefs:HasKey("BestTime" .. sceneName) then
                    self.bestLapTime:SetText("Best " .. PlayerPrefs:GetString("BestTime" .. sceneName))
                else
                    self.bestLapTime:SetText("Best --:--:--")
                end
            end
        end

        if self.wrongwayText then
            if playerStatistics and playerStatistics.goingWrongway then
                self.wrongwayText:SetText("Wrong Way!")
            else
                self.wrongwayText:SetText("")
            end
        end
    end)
end

function RaceUI:ShowRaceResults()
     local raceCompletedUI = self.race_completePanel:GetComponent("RaceCompletedUI")
     if raceCompletedUI then
        raceCompletedUI:UpdateResults()
     end
end

function RaceUI:HandlePanelActivation()
    pcall(function()
        local raceManager = RaceManager.instance

        if UIManager.instance.isFullscreenDialogDisplayed then
            if self.raceSessionStartPanel then self.raceSessionStartPanel:SetActive(false) end
            if self.racePanel then self.racePanel:SetActive(false) end
            if self.race_completePanel then self.race_completePanel:SetActive(false) end
            if self.race_ko_Panel then self.race_ko_Panel:SetActive(false) end
        else
            if not raceManager.racePaused then
                if raceManager.isUserStarting or raceManager.countDownStarted or raceManager.raceStarted then
                    if self.raceSessionStartPanel then self.raceSessionStartPanel:SetActive(false) end
                else
                    local bShowRaceStart = (raceManager._raceType == RaceManager.RaceType.Circuit)

                    if raceManager.isSessionEnding then
                        bShowRaceStart = false
                    end

                    if self.raceSessionStartPanel then self.raceSessionStartPanel:SetActive(bShowRaceStart) end
                end

                if self.racePanel then self.racePanel:SetActive(true) end
                if self.pausePanel then self.pausePanel:SetActive(false) end
                if self.race_completePanel then self.race_completePanel:SetActive(false) end
                if self.race_ko_Panel then self.race_ko_Panel:SetActive(false) end
            end
        end

        if raceManager.raceCompleted then
            if self.pausePanel then self.pausePanel:SetActive(false) end
            if self.racePanel then self.racePanel:SetActive(true) end
            if self.race_completePanel then self.race_completePanel:SetActive(true) end
            if self.raceSessionStartPanel then self.raceSessionStartPanel:SetActive(false) end
            if self.race_ko_Panel then self.race_ko_Panel:SetActive(false) end
            self:ShowRaceResults()
        end

        if raceManager.raceKO then
            if self.pausePanel then self.pausePanel:SetActive(false) end
            if self.racePanel then self.racePanel:SetActive(false) end
            if self.race_completePanel then self.race_completePanel:SetActive(false) end
            if self.race_ko_Panel then self.race_ko_Panel:SetActive(true) end
        end
    end)
end

-- Used to show useful race info
function RaceUI:ShowRaceInfo(info, time)
    -- The C# version uses a coroutine. This would require a coroutine scheduler.
    -- This is a simplified version.
    if not self.raceInfo then return end

    if self.raceInfo:GetText() == "" then
        self.raceInfo:SetText(info)
        -- In a real engine, you'd start a timer here to clear the text
        -- and call CheckRaceInfoList after 'time' seconds.
        -- For example: Timer.Start(time, function() self:OnRaceInfoEnd() end)
    else
        table.insert(self.raceInfos, info)
    end
end

function RaceUI:OnRaceInfoEnd()
    if self.raceInfo then
        self.raceInfo:SetText("")
    end
    self:CheckRaceInfoList()
end

function RaceUI:CheckRaceInfoList()
    if #self.raceInfos > 0 then
        local nextInfo = self.raceInfos[#self.raceInfos]
        table.remove(self.raceInfos)
        self:ShowRaceInfo(nextInfo, 2.0)
    end
end

-- UI button functions
function RaceUI:PauseResume()
    RaceManager.instance.racePaused = not RaceManager.instance.racePaused
end

function RaceUI:Restart()
    local raceManager = RaceManager.instance
    if raceManager then
        raceManager:RestartRace()
    end
end

function RaceUI:ClickExit()
    local applicationManager = ApplicationManager.instance
    if applicationManager then
        applicationManager.isRaceMode = false
    end
end