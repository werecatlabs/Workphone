if not RacingSupport then include("RacingSupport.lua") end
-- CarController is the single owner of keyboard/gamepad handling.
class 'PlayerControl' (RacingComponent)
function PlayerControl:__init(component) BaseComponent.__init(self, component) end
function PlayerControl:bind(car) self.car = assert(car,"Native CarController required") end
function PlayerControl:enable(value)
    assert(self.car,"Player control is unbound")
    if value then self.car:usePlayerControls() else self.car:setControls(0, 1, 0) end
end
function PlayerControl:shutdown() if self.car then self.car:setControls(0,1,0) end; self.car=nil end
