if not RacingSupport then include("RacingSupport.lua") end
class 'OpponentControl' (BaseComponent)
function OpponentControl:__init(component)
    BaseComponent.__init(self, component)
    self.targetSpeed, self.lookAhead, self.enabled, self.zones = 25, 12, false, {}
end
function OpponentControl:bind(car, actor, body, circuit)
    assert(car and actor and body and circuit, "AI requires car, actor, body and circuit")
    self.car, self.actor, self.body, self.circuit = car, actor, body, circuit
end
function OpponentControl:enable(value)
    self.enabled = value == true
    if not self.enabled and self.car then self.car:setControls(0, 1, 0) end
end
function OpponentControl:update(dt)
    RacingSupport.delta(self, dt)
    if not self.enabled or not self.car then return end
    local position, speed = self.actor:getPosition(), RacingSupport.length(self.body:getLinearVelocity())
    local index = self.circuit:nearest(position)
    local route = self.circuit:GetRoutePoint(self.circuit:distanceAt(index) + self.lookAhead + speed*0.4)
    local heading = self.actor:getWorldTransform():forward()
    local desired = RacingSupport.unit(route.position - position)
    -- -Z is forward: positive steering points toward +X.
    local cross = heading.x*desired.z - heading.z*desired.x
    local steer = RacingSupport.clamp(cross*2.5, -1, 1)
    local target = self.targetSpeed * math.max(0.25, 1 - math.abs(steer)*0.6)
    for _, zone in ipairs(self.zones) do if zone:contains(position) then target = math.min(target, zone.speedLimit) end end
    local error = target - speed
    local throttle, brake = RacingSupport.clamp(error*0.2, 0, 1), RacingSupport.clamp(-error*0.2, 0, 1)
    self.car:setControls(throttle, brake, steer)
end
function OpponentControl:shutdown() self:enable(false); self.car, self.actor, self.body, self.circuit = nil, nil, nil, nil end
