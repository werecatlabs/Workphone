--Statistics.lua keeps track of the racer's rank, lap, race times, saving best times, wrong way detecion etc.
class 'Statistics' (BaseComponent)

function Statistics:__init()

    --Int
    self.m_FinishRank = -1 --current rank
    self.m_Rank = 0 --current rank
    self.m_Lap = 0 --current lap
    self.checkpoint = 0 --current checkpoint(Checkpoint Race)

    --Strings
    self.currentLapTime = "" --current lap time string displayed by RaceUI.cs
    self.prevLapTime = "" --Previous lap time string displayed by RaceUI.cs
    self.totalRaceTime = "" --Total lap time string displayed by RaceUI.cs
    self.bestLapTime = "" --Best lap time string for current session;

    --Floats
    self.lapTimeCounter = 0.0 -- keeps track of our current Lap time counter
    self.m_TotalTimeCounter = 0.0 --keeps track of our total race time
    self.prevLapCounter = 0.0
    self.bestLapCounter = 0.0 --keeps track of the current session's best lap time
    self.dotProduct = 0.0 --used for wrong way detection
    self.registerDistance = 10.0 --distance to register a passed node
    self.reviveTimer = 0.0
    self.wrongwayTimer = 0.0 --delay timer

    --Hidden Vars
    self.lastPassedNode = nil --last node to passed - used when respawning.
    self.target = nil --progress tracker target
    self.currentNodeNumber = 0 --next node index in the "path" list
    self.path = {}
    self.passednodes = {}
    self.checkpoints = {}
    self.passedcheckpoints = {}

    self.finishedRace = false
    self.knockedOut = false
    self.goingWrongway = false
    self.passedAllNodes = false
    self.speedRecord = 0.0 --speed trap top speed
    
    return self
end


function Statistics:OnDestroy()
    -- Assuming a global ApplicationManager with an event system
    -- ApplicationManager:Unsubscribe("sceneLoaded", self.SceneWasLoaded)
    -- ApplicationManager:Unsubscribe("sceneUnloaded", self.SceneWasUnloaded)
end

function Statistics:SetRank(value)
    if self.m_Rank ~= value then
        self.m_Rank = value

        -- Assuming GetComponent is available and returns a RaceView-like object
        -- local raceData = self:GetComponent("RaceView")
        -- if raceData then
        --     raceData:OnSetRank(value)
        -- end
    end
end

function Statistics:SetRankFromRPC(value)
    if self.m_Rank ~= value then
        self.m_Rank = value
    end
end

function Statistics:SetLap(value)
    if self.m_Lap ~= value then
        self.m_Lap = value

        -- Assuming GetComponent is available and returns a RaceView-like object
        -- local raceData = self:GetComponent("RaceView")
        -- if raceData then
        --     raceData:OnSetLap(value)
        -- end
    end
end

function Statistics:SetLapFromRPC(value)
    if self.m_Lap ~= value then
        self.m_Lap = value
    end
end

function Statistics:Setup()
    self.lapTimeCounter = 0.0
    self.m_TotalTimeCounter = 0.0
    self.bestLapCounter = 0.0
    self.dotProduct = 0.0

    self.m_FinishRank = -1

    self.finishedRace = false
    self.knockedOut = false
    self.goingWrongway = false
    self.passedAllNodes = false
    self.speedRecord = 0.0

    -- Assuming RaceManager.instance is available
    if not RaceManager.instance then
        -- self.enabled = false; -- Or equivalent logic to disable this script
        return
    else
        self.path = {}
        self.passednodes = {}
        self.checkpoints = {}
        self.passedcheckpoints = {}

        self:FindPath()
        --self:FindCheckpoints()
        self:Initialize()
    end
end

function Statistics:SceneWasLoaded(scene, loadSceneMode)
    self:Setup()
end

function Statistics:SceneWasUnloaded(scene)
end

function Statistics:Start()
    -- Assuming a global ApplicationManager with an event system
    -- ApplicationManager:Subscribe("sceneLoaded", self.SceneWasLoaded)
    -- ApplicationManager:Subscribe("sceneUnloaded", self.SceneWasUnloaded)

    -- Assuming RaceManager.instance and StartCoroutine are available
    -- local raceManager = RaceManager.instance
    -- if raceManager then
    --     StartCoroutine(raceManager:UpdateRaceData())
    -- end
end

function Statistics:UpdateLogic()
    -- Implementation is partial as C# source was truncated
    local laps = math.max(1, self.m_Lap)
    self:SetLap(laps)

    -- ... more implementation needed
end


function Statistics:EnterRace()
    -- Assuming RaceManager.instance is available
    if not RaceManager.instance then
        -- self.enabled = false;
        -- self:GetComponent("ProgressTracker").enabled = self.enabled
        return
    else
        self:FindPath()
        self:Initialize()
    end
end

function Statistics:Initialize()
    self:SetLap(1)
    -- Assuming RaceManager.instance is available
    self.m_Rank = RaceManager.instance.playerStartRank
end

function Statistics:FindPath()
    -- Assuming RaceManager.instance is available
    local raceManager = RaceManager.instance
    if raceManager then
        local pathContainer = raceManager.pathContainer
        if not pathContainer then
            -- Assuming a global GameObject.FindObjectOfType
            -- local circuit = GameObject.FindObjectOfType("WaypointCircuit")
            -- if circuit then
            --     pathContainer = circuit.transform
            -- end
        end

        if pathContainer then
            -- Assuming GetComponentsInChildren exists and returns a list of transforms
            -- local nodes = pathContainer:GetComponentsInChildren("Transform")
            -- for _, p in ipairs(nodes) do
            --     if p ~= pathContainer then
            --         table.insert(self.path, p)
            --     end
            -- end
            -- self.passednodes = {} -- resize to path.Count
            -- self.lastPassedNode = self.path[1]
        end
    end
end

function Statistics:Update()
    -- Assuming ApplicationManager.instance is available
    local applicationManager = ApplicationManager.instance
    if applicationManager and applicationManager.isRaceMode then
        self:GetPath()
        self:CalculateRaceTimes()
        self:CalculateAngleDifference()
        self:Revive()
    end

    self:UpdateLogic()
end

function Statistics:GetPath()
    if #self.path == 0 then
        self:Setup()
        return
    end

    local n = self.currentNodeNumber
    if n >= #self.path then
        return
    end

    local node = self.path[n]

    if not self.target then
        -- Assuming ApplicationManager.instance is available
        local player = ApplicationManager.instance.pilotObject
        if player then
            self.target = player.transform
        end
        return
    end

    local nodeVector
    if self.target and node then
        -- Assuming InverseTransformPoint and magnitude are available
        -- nodeVector = self.target:InverseTransformPoint(node.position)
    end

    -- register that we have passed this node
    if nodeVector and nodeVector.magnitude <= self.registerDistance then
        self.currentNodeNumber = self.currentNodeNumber + 1
        self.passednodes[n] = true
        if n > 1 then
            self.lastPassedNode = self.path[n - 1]
        else
            self.lastPassedNode = self.path[#self.path]
        end
    end

    --Check if all nodes have been passed
    local allPassed = true
    for _, pass in ipairs(self.passednodes) do
        if not pass then
            allPassed = false
            break
        end
    end
    self.passedAllNodes = allPassed

    --Reset the currentNodeNumber after passing all the nodes
    if self.currentNodeNumber >= #self.path then
        self.currentNodeNumber = 0
    end
end

function Statistics:CalculateAngleDifference()
    if not self.target then
        return
    end

    --check if the racer is going the wrong way
    -- Assuming eulerAngles and rigidbody components are available
    -- local nodeAngle = self.target.transform.eulerAngles.y
    -- local transformAngle = self.transform.eulerAngles.y
    -- local angleDifference = nodeAngle - transformAngle

    -- if math.abs(angleDifference) <= 230 and math.abs(angleDifference) >= 120 then
    --     local rigidbody = self:GetComponent("Rigidbody")
    --     if rigidbody then
    --         if (rigidbody.velocity.magnitude * 2.237) > 10.0 then
    --             self.goingWrongway = true
    --         else
    --             self.goingWrongway = false
    --         end
    --     end
    -- else
    --     self.goingWrongway = false
    -- end
end

function Statistics:CalculateRaceTimes()
    if RaceManager.instance.raceStarted and not self.knockedOut and not self.finishedRace then
        if RaceManager.instance._raceType ~= "Checkpoints" and RaceManager.instance._raceType ~= "Elimination" then
            self.lapTimeCounter = self.lapTimeCounter + Time.deltaTime
        else
            -- Checkpoint race
            self.lapTimeCounter = self.lapTimeCounter - Time.deltaTime

            if self.lapTimeCounter <= 0 then
                if RaceManager.instance._raceType == "Checkpoints" then
                    self.knockedOut = true
                    -- Assuming a global 'gameObject' reference to self
                    RaceManager.instance:KnockoutRacer(gameObject)
                end
            end
        end
        self.m_TotalTimeCounter = self.m_TotalTimeCounter + Time.deltaTime
    end

    -- Format the time strings
    self.bestLapTime = RaceManager.instance:FormatTime(self.bestLapCounter)
    self.currentLapTime = RaceManager.instance:FormatTime(self.lapTimeCounter)
    self.totalRaceTime = RaceManager.instance:FormatTime(self.m_TotalTimeCounter)
end

function Statistics:GhostVehicleLogic(firstLap, beatLastLap)
    -- Assuming GetComponent and a 'gameObject' reference exist
    -- if not RaceManager.instance.enableGhostVehicle or not self:GetComponent("GhostVehicle") then
    --     return
    -- end

    -- --Always create a ghost and cache values after the first lap
    -- if firstLap then
    --     self:GetComponent("GhostVehicle"):CacheValues()
    --     RaceManager.instance:CreateGhostVehicle(gameObject)
    -- end

    -- --Create a ghost & cache the values if we beat the last ghost
    -- if beatLastLap then
    --     self:GetComponent("GhostVehicle"):CacheValues()
    --     RaceManager.instance:CreateGhostVehicle(gameObject)
    -- else
    --     --Use the cached values if we dont beat the last lap
    --     if not firstLap then
    --         self:GetComponent("GhostVehicle"):UseCachedValues()
    --         RaceManager.instance:CreateGhostVehicle(gameObject)
    --     end
    -- end

    -- --Reset the recorded values
    -- self:GetComponent("GhostVehicle"):ClearValues()
end

function Statistics:CheckForBestTime()
    --Best Lap Time
    if self.bestLapCounter == 0 then
        self.bestLapCounter = self.lapTimeCounter
        self.bestLapTime = RaceManager.instance:FormatTime(self.bestLapCounter)
        self:GhostVehicleLogic(true, false)
    elseif self.prevLapCounter < self.bestLapCounter then
        self.bestLapCounter = self.prevLapCounter
        self.bestLapTime = RaceManager.instance:FormatTime(self.bestLapCounter)
        self:GhostVehicleLogic(false, true)
    elseif self.prevLapCounter > self.bestLapCounter then
        self:GhostVehicleLogic(false, false)
    end

    -- Save Best Track Lap Time
    -- Assuming a 'gameObject' reference
    -- if gameObject.tag == "Player" then
    --     local applicationManager = ApplicationManager.instance
    --     local scene = applicationManager.selectedScenery
    --     if scene then
    --         local sceneName = scene.label

    --         -- Save a new best time if we don't currently have one
    --         -- This part requires a PlayerPrefs equivalent for Lua
    --         if not PlayerPrefs.HasKey("BestTimeFloat" .. sceneName) then
    --             PlayerPrefs.SetString("BestTime" .. sceneName, RaceManager.instance:FormatTime(self.lapTimeCounter))
    --             PlayerPrefs.SetFloat("BestTimeFloat" .. sceneName, math.abs(self.lapTimeCounter))
    --             -- PlayerPrefs.Save()

    --             if RaceManager.instance.showRaceInfoMessages then
    --                 RaceUI.instance:ShowRaceInfo("New best time!", 2.0)
    --             end
    --         end

    --         -- Save a new best time if we beat our current best time
    --         if PlayerPrefs.GetFloat("BestTimeFloat" .. sceneName) > self.lapTimeCounter then
    --             PlayerPrefs.SetFloat("BestTimeFloat" .. sceneName, self.lapTimeCounter)
    --             PlayerPrefs.SetString("BestTime" .. sceneName, RaceManager.instance:FormatTime(self.lapTimeCounter))
    --             -- PlayerPrefs.Save()

    --             if RaceManager.instance.showRaceInfoMessages then
    --                 RaceUI.instance:ShowRaceInfo("New best time!", 2.0)
    --             end
    --         end
    --     end
    -- end
end

function Statistics:KnockoutLastPlace()
    -- if self.m_Rank == RankManager.instance.currentRacers - 1 then
    --     RaceManager.instance:KnockoutRacer(RankManager.instance.racerRanks[RankManager.instance.currentRacers].racer) -- Lua arrays are 1-based
    -- end
end

function Statistics:NewLapOld()
    -- Assuming a 'gameObject' and 'PlayerPrefs' equivalent
    -- if gameObject.tag == "Player" then
    --     --Save a new best time if we dont currently have one
    --     if PlayerPrefs.GetFloat("BestTimeFloat" .. Application.loadedLevelName) == 0 then
    --         PlayerPrefs.SetString("BestTime" .. Application.loadedLevelName, self.currentLapTime)
    --         PlayerPrefs.SetFloat("BestTimeFloat" .. Application.loadedLevelName, self.lapTimeCounter)
    --         -- PlayerPrefs.Save()

    --         if RaceManager.instance.showRaceInfoMessages then
    --             RaceUI.instance:ShowRaceInfo("You have a new best time!", 2.0)
    --         end
    --     end
    --     --Save a new best time if we beat our current best time
    --     if PlayerPrefs.GetFloat("BestTimeFloat" .. Application.loadedLevelName) > self.lapTimeCounter then
    --         PlayerPrefs.SetString("BestTime" .. Application.loadedLevelName, self.currentLapTime)
    --         PlayerPrefs.SetFloat("BestTimeFloat" .. Application.loadedLevelName, self.lapTimeCounter)
    --         -- PlayerPrefs.Save()

    --         if RaceManager.instance.showRaceInfoMessages then
    --             RaceUI.instance:ShowRaceInfo("You have a new best time!", 2.0)
    --         end
    --     end
    -- end

    -- --Check for knockout
    -- if RaceManager.instance._raceType == "LapKnockout" then
    --     --knock out the last racer when the 2nd last racer passes the finish line!
    --     if self.m_Rank == RankManager.instance.currentRacers - 1 then
    --         RaceManager.instance:KnockoutRacer(RankManager.instance.racerRanks[RankManager.instance.currentRacers].racer)
    --     end
    -- end

    -- --Reset our passed nodes
    for i = 1, #self.passednodes do
        self.passednodes[i] = false
    end

    -- --add a lap or finish the race deopending on our current lap
    -- if self.m_Lap < RaceManager.instance.totalLaps then
    --     self:SetLap(self.m_Lap + 1)

    --     --Show the final lap indication text if set to true in RaceManager
    --     if self.m_Lap == RaceManager.instance.totalLaps and RaceManager.instance.showRaceInfoMessages and gameObject.tag == "Player" then
    --         RaceUI.instance:ShowRaceInfo("Final Lap!", 2.0)
    --     end
    -- else
    --     if not self.knockedOut and not self.finishedRace then
    --         self:CompleteRace()
    --     end
    -- end

    -- --Set the previous lap time and reset the lap counter
    self.prevLapTime = self.currentLapTime
    self.lapTimeCounter = 0.0
end

function Statistics:NewLap()
    if self.finishedRace or self.knockedOut then
        return
    end

    self.prevLapTime = self.currentLapTime
    self.prevLapCounter = self.lapTimeCounter

    self:CheckForBestTime()

    --Check for knockout
    if RaceManager.instance._raceType == "LapKnockout" then
        self:KnockoutLastPlace()
    end

    --Reset our passed nodes & checkpoints
    for i = 1, #self.passednodes do
        self.passednodes[i] = false
    end

    for i = 1, #self.passedcheckpoints do
        self.passedcheckpoints[i] = false
    end

    --add a lap or finish the race depending on our current lap and Race Type
    if RaceManager.instance._raceType == "TimeTrial" then
        --Create the ghost vehicle
        if RaceManager.instance.enableGhostVehicle then
            self:GhostVehicleLogic()
        end
        self:SetLap(self.m_Lap + 1)
    else
        if self.m_Lap < RaceManager.instance.totalLaps then
            self:SetLap(self.m_Lap + 1)

            --Show the final lap indication text if set to true in RaceManagesr
            -- if self.m_Lap == RaceManager.instance.totalLaps and RaceManager.instance.showRaceInfoMessages and gameObject.tag == "Player" then
            --     RaceUI.instance:ShowRaceInfo("Final Lap!", 2.0)
            -- end
        else
            if not self.knockedOut and not self.finishedRace then
                self:FinishRace()
            end
        end
    end

    --Set the previous lap time and reset the lap counter
    self.prevLapTime = self.currentLapTime

    if RaceManager.instance._raceType ~= "Checkpoints" or RaceManager.instance._raceType ~= "Elimination" then
        self.lapTimeCounter = 0.0
    end
end

function Statistics:CompleteRace()
    -- local applicationManager = ApplicationManager.instance
    -- local modelController = applicationManager.modelController

    -- if gameObject == modelController.gameObject or gameObject.tag == "Player" then
    --     RaceManager.instance.raceCompleted = true

    --     if self.m_Rank == 1 then
    --         -- print("You finished 1st!")
    --     elseif self.m_Rank == 2 then
    --         -- print("You finished 2nd!")
    --     elseif self.m_Rank == 3 then
    --         -- print("You finished 3rd!")
    --     end
    -- end

    -- if RaceManager.instance.continueAfterFinish then
    --     self:AIMode()
    -- else
    --     self:GetComponent("Car_Controller").controllable = false
    -- end

    self.finishedRace = true
end

function Statistics:AIMode()
    --[[
    if self:GetComponent("PlayerControl") then
        -- Destroy(self:GetComponent("PlayerControl"))
        -- gameObject:AddComponent("OpponentControl")
    end
    --]]
end

function Statistics:Revive()
    if self.finishedRace and self.knockedOut then
        return
    end

    --incase the car flips over or going wrong way then respawn
    -- local zAngle = self.transform.localEulerAngles.z
    -- if (zAngle > 80 and zAngle < 280) or (RaceManager.instance.forceWrongwayRespawn and self.goingWrongway) then
    --     self.reviveTimer = self.reviveTimer + Time.deltaTime
    -- else
    --     self.reviveTimer = 0.0
    -- end

    -- if self.reviveTimer >= 5.0 then
    --     RaceManager.instance:RespawnRacer(self.transform, self.lastPassedNode, 3.0)
    --     self.reviveTimer = 0.0
    -- end
end

function Statistics:OnTriggerEnter(other)
    -- local applicationManager = ApplicationManager.instance
    -- if applicationManager then
    --     if MultiplayerConnector.inRoom then
    --         if gameObject then
    --             local view = self:GetComponent("PhotonView")
    --             if view and view.isMine then
    --                 local raceManager = RaceManager.instance
    --                 if raceManager then
    --                     if other.tag == "FinishLine" and self.lapTimeCounter > 3.0 and raceManager.raceStarted and self.passedAllNodes then
    --                         self:NewLap()
    --                     end
    --                 end
    --             end
    --         end
    --     else
    --         local raceManager = RaceManager.instance
    --         if raceManager then
    --             if other.tag == "FinishLine" and self.lapTimeCounter > 3.0 and self.passedAllNodes then
    --                 self:NewLap()
    --             end
    --         end
    --     end
    -- end
end

function Statistics:FinishRace()
    -- local applicationManager = ApplicationManager.instance
    -- if applicationManager then
    --     local modelController = applicationManager.modelController
    --     if modelController then
    --         if modelController.gameObject == gameObject then
    --             RaceManager.instance:EndRace(self.m_Rank)
    --         elseif MultiplayerConnector.inRoom then
    --             -- More logic was here in C#
    --         end
    --     end
    -- end
end


return Statistics
