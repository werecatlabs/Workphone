class 'ProgressTracker' (BaseComponent)

function ProgressTracker:__init()
    BaseComponent.__init(self)
    self.circuit = nil
    self.lookAheadForTargetOffset = 0.5
    self.lookAheadForTargetFactor = 0.15
    self.lookAheadForSpeedOffset = 50
    self.lookAheadForSpeedFactor = 0.5
    self.targetPoint = nil
    self.speedPoint = nil
    self.progressPoint = nil
    self.target = nil
    self.m_Statistics = nil
    self.m_ProgressDistance = 0.0
    self.m_RaceCompletion = 0.0
    self.lastPosition = Vector3.zero
    self.speed = 0.0
    self.m_CurrentIndex = 0
    self.isPlayer = false
end

function ProgressTracker:get_currentIndex()
    return self.m_CurrentIndex
end

function ProgressTracker:set_currentIndex(value)
    self.m_CurrentIndex = value
end

function ProgressTracker:get_progressDistance()
    return self.m_ProgressDistance
end

function ProgressTracker:get_raceCompletion()
    return self.m_RaceCompletion
end

function ProgressTracker:set_raceCompletion(value)
    self.m_RaceCompletion = value
end

function ProgressTracker:SetProgressDistance(value)
    self.m_ProgressDistance = value
end

function ProgressTracker:Awake()
    if not RaceManager.instance then return end
    self.target = GameObject("New Progress Tracker").transform
    self.circuit = FindObjectOfType("WaypointCircuit")
    self.m_Statistics = self:GetComponent("Statistics")
    if self.m_Statistics then
        self.m_Statistics.target = self.target
        if #self.m_Statistics.path > 0 then
            local distance = -Vector3.Distance(self.transform.position, self.m_Statistics.path[1].position)
            self.m_ProgressDistance = distance
        else
            self.m_ProgressDistance = 0.0
        end
    end
    ApplicationManager.sceneLoaded:AddListener(self, self.SceneWasLoaded)
    ApplicationManager.sceneUnloaded:AddListener(self, self.SceneWasUnloaded)
end

function ProgressTracker:OnDestroy()
    ApplicationManager.sceneLoaded:RemoveListener(self, self.SceneWasLoaded)
    ApplicationManager.sceneUnloaded:RemoveListener(self, self.SceneWasUnloaded)
end

function ProgressTracker:Start()
    if self.target then
        self.target.name = self.name .. "_ProgressTracker"
    end
end

function ProgressTracker:CalulcateProgress(numLaps, nodeDist, routePoint)
    local targetPointIndex = self.currentIndex
    local currentLap = math.max(numLaps - 1, 0)
    local lapDistance = RaceManager.instance.raceDistance / RaceManager.instance.totalLaps
    self.m_ProgressDistance = currentLap * lapDistance
    self.m_ProgressDistance = self.m_ProgressDistance + targetPointIndex * nodeDist
    self.m_ProgressDistance = self.m_ProgressDistance - (routePoint.position - self.gameObject.transform.position):magnitude()
end

function ProgressTracker:CalculateRaceCompletion(statistics)
    if not statistics.finishedRace then
        if not statistics.knockedOut then
            self.raceCompletion = self.progressDistance / RaceManager.instance.raceDistance
        end
    else
        self.raceCompletion = 100
    end
    self.raceCompletion = math.floor(self.raceCompletion * 100 + 0.5) / 100
end

function ProgressTracker:Update()
    local applicationManager = ApplicationManager.instance
    if not applicationManager or not self.circuit then return end
    local statistics = self:GetComponent("Statistics")
    if not RaceManager.instance.raceStarted then
        if statistics and #statistics.path > 0 then
            local distance = -Vector3.Distance(self.transform.position, statistics.path[1].position)
            self.m_ProgressDistance = distance
        end
        return
    end
    if Time.deltaTime > 0 then
        self.speed = Mathf.Lerp(self.speed, (self.lastPosition - self.transform.position):magnitude() / Time.deltaTime, Time.deltaTime)
    end
    local routePointDistance = self.m_ProgressDistance + self.lookAheadForTargetOffset + self.lookAheadForTargetFactor * self.speed
    local routePoint = self.circuit:GetRoutePointByIndex(self.m_CurrentIndex)
    local nodeDist = (RaceManager.instance.raceDistance / RaceManager.instance.totalLaps) / self.circuit.numPoints
    local numLaps = statistics and statistics.lap or 0
    if MultiplayerConnector.inRoom then
        local view = PhotonView.Get(self)
        if view and view.isMine then
            self:CalulcateProgress(numLaps, nodeDist, routePoint)
        end
    else
        self:CalulcateProgress(numLaps, nodeDist, routePoint)
    end
    if (self.transform.position - routePoint.position):magnitude() < 3.0 then
        self.m_CurrentIndex = self.m_CurrentIndex + 1
        if self.m_CurrentIndex >= self.circuit.numPoints then
            self.m_CurrentIndex = 0
        end
        if applicationManager:IsPlayer(self.gameObject) then
            local startPosition = routePoint.position
            local startOrientation = Quaternion.LookRotation(-routePoint.direction, Vector3.up)
            if applicationManager.isRaceMode then
                if MultiplayerConnector.inRoom then
                    if RaceManager.instance.raceStarted then
                        applicationManager:SetStartTransform(startPosition, startOrientation)
                    end
                else
                    applicationManager:SetStartTransform(startPosition, startOrientation)
                end
            else
                applicationManager:SetStartTransform(startPosition, startOrientation)
            end
        end
    end
    if self.target then
        self.target.position = routePoint.position
        self.target.rotation = Quaternion.LookRotation(routePoint.direction)
    end
    self.lastPosition = self.transform.position
    if MultiplayerConnector.inRoom then
        local view = PhotonView.Get(self)
        if view and view.isMine then
            self:CalculateRaceCompletion(statistics)
        end
    else
        self:CalculateRaceCompletion(statistics)
    end
end

function ProgressTracker:SceneWasLoaded(scene, loadSceneMode)
    self.circuit = FindObjectOfType("WaypointCircuit")
end

function ProgressTracker:SceneWasUnloaded(scene)
    self.circuit = nil
end

function ProgressTracker:RestartRace()
    self.m_ProgressDistance = 0.0
    self.raceCompletion = 0.0
    self.lastPosition = Vector3.zero
    self.speed = 0.0
    self.m_CurrentIndex = 0
end

return ProgressTracker