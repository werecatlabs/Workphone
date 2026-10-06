if not RacingSupport then include("RacingSupport.lua") end
-- Native Rigidbody owns integration. Call physicsUpdate exactly once per physics
-- step from the physics owner; the ordinary Script update does not apply forces.
class 'RacingDroneController' (BaseComponent)
function RacingDroneController:__init(component)
    BaseComponent.__init(self, component)
    self.throttle, self.roll, self.pitch, self.yaw = 0, 0, 0, 0
    self.maxAcceleration, self.maxTorque, self.drag, self.armed = 20, 4, 0.4, false
end
function RacingDroneController:bind(body, actor) assert(body and actor, "Body and actor required"); self.body, self.actor = body, actor end
function RacingDroneController:setControls(throttle, roll, pitch, yaw)
    self.throttle = RacingSupport.number(throttle, 0, 1, "throttle")
    self.roll = RacingSupport.number(roll, -1, 1, "roll")
    self.pitch = RacingSupport.number(pitch, -1, 1, "pitch")
    self.yaw = RacingSupport.number(yaw, -1, 1, "yaw")
end
function RacingDroneController:arm(value) self.armed = value == true end
function RacingDroneController:physicsUpdate()
    if not self.armed or not self.body then return end
    local rotation = self.actor:getWorldTransform():getOrientation()
    local mass = self.body:getMass()
    local up = self.actor:getWorldTransform():up()
    self.body:addForce(up * (self.throttle*self.maxAcceleration*mass) - self.body:getLinearVelocity()*(self.drag*mass))
    -- Transform basis vectors instead of relying on unbound quaternion-vector operators.
    local transform = self.actor:getWorldTransform()
    self.body:addTorque((transform:right()*self.pitch + transform:up()*self.yaw + transform:forward()*self.roll)*self.maxTorque)
end
function RacingDroneController:shutdown() self.armed = false; self.body, self.actor = nil, nil end
