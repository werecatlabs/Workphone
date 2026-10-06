if not RacingSupport then include("RacingSupport.lua") end
-- Plane crossing with a finite rectangular aperture. Test a swept segment to
-- detect fast racers, require forward travel, and reject teleports by distance.
class 'RaceGate' (RacingComponent)
function RaceGate:__init(component)
    BaseComponent.__init(self, component); self.halfWidth, self.halfHeight, self.maxStep = 7, 5, 100
end
function RaceGate:configure(position, normal, halfWidth, halfHeight)
    normal = RacingSupport.vector(normal); assert(RacingSupport.length(normal)>0.00001,"Gate normal must be nonzero")
    self.position, self.normal = RacingSupport.vector(position), RacingSupport.unit(normal)
    self.halfWidth = RacingSupport.number(halfWidth or self.halfWidth, 0.01, 1000, "gate width")
    self.halfHeight = RacingSupport.number(halfHeight or self.halfHeight, 0.01, 1000, "gate height")
    local n = self.normal
    local right = Vector3F(-n.z, 0, n.x)
    if RacingSupport.length(right) < 0.001 then right = Vector3F(1, 0, 0) end
    self.right = RacingSupport.unit(right)
    self.up = RacingSupport.unit(Vector3F(n.y*self.right.z-n.z*self.right.y,
        n.z*self.right.x-n.x*self.right.z, n.x*self.right.y-n.y*self.right.x))
    return self
end
function RaceGate:crossed(previous, current)
    assert(self.position, "Gate must be configured")
    local delta = current - previous
    if RacingSupport.length(delta) > self.maxStep then return false end
    local a, b = (previous-self.position):dotProduct(self.normal), (current-self.position):dotProduct(self.normal)
    if a >= 0 or b < 0 then return false end
    local point = previous + delta*(-a/(b-a)) - self.position
    return math.abs(point:dotProduct(self.right)) <= self.halfWidth and math.abs(point:dotProduct(self.up)) <= self.halfHeight
end
