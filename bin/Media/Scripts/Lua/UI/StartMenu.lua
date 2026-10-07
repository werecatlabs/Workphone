include("UIDialog.lua")
if not UICanvas then include("UICanvas.lua") end

class 'StartMenu' (UIDialog)

local function tryCall(object, names, ...)
	if not object then return false, nil end

	for _, name in ipairs(names) do
		local ok, fn = pcall(function() return object[name] end)
		if ok and fn then
			local called, result = pcall(fn, object, ...)
			if called then return true, result end
		end
	end

	return false, nil
end

local function getActor(value)
	if not value then return nil end
	local ok, actor = tryCall(value, { "getActor", "GetActor" })
	if ok and actor then return actor end
	local hasName, getNameMethod = pcall(function() return value.getName end)
	if hasName and getNameMethod then return value end
	return nil
end

function StartMenu:__init(component)
	UIDialog.__init(self, component)
	self.component = component

	-- Content and navigation.
	self.title = "LIONCAT"
	self.subtitle = "Simulation"
	self.subtitleHeight = 48
	self.buttonCentreY = 15
	self.versionText = ""
	self.startScene = "Game"
	self.workshopScene = "Workshop"
	self.settingsActorName = "Settings"
	self.materialPath = "DefaultUI.mat"

	-- Labels are properties so localization/bootstrap code can replace them.
	self.resumeLabel = "Continue"
	self.startLabel = "Start"
	self.workshopLabel = "Workshop"
	self.settingsLabel = "Settings"
	self.exitLabel = "Quit"

	-- Menu composition and input policy.
	self.showResume = false
	self.showWorkshop = true
	self.showSettings = true
	self.showExit = true
	self.interactionDelay = 0.75
	self.debounceTime = 0.25

	-- Runtime state and injected integration points.
	self.generatedRoot = nil
	self.statusText = nil
	self.versionComponent = nil
	self.settingsController = nil
	self.navigationController = nil
	self.callbacks = {}
	self.buttons = {}
	self.buttonActors = {}
	self.buttonActions = {}
	self.generationWarnings = {}
	self.lastActionTime = -1000.0
	self.busy = false
	self.visible = true
	self.lastError = ""
end

function StartMenu:__finalize()
	self.callbacks = {}
	self.buttons = {}
	self.buttonActors = {}
	self.buttonActions = {}
	UIDialog.__finalize(self)
end

function StartMenu:update()
	-- UI input is event driven. Intentionally no per-frame logging or polling.
end

function StartMenu:start()
	UIDialog.start(self)
	self:updateVersionText()
	return true
end

function StartMenu:getProperties(parameters)
	local properties = parameters:at(0)

	properties:setPropertyAsString("title", self.title)
	properties:setPropertyAsString("subtitle", self.subtitle)
	properties:setPropertyAsString("versionText", self.versionText)
	properties:setPropertyAsString("startScene", self.startScene)
	properties:setPropertyAsString("workshopScene", self.workshopScene)
	properties:setPropertyAsString("settingsActorName", self.settingsActorName)
	properties:setPropertyAsString("materialPath", self.materialPath)
	properties:setPropertyAsString("resumeLabel", self.resumeLabel)
	properties:setPropertyAsString("startLabel", self.startLabel)
	properties:setPropertyAsString("workshopLabel", self.workshopLabel)
	properties:setPropertyAsString("settingsLabel", self.settingsLabel)
	properties:setPropertyAsString("exitLabel", self.exitLabel)
	properties:setPropertyAsBool("showResume", self.showResume)
	properties:setPropertyAsBool("showWorkshop", self.showWorkshop)
	properties:setPropertyAsBool("showSettings", self.showSettings)
	properties:setPropertyAsBool("showExit", self.showExit)
	properties:setPropertyAsFloat("interactionDelay", self.interactionDelay)
	properties:setPropertyAsFloat("debounceTime", self.debounceTime)
	properties:setButtonPressed("Generate", false)
	properties:setButtonPressed("Validate", false)
end

function StartMenu:setProperties(parameters)
	local properties = parameters:at(0)

	local function readString(name)
		if properties:hasProperty(name) then self[name] = properties:getPropertyAsString(name) end
	end
	local function readBool(name)
		if properties:hasProperty(name) then self[name] = properties:getPropertyAsBool(name) end
	end
	local function readFloat(name)
		if properties:hasProperty(name) then self[name] = properties:getPropertyAsFloat(name) end
	end

	for _, name in ipairs({
		"title", "subtitle", "versionText", "startScene", "workshopScene",
		"settingsActorName", "materialPath", "resumeLabel", "startLabel",
		"workshopLabel", "settingsLabel", "exitLabel"
	}) do
		readString(name)
	end

	for _, name in ipairs({ "showResume", "showWorkshop", "showSettings", "showExit" }) do
		readBool(name)
	end
	readFloat("interactionDelay")
	readFloat("debounceTime")

	if properties:isButtonPressed("Validate") then self:validate(true) end
	if properties:isButtonPressed("Generate") then self:generate() end
end

function StartMenu:setNavigationController(value) self.navigationController = value end
function StartMenu:setSettingsController(value) self.settingsController = value end
function StartMenu:setCallback(action, callback) self.callbacks[action] = callback end

function StartMenu:validate(verbose)
	local errors = {}
	local warnings = {}

	local hasActor, ownerActor = tryCall(self.component, { "getActor", "GetActor" })
	if not hasActor or not ownerActor then
		table.insert(errors, "StartMenu has no owning component/actor")
	end
	if type(self.title) ~= "string" then table.insert(errors, "title must be a string")
	elseif self.title == "" then table.insert(warnings, "title is empty") end
	if type(self.subtitle) ~= "string" then table.insert(errors, "subtitle must be a string") end
	if type(self.versionText) ~= "string" then table.insert(errors, "versionText must be a string") end
	if type(self.startScene) ~= "string" or self.startScene == "" then
		table.insert(errors, "startScene is empty or invalid")
	end
	if self.showWorkshop and (type(self.workshopScene) ~= "string" or self.workshopScene == "") then
		table.insert(errors, "workshopScene is empty while Workshop is enabled")
	end
	if type(self.materialPath) ~= "string" then table.insert(errors, "materialPath must be a string") end
	if type(self.settingsActorName) ~= "string" then
		table.insert(errors, "settingsActorName must be a string")
	end
	for _, name in ipairs({ "showResume", "showWorkshop", "showSettings", "showExit" }) do
		if type(self[name]) ~= "boolean" then table.insert(errors, name .. " must be a boolean") end
	end
	if type(self.interactionDelay) ~= "number" or self.interactionDelay ~= self.interactionDelay or
		self.interactionDelay < 0.0 then
		table.insert(errors, "interactionDelay must be a non-negative number")
	end
	if type(self.debounceTime) ~= "number" or self.debounceTime ~= self.debounceTime or
		self.debounceTime < 0.0 then
		table.insert(errors, "debounceTime must be a non-negative number")
	end
	if not self.showResume and not self.showWorkshop and not self.showSettings and not self.showExit then
		table.insert(warnings, "Start is the only enabled menu action")
	end
	if type(self.title) == "string" and #self.title > 128 then
		table.insert(warnings, "title may overflow the generated layout")
	end
	if self.materialPath == "" then
		table.insert(warnings, "materialPath is empty; UI components will use engine defaults")
	end

	self.lastError = table.concat(errors, "; ")
	if verbose then
		for _, message in ipairs(errors) do print("StartMenu error: " .. message) end
		for _, message in ipairs(warnings) do print("StartMenu warning: " .. message) end
		if #errors == 0 then print("StartMenu validation passed") end
	end

	return #errors == 0, errors, warnings
end

function StartMenu:generate()
	local valid = self:validate(false)
	if not valid then
		print("StartMenu:generate aborted: " .. self.lastError)
		return nil
	end
	self:updateVersionText()

	local managerOk, applicationManager = pcall(function() return IApplicationManager.instance() end)
	local gameManagerOk, gameManager = tryCall(applicationManager, { "getGameManager", "GetGameManager" })
	local rootOk, root = tryCall(self.component, { "getActor", "GetActor" })
	if not managerOk or not gameManagerOk or not gameManager or not rootOk or not root then
		self.lastError = "game manager or root actor is unavailable"
		print("StartMenu:generate aborted: " .. self.lastError)
		return nil
	end

	-- Keep the current menu alive until its replacement has been built. This
	-- also avoids deleting unrelated children owned by the StartMenu actor.
	local oldRoots = {}
	local function sameActor(a, b)
		if rawequal(a, b) then return true end
		if not a or not b then return false end
		return a:getHandle():getInstanceId() == b:getHandle():getInstanceId()
	end
	local function rememberOldRoot(candidate)
		if not candidate then return end
		for _, existing in ipairs(oldRoots) do
			if sameActor(existing, candidate) then return end
		end
		table.insert(oldRoots, candidate)
	end
	rememberOldRoot(self.generatedRoot)
	pcall(function()
		local children = root:getChildren()
		if children then
			for index = 0, children:size() - 1 do
				local child = children:at(index)
				if child and child:getName() == "__StartMenuGenerated" then
					rememberOldRoot(child)
				end
			end
		end
	end)

	local newRoot = nil
	local newButtons = {}
	local newButtonActors = {}
	local newButtonActions = {}
	local generationWarnings = {}
	local previousVisible = self.visible ~= false

	local function destroyGenerated(candidate)
		if not candidate then return true end
		local ok = pcall(function() gameManager:destroyActor(candidate, true) end)
		if not ok then
			pcall(function() root:removeChild(candidate) end)
			pcall(function() candidate:setEnabled(false, false) end)
		end
		return ok
	end

	local buildOk, buildResult = xpcall(function()

	local colours = {
		background = ColourF(0.012, 0.022, 0.040, 1.0),
		card = ColourF(0.030, 0.050, 0.080, 0.98),
		button = ColourF(0.080, 0.135, 0.210, 1.0),
		normal = ColourF(0.100, 0.190, 0.300, 1.0),
		highlight = ColourF(0.180, 0.450, 0.730, 1.0),
		pressed = ColourF(0.060, 0.290, 0.520, 1.0),
		disabled = ColourF(0.120, 0.130, 0.150, 0.60),
		primaryText = ColourF(0.940, 0.970, 1.0, 1.0),
		secondaryText = ColourF(0.610, 0.700, 0.810, 1.0),
		danger = ColourF(0.400, 0.110, 0.130, 1.0)
	}

	local function addComponent(actor, className)
		if not actor then return nil end
		local ok, component = pcall(function() return UICanvas.addComponent(actor, className) end)
		if not ok then
			table.insert(generationWarnings, "unable to add " .. className .. ": " .. tostring(component))
			return nil
		end
		return component
	end

	local function createActor(parent, name)
		if not parent then return nil end
		local actor = nil
		local ok, errorMessage = pcall(function()
			actor = gameManager:createActor()
			if actor then
				actor:setName(name)
				parent:addChild(actor)
			end
		end)
		if not ok then
			table.insert(generationWarnings, "unable to create " .. name .. ": " .. tostring(errorMessage))
			if actor then pcall(function() gameManager:destroyActor(actor, true) end) end
			return nil
		end
		if not actor then return nil end
		return actor
	end

	local function addTransform(actor, position, size, zOrder)
		if not actor then return nil end
		local transform = addComponent(actor, "LayoutTransform")
		if transform then
			UICanvas.center(transform)
			transform:setPosition(position)
			transform:setSize(size)
			transform:setZOrder(zOrder or 0, false)
		end
		return transform
	end

	local function addMaterial(actor)
		if not actor or not self.materialPath or self.materialPath == "" then return nil end
		local material = addComponent(actor, "Material")
		if material then
			local ok, errorMessage = pcall(function() material:setMaterialPath(self.materialPath) end)
			if not ok then
				table.insert(generationWarnings, "unable to set material path: " .. tostring(errorMessage))
			end
		end
		return material
	end

	local function createPanel(parent, name, position, size, colour, zOrder)
		local actor = createActor(parent, name)
		if not actor then return nil end
		local transform = addTransform(actor, position, size, zOrder)
		local image = addComponent(actor, "Image")
		if image then image:setColour(colour) end
		addMaterial(actor)
		return actor, image, transform
	end

	local function createText(parent, name, value, position, size, colour, zOrder)
		local actor = createActor(parent, name)
		if not actor then return nil, nil end
		local transform = addTransform(actor, position, size, zOrder or 3)
		local text = addComponent(actor, "Text")
		if text then
			text:setText(tostring(value or ""))
			local properties = text:getProperties()
			properties:setPropertyAsInt("size", name == "Title" and 40 or name == "Subtitle" and 20 or 16)
			text:setProperties(properties)
			text:setColour(colour or colours.primaryText)
			text:setHorizontalAlignment(1)
			text:setVerticalAlignment(1)
		end
		return actor, text, transform
	end

	local function createButton(parent, action, label, position, danger)
		local actorName = action .. "Button"
		local actor = createActor(parent, actorName)
		if not actor then return nil, nil end
		local transform = addTransform(actor, position, Vector2F(390.0, 72.0), 2)

		local image = addComponent(actor, "Image")
		if image then image:setColour(danger and colours.danger or colours.button) end

		local button = addComponent(actor, "Button")
		local listenerWired = false
		if button then
			button:setTextStr(tostring(label or action))
			button:setTextSize(26)
			button:setNormalColour(danger and colours.danger or colours.normal)
			button:setHighlightedColour(colours.highlight)
			button:setPressedColour(colours.pressed)
			button:setDisabledColour(colours.disabled)

			-- Route native clicks to the owning script through a reusable adapter.
			local functionNames = {
				Resume = "handleResumeClicked",
				Start = "handleStartClicked",
				Workshop = "handleWorkshopClicked",
				Settings = "handleSettingsClicked",
				Exit = "handleExitClicked"
			}
			UICanvas.bindButton(button, self.component, functionNames[action])
			listenerWired = true
		end
		if button and not listenerWired then
			table.insert(generationWarnings, action .. " button has no activation listener")
		end

		addMaterial(actor)
		createText(actor, "Text", label, Vector2F(0.0, 0.0),
			Vector2F(370.0, 58.0), colours.primaryText, 3)

		if not transform or not listenerWired then button = nil end
		newButtons[action] = button
		newButtonActors[action] = actor
		newButtonActions[actorName] = action
		return actor, button
	end

	local canvas = assert(createActor(root, "__StartMenuGenerated"), "Cannot create menu canvas")
	newRoot = canvas
	addTransform(canvas, Vector2F(0, 0), Vector2F(1920, 1080), 20)
	UICanvas.attach(canvas, true)
	local background, backgroundImage, backgroundTransform = createPanel(
		canvas, "Backdrop", Vector2F(0.0, 0.0),
		Vector2F(1920.0, 1080.0), colours.background, 0)
	if not background then
		self.lastError = "failed to create background actor"
		return nil
	end
	if not backgroundImage then
		self.lastError = "failed to create background image component"
		return nil
	end
	if not backgroundTransform then
		self.lastError = "failed to create background layout transform"
		return nil
	end

	local specs = {}
	if self.showResume then table.insert(specs, { "Resume", self.resumeLabel, false }) end
	table.insert(specs, { "Start", self.startLabel, false })
	if self.showWorkshop then table.insert(specs, { "Workshop", self.workshopLabel, false }) end
	if self.showSettings then table.insert(specs, { "Settings", self.settingsLabel, false }) end
	if self.showExit then table.insert(specs, { "Exit", self.exitLabel, true }) end
	local subtitleHeight = math.max(48, self.subtitleHeight or 48)
	local buttonsHeight = #specs * 72 + (#specs - 1) * 16
	local cardHeight = math.max(900, 284 + subtitleHeight + buttonsHeight)
	assert(cardHeight <= 1040, "Menu content exceeds the canvas height")
	local titleY = -cardHeight * 0.5 + 64
	local subtitleY = titleY + 52 + subtitleHeight * 0.5
	local firstButtonY = subtitleY + subtitleHeight * 0.5 + 60
	local statusY = firstButtonY + (#specs - 1) * 88 + 84
	local card, cardImage, cardTransform = createPanel(background, "MenuCard", Vector2F(0.0, 0.0),
		Vector2F(720.0, cardHeight), colours.card, 1)
	if not card or not cardImage or not cardTransform then
		self.lastError = "failed to create complete menu card"
		print("StartMenu:generate aborted: " .. self.lastError)
		return nil
	end
	local titleActor, titleText, titleTransform = createText(
		card, "Title", self.title, Vector2F(0.0, titleY),
		Vector2F(660.0, 80.0), colours.primaryText, 3)
	if not titleActor or not titleText or not titleTransform then
		self.lastError = "failed to create complete title text"
		return nil
	end
	local _, subtitleText = createText(card, "Subtitle", self.subtitle,
		Vector2F(0.0, subtitleY),
		Vector2F(660.0, subtitleHeight), colours.secondaryText, 3)
	if self.subtitle ~= "" and not subtitleText then
		table.insert(generationWarnings, "subtitle text component was not created")
	end

	local spacing = 88.0
	for index, spec in ipairs(specs) do
		local y = firstButtonY + (index - 1) * spacing
		createButton(card, spec[1], spec[2], Vector2F(0.0, y), spec[3])
	end
	for _, spec in ipairs(specs) do
		if not newButtons[spec[1]] then
			self.lastError = "failed to create or wire the " .. spec[1] .. " button"
			return nil
		end
	end

	local statusActor, statusText, statusTransform = createText(
		card, "Status", "", Vector2F(0.0, statusY),
		Vector2F(660.0, 48.0), colours.secondaryText, 3)
	if not statusActor or not statusText or not statusTransform then
		self.lastError = "failed to create complete status text"
		return nil
	end
	local _, versionComponent = createText(card, "Version", self.versionText, Vector2F(0.0, cardHeight * 0.5 - 28),
		Vector2F(660.0, 24.0), colours.secondaryText, 3)
	if self.versionText ~= "" and not versionComponent then
		table.insert(generationWarnings, "version text component was not created")
	end

	return {
		root = canvas,
		statusActor = statusActor,
		statusText = statusText,
		versionComponent = versionComponent
	}
	end, function(failure) return debug.traceback(tostring(failure), 2) end)

	if not buildOk or not buildResult then
		if not buildOk then self.lastError = "generation failed: " .. tostring(buildResult) end
		if newRoot and not destroyGenerated(newRoot) then
			table.insert(generationWarnings, "failed generated hierarchy required fallback cleanup")
		end
		self.generationWarnings = generationWarnings
		pcall(function()
			self:setStatus(self.lastError ~= "" and self.lastError or "Menu generation failed", true)
		end)
		for _, warning in ipairs(generationWarnings) do print("StartMenu warning: " .. warning) end
		print("StartMenu:generate aborted: " .. (self.lastError ~= "" and self.lastError or "unknown error"))
		return nil
	end

	-- Commit only after all required UI exists. The old hierarchy is destroyed
	-- last, so a failed rebuild leaves the prior menu usable.
	for _, oldRoot in ipairs(oldRoots) do
		if not sameActor(oldRoot, buildResult.root) then
			local destroyed = destroyGenerated(oldRoot)
			if not destroyed then
				table.insert(generationWarnings, "a previous generated hierarchy could not be destroyed")
			end
		end
	end

	self.generatedRoot = buildResult.root
	self.statusActor = buildResult.statusActor
	self.statusText = buildResult.statusText
	self.versionComponent = buildResult.versionComponent
	self.buttons = newButtons
	self.buttonActors = newButtonActors
	self.buttonActions = newButtonActions
	self.busy = false
	self.visible = previousVisible
	buildResult.root:setEnabled(previousVisible, false)
	self.lastError = ""
	self.generationWarnings = generationWarnings
	for _, warning in ipairs(generationWarnings) do print("StartMenu warning: " .. warning) end
	return buildResult.root
end

function StartMenu:getTime()
	local applicationManager = IApplicationManager.instance()
	local timer = applicationManager and applicationManager:getTimer()
	return timer and timer:getTimeSinceLevelLoad() or 0.0
end

function StartMenu:updateVersionText()
	local productName = ""
	local version = ""
	local buildNumber = tostring(self.buildNumber or "")
	local managerOk, applicationManager = pcall(function() return IApplicationManager.instance() end)
	if managerOk and applicationManager then
		local _, product = tryCall(applicationManager,
			{ "getProductName", "GetProductName", "getApplicationName", "GetApplicationName" })
		local _, appVersion = tryCall(applicationManager,
			{ "getVersion", "GetVersion", "getApplicationVersion", "GetApplicationVersion" })
		local _, build = tryCall(applicationManager,
			{ "getBuildNumber", "GetBuildNumber", "getBuild", "GetBuild" })
		productName = tostring(product or "")
		version = tostring(appVersion or "")
		if build ~= nil and tostring(build) ~= "" then buildNumber = tostring(build) end
	end

	if self.versionText == "" then
		local parts = {}
		if productName ~= "" then table.insert(parts, productName) end
		if version ~= "" then table.insert(parts, version) end
		if buildNumber ~= "" then table.insert(parts, "Build " .. buildNumber) end
		self.versionText = table.concat(parts, " ")
	end
	if self.versionComponent then
		tryCall(self.versionComponent, { "setText", "SetText" }, self.versionText)
	end
	return self.versionText
end

function StartMenu:canAcceptInput()
	local now = self:getTime()
	return not self.busy and now >= self.interactionDelay and
		(now - self.lastActionTime) >= self.debounceTime
end

function StartMenu:setBusy(value, message)
	self.busy = value == true
	for _, button in pairs(self.buttons) do
		if button then button:setEnabled(not self.busy) end
	end
	self:setStatus(message or "", self.busy)
end

function StartMenu:setStatus(message, isError)
	if self.statusText then
		self.statusText:setText(message or "")
		if isError then self.statusText:setColour(ColourF(1.0, 0.35, 0.35, 1.0))
		else self.statusText:setColour(ColourF(0.61, 0.70, 0.81, 1.0)) end
	end
	if message and message ~= "" and isError then self.lastError = message end
end

function StartMenu:invokeCallback(action, ...)
	local callback = self.callbacks[action]
	if not callback then return false end
	local ok, result = pcall(callback, self, ...)
	if not ok then
		self:setStatus("Action failed: " .. tostring(result), true)
		return true
	end
	return true, result
end

function StartMenu:loadScene(sceneName)
	if not sceneName or sceneName == "" then
		self:setStatus("No scene is configured for this action", true)
		return false
	end
	if not self:canAcceptInput() then return false end

	local applicationManager = IApplicationManager.instance()
	local sceneManager = applicationManager and applicationManager:getSceneManager()
	if not sceneManager then
		self:setStatus("Scene manager is unavailable", true)
		return false
	end

	self.lastActionTime = self:getTime()
	self:setBusy(true, "Loading " .. sceneName .. "...")
	local ok, errorMessage = pcall(function() sceneManager:loadScene(sceneName) end)
	if not ok then
		self:setBusy(false, "")
		self:setStatus("Unable to load scene: " .. tostring(errorMessage), true)
		return false
	end
	return true
end

function StartMenu:handleResumeClicked()
	if not self:canAcceptInput() then return false end
	self.lastActionTime = self:getTime()
	if self:invokeCallback("Resume") then return true end
	local called = tryCall(self.navigationController, { "resume", "Resume", "hidePauseMenu", "HidePauseMenu" })
	if not called then self:setStatus("Resume is not available", true) end
	return called
end

function StartMenu:onClickLeaveTraining()
	if not self:canAcceptInput() then return false end
	self.lastActionTime = self:getTime()
	if self:invokeCallback("LeaveTraining") then return true end

	local managerOk, applicationManager = pcall(function() return IApplicationManager.instance() end)
	local _, trainingManager = tryCall(managerOk and applicationManager or nil,
		{ "getTrainingManager", "GetTrainingManager" })
	if not trainingManager then
		local singletonOk, singleton = pcall(function() return TrainingManager.instance() end)
		if singletonOk then trainingManager = singleton end
	end
	if not trainingManager then
		self:setStatus("Training manager is unavailable", true)
		return false
	end

	tryCall(trainingManager, { "setTrainingMode", "SetTrainingMode" }, false)
	pcall(function() trainingManager.isTrainingMode = false end)
	local called = tryCall(trainingManager, { "destroyGame", "DestroyGame", "leaveTraining", "LeaveTraining" })
	if not called then self:setStatus("Unable to leave training", true) end
	return called
end

function StartMenu:onClickModels()
	if not self:canAcceptInput() then return false end
	self.lastActionTime = self:getTime()
	if self:invokeCallback("Models") then return true end
	local called = tryCall(self.navigationController,
		{ "showModelHanger", "ShowModelHanger", "handleAircraftClicked", "HandleAircraftClicked" })
	if not called then self:setStatus("Aircraft selection is unavailable", true) end
	return called
end

function StartMenu:onClickSceneries()
	if not self:canAcceptInput() then return false end
	self.lastActionTime = self:getTime()
	if self:invokeCallback("Sceneries") then return true end
	local called = tryCall(self.navigationController,
		{ "showScenerySelector", "ShowScenerySelector", "handleSceneryClicked", "HandleSceneryClicked" })
	if not called then self:setStatus("Scenery selection is unavailable", true) end
	return called
end

function StartMenu:onClickTraining()
	if not self:canAcceptInput() then return false end
	self.lastActionTime = self:getTime()
	if self:invokeCallback("Training") then return true end
	local called = tryCall(self.navigationController, { "showTraining", "ShowTraining" })
	if not called then
		called = tryCall(self.navigationController,
			{ "showExclusiveDialog", "ShowExclusiveDialog" }, "training_menu")
	end
	if not called then self:setStatus("Training is unavailable", true) end
	return called
end

function StartMenu:onClickStats()
	if not self:canAcceptInput() then return false end
	self.lastActionTime = self:getTime()
	if self:invokeCallback("Stats") then return true end
	local called = tryCall(self.navigationController, { "showStats", "ShowStats" })
	if not called then
		called = tryCall(self.navigationController,
			{ "showExclusiveDialog", "ShowExclusiveDialog" }, "stats_menu")
	end
	if not called then self:setStatus("Flight statistics are unavailable", true) end
	return called
end

function StartMenu:assignButtonMaterials()
	if not self.materialPath or self.materialPath == "" then return false end
	local assigned = false
	for _, actor in pairs(self.buttonActors) do
		local found, material = tryCall(actor, { "getComponent", "GetComponent" }, "Material")
		if found and material then
			assigned = tryCall(material, { "setMaterialPath", "SetMaterialPath" }, self.materialPath) or assigned
		end
	end
	return assigned
end

function StartMenu:setupButtons()
	return self:generate()
end

function StartMenu:handleStartClicked()
	if not self:canAcceptInput() then return false end
	if self.callbacks.Start then
		self.lastActionTime = self:getTime()
		return self:invokeCallback("Start", self.startScene)
	end
	return self:loadScene(self.startScene)
end

function StartMenu:handleWorkshopClicked()
	if not self:canAcceptInput() then return false end
	if self.callbacks.Workshop then
		self.lastActionTime = self:getTime()
		return self:invokeCallback("Workshop", self.workshopScene)
	end
	return self:loadScene(self.workshopScene)
end

function StartMenu:handleSettingsClicked()
	if not self:canAcceptInput() then return false end
	self.lastActionTime = self:getTime()
	if self:invokeCallback("Settings") then return true end

	local called = tryCall(self.settingsController, { "show", "Show", "showSettings", "ShowSettings" })
	if not called then
		local applicationManager = IApplicationManager.instance()
		local gameManager = applicationManager and applicationManager:getGameManager()
		local actor = gameManager and gameManager:getActorByName(self.settingsActorName)
		if actor then
			actor:setEnabled(true)
			called = true
		end
	end

	if called then self:hide()
	else self:setStatus("Settings screen is unavailable", true) end
	return called
end

function StartMenu:handleExitClicked()
	if not self:canAcceptInput() then return false end
	self.lastActionTime = self:getTime()
	if self.callbacks.Exit then return self:invokeCallback("Exit") end

	local applicationManager = IApplicationManager.instance()
	if not applicationManager then
		self:setStatus("Application manager is unavailable", true)
		return false
	end
	self:setBusy(true, "Closing...")
	applicationManager:setQuit(true)
	return true
end

function StartMenu:dispatchAction(action)
	local handlers = {
		Resume = self.handleResumeClicked,
		Start = self.handleStartClicked,
		Workshop = self.handleWorkshopClicked,
		Settings = self.handleSettingsClicked,
		Exit = self.handleExitClicked
	}
	local handler = handlers[action]
	if not handler then return false end
	return handler(self)
end

function StartMenu:handleEvent(parameters, results)
	if not parameters then return end
	local eventHash = parameters:at(1)
	if eventHash ~= IEvent.CLICK_HASH and eventHash ~= IEvent.ACTIVATE_HASH then return end

	local sender = parameters:at(3)
	local actor = nil
	local objectOk, object = pcall(function() return parameters:at(4) end)
	if objectOk then actor = getActor(object) end
	if not actor then actor = getActor(sender) end
	if not actor then return end
	local action = self.buttonActions[actor:getName()]
	if action then self:dispatchAction(action) end
end

function StartMenu:show()
	if self.generatedRoot then self.generatedRoot:setEnabled(true, false) end
	self.visible = true
	self:setBusy(false, "")
end

function StartMenu:hide()
	if self.generatedRoot then self.generatedRoot:setEnabled(false, false) end
	self.visible = false
end

function StartMenu:isVisible()
	return self.visible and self.generatedRoot ~= nil and self.generatedRoot:isEnabled()
end

-- Compatibility aliases for existing C#/Lua UI bindings.
StartMenu.Start = StartMenu.start
StartMenu.Generate = StartMenu.generate
StartMenu.Validate = StartMenu.validate
StartMenu.Show = StartMenu.show
StartMenu.Hide = StartMenu.hide
StartMenu.IsVisible = StartMenu.isVisible
StartMenu.UpdateVersionText = StartMenu.updateVersionText
StartMenu.OnClickResume = StartMenu.handleResumeClicked
StartMenu.OnClickLeaveTraining = StartMenu.onClickLeaveTraining
StartMenu.OnClickModels = StartMenu.onClickModels
StartMenu.OnClickSceneries = StartMenu.onClickSceneries
StartMenu.OnClickTraining = StartMenu.onClickTraining
StartMenu.OnClickStats = StartMenu.onClickStats
StartMenu.AssignButtonMaterials = StartMenu.assignButtonMaterials
StartMenu.SetupButtons = StartMenu.setupButtons
StartMenu.HandleResumeClicked = StartMenu.handleResumeClicked
StartMenu.HandleStartClicked = StartMenu.handleStartClicked
StartMenu.HandleWorkshopClicked = StartMenu.handleWorkshopClicked
StartMenu.HandleSettingsClicked = StartMenu.handleSettingsClicked
StartMenu.HandleExitClicked = StartMenu.handleExitClicked
