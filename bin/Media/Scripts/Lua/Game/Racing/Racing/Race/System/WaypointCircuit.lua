-- Reuse the procedural circuit's path and native nearest-point search.
class 'WaypointCircuit' (BaseComponent)
function WaypointCircuit:__init(component)
    BaseComponent.__init(self, component)
    self.points, self.distances, self.Length, self.numPoints = {}, {}, 0, 0
end
function WaypointCircuit:setProceduralScene(scene)
    self.scene = scene
    self.Length, self.numPoints = scene:getCircuitLength(), scene:getCircuitSampleCount()
end
function WaypointCircuit:SetWaypoints(points)
    self.scene, self.points, self.distances, self.Length = nil, points, {}, 0
    self.numPoints = #points
    for i, point in ipairs(points) do
        self.distances[i] = self.Length
        self.Length = self.Length + (points[i % #points + 1] - point):length()
    end
end
function WaypointCircuit:position(index)
    if self.scene then return self.scene:getCircuitPosition(index % self.numPoints) end
    return self.points[index % self.numPoints + 1]
end
function WaypointCircuit:nearest(position)
    if self.scene then return self.scene:nearestCircuitSample(position) end
    local nearest, distance = 0, math.huge
    for i, point in ipairs(self.points) do
        local delta = point - position
        local squared = delta:dotProduct(delta)
        if squared < distance then nearest, distance = i - 1, squared end
    end
    return nearest
end
function WaypointCircuit:GetRoutePoint(distance)
    assert(self.numPoints > 1 and self.Length > 0, "Circuit has no route")
    local index, alpha
    distance = distance % self.Length
    if self.scene then
        local fraction = distance / self.Length * self.numPoints
        index, alpha = math.floor(fraction), fraction % 1
    else
        index = self.numPoints - 1
        for i = 1, self.numPoints - 1 do
            if distance < self.distances[i + 1] then index = i - 1; break end
        end
        local start = self.distances[index + 1]
        local finish = self.distances[index + 2] or self.Length
        alpha = (distance - start) / math.max(0.0001, finish - start)
    end
    local p0, p1 = self:position(index), self:position(index + 1)
    local direction = p1 - p0
    direction:normalise()
    return {position = p0 + (p1 - p0) * alpha, direction = direction, index = index}
end
function WaypointCircuit:GetRoutePosition(distance) return self:GetRoutePoint(distance).position end
