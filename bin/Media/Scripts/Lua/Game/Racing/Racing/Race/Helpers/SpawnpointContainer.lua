if not RacingSupport then include("RacingSupport.lua") end
class 'SpawnpointContainer' (RacingComponent)
function SpawnpointContainer:__init(component) BaseComponent.__init(self, component); self.points = {} end
function SpawnpointContainer:setPoints(points)
    local validated = {}
    for i, point in ipairs(points) do validated[i] = {position=RacingSupport.vector(point.position), rotation=RacingSupport.quaternion(point.rotation)} end
    self.points = validated
end
function SpawnpointContainer:generateGrid(circuit, count, spacing, width)
    RacingSupport.integer(count, 1, 64, "grid size")
    spacing = RacingSupport.number(spacing or 6, 1, 100, "grid spacing")
    width = RacingSupport.number(width or 2.5, 0, 20, "grid width")
    local points = {}
    for i=1,count do
        local route = circuit:GetRoutePoint(-math.floor((i-1)/2)*spacing-2)
        local d = route.direction
        local angle = RacingSupport.atan2(-d.x, -d.z)
        points[i] = {position=route.position + Vector3F(-d.z, 0, d.x)*((i%2 == 1 and -1 or 1)*width),
            rotation=Quaternion(math.cos(angle/2), 0, math.sin(angle/2), 0)}
    end
    self:setPoints(points)
end
function SpawnpointContainer:GetStartTransform(rank)
    RacingSupport.integer(rank, 1, #self.points, "start rank")
    local point = self.points[rank]
    return RacingSupport.vector(point.position), RacingSupport.quaternion(point.rotation)
end
