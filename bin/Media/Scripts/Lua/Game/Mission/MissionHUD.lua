include("BaseComponent.lua")
include("MissionManager.lua")

class 'MissionHUD' (BaseComponent)

function MissionHUD:__init(component)
	BaseComponent.__init(self, component)
	self.missionManager = nil
	self.missionTitleText = nil
	self.objectiveText = nil
	self.descriptionText = nil
	self.progressText = nil
	self.timerText = nil
	self.statusText = nil
	self.generatedRoot = nil
	self.updateInterval = 0.10
	self.nextUpdate = 0.0
	self.visibleWhenInactive = false
	self.materialPath = "DefaultUI.mat"
	self.lastError = ""
end

function MissionHUD:setMissionManager(value) self.missionManager = value end
function MissionHUD:setMissionTitleText(value) self.missionTitleText = value end
function MissionHUD:setObjectiveText(value) self.objectiveText = value end
function MissionHUD:setDescriptionText(value) self.descriptionText = value end
function MissionHUD:setProgressText(value) self.progressText = value end
function MissionHUD:setTimerText(value) self.timerText = value end
function MissionHUD:setStatusText(value) self.statusText = value end

function MissionHUD:getMissionManager()
	return self.missionManager or MissionManager.instance()
end

function MissionHUD:setText(control, value)
	if not control then return false end
	local called = MissionRuntime.tryCall(control, { "setText", "SetText", "setTextStr", "SetTextStr" }, tostring(value or ""))
	return called
end

function MissionHUD:clear()
	self:setText(self.missionTitleText, "")
	self:setText(self.objectiveText, "")
	self:setText(self.descriptionText, "")
	self:setText(self.progressText, "")
	self:setText(self.timerText, "")
	self:setText(self.statusText, "")
end

function MissionHUD:formatTime(seconds)
	seconds = math.max(0, math.floor(tonumber(seconds) or 0))
	local hours = math.floor(seconds / 3600)
	local minutes = math.floor((seconds % 3600) / 60)
	local remaining = seconds % 60
	return hours > 0 and string.format("%02d:%02d:%02d", hours, minutes, remaining) or string.format("%02d:%02d", minutes, remaining)
end

function MissionHUD:refresh()
	local manager = self:getMissionManager()
	local state = manager and manager:getActiveMission() or nil
	if not state then self:clear(); return false end
	local definition = state.definition
	local objective = state:getCurrentObjective()
	self:setText(self.missionTitleText, definition.missionTitle)
	local displayTime = state.endedAt or MissionRuntime.currentTime(manager:getApplicationManager())
	self:setText(self.timerText, self:formatTime(displayTime - state.startedAt))
	if state.status == "complete" then
		self:setText(self.objectiveText, "Mission Complete")
		self:setText(self.descriptionText, "All required objectives completed")
		self:setText(self.progressText, string.format("Score %d", state.score))
		self:setText(self.statusText, "COMPLETE")
		return true
	elseif state.status == "failed" or state.status == "aborted" then
		self:setText(self.objectiveText, state.failureReason)
		self:setText(self.descriptionText, "")
		self:setText(self.progressText, string.format("Score %d", state.score))
		self:setText(self.statusText, string.upper(state.status))
		return true
	end
	if not objective then return false end
	self:setText(self.objectiveText, objective.title)
	self:setText(self.descriptionText, objective.description)
	local progress = state:getProgress(objective.id)
	local required = manager:getObjectiveRequiredProgress(objective)
	if manager.lastDistance and (objective.type == MissionObjectiveType.GoToLocation or objective.type == MissionObjectiveType.FollowRoute or objective.type == MissionObjectiveType.TimeTrial) then
		self:setText(self.progressText, string.format("%.0f m remaining", manager.lastDistance))
	elseif required > 1.0 or objective.duration > 0.0 then
		self:setText(self.progressText, string.format("%.0f / %.0f", math.min(progress, required), required))
	else
		self:setText(self.progressText, string.format("Mission %.0f%%", state:getCompletionRatio() * 100.0))
	end
	self:setText(self.statusText, objective.optional and "OPTIONAL" or "ACTIVE")
	return true
end

function MissionHUD:start()
	if not self.generatedRoot then self:generate() end
	return self:refresh()
end

function MissionHUD:update()
	local now = MissionRuntime.currentTime()
	if now < self.nextUpdate then return end
	self.nextUpdate = now + self.updateInterval
	self:refresh()
end

function MissionHUD:generate()
	local _, root = MissionRuntime.tryCall(self.component, { "getActor", "GetActor" })
	local applicationManager = self:getMissionManager() and self:getMissionManager():getApplicationManager() or nil
	local _, gameManager = MissionRuntime.tryCall(applicationManager, { "getGameManager", "GetGameManager" })
	if not root or not gameManager then self.lastError = "MissionHUD requires an owning actor and game manager"; return nil end

	local oldRoot = self.generatedRoot
	local createdRoot = nil
	local function addComponent(actor, className)
		local _, component = MissionRuntime.tryCall(actor, { "addComponent", "AddComponent" }, className)
		return component
	end
	local function createActor(parent, name)
		local _, actor = MissionRuntime.tryCall(gameManager, { "createActor", "CreateActor" })
		if not actor then return nil end
		MissionRuntime.tryCall(actor, { "setName", "SetName" }, name)
		if not MissionRuntime.tryCall(parent, { "addChild", "AddChild" }, actor) then
			MissionRuntime.tryCall(gameManager, { "destroyActor", "DestroyActor" }, actor, true)
			return nil
		end
		return actor
	end
	local function transform(actor, x, y, width, height, z)
		local component = addComponent(actor, "LayoutTransform")
		if not component then return false end
		MissionRuntime.tryCall(component, { "setPosition", "SetPosition" }, Vector2F(x, y))
		MissionRuntime.tryCall(component, { "setSize", "SetSize" }, Vector2F(width, height))
		MissionRuntime.tryCall(component, { "setZOrder", "SetZOrder" }, z or 0)
		return true
	end
	local function text(parent, name, value, x, y, width, height, colour)
		local actor = createActor(parent, name)
		if not actor or not transform(actor, x, y, width, height, 3) then return nil end
		local component = addComponent(actor, "Text")
		if component then
			MissionRuntime.tryCall(component, { "setText", "SetText" }, value)
			MissionRuntime.tryCall(component, { "setColour", "SetColour", "setColor", "SetColor" }, colour)
			MissionRuntime.tryCall(component, { "setHorizontalAlignment", "SetHorizontalAlignment" }, 0)
		end
		return component
	end

	local ok, result = pcall(function()
		createdRoot = createActor(root, "__MissionHUDGenerated")
		if not createdRoot or not transform(createdRoot, -590.0, -365.0, 690.0, 260.0, 15) then error("could not create MissionHUD root") end
		local image = addComponent(createdRoot, "Image")
		if image then MissionRuntime.tryCall(image, { "setColour", "SetColour", "setColor", "SetColor" }, ColourF(0.015, 0.030, 0.050, 0.90)) end
		if self.materialPath ~= "" then
			local material = addComponent(createdRoot, "Material")
			if material then MissionRuntime.tryCall(material, { "setMaterialPath", "SetMaterialPath" }, self.materialPath) end
		end
		local primary = ColourF(0.93, 0.97, 1.0, 1.0)
		local secondary = ColourF(0.53, 0.70, 0.84, 1.0)
		local accent = ColourF(0.12, 0.72, 0.90, 1.0)
		return {
			title = text(createdRoot, "MissionTitle", "", -95.0, -92.0, 470.0, 42.0, accent),
			timer = text(createdRoot, "Timer", "", 255.0, -92.0, 130.0, 42.0, secondary),
			objective = text(createdRoot, "Objective", "", -25.0, -38.0, 610.0, 42.0, primary),
			description = text(createdRoot, "Description", "", -25.0, 12.0, 610.0, 54.0, secondary),
			progress = text(createdRoot, "Progress", "", -110.0, 78.0, 430.0, 38.0, primary),
			status = text(createdRoot, "Status", "", 250.0, 78.0, 140.0, 38.0, accent)
		}
	end)
	if not ok or not result or not result.title or not result.objective then
		if createdRoot then MissionRuntime.tryCall(gameManager, { "destroyActor", "DestroyActor" }, createdRoot, true) end
		self.lastError = "MissionHUD generation failed: " .. tostring(result)
		return nil
	end
	self.generatedRoot = createdRoot
	self.missionTitleText = result.title
	self.timerText = result.timer
	self.objectiveText = result.objective
	self.descriptionText = result.description
	self.progressText = result.progress
	self.statusText = result.status
	if oldRoot and oldRoot ~= createdRoot then MissionRuntime.tryCall(gameManager, { "destroyActor", "DestroyActor" }, oldRoot, true) end
	self.lastError = ""
	return createdRoot
end

function MissionHUD:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsFloat("updateInterval", self.updateInterval)
	properties:setPropertyAsString("materialPath", self.materialPath)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("Generate", false)
end

function MissionHUD:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("updateInterval") then self.updateInterval = MissionRuntime.clamp(properties:getPropertyAsFloat("updateInterval"), 0.02, 5.0, 0.10) end
	if properties:hasProperty("materialPath") then self.materialPath = properties:getPropertyAsString("materialPath") end
	if properties:isButtonPressed("Generate") then self:generate() end
end

MissionHUD.Start = MissionHUD.start
MissionHUD.Update = MissionHUD.update
MissionHUD.Refresh = MissionHUD.refresh
MissionHUD.Clear = MissionHUD.clear
MissionHUD.Generate = MissionHUD.generate
MissionHUD.SetMissionManager = MissionHUD.setMissionManager
MissionHUD.SetMissionTitleText = MissionHUD.setMissionTitleText
MissionHUD.SetObjectiveText = MissionHUD.setObjectiveText
