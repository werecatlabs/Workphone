if not RacingSupport then include("RacingSupport.lua") end
class 'RaceView' (RacingComponent)
function RaceView:__init(component)
    BaseComponent.__init(self, component)
    self.id, self.m_StartRank, self.m_Rank, self.localPlayer = 0, 1, 1, false
end
function RaceView:bind(id, actor, statistics, progress, grid)
    self.id = RacingSupport.integer(id, 0, 2147483647, "racer ID")
    assert(actor and statistics and progress, "Actor, statistics and progress required")
    self.actor, self.statistics, self.progress, self.grid = actor, statistics, progress, grid
end
function RaceView:SetStartRank(rank)
    self.m_StartRank = RacingSupport.integer(rank, 1, 64, "grid rank")
    if self.grid then self.m_StartPosition, self.m_StartRotation = self.grid:GetStartTransform(rank) end
end
function RaceView:SetupGridPosition()
    assert(self.actor and self.m_StartPosition, "Grid position is unconfigured")
    assert(not self.m_IsStarted, "Cannot move a running racer to the grid")
    self.actor:setPosition(self.m_StartPosition); self.actor:setOrientation(self.m_StartRotation)
end
function RaceView:SetRank(rank)
    self.m_Rank = RacingSupport.integer(rank, 1, 64, "rank")
    if self.statistics then self.statistics:SetRank(rank) end
end
function RaceView:RestartRace()
    self.m_IsStarted, self.isFinished = false, false
    self.statistics:Setup(); self.progress:RestartRace()
end
function RaceView:FinishedRace() self.isFinished = true; self.statistics.finishedRace = true end
function RaceView:SetTotalTime(value) self.statistics.m_TotalTimeCounter = RacingSupport.number(value, 0, 86400, "total time") end
function RaceView:SetBestLapCounter(value) self.statistics.bestLapCounter = RacingSupport.number(value, 0, 86400, "lap time") end
function RaceView:SetRaceProgress(value) self.progress:SetProgressDistance(value) end
function RaceView:SetRaceCompletion(value) self.progress:set_raceCompletion(value) end
function RaceView:IsReady() return self.actor ~= nil and self.statistics ~= nil and self.progress ~= nil end
function RaceView:IsPlayer() return self.localPlayer end
function RaceView:GetId() return self.id end
function RaceView:get_rank() return self.m_Rank end
function RaceView:get_startRank() return self.m_StartRank end
function RaceView:get_startPosition() return self.m_StartPosition end
function RaceView:get_startRotation() return self.m_StartRotation end
function RaceView:get_isStarted() return self.m_IsStarted == true end
function RaceView:ToData()
    local s = assert(self.statistics, "Racer is unbound")
    return {id=self.id, rank=self.m_Rank, startRank=self.m_StartRank, lap=s.m_Lap,
        total=s.m_TotalTimeCounter, best=s.bestLapCounter, progress=self.progress:get_progressDistance(),
        completion=self.progress:get_raceCompletion(), finished=s.finishedRace == true}
end
function RaceView.validate(data)
    assert(type(data) == "table", "Racer data must be a table")
    local r = RacingSupport
    local result = {id=r.integer(data.id,0,2147483647), rank=r.integer(data.rank,1,64),
        startRank=r.integer(data.startRank,1,64), lap=r.integer(data.lap,1,10000),
        total=r.number(data.total,0,86400), best=r.number(data.best,0,86400),
        progress=r.number(data.progress,0,10000000), completion=r.number(data.completion,0,10000)}
    assert(type(data.finished) == "boolean", "Invalid finish flag"); result.finished = data.finished
    return result
end
function RaceView:FromData(data)
    data = RaceView.validate(data)
    assert(data.id == self.id and self:IsReady(), "Snapshot does not belong to this racer")
    self:SetRank(data.rank); self.m_StartRank = data.startRank
    local s = self.statistics
    s.m_Lap, s.m_TotalTimeCounter, s.bestLapCounter, s.finishedRace = data.lap, data.total, data.best, data.finished
    self:SetRaceProgress(data.progress); self:SetRaceCompletion(data.completion); self.isFinished = data.finished
end
function RaceView:shutdown() self.actor, self.statistics, self.progress, self.grid = nil, nil, nil, nil end
