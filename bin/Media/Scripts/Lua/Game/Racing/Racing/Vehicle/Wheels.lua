if not RacingSupport then include("RacingSupport.lua") end
class 'Wheels' (RacingComponent)
function Wheels:__init(component) BaseComponent.__init(self, component); self.wheels = {} end
function Wheels:bind(wheels)
    assert(#wheels > 0, "Native wheel controllers required")
    self.wheels = wheels
end
function Wheels:configure(radius, damping, travel)
    RacingSupport.number(radius, 0.01, 10, "radius")
    RacingSupport.number(damping, 0, 100000, "damping")
    RacingSupport.number(travel, 0, 10, "travel")
    for _, wheel in ipairs(self.wheels) do
        wheel:setRadius(radius); wheel:setWheelDamping(damping); wheel:setSuspensionDistance(travel)
    end
end
function Wheels:reset() for _, wheel in ipairs(self.wheels) do wheel:reset() end end
function Wheels:shutdown() self.wheels = {} end
