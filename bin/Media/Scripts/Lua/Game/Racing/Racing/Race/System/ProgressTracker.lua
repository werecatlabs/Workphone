class 'ProgressTracker' (BaseComponent)
function ProgressTracker:__init(component)
    BaseComponent.__init(self, component)
    self.m_CurrentIndex, self.m_ProgressDistance, self.m_RaceCompletion = 0, 0, 0
end
function ProgressTracker:bind(actor, circuit) self.actor, self.circuit = actor, circuit end
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
    self.m_CurrentIndex, self.m_ProgressDistance = index, index / count * circuit.Length
    return {index = index, count = count, onRoad = distance < 6.85,
        offTrack = distance > 11, wrongWay = forward:dotProduct(direction) < -0.3, offset = distance}
end
function ProgressTracker:get_currentIndex() return self.m_CurrentIndex end
function ProgressTracker:get_progressDistance() return self.m_ProgressDistance end
function ProgressTracker:get_raceCompletion() return self.m_RaceCompletion end
function ProgressTracker:RestartRace() self.m_CurrentIndex, self.m_ProgressDistance, self.m_RaceCompletion = 0, 0, 0 end
