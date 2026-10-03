include("BaseComponent.lua")
include("MissionState.lua")
include("MissionLibrary.lua")

class 'MissionManager' (BaseComponent)

function MissionManager:__init(component)
	BaseComponent.__init(self, component)
	self.startingMission = nil
	self.startingMissionId = ""
	self.player = nil
	self.activeMission = nil
	self.missions = {}
	self.missionOrder = {}
	self.listeners = {}
	self.customHandlers = {}
	self.telemetryProvider = nil
	self.playerPositionProvider = nil
	self.applicationController = nil
	self.uiManager = nil
	self.autoRegisterBuiltIns = true
	self.autoStart = true
	self.updateInterval = 0.05
	self.lastUpdateTime = nil
	self.lastTelemetry = {}
	self.lastPlayerPosition = nil
	self.lastGrounded = nil
	self.lastDistance = nil
	self.lastError = ""
	self.initialised = false
	MissionManager._instance = self
end

function MissionManager:__finalize()
	if MissionManager._instance == self then MissionManager._instance = nil end
	self.listeners = {}
	self.customHandlers = {}
	BaseComponent.__finalize(self)
end

function MissionManager.instance() return MissionManager._instance end
function MissionManager:getActiveMission() return self.activeMission end

function MissionManager:setApplicationController(value) self.applicationController = value end
function MissionManager:setUIManager(value) self.uiManager = value end
function MissionManager:setPlayer(value) self.player = value end
function MissionManager:setTelemetryProvider(value) self.telemetryProvider = value end
function MissionManager:setPlayerPositionProvider(value) self.playerPositionProvider = value end
function MissionManager:setStartingMission(value) self.startingMission = value end
function MissionManager:setStartingMissionId(value) self.startingMissionId = tostring(value or "") end

function MissionManager:getApplicationManager()
	if self.applicationController then return self.applicationController end
	local ok, manager = pcall(function() return IApplicationManager.instance() end)
	return ok and manager or nil
end

function MissionManager:getUIManager()
	if self.uiManager then return self.uiManager end
	local ok, manager = pcall(function() return UIManager.instance() end)
	return ok and manager or nil
end

function MissionManager:addListener(eventName, callback)
	if type(callback) ~= "function" then return false end
	eventName = tostring(eventName or "")
	if eventName == "" then return false end
	self.listeners[eventName] = self.listeners[eventName] or {}
	table.insert(self.listeners[eventName], callback)
	return true
end

function MissionManager:removeListener(eventName, callback)
	local callbacks = self.listeners[tostring(eventName or "")]
	if not callbacks then return false end
	for index = #callbacks, 1, -1 do
		if callbacks[index] == callback then table.remove(callbacks, index); return true end
	end
	return false
end

function MissionManager:emit(eventName, ...)
	for _, callback in ipairs(self.listeners[eventName] or {}) do
		local ok, errorMessage = pcall(callback, self, ...)
		if not ok then self.lastError = "Mission listener failed: " .. tostring(errorMessage) end
	end
end

function MissionManager:notify(message, isError)
	local ui = self:getUIManager()
	if isError then MissionRuntime.tryCall(ui, { "showError", "ShowError" }, message, 4.0)
	else MissionRuntime.tryCall(ui, { "showInformation", "ShowInformation" }, message, 3.0) end
end

function MissionManager:registerMission(mission)
	if type(mission) == "table" and not mission.validate then mission = MissionDefinition(mission) end
	if not mission then return false end
	local valid, errors, warnings = mission:validate()
	if not valid then
		self.lastError = table.concat(errors, "; ")
		return false, errors, warnings
	end
	if not self.missions[mission.missionId] then table.insert(self.missionOrder, mission.missionId) end
	self.missions[mission.missionId] = mission
	self:emit("missionRegistered", mission, warnings)
	return true, mission, warnings
end

function MissionManager:unregisterMission(missionId)
	missionId = tostring(missionId or "")
	if self.activeMission and self.activeMission.definition and self.activeMission.definition.missionId == missionId then return false end
	if not self.missions[missionId] then return false end
	self.missions[missionId] = nil
	for index = #self.missionOrder, 1, -1 do if self.missionOrder[index] == missionId then table.remove(self.missionOrder, index) end end
	return true
end

function MissionManager:getMission(missionId) return self.missions[tostring(missionId or "")] end
function MissionManager:getMissions()
	local result = {}
	for _, missionId in ipairs(self.missionOrder) do if self.missions[missionId] then table.insert(result, self.missions[missionId]) end end
	return result
end

function MissionManager:registerBuiltInMissions()
	return MissionLibrary.registerWith(self)
end

function MissionManager:start()
	if self.initialised then return true end
	self.initialised = true
	if self.autoRegisterBuiltIns then self:registerBuiltInMissions() end
	local mission = self.startingMission
	if not mission and self.startingMissionId ~= "" then mission = self:getMission(self.startingMissionId) end
	if self.autoStart and mission then self:startMission(mission) end
	return true
end

function MissionManager:canStartMission(mission, position)
	if type(mission) == "string" then mission = self:getMission(mission) end
	if not mission then return false, "Mission not found" end
	local valid, errors = mission:validate()
	if not valid then return false, table.concat(errors, "; ") end
	if position and mission.startRadius > 0.0 then
		local distance = MissionRuntime.distance(position, mission.startPosition)
		if distance > mission.startRadius then return false, "Move within the mission start area" end
	end
	return true
end

function MissionManager:startMission(mission)
	if type(mission) == "string" then mission = self:getMission(mission) end
	if type(mission) == "table" and not mission.validate then mission = MissionDefinition(mission) end
	local canStart, reason = self:canStartMission(mission)
	if not canStart then self.lastError = tostring(reason); self:notify(self.lastError, true); return false end
	if self.activeMission and not self.activeMission:isTerminal() then self:abortMission("Replaced by another mission") end

	self.activeMission = MissionState(mission)
	local now = MissionRuntime.currentTime(self:getApplicationManager())
	self.activeMission.startedAt = now
	self.activeMission:markObjectiveStarted(self.activeMission:getCurrentObjective(), now)
	self.lastUpdateTime = now
	self.lastTelemetry = {}
	self.lastGrounded = nil
	self.lastDistance = nil
	self.lastError = ""
	self:emit("missionStarted", self.activeMission, mission)
	self:notify("Mission started: " .. mission.missionTitle, false)
	self:printCurrentObjective()
	return true
end

function MissionManager:restartMission()
	local definition = self.activeMission and self.activeMission.definition or nil
	return definition and self:startMission(definition) or false
end

function MissionManager:abortMission(reason)
	if not self.activeMission or self.activeMission:isTerminal() then return false end
	self.activeMission.status = "aborted"
	self.activeMission.failureReason = tostring(reason or "Mission aborted")
	self.activeMission.endedAt = MissionRuntime.currentTime(self:getApplicationManager())
	self:emit("missionAborted", self.activeMission, self.activeMission.failureReason)
	self:notify(self.activeMission.failureReason, true)
	return true
end

function MissionManager:failMission(reason)
	if not self.activeMission or self.activeMission:isTerminal() then return false end
	self.activeMission:fail(reason, MissionRuntime.currentTime(self:getApplicationManager()))
	self:emit("missionFailed", self.activeMission, self.activeMission.failureReason)
	self:notify(self.activeMission.failureReason, true)
	return true
end

function MissionManager:getPlayerPosition(telemetry)
	if type(self.playerPositionProvider) == "function" then
		local ok, value = pcall(self.playerPositionProvider, self)
		if ok and value then return value end
	elseif self.playerPositionProvider then
		local called, value = MissionRuntime.tryCall(self.playerPositionProvider, { "getPosition", "GetPosition" })
		if called and value then return value end
	end
	if telemetry and telemetry.position then return telemetry.position end
	local position = MissionRuntime.read(self.player, { "getPosition", "GetPosition" }, { "position" }, nil)
	if position then return position end
	local _, transform = MissionRuntime.tryCall(self.player, { "getTransform", "GetTransform" })
	return MissionRuntime.read(transform, { "getPosition", "GetPosition" }, { "position" }, nil)
end

function MissionManager:getTelemetry()
	local value = nil
	if type(self.telemetryProvider) == "function" then
		local ok, result = pcall(self.telemetryProvider, self)
		if ok then value = result end
	elseif self.telemetryProvider then
		local called, result = MissionRuntime.tryCall(self.telemetryProvider,
			{ "getTelemetry", "GetTelemetry", "getFlightTelemetry", "GetFlightTelemetry" })
		if called then value = result end
	end
	if not value then
		local _, result = MissionRuntime.tryCall(self:getApplicationManager(),
			{ "getFlightTelemetry", "GetFlightTelemetry", "getTelemetry", "GetTelemetry" })
		value = result
	end
	value = value or {}
	if type(value) ~= "table" then
		value = {
			altitude = MissionRuntime.read(value, { "getAltitude", "GetAltitude" }, { "altitude" }, nil),
			airspeed = MissionRuntime.read(value, { "getAirspeed", "GetAirspeed", "getSpeed", "GetSpeed" }, { "airspeed", "speed" }, nil),
			heading = MissionRuntime.read(value, { "getHeading", "GetHeading" }, { "heading" }, nil),
			verticalSpeed = MissionRuntime.read(value, { "getVerticalSpeed", "GetVerticalSpeed" }, { "verticalSpeed" }, nil),
			grounded = MissionRuntime.read(value, { "isGrounded", "IsGrounded", "getGrounded", "GetGrounded" }, { "grounded" }, nil),
			position = MissionRuntime.read(value, { "getPosition", "GetPosition" }, { "position" }, nil)
		}
	end
	value.position = value.position or self:getPlayerPosition(value)
	if value.altitude == nil and value.position then local _, y = MissionRuntime.vectorComponents(value.position); value.altitude = y end
	return value
end

function MissionManager:getObjectiveRequiredProgress(objective)
	if objective.duration and objective.duration > 0.0 then return objective.duration end
	return math.max(1, objective.requiredAmount or 1)
end

function MissionManager:updateProgress(objective, value, absolute)
	if not self.activeMission or not objective then return 0.0 end
	local progress = absolute and self.activeMission:setProgress(objective.id, value) or self.activeMission:addProgress(objective.id, value)
	local required = self:getObjectiveRequiredProgress(objective)
	self:emit("objectiveProgress", self.activeMission, objective, progress, required)
	if progress >= required then self:completeObjective(objective.id) end
	return progress
end

function MissionManager:thresholdReached(value, objective)
	value = tonumber(value)
	if not value then return false end
	local target = objective.targetValue
	if target == nil then target = tonumber(objective.metadata.targetValue) end
	if target == nil then local _, y = MissionRuntime.vectorComponents(objective.worldPosition); target = y end
	target = tonumber(target) or 0.0
	local tolerance = tonumber(objective.tolerance) or 0.0
	local comparison = string.lower(tostring(objective.metadata.comparison or "atleast"))
	if comparison == "atmost" or comparison == "below" or comparison == "maximum" then return value <= target + tolerance end
	if comparison == "equal" or comparison == "within" then return math.abs(value - target) <= tolerance end
	return value >= target - tolerance
end

function MissionManager:updateLocationObjective(objective, telemetry)
	local position = self:getPlayerPosition(telemetry)
	if not position then return false end
	local distance = MissionRuntime.distance(position, objective.worldPosition)
	self.lastDistance = distance
	self.activeMission:setProgress(objective.id, math.max(0.0, objective.completionRadius - distance))
	self:emit("objectiveDistance", self.activeMission, objective, distance)
	if distance <= objective.completionRadius then return self:completeObjective(objective.id) end
	return false
end

function MissionManager:updateLandingObjective(objective, telemetry, requireZone)
	local grounded = telemetry.grounded
	if grounded == nil then return false end
	local landed = self.lastGrounded == false and MissionRuntime.asBoolean(grounded, false)
	if not landed then return false end
	local maximum = tonumber(objective.metadata.maximumVerticalSpeed) or 5.0
	if math.abs(tonumber(telemetry.verticalSpeed) or 0.0) > maximum then
		if objective.type == MissionObjectiveType.EmergencyLanding then return self:failMission("Landing impact was too hard") end
		return false
	end
	if requireZone then
		local position = self:getPlayerPosition(telemetry)
		if not position or MissionRuntime.distance(position, objective.worldPosition) > objective.completionRadius then return false end
	end
	return self:completeObjective(objective.id)
end

function MissionManager:updateCurrentObjective(deltaTime, telemetry)
	if not self.activeMission or self.activeMission:isTerminal() then return false end
	local objective = self.activeMission:getCurrentObjective()
	if not objective then return false end
	local now = MissionRuntime.currentTime(self:getApplicationManager())
	self.activeMission:markObjectiveStarted(objective, now)
	if objective.timeLimit > 0.0 and self.activeMission:getObjectiveElapsed(objective, now) > objective.timeLimit then
		if objective.failOnTimeout or not objective.optional then return self:failMission("Objective timed out: " .. objective.title) end
		return self:skipCurrentObjective()
	end

	if objective.type == MissionObjectiveType.GoToLocation or objective.type == MissionObjectiveType.FollowRoute or objective.type == MissionObjectiveType.TimeTrial then
		return self:updateLocationObjective(objective, telemetry)
	elseif objective.type == MissionObjectiveType.ReachAltitude then
		if self:thresholdReached(telemetry.altitude, objective) then return self:completeObjective(objective.id) end
	elseif objective.type == MissionObjectiveType.MaintainAltitude then
		local target = objective.targetValue or objective.metadata.targetValue or 0.0
		local inside = math.abs((tonumber(telemetry.altitude) or -100000.0) - target) <= math.max(1.0, objective.tolerance)
		return self:updateProgress(objective, inside and deltaTime or 0.0, not inside)
	elseif objective.type == MissionObjectiveType.ReachSpeed then
		if self:thresholdReached(telemetry.airspeed or telemetry.speed, objective) then return self:completeObjective(objective.id) end
	elseif objective.type == MissionObjectiveType.MaintainHeading then
		local heading = tonumber(telemetry.heading)
		local inside = heading ~= nil and MissionRuntime.headingDelta(heading, objective.targetValue or objective.metadata.heading or 0.0) <= math.max(1.0, objective.tolerance)
		return self:updateProgress(objective, inside and deltaTime or 0.0, not inside)
	elseif objective.type == MissionObjectiveType.TakeOff then
		local grounded = telemetry.grounded
		if self.lastGrounded == true and grounded ~= nil and not MissionRuntime.asBoolean(grounded, true) then return self:completeObjective(objective.id) end
	elseif objective.type == MissionObjectiveType.Land then
		return self:updateLandingObjective(objective, telemetry, false)
	elseif objective.type == MissionObjectiveType.TouchdownZone or objective.type == MissionObjectiveType.EmergencyLanding then
		return self:updateLandingObjective(objective, telemetry, true)
	else
		local handler = self.customHandlers[objective.targetId] or self.customHandlers[objective.type]
		if handler then
			local ok, result = pcall(handler, self, self.activeMission, objective, telemetry, deltaTime)
			if not ok then self.lastError = "Mission handler failed: " .. tostring(result)
			elseif result == true then return self:completeObjective(objective.id)
			elseif tonumber(result) then return self:updateProgress(objective, tonumber(result), false) end
		end
	end
	return false
end

function MissionManager:update()
	if not self.initialised then self:start() end
	if not self.activeMission or self.activeMission:isTerminal() then return end
	local now = MissionRuntime.currentTime(self:getApplicationManager())
	if self.lastUpdateTime and now - self.lastUpdateTime < self.updateInterval then return end
	local deltaTime = self.lastUpdateTime and math.max(0.0, now - self.lastUpdateTime) or self.updateInterval
	self.lastUpdateTime = now
	if self.activeMission.definition.timeLimit > 0.0 and now - self.activeMission.startedAt > self.activeMission.definition.timeLimit then
		self:failMission("Mission time limit exceeded")
		return
	end
	local telemetry = self:getTelemetry()
	self:updateCurrentObjective(deltaTime, telemetry)
	self.lastTelemetry = telemetry
	self.lastPlayerPosition = telemetry.position
	if telemetry.grounded ~= nil then self.lastGrounded = MissionRuntime.asBoolean(telemetry.grounded, false) end
end

function MissionManager:completeObjective(objectiveId)
	if not self.activeMission or self.activeMission:isTerminal() then return false end
	local objective = self.activeMission:getCurrentObjective()
	if not objective or objective.id ~= objectiveId then return false end
	local now = MissionRuntime.currentTime(self:getApplicationManager())
	local completed = self.activeMission:completeCurrentObjective(now)
	if not completed then return false end
	self:emit("objectiveCompleted", self.activeMission, completed)
	self:notify("Objective complete: " .. completed.title, false)
	if self.activeMission:isComplete() then
		self.activeMission.score = self.activeMission.score + self.activeMission.definition.rewardScore
		self:emit("missionCompleted", self.activeMission, self.activeMission.definition)
		self:notify("Mission complete: " .. self.activeMission.definition.missionTitle, false)
	else
		local nextObjective = self.activeMission:getCurrentObjective()
		self.activeMission:markObjectiveStarted(nextObjective, now)
		self:printCurrentObjective()
	end
	return true
end

function MissionManager:skipCurrentObjective()
	if not self.activeMission then return false end
	local objective = self.activeMission:getCurrentObjective()
	if not objective or not objective.optional then return false end
	local skipped = self.activeMission:skipCurrentObjective(MissionRuntime.currentTime(self:getApplicationManager()))
	if skipped then self:emit("objectiveSkipped", self.activeMission, objective); self:printCurrentObjective() end
	return skipped
end

function MissionManager:completeObjectiveByTargetId(targetId, amount)
	if not self.activeMission or self.activeMission:isTerminal() then return false end
	local objective = self.activeMission:getCurrentObjective()
	if not objective or objective.targetId ~= tostring(targetId or "") then return false end
	self:updateProgress(objective, tonumber(amount) or 1.0, false)
	return true
end

function MissionManager:reportEvent(eventName, targetId, amount, data)
	if not self.activeMission or self.activeMission:isTerminal() then return false end
	eventName = string.lower(tostring(eventName or "custom"))
	local objective = self.activeMission:getCurrentObjective()
	if not objective then return false end
	local eventTypes = {
		collect = MissionObjectiveType.CollectItem, pickup = MissionObjectiveType.CollectItem,
		kill = MissionObjectiveType.KillTarget, destroy = MissionObjectiveType.KillTarget,
		talk = MissionObjectiveType.TalkToNPC, interact = MissionObjectiveType.TalkToNPC,
		gate = MissionObjectiveType.PassGate, manoeuvre = MissionObjectiveType.AerobaticManeuver,
		maneuver = MissionObjectiveType.AerobaticManeuver, custom = MissionObjectiveType.Custom
	}
	if eventName == "takeoff" and objective.type == MissionObjectiveType.TakeOff then return self:completeObjective(objective.id) end
	if eventName == "land" and (objective.type == MissionObjectiveType.Land or objective.type == MissionObjectiveType.TouchdownZone or objective.type == MissionObjectiveType.EmergencyLanding) then
		return self:completeObjective(objective.id)
	end
	local expectedType = eventTypes[eventName]
	if expectedType and objective.type ~= expectedType then return false end
	if objective.targetId ~= "" and objective.targetId ~= tostring(targetId or "") then return false end
	self:emit("missionEvent", self.activeMission, objective, eventName, targetId, data)
	self:updateProgress(objective, tonumber(amount) or 1.0, false)
	return true
end

function MissionManager:registerObjectiveHandler(key, callback)
	if type(callback) ~= "function" then return false end
	self.customHandlers[key] = callback
	return true
end

function MissionManager:printCurrentObjective()
	local objective = self.activeMission and self.activeMission:getCurrentObjective() or nil
	if objective then
		print("New objective: " .. objective.title)
		self:emit("objectiveStarted", self.activeMission, objective)
	end
end

function MissionManager:saveState()
	return self.activeMission and self.activeMission:toTable(MissionRuntime.currentTime(self:getApplicationManager())) or nil
end

function MissionManager:loadState(values)
	if type(values) ~= "table" then return false end
	local mission = self:getMission(values.missionId)
	if not mission then self.lastError = "Saved mission is not registered: " .. tostring(values.missionId); return false end
	self.activeMission = MissionState(mission)
	local now = MissionRuntime.currentTime(self:getApplicationManager())
	self.activeMission:restore(values, now)
	self.lastUpdateTime = now
	self:emit("missionRestored", self.activeMission)
	return true
end

function MissionManager:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("startingMissionId", self.startingMissionId)
	properties:setPropertyAsBool("autoRegisterBuiltIns", self.autoRegisterBuiltIns)
	properties:setPropertyAsBool("autoStart", self.autoStart)
	properties:setPropertyAsFloat("updateInterval", self.updateInterval)
	properties:setPropertyAsInt("registeredMissionCount", #self.missionOrder)
	properties:setPropertyAsString("activeMissionId", self.activeMission and self.activeMission.definition.missionId or "")
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("StartMission", false)
	properties:setButtonPressed("AbortMission", false)
end

function MissionManager:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("startingMissionId") then self.startingMissionId = properties:getPropertyAsString("startingMissionId") end
	if properties:hasProperty("autoRegisterBuiltIns") then self.autoRegisterBuiltIns = properties:getPropertyAsBool("autoRegisterBuiltIns") end
	if properties:hasProperty("autoStart") then self.autoStart = properties:getPropertyAsBool("autoStart") end
	if properties:hasProperty("updateInterval") then self.updateInterval = MissionRuntime.clamp(properties:getPropertyAsFloat("updateInterval"), 0.01, 5.0, 0.05) end
	if properties:isButtonPressed("StartMission") and self.startingMissionId ~= "" then self:startMission(self.startingMissionId) end
	if properties:isButtonPressed("AbortMission") then self:abortMission() end
end

MissionManager.Instance = MissionManager.instance
MissionManager.ActiveMission = MissionManager.getActiveMission
MissionManager.Start = MissionManager.start
MissionManager.Update = MissionManager.update
MissionManager.StartMission = MissionManager.startMission
MissionManager.RestartMission = MissionManager.restartMission
MissionManager.AbortMission = MissionManager.abortMission
MissionManager.FailMission = MissionManager.failMission
MissionManager.CompleteObjective = MissionManager.completeObjective
MissionManager.CompleteObjectiveByTargetId = MissionManager.completeObjectiveByTargetId
MissionManager.ReportEvent = MissionManager.reportEvent
MissionManager.RegisterMission = MissionManager.registerMission
MissionManager.UnregisterMission = MissionManager.unregisterMission
MissionManager.GetMission = MissionManager.getMission
MissionManager.GetMissions = MissionManager.getMissions
MissionManager.RegisterBuiltInMissions = MissionManager.registerBuiltInMissions
MissionManager.UpdateCurrentObjective = MissionManager.updateCurrentObjective
MissionManager.UpdateGoToLocationObjective = MissionManager.updateLocationObjective
MissionManager.PrintCurrentObjective = MissionManager.printCurrentObjective
MissionManager.SkipCurrentObjective = MissionManager.skipCurrentObjective
MissionManager.SaveState = MissionManager.saveState
MissionManager.LoadState = MissionManager.loadState
