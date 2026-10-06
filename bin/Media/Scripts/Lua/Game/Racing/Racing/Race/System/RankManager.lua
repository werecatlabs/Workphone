if not RacingSupport then include("RacingSupport.lua") end
class 'RankManager' (RacingComponent)
function RankManager:__init(component)
    BaseComponent.__init(self, component); self.racers, self.racerRanks, self.accumulator = {}, {}, 0
end
function RankManager:register(view)
    assert(view.id ~= nil and view.statistics and view.progress, "Racer needs ID, statistics and progress")
    assert(not self.racers[view.id] or self.racers[view.id] == view, "Duplicate racer ID")
    self.racers[view.id] = view
end
function RankManager:remove(id) self.racers[id] = nil; self:refresh() end
function RankManager:refresh()
    local rows = {}
    for id, view in pairs(self.racers) do
        if not view.statistics.knockedOut then
            rows[#rows+1] = {id=id, view=view, racerStats=view.statistics, raceCompletion=view.progress:get_raceCompletion()}
        end
    end
    table.sort(rows, function(a,b)
        local sa, sb = a.racerStats, b.racerStats
        if sa.finishedRace ~= sb.finishedRace then return sa.finishedRace end
        if sa.finishedRace and sa.m_TotalTimeCounter ~= sb.m_TotalTimeCounter then return sa.m_TotalTimeCounter < sb.m_TotalTimeCounter end
        if a.raceCompletion ~= b.raceCompletion then return a.raceCompletion > b.raceCompletion end
        return a.id < b.id
    end)
    for rank, row in ipairs(rows) do
        row.view:SetRank(rank)
        if row.racerStats.finishedRace then row.racerStats.m_FinishRank = rank end
    end
    self.racerRanks, self.totalRacers, self.currentRacers = rows, #rows, #rows
    return rows
end
function RankManager:update(dt)
    self.accumulator = self.accumulator + RacingSupport.delta(self, dt)
    if self.accumulator >= 0.1 then self.accumulator = self.accumulator % 0.1; self:refresh() end
end
function RankManager:shutdown() self.racers, self.racerRanks = {}, {} end
RankManager.RefreshRacerCount, RankManager.SetCarRank, RankManager.Update = RankManager.refresh, RankManager.refresh, RankManager.update
