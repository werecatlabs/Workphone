if not RacingSupport then include("RacingSupport.lua") end
class 'WaypointArrow' (RacingComponent)
function WaypointArrow:__init(component) BaseComponent.__init(self, component) end
function WaypointArrow:bind(arrow, target) self.arrow, self.target = assert(arrow), assert(target) end
function WaypointArrow:point(position)
    if not self.arrow then return end
    local delta = position - self.arrow:getPosition()
    if delta.x*delta.x + delta.z*delta.z < 0.0001 then return end
    local yaw = RacingSupport.atan2(-delta.x, -delta.z)
    self.arrow:setOrientation(Quaternion(math.cos(yaw/2), 0, math.sin(yaw/2), 0))
end
function WaypointArrow:update() if self.target then self:point(self.target:getPosition()) end end
function WaypointArrow:shutdown() self.arrow, self.target = nil, nil end
