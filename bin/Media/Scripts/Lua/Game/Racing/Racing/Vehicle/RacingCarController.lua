if not RacingSupport then include("RacingSupport.lua") end
class 'RacingCarController' (BaseComponent)
function RacingCarController:__init(component) BaseComponent.__init(self, component) end
function RacingCarController:bind(car, actor)
    assert(car, "Native CarController required")
    self.car, self.actor = car, actor
end
function RacingCarController:setControls(throttle, brake, steering)
    assert(self.car, "Controller is unbound")
    self.car:setControls(RacingSupport.number(throttle, -1, 1, "throttle"),
        RacingSupport.number(brake, 0, 1, "brake"), RacingSupport.number(steering, -1, 1, "steering"))
end
function RacingCarController:stop() if self.car then self.car:setControls(0, 1, 0) end end
function RacingCarController:usePlayerControls() assert(self.car, "Controller is unbound"); self.car:usePlayerControls() end
function RacingCarController:shutdown() self:stop(); self.car, self.actor = nil, nil end
