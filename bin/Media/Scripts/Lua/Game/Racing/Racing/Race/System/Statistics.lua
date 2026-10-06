if not RaceSession then include("RaceSession.lua") end
if not RacingSupport then include("RacingSupport.lua") end
class 'Statistics' (RacingComponent)
function Statistics:__init(component) BaseComponent.__init(self, component); self:Setup() end
function Statistics:Setup()
    self.m_Rank, self.m_FinishRank, self.m_Lap = 1, -1, 1
    self.lapTimeCounter, self.m_TotalTimeCounter, self.bestLapCounter, self.prevLapCounter = 0, 0, 0, 0
    self.currentLapTime, self.prevLapTime, self.bestLapTime, self.totalRaceTime = "", "", "", ""
    self.finishedRace, self.goingWrongway, self.speedRecord, self.knockedOut = false, false, 0, false
end
function Statistics:updateSession(session, progress, speed)
    self.m_Lap = session.lap or 1
    self.finishedRace, self.goingWrongway = session.state == "finished", progress.wrongWay
    self.m_FinishRank = self.finishedRace and self.m_Rank or -1
    self.speedRecord = math.max(self.speedRecord, speed or 0)
    self.m_TotalTimeCounter = session.elapsed or 0
    self.lapTimeCounter = self.m_TotalTimeCounter - (session.lapStart or 0)
    self.prevLapCounter, self.bestLapCounter = session.lastLapTime or 0, session.bestLapTime or 0
    self.currentLapTime, self.prevLapTime = RaceSession.formatTime(self.lapTimeCounter), RaceSession.formatTime(self.prevLapCounter)
    self.bestLapTime, self.totalRaceTime = RaceSession.formatTime(self.bestLapCounter), RaceSession.formatTime(self.m_TotalTimeCounter)
end
function Statistics:SetRank(value) self.m_Rank = RacingSupport.integer(value,1,64,"rank") end
function Statistics:SetLap(value) self.m_Lap = RacingSupport.integer(value,1,10000,"lap") end
