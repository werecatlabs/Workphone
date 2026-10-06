-- CarController is the single owner of keyboard/gamepad handling.
class 'PlayerControl' (BaseComponent)
function PlayerControl:__init(component) BaseComponent.__init(self, component) end
function PlayerControl:bind(car) self.car = car end
function PlayerControl:enable(value)
    if value then self.car:usePlayerControls() else self.car:setControls(0, 1, 0) end
end
