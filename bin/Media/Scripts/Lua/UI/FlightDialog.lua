include("UIDialog.lua")

class 'FlightDialog' (UIDialog)

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

local function getSingleton(name)
	local classObject = rawget(_G, name)
	if not classObject then return nil end
	local ok, instance = pcall(function() return classObject.instance end)
	if not ok or instance == nil then return classObject end
	if type(instance) == "function" then
		local called, value = pcall(instance)
		if not called then called, value = pcall(instance, classObject) end
		return called and value or nil
	end
	return instance
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
		if not found then found, item = pcall(function() return value[index] end) end
		if found and item ~= nil then table.insert(result, item) end
	end
	return result
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

local function clamp(value, minimum, maximum, defaultValue)
	value = tonumber(value)
	if not value or value ~= value then value = defaultValue or minimum end
	if value < minimum then return minimum end
	if value > maximum then return maximum end
	return value
end

FlightDialog.tryCallObject = tryCall
FlightDialog.readObjectMember = readMember
FlightDialog.writeObjectMember = writeMember
FlightDialog.toArray = toArray
FlightDialog.asBoolean = asBoolean
FlightDialog.clamp = clamp

function FlightDialog:__init(component)
	UIDialog.__init(self, component)
	self.component = component
	self.controls = {}
	self.values = {}
	self.actions = {}
	self.callbacks = {}
	self.subscriptions = {}
	self.applicationController = nil
	self.loadingManager = nil
	self.uiManager = nil
	self.uiStateManager = nil
	self.inputManager = nil
	self.modelManager = nil
	self.sceneManager = nil
	self.networkManager = nil
	self.fileProvider = nil
	self.updateInterval = 0.10
	self.nextUpdateTime = 0.0
	self.enabled = false
	self.busy = false
	self.dirty = false
	self.status = ""
	self.lastError = ""
end

function FlightDialog:__finalize()
	self:unsubscribeAll()
	self.callbacks = {}
	self.actions = {}
	self.controls = {}
	UIDialog.__finalize(self)
end

function FlightDialog:setApplicationController(value) self.applicationController = value end
function FlightDialog:setLoadingManager(value) self.loadingManager = value end
function FlightDialog:setUIManager(value) self.uiManager = value end
function FlightDialog:setUIStateManager(value) self.uiStateManager = value end
function FlightDialog:setInputManager(value) self.inputManager = value end
function FlightDialog:setModelManager(value) self.modelManager = value end
function FlightDialog:setSceneManager(value) self.sceneManager = value end
function FlightDialog:setNetworkManager(value) self.networkManager = value end
function FlightDialog:setFileProvider(value) self.fileProvider = value end
function FlightDialog:setUpdateInterval(value) self.updateInterval = clamp(value, 0.0, 60.0, 0.10) end
function FlightDialog:setCallback(name, callback) self.callbacks[tostring(name or "")] = callback end
function FlightDialog:bindControl(name, control) self.controls[tostring(name or "")] = control; return control end
function FlightDialog:getControl(name) return self.controls[tostring(name or "")] end
function FlightDialog:getValue(name, defaultValue) local value = self.values[name]; if value == nil then return defaultValue end; return value end
function FlightDialog:setValue(name, value) self.values[name] = value; self.dirty = true; return value end

function FlightDialog:getApplicationManager()
	return self.applicationController or getSingleton("ApplicationManager") or getSingleton("IApplicationManager")
end

function FlightDialog:getManager(explicit, applicationGetters, globalName)
	if explicit then return explicit end
	local called, value = tryCall(self:getApplicationManager(), applicationGetters or {})
	if called and value then return value end
	return globalName and getSingleton(globalName) or nil
end

function FlightDialog:getLoadingManager() return self:getManager(self.loadingManager, { "getLoadingManager", "GetLoadingManager" }, "LoadingManager") end
function FlightDialog:getUIManager() return self.uiManager or getSingleton("UIManager") end
function FlightDialog:getUIStateManager() return self:getManager(self.uiStateManager, { "getUIStateManager", "GetUIStateManager" }, "UIStateManager") end
function FlightDialog:getInputManager() return self:getManager(self.inputManager, { "getInputManager", "GetInputManager" }, "InputManager") end
function FlightDialog:getModelManager() return self:getManager(self.modelManager, { "getModelManager", "GetModelManager" }, "ModelManager") end
function FlightDialog:getSceneManager() return self:getManager(self.sceneManager, { "getSceneManager", "GetSceneManager" }, "SceneManager") end
function FlightDialog:getNetworkManager() return self:getManager(self.networkManager, { "getNetworkManager", "GetNetworkManager" }, "NetworkManager") end

function FlightDialog:call(object, names, ...)
	return tryCall(object, names, ...)
end

function FlightDialog:read(object, getterNames, fieldNames, defaultValue)
	return readMember(object, getterNames or {}, fieldNames or {}, defaultValue)
end

function FlightDialog:write(object, setterNames, fieldNames, value)
	return writeMember(object, setterNames or {}, fieldNames or {}, value)
end

function FlightDialog:list(value) return toArray(value) end
function FlightDialog:bool(value, defaultValue) return asBoolean(value, defaultValue) end
function FlightDialog:limit(value, minimum, maximum, defaultValue) return clamp(value, minimum, maximum, defaultValue) end

function FlightDialog:getTime()
	local _, timer = tryCall(self:getApplicationManager(), { "getTimer", "GetTimer" })
	local called, value = tryCall(timer, { "getTimeSinceLevelLoad", "GetTimeSinceLevelLoad", "getTime", "GetTime" })
	if called and tonumber(value) then return tonumber(value) end
	local ok, fallback = pcall(function() return os.clock() end)
	return ok and fallback or 0.0
end

function FlightDialog:setControlText(controlOrName, value)
	local control = type(controlOrName) == "string" and self.controls[controlOrName] or controlOrName
	if not control then return false end
	local text = tostring(value or "")
	local called = tryCall(control, { "setText", "SetText", "setValue", "SetValue", "setLabel", "SetLabel" }, text)
	if called then return true end
	return writeMember(control, {}, { "text", "value", "label" }, text)
end

function FlightDialog:getControlValue(controlOrName, defaultValue)
	local control = type(controlOrName) == "string" and self.controls[controlOrName] or controlOrName
	return readMember(control,
		{ "getValue", "GetValue", "getText", "GetText", "getSelectedIndex", "GetSelectedIndex" },
		{ "value", "text", "selectedIndex" }, defaultValue)
end

function FlightDialog:setControlValue(controlOrName, value)
	local control = type(controlOrName) == "string" and self.controls[controlOrName] or controlOrName
	if not control then return false end
	return writeMember(control,
		{ "setValue", "SetValue", "setSelectedIndex", "SetSelectedIndex", "setText", "SetText" },
		{ "value", "selectedIndex", "text" }, value)
end

function FlightDialog:setControlEnabled(controlOrName, enabled)
	local control = type(controlOrName) == "string" and self.controls[controlOrName] or controlOrName
	if not control then return false end
	local target = readMember(control, { "getGameObject", "GetGameObject" }, { "gameObject" }, control)
	return writeMember(target,
		{ "setInteractable", "SetInteractable", "setEnabled", "SetEnabled", "setActive", "SetActive", "setVisible", "SetVisible" },
		{ "interactable", "enabled", "active", "visible" }, enabled == true)
end

function FlightDialog:setControlVisible(controlOrName, visible)
	local control = type(controlOrName) == "string" and self.controls[controlOrName] or controlOrName
	if not control then return false end
	local target = readMember(control, { "getGameObject", "GetGameObject" }, { "gameObject" }, control)
	return writeMember(target,
		{ "setActive", "SetActive", "setVisible", "SetVisible", "setEnabled", "SetEnabled" },
		{ "active", "visible", "enabled" }, visible == true)
end

function FlightDialog:setStatus(message, isError)
	self.status = tostring(message or "")
	if isError then self.lastError = self.status end
	self:setControlText("status", self.status)
	return not isError
end

function FlightDialog:setBusy(value, message)
	self.busy = value == true
	if message ~= nil then self:setStatus(message, false) end
	for name, control in pairs(self.controls) do
		if string.find(string.lower(name), "button", 1, true) then self:setControlEnabled(control, not self.busy) end
	end
end

function FlightDialog:emit(name, ...)
	local callback = self.callbacks[name]
	if not callback then return false, nil end
	local ok, result = pcall(callback, self, ...)
	if not ok then self:setStatus(tostring(name) .. " callback failed: " .. tostring(result), true); return false, nil end
	return true, result
end

function FlightDialog:registerAction(name, handler)
	if not name or name == "" then return false end
	self.actions[tostring(name)] = handler
	return true
end

function FlightDialog:invokeAction(name, ...)
	local handler = self.actions[name]
	if type(handler) == "string" then handler = self[handler] end
	if type(handler) ~= "function" then return false end
	local ok, result = pcall(handler, self, ...)
	if not ok then self:setStatus("Action failed: " .. tostring(result), true); return false end
	return result ~= false
end

function FlightDialog:subscribe(source, addNames, removeNames, callback)
	if not source or type(callback) ~= "function" then return false end
	local called = tryCall(source, addNames or { "addListener", "AddListener", "subscribe", "Subscribe" }, callback)
	if called then table.insert(self.subscriptions, { source = source, remove = removeNames, callback = callback }) end
	return called
end

function FlightDialog:unsubscribeAll()
	for _, item in ipairs(self.subscriptions) do
		tryCall(item.source, item.remove or { "removeListener", "RemoveListener", "unsubscribe", "Unsubscribe" }, item.callback)
	end
	self.subscriptions = {}
end

function FlightDialog:start()
	UIDialog.start(self)
	self.enabled = true
	self.nextUpdateTime = self:getTime()
	return true
end

function FlightDialog:update()
	UIDialog.update(self)
end

function FlightDialog:shouldUpdate()
	local now = self:getTime()
	if now < self.nextUpdateTime then return false end
	self.nextUpdateTime = now + self.updateInterval
	return true
end

function FlightDialog:onEnable() self.enabled = true; return true end
function FlightDialog:onDisable() self.enabled = false; self:unsubscribeAll(); return true end
function FlightDialog:onDestroy() return self:onDisable() end

function FlightDialog:clickClose()
	local called = tryCall(self:getUIStateManager(), { "hideDialogs", "HideDialogs", "hideAllDialogs", "HideAllDialogs" })
	if not called then self:hide() end
	self:emit("closed")
	return true
end

function FlightDialog:handleEvent(parameters, results)
	if not parameters then return false end
	local ok, eventHash = pcall(function() return parameters:at(1) end)
	if not ok then return false end
	local clickHash, activateHash = nil, nil
	pcall(function() clickHash = IEvent.CLICK_HASH; activateHash = IEvent.ACTIVATE_HASH end)
	if eventHash ~= clickHash and eventHash ~= activateHash then return false end
	local sender = nil
	pcall(function() sender = parameters:at(3) end)
	local actor = readMember(sender, { "getActor", "GetActor" }, {}, sender)
	local name = readMember(actor, { "getName", "GetName" }, { "name" }, "")
	return self:invokeAction(tostring(name), sender, results)
end

function FlightDialog:getProperties(parameters)
	UIDialog.getProperties(self, parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsFloat("updateInterval", self.updateInterval)
	properties:setPropertyAsBool("busy", self.busy)
	properties:setPropertyAsBool("dirty", self.dirty)
	properties:setPropertyAsString("status", self.status)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("Close", false)
end

function FlightDialog:setProperties(parameters)
	UIDialog.setProperties(self, parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("updateInterval") then self:setUpdateInterval(properties:getPropertyAsFloat("updateInterval")) end
	if properties:isButtonPressed("Close") then self:clickClose() end
end

FlightDialog.Start = FlightDialog.start
FlightDialog.Update = FlightDialog.update
FlightDialog.OnEnable = FlightDialog.onEnable
FlightDialog.OnDisable = FlightDialog.onDisable
FlightDialog.OnDestroy = FlightDialog.onDestroy
FlightDialog.ClickClose = FlightDialog.clickClose
FlightDialog.CloseDialog = FlightDialog.clickClose
FlightDialog.HandleEvent = FlightDialog.handleEvent
