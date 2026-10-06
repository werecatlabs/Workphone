if not RacingCarController then include("RacingCarController.lua") end
-- A two-wheel native vehicle supplies tire/suspension physics. This controls its
-- visual rider lean; it never rotates the physics actor to fake stabilization.
class 'RacingMotorbikeController' (RacingCarController)
function RacingMotorbikeController:__init(component)
    RacingCarController.__init(self, component); self.lean, self.maxLean = 0, 0.55
end
function RacingMotorbikeController:bindRider(rider, body)
    self.rider, self.body, self.riderRotation = rider, body, rider:getOrientation()
end
function RacingMotorbikeController:update(dt)
    dt = RacingSupport.delta(self, dt)
    if not self.rider or not self.car then return end
    local speed = self.body and RacingSupport.length(self.body:getLinearVelocity()) or 0
    local target = -self.car:getSteering() * self.maxLean * math.min(speed / 10, 1)
    self.lean = self.lean + (target - self.lean) * (1 - math.exp(-8 * dt))
    self.rider:setOrientation(self.riderRotation * Quaternion(math.cos(self.lean/2), 0, 0, math.sin(self.lean/2)))
end
function RacingMotorbikeController:shutdown()
    if self.rider then self.rider:setOrientation(self.riderRotation) end
    self.rider, self.body = nil, nil; RacingCarController.shutdown(self)
end
