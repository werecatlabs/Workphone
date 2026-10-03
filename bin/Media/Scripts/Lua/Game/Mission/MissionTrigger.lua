include("BaseComponent.lua")
include("MissionManager.lua")

class 'MissionTrigger' (BaseComponent)

function MissionTrigger:__init(component)
	BaseComponent.__init(self, component)
	self.missionManager = nil
	self.eventName = "custom"
	self.targetId = ""
	self.amount = 1.0
	self.oneShot = true
	self.enabled = true
	self.requirePlayer = true
	self.playerTag = "Player"
	self.activationCount = 0
end

function MissionTrigger:setMissionManager(value) self.missionManager = value end
function MissionTrigger:getMissionManager() return self.missionManager or MissionManager.instance() end

function MissionTrigger:matchesActor(actor)
	if not self.requirePlayer then return true end
	if not actor then return false end
	local tag = MissionRuntime.read(actor, { "getTag", "GetTag" }, { "tag" }, "")
	if tostring(tag) == self.playerTag then return true end
	local name = MissionRuntime.read(actor, { "getName", "GetName" }, { "name" }, "")
	return string.lower(tostring(name)):find("player", 1, true) ~= nil
end

function MissionTrigger:activate(actor, data, ignoreActorRequirement)
	if not self.enabled or (self.oneShot and self.activationCount > 0) then return false end
	if self.requirePlayer and not ignoreActorRequirement and not self:matchesActor(actor) then return false end
	local manager = self:getMissionManager()
	if not manager then return false end
	local accepted = manager:reportEvent(self.eventName, self.targetId, self.amount, data or actor)
	if accepted then
		self.activationCount = self.activationCount + 1
		if self.oneShot then self.enabled = false end
	end
	return accepted
end

function MissionTrigger:onTriggerEnter(actor) return self:activate(actor) end
function MissionTrigger:onCollisionEnter(actor) return self:activate(actor) end

function MissionTrigger:handleEvent(parameters)
	if not parameters then return false end
	local actor = nil
	pcall(function() actor = parameters:at(3) end)
	return self:activate(actor, parameters)
end

function MissionTrigger:reset()
	self.activationCount = 0
	self.enabled = true
	return true
end

function MissionTrigger:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("eventName", self.eventName)
	properties:setPropertyAsString("targetId", self.targetId)
	properties:setPropertyAsFloat("amount", self.amount)
	properties:setPropertyAsBool("oneShot", self.oneShot)
	properties:setPropertyAsBool("enabled", self.enabled)
	properties:setPropertyAsBool("requirePlayer", self.requirePlayer)
	properties:setPropertyAsString("playerTag", self.playerTag)
	properties:setButtonPressed("Activate", false)
	properties:setButtonPressed("Reset", false)
end

function MissionTrigger:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("eventName") then self.eventName = properties:getPropertyAsString("eventName") end
	if properties:hasProperty("targetId") then self.targetId = properties:getPropertyAsString("targetId") end
	if properties:hasProperty("amount") then self.amount = properties:getPropertyAsFloat("amount") end
	if properties:hasProperty("oneShot") then self.oneShot = properties:getPropertyAsBool("oneShot") end
	if properties:hasProperty("enabled") then self.enabled = properties:getPropertyAsBool("enabled") end
	if properties:hasProperty("requirePlayer") then self.requirePlayer = properties:getPropertyAsBool("requirePlayer") end
	if properties:hasProperty("playerTag") then self.playerTag = properties:getPropertyAsString("playerTag") end
	if properties:isButtonPressed("Activate") then self:activate(nil, nil, true) end
	if properties:isButtonPressed("Reset") then self:reset() end
end

MissionTrigger.Activate = MissionTrigger.activate
MissionTrigger.OnTriggerEnter = MissionTrigger.onTriggerEnter
MissionTrigger.OnCollisionEnter = MissionTrigger.onCollisionEnter
MissionTrigger.HandleEvent = MissionTrigger.handleEvent
MissionTrigger.Reset = MissionTrigger.reset
