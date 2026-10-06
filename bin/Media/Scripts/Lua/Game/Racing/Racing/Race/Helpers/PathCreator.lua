if not RacingSupport then include("RacingSupport.lua") end
if not WaypointCircuit then include("WaypointCircuit.lua") end
class 'PathCreator' (RacingComponent)
function PathCreator:__init(component) BaseComponent.__init(self, component); self.circuit = WaypointCircuit(component) end
function PathCreator:fromActors(actors)
    local points = {}
    for i, actor in ipairs(actors) do points[i] = actor:getPosition() end
    self.circuit:SetWaypoints(points)
    return self.circuit
end
function PathCreator:fromProceduralScene(scene) self.circuit:setProceduralScene(scene); return self.circuit end
function PathCreator:sample(spacing)
    RacingSupport.number(spacing,0.0001,10000000,"path spacing")
    assert(self.circuit.Length>0 and self.circuit.Length/spacing <= 100000, "Empty or excessively dense path")
    local points, distance = {}, 0
    while distance < self.circuit.Length do points[#points+1] = self.circuit:GetRoutePosition(distance); distance = distance+spacing end
    return points
end
