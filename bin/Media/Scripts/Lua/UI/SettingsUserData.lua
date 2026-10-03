include("UIDialog.lua")

class 'SettingsUserData' (UIDialog)

local RESET_MESSAGE = "Are you sure you want to reset all user data? " ..
	"This will delete all customized models, transmitter profiles and settings. " ..
	"The simulator will be reset to factory defaults. This action can't be undone."

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

local function hasMethod(object, names)
	if not object then return false end
	for _, name in ipairs(names) do
		local ok, fn = pcall(function() return object[name] end)
		if ok and fn then return true end
	end
	return false
end

local function sqlEscape(value)
	return string.gsub(tostring(value), "'", "''")
end

function SettingsUserData:__init(component)
	UIDialog.__init(self, component)
	self.component = component
	self.resetUserDataButton = nil
	self.confirmationDialog = nil
	self.uiManager = nil
	self.applicationController = nil
	self.settingsStore = nil
	self.preferenceStore = nil
	self.transmitter = nil
	self.databaseManager = nil
	self.loadingManager = nil
	self.resetService = nil
	self.onResetCompleted = nil
	self.onResetFailed = nil

	self.startMenuState = nil
	self.isEnabled = false
	self.isInitialising = false
	self.awaitingConfirmation = false
	self.resetInProgress = false
	self.lastResetSucceeded = false
	self.lastError = ""
	self.lastWarnings = {}
	self.resetCount = 0
	self.confirmCallback = function() self:onConfirmEvent() end
	self.cancelCallback = function() self:onCancelEvent() end
end

function SettingsUserData:__finalize()
	pcall(function() self:closeConfirmation() end)
	UIDialog.__finalize(self)
end

function SettingsUserData:setResetUserDataButton(value) self.resetUserDataButton = value end
function SettingsUserData:setConfirmationDialog(value) self.confirmationDialog = value end
function SettingsUserData:setUIManager(value) self.uiManager = value end
function SettingsUserData:setApplicationController(value) self.applicationController = value end
function SettingsUserData:setSettingsStore(value) self.settingsStore = value end
function SettingsUserData:setPreferenceStore(value) self.preferenceStore = value end
function SettingsUserData:setTransmitter(value) self.transmitter = value end
function SettingsUserData:setDatabaseManager(value) self.databaseManager = value end
function SettingsUserData:setLoadingManager(value) self.loadingManager = value end
function SettingsUserData:setResetService(value) self.resetService = value end
function SettingsUserData:setStartMenuState(value) self.startMenuState = value end
function SettingsUserData:setResetCompletedCallback(value) self.onResetCompleted = value end
function SettingsUserData:setResetFailedCallback(value) self.onResetFailed = value end

function SettingsUserData:getApplicationManager()
	local ok, manager = pcall(function() return IApplicationManager.instance() end)
	return ok and manager or nil
end

function SettingsUserData:getDatabase()
	if self.settingsStore then return self.settingsStore end
	local ok, database = tryCall(self:getApplicationManager(), { "getDatabase", "GetDatabase" })
	return ok and database or nil
end

function SettingsUserData:getUIManager()
	if self.uiManager then return self.uiManager end
	local ok, manager = pcall(function()
		if UIManager and UIManager.instance then return UIManager.instance() end
		return nil
	end)
	return ok and manager or nil
end

function SettingsUserData:getConfirmationDialog()
	if self.confirmationDialog then return self.confirmationDialog end
	local ui = self:getUIManager()
	local found, dialog = tryCall(ui,
		{ "getMessageBoxConfirm", "GetMessageBoxConfirm", "getConfirmationDialog", "GetConfirmationDialog" })
	if found and dialog then return dialog end
	local ok, direct = pcall(function() return ui and ui.messageBoxConfirm end)
	return ok and direct or nil
end

function SettingsUserData:setResetButtonEnabled(enabled)
	if not self.resetUserDataButton then return false end
	local called = tryCall(self.resetUserDataButton,
		{ "setInteractable", "SetInteractable", "setEnabled", "SetEnabled" }, enabled == true)
	if called then return true end
	local ok = pcall(function() self.resetUserDataButton.interactable = enabled == true end)
	return ok
end

function SettingsUserData:setup()
	if self.isInitialising then return false end
	self.isInitialising = true
	self.lastError = ""
	if not self.awaitingConfirmation and not self.resetInProgress then self:setResetButtonEnabled(true) end
	self.isInitialising = false
	return true
end

function SettingsUserData:showConfirmation()
	local ui = self:getUIManager()
	local shown = tryCall(ui,
		{ "showConfirmation", "ShowConfirmation", "showConfirmMessage", "ShowConfirmMessage" },
		RESET_MESSAGE, self.confirmCallback, self.cancelCallback)
	if shown then return true end

	local dialog = self:getConfirmationDialog()
	if not dialog then return false end
	tryCall(dialog, { "setMessageText", "SetMessageText", "setText", "SetText" }, RESET_MESSAGE)
	local confirmBound = tryCall(dialog,
		{ "setConfirmCallback", "SetConfirmCallback", "setOnConfirm", "SetOnConfirm" }, self.confirmCallback)
	local cancelBound = tryCall(dialog,
		{ "setCancelCallback", "SetCancelCallback", "setOnCancel", "SetOnCancel" }, self.cancelCallback)
	local visible = tryCall(dialog, { "show", "Show", "setVisible", "SetVisible", "setEnabled", "SetEnabled" }, true)
	if not visible then
		local actorFound, actor = tryCall(dialog, { "getActor", "GetActor" })
		if actorFound and actor then visible = tryCall(actor, { "setEnabled", "SetEnabled" }, true) end
	end
	return visible and confirmBound and cancelBound
end

function SettingsUserData:closeConfirmation()
	local dialog = self:getConfirmationDialog()
	if dialog then
		tryCall(dialog, { "clearCallbacks", "ClearCallbacks", "removeCallbacks", "RemoveCallbacks" })
		tryCall(dialog, { "hide", "Hide", "setVisible", "SetVisible", "setEnabled", "SetEnabled" }, false)
		local actorFound, actor = tryCall(dialog, { "getActor", "GetActor" })
		if actorFound and actor then tryCall(actor, { "setEnabled", "SetEnabled" }, false) end
	end
	local ui = self:getUIManager()
	tryCall(ui, { "hideConfirmation", "HideConfirmation", "cancelConfirmation", "CancelConfirmation" })
	self.awaitingConfirmation = false
	if not self.resetInProgress then self:setResetButtonEnabled(true) end
end

function SettingsUserData:resetAllUserData()
	if self.awaitingConfirmation or self.resetInProgress then return false end
	self.lastError = ""
	self.lastWarnings = {}
	self.lastResetSucceeded = false
	self.awaitingConfirmation = true
	self:setResetButtonEnabled(false)
	if not self:showConfirmation() then
		self.awaitingConfirmation = false
		self:setResetButtonEnabled(true)
		self.lastError = "A confirmation dialog with confirm and cancel callbacks is required"
		print("SettingsUserData: " .. self.lastError)
		return false
	end
	return true
end

function SettingsUserData:recordWarning(message)
	table.insert(self.lastWarnings, tostring(message))
	print("SettingsUserData: " .. tostring(message))
end

function SettingsUserData:isInStartMenu()
	local called, value = tryCall(self.applicationController,
		{ "isInStartMenu", "IsInStartMenu", "getIsInStartMenu", "GetIsInStartMenu" })
	if called then return value == true, true end
	called, value = tryCall(self.applicationController,
		{ "getApplicationState", "GetApplicationState", "applicationState" })
	if not called then
		local ok, direct = pcall(function() return self.applicationController and self.applicationController.applicationState end)
		if ok and direct ~= nil then called, value = true, direct end
	end
	if not called then return true, false end
	if self.startMenuState ~= nil then return value == self.startMenuState, true end
	if type(value) == "string" then
		local state = string.lower(value)
		return string.find(state, "start_menu", 1, true) ~= nil or
			string.find(state, "startmenu", 1, true) ~= nil, true
	end
	-- Unknown numeric enum: default to the safer start-menu behavior.
	return true, false
end

function SettingsUserData:writeFirstRunFalse()
	local store = self:getDatabase()
	if not store then return false end
	local called, result = tryCall(store,
		{ "setSettingAsBool", "SetSettingAsBool" }, "firstrun", false)
	if called then return result ~= false end

	local key = sqlEscape("firstrun")
	local sql = "UPDATE settings SET value='0' WHERE param='" .. key .. "'"
	local updated, affected = tryCall(store, { "executeDML", "ExecuteDML" }, sql)
	if not updated or (tonumber(affected) and tonumber(affected) < 0) then return false end
	if tonumber(affected) == 0 then
		local inserted, insertResult = tryCall(store, { "executeDML", "ExecuteDML" },
			"INSERT INTO settings (param, value) VALUES ('firstrun', '0')")
		return inserted and (not tonumber(insertResult) or tonumber(insertResult) >= 0)
	end
	return true
end

function SettingsUserData:canDispatchReset()
	if hasMethod(self.resetService, { "resetAllUserData", "ResetAllUserData" }) then return true end
	return hasMethod(self.applicationController,
		{ "resetUserData", "ResetUserData", "sendPluginEvent", "SendPluginEvent" })
end

function SettingsUserData:clearPreferences()
	local store = self.preferenceStore
	if not store then
		local _, value = tryCall(self.applicationController,
			{ "getPreferenceStore", "GetPreferenceStore", "getPlayerPreferences", "GetPlayerPreferences" })
		store = value
	end
	local called, result = tryCall(store,
		{ "deleteAll", "DeleteAll", "clear", "Clear", "reset", "Reset" })
	if not called then
		called, result = tryCall(self.applicationController,
			{ "deleteAllPreferences", "DeleteAllPreferences", "clearPreferences", "ClearPreferences" })
	end
	return called and result ~= false
end

function SettingsUserData:clearTransmitterProfiles()
	local transmitter = self.transmitter
	if not transmitter then
		local _, value = tryCall(self.applicationController, { "getTransmitter", "GetTransmitter" })
		transmitter = value
	end
	local called, result = tryCall(transmitter, { "deleteAll", "DeleteAll", "resetProfiles", "ResetProfiles" })
	if not called then
		called, result = tryCall(self.applicationController,
			{ "deleteAllTransmitterProfiles", "DeleteAllTransmitterProfiles" })
	end
	return called and result ~= false
end

function SettingsUserData:sendPluginEvent(name)
	local called, result = tryCall(self.applicationController,
		{ "sendPluginEvent", "SendPluginEvent" }, name)
	if not called then
		called, result = tryCall(self.applicationController,
			{ "sendPluginEvent", "SendPluginEvent" }, name, {})
	end
	return called and result ~= false
end

function SettingsUserData:dispatchReset()
	local called, result = tryCall(self.resetService,
		{ "resetAllUserData", "ResetAllUserData" })
	if called then return result ~= false end
	called, result = tryCall(self.applicationController,
		{ "resetUserData", "ResetUserData" })
	if called then return result ~= false end
	return self:sendPluginEvent("ResetUserData")
end

function SettingsUserData:updateDatabaseLevel()
	local manager = self.databaseManager
	if not manager then
		local _, value = tryCall(self.applicationController,
			{ "getDatabaseManager", "GetDatabaseManager" })
		manager = value
	end
	local called, result = tryCall(manager,
		{ "updateDatabaseLevel", "UpdateDatabaseLevel" })
	if not called then
		called, result = tryCall(self.applicationController,
			{ "updateDatabaseLevel", "UpdateDatabaseLevel" })
	end
	return called and result ~= false
end

function SettingsUserData:reloadApplication()
	local manager = self.loadingManager
	if not manager then
		local _, value = tryCall(self.applicationController,
			{ "getLoadingManager", "GetLoadingManager" })
		manager = value
	end
	local called, result = tryCall(manager, { "reload", "Reload" })
	if not called then
		called, result = tryCall(self.applicationController,
			{ "reload", "Reload", "reloadApplication", "ReloadApplication" })
	end
	return called and result ~= false
end

function SettingsUserData:notifyResetFailed(message)
	self.lastError = tostring(message)
	self.lastResetSucceeded = false
	print("SettingsUserData: " .. self.lastError)
	if self.onResetFailed then pcall(self.onResetFailed, self, self.lastError, self.lastWarnings) end
end

function SettingsUserData:onConfirmEvent()
	if not self.awaitingConfirmation or self.resetInProgress then return false end
	self.awaitingConfirmation = false
	self.resetInProgress = true
	self:setResetButtonEnabled(false)
	self:closeConfirmation()

	-- Do not begin an irreversible, multi-step reset without a reset dispatcher.
	if not self:canDispatchReset() then
		self.resetInProgress = false
		self:setResetButtonEnabled(true)
		self:notifyResetFailed("Reset aborted: no reset service or plugin-event adapter is configured")
		return false
	end

	local inStartMenu, stateKnown = self:isInStartMenu()
	if not stateKnown then self:recordWarning("Application state is unavailable; active-model teardown and reload were skipped") end
	if not self:clearPreferences() then self:recordWarning("Player preferences could not be cleared") end
	if not self:clearTransmitterProfiles() then self:recordWarning("Transmitter profiles could not be cleared") end
	if not inStartMenu and not self:sendPluginEvent("destroyModel") then
		self:recordWarning("The active model could not be destroyed")
	end

	local dispatched = self:dispatchReset()
	if not dispatched then
		self.resetInProgress = false
		self:setResetButtonEnabled(true)
		self:notifyResetFailed("The reset service rejected or failed the ResetUserData request")
		return false
	end
	if not self:updateDatabaseLevel() then self:recordWarning("The database migration level could not be refreshed") end
	if not self:writeFirstRunFalse() then self:recordWarning("The firstrun setting could not be updated") end

	self:setup()
	if not inStartMenu and not self:reloadApplication() then
		self:recordWarning("User data was reset, but the active application could not be reloaded")
	end

	self.resetInProgress = false
	self.lastResetSucceeded = true
	self.resetCount = self.resetCount + 1
	self:setResetButtonEnabled(true)
	if self.onResetCompleted then
		local ok, errorMessage = pcall(self.onResetCompleted, self, self.lastWarnings)
		if not ok then self:recordWarning("Reset completion callback failed: " .. tostring(errorMessage)) end
	end
	return true
end

function SettingsUserData:onCancelEvent()
	if not self.awaitingConfirmation then return false end
	self:closeConfirmation()
	self:setup()
	return true
end

function SettingsUserData:deleteAllModels()
	if not self.resetInProgress then
		self.lastError = "DeleteAllModels is only allowed during a confirmed user-data reset"
		return false
	end
	local called, result = tryCall(self.resetService,
		{ "deleteAllModels", "DeleteAllModels" })
	if not called then
		called, result = tryCall(self.applicationController,
			{ "deleteAllModels", "DeleteAllModels" })
	end
	return called and result ~= false
end

function SettingsUserData:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsBool("awaitingConfirmation", self.awaitingConfirmation)
	properties:setPropertyAsBool("resetInProgress", self.resetInProgress)
	properties:setPropertyAsBool("lastResetSucceeded", self.lastResetSucceeded)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setPropertyAsInt("resetCount", self.resetCount)
	properties:setButtonPressed("Reset All User Data", false)
end

function SettingsUserData:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:isButtonPressed("Reset All User Data") then self:resetAllUserData() end
end

function SettingsUserData:start() return self:setup() end
function SettingsUserData:update() end
function SettingsUserData:onEnable() self.isEnabled = true; return self:setup() end
function SettingsUserData:onDisable()
	self.isEnabled = false
	if self.awaitingConfirmation then self:closeConfirmation() end
	return true
end

-- Compatibility aliases for the original C# API and event names.
SettingsUserData.Start = SettingsUserData.start
SettingsUserData.Update = SettingsUserData.update
SettingsUserData.Setup = SettingsUserData.setup
SettingsUserData.OnEnable = SettingsUserData.onEnable
SettingsUserData.OnDisable = SettingsUserData.onDisable
SettingsUserData.DeleteAllModels = SettingsUserData.deleteAllModels
SettingsUserData.ResetAllUserData = SettingsUserData.resetAllUserData
SettingsUserData.OnConfirmEvent = SettingsUserData.onConfirmEvent
SettingsUserData.OnCancelEvent = SettingsUserData.onCancelEvent
