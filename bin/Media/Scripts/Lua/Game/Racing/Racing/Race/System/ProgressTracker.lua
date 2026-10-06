if not RacingSupport then include("RacingSupport.lua") end
if not RaceSession then include("RaceSession.lua") end
class 'ProgressTracker' (RacingComponent)
function ProgressTracker:__init(component)
    BaseComponent.__init(self, component)
    self.m_CurrentIndex, self.m_ProgressDistance, self.m_RaceCompletion = 0, 0, 0
    self.lapProgress = {}; RaceSession.resetProgress(self.lapProgress)
    self.roadHalfWidth, self.runoffWidth = 6.85, 11
end
function ProgressTracker:bind(actor, circuit)
    assert(actor and circuit, "Progress requires actor and circuit")
    self.actor, self.circuit = actor, circuit; self:RestartRace()
end
function ProgressTracker:sample()
    local circuit, actor = self.circuit, self.actor
    local count = circuit.numPoints
    assert(count > 1, "Circuit must contain at least two samples")
    local position = actor:getPosition()
    local index = circuit:nearest(position)
    local offset = position - circuit:position(index)
    local distance = math.sqrt(offset.x * offset.x + offset.z * offset.z)
    local direction = circuit:position(index + 1) - circuit:position(index)
    direction:normalise()
    local forward = actor:getWorldTransform():forward()
    local onRoad = distance < self.roadHalfWidth
    RaceSession.advanceLap(self.lapProgress, index, count, onRoad, 0)
    self.m_CurrentIndex, self.m_ProgressDistance = index, circuit:distanceAt(index)
    self.m_RaceCompletion = (self.lapProgress.lap-1) + self.m_ProgressDistance/circuit.Length
    return {index = index, count = count, onRoad = onRoad,
        offTrack = distance > self.runoffWidth, wrongWay = forward:dotProduct(direction) < -0.3, offset = distance}
end
function ProgressTracker:get_currentIndex() return self.m_CurrentIndex end
function ProgressTracker:get_progressDistance() return self.m_ProgressDistance end
function ProgressTracker:get_raceCompletion() return self.m_RaceCompletion end
function ProgressTracker:SetProgressDistance(value) self.m_ProgressDistance = RacingSupport.number(value, 0, 10000000, "progress") end
function ProgressTracker:set_raceCompletion(value) self.m_RaceCompletion = RacingSupport.number(value, 0, 10000, "completion") end
function ProgressTracker:RestartRace()
    self.m_CurrentIndex, self.m_ProgressDistance, self.m_RaceCompletion = 0, 0, 0
    RaceSession.resetProgress(self.lapProgress)
end
