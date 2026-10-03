include("UIDialog.lua")

class 'VehicleSelect' (UIDialog)

local FILTER_ALL = "all"
local FILTER_HELICOPTERS = "helicopters"
local FILTER_PLANES = "planes"
local FILTER_DRONES = "drones"
local FILTER_CARS = "cars"

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

local function tryStatic(object, names, ...)
	if not object then return false, nil end
	for _, name in ipairs(names) do
		local found, fn = pcall(function() return object[name] end)
		if found and fn then
			local called, result = pcall(fn, ...)
			if not called then called, result = pcall(fn, object, ...) end
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

local function asBoolean(value, defaultValue)
	if value == nil then return defaultValue == true end
	if type(value) == "boolean" then return value end
	if type(value) == "number" then return value ~= 0 end
	local text = string.lower(tostring(value))
	if text == "true" or text == "yes" or text == "on" or text == "1" then return true end
	if text == "false" or text == "no" or text == "off" or text == "0" or text == "" then return false end
	return defaultValue == true
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
	count = math.max(0, math.floor(tonumber(count) or 0))
	local result = {}
	for index = 0, count - 1 do
		local found, item = tryCall(value, { "at", "At", "get", "Get", "getItem", "GetItem" }, index)
		if not found then
			found, item = pcall(function() return value[index] end)
		end
		if found and item ~= nil then table.insert(result, item) end
	end
	return result
end

local function getGlobalSingleton(name)
	local classObject = rawget(_G, name)
	if not classObject then return nil end
	local ok, instance = pcall(function() return classObject.instance end)
	if not ok or instance == nil then return classObject end
	if type(instance) == "function" then
		local called, value = pcall(instance)
		if not called then called, value = pcall(instance, classObject) end
		if called then return value end
		return nil
	end
	return instance
end

local function lower(value)
	return string.lower(tostring(value or ""))
end

function VehicleSelect:__init(component)
	UIDialog.__init(self, component)
	self.component = component

	self.showAllModels = false
	self.showHelicopters = false
	self.showPlanes = false
	self.showDrones = false
	self.showCars = false
	self.enableAll = false
	self.activeFilter = FILTER_ALL
	self.numItemsWidth = 5
	self.borderWidth = 20.0
	self.borderHeight = 20.0
	self.carModelType = 4

	-- ModelHangerPanel controls.
	self.workbenchButton = nil
	self.flyButton = nil
	self.flyText = nil
	self.driveText = nil
	self.modelIcons = {}

	-- ModelHangerDialog controls and child panels.
	self.itemPrefab = nil
	self.scrollContent = nil
	self.modelSelectPanel = nil
	self.myModelsPanel = nil
	self.onlineModelsPanel = nil
	self.steamWorkshopBrowser = nil
	self.panelToggleGroup = nil
	self.modelNameText = nil

	self.applicationController = nil
	self.loadingManager = nil
	self.uiManager = nil
	self.uiStateManager = nil
	self.modelManager = nil
	self.directoryProvider = nil

	self.onSelectionChanged = nil
	self.onFilterChanged = nil
	self.onModelLaunch = nil
	self.onWorkshopImported = nil
	self.toggleSubscribed = false
	self.workshopSubscribed = false
	self.thumbnailBundleLoaded = false
	self.isEnabled = false
	self.lastSelectedModel = nil
	self.lastButtonsEnabled = nil
	self.lastError = ""

	self.toggleCallback = function() return self:clickPanelToggleGroup() end
	self.workshopCallback = function(args) return self:onItemPlayButtonClick(args) end
end

function VehicleSelect:__finalize()
	pcall(function() self:onDisable() end)
	self:unbindModelIcons()
	self.toggleCallback = nil
	self.workshopCallback = nil
	self.modelIcons = {}
	UIDialog.__finalize(self)
end

function VehicleSelect:setWorkbenchButton(value) self.workbenchButton = value end
function VehicleSelect:setFlyButton(value) self.flyButton = value end
function VehicleSelect:setFlyText(value) self.flyText = value end
function VehicleSelect:setDriveText(value) self.driveText = value end
function VehicleSelect:bindModelIcon(icon)
	if not icon then return false end
	local callback = function(selectedIcon) return self:clickModelIcon(selectedIcon or icon) end
	local called = tryCall(icon,
		{ "setSelectedCallback", "SetSelectedCallback", "setModelSelectedCallback", "SetModelSelectedCallback" }, callback)
	if called then pcall(function() icon.__vehicleSelectCallback = callback end) end
	return called
end
function VehicleSelect:unbindModelIcons()
	for _, icon in ipairs(self.modelIcons) do
		tryCall(icon, { "setSelectedCallback", "SetSelectedCallback", "setModelSelectedCallback", "SetModelSelectedCallback" }, nil)
		pcall(function() icon.__vehicleSelectCallback = nil end)
	end
end
function VehicleSelect:setModelIcons(value)
	self:unbindModelIcons()
	self.modelIcons = toArray(value)
	for _, icon in ipairs(self.modelIcons) do self:bindModelIcon(icon) end
end
function VehicleSelect:addModelIcon(value)
	if not value then return false end
	table.insert(self.modelIcons, value)
	self:bindModelIcon(value)
	return true
end
function VehicleSelect:clearModelIcons() self:unbindModelIcons(); self.modelIcons = {} end
function VehicleSelect:setItemPrefab(value) self.itemPrefab = value end
function VehicleSelect:setScrollContent(value) self.scrollContent = value end
function VehicleSelect:configureChildPanel(panel)
	if not panel or panel == self then return false end
	local callback = function(first, second, third)
		if first == panel then return self:onPanelModelSelected(second, third) end
		return self:onPanelModelSelected(first, second)
	end
	local called = tryCall(panel,
		{ "setModelSelectedCallback", "SetModelSelectedCallback", "setSelectionChangedCallback", "SetSelectionChangedCallback" }, callback)
	if called then pcall(function() panel.__vehicleSelectCallback = callback end) end
	return called
end
function VehicleSelect:setModelSelectPanel(value) self.modelSelectPanel = value; self:configureChildPanel(value) end
function VehicleSelect:setMyModelsPanel(value) self.myModelsPanel = value; self:configureChildPanel(value) end
function VehicleSelect:setOnlineModelsPanel(value) self.onlineModelsPanel = value; self:configureChildPanel(value) end
function VehicleSelect:setSteamWorkshopBrowser(value) self.steamWorkshopBrowser = value end
function VehicleSelect:setPanelToggleGroup(value) self.panelToggleGroup = value end
function VehicleSelect:setModelNameText(value) self.modelNameText = value end
function VehicleSelect:setApplicationController(value) self.applicationController = value end
function VehicleSelect:setLoadingManager(value) self.loadingManager = value end
function VehicleSelect:setUIManager(value) self.uiManager = value end
function VehicleSelect:setUIStateManager(value) self.uiStateManager = value end
function VehicleSelect:setModelManager(value) self.modelManager = value end
function VehicleSelect:setDirectoryProvider(value) self.directoryProvider = value end
function VehicleSelect:setSelectionChangedCallback(value) self.onSelectionChanged = value end
function VehicleSelect:setFilterChangedCallback(value) self.onFilterChanged = value end
function VehicleSelect:setModelLaunchCallback(value) self.onModelLaunch = value end
function VehicleSelect:setWorkshopImportedCallback(value) self.onWorkshopImported = value end
function VehicleSelect:setCarModelType(value) self.carModelType = tonumber(value) or self.carModelType end
function VehicleSelect:getShowAllModels() return self.showAllModels end
function VehicleSelect:getShowHelicopters() return self.showHelicopters end
function VehicleSelect:getShowPlanes() return self.showPlanes end
function VehicleSelect:getShowDrones() return self.showDrones end
function VehicleSelect:getShowCars() return self.showCars end
function VehicleSelect:getEnableAll() return self.enableAll end
function VehicleSelect:setShowAllModels(value) self.showAllModels = value == true end
function VehicleSelect:setShowHelicopters(value) self.showHelicopters = value == true end
function VehicleSelect:setShowPlanes(value) self.showPlanes = value == true end
function VehicleSelect:setShowDrones(value) self.showDrones = value == true end
function VehicleSelect:setShowCars(value) self.showCars = value == true end
function VehicleSelect:setEnableAll(value) self.enableAll = value == true end

function VehicleSelect:getApplicationManager()
	return self.applicationController or getGlobalSingleton("ApplicationManager") or getGlobalSingleton("IApplicationManager")
end

function VehicleSelect:getLoadingManager()
	if self.loadingManager then return self.loadingManager end
	local called, value = tryCall(self:getApplicationManager(), { "getLoadingManager", "GetLoadingManager" })
	return called and value or getGlobalSingleton("LoadingManager")
end

function VehicleSelect:getUIManager()
	return self.uiManager or getGlobalSingleton("UIManager")
end

function VehicleSelect:getUIStateManager()
	if self.uiStateManager then return self.uiStateManager end
	local called, value = tryCall(self:getApplicationManager(), { "getUIStateManager", "GetUIStateManager" })
	return called and value or getGlobalSingleton("UIStateManager")
end

function VehicleSelect:getModelManager()
	return self.modelManager or getGlobalSingleton("ModelManager")
end

function VehicleSelect:getSelectedModelData()
	return readMember(self:getApplicationManager(),
		{ "getSelectedModelData", "GetSelectedModelData" }, { "selectedModelData" }, nil)
end

function VehicleSelect:getModelValue(model, name, defaultValue)
	local suffix = string.upper(string.sub(name, 1, 1)) .. string.sub(name, 2)
	return readMember(model, { "get" .. suffix, "Get" .. suffix }, { name, "m_" .. suffix }, defaultValue)
end

function VehicleSelect:setObjectEnabled(object, enabled)
	if not object then return false end
	local target = readMember(object, { "getGameObject", "GetGameObject" }, { "gameObject" }, object)
	local called = tryCall(target,
		{ "setActive", "SetActive", "setEnabled", "SetEnabled", "setVisible", "SetVisible" }, enabled == true)
	if called then return true end
	return writeMember(target, {}, { "active", "enabled", "visible" }, enabled == true)
end

function VehicleSelect:isObjectActive(object)
	if not object then return false end
	local target = readMember(object, { "getGameObject", "GetGameObject" }, { "gameObject" }, object)
	local called, value = tryCall(target,
		{ "isActiveInHierarchy", "IsActiveInHierarchy", "isActive", "IsActive", "isEnabled", "IsEnabled" })
	if called then return value == true end
	return asBoolean(readMember(target, {},
		{ "activeInHierarchy", "activeSelf", "active", "enabled", "visible" }, false), false)
end

function VehicleSelect:setButtonInteractable(button, enabled)
	if not button then return false end
	local called = tryCall(button,
		{ "setInteractable", "SetInteractable", "setEnabled", "SetEnabled" }, enabled == true)
	if called then return true end
	return writeMember(button, {}, { "interactable", "enabled" }, enabled == true)
end

function VehicleSelect:setText(control, value)
	if not control then return false end
	local text = tostring(value or "")
	local called = tryCall(control, { "setText", "SetText", "setValue", "SetValue" }, text)
	if called then return true end
	return writeMember(control, {}, { "text", "value" }, text)
end

function VehicleSelect:getText(control)
	return tostring(readMember(control,
		{ "getText", "GetText", "getValue", "GetValue" }, { "text", "value" }, "") or "")
end

function VehicleSelect:updateActionButtons(force)
	local selected = self:getSelectedModelData()
	local enabled = selected ~= nil
	if force == true or enabled ~= self.lastButtonsEnabled then
		self:setButtonInteractable(self.workbenchButton, enabled)
		self:setButtonInteractable(self.flyButton, enabled)
		self.lastButtonsEnabled = enabled
	end
	self.lastSelectedModel = selected
	return enabled
end

function VehicleSelect:getPanelList()
	local panels = {}
	if self.modelSelectPanel then table.insert(panels, self.modelSelectPanel) end
	if self.myModelsPanel then table.insert(panels, self.myModelsPanel) end
	if self.onlineModelsPanel then table.insert(panels, self.onlineModelsPanel) end
	return panels
end

function VehicleSelect:applyFilterToPanel(panel)
	if not panel then return false end
	local changed = false
	changed = writeMember(panel, { "setShowAllModels", "SetShowAllModels" }, { "showAllModels" }, self.showAllModels) or changed
	changed = writeMember(panel, { "setShowHelicopters", "SetShowHelicopters" }, { "showHelicopters" }, self.showHelicopters) or changed
	changed = writeMember(panel, { "setShowPlanes", "SetShowPlanes" }, { "showPlanes" }, self.showPlanes) or changed
	changed = writeMember(panel, { "setShowDrones", "SetShowDrones" }, { "showDrones" }, self.showDrones) or changed
	changed = writeMember(panel, { "setShowCars", "SetShowCars" }, { "showCars" }, self.showCars) or changed
	changed = writeMember(panel, { "setEnableAll", "SetEnableAll" }, { "enableAll" }, self.enableAll) or changed
	return changed
end

function VehicleSelect:setupPanel(panel)
	if not panel then return false end
	local called, result = tryCall(panel, { "setup", "Setup", "populate", "Populate", "refresh", "Refresh" })
	return called and result ~= false
end

function VehicleSelect:getActivePanel()
	for _, panel in ipairs(self:getPanelList()) do
		if panel and self:isObjectActive(panel) then return panel end
	end
	return self.modelSelectPanel or self.myModelsPanel or self.onlineModelsPanel
end

function VehicleSelect:populate()
	for _, panel in ipairs(self:getPanelList()) do self:applyFilterToPanel(panel) end
	local panel = self:getActivePanel()
	local populated = self:setupPanel(panel)
	if not panel and #self.modelIcons > 0 then populated = true end
	if self.onFilterChanged then
		local ok, errorMessage = pcall(self.onFilterChanged, self, self.activeFilter, panel)
		if not ok then self.lastError = "Filter callback failed: " .. tostring(errorMessage); return false end
	end
	if populated or not panel then self.lastError = "" end
	return populated or panel == nil
end

function VehicleSelect:setFilter(filter)
	filter = lower(filter)
	if filter ~= FILTER_HELICOPTERS and filter ~= FILTER_PLANES and filter ~= FILTER_DRONES and filter ~= FILTER_CARS then
		filter = FILTER_ALL
	end
	self.activeFilter = filter
	self.showAllModels = filter == FILTER_ALL
	self.showHelicopters = filter == FILTER_HELICOPTERS
	self.showPlanes = filter == FILTER_PLANES
	self.showDrones = filter == FILTER_DRONES
	self.showCars = filter == FILTER_CARS
	return self:populate()
end

function VehicleSelect:clickAllModels() return self:setFilter(FILTER_ALL) end
function VehicleSelect:clickHelicopters() return self:setFilter(FILTER_HELICOPTERS) end
function VehicleSelect:clickPlanes() return self:setFilter(FILTER_PLANES) end
function VehicleSelect:clickDrones() return self:setFilter(FILTER_DRONES) end
function VehicleSelect:clickCars() return self:setFilter(FILTER_CARS) end

function VehicleSelect:setup()
	return self:clickAllModels()
end

function VehicleSelect:isCarModel(setup)
	local value = self:getModelValue(setup, "modelType", nil)
	if tonumber(value) then return tonumber(value) == tonumber(self.carModelType) end
	local text = lower(value)
	return string.find(text, "car", 1, true) ~= nil or string.find(text, "truck", 1, true) ~= nil
end

function VehicleSelect:selectPanelIcon(item)
	if not item then self.lastError = "A vehicle icon is required"; return false end
	for _, icon in ipairs(self.modelIcons) do
		if icon and icon ~= item then tryCall(icon, { "deselect", "Deselect" }) end
	end
	local selected = tryCall(item, { "select", "Select" })
	local setup = readMember(item,
		{ "getModelSetup", "GetModelSetup" }, { "modelSetup" }, self:getSelectedModelData())
	local isCar = self:isCarModel(setup)
	self:setObjectEnabled(self.flyText, not isCar)
	self:setObjectEnabled(self.driveText, isCar)
	return selected
end

function VehicleSelect:onPanelModelSelected(item, setup)
	setup = setup or self:getSelectedModelData() or readMember(item,
		{ "getModelSetup", "GetModelSetup" }, { "modelSetup" }, nil)
	if setup then self:setText(self.modelNameText, self:getModelValue(setup, "modelName", "")) end
	self:updateActionButtons(true)
	if self.onSelectionChanged then
		local ok, errorMessage = pcall(self.onSelectionChanged, self, item, setup)
		if not ok then self.lastError = "Selection callback failed: " .. tostring(errorMessage); return false end
	end
	self.lastError = ""
	return true
end

function VehicleSelect:clickModelIcon(item)
	if not item then self.lastError = "A vehicle icon is required"; return false end
	local selectedData = self:getSelectedModelData() or readMember(item,
		{ "getModelSetup", "GetModelSetup" }, { "modelSetup" }, nil)
	if selectedData then
		self:setText(self.modelNameText, self:getModelValue(selectedData, "modelName", ""))
	end

	local handled = self:selectPanelIcon(item)
	local selectionPanels = {}
	if self.modelSelectPanel then table.insert(selectionPanels, self.modelSelectPanel) end
	if self.myModelsPanel then table.insert(selectionPanels, self.myModelsPanel) end
	for _, panel in ipairs(selectionPanels) do
		if panel and panel ~= self then
			local called, result = tryCall(panel, { "clickModelIcon", "ClickModelIcon" }, item)
			if called and result ~= false then handled = true end
		end
	end
	self:updateActionButtons(true)
	if self.onSelectionChanged then
		local ok, errorMessage = pcall(self.onSelectionChanged, self, item, selectedData)
		if not ok then self.lastError = "Selection callback failed: " .. tostring(errorMessage); return false end
	end
	self.lastError = ""
	return handled
end

function VehicleSelect:getToggleIndex()
	local value = readMember(self.panelToggleGroup,
		{ "getButtonIndex", "GetButtonIndex", "getSelectedIndex", "GetSelectedIndex" },
		{ "buttonIndex", "selectedIndex" }, 0)
	return math.max(0, math.min(2, math.floor(tonumber(value) or 0)))
end

function VehicleSelect:setToggleIndex(index)
	index = math.max(0, math.min(2, math.floor(tonumber(index) or 0)))
	local called = tryCall(self.panelToggleGroup,
		{ "setButtonActive", "SetButtonActive", "setSelectedIndex", "SetSelectedIndex" }, index)
	return called
end

function VehicleSelect:clickPanelToggleGroup(index)
	index = index ~= nil and math.floor(tonumber(index) or 0) or self:getToggleIndex()
	index = math.max(0, math.min(2, index))
	local panels = { self.modelSelectPanel, self.myModelsPanel, self.onlineModelsPanel }
	for panelIndex = 1, 3 do self:setObjectEnabled(panels[panelIndex], panelIndex - 1 == index) end
	local active = panels[index + 1]
	self:applyFilterToPanel(active)
	self:setupPanel(active)
	return active ~= nil
end

function VehicleSelect:subscribeToggle()
	if self.toggleSubscribed or not self.panelToggleGroup then return false end
	local called = tryCall(self.panelToggleGroup,
		{ "addButtonToggledListener", "AddButtonToggledListener", "addListener", "AddListener", "subscribe", "Subscribe" },
		self.toggleCallback)
	if not called then
		local event = readMember(self.panelToggleGroup, {}, { "buttonToggled", "ButtonToggled" }, nil)
		called = tryCall(event, { "addListener", "AddListener", "add", "Add", "subscribe", "Subscribe" }, self.toggleCallback)
	end
	self.toggleSubscribed = called
	return called
end

function VehicleSelect:unsubscribeToggle()
	if not self.toggleSubscribed or not self.panelToggleGroup then return false end
	local called = tryCall(self.panelToggleGroup,
		{ "removeButtonToggledListener", "RemoveButtonToggledListener", "removeListener", "RemoveListener", "unsubscribe", "Unsubscribe" },
		self.toggleCallback)
	if not called then
		local event = readMember(self.panelToggleGroup, {}, { "buttonToggled", "ButtonToggled" }, nil)
		called = tryCall(event, { "removeListener", "RemoveListener", "remove", "Remove", "unsubscribe", "Unsubscribe" }, self.toggleCallback)
	end
	self.toggleSubscribed = false
	return called
end

function VehicleSelect:subscribeWorkshop()
	if self.workshopSubscribed or not self.steamWorkshopBrowser then return false end
	local called = tryCall(self.steamWorkshopBrowser,
		{ "addPlayButtonClickListener", "AddPlayButtonClickListener", "addOnPlayButtonClick", "AddOnPlayButtonClick", "subscribe", "Subscribe" },
		self.workshopCallback)
	if not called then
		local event = readMember(self.steamWorkshopBrowser, {}, { "OnPlayButtonClick", "onPlayButtonClick" }, nil)
		called = tryCall(event, { "addListener", "AddListener", "add", "Add", "subscribe", "Subscribe" }, self.workshopCallback)
	end
	self.workshopSubscribed = called
	return called
end

function VehicleSelect:unsubscribeWorkshop()
	if not self.workshopSubscribed or not self.steamWorkshopBrowser then return false end
	local called = tryCall(self.steamWorkshopBrowser,
		{ "removePlayButtonClickListener", "RemovePlayButtonClickListener", "removeOnPlayButtonClick", "RemoveOnPlayButtonClick", "unsubscribe", "Unsubscribe" },
		self.workshopCallback)
	if not called then
		local event = readMember(self.steamWorkshopBrowser, {}, { "OnPlayButtonClick", "onPlayButtonClick" }, nil)
		called = tryCall(event, { "removeListener", "RemoveListener", "remove", "Remove", "unsubscribe", "Unsubscribe" }, self.workshopCallback)
	end
	self.workshopSubscribed = false
	return called
end

function VehicleSelect:start()
	UIDialog.start(self)
	self:subscribeToggle()
	self:updateActionButtons(true)
	return true
end

function VehicleSelect:update()
	UIDialog.update(self)
	self:updateActionButtons(false)
end

function VehicleSelect:onEnable()
	if self.isEnabled then return false end
	self.isEnabled = true
	self.thumbnailBundleLoaded = tryCall(self:getLoadingManager(), { "loadThumbsModelsBundle", "LoadThumbsModelsBundle" })
	self:setup()

	local index = 0
	if self.myModelsPanel and self:isObjectActive(self.myModelsPanel) then index = 1
	elseif self.onlineModelsPanel and self:isObjectActive(self.onlineModelsPanel) then index = 2 end
	self:setToggleIndex(index)
	self:clickPanelToggleGroup(index)
	self:subscribeToggle()
	self:subscribeWorkshop()
	self:updateActionButtons(true)
	return true
end

function VehicleSelect:onDisable()
	if not self.isEnabled and not self.workshopSubscribed and not self.toggleSubscribed then return false end
	self.isEnabled = false
	self:unsubscribeWorkshop()
	self:unsubscribeToggle()
	if self.thumbnailBundleLoaded then
		tryCall(self:getLoadingManager(), { "unloadThumbsModelsBundle", "UnloadThumbsModelsBundle" })
		self.thumbnailBundleLoaded = false
	end
	return true
end

function VehicleSelect:onBecameVisible()
	return self:setup()
end

function VehicleSelect:onDestroy()
	return self:onDisable()
end

function VehicleSelect:awake()
	return true
end

function VehicleSelect:clickClose()
	local called, result = tryCall(self:getUIStateManager(), { "hideModelHanger", "HideModelHanger", "hideVehicleSelect", "HideVehicleSelect" })
	if not called then return self:hide() end
	return result ~= false
end

function VehicleSelect:launchSelectedModel(workbench)
	local setup = self:getSelectedModelData()
	if not setup then self.lastError = "Select a model before continuing"; return false end
	local referenceId = tonumber(self:getModelValue(setup, "referenceId", nil))
	if not referenceId then self.lastError = "The selected model has no reference ID"; return false end
	local modelName = self:getText(self.modelNameText)
	if modelName == "" then modelName = tostring(self:getModelValue(setup, "modelName", "") or "") end
	local loading = self:getLoadingManager()
	local names = workbench and
		{ "selectReferenceModelAndGoToWB", "SelectReferenceModelAndGoToWB", "selectReferenceModelAndGoToWorkbench", "SelectReferenceModelAndGoToWorkbench" } or
		{ "selectReferenceModelAndGoToScene", "SelectReferenceModelAndGoToScene", "selectReferenceModelAndFly", "SelectReferenceModelAndFly" }
	local called, result = tryCall(loading, names, math.floor(referenceId), modelName)
	if not called or result == false then
		self.lastError = workbench and "Could not open the selected model in the workbench" or "Could not launch the selected model"
		return false
	end
	if self.onModelLaunch then
		local ok, errorMessage = pcall(self.onModelLaunch, self, setup, math.floor(referenceId), modelName, workbench == true)
		if not ok then self.lastError = "Model launch callback failed: " .. tostring(errorMessage); return false end
	end
	self.lastError = ""
	return true
end

function VehicleSelect:clickSelectModelWB() return self:launchSelectedModel(true) end
function VehicleSelect:clickSelectModel() return self:launchSelectedModel(false) end

function VehicleSelect:getWorkshopItem(args)
	return readMember(args, { "getItem", "GetItem" }, { "Item", "item" }, args)
end

function VehicleSelect:getWorkshopModelFiles(folder, item)
	local provider = self.directoryProvider
	local called, files
	if type(provider) == "function" then
		called, files = pcall(provider, folder, "*.mxp", item)
	else
		called, files = tryCall(provider, { "getFiles", "GetFiles", "findFiles", "FindFiles" }, folder, "*.mxp", false)
	end
	if called then return toArray(files) end

	called, files = tryCall(self.steamWorkshopBrowser,
		{ "getInstalledModelFiles", "GetInstalledModelFiles", "getModelFiles", "GetModelFiles" }, item, ".mxp")
	if called then return toArray(files) end

	local system = rawget(_G, "System")
	local directory = readMember(system, {}, { "IO" }, nil)
	directory = readMember(directory, {}, { "Directory" }, nil)
	called, files = tryStatic(directory, { "GetFiles", "getFiles" }, folder, "*.mxp")
	return called and toArray(files) or {}
end

function VehicleSelect:showInformation(message, duration)
	local called = tryCall(self:getUIManager(), { "showInformation", "ShowInformation" }, tostring(message), tonumber(duration) or 5.0)
	return called
end

function VehicleSelect:onItemPlayButtonClick(args)
	local item = self:getWorkshopItem(args)
	if not item then self.lastError = "Workshop item data is unavailable"; return false end
	local folder = tostring(readMember(item,
		{ "getInstalledLocalFolder", "GetInstalledLocalFolder" }, { "InstalledLocalFolder", "installedLocalFolder" }, "") or "")
	local itemName = tostring(readMember(item, { "getName", "GetName" }, { "Name", "name" }, "Model") or "Model")
	if folder == "" then self.lastError = "The workshop item is not installed locally"; self:showInformation("Error importing model.", 5.0); return false end

	local files = self:getWorkshopModelFiles(folder, item)
	local imported = 0
	local manager = self:getModelManager()
	for _, fileName in ipairs(files) do
		if lower(fileName):sub(-4) == ".mxp" then
			local called, result = tryCall(manager, { "importModel", "ImportModel" }, fileName)
			if called and result ~= false then imported = imported + 1 end
		end
	end
	if imported == 0 then
		self.lastError = "No model package could be imported from the workshop item"
		self:showInformation("Error importing model.", 5.0)
		return false
	end

	self:showInformation(itemName .. " has been imported to My Models.", 5.0)
	self:setupPanel(self.myModelsPanel)
	if self.onWorkshopImported then
		local ok, errorMessage = pcall(self.onWorkshopImported, self, item, imported, files)
		if not ok then self.lastError = "Workshop import callback failed: " .. tostring(errorMessage); return false end
	end
	self.lastError = ""
	return true
end

function VehicleSelect:toggleEnableAll()
	self.enableAll = not self.enableAll
	for _, panel in ipairs(self:getPanelList()) do self:applyFilterToPanel(panel) end
	self:setupPanel(self:getActivePanel())
	return self.enableAll
end

function VehicleSelect:getProperties(parameters)
	UIDialog.getProperties(self, parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("activeFilter", self.activeFilter)
	properties:setPropertyAsBool("showAllModels", self.showAllModels)
	properties:setPropertyAsBool("showHelicopters", self.showHelicopters)
	properties:setPropertyAsBool("showPlanes", self.showPlanes)
	properties:setPropertyAsBool("showDrones", self.showDrones)
	properties:setPropertyAsBool("showCars", self.showCars)
	properties:setPropertyAsBool("enableAll", self.enableAll)
	properties:setPropertyAsInt("numItemsWidth", self.numItemsWidth)
	properties:setPropertyAsFloat("borderWidth", self.borderWidth)
	properties:setPropertyAsFloat("borderHeight", self.borderHeight)
	properties:setPropertyAsInt("modelIconCount", #self.modelIcons)
	properties:setPropertyAsBool("hasSelection", self:getSelectedModelData() ~= nil)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("All Models", false)
	properties:setButtonPressed("Helicopters", false)
	properties:setButtonPressed("Planes", false)
	properties:setButtonPressed("Drones", false)
	properties:setButtonPressed("Cars", false)
	properties:setButtonPressed("Toggle Enable All", false)
	properties:setButtonPressed("Select Model", false)
	properties:setButtonPressed("Open Workbench", false)
end

function VehicleSelect:setProperties(parameters)
	UIDialog.setProperties(self, parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("enableAll") then self.enableAll = properties:getPropertyAsBool("enableAll") end
	if properties:hasProperty("numItemsWidth") then self.numItemsWidth = math.max(1, properties:getPropertyAsInt("numItemsWidth")) end
	if properties:hasProperty("borderWidth") then self.borderWidth = math.max(0.0, properties:getPropertyAsFloat("borderWidth")) end
	if properties:hasProperty("borderHeight") then self.borderHeight = math.max(0.0, properties:getPropertyAsFloat("borderHeight")) end
	if properties:isButtonPressed("All Models") then self:clickAllModels() end
	if properties:isButtonPressed("Helicopters") then self:clickHelicopters() end
	if properties:isButtonPressed("Planes") then self:clickPlanes() end
	if properties:isButtonPressed("Drones") then self:clickDrones() end
	if properties:isButtonPressed("Cars") then self:clickCars() end
	if properties:isButtonPressed("Toggle Enable All") then self:toggleEnableAll() end
	if properties:isButtonPressed("Select Model") then self:clickSelectModel() end
	if properties:isButtonPressed("Open Workbench") then self:clickSelectModelWB() end
end

-- Compatibility aliases for ModelHangerPanel.cs, ModelHangerDialog.cs and Unity wiring.
VehicleSelect.Start = VehicleSelect.start
VehicleSelect.Update = VehicleSelect.update
VehicleSelect.Setup = VehicleSelect.setup
VehicleSelect.Populate = VehicleSelect.populate
VehicleSelect.OnEnable = VehicleSelect.onEnable
VehicleSelect.OnDisable = VehicleSelect.onDisable
VehicleSelect.OnDestroy = VehicleSelect.onDestroy
VehicleSelect.Awake = VehicleSelect.awake
VehicleSelect.OnBecameVisible = VehicleSelect.onBecameVisible
VehicleSelect.ClickPanelToggleGroup = VehicleSelect.clickPanelToggleGroup
VehicleSelect.ClickAllModels = VehicleSelect.clickAllModels
VehicleSelect.ClickHelicopters = VehicleSelect.clickHelicopters
VehicleSelect.ClickPlanes = VehicleSelect.clickPlanes
VehicleSelect.ClickDrones = VehicleSelect.clickDrones
VehicleSelect.ClickCars = VehicleSelect.clickCars
VehicleSelect.ClickClose = VehicleSelect.clickClose
VehicleSelect.ClickSelectModelWB = VehicleSelect.clickSelectModelWB
VehicleSelect.ClickSelectModel = VehicleSelect.clickSelectModel
VehicleSelect.ClickModelIcon = VehicleSelect.clickModelIcon
VehicleSelect.OnPanelModelSelected = VehicleSelect.onPanelModelSelected
VehicleSelect.OnItemPlayButtonClick = VehicleSelect.onItemPlayButtonClick
