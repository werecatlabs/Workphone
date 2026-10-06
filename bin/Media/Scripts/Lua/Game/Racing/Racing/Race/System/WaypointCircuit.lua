-- Reuse the procedural circuit's path and native nearest-point search.
if not RacingSupport then include("RacingSupport.lua") end
class 'WaypointCircuit' (RacingComponent)
function WaypointCircuit:__init(component)
    BaseComponent.__init(self, component)
    self.points, self.distances, self.Length, self.numPoints = {}, {}, 0, 0
end
function WaypointCircuit:setProceduralScene(scene)
    local length, count = scene:getCircuitLength(), scene:getCircuitSampleCount()
    RacingSupport.number(length, 0.001, 10000000, "circuit length")
    RacingSupport.integer(count, 2, 1000000, "circuit samples")
    self.scene, self.Length, self.numPoints = scene, length, count
end
function WaypointCircuit:SetWaypoints(points)
    RacingSupport.integer(#points, 2, 100000, "point count")
    local copy, distances, length = {}, {}, 0
    for i, point in ipairs(points) do copy[i] = RacingSupport.vector(point) end
    for i, point in ipairs(copy) do
        distances[i] = length
        local segment = RacingSupport.length(copy[i % #copy + 1] - point)
        assert(segment > 0.0001, "Adjacent waypoints must differ")
        length = length + segment
    end
    self.scene, self.points, self.distances, self.Length, self.numPoints = nil, copy, distances, length, #copy
end
function WaypointCircuit:position(index)
    assert(self.numPoints > 1, "Circuit has no route")
    RacingSupport.integer(index, -100000000, 100000000, "sample index")
    if self.scene then return self.scene:getCircuitPosition(index % self.numPoints) end
    return RacingSupport.vector(self.points[index % self.numPoints + 1])
end
function WaypointCircuit:distanceAt(index)
    assert(self.numPoints > 1, "Circuit has no route")
    index = index % self.numPoints
    return self.scene and index/self.numPoints*self.Length or self.distances[index+1]
end
function WaypointCircuit:nearest(position)
    assert(self.numPoints > 1, "Circuit has no route")
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
    RacingSupport.number(distance, -100000000, 100000000, "route distance")
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
