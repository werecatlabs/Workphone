-- Run from the repository root using luavm.exe. Strict native-interface doubles,
-- meaningful behavior checks, and full-directory load/constructor coverage.
dofile("Tests/lua/RacingGameFullTests.lua")
local R = RacingSupport
local app = IApplicationManager.instance()
local q = {}; q.__index = q
function Quaternion(w,x,y,z) return setmetatable({w=w or 1,x=x or 0,y=y or 0,z=z or 0},q) end
function q:W() return self.w end
function q:X() return self.x end
function q:Y() return self.y end
function q:Z() return self.z end
function q.__mul(a,b)
    return Quaternion(a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
        a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y, a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x, a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w)
end
local function near(a,b) assert(math.abs(a-b)<0.0001, tostring(a).." ~= "..tostring(b)) end
local function reject(callback) assert(not pcall(callback), "Invalid input was accepted") end
local function actor(position)
    local value={position=position or Vector3F(0,0,0),rotation=Quaternion(),enabled=true}
    function value:getPosition() return self.position end
    function value:setPosition(v) self.position=v end
    function value:getOrientation() return self.rotation end
    function value:setOrientation(v) self.rotation=v end
    function value:setEnabled(v) self.enabled=v end
    function value:getComponent() return nil end
    function value:getWorldTransform()
        return {forward=function() return self.forward or Vector3F(0,0,-1) end,
            up=function() return Vector3F(0,1,0) end, right=function() return Vector3F(1,0,0) end}
    end
    return value
end
local function label()
    return {text="",getText=function(self) return self.text end,setText=function(self,v) self.text=v end,
        setColour=function(self,v) self.colour=v end}
end
local files=dofile("Tests/lua/RacingScriptManifest.lua")
local classes={}
for _,path in ipairs(files) do
    assert(loadfile(path))
    dofile(path)
    local file=assert(io.open(path,"r")); local text=file:read("*a"); file:close()
    local name=text:match("class%s*'([^']+)'")
    if name then
        assert(not classes[name],"Duplicate racing class: "..name); classes[name]=true
        local object=_G[name]({getActor=function() return actor() end})
        assert(object.component,"Constructor dropped its component: "..name)
        object:__finalize(); object:__finalize() -- No bound resources: destruction is idempotent.
    end
    assert(not text:match("Photon") and not text:match("GameObject") and not text:match("Time%.time"),"Legacy API survived")
end

local circuit=WaypointCircuit(nil)
circuit:SetWaypoints({Vector3F(0,0,0),Vector3F(0,0,-10),Vector3F(10,0,-10),Vector3F(10,0,0)})
near(circuit.Length,40); near(circuit:GetRoutePosition(5).z,-5); near(circuit:GetRoutePosition(-5).x,5)
near(circuit:distanceAt(2),20)
reject(function() circuit:SetWaypoints({Vector3F(0,0,0),Vector3F(0,0,0)}) end)
near(circuit.Length,40) -- failed edits must preserve the working route
local grid=SpawnpointContainer(nil); grid:generateGrid(circuit,4,6,2)
local p,rotation=grid:GetStartTransform(1); assert(p and rotation)
reject(function() grid:GetStartTransform(5) end)
local path=PathCreator(nil); path.circuit=circuit; assert(#path:sample(5)==8)

local gate=Checkpoint(nil); gate:configure(Vector3F(0,0,0),Vector3F(0,0,-1),2,2)
assert(gate:crossed(Vector3F(0,0,1),Vector3F(0,0,-1)))
assert(not gate:crossed(Vector3F(0,0,-1),Vector3F(0,0,1)))
assert(not gate:crossed(Vector3F(3,0,1),Vector3F(3,0,-1)))
assert(not gate:crossed(Vector3F(0,0,101),Vector3F(0,0,-1)))
local gate2=Checkpoint(nil); gate2:configure(Vector3F(0,0,-10),Vector3F(0,0,-1),2,2)
local checkpoints=CheckpointContainer(nil); checkpoints:setCheckpoints({gate,gate2})
assert(checkpoints:sample("a",Vector3F(0,0,1))==nil)
assert(checkpoints:sample("a",Vector3F(0,0,-1))=="checkpoint")
assert(checkpoints:sample("a",Vector3F(0,0,-11))=="lap")
checkpoints:sample("b",Vector3F(0,0,-9)); assert(checkpoints:sample("b",Vector3F(0,0,-11))==nil)

local car={setControls=function(self,t,b,s) self.controls={t,b,s} end,usePlayerControls=function(self) self.player=true end,getSteering=function() return 0.5 end}
local body={getLinearVelocity=function() return Vector3F(0,0,0) end,getMass=function() return 2 end,
    addForce=function(self,v) self.force=v end,addTorque=function(self,v) self.torque=v end}
local native=RacingCarController(nil); native:bind(car,actor()); native:setControls(1,0,0.5)
reject(function() native:setControls(2,0,0) end); native:shutdown(); assert(car.controls[2]==1)
local ai=OpponentControl(nil); local racer=actor()
local route={nearest=function() return 0 end,distanceAt=function() return 0 end,
    GetRoutePoint=function() return {position=Vector3F(10,0,-10),direction=Vector3F(0,0,-1)} end}
ai:bind(car,racer,body,route); ai:enable(true); ai:update(0.1)
assert(car.controls[1]>0 and car.controls[3]>0,"AI must steer right for a +X target with -Z forward")
local brake=Brakezone(nil); brake:configure(Vector3F(0,0,0),5,0); ai.zones={brake}; ai:update(0.1); assert(car.controls[1]==0)
ai:shutdown(); assert(car.controls[2]==1)
local mobile=MobileControlManager(nil); mobile:bind(car); mobile:enable(true)
mobile:setTouch(1,"throttle",1); mobile:setTouch(2,"steering",-0.5); assert(car.controls[1]==1 and car.controls[3]==-0.5)
mobile:release(1); assert(car.controls[1]==0); mobile:cancel(); assert(car.controls[2]==1)
local drone=RacingDroneController(nil); drone:bind(body,racer); drone:setControls(1,0,0,0)
drone:physicsUpdate(); assert(body.force==nil); drone:arm(true); drone:physicsUpdate(); near(body.force.y,40)
local bike=RacingMotorbikeController(nil); bike:bind(car,racer); bike:bindRider(actor(),{getLinearVelocity=function() return Vector3F(0,0,-10) end}); bike:update(0.1); assert(bike.lean<0); bike:shutdown()

local replay=ReplayManager(nil); replay:append(0,Vector3F(0,0,0),Quaternion()); replay:append(2,Vector3F(10,0,0),Quaternion(-1,0,0,0))
p,rotation=replay:sample(1); near(p.x,5); near(rotation:W(),1)
reject(function() replay:append(1,Vector3F(0,0,0),Quaternion()) end)
local visual=actor(); replay:play(visual,false); replay:update(1); near(visual.position.x,5)
replay:pause(); replay:update(10); near(visual.position.x,5); replay:resume(); replay:update(5); assert(replay.state=="complete"); near(visual.position.x,10)
local ghost=GhostVehicle(nil); ghost:bind(visual,replay.frames); ghost:start(true); ghost:update(3); near(visual.position.x,5); ghost:shutdown(); assert(not visual.enabled)
local badGhost=actor(); badGhost.getComponent=function() return {} end
reject(function() GhostVehicle(nil):bind(badGhost,replay.frames) end)
local marks=Skidmark(nil); marks.capacity=2
marks:add(Vector3F(0,0,0),1,true)
for i=1,4 do marks:add(Vector3F(i,0,0),1,true) end
assert(marks.count==2 and #marks.segments==2); marks:add(Vector3F(5,0,0),0,false); assert(marks.previous==nil)

local follow=PlayerFPCamera(nil); follow:bind(visual,racer); follow:update(0.1); near(visual.position.y,1)
local minimap=MinimapFollowTarget(nil); minimap:bind(visual,racer); minimap:update(0.1); near(visual.position.y,40)
local arrow=WaypointArrow(nil); arrow:bind(visual,racer); arrow:point(Vector3F(10,40,-10)); assert(visual.rotation:Y()<0)
local c1,c2={setActive=function(self,v) self.active=v end},{setActive=function(self,v) self.active=v end}
local switch=CameraSwitcher(nil); switch:bind({c1,c2}); switch:next(); assert(c2.active and not c1.active)
local text=label(); local fade=TextAlpha(nil); fade:bind(text); fade:fade(0,1); fade:update(0.5); near(fade.alpha,0.5)
local fps=FramerateCounter(nil); fps:bind(text); for _=1,5 do fps:update(0.1) end; near(fps.fps,10)
local clicks=0; local button=UIButton(nil); button:bind({},function() clicks=clicks+1 end)
assert(button:activate() and not button:activate()); assert(clicks==0); button:update(); assert(clicks==1)

local function view(id,completion,time,finished)
    local stats,progress=Statistics(nil),ProgressTracker(nil)
    stats.m_TotalTimeCounter,stats.finishedRace=time or 0,finished or false
    progress.m_RaceCompletion=completion
    local v=RaceView(nil); v:bind(id,actor(),stats,progress); return v
end
local v1,v2,v3=view(1,0.9,0,false),view(2,0.1,50,true),view(3,0.5,40,true)
local ranks=RankManager(nil); ranks:register(v1); ranks:register(v2); ranks:register(v3)
local rows=ranks:refresh(); assert(rows[1].id==3 and rows[2].id==2 and rows[3].id==1)
v2.statistics.finishedRace,v3.statistics.finishedRace=false,false; v1.progress.m_RaceCompletion,v2.progress.m_RaceCompletion=0.5,0.5
rows=ranks:refresh(); assert(rows[1].id==1 and rows[2].id==2 and rows[3].id==3,"Ties use racer ID")
local snapshot=v1:ToData(); snapshot.rank=2; v1:FromData(snapshot); assert(v1.m_Rank==2)
snapshot.rank=0; reject(function() v1:FromData(snapshot) end); assert(v1.m_Rank==2)
local sessionManager=RaceManager(nil); local managerView=RaceManagerView(nil); managerView:bind(sessionManager,ranks,nil)
managerView:JoinRace(v1); managerView:JoinRace(v2); assert(sessionManager.totalRacers==2); managerView:LeaveRace(v2); assert(sessionManager.totalRacers==1)
reject(function() managerView:receive(v1:ToData()) end)
local standings=RaceStandingsUI(nil); assert(standings:format(ranks:refresh()):find("Racer 1",1,true))

local loader=DataLoader(nil); local encoded=loader:serialize({text="a=\nb%\0",boolean=false,number=12.25})
local decoded=loader:deserialize(encoded); assert(decoded.text=="a=\nb%\0" and decoded.boolean==false and decoded.number==12.25)
reject(function() loader:deserialize("WORKPHONE_RACING_DATA=1\nx=n:1\nx=n:2\n") end)
reject(function() loader:serialize({x=0/0}) end)
local temporary=os.tmpname(); os.remove(temporary)
assert(loader:save(temporary,{value=1}))
local rename=os.rename
os.rename=function(from,to) if from==temporary..".tmp" then return nil,"replacement failed" end; return rename(from,to) end
assert(not loader:save(temporary,{value=2})); assert(loader:load(temporary).value==1,"Failed replacement must restore original")
os.rename=rename; os.remove(temporary)
local player=PlayerData(nil); player:open(temporary)
local rewards=RaceRewards(nil); rewards:bind(player)
assert(rewards:claim("run_1",1,2,true)); assert(player.profile.credits==125)
assert(not rewards:claim("run_1",1,2,true)); assert(player.profile.credits==125)
local upgrades=VehicleUpgrades(nil); local applied
upgrades:bind(player,{engine={maxLevel=2,baseCost=50}},{engine=function(level) applied=level end})
assert(upgrades:purchase("engine")); upgrades:apply(); assert(applied==1 and player.profile.credits==75)
assert(not upgrades:purchase("engine")); assert(player.profile.credits==75)
local restored=PlayerData(nil); restored:open(temporary); assert(restored.profile.credits==75 and restored.profile.claims.run_1 and restored.profile.upgrades.engine==1)
local save=player.saveProfile; player.saveProfile=function() return false,"disk full" end
assert(not rewards:claim("run_2",1,1,true)); assert(player.profile.credits==75 and not player.profile.claims.run_2)
player.saveProfile=save
os.remove(temporary); os.remove(temporary..".profile"); os.remove(temporary..".profile.bak"); os.remove(temporary..".profile.tmp")

local audio={setLoop=function(self,v) self.loop=v end,setVolume=function(self,v) self.volume=v end,
    stop=function(self) self.playing=false end,play=function(self) self.playing=true end,setPosition=function(self,v) self.position=v end}
local sounds=SoundManager(nil); sounds:register("music",audio); assert(sounds:playMusic("music")); sounds:mute(true); assert(audio.volume==0)
sounds:shutdown(); assert(not audio.playing)

-- IPacket wire-size double: no invented send/subscribe APIs on native objects.
local function packet()
    local value={fields={},cursor=0,length=0,address="trusted"}
    for name,size in pairs({UInt8=1,UInt16=2,UInt32=4,Int32=4,Float=4,Bool=1}) do
        value["write"..name]=function(self,item) self.fields[#self.fields+1]=item; self.length=self.length+size end
        value["read"..name]=function(self) self.cursor=self.cursor+1; return assert(self.fields[self.cursor]~=nil and self.fields[self.cursor],"Truncated packet") end
    end
    -- false is a valid native boolean read.
    function value:readBool() self.cursor=self.cursor+1; return self.fields[self.cursor] end
    function value:getDataLength() return self.length end
    function value:resetReadPointer() self.cursor=0 end
    function value:getSystemAddress() return self.address end
    return value
end
local network={createPacket=function() return packet() end,sendPacketUnreliable=function(self,p) self.sent=p end,sendPacket=function(self,p) self.sent=p end}
local sender,receiver=RacingNetworkTransport(nil),RacingNetworkTransport(nil)
sender:bind(network,1,function() return true end); receiver:bind(network,1,function(address) return address=="trusted" end)
local received=0; receiver:subscribe(v1.id,function(data) v1:FromData(data); received=received+1 end)
sender:sendRacer(v1:ToData()); assert(network.sent.length==39 and receiver:receive(network.sent)); assert(received==1)
assert(not receiver:receive(network.sent)); assert(received==1,"Stale packet must not apply")
sender:sendRacer(v1:ToData()); network.sent.address="intruder"; assert(not receiver:receive(network.sent)); assert(received==1)
network.sent.address="trusted"; network.sent.fields[11]=0/0; assert(not receiver:receive(network.sent)); assert(received==1)
sender:sendRacer(v1:ToData()); network.sent.fields[5]=2; assert(not receiver:receive(network.sent)); assert(received==1)
managerView.manager:InitializeRace("race",3)
local client=RaceManagerView(nil); client:bind(RaceManager(nil),nil,nil); client.isHost=false
receiver:subscribeSession(0,function(data) client:receiveSession(data) end)
sender:sendSession(managerView:ToData()); assert(network.sent.length==43 and receiver:receive(network.sent))
assert(client.manager.session.state=="countdown" and client.manager.totalLaps==3)
managerView.manager:PauseRace(); sender:sendSession(managerView:ToData()); assert(receiver:receive(network.sent))
assert(client.manager.session.state=="paused" and client.manager.session.resumeState=="countdown")
receiver:shutdown(); assert(next(receiver.handlers)==nil)

local damagedUI=RaceUI(nil); local root=actor(); root.addChild=function() end; root.setName=function() end; root.addComponent=function() error("component unavailable") end
local cleaned=0
reject(function() damagedUI:generate({createActor=function() return root end,destroyActor=function(_,item) assert(item==root); cleaned=cleaned+1 end},actor()) end)
-- Owner's parenting API is required before layout creation.
assert(damagedUI.root==nil)
local owner=actor(); owner.addChild=function() end
reject(function() damagedUI:generate({createActor=function() return root end,destroyActor=function(_,item) assert(item==root); cleaned=cleaned+1 end},owner) end)
assert(cleaned==2 and damagedUI.root==nil,"Partial HUD must be destroyed")
local failingGame=RacingGameFull({getActor=function() return owner end}); local attempts=0
failingGame.initializeGame=function() attempts=attempts+1; error("missing UI binding") end
app.playing=true; failingGame:update(); failingGame:update(); assert(attempts==1,"Failed startup must not retry every frame")
app.playing=false
print("Racing component directory, physics interfaces, AI, checkpoints, ranking, replay, UI, data and networking: PASS ("..#files.." scripts)")
