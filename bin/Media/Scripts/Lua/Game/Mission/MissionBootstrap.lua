include("BaseComponent.lua")
include("MissionManager.lua")
include("MissionHUD.lua")

class 'MissionBootstrap' (BaseComponent)

function MissionBootstrap:__init(component)
	BaseComponent.__init(self, component)
	self.missionManager = nil
	self.missionHUD = nil
	self.startingMissionId = ""
	self.autoStart = false
	self.registerBuiltIns = true
	self.started = false
end

function MissionBootstrap:setMissionManager(value) self.missionManager = value end
function MissionBootstrap:setMissionHUD(value) self.missionHUD = value end

function MissionBootstrap:start()
	if self.started then return true end
	local manager = self.missionManager or MissionManager.instance()
	if not manager then return false end
	self.missionManager = manager
	manager:start()
	if self.registerBuiltIns and #manager.missionOrder == 0 then manager:registerBuiltInMissions() end
	if self.missionHUD then
		self.missionHUD:setMissionManager(manager)
		self.missionHUD:start()
	end
	if self.autoStart and self.startingMissionId ~= "" then manager:startMission(self.startingMissionId) end
	self.started = true
	return true
end

function MissionBootstrap:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("startingMissionId", self.startingMissionId)
	properties:setPropertyAsBool("autoStart", self.autoStart)
	properties:setPropertyAsBool("registerBuiltIns", self.registerBuiltIns)
	properties:setButtonPressed("Start", false)
end

function MissionBootstrap:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("startingMissionId") then self.startingMissionId = properties:getPropertyAsString("startingMissionId") end
	if properties:hasProperty("autoStart") then self.autoStart = properties:getPropertyAsBool("autoStart") end
	if properties:hasProperty("registerBuiltIns") then self.registerBuiltIns = properties:getPropertyAsBool("registerBuiltIns") end
	if properties:isButtonPressed("Start") then self:start() end
end

MissionBootstrap.Start = MissionBootstrap.start
MissionBootstrap.SetMissionManager = MissionBootstrap.setMissionManager
MissionBootstrap.SetMissionHUD = MissionBootstrap.setMissionHUD
