if not RacingSupport then include("RacingSupport.lua") end
class 'RaceRewards' (RacingComponent)
function RaceRewards:__init(component) BaseComponent.__init(self,component); self.baseReward=100 end
function RaceRewards:bind(playerData) self.playerData=assert(playerData) end
function RaceRewards:claim(runId, rank, racers, completed)
    assert(type(runId)=="string" and runId:match("^[%w_-]+$") and #runId<=64,"Invalid run ID")
    RacingSupport.integer(racers,1,64,"racers"); RacingSupport.integer(rank,1,racers,"finish rank")
    if not completed then return false,"Race is incomplete" end
    local profile=assert(self.playerData,"Player data required").profile
    if profile.claims[runId] then return false,"Reward already claimed" end
    local count=0; for _ in pairs(profile.claims) do count=count+1 end
    if count>=2048 then return false,"Reward history is full" end
    local reward=self.baseReward+(racers-rank)*25
    local previous=profile.credits
    profile.credits,profile.claims[runId]=previous+reward,true
    local ok,failure=self.playerData:saveProfile()
    if not ok then profile.credits,profile.claims[runId]=previous,nil; return false,failure end
    return true,reward
end
