include("BaseComponent.lua")
include("MissionManager.lua")

class 'MissionObjectiveSceneMarker' (BaseComponent)

function MissionObjectiveSceneMarker:__init(component)
	BaseComponent.__init(self, component)
	self.mission = nil
	self.missionId = ""
	self.objectiveId = ""
	self.objectiveIndex = 0
	self.missionManager = nil
	self.markerVisual = nil
	self.authoringMode = false
	self.visibleOnlyWhenActive = true
	self.lastError = ""
end

function MissionObjectiveSceneMarker:setMission(value) self.mission = value end
function MissionObjectiveSceneMarker:setMissionManager(value) self.missionManager = value end
function MissionObjectiveSceneMarker:setMarkerVisual(value) self.markerVisual = value end

function MissionObjectiveSceneMarker:getMissionManager() return self.missionManager or MissionManager.instance() end

function MissionObjectiveSceneMarker:resolveMission()
	if self.mission then return self.mission end
	local manager = self:getMissionManager()
	if manager and self.missionId ~= "" then self.mission = manager:getMission(self.missionId) end
	return self.mission
end

function MissionObjectiveSceneMarker:getObjective()
	local mission = self:resolveMission()
	if not mission then return nil end
	if self.objectiveId ~= "" then return mission:getObjective(self.objectiveId) end
	return mission.objectives[self.objectiveIndex + 1]
end

function MissionObjectiveSceneMarker:getPosition()
	local _, actor = MissionRuntime.tryCall(self.component, { "getActor", "GetActor" })
	local position = MissionRuntime.read(actor, { "getPosition", "GetPosition" }, { "position" }, nil)
	if position then return position end
	local _, transform = MissionRuntime.tryCall(actor, { "getTransform", "GetTransform" })
	return MissionRuntime.read(transform, { "getPosition", "GetPosition" }, { "position" }, nil)
end

function MissionObjectiveSceneMarker:syncToMission()
	local objective = self:getObjective()
	local position = self:getPosition()
	if not objective or not position then return false end
	objective.worldPosition = position
	return true
end

function MissionObjectiveSceneMarker:updateVisibility()
	if not self.visibleOnlyWhenActive then return true end
	local manager = self:getMissionManager()
	local current = manager and manager.activeMission and manager.activeMission:getCurrentObjective() or nil
	local objective = self:getObjective()
	local visible = current ~= nil and current == objective
	local target = self.markerVisual
	if target then MissionRuntime.tryCall(target, { "setEnabled", "SetEnabled", "setVisible", "SetVisible" }, visible) end
	return visible
end

function MissionObjectiveSceneMarker:update()
	if self.authoringMode then self:syncToMission() end
	self:updateVisibility()
end

function MissionObjectiveSceneMarker:activate()
	local objective = self:getObjective()
	local manager = self:getMissionManager()
	return objective and manager and manager:completeObjective(objective.id) or false
end

function MissionObjectiveSceneMarker:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("missionId", self.missionId)
	properties:setPropertyAsString("objectiveId", self.objectiveId)
	properties:setPropertyAsInt("objectiveIndex", self.objectiveIndex)
	properties:setPropertyAsBool("authoringMode", self.authoringMode)
	properties:setPropertyAsBool("visibleOnlyWhenActive", self.visibleOnlyWhenActive)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("SyncToMission", false)
end

function MissionObjectiveSceneMarker:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("missionId") then self.missionId = properties:getPropertyAsString("missionId"); self.mission = nil end
	if properties:hasProperty("objectiveId") then self.objectiveId = properties:getPropertyAsString("objectiveId") end
	if properties:hasProperty("objectiveIndex") then self.objectiveIndex = math.max(0, properties:getPropertyAsInt("objectiveIndex")) end
	if properties:hasProperty("authoringMode") then self.authoringMode = properties:getPropertyAsBool("authoringMode") end
	if properties:hasProperty("visibleOnlyWhenActive") then self.visibleOnlyWhenActive = properties:getPropertyAsBool("visibleOnlyWhenActive") end
	if properties:isButtonPressed("SyncToMission") then self:syncToMission() end
end

MissionObjectiveSceneMarker.Update = MissionObjectiveSceneMarker.update
MissionObjectiveSceneMarker.SyncToMission = MissionObjectiveSceneMarker.syncToMission
MissionObjectiveSceneMarker.UpdateVisibility = MissionObjectiveSceneMarker.updateVisibility
MissionObjectiveSceneMarker.Activate = MissionObjectiveSceneMarker.activate
