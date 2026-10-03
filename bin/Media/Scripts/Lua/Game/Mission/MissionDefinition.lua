include("MissionObjective.lua")

class 'MissionDefinition'

function MissionDefinition:__init(values)
	values = values or {}
	self.missionId = tostring(values.missionId or MissionRuntime.generateId("mission"))
	self.missionTitle = tostring(values.missionTitle or "Untitled Mission")
	self.missionDescription = tostring(values.missionDescription or "")
	self.startPosition = values.startPosition or MissionRuntime.makeVector3(0.0, 0.0, 0.0)
	self.startRadius = math.max(0.1, tonumber(values.startRadius) or 5.0)
	self.objectives = {}
	for _, objective in ipairs(MissionRuntime.toArray(values.objectives)) do
		table.insert(self.objectives, self:normaliseObjective(objective))
	end

	self.category = tostring(values.category or "General")
	self.difficulty = tostring(values.difficulty or "Normal")
	self.sceneName = tostring(values.sceneName or "")
	self.aircraftTypes = MissionRuntime.deepCopy(values.aircraftTypes or {})
	self.tags = MissionRuntime.deepCopy(values.tags or {})
	self.prerequisites = MissionRuntime.deepCopy(values.prerequisites or {})
	self.timeLimit = math.max(0.0, tonumber(values.timeLimit) or 0.0)
	self.rewardScore = math.floor(tonumber(values.rewardScore) or 0)
	self.repeatable = MissionRuntime.asBoolean(values.repeatable, true)
	self.version = math.max(1, math.floor(tonumber(values.version) or 1))
end

function MissionDefinition:normaliseObjective(value)
	if value and value.getTypeName and value.toTable then return value end
	return MissionObjective(value or {})
end

function MissionDefinition:getObjective(objectiveId)
	for _, objective in ipairs(self.objectives) do
		if objective.id == objectiveId then return objective end
	end
	return nil
end

function MissionDefinition:getObjectiveIndex(objectiveId)
	for index, objective in ipairs(self.objectives) do
		if objective.id == objectiveId then return index - 1 end
	end
	return -1
end

function MissionDefinition:addObjective(objective, index)
	objective = self:normaliseObjective(objective)
	if index then table.insert(self.objectives, math.max(1, math.min(#self.objectives + 1, math.floor(index) + 1)), objective)
	else table.insert(self.objectives, objective) end
	return objective
end

function MissionDefinition:removeObjective(objectiveId)
	for index, objective in ipairs(self.objectives) do
		if objective.id == objectiveId then return table.remove(self.objectives, index) end
	end
	return nil
end

function MissionDefinition:validate()
	local errors, warnings, ids = {}, {}, {}
	if self.missionId == "" then table.insert(errors, "mission id is empty") end
	if self.missionTitle == "" then table.insert(errors, "mission title is empty") end
	if #self.objectives == 0 then table.insert(errors, "mission has no objectives") end
	local requiredCount = 0
	for index, objective in ipairs(self.objectives) do
		if ids[objective.id] then table.insert(errors, "duplicate objective id " .. objective.id) end
		ids[objective.id] = true
		if not objective.optional then requiredCount = requiredCount + 1 end
		local valid, objectiveErrors, objectiveWarnings = objective:validate()
		if not valid then for _, message in ipairs(objectiveErrors) do table.insert(errors, "objective " .. index .. ": " .. message) end end
		for _, message in ipairs(objectiveWarnings) do table.insert(warnings, "objective " .. index .. ": " .. message) end
	end
	if requiredCount == 0 and #self.objectives > 0 then table.insert(warnings, "mission contains only optional objectives") end
	return #errors == 0, errors, warnings
end

function MissionDefinition:toTable()
	local objectives = {}
	for _, objective in ipairs(self.objectives) do table.insert(objectives, objective:toTable()) end
	return {
		missionId = self.missionId,
		missionTitle = self.missionTitle,
		missionDescription = self.missionDescription,
		startPosition = MissionRuntime.vectorToTable(self.startPosition),
		startRadius = self.startRadius,
		objectives = objectives,
		category = self.category,
		difficulty = self.difficulty,
		sceneName = self.sceneName,
		aircraftTypes = MissionRuntime.deepCopy(self.aircraftTypes),
		tags = MissionRuntime.deepCopy(self.tags),
		prerequisites = MissionRuntime.deepCopy(self.prerequisites),
		timeLimit = self.timeLimit,
		rewardScore = self.rewardScore,
		repeatable = self.repeatable,
		version = self.version
	}
end

function MissionDefinition.fromTable(values) return MissionDefinition(values or {}) end

MissionDefinition.GetObjective = MissionDefinition.getObjective
MissionDefinition.GetObjectiveIndex = MissionDefinition.getObjectiveIndex
MissionDefinition.AddObjective = MissionDefinition.addObjective
MissionDefinition.RemoveObjective = MissionDefinition.removeObjective
MissionDefinition.Validate = MissionDefinition.validate
MissionDefinition.ToTable = MissionDefinition.toTable
MissionDefinition.FromTable = MissionDefinition.fromTable
