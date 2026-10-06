if not RacingSupport then include("RacingSupport.lua") end
-- Bounded ribbon data. A renderer callback receives a segment; no per-frame actors.
class 'Skidmark' (RacingComponent)
function Skidmark:__init(component)
    BaseComponent.__init(self, component); self.capacity, self.minimumDistance = 256, 0.15
    self:clear()
end
function Skidmark:bind(renderer) assert(type(renderer) == "function", "Ribbon renderer required"); self.renderer = renderer end
function Skidmark:add(position, intensity, contact)
    position = RacingSupport.vector(position); intensity = RacingSupport.number(intensity, 0, 1, "intensity")
    if not contact or intensity <= 0 then self.previous = nil; return false end
    if self.previous and RacingSupport.length(position - self.previous) < self.minimumDistance then return false end
    local segment = {from = self.previous, to = position, intensity = intensity}
    self.previous = position
    if not segment.from then return false end
    self.head = self.head % self.capacity + 1; self.segments[self.head] = segment
    self.count = math.min(self.capacity, self.count + 1)
    if self.renderer then self.renderer(segment, self.head) end
    return true
end
function Skidmark:clear() self.previous, self.head, self.count, self.segments = nil, 0, 0, {} end
function Skidmark:shutdown() self:clear(); self.renderer = nil end
