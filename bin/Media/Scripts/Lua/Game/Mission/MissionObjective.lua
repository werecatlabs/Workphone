include("MissionRuntime.lua")

MissionObjectiveType = MissionObjectiveType or {
	GoToLocation = 0,
	CollectItem = 1,
	KillTarget = 2,
	TalkToNPC = 3,
	Custom = 4,
	ReachAltitude = 5,
	MaintainAltitude = 6,
	ReachSpeed = 7,
	MaintainHeading = 8,
	PassGate = 9,
	TakeOff = 10,
	Land = 11,
	TouchdownZone = 12,
	FollowRoute = 13,
	AerobaticManeuver = 14,
	TimeTrial = 15,
	EmergencyLanding = 16
}

MissionObjectiveTypeNames = MissionObjectiveTypeNames or {}
for name, value in pairs(MissionObjectiveType) do MissionObjectiveTypeNames[value] = name end

class 'MissionObjective'

function MissionObjective:__init(values)
	values = values or {}
	self.id = tostring(values.id or MissionRuntime.generateId("objective"))
	self.title = tostring(values.title or "New Objective")
	self.description = tostring(values.description or "")
	self.type = values.type
	if type(self.type) == "string" then self.type = MissionObjectiveType[self.type] or MissionObjectiveType.Custom end
	self.type = math.floor(tonumber(self.type) or MissionObjectiveType.Custom)
	self.worldPosition = values.worldPosition or MissionRuntime.makeVector3(0.0, 0.0, 0.0)
	self.completionRadius = math.max(0.1, tonumber(values.completionRadius) or 3.0)
	self.targetId = tostring(values.targetId or "")
	self.requiredAmount = math.max(1, math.floor(tonumber(values.requiredAmount) or 1))
	self.optional = MissionRuntime.asBoolean(values.optional, false)

	-- Flight-sim extensions. Existing Unity fields retain their original names.
	self.targetValue = tonumber(values.targetValue)
	self.tolerance = math.max(0.0, tonumber(values.tolerance) or 0.0)
	self.duration = math.max(0.0, tonumber(values.duration) or 0.0)
	self.timeLimit = math.max(0.0, tonumber(values.timeLimit) or 0.0)
	self.score = math.floor(tonumber(values.score) or 100)
	self.failOnTimeout = MissionRuntime.asBoolean(values.failOnTimeout, false)
	self.metadata = MissionRuntime.deepCopy(values.metadata or {})
end

function MissionObjective:getTypeName()
	return MissionObjectiveTypeNames[self.type] or "Custom"
end

function MissionObjective:isEventDriven()
	return self.type == MissionObjectiveType.CollectItem or
		self.type == MissionObjectiveType.KillTarget or
		self.type == MissionObjectiveType.TalkToNPC or
		self.type == MissionObjectiveType.Custom or
		self.type == MissionObjectiveType.PassGate or
		self.type == MissionObjectiveType.AerobaticManeuver
end

function MissionObjective:validate()
	local errors, warnings = {}, {}
	if self.id == "" then table.insert(errors, "objective id is empty") end
	if self.title == "" then table.insert(errors, "objective title is empty") end
	if not MissionObjectiveTypeNames[self.type] then table.insert(errors, "unknown objective type " .. tostring(self.type)) end
	if self.completionRadius <= 0.0 then table.insert(errors, "completion radius must be positive") end
	if self.requiredAmount < 1 then table.insert(errors, "required amount must be at least one") end
	if self:isEventDriven() and self.targetId == "" then table.insert(warnings, self.title .. " has no target id") end
	if (self.type == MissionObjectiveType.MaintainAltitude or self.type == MissionObjectiveType.MaintainHeading) and self.duration <= 0.0 then
		table.insert(warnings, self.title .. " has no hold duration; it will complete immediately")
	end
	return #errors == 0, errors, warnings
end

function MissionObjective:toTable()
	return {
		id = self.id,
		title = self.title,
		description = self.description,
		type = self.type,
		worldPosition = MissionRuntime.vectorToTable(self.worldPosition),
		completionRadius = self.completionRadius,
		targetId = self.targetId,
		requiredAmount = self.requiredAmount,
		optional = self.optional,
		targetValue = self.targetValue,
		tolerance = self.tolerance,
		duration = self.duration,
		timeLimit = self.timeLimit,
		score = self.score,
		failOnTimeout = self.failOnTimeout,
		metadata = MissionRuntime.deepCopy(self.metadata)
	}
end

MissionObjective.GetTypeName = MissionObjective.getTypeName
MissionObjective.IsEventDriven = MissionObjective.isEventDriven
MissionObjective.Validate = MissionObjective.validate
MissionObjective.ToTable = MissionObjective.toTable
