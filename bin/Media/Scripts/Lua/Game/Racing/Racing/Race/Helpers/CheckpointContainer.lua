if not RacingSupport then include("RacingSupport.lua") end
class 'CheckpointContainer' (RacingComponent)
function CheckpointContainer:__init(component) BaseComponent.__init(self, component); self.gates, self.racers = {}, {} end
function CheckpointContainer:setCheckpoints(gates)
    RacingSupport.integer(#gates,2,10000,"checkpoint count")
    local copy={}
    for i,gate in ipairs(gates) do assert(gate.position and gate.normal,"Checkpoint is unconfigured"); copy[i]=gate end
    self.gates, self.racers = copy, {}
end
function CheckpointContainer:reset(id) self.racers[id] = nil end
function CheckpointContainer:sample(id, position)
    assert((type(id)=="string" and #id>0 and #id<=64) or (RacingSupport.finite(id) and id>=0 and id==math.floor(id)), "Racer ID required")
    position = RacingSupport.vector(position)
    local state = self.racers[id]
    if not state then
        local count=0; for _ in pairs(self.racers) do count=count+1 end
        assert(count<64,"Too many racers in checkpoint container")
        state = {next=1, laps=0}; self.racers[id] = state
    end
    local gate, event = self.gates[state.next], nil
    if gate and state.previous and gate:crossed(state.previous, position) then
        state.next = state.next + 1
        if state.next > #self.gates then state.next, state.laps, event = 1, state.laps+1, "lap" else event = "checkpoint" end
    end
    state.previous = position
    return event, state.next, state.laps
end
function CheckpointContainer:remove(id) self.racers[id] = nil end
function CheckpointContainer:shutdown() self.gates, self.racers = {}, {} end
