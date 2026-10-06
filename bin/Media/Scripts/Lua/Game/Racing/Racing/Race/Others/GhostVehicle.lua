if not RacingSupport then include("RacingSupport.lua") end
if not ReplayManager then include("ReplayManager.lua") end
class 'GhostVehicle' (RacingComponent)
function GhostVehicle:__init(component) BaseComponent.__init(self, component); self.replay = ReplayManager(component) end
function GhostVehicle:bind(actor, frames)
    assert(not actor:getComponent("Rigidbody") and not actor:getComponent("CarController"), "Ghost must be visual only")
    local replay = ReplayManager(self.component)
    for _, frame in ipairs(frames) do replay:append(frame.time, frame.position, frame.rotation) end
    assert(#replay.frames >= 2, "Ghost requires replay samples")
    self.actor, self.replay = actor, replay
end
function GhostVehicle:start(loop) self.actor:setEnabled(true); self.replay:play(self.actor, loop) end
function GhostVehicle:update(dt) self.replay:update(dt) end
function GhostVehicle:hide() self.replay:stop(); if self.actor then self.actor:setEnabled(false) end end
function GhostVehicle:shutdown() self:hide(); self.actor = nil; self.replay:shutdown() end
