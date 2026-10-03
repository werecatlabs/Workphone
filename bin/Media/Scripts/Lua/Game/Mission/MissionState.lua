include("MissionDefinition.lua")

class 'MissionState'

local function normaliseIdSet(values)
	local result = {}
	for key, value in pairs(type(values) == "table" and values or {}) do
		if type(key) == "number" then
			result[tostring(value)] = true
		elseif MissionRuntime.asBoolean(value, false) then
			result[tostring(key)] = true
		end
	end
	return result
end

function MissionState:__init(definition)
	self.definition = definition
	self.currentObjectiveIndex = 0
	self.completedObjectiveIds = {}
	self.failedObjectiveIds = {}
	self.objectiveProgress = {}
	self.objectiveStartedAt = {}
	self.startedAt = MissionRuntime.currentTime()
	self.endedAt = nil
	self.score = 0
	self.status = "active"
	self.failureReason = ""
end

function MissionState:getCurrentObjective()
	if not self.definition then return nil end
	local index = self.currentObjectiveIndex + 1
	if index < 1 or index > #self.definition.objectives then return nil end
	return self.definition.objectives[index]
end

function MissionState:isComplete()
	if not self.definition then return true end
	for _, objective in ipairs(self.definition.objectives) do
		if not objective.optional and not self.completedObjectiveIds[objective.id] then return false end
	end
	return true
end

function MissionState:isTerminal()
	return self.status == "complete" or self.status == "failed" or self.status == "aborted"
end

function MissionState:getProgress(objectiveId)
	return tonumber(self.objectiveProgress[objectiveId]) or 0.0
end

function MissionState:setProgress(objectiveId, value)
	self.objectiveProgress[objectiveId] = math.max(0.0, tonumber(value) or 0.0)
	return self.objectiveProgress[objectiveId]
end

function MissionState:addProgress(objectiveId, amount)
	return self:setProgress(objectiveId, self:getProgress(objectiveId) + (tonumber(amount) or 1.0))
end

function MissionState:getObjectiveElapsed(objective, now)
	if not objective then return 0.0 end
	local started = self.objectiveStartedAt[objective.id] or self.startedAt
	return math.max(0.0, (now or MissionRuntime.currentTime()) - started)
end

function MissionState:markObjectiveStarted(objective, now)
	if objective and not self.objectiveStartedAt[objective.id] then
		self.objectiveStartedAt[objective.id] = now or MissionRuntime.currentTime()
	end
end

function MissionState:completeCurrentObjective(now)
	local objective = self:getCurrentObjective()
	if not objective then return nil end
	self.completedObjectiveIds[objective.id] = true
	self.objectiveProgress[objective.id] = math.max(self:getProgress(objective.id), objective.requiredAmount)
	self.score = self.score + objective.score
	self.currentObjectiveIndex = self.currentObjectiveIndex + 1
	if self:isComplete() then
		self.status = "complete"
		self.endedAt = now or MissionRuntime.currentTime()
	end
	return objective
end

function MissionState:skipCurrentObjective(now)
	local objective = self:getCurrentObjective()
	if not objective or not objective.optional then return false end
	self.currentObjectiveIndex = self.currentObjectiveIndex + 1
	if self:isComplete() then self.status = "complete"; self.endedAt = now or MissionRuntime.currentTime() end
	return true
end

function MissionState:fail(reason, now)
	self.status = "failed"
	self.failureReason = tostring(reason or "Mission failed")
	self.endedAt = now or MissionRuntime.currentTime()
	local objective = self:getCurrentObjective()
	if objective then self.failedObjectiveIds[objective.id] = true end
	return true
end

function MissionState:getCompletionRatio()
	if not self.definition or #self.definition.objectives == 0 then return 1.0 end
	local required, complete = 0, 0
	for _, objective in ipairs(self.definition.objectives) do
		if not objective.optional then
			required = required + 1
			if self.completedObjectiveIds[objective.id] then complete = complete + 1 end
		end
	end
	return required == 0 and 1.0 or complete / required
end

function MissionState:toTable(now)
	now = tonumber(now) or MissionRuntime.currentTime()
	local referenceTime = self.endedAt or now
	local objectiveElapsed = {}
	for objectiveId, startedAt in pairs(self.objectiveStartedAt) do
		objectiveElapsed[objectiveId] = math.max(0.0, referenceTime - (tonumber(startedAt) or referenceTime))
	end
	return {
		missionId = self.definition and self.definition.missionId or "",
		currentObjectiveIndex = self.currentObjectiveIndex,
		completedObjectiveIds = MissionRuntime.deepCopy(self.completedObjectiveIds),
		failedObjectiveIds = MissionRuntime.deepCopy(self.failedObjectiveIds),
		objectiveProgress = MissionRuntime.deepCopy(self.objectiveProgress),
		objectiveStartedAt = MissionRuntime.deepCopy(self.objectiveStartedAt),
		objectiveElapsed = objectiveElapsed,
		startedAt = self.startedAt,
		elapsedTime = math.max(0.0, referenceTime - self.startedAt),
		endedAt = self.endedAt,
		score = self.score,
		status = self.status,
		failureReason = self.failureReason
	}
end

function MissionState:restore(values, now)
	values = values or {}
	now = tonumber(now) or MissionRuntime.currentTime()
	local maximumIndex = self.definition and #self.definition.objectives or 0
	self.currentObjectiveIndex = math.min(maximumIndex, math.max(0, math.floor(tonumber(values.currentObjectiveIndex) or 0)))
	self.completedObjectiveIds = normaliseIdSet(values.completedObjectiveIds)
	self.failedObjectiveIds = normaliseIdSet(values.failedObjectiveIds)
	self.objectiveProgress = MissionRuntime.deepCopy(values.objectiveProgress or {})
	self.objectiveStartedAt = {}
	if type(values.objectiveElapsed) == "table" then
		for objectiveId, elapsed in pairs(values.objectiveElapsed) do
			self.objectiveStartedAt[tostring(objectiveId)] = now - math.max(0.0, tonumber(elapsed) or 0.0)
		end
	else
		self.objectiveStartedAt = MissionRuntime.deepCopy(values.objectiveStartedAt or {})
	end
	if values.elapsedTime ~= nil then
		self.startedAt = now - math.max(0.0, tonumber(values.elapsedTime) or 0.0)
	else
		self.startedAt = tonumber(values.startedAt) or now
	end
	self.endedAt = tonumber(values.endedAt)
	self.score = math.floor(tonumber(values.score) or 0)
	self.status = tostring(values.status or "active")
	if self.status ~= "active" and self.status ~= "complete" and self.status ~= "failed" and self.status ~= "aborted" then
		self.status = "active"
	end
	self.failureReason = tostring(values.failureReason or "")
	if self.status == "active" then
		for index, objective in ipairs(self.definition and self.definition.objectives or {}) do
			if not objective.optional and not self.completedObjectiveIds[objective.id] and index - 1 < self.currentObjectiveIndex then
				self.currentObjectiveIndex = index - 1
				break
			end
		end
	end
	if self.status == "active" and self:isComplete() then
		self.status = "complete"
		self.endedAt = now
	elseif self.status ~= "active" and values.elapsedTime ~= nil then
		self.endedAt = self.startedAt + math.max(0.0, tonumber(values.elapsedTime) or 0.0)
	end
	return true
end

MissionState.CurrentObjective = MissionState.getCurrentObjective
MissionState.GetCurrentObjective = MissionState.getCurrentObjective
MissionState.IsComplete = MissionState.isComplete
MissionState.IsTerminal = MissionState.isTerminal
MissionState.CompleteCurrentObjective = MissionState.completeCurrentObjective
MissionState.SkipCurrentObjective = MissionState.skipCurrentObjective
MissionState.GetProgress = MissionState.getProgress
MissionState.SetProgress = MissionState.setProgress
MissionState.AddProgress = MissionState.addProgress
MissionState.GetCompletionRatio = MissionState.getCompletionRatio
MissionState.ToTable = MissionState.toTable
MissionState.Restore = MissionState.restore
