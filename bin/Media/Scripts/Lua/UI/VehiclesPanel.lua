include("BaseComponent.lua")

class 'VehiclesPanel' (BaseComponent)

local function tryCall(object, names, ...)
	if not object then return false, nil end
	for _, name in ipairs(names) do
		local found, fn = pcall(function() return object[name] end)
		if found and fn then
			local called, result = pcall(fn, object, ...)
			if called then return true, result end
		end
	end
	return false, nil
end

local function readMember(object, getterNames, fieldNames, defaultValue)
	local called, value = tryCall(object, getterNames)
	if called and value ~= nil then return value end
	if object then
		for _, name in ipairs(fieldNames) do
			local ok, direct = pcall(function() return object[name] end)
			if ok and direct ~= nil then return direct end
		end
	end
	return defaultValue
end

local function writeMember(object, setterNames, fieldNames, value)
	if not object then return false end
	local called, result = tryCall(object, setterNames, value)
	if called then return result ~= false end
	for _, name in ipairs(fieldNames) do
		local ok = pcall(function() object[name] = value end)
		if ok then return true end
	end
	return false
end

local function toArray(value)
	if not value then return {} end
	if type(value) == "table" then
		local result = {}
		for _, item in ipairs(value) do table.insert(result, item) end
		return result
	end
	local called, count = tryCall(value, { "size", "Size", "count", "Count", "getSize", "GetSize" })
	if not called then count = readMember(value, {}, { "Count", "count", "Length", "length" }, 0) end
	local result = {}
	for index = 0, math.max(0, math.floor(tonumber(count) or 0)) - 1 do
		local found, item = tryCall(value, { "at", "At", "get", "Get", "getItem", "GetItem" }, index)
		if not found then
			found, item = pcall(function() return value[index] end)
		end
		if found and item ~= nil then table.insert(result, item) end
	end
	return result
end

local function getGlobalSingleton(name)
	local object = rawget(_G, name)
	if not object then return nil end
	local ok, instance = pcall(function() return object.instance end)
	if not ok or instance == nil then return object end
	if type(instance) == "function" then
		local called, value = pcall(instance)
		if not called then called, value = pcall(instance, object) end
		return called and value or nil
	end
	return instance
end

local function lower(value) return string.lower(tostring(value or "")) end

function VehiclesPanel:__init(component)
	BaseComponent.__init(self, component)
	self.component = component
	self.showAllModels = false
	self.showHelicopters = false
	self.showPlanes = false
	self.showDrones = false
	self.showCars = false
	self.enableAll = false
	self.workbenchButton = nil
	self.flyButton = nil
	self.flyText = nil
	self.driveText = nil
	self.modelIcons = {}
	self.modelHangerDialog = nil
	self.applicationController = nil
	self.carModelType = 4
	self.onModelSelected = nil
	self.lastButtonsEnabled = nil
	self.lastError = ""
end

function VehiclesPanel:__finalize()
	self:unbindModelIcons()
	self.modelIcons = {}
	BaseComponent.__finalize(self)
end

function VehiclesPanel:getShowAllModels() return self.showAllModels end
function VehiclesPanel:setShowAllModels(value) self.showAllModels = value == true end
function VehiclesPanel:getShowHelicopters() return self.showHelicopters end
function VehiclesPanel:setShowHelicopters(value) self.showHelicopters = value == true end
function VehiclesPanel:getShowPlanes() return self.showPlanes end
function VehiclesPanel:setShowPlanes(value) self.showPlanes = value == true end
function VehiclesPanel:getShowDrones() return self.showDrones end
function VehiclesPanel:setShowDrones(value) self.showDrones = value == true end
function VehiclesPanel:getShowCars() return self.showCars end
function VehiclesPanel:setShowCars(value) self.showCars = value == true end
function VehiclesPanel:getEnableAll() return self.enableAll end
function VehiclesPanel:setEnableAll(value) self.enableAll = value == true end
function VehiclesPanel:setWorkbenchButton(value) self.workbenchButton = value end
function VehiclesPanel:setFlyButton(value) self.flyButton = value end
function VehiclesPanel:setFlyText(value) self.flyText = value end
function VehiclesPanel:setDriveText(value) self.driveText = value end
function VehiclesPanel:setModelHangerDialog(value) self.modelHangerDialog = value end
function VehiclesPanel:setApplicationController(value) self.applicationController = value end
function VehiclesPanel:setCarModelType(value) self.carModelType = tonumber(value) or self.carModelType end
function VehiclesPanel:setModelSelectedCallback(value) self.onModelSelected = value end

function VehiclesPanel:getApplicationManager()
	return self.applicationController or getGlobalSingleton("ApplicationManager") or getGlobalSingleton("IApplicationManager")
end

function VehiclesPanel:getSelectedModelData()
	return readMember(self:getApplicationManager(),
		{ "getSelectedModelData", "GetSelectedModelData" }, { "selectedModelData" }, nil)
end

function VehiclesPanel:setButtonInteractable(button, enabled)
	if not button then return false end
	local called = tryCall(button,
		{ "setInteractable", "SetInteractable", "setEnabled", "SetEnabled" }, enabled == true)
	if called then return true end
	return writeMember(button, {}, { "interactable", "enabled" }, enabled == true)
end

function VehiclesPanel:setObjectEnabled(object, enabled)
	if not object then return false end
	local target = readMember(object, { "getGameObject", "GetGameObject" }, { "gameObject" }, object)
	local called = tryCall(target,
		{ "setActive", "SetActive", "setEnabled", "SetEnabled", "setVisible", "SetVisible" }, enabled == true)
	if called then return true end
	return writeMember(target, {}, { "active", "enabled", "visible" }, enabled == true)
end

function VehiclesPanel:getModelSetup(icon)
	return readMember(icon, { "getModelSetup", "GetModelSetup" }, { "modelSetup" }, nil)
end

function VehiclesPanel:isCarModel(setup)
	local value = readMember(setup, { "getModelType", "GetModelType" }, { "modelType", "m_ModelType" }, nil)
	if tonumber(value) then return tonumber(value) == tonumber(self.carModelType) end
	local text = lower(value)
	return string.find(text, "car", 1, true) ~= nil or string.find(text, "truck", 1, true) ~= nil
end

function VehiclesPanel:bindModelIcon(icon)
	if not icon then return false end
	local callback = function(selectedIcon)
		return self:clickModelIcon(selectedIcon or icon)
	end
	local called = tryCall(icon,
		{ "setSelectedCallback", "SetSelectedCallback", "setModelSelectedCallback", "SetModelSelectedCallback" }, callback)
	if called then
		local ok = pcall(function() icon.__vehiclesPanelCallback = callback end)
		return ok or called
	end
	return false
end

function VehiclesPanel:unbindModelIcons()
	for _, icon in ipairs(self.modelIcons) do
		tryCall(icon, { "setSelectedCallback", "SetSelectedCallback", "setModelSelectedCallback", "SetModelSelectedCallback" }, nil)
		pcall(function() icon.__vehiclesPanelCallback = nil end)
	end
end

function VehiclesPanel:setModelIcons(value)
	self:unbindModelIcons()
	self.modelIcons = toArray(value)
	for _, icon in ipairs(self.modelIcons) do self:bindModelIcon(icon) end
end

function VehiclesPanel:addModelIcon(icon)
	if not icon then return false end
	table.insert(self.modelIcons, icon)
	self:bindModelIcon(icon)
	return true
end

function VehiclesPanel:clearModelIcons()
	self:unbindModelIcons()
	self.modelIcons = {}
end

function VehiclesPanel:updateActionButtons(force)
	local enabled = self:getSelectedModelData() ~= nil
	if force == true or enabled ~= self.lastButtonsEnabled then
		self:setButtonInteractable(self.workbenchButton, enabled)
		self:setButtonInteractable(self.flyButton, enabled)
		self.lastButtonsEnabled = enabled
	end
	return enabled
end

function VehiclesPanel:setup()
	for _, icon in ipairs(self.modelIcons) do self:bindModelIcon(icon) end
	self:updateActionButtons(true)
	return true
end

function VehiclesPanel:start()
	return self:setup()
end

function VehiclesPanel:update()
	self:updateActionButtons(false)
end

function VehiclesPanel:onDestroy()
	self:unbindModelIcons()
	return true
end

function VehiclesPanel:onGUI(event)
	if not event then
		local eventClass = rawget(_G, "Event")
		event = readMember(eventClass, {}, { "current" }, nil)
	end
	if not event then return false end
	local eventType = lower(readMember(event, { "getType", "GetType" }, { "type" }, ""))
	local keyCode = lower(readMember(event, { "getKeyCode", "GetKeyCode" }, { "keyCode" }, ""))
	local shift = readMember(event, { "getShift", "GetShift" }, { "shift" }, false) == true
	local isKeyDown = string.find(eventType, "keydown", 1, true) ~= nil
	local isE = keyCode == "e" or string.find(keyCode, ".e", 1, true) ~= nil
	pcall(function()
		isKeyDown = isKeyDown or event.type == EventType.KeyDown
		isE = isE or event.keyCode == KeyCode.E
	end)
	if isKeyDown and shift and isE then
		self:toggleEnableAll()
		return true
	end
	return false
end

function VehiclesPanel:clickModelIcon(item)
	if not item then self.lastError = "A vehicle icon is required"; return false end
	for _, icon in ipairs(self.modelIcons) do
		if icon and icon ~= item then tryCall(icon, { "deselect", "Deselect" }) end
	end
	local selected = tryCall(item, { "select", "Select" })
	local setup = self:getModelSetup(item) or self:getSelectedModelData()
	local isCar = self:isCarModel(setup)
	self:setObjectEnabled(self.flyText, not isCar)
	self:setObjectEnabled(self.driveText, isCar)
	self:updateActionButtons(true)

	if self.onModelSelected then
		local ok, errorMessage = pcall(self.onModelSelected, self, item, setup)
		if not ok then self.lastError = "Model selection callback failed: " .. tostring(errorMessage); return false end
	end
	if self.modelHangerDialog and self.modelHangerDialog ~= self then
		tryCall(self.modelHangerDialog,
			{ "onPanelModelSelected", "OnPanelModelSelected", "updateSelectedModel", "UpdateSelectedModel" }, item, setup)
	end
	self.lastError = ""
	return selected
end

function VehiclesPanel:toggleEnableAll()
	self.enableAll = not self.enableAll
	self:setup()
	return self.enableAll
end

function VehiclesPanel:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsBool("showAllModels", self.showAllModels)
	properties:setPropertyAsBool("showHelicopters", self.showHelicopters)
	properties:setPropertyAsBool("showPlanes", self.showPlanes)
	properties:setPropertyAsBool("showDrones", self.showDrones)
	properties:setPropertyAsBool("showCars", self.showCars)
	properties:setPropertyAsBool("enableAll", self.enableAll)
	properties:setPropertyAsInt("modelIconCount", #self.modelIcons)
	properties:setPropertyAsBool("hasSelection", self:getSelectedModelData() ~= nil)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("Setup", false)
	properties:setButtonPressed("Toggle Enable All", false)
end

function VehiclesPanel:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("showAllModels") then self.showAllModels = properties:getPropertyAsBool("showAllModels") end
	if properties:hasProperty("showHelicopters") then self.showHelicopters = properties:getPropertyAsBool("showHelicopters") end
	if properties:hasProperty("showPlanes") then self.showPlanes = properties:getPropertyAsBool("showPlanes") end
	if properties:hasProperty("showDrones") then self.showDrones = properties:getPropertyAsBool("showDrones") end
	if properties:hasProperty("showCars") then self.showCars = properties:getPropertyAsBool("showCars") end
	if properties:hasProperty("enableAll") then self.enableAll = properties:getPropertyAsBool("enableAll") end
	if properties:isButtonPressed("Setup") then self:setup() end
	if properties:isButtonPressed("Toggle Enable All") then self:toggleEnableAll() end
end

-- Compatibility aliases for ModelHangerPanel.cs and existing event wiring.
VehiclesPanel.Start = VehiclesPanel.start
VehiclesPanel.Update = VehiclesPanel.update
VehiclesPanel.Setup = VehiclesPanel.setup
VehiclesPanel.OnDestroy = VehiclesPanel.onDestroy
VehiclesPanel.OnGUI = VehiclesPanel.onGUI
VehiclesPanel.ClickModelIcon = VehiclesPanel.clickModelIcon
