if not RaceGate then include("RaceGate.lua") end
class 'DroneGate' (RaceGate)
function DroneGate:__init(component)
    RaceGate.__init(self, component); self.halfWidth, self.halfHeight = 2, 2
end
