if not RacingSupport then include("RacingSupport.lua") end
class 'TargetFollower' (RacingComponent)
function TargetFollower:__init(component)
    BaseComponent.__init(self, component); self.offset, self.smoothing, self.localOffset = Vector3F(0,3,8), 12, true
end
function TargetFollower:bind(actor, target, offset)
    assert(actor and target and actor ~= target, "Follower and target must differ")
    self.actor, self.target = actor, target
    if offset then self.offset = RacingSupport.vector(offset) end
end
function TargetFollower:update(dt)
    dt = RacingSupport.delta(self, dt)
    if not self.actor or not self.target then return end
    local offset = self.offset
    if self.localOffset then
        local transform = self.target:getWorldTransform()
        offset = transform:right()*offset.x + transform:up()*offset.y + transform:forward()*(-offset.z)
    end
    local desired = self.target:getPosition()+offset
    local alpha = self.smoothing <= 0 and 1 or 1-math.exp(-self.smoothing*dt)
    local position = self.actor:getPosition()
    self.actor:setPosition(position+(desired-position)*alpha)
    self.actor:setOrientation(self.fixedRotation or self.target:getOrientation())
end
function TargetFollower:shutdown() self.actor, self.target = nil, nil end
