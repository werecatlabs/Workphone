if not RaceSession then include("RaceSession.lua") end
if not RacingSupport then include("RacingSupport.lua") end
class 'RaceStandingsUI' (RacingComponent)
function RaceStandingsUI:__init(component) BaseComponent.__init(self, component) end
function RaceStandingsUI:bind(label, ranks) self.label, self.ranks = label, ranks end
function RaceStandingsUI:format(rows)
    local lines = {"POS  RACER  BEST LAP  TOTAL"}
    for position, row in ipairs(rows) do
        local stats, view = row.racerStats, row.view
        local name = tostring(view.name or ("Racer " .. view.id)):gsub("[%c]", " "):sub(1,32)
        lines[#lines+1] = string.format("%d  %s  %s  %s", position, name,
            RaceSession.formatTime(stats.bestLapCounter), RaceSession.formatTime(stats.m_TotalTimeCounter))
    end
    return table.concat(lines, "\n")
end
function RaceStandingsUI:update()
    if self.label and self.ranks then RacingSupport.label(self.label, self:format(self.ranks.racerRanks)) end
end
function RaceStandingsUI:shutdown() self.label,self.ranks=nil,nil end
