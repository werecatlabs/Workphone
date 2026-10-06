if not RacingSupport then include("RacingSupport.lua") end
class 'ReplayManager' (RacingComponent)
function ReplayManager:__init(component)
    BaseComponent.__init(self, component); self.interval, self.capacity = 0.05, 12000
    self.frames, self.state, self.elapsed, self.cursor = {}, "idle", 0, 1
end
function ReplayManager:record(actor)
    assert(actor, "Replay actor required")
    self.actor, self.frames, self.elapsed, self.nextSample, self.state = actor, {}, 0, 0, "recording"
    self.lastUpdate=RacingSupport.now()
end
function ReplayManager:append(time, position, rotation)
    assert(#self.frames < self.capacity, "Replay capacity exceeded")
    RacingSupport.number(time, 0, 86400, "replay time")
    assert(#self.frames == 0 or time > self.frames[#self.frames].time, "Replay timestamps must increase")
    self.frames[#self.frames+1] = {time=time, position=RacingSupport.vector(position), rotation=RacingSupport.quaternion(rotation)}
end
function ReplayManager:play(actor, loop)
    assert(#self.frames >= 2, "Replay needs at least two samples")
    self.actor, self.loop, self.elapsed, self.cursor, self.state = assert(actor), loop == true, 0, 1, "playing"
    self.lastUpdate=RacingSupport.now()
    self:apply(0)
end
function ReplayManager:sample(time)
    local frames = self.frames
    if #frames == 0 then return nil end
    time = RacingSupport.clamp(time, frames[1].time, frames[#frames].time)
    local low, high = 1, #frames
    while low < high do local mid=math.floor((low+high+1)/2); if frames[mid].time <= time then low=mid else high=mid-1 end end
    local a, b = frames[low], frames[math.min(low+1,#frames)]
    local alpha = b.time > a.time and (time-a.time)/(b.time-a.time) or 0
    return a.position+(b.position-a.position)*alpha, RacingSupport.rotation(a.rotation,b.rotation,alpha)
end
function ReplayManager:apply(time)
    local position, rotation = self:sample(time)
    if position then self.actor:setPosition(position); self.actor:setOrientation(rotation) end
end
function ReplayManager:update(dt)
    dt = RacingSupport.delta(self, dt)
    if self.state == "paused" or self.state == "idle" or self.state == "complete" then return end
    self.elapsed = self.elapsed+dt
    if self.state == "recording" then
        if self.elapsed >= self.nextSample then
            if #self.frames >= self.capacity then self.state = "complete"; return end
            self:append(self.elapsed,self.actor:getPosition(),self.actor:getOrientation())
            self.nextSample = self.elapsed+self.interval
        end
    else
        local duration = self.frames[#self.frames].time
        if self.elapsed >= duration then
            if self.loop then self.elapsed = self.elapsed % duration else self.elapsed, self.state = duration, "complete" end
        end
        self:apply(self.elapsed)
    end
end
function ReplayManager:pause() if self.state == "playing" or self.state == "recording" then self.resumeState,self.state=self.state,"paused" end end
function ReplayManager:resume() if self.state == "paused" then self.state,self.resumeState=self.resumeState,nil; self.lastUpdate=RacingSupport.now() end end
function ReplayManager:stop() self.state, self.actor = "idle", nil end
function ReplayManager:shutdown() self:stop(); self.frames = {} end
