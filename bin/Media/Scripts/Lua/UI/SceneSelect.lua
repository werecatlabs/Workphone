include("UIDialog.lua")

class 'SceneSelect' (UIDialog)

local FILTER_ALL = "all"
local FILTER_PHOTO = "photo"
local FILTER_3D = "3d"
local FILTER_RACE = "race"
local FILTER_CUSTOM = "custom"
local FILTER_ONLINE = "online"

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

local function asBoolean(value, defaultValue)
	if value == nil then return defaultValue == true end
	if type(value) == "boolean" then return value end
	if type(value) == "number" then return value ~= 0 end
	value = string.lower(tostring(value))
	if value == "true" or value == "yes" or value == "on" or value == "1" then return true end
	if value == "false" or value == "no" or value == "off" or value == "0" or value == "" then return false end
	return defaultValue == true
end

local function toArray(value)
	if not value then return {} end
	if type(value) == "table" then
		local result = {}
		for _, item in ipairs(value) do table.insert(result, item) end
		return result
	end
	local called, size = tryCall(value, { "size", "Size", "getSize", "GetSize", "count", "Count" })
	if not called or not tonumber(size) then return {} end
	local result = {}
	for index = 0, tonumber(size) - 1 do
		local found, item = tryCall(value, { "at", "At", "get", "Get", "getItem", "GetItem" }, index)
		if found and item then table.insert(result, item) end
	end
	return result
end

local function sqlEscape(value)
	return string.gsub(tostring(value), "'", "''")
end

local function getGlobalSingleton(name)
	local ok, value = pcall(function()
		local classObject = _G and _G[name]
		if not classObject then return nil end
		local instance = classObject.instance
		if instance == nil then return classObject end
		if type(instance) == "function" then
			local called, result = pcall(instance)
			if called then return result end
			called, result = pcall(instance, classObject)
			if called then return result end
		end
		return instance
	end)
	return ok and value or nil
end

function SceneSelect:__init(component)
	UIDialog.__init(self, component)
	self.component = component
	self.showAllScenery = false
	self.showPhotoScenery = false
	self.show3dScenery = false
	self.showRaceScenery = false
	self.showCustomScenery = false
	self.activeFilter = FILTER_ALL

	self.numItemsWidth = 5
	self.itemWidth = 240.0
	self.itemHeight = 160.0
	self.borderWidth = 20.0
	self.borderHeight = 20.0
	self.photoSceneType = 0
	self.threeDSceneType = 1
	self.editorSceneId = 35
	self.flightState = nil
	self.workbenchState = nil

	self.itemPrefab = nil
	self.scrollContent = nil
	self.customScrollContent = nil
	self.onlineScrollContent = nil
	self.scenerySelectPanel = nil
	self.raceSelectPanel = nil
	self.builtInSelectPanel = nil
	self.customSelectPanel = nil
	self.onlineScenesPanel = nil
	self.selectButton = nil

	self.applicationController = nil
	self.sceneryManager = nil
	self.loadingManager = nil
	self.uiManager = nil
	self.uiStateManager = nil
	self.raceManager = nil
	self.multiplayerConnector = nil
	self.settingsStore = nil
	self.databaseManager = nil
	self.itemFactory = nil
	self.thumbnailProvider = nil
	self.onlineSceneProvider = nil
	self.onSceneListChanged = nil
	self.onSceneLaunched = nil

	self.sceneries = nil
	self.customSceneries = nil
	self.items = {}
	self.isEnabled = false
	self.isPopulating = false
	self.lastFilter = nil
	self.lastRaceUiState = nil
	self.lastError = ""
end

function SceneSelect:__finalize()
	pcall(function() self:onDisable() end)
	UIDialog.__finalize(self)
end

function SceneSelect:setItemPrefab(value) self.itemPrefab = value end
function SceneSelect:setScrollContent(value) self.scrollContent = value end
function SceneSelect:setCustomScrollContent(value) self.customScrollContent = value end
function SceneSelect:setOnlineScrollContent(value) self.onlineScrollContent = value end
function SceneSelect:setScenerySelectPanel(value) self.scenerySelectPanel = value end
function SceneSelect:setRaceSelectPanel(value) self.raceSelectPanel = value end
function SceneSelect:setBuiltInSelectPanel(value) self.builtInSelectPanel = value end
function SceneSelect:setCustomSelectPanel(value) self.customSelectPanel = value end
function SceneSelect:setOnlineScenesPanel(value) self.onlineScenesPanel = value end
function SceneSelect:setSelectButton(value) self.selectButton = value end
function SceneSelect:setApplicationController(value) self.applicationController = value end
function SceneSelect:setSceneryManager(value) self.sceneryManager = value end
function SceneSelect:setLoadingManager(value) self.loadingManager = value end
function SceneSelect:setUIManager(value) self.uiManager = value end
function SceneSelect:setUIStateManager(value) self.uiStateManager = value end
function SceneSelect:setRaceManager(value) self.raceManager = value end
function SceneSelect:setMultiplayerConnector(value) self.multiplayerConnector = value end
function SceneSelect:setSettingsStore(value) self.settingsStore = value end
function SceneSelect:setDatabaseManager(value) self.databaseManager = value end
function SceneSelect:setItemFactory(value) self.itemFactory = value end
function SceneSelect:setThumbnailProvider(value) self.thumbnailProvider = value end
function SceneSelect:setOnlineSceneProvider(value) self.onlineSceneProvider = value end
function SceneSelect:setSceneries(value) self.sceneries = value end
function SceneSelect:setCustomSceneries(value) self.customSceneries = value end
function SceneSelect:setSceneListChangedCallback(value) self.onSceneListChanged = value end
function SceneSelect:setSceneLaunchedCallback(value) self.onSceneLaunched = value end
function SceneSelect:setFlightState(value) self.flightState = value end
function SceneSelect:setWorkbenchState(value) self.workbenchState = value end

function SceneSelect:getApplicationManager()
	if self.applicationController then return self.applicationController end
	local ok, manager = pcall(function()
		if ApplicationManager then
			local instance = ApplicationManager.instance
			if type(instance) == "function" then
				local called, value = pcall(instance)
				if called then return value end
			else return instance end
		end
		return nil
	end)
	if ok and manager then return manager end
	ok, manager = pcall(function() return IApplicationManager.instance() end)
	return ok and manager or nil
end

function SceneSelect:getDatabase()
	if self.settingsStore then return self.settingsStore end
	local called, database = tryCall(self:getApplicationManager(), { "getDatabase", "GetDatabase" })
	return called and database or nil
end

function SceneSelect:getManager(explicit, getterNames)
	if explicit then return explicit end
	local called, value = tryCall(self:getApplicationManager(), getterNames)
	return called and value or nil
end

function SceneSelect:getSceneryManager()
	return self:getManager(self.sceneryManager, { "getSceneryManager", "GetSceneryManager" }) or
		getGlobalSingleton("SceneryManager")
end

function SceneSelect:getLoadingManager()
	return self:getManager(self.loadingManager, { "getLoadingManager", "GetLoadingManager" }) or
		getGlobalSingleton("LoadingManager")
end

function SceneSelect:getUIManager()
	if self.uiManager then return self.uiManager end
	local ok, value = pcall(function()
		if UIManager and UIManager.instance then return UIManager.instance() end
		return nil
	end)
	return ok and value or nil
end

function SceneSelect:getUIStateManager()
	return self:getManager(self.uiStateManager, { "getUIStateManager", "GetUIStateManager" }) or
		getGlobalSingleton("UIStateManager")
end

function SceneSelect:getRaceManager()
	return self:getManager(self.raceManager, { "getRaceManager", "GetRaceManager" }) or
		getGlobalSingleton("RaceManager")
end

function SceneSelect:setObjectEnabled(object, enabled)
	if not object then return false end
	local called = tryCall(object,
		{ "setActive", "SetActive", "setEnabled", "SetEnabled", "setVisible", "SetVisible" }, enabled == true)
	return called
end

function SceneSelect:setButtonEnabled(enabled)
	if not self.selectButton then return false end
	local called = tryCall(self.selectButton,
		{ "setInteractable", "SetInteractable", "setEnabled", "SetEnabled" }, enabled == true)
	if called then return true end
	return pcall(function() self.selectButton.interactable = enabled == true end)
end

function SceneSelect:destroyChildren(parent)
	if not parent then return false end
	local called = tryCall(parent,
		{ "destroyChildren", "DestroyChildren", "destroyAllChildren", "DestroyAllChildren", "removeChildren", "RemoveChildren" })
	if called then return true end
	local actorFound, actor = tryCall(parent, { "getActor", "GetActor" })
	if actorFound and actor then return tryCall(actor, { "destroyChildren", "DestroyChildren" }) end
	return false
end

function SceneSelect:getSceneValue(scene, name, defaultValue)
	local getter = "get" .. string.upper(string.sub(name, 1, 1)) .. string.sub(name, 2)
	return readMember(scene, { getter, "Get" .. string.sub(getter, 4) }, { name }, defaultValue)
end

function SceneSelect:getAllSceneries()
	if self.sceneries then return toArray(self.sceneries) end
	local manager = self:getSceneryManager()
	local called, scenes = tryCall(manager, { "getSceneries", "GetSceneries", "getScenes", "GetScenes" })
	if not called then
		local ok, direct = pcall(function() return manager and manager.sceneries end)
		if ok then scenes = direct end
	end
	return toArray(scenes)
end

function SceneSelect:getCustomSceneList()
	if self.customSceneries then return toArray(self.customSceneries) end
	local manager = self:getSceneryManager()
	local called, scenes = tryCall(manager,
		{ "getCustomSceneries", "GetCustomSceneries", "getCustomScenes", "GetCustomScenes" })
	if called then return toArray(scenes) end
	local result = {}
	for _, scene in ipairs(self:getAllSceneries()) do
		if asBoolean(self:getSceneValue(scene, "isCustomScene", false), false) or
			asBoolean(self:getSceneValue(scene, "isSceneEditor", false), false) then
			table.insert(result, scene)
		end
	end
	return result
end

function SceneSelect:getOnlineSceneList()
	local called, scenes = tryCall(self.onlineSceneProvider,
		{ "getSceneries", "GetSceneries", "getScenes", "GetScenes", "refresh", "Refresh" })
	return called and toArray(scenes) or {}
end

function SceneSelect:isSceneType(scene, expected, textName)
	local value = self:getSceneValue(scene, "sceneType", self.photoSceneType)
	if tonumber(value) then return tonumber(value) == expected end
	return string.find(string.lower(tostring(value)), textName, 1, true) ~= nil
end

function SceneSelect:sceneMatchesFilter(scene, filter)
	if not scene or asBoolean(self:getSceneValue(scene, "isWorkbench", false), false) then return false end
	local isEditor = asBoolean(self:getSceneValue(scene, "isSceneEditor", false), false)
	local isCustom = asBoolean(self:getSceneValue(scene, "isCustomScene", false), false)
	if filter == FILTER_CUSTOM then return isCustom or isEditor end
	if filter == FILTER_ONLINE then return true end
	if isEditor or isCustom then return false end
	if filter == FILTER_ALL then return true end
	if filter == FILTER_PHOTO then return self:isSceneType(scene, self.photoSceneType, "photo") end
	if filter == FILTER_3D then return self:isSceneType(scene, self.threeDSceneType, "3d") end
	if filter == FILTER_RACE then return asBoolean(self:getSceneValue(scene, "isRaceTrack", false), false) end
	return false
end

function SceneSelect:getFilteredSceneries(filter)
	local source
	if filter == FILTER_CUSTOM then source = self:getCustomSceneList()
	elseif filter == FILTER_ONLINE then source = self:getOnlineSceneList()
	else source = self:getAllSceneries() end
	local result = {}
	for _, scene in ipairs(source) do
		if self:sceneMatchesFilter(scene, filter) then table.insert(result, scene) end
	end
	return result
end

function SceneSelect:getGameManager()
	local called, manager = tryCall(self:getApplicationManager(), { "getGameManager", "GetGameManager" })
	return called and manager or nil
end

function SceneSelect:createItemActor(parent)
	local called, actor, icon
	called, actor = tryCall(self.itemPrefab, { "createActor", "CreateActor", "instantiate", "Instantiate" })
	if not called then
		local gameManager = self:getGameManager()
		called, actor = tryCall(gameManager, { "createActor", "CreateActor" })
	end
	if not called or not actor then return nil, nil end
	tryCall(parent, { "addChild", "AddChild" }, actor)
	if parent then
		local parentActorFound, parentActor = tryCall(parent, { "getActor", "GetActor" })
		if parentActorFound and parentActor then tryCall(parentActor, { "addChild", "AddChild" }, actor) end
	end

	local found, userComponent = tryCall(actor, { "getComponent", "GetComponent" }, "UserComponent")
	if not found or not userComponent then
		local typeOk, typeInfo = pcall(function() return UserComponent.typeInfo() end)
		if typeOk then found, userComponent = tryCall(actor, { "addComponentById", "AddComponentById" }, typeInfo) end
	end
	if userComponent then
		tryCall(userComponent, { "setClassName", "SetClassName" }, "SceneIcon")
		local hasClass, scriptClass = tryCall(userComponent, { "getScriptClass", "GetScriptClass" })
		if hasClass then icon = scriptClass end
		if not icon then icon = userComponent end
	end
	return actor, icon or userComponent
end

function SceneSelect:loadThumbnail(scene)
	local name = tostring(self:getSceneValue(scene, "thumbnailName", "") or "")
	if name == "" then return nil end
	local called, value = tryCall(self.thumbnailProvider,
		{ "loadThumbnail", "LoadThumbnail", "getThumbnail", "GetThumbnail", "load", "Load" }, name, scene)
	if called then return value end
	local loading = self:getLoadingManager()
	local _, bundle = tryCall(loading,
		{ "getSceneryThumbnailBundle", "GetSceneryThumbnailBundle", "getThumbsSceneryAssetBundle", "GetThumbsSceneryAssetBundle" })
	if not bundle then
		local ok, direct = pcall(function() return loading and loading.thumbsSceneryAssetBundle end)
		if ok then bundle = direct end
	end
	called, value = tryCall(bundle, { "loadAsset", "LoadAsset", "loadSprite", "LoadSprite" }, name)
	if called then return value end
	-- Workphone Image can resolve a texture name directly.
	return name
end

function SceneSelect:positionItem(actor, index)
	if not actor then return false end
	local found, transform = tryCall(actor, { "getComponent", "GetComponent" }, "LayoutTransform")
	if not found or not transform then return false end
	local columns = math.max(1, math.floor(tonumber(self.numItemsWidth) or 5))
	local totalWidth = self.itemWidth + self.borderWidth
	local totalHeight = self.itemHeight + self.borderHeight
	local column = index % columns
	local row = math.floor(index / columns)
	local centerX = ((columns - 1) * totalWidth) * 0.5
	local x = column * totalWidth - centerX
	local y = row * -totalHeight
	local ok, position = pcall(function() return Vector2F(x, y) end)
	if ok then tryCall(transform, { "setPosition", "SetPosition" }, position) end
	return ok
end

function SceneSelect:updateContentSize(parent, itemCount)
	if not parent then return false end
	local actor = parent
	local hasActor, resolved = tryCall(parent, { "getActor", "GetActor" })
	if hasActor and resolved then actor = resolved end
	local found, transform = tryCall(actor, { "getComponent", "GetComponent" }, "LayoutTransform")
	if not found or not transform then return false end
	local columns = math.max(1, math.floor(tonumber(self.numItemsWidth) or 5))
	local rows = math.max(1, math.ceil(itemCount / columns))
	local height = rows * (self.itemHeight + self.borderHeight) + self.borderHeight
	local width = columns * (self.itemWidth + self.borderWidth)
	local ok, size = pcall(function() return Vector2F(width, height) end)
	if ok then return tryCall(transform, { "setSize", "SetSize" }, size) end
	return false
end

function SceneSelect:createSceneItem(scene, index, parent)
	local called, actor, icon = false, nil, nil
	if self.itemFactory then
		called, actor = tryCall(self.itemFactory,
			{ "createSceneIcon", "CreateSceneIcon", "createItem", "CreateItem" }, scene, index, parent, self)
		if type(actor) == "table" and actor.actor then
			icon = actor.icon or actor.sceneIcon
			actor = actor.actor
		end
		if called and actor then
			local found, value = tryCall(actor, { "getSceneIcon", "GetSceneIcon", "getScriptClass", "GetScriptClass" })
			if found then icon = value end
		end
	end
	if not actor then actor, icon = self:createItemActor(parent) end
	if not actor then return nil end
	if not icon then
		local found, component = tryCall(actor, { "getComponent", "GetComponent" }, "UserComponent")
		if found and component then
			local hasClass, value = tryCall(component, { "getScriptClass", "GetScriptClass" })
			icon = hasClass and value or component
		end
	end
	if icon then
		tryCall(icon, { "setApplicationController", "SetApplicationController" }, self:getApplicationManager())
		tryCall(icon, { "setSceneSetup", "SetSceneSetup" }, scene)
		tryCall(icon, { "setSprite", "SetSprite" }, self:loadThumbnail(scene))
	end
	self:positionItem(actor, index)
	return { actor = actor, icon = icon, scene = scene }
end

function SceneSelect:populate(filter, parent)
	if self.isPopulating then return false end
	self.isPopulating = true
	self.lastError = ""
	local ok, result = pcall(function()
		self:destroyChildren(parent)
		self.items = {}
		local scenes = self:getFilteredSceneries(filter)
		for index, scene in ipairs(scenes) do
			local item = self:createSceneItem(scene, index - 1, parent)
			if item then table.insert(self.items, item) end
		end
		self:updateContentSize(parent, #self.items)
		self.lastFilter = filter
		if self.onSceneListChanged then
			local callbackOk, callbackError = pcall(self.onSceneListChanged, self, filter, self.items, scenes)
			if not callbackOk then error(callbackError) end
		end
		return #self.items
	end)
	self.isPopulating = false
	if not ok then
		self.lastError = "Unable to populate scenery list: " .. tostring(result)
		print("SceneSelect: " .. self.lastError)
		return false
	end
	return result
end

function SceneSelect:populateSceneries()
	return self:populate(self.activeFilter, self.scrollContent)
end

function SceneSelect:populateCustomSceneries()
	return self:populate(FILTER_CUSTOM, self.customScrollContent or self.scrollContent)
end

function SceneSelect:setFilter(filter)
	self.activeFilter = filter
	self.showAllScenery = filter == FILTER_ALL
	self.showPhotoScenery = filter == FILTER_PHOTO
	self.show3dScenery = filter == FILTER_3D
	self.showRaceScenery = filter == FILTER_RACE
	self.showCustomScenery = filter == FILTER_CUSTOM or filter == FILTER_ONLINE

	local builtIn = filter ~= FILTER_CUSTOM and filter ~= FILTER_ONLINE
	self:setObjectEnabled(self.builtInSelectPanel, builtIn)
	self:setObjectEnabled(self.customSelectPanel, filter == FILTER_CUSTOM)
	self:setObjectEnabled(self.onlineScenesPanel, filter == FILTER_ONLINE)
	if filter == FILTER_CUSTOM then return self:populateCustomSceneries() end
	if filter == FILTER_ONLINE then
		if self.onlineScrollContent then return self:populate(FILTER_ONLINE, self.onlineScrollContent) end
		local called, result = tryCall(self.onlineSceneProvider,
			{ "populate", "Populate", "show", "Show", "refresh", "Refresh" }, self.onlineScenesPanel, self)
		self.lastFilter = FILTER_ONLINE
		return called and result ~= false
	end
	return self:populateSceneries()
end

function SceneSelect:clickAllScenery() return self:setFilter(FILTER_ALL) end
function SceneSelect:clickPhotoScenery() return self:setFilter(FILTER_PHOTO) end
function SceneSelect:click3dScenery() return self:setFilter(FILTER_3D) end
function SceneSelect:clickRaceTrackScenery() return self:setFilter(FILTER_RACE) end
function SceneSelect:clickCustomScenery() return self:setFilter(FILTER_CUSTOM) end
function SceneSelect:clickOnlineScenes() return self:setFilter(FILTER_ONLINE) end

function SceneSelect:getSelectedScenery()
	local manager = self:getApplicationManager()
	local called, selected = tryCall(manager, { "getSelectedScenery", "GetSelectedScenery" })
	if called then return selected end
	local ok, direct = pcall(function() return manager and manager.selectedScenery end)
	return ok and direct or nil
end

function SceneSelect:setSelectedScenery(scene)
	local manager = self:getApplicationManager()
	local called, result = tryCall(manager, { "setSelectedScenery", "SetSelectedScenery" }, scene)
	if called then return result ~= false end
	return pcall(function() manager.selectedScenery = scene end)
end

function SceneSelect:isRaceMode()
	local manager = self:getApplicationManager()
	local race = readMember(manager, { "getIsRaceMode", "GetIsRaceMode" }, { "isRaceMode" }, false)
	local entering = readMember(manager,
		{ "getEnterRaceModeOnLevelLoad", "GetEnterRaceModeOnLevelLoad" }, { "enterRaceModeOnLevelLoad" }, false)
	return asBoolean(race, false) or asBoolean(entering, false)
end

function SceneSelect:isStartMenu()
	local ui = self:getUIManager()
	local value = readMember(ui, { "getIsStartMenu", "GetIsStartMenu" }, { "isStartMenu" }, nil)
	if value ~= nil then return asBoolean(value, false) end
	local manager = self:getApplicationManager()
	local called, result = tryCall(manager, { "isInStartMenu", "IsInStartMenu" })
	return called and result == true or false
end

function SceneSelect:getApplicationState()
	local manager = self:getApplicationManager()
	return readMember(manager, { "getApplicationState", "GetApplicationState" }, { "applicationState" }, "")
end

function SceneSelect:stateContains(name)
	local state = self:getApplicationState()
	if name == "flight" and self.flightState ~= nil and state == self.flightState then return true end
	if name == "work" and self.workbenchState ~= nil and state == self.workbenchState then return true end
	return string.find(string.lower(tostring(state)), string.lower(name), 1, true) ~= nil
end

function SceneSelect:setup()
	if self:isRaceMode() then return self:clickRaceTrackScenery() end
	return self:clickAllScenery()
end

function SceneSelect:onBecameVisible() return self:setup() end

function SceneSelect:onEnable()
	self.isEnabled = true
	tryCall(self:getLoadingManager(),
		{ "loadSceneThumbsBundle", "LoadSceneThumbsBundle", "loadSceneryThumbnails", "LoadSceneryThumbnails" })
	return self:setup()
end

function SceneSelect:onDisable()
	if not self.isEnabled then return true end
	self.isEnabled = false
	tryCall(self:getLoadingManager(),
		{ "unloadSceneThumbsBundle", "UnloadSceneThumbsBundle", "unloadSceneryThumbnails", "UnloadSceneryThumbnails" })
	return true
end

function SceneSelect:start()
	UIDialog.start(self)
	return self:setup()
end

function SceneSelect:updateRacePanels()
	local manager = self:getApplicationManager()
	if not manager then
		self:setObjectEnabled(self.scenerySelectPanel, false)
		self:setObjectEnabled(self.raceSelectPanel, true)
		return "unavailable"
	end
	if not self:isRaceMode() then
		self:setObjectEnabled(self.scenerySelectPanel, true)
		self:setObjectEnabled(self.raceSelectPanel, false)
		return "normal"
	end
	local raceManager = self:getRaceManager()
	local started = asBoolean(readMember(raceManager,
		{ "getRaceStarted", "GetRaceStarted", "isRaceStarted", "IsRaceStarted" }, { "raceStarted" }, false), false)
	if started then
		self:setObjectEnabled(self.scenerySelectPanel, true)
		self:setObjectEnabled(self.raceSelectPanel, false)
		return "race_started"
	end
	self:setObjectEnabled(self.scenerySelectPanel, false)
	self:setObjectEnabled(self.raceSelectPanel, true)
	if self.activeFilter ~= FILTER_RACE then self:clickRaceTrackScenery() end
	return "race_setup"
end

function SceneSelect:update()
	UIDialog.update(self)
	self:setButtonEnabled(self:getSelectedScenery() ~= nil)
	self.lastRaceUiState = self:updateRacePanels()
end

function SceneSelect:writeSetting(key, value)
	local store = self:getDatabase()
	if not store then return false end
	local text = tostring(value)
	local called, result = tryCall(store,
		{ "setSetting", "SetSetting", "setSettingAsString", "SetSettingAsString" }, key, text)
	if called then return result ~= false end
	local escapedKey, escapedValue = sqlEscape(key), sqlEscape(text)
	local updated, affected = tryCall(store, { "executeDML", "ExecuteDML" },
		"UPDATE settings SET value='" .. escapedValue .. "' WHERE param='" .. escapedKey .. "'")
	if not updated or (tonumber(affected) and tonumber(affected) < 0) then return false end
	if tonumber(affected) == 0 then
		local inserted, insertResult = tryCall(store, { "executeDML", "ExecuteDML" },
			"INSERT INTO settings (param, value) VALUES ('" .. escapedKey .. "', '" .. escapedValue .. "')")
		return inserted and (not tonumber(insertResult) or tonumber(insertResult) >= 0)
	end
	return true
end

function SceneSelect:writeSimState(key, value)
	local store = self:getDatabase()
	if not store then return false end
	local escapedKey, escapedValue = sqlEscape(key), sqlEscape(value)
	local updated, affected = tryCall(store, { "executeDML", "ExecuteDML" },
		"UPDATE sim_states SET value='" .. escapedValue .. "' WHERE param='" .. escapedKey .. "'")
	if not updated or (tonumber(affected) and tonumber(affected) < 0) then return false end
	if tonumber(affected) == 0 then
		local inserted = tryCall(store, { "executeDML", "ExecuteDML" },
			"INSERT INTO sim_states (param, value) VALUES ('" .. escapedKey .. "', '" .. escapedValue .. "')")
		return inserted
	end
	return true
end

function SceneSelect:persistSelectedScene(scene, editorMode)
	if not scene then return false end
	local sceneId = math.floor(tonumber(self:getSceneValue(scene, "sceneId", -1)) or -1)
	local isNight = asBoolean(self:getSceneValue(scene, "isNight", false), false)
	local editorSaved = self:writeSetting("IsSceneEditor", editorMode and "1" or "0")
	local sceneSaved = self:writeSimState("scenery_id", sceneId)
	local nightSaved = self:writeSetting("night_fly", isNight and "true" or "false")
	local db = self.databaseManager
	if not db then local _, value = tryCall(self:getApplicationManager(), { "getDatabaseManager", "GetDatabaseManager" }); db = value end
	tryCall(db, { "setCurrentSceneId", "SetCurrentSceneId" }, sceneId)
	return sceneId >= 0 and editorSaved and sceneSaved and nightSaved
end

function SceneSelect:showLoadingScreen()
	local ui = self:getUIManager()
	local called = tryCall(ui, { "showLoadingScreen", "ShowLoadingScreen" })
	return called
end

function SceneSelect:hideSelectorDialog()
	self:hide(true)
	tryCall(self:getUIStateManager(), { "hideScenerySelector", "HideScenerySelector" })
end

function SceneSelect:launchScene(scene, editorMode, action)
	if not scene then self.lastError = "Select a scenery before continuing"; return false end
	local persisted = self:persistSelectedScene(scene, editorMode)
	local manager = self:getApplicationManager()
	local sceneId = math.floor(tonumber(self:getSceneValue(scene, "sceneId", -1)) or -1)
	if self:isStartMenu() then
		local raceTrack = asBoolean(self:getSceneValue(scene, "isRaceTrack", false), false)
		if action == "new" and raceTrack and self:isRaceMode() then
			tryCall(manager, { "clickRaceSceneryStart", "ClickRaceSceneryStart" })
		else
			tryCall(manager, { "clickStartMenuCurrentScene", "ClickStartMenuCurrentScene", "startSelectedScene", "StartSelectedScene" })
		end
	else
		self:hideSelectorDialog()
		self:showLoadingScreen()
		if self:stateContains("flight") then
			tryCall(manager, { "loadCurrentScene", "LoadCurrentScene", "loadSelectedScene", "LoadSelectedScene" })
		elseif self:stateContains("work") then
			tryCall(manager, { "clickStartMenuCurrentScene", "ClickStartMenuCurrentScene", "startSelectedScene", "StartSelectedScene" })
		else
			tryCall(manager, { "loadSelectedScene", "LoadSelectedScene" }, scene)
		end
	end
	if self.onSceneLaunched then
		local ok, errorMessage = pcall(self.onSceneLaunched, self, scene, sceneId, editorMode, action)
		if not ok then self.lastError = "Scene launch callback failed: " .. tostring(errorMessage); return false end
	end
	if persisted then self.lastError = ""
	else self.lastError = "Scene launched, but its selection could not be fully persisted" end
	return true
end

function SceneSelect:getSceneSetup(sceneId)
	local manager = self:getApplicationManager()
	local called, scene = tryCall(manager, { "getSceneSetup", "GetSceneSetup" }, sceneId)
	if called and scene then return scene end
	for _, value in ipairs(self:getAllSceneries()) do
		if tonumber(self:getSceneValue(value, "sceneId", -1)) == sceneId then return value end
	end
	return nil
end

function SceneSelect:clickNewCustomScene()
	local scene = self:getSceneSetup(self.editorSceneId)
	if not scene then self.lastError = "Scene editor setup " .. self.editorSceneId .. " is unavailable"; return false end
	self:setSelectedScenery(scene)
	return self:launchScene(scene, true, "new")
end

function SceneSelect:clickEditCustomScene()
	return self:launchScene(self:getSelectedScenery(), true, "edit")
end

function SceneSelect:clickSelectScenery()
	local connector = self.multiplayerConnector or getGlobalSingleton("MultiplayerConnector")
	if connector then
		local connected = asBoolean(readMember(connector,
			{ "isConnected", "IsConnected", "getIsConnected", "GetIsConnected" }, { "isConnected" }, false), false)
		local createRoom = asBoolean(readMember(self:getApplicationManager(),
			{ "getCreateRoomOnLevelLoad", "GetCreateRoomOnLevelLoad" }, { "createRoomOnLevelLoad" }, false), false)
		if connected and createRoom then tryCall(connector, { "levelLoading", "LevelLoading" }) end
	end
	return self:launchScene(self:getSelectedScenery(), false, "select")
end

function SceneSelect:clickClose()
	local manager = self:getApplicationManager()
	local ui = self:getUIManager()
	if self:isStartMenu() and self:isRaceMode() then
		self:hide(true)
		local shown = tryCall(ui, { "showRaceSettingsMenu", "ShowRaceSettingsMenu" })
		if not shown then
			local ok, menu = pcall(function() return ui and ui.raceSettingsMenu end)
			if ok and menu then
				local dialog = readMember(menu, { "getDialog", "GetDialog" }, { "dialog" }, menu)
				tryCall(dialog, { "show", "Show" })
			end
		end
		return true
	end
	self:hideSelectorDialog()
	tryCall(manager, { "sendPluginEvent", "SendPluginEvent" }, "hideDialog", "scenery_selector")
	local stateManager = self:getUIStateManager()
	if self:stateContains("work") then
		tryCall(stateManager, { "showWorkbenchDialogs", "ShowWorkbenchDialogs" })
	elseif self:stateContains("flight") then
		tryCall(stateManager, { "showFlightDialogs", "ShowFlightDialogs" })
	end
	return true
end

function SceneSelect:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("activeFilter", self.activeFilter)
	properties:setPropertyAsInt("numItemsWidth", self.numItemsWidth)
	properties:setPropertyAsFloat("itemWidth", self.itemWidth)
	properties:setPropertyAsFloat("itemHeight", self.itemHeight)
	properties:setPropertyAsFloat("borderWidth", self.borderWidth)
	properties:setPropertyAsFloat("borderHeight", self.borderHeight)
	properties:setPropertyAsInt("editorSceneId", self.editorSceneId)
	properties:setPropertyAsInt("itemCount", #self.items)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("All Scenery", false)
	properties:setButtonPressed("Photo Scenery", false)
	properties:setButtonPressed("3D Scenery", false)
	properties:setButtonPressed("Race Scenery", false)
	properties:setButtonPressed("Custom Scenery", false)
	properties:setButtonPressed("Online Scenery", false)
end

function SceneSelect:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("numItemsWidth") then self.numItemsWidth = math.max(1, properties:getPropertyAsInt("numItemsWidth")) end
	if properties:hasProperty("itemWidth") then self.itemWidth = math.max(1.0, properties:getPropertyAsFloat("itemWidth")) end
	if properties:hasProperty("itemHeight") then self.itemHeight = math.max(1.0, properties:getPropertyAsFloat("itemHeight")) end
	if properties:hasProperty("borderWidth") then self.borderWidth = math.max(0.0, properties:getPropertyAsFloat("borderWidth")) end
	if properties:hasProperty("borderHeight") then self.borderHeight = math.max(0.0, properties:getPropertyAsFloat("borderHeight")) end
	if properties:hasProperty("editorSceneId") then self.editorSceneId = properties:getPropertyAsInt("editorSceneId") end
	if properties:isButtonPressed("All Scenery") then self:clickAllScenery() end
	if properties:isButtonPressed("Photo Scenery") then self:clickPhotoScenery() end
	if properties:isButtonPressed("3D Scenery") then self:click3dScenery() end
	if properties:isButtonPressed("Race Scenery") then self:clickRaceTrackScenery() end
	if properties:isButtonPressed("Custom Scenery") then self:clickCustomScenery() end
	if properties:isButtonPressed("Online Scenery") then self:clickOnlineScenes() end
end

-- Compatibility aliases for ScenerySelector.cs and existing UI event wiring.
SceneSelect.Start = SceneSelect.start
SceneSelect.Update = SceneSelect.update
SceneSelect.Setup = SceneSelect.setup
SceneSelect.OnEnable = SceneSelect.onEnable
SceneSelect.OnDisable = SceneSelect.onDisable
SceneSelect.OnBecameVisible = SceneSelect.onBecameVisible
SceneSelect.PopulateSceneries = SceneSelect.populateSceneries
SceneSelect.PopulateCustomSceneries = SceneSelect.populateCustomSceneries
SceneSelect.ClickAllScenery = SceneSelect.clickAllScenery
SceneSelect.ClickPhotoScenery = SceneSelect.clickPhotoScenery
SceneSelect.Click3dScenery = SceneSelect.click3dScenery
SceneSelect.ClickRaceTrackScenery = SceneSelect.clickRaceTrackScenery
SceneSelect.ClickCustomScenery = SceneSelect.clickCustomScenery
SceneSelect.ClickOnlineScenes = SceneSelect.clickOnlineScenes
SceneSelect.ClickClose = SceneSelect.clickClose
SceneSelect.ClickNewCustomScene = SceneSelect.clickNewCustomScene
SceneSelect.ClickEditCustomScene = SceneSelect.clickEditCustomScene
SceneSelect.ClickSelectScenery = SceneSelect.clickSelectScenery
