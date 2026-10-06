if not RacingSupport then include("RacingSupport.lua") end
if not RaceView then include("RaceView.lua") end
if not RacingNetworkTransport then include("RacingNetworkTransport.lua") end
class 'RaceManagerView' (RacingComponent)
function RaceManagerView:__init(component)
    BaseComponent.__init(self, component); self.views, self.isHost, self.id = {}, true, 0
end
function RaceManagerView:bind(manager, ranks, transport) self.manager, self.ranks, self.transport = manager, ranks, transport end
function RaceManagerView:JoinRace(view)
    assert(view:IsReady(), "Racer is not ready")
    assert(not self.views[view.id] or self.views[view.id] == view, "Duplicate racer")
    local count = 0; for _ in pairs(self.views) do count=count+1 end
    assert(self.views[view.id] or count < 64, "Race is full")
    self.views[view.id] = view
    if self.ranks then self.ranks:register(view); self.ranks:refresh() end
    self:RefreshRaceViews()
end
function RaceManagerView:LeaveRace(view)
    if self.views[view.id] ~= view then return false end
    self.views[view.id] = nil
    if self.ranks then self.ranks:remove(view.id) end
    self:RefreshRaceViews(); return true
end
function RaceManagerView:RefreshRaceViews()
    local count = 0; for _ in pairs(self.views) do count=count+1 end
    if self.manager then self.manager.totalRacers = count end
end
function RaceManagerView:GetRaceViewById(id) return self.views[id] end
function RaceManagerView:HasRaceView(view) return self.views[view.id] == view end
function RaceManagerView:SetTotalLaps(laps)
    assert(self.manager, "Manager is unbound"); self.manager.totalLaps = RacingSupport.integer(laps, 1, 10, "laps")
end
function RaceManagerView:RestartRace()
    assert(self.isHost and self.manager, "Only the host may restart")
    for _, view in pairs(self.views) do view:RestartRace() end
    self.manager:InitializeRace(self.manager._raceType == 1 and "timeTrial" or "race", self.manager.totalLaps)
end
function RaceManagerView:publish()
    assert(self.isHost and self.transport, "Host transport required")
    self.transport:sendSession(self:ToData())
    for _, view in pairs(self.views) do self.transport:sendRacer(view:ToData()) end
end
local states={idle=0,countdown=1,racing=2,paused=3,finished=4}
local names={[0]="idle",[1]="countdown",[2]="racing",[3]="paused",[4]="finished"}
function RaceManagerView:ToData()
    local s=assert(self.manager,"Manager is unbound").session
    return {id=self.id,mode=s.mode or "race",state=states[s.state],resume=states[s.resumeState] or 0,
        laps=self.manager.totalLaps,racers=math.max(1,self.manager.totalRacers),elapsed=s.elapsed or 0,countdown=s.countdown or 0,
        lap=s.lap or 1,completed=s.completedLaps or 0,best=s.bestLapTime or 0,last=s.lastLapTime or 0}
end
function RaceManagerView:receiveSession(data)
    assert(not self.isHost and self.manager,"Only clients receive host session state")
    data=RacingNetworkTransport.validateSession(data); assert(data.id==self.id,"Wrong session view")
    local s=self.manager.session
    s.mode,s.state,s.resumeState=data.mode,names[data.state],data.resume>0 and names[data.resume] or nil
    s.lapLimit,s.elapsed,s.countdown,s.lap,s.completedLaps=data.laps,data.elapsed,data.countdown,data.lap,data.completed
    s.bestLapTime,s.lastLapTime,s.lapTimes=data.best,data.last,s.lapTimes or {}
    s.lapStart=s.elapsed; self.manager.totalLaps,self.manager.totalRacers=data.laps,data.racers
    self.manager:syncState()
end
function RaceManagerView:receive(data)
    assert(not self.isHost, "Host state cannot be replaced by a remote snapshot")
    local view = self.views[data.id]; assert(view, "Unknown racer")
    view:FromData(data)
end
function RaceManagerView:shutdown()
    if self.ranks then for id in pairs(self.views) do self.ranks:remove(id) end end
    self.views, self.manager, self.ranks, self.transport = {}, nil, nil, nil
end
