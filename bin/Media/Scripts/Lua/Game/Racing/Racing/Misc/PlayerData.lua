if not RacingSupport then include("RacingSupport.lua") end
if not RacingRecords then include("RacingRecords.lua") end
if not DataLoader then include("DataLoader.lua") end
class 'PlayerData' (RacingComponent)
function PlayerData:__init(component)
    BaseComponent.__init(self, component); self.loader=DataLoader(component)
    self.profile={credits=0,upgrades={},claims={}}
end
function PlayerData:open(path)
    self.records = RacingRecords.new(path)
    self.records:load()
    self.profilePath=path..".profile"
    local data,failure=self.loader:load(self.profilePath)
    if data then
        local ok,result=pcall(self.decodeProfile,self,data)
        if ok then self.profile=result else self.profileError=tostring(result) end
    else
        local exists=io.open(self.profilePath,"rb")
        if exists then exists:close(); self.profileError=failure end
    end
    return self.records
end
function PlayerData:save() assert(self.records,"Player data is unopened"); return self.records:save() end
function PlayerData:decodeProfile(data)
    local profile={credits=RacingSupport.integer(data.credits or 0,0,1000000000,"credits"),upgrades={},claims={}}
    for key,value in pairs(data) do
        local upgrade=key:match("^upgrade:([%w_]+)$")
        local claim=key:match("^claim:([%w_-]+)$")
        if upgrade then profile.upgrades[upgrade]=RacingSupport.integer(value,0,100,"upgrade level")
        elseif claim then assert(value==true and #claim<=64,"Invalid reward claim"); profile.claims[claim]=true
        else assert(key=="credits","Unknown profile field") end
    end
    return profile
end
function PlayerData:saveProfile()
    if self.profileError then return false,"Profile could not be loaded: "..self.profileError end
    assert(self.profilePath,"Player data is unopened")
    local data={credits=self.profile.credits}
    for name,level in pairs(self.profile.upgrades) do data["upgrade:"..name]=level end
    for id,claimed in pairs(self.profile.claims) do data["claim:"..id]=claimed end
    local ok,failure=pcall(self.decodeProfile,self,data)
    if not ok then return false,tostring(failure) end
    return self.loader:save(self.profilePath,data)
end
