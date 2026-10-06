if not RacingSupport then include("RacingSupport.lua") end
if not RaceSession then include("RaceSession.lua") end
class 'Laps' (RacingComponent)
function Laps:__init(component) BaseComponent.__init(self, component); self:reset() end
function Laps:reset() RaceSession.resetProgress(self); self.completedLaps = 0 end
function Laps:sample(index, count, onRoad, elapsed)
    if RaceSession.advanceLap(self, index, count, onRoad, elapsed) then
        self.completedLaps = self.completedLaps + 1; return true
    end
    return false
end
