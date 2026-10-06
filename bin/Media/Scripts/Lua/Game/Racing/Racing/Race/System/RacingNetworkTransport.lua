if not RacingSupport then include("RacingSupport.lua") end
if not RaceView then include("RaceView.lua") end
-- Uses only Workphone INetworkManager/IPacket. The native listener forwards full
-- application packets to receive(); authenticate addresses in the supplied policy.
class 'RacingNetworkTransport' (RacingComponent)
local MAGIC, VERSION, RACER, SESSION = 0x574E4554, 1, 64, 65
function RacingNetworkTransport:__init(component)
    BaseComponent.__init(self, component); self.outgoing, self.incoming, self.handlers, self.sessionHandlers = {}, {}, {}, {}
end
function RacingNetworkTransport:bind(manager, epoch, authorize)
    assert(manager and type(authorize) == "function", "Network manager and sender policy required")
    self.manager, self.epoch, self.authorize = manager, RacingSupport.integer(epoch, 1, 4294967295, "session epoch"), authorize
    self.outgoing, self.incoming = {}, {}
    self.handlers, self.sessionHandlers = {}, {}
end
function RacingNetworkTransport:subscribe(id, callback)
    RacingSupport.integer(id, 0, 2147483647, "view ID"); assert(type(callback) == "function", "Receiver required")
    assert(not self.handlers[id], "Receiver already registered")
    self.handlers[id] = callback
end
function RacingNetworkTransport:unsubscribe(id) self.handlers[id], self.incoming[id] = nil, nil end
function RacingNetworkTransport:unsubscribeSession(id) self.sessionHandlers[id], self.incoming["session:"..id] = nil, nil end
function RacingNetworkTransport:subscribeSession(id,callback)
    RacingSupport.integer(id,0,2147483647,"session view ID"); assert(type(callback)=="function" and not self.sessionHandlers[id],"Session receiver already registered or invalid")
    self.sessionHandlers[id]=callback
end
function RacingNetworkTransport.validateSession(data)
    local r=RacingSupport
    assert(type(data)=="table","Session data required")
    local value={id=r.integer(data.id,0,2147483647),state=r.integer(data.state,0,4),resume=r.integer(data.resume,0,2),
        laps=r.integer(data.laps,1,10),racers=r.integer(data.racers,1,64),elapsed=r.number(data.elapsed,0,86400),
        countdown=r.number(data.countdown,0,3),lap=r.integer(data.lap,1,10000),completed=r.integer(data.completed,0,9999),
        best=r.number(data.best,0,86400),last=r.number(data.last,0,86400)}
    assert(data.mode=="race" or data.mode=="timeTrial","Unknown race mode"); value.mode=data.mode
    assert(value.state~=3 or value.resume>0,"Paused session requires resume state")
    return value
end
function RacingNetworkTransport:sendSession(data)
    assert(self.manager,"Transport is unbound"); data=RacingNetworkTransport.validateSession(data)
    local key="session:"..data.id; local sequence=(self.outgoing[key] or 0)+1
    assert(sequence<=4294967295,"Sequence exhausted; start a new session epoch")
    local packet=assert(self.manager:createPacket(),"Packet allocation failed")
    packet:writeUInt32(MAGIC); packet:writeUInt8(VERSION); packet:writeUInt8(SESSION); packet:writeInt32(data.id)
    packet:writeUInt32(self.epoch); packet:writeUInt32(sequence)
    packet:writeUInt8(data.mode=="timeTrial" and 1 or 0); packet:writeUInt8(data.state); packet:writeUInt8(data.resume)
    packet:writeUInt8(data.laps); packet:writeUInt8(data.racers)
    packet:writeFloat(data.elapsed); packet:writeFloat(data.countdown)
    packet:writeUInt16(data.lap); packet:writeUInt16(data.completed); packet:writeFloat(data.best); packet:writeFloat(data.last)
    self.manager:sendPacket(packet); self.outgoing[key]=sequence
end
function RacingNetworkTransport:sendRacer(data)
    assert(self.manager, "Transport is unbound"); data = RaceView.validate(data)
    local sequence = (self.outgoing[data.id] or 0) + 1
    assert(sequence <= 4294967295, "Sequence exhausted; start a new session epoch")
    local packet = assert(self.manager:createPacket(), "Packet allocation failed")
    packet:writeUInt32(MAGIC); packet:writeUInt8(VERSION); packet:writeUInt8(RACER); packet:writeInt32(data.id)
    packet:writeUInt32(self.epoch); packet:writeUInt32(sequence)
    packet:writeUInt8(data.rank); packet:writeUInt8(data.startRank); packet:writeUInt16(data.lap)
    packet:writeFloat(data.total); packet:writeFloat(data.best); packet:writeFloat(data.progress); packet:writeFloat(data.completion)
    packet:writeBool(data.finished)
    self.manager:sendPacketUnreliable(packet)
    self.outgoing[data.id] = sequence
end
function RacingNetworkTransport:receive(packet)
    local ok, failure = pcall(function()
        local length=packet:getDataLength()
        assert(self.manager and (length==39 or length==43), "Invalid racing packet length")
        packet:resetReadPointer()
        assert(packet:readUInt32() == MAGIC and packet:readUInt8() == VERSION, "Unsupported packet")
        local kind=packet:readUInt8()
        assert((kind==RACER and length==39) or (kind==SESSION and length==43),"Invalid packet kind")
        local id, epoch, sequence = packet:readInt32(), packet:readUInt32(), packet:readUInt32()
        local key=kind==SESSION and "session:"..id or id
        local handler
        if kind==SESSION then handler=self.sessionHandlers[id] else handler=self.handlers[id] end
        assert(epoch == self.epoch and sequence > (self.incoming[key] or 0), "Stale snapshot")
        assert(handler and self.authorize(packet:getSystemAddress(), id, kind), "Unauthorized racing snapshot")
        local data
        if kind==RACER then
            data=RaceView.validate({id=id, rank=packet:readUInt8(), startRank=packet:readUInt8(), lap=packet:readUInt16(),
                total=packet:readFloat(), best=packet:readFloat(), progress=packet:readFloat(), completion=packet:readFloat(), finished=packet:readBool()})
        else
            local mode=packet:readUInt8(); assert(mode==0 or mode==1,"Invalid race mode")
            data=RacingNetworkTransport.validateSession({id=id,mode=mode==0 and "race" or "timeTrial",state=packet:readUInt8(),resume=packet:readUInt8(),
                laps=packet:readUInt8(),racers=packet:readUInt8(),elapsed=packet:readFloat(),countdown=packet:readFloat(),
                lap=packet:readUInt16(),completed=packet:readUInt16(),best=packet:readFloat(),last=packet:readFloat()})
        end
        handler(data)
        self.incoming[key] = sequence
    end)
    return ok, ok and nil or tostring(failure)
end
function RacingNetworkTransport:shutdown()
    self.manager, self.authorize, self.handlers, self.incoming, self.outgoing = nil, nil, {}, {}, {}
    self.sessionHandlers={}
end
