include("MissionDefinition.lua")

class 'MissionAuthoring'

function MissionAuthoring:__init()
	self.selectedMission = nil
	self.selectedObjectiveIndex = -1
	self.lastErrors = {}
	self.lastWarnings = {}
end

function MissionAuthoring:createMission(values)
	self.selectedMission = MissionDefinition(values or {})
	self.selectedObjectiveIndex = -1
	return self.selectedMission
end

function MissionAuthoring:selectMission(mission)
	self.selectedMission = mission and mission.validate and mission or MissionDefinition(mission or {})
	self.selectedObjectiveIndex = -1
	return self.selectedMission
end

function MissionAuthoring:addObjective(values, index)
	if not self.selectedMission then return nil end
	local objective = self.selectedMission:addObjective(values or {}, index)
	self.selectedObjectiveIndex = self.selectedMission:getObjectiveIndex(objective.id)
	return objective
end

function MissionAuthoring:removeObjective(index)
	if not self.selectedMission then return nil end
	index = math.floor(tonumber(index) or self.selectedObjectiveIndex)
	local objective = self.selectedMission.objectives[index + 1]
	if not objective then return nil end
	local removed = self.selectedMission:removeObjective(objective.id)
	self.selectedObjectiveIndex = math.min(index, #self.selectedMission.objectives - 1)
	return removed
end

function MissionAuthoring:moveObjective(fromIndex, toIndex)
	if not self.selectedMission then return false end
	fromIndex, toIndex = math.floor(tonumber(fromIndex) or -1), math.floor(tonumber(toIndex) or -1)
	if fromIndex < 0 or fromIndex >= #self.selectedMission.objectives then return false end
	if toIndex < 0 or toIndex >= #self.selectedMission.objectives then return false end
	local objective = table.remove(self.selectedMission.objectives, fromIndex + 1)
	table.insert(self.selectedMission.objectives, toIndex + 1, objective)
	self.selectedObjectiveIndex = toIndex
	return true
end

function MissionAuthoring:createObjectiveMarker(index)
	if not self.selectedMission then return nil end
	index = math.floor(tonumber(index) or self.selectedObjectiveIndex)
	local objective = self.selectedMission.objectives[index + 1]
	if not objective then return nil end
	return {
		name = "MissionObjective_" .. tostring(index),
		missionId = self.selectedMission.missionId,
		objectiveId = objective.id,
		objectiveIndex = index,
		position = MissionRuntime.vectorToTable(objective.worldPosition),
		radius = objective.completionRadius
	}
end

function MissionAuthoring:validate()
	if not self.selectedMission then
		self.lastErrors, self.lastWarnings = { "no mission selected" }, {}
		return false, self.lastErrors, self.lastWarnings
	end
	local valid, errors, warnings = self.selectedMission:validate()
	self.lastErrors, self.lastWarnings = errors, warnings
	return valid, errors, warnings
end

function MissionAuthoring:exportDefinition()
	local valid = self:validate()
	if not valid then return nil, self.lastErrors end
	return self.selectedMission:toTable()
end

function MissionAuthoring:importDefinition(values)
	return self:selectMission(MissionDefinition.fromTable(values or {}))
end

MissionAuthoring.CreateMission = MissionAuthoring.createMission
MissionAuthoring.SelectMission = MissionAuthoring.selectMission
MissionAuthoring.AddObjective = MissionAuthoring.addObjective
MissionAuthoring.RemoveObjective = MissionAuthoring.removeObjective
MissionAuthoring.MoveObjective = MissionAuthoring.moveObjective
MissionAuthoring.CreateObjectiveMarker = MissionAuthoring.createObjectiveMarker
MissionAuthoring.Validate = MissionAuthoring.validate
MissionAuthoring.ExportDefinition = MissionAuthoring.exportDefinition
MissionAuthoring.ImportDefinition = MissionAuthoring.importDefinition
