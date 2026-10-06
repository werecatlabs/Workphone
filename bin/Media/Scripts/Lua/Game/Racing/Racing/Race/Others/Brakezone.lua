if not RacingSupport then include("RacingSupport.lua") end
class 'Brakezone' (RacingComponent)
function Brakezone:__init(component) BaseComponent.__init(self, component) end
function Brakezone:configure(center, radius, speed)
    self.center, self.radius = RacingSupport.vector(center), RacingSupport.number(radius, 0.1, 10000, "radius")
    self.speedLimit = RacingSupport.number(speed, 0, 200, "speed limit")
end
function Brakezone:contains(position)
    assert(self.center, "Brake zone is unconfigured")
    local delta = position - self.center
    return delta.x*delta.x + delta.z*delta.z <= self.radius*self.radius
end
