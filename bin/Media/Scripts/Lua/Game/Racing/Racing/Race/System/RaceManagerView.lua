class 'RaceManagerView' (BaseComponent)

function RaceManagerView:__init()
    BaseComponent.__init(self)
    self.startTime = 0.0
    self.endTime = 0.0
    self.m_IsStarted = false
    self.isFinished = false
    self.isHost = false
    self.m_TotalLaps = 3
    self.m_NextStateUpdateTime = 0.0
    self.m_ViewData = nil
end

function RaceManagerView:GetRaceViewById(id)
    local raceManager = RaceManager.instance
    if raceManager then
        for _, raceView in ipairs(raceManager.m_RaceViews) do
            if raceView:GetId() == id then
                return raceView
            end
        end
    end
    return nil
end

function RaceManagerView:get_isStarted()
    return self.m_IsStarted
end

function RaceManagerView:set_isStarted(value)
    self.m_IsStarted = value
end

function RaceManagerView:get_viewData()
    return self.m_ViewData
end

function RaceManagerView:set_viewData(value)
    self.m_ViewData = value
end

function RaceManagerView:OnJoinedRoom()
    local rankManager = RankManager.instance
    if rankManager then
        rankManager:RefreshRacerCount()
    end
end

function RaceManagerView:OnLeftRoom()
    local rankManager = RankManager.instance
    if rankManager then
        rankManager:RefreshRacerCount()
    end
    local raceManager = RaceManager.instance
    if raceManager and not raceManager.isRestarting and raceManager.isHost then
        self:OnHostQuit()
    end
end

function RaceManagerView:HostQuit()
    local applicationManager = ApplicationManager.instance
    if applicationManager and applicationManager.isRaceMode then
        local raceManager = RaceManager.instance
        if raceManager then
            raceManager:HostQuit()
        end
    end
end

function RaceManagerView:OnHostQuit()
    -- To be overridden if needed
end

function RaceManagerView:SetTotalLaps(totalLaps)
    self.m_TotalLaps = totalLaps
    local raceManager = RaceManager.instance
    if raceManager then
        raceManager.totalLaps = totalLaps
    end
    print("RaceManagerView.OnSetTotalLapsRPC: " .. tostring(totalLaps))
end

function RaceManagerView:OnSetTotalLaps(totalLaps)
    -- To be overridden if needed
end

function RaceManagerView:ToJson()
    local data = {}
    local raceManager = RaceManager.instance
    if raceManager then
        data.totalLaps = raceManager.totalLaps
        data.totalRacers = raceManager.totalRacers
        data.raceViewData = {}
        for _, raceView in ipairs(raceManager.m_RaceViews) do
            table.insert(data.raceViewData, raceView:ToJson())
        end
    end
    return data
end

function RaceManagerView:FromJson(data)
    local raceManager = RaceManager.instance
    if raceManager then
        raceManager.totalLaps = data.totalLaps
        raceManager.totalRacers = data.totalRacers
        if RankManager.instance then
            RankManager.instance.totalRacers = data.totalRacers
        end
        if data.raceViewData then
            for _, raceViewData in ipairs(data.raceViewData) do
                local raceView = self:GetRaceViewById(raceViewData.id)
                if raceView then
                    raceView:FromJson(raceViewData)
                end
            end
        end
    end
    self.viewData = data
end

function RaceManagerView:SetViewData(dataStr)
    -- Assume a JSON decode function is available as JsonUtility.FromJson
    local ok, data = pcall(function() return JsonUtility.FromJson(dataStr) end)
    if ok and data then
        self:FromJson(data)
    else
        print("RaceManagerView:SetViewData error: " .. tostring(data))
    end
end

function RaceManagerView:JoinRace(raceView)
    local raceManager = RaceManager.instance
    if raceManager then
        raceManager:JoinRace(raceView)
    end
    local modelController = raceView:GetComponent("ModelController")
    if modelController then
        local applicationManager = ApplicationManager.instance
        if applicationManager and applicationManager.isRaceMode then
            local playerName = modelController.playerName
            if playerName and playerName ~= "" then
                if UIManager and UIManager.instance then
                    UIManager.instance:ShowInformation(playerName .. " joined race.", 1.0)
                end
            end
        end
    end
end

function RaceManagerView:LeaveRace(raceView)
    local raceManager = RaceManager.instance
    if raceManager then
        raceManager:LeaveRace(raceView)
    end
    local modelController = raceView:GetComponent("ModelController")
    if modelController then
        local applicationManager = ApplicationManager.instance
        if applicationManager and applicationManager.isRaceMode then
            local playerName = modelController.playerName
            if playerName and playerName ~= "" then
                if UIManager and UIManager.instance then
                    UIManager.instance:ShowInformation(playerName .. " left race.", 1.0)
                end
            end
        end
    end
end

function RaceManagerView:HasRaceView(raceView)
    local raceManager = RaceManager.instance
    if raceManager then
        for _, rv in ipairs(raceManager.m_RaceViews) do
            if rv == raceView then
                return true
            end
        end
    end
    return false
end

function RaceManagerView:HasActiveRaceView(raceView)
    local raceManager = RaceManager.instance
    if raceManager then
        for _, rv in ipairs(raceManager.m_ActiveRaceViews) do
            if rv == raceView then
                return true
            end
        end
    end
    return false
end

function RaceManagerView:RefreshRaceViews()
    -- To be overridden if needed
end

function RaceManagerView:RefreshRaceViewsCoroutine(delay)
    -- Simulate coroutine with timer if available, otherwise just call after delay
    if Timer then
        Timer.Create(delay, function() self:RefreshRaceViews() end)
    else
        self:RefreshRaceViews()
    end
end

function RaceManagerView:RefreshRaceViewsWithDelay(delay)
    self:RefreshRaceViewsCoroutine(delay)
end

function RaceManagerView:RestartRace()
    local raceManager = RaceManager.instance
    if raceManager then
        raceManager:RestartRace()
    end
end

function RaceManagerView:OnRestartRace()
    -- To be overridden if needed
end