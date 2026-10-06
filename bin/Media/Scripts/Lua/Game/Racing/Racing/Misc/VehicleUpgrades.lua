if not RacingSupport then include("RacingSupport.lua") end
class 'VehicleUpgrades' (RacingComponent)
function VehicleUpgrades:__init(component) BaseComponent.__init(self,component); self.catalog,self.levels,self.appliers={},{},{} end
function VehicleUpgrades:bind(playerData, catalog, appliers)
    for name,upgrade in pairs(catalog) do
        assert(type(name)=="string" and name:match("^[%w_]+$") and type(appliers[name])=="function","Upgrade needs a native applier")
        RacingSupport.integer(upgrade.maxLevel,1,100,"maximum level"); RacingSupport.integer(upgrade.baseCost,1,1000000,"upgrade cost")
    end
    self.playerData,self.catalog,self.appliers=assert(playerData),catalog,appliers
    self.levels=playerData.profile.upgrades
end
function VehicleUpgrades:cost(name)
    local upgrade=assert(self.catalog[name],"Unknown upgrade"); local level=self.levels[name] or 0
    if level>=upgrade.maxLevel then return nil end
    return upgrade.baseCost*(level+1)
end
function VehicleUpgrades:purchase(name)
    local cost=self:cost(name)
    if not cost then return false,"Upgrade is at maximum level" end
    local profile=self.playerData.profile
    if profile.credits<cost then return false,"Not enough credits" end
    local oldCredits,oldLevel=profile.credits,self.levels[name] or 0
    profile.credits,self.levels[name]=oldCredits-cost,oldLevel+1
    local ok,failure=self.playerData:saveProfile()
    if not ok then profile.credits,self.levels[name]=oldCredits,oldLevel; return false,failure end
    -- Native tuning is applied at race setup, never while physics is running.
    return true
end
function VehicleUpgrades:apply()
    for name in pairs(self.catalog) do self.appliers[name](self.levels[name] or 0) end
end
function VehicleUpgrades:shutdown() self.playerData,self.appliers,self.catalog,self.levels=nil,{},{},{} end
