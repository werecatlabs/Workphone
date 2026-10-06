if not RaceSession then include("RaceSession.lua") end
class 'RaceManager' (BaseComponent)
RaceManager.RaceType = {Circuit = 0, TimeTrial = 1}
RaceManager.RaceState = {Initialising = 0, StartingGrid = 1, Racing = 2, Paused = 3, Complete = 4, None = 7}
local states = {idle = 7, countdown = 1, racing = 2, paused = 3, finished = 4}
function RaceManager:__init(component)
    BaseComponent.__init(self, component)
    self.session, self.listeners = RaceSession.new(), {}
    self.totalLaps, self.totalRacers = 3, 1
    self.m_RaceState, self._raceType = self.RaceState.None, self.RaceType.Circuit
end
function RaceManager:InitializeRace(mode, laps)
    self._raceType = mode == "timeTrial" and self.RaceType.TimeTrial or self.RaceType.Circuit
    self.totalLaps = laps or self.totalLaps
    self.session:begin(mode, self.totalLaps)
    self:syncState()
end
function RaceManager:syncState()
    self.m_RaceState = states[self.session.state]
    self.racePaused, self.raceCompleted = self.session.state == "paused", self.session.state == "finished"
    self.m_RaceStarted = self.session.state == "racing"
    self.currentCountdownTime = math.ceil(self.session.countdown or 0)
end
function RaceManager:OnUpdate(dt, progress)
    local event = self.session:update(dt, progress.index, progress.count, progress.onRoad)
    self:syncState()
    if event and self.listeners[event] then self.listeners[event](self.session) end
    return event
end
function RaceManager:PauseRace()
    local changed = self.session:pause()
    self:syncState()
    return changed
end
function RaceManager:ResumeRace()
    local changed = self.session:resume()
    self:syncState()
    return changed
end
function RaceManager:EndRace() self.session:finish(); self:syncState() end
function RaceManager:FormatTime(seconds) return RaceSession.formatTime(seconds) end
function RaceManager:setListener(event, callback) self.listeners[event] = callback end
