include("UIDialog.lua")

class 'SettingsGeneral' (UIDialog)

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

local function asBoolean(value, defaultValue)
	if value == nil then return defaultValue == true end
	if type(value) == "boolean" then return value end
	if type(value) == "number" then return value ~= 0 end
	value = string.lower(tostring(value))
	if value == "true" or value == "yes" or value == "on" or value == "1" then return true end
	if value == "false" or value == "no" or value == "off" or value == "0" or value == "" then return false end
	return defaultValue == true
end

local function suppliedOr(value, fallback)
	if value ~= nil then return value end
	return fallback
end

local function sqlEscape(value)
	return string.gsub(tostring(value), "'", "''")
end

function SettingsGeneral:__init(component)
	UIDialog.__init(self, component)
	self.component = component
	self.text = nil
	self.autoConfigSwitch = nil
	self.autoConfiguration = nil
	self.deviceRating = nil
	self.applicationController = nil
	self.settingsStore = nil
	self.onGeneralSettingsChanged = nil

	self.enableAutoConfig = true
	self.deviceDataText = ""
	self.refreshDelay = 3.0
	self.refreshPending = false
	self.refreshAt = nil
	self.isInitialising = false
	self.isEnabled = false
	self.lastError = ""
end

function SettingsGeneral:__finalize()
	self.refreshPending = false
	UIDialog.__finalize(self)
end

function SettingsGeneral:setText(value) self.text = value end
function SettingsGeneral:setAutoConfigSwitch(value) self.autoConfigSwitch = value end
function SettingsGeneral:setAutoConfiguration(value) self.autoConfiguration = value end
function SettingsGeneral:setDeviceRating(value) self.deviceRating = value end
function SettingsGeneral:setApplicationController(value) self.applicationController = value end
function SettingsGeneral:setSettingsStore(value) self.settingsStore = value end
function SettingsGeneral:setGeneralSettingsChangedCallback(value) self.onGeneralSettingsChanged = value end
function SettingsGeneral:setRefreshDelay(value)
	value = tonumber(value) or 3.0
	self.refreshDelay = math.max(0.0, math.min(value, 60.0))
end

function SettingsGeneral:getApplicationManager()
	local ok, manager = pcall(function() return IApplicationManager.instance() end)
	return ok and manager or nil
end

function SettingsGeneral:getDatabase()
	if self.settingsStore then return self.settingsStore end
	local ok, database = tryCall(self:getApplicationManager(), { "getDatabase", "GetDatabase" })
	return ok and database or nil
end

function SettingsGeneral:getTime()
	local ok, timer = tryCall(self:getApplicationManager(), { "getTimer", "GetTimer" })
	if not ok or not timer then return nil end
	local called, value = tryCall(timer,
		{ "getTimeSinceLevelLoad", "getTimeSinceSceneLoad", "getTime", "GetTime" })
	return called and tonumber(value) or nil
end

function SettingsGeneral:readSetting(key, defaultValue)
	local store = self:getDatabase()
	if not store then return defaultValue, false end
	local called, value = tryCall(store,
		{ "getSetting", "GetSetting", "getSettingAsString", "GetSettingAsString" }, key, defaultValue)
	if not called then
		called, value = tryCall(store,
			{ "getSetting", "GetSetting", "getSettingAsString", "GetSettingAsString" }, key)
	end
	if called and value ~= nil then return value, true end

	local sql = "SELECT value FROM settings WHERE param='" .. sqlEscape(key) .. "' LIMIT 1"
	local queried, query = tryCall(store, { "executeQuery", "ExecuteQuery" }, sql)
	if not queried or not query then return defaultValue, false end
	local ok, result = pcall(function()
		if query:eof() then return defaultValue end
		return query:getFieldValue("value")
	end)
	if not ok or result == nil then return defaultValue, false end
	return result, true
end

function SettingsGeneral:writeSetting(key, value)
	local store = self:getDatabase()
	if not store then return false end
	local textValue = tostring(value)
	local called, result = tryCall(store,
		{ "setSetting", "SetSetting", "setSettingAsString", "SetSettingAsString" }, key, textValue)
	if called then return result ~= false end

	local escapedKey = sqlEscape(key)
	local escapedValue = sqlEscape(textValue)
	local sql = "UPDATE settings SET value='" .. escapedValue .. "' WHERE param='" .. escapedKey .. "'"
	local updated, affected = tryCall(store, { "executeDML", "ExecuteDML" }, sql)
	if not updated or (tonumber(affected) and tonumber(affected) < 0) then return false end
	if tonumber(affected) == 0 then
		local insertSql = "INSERT INTO settings (param, value) VALUES ('" .. escapedKey .. "', '" .. escapedValue .. "')"
		local inserted, insertResult = tryCall(store, { "executeDML", "ExecuteDML" }, insertSql)
		return inserted and (not tonumber(insertResult) or tonumber(insertResult) >= 0)
	end
	return true
end

function SettingsGeneral:writeBoolean(key, value)
	return self:writeSetting(key, value and "1" or "0")
end

function SettingsGeneral:setSwitchValue(value)
	if not self.autoConfigSwitch then return false end
	local called = tryCall(self.autoConfigSwitch,
		{ "setValue", "SetValue", "setIsOn", "SetIsOn", "setValueAsString", "SetValueAsString" }, value)
	if called then return true end
	local readable, current = pcall(function() return self.autoConfigSwitch.isOn end)
	if readable and current ~= nil then return pcall(function() self.autoConfigSwitch.isOn = value == true end) end
	return false
end

function SettingsGeneral:getSwitchValue(fallback)
	if not self.autoConfigSwitch then return fallback end
	local called, value = tryCall(self.autoConfigSwitch,
		{ "getValue", "GetValue", "getIsOn", "GetIsOn" })
	if called and value ~= nil then return asBoolean(value, fallback) end
	local ok, direct = pcall(function() return self.autoConfigSwitch.isOn end)
	if ok and direct ~= nil then return asBoolean(direct, fallback) end
	return fallback
end

function SettingsGeneral:setTextValue(value)
	self.deviceDataText = tostring(value or "")
	if not self.text then return false end
	local called = tryCall(self.text, { "setText", "SetText", "setValue", "SetValue" }, self.deviceDataText)
	if called then return true end
	local readable, current = pcall(function() return self.text.text end)
	if readable and current ~= nil then return pcall(function() self.text.text = self.deviceDataText end) end
	return false
end

function SettingsGeneral:getAutoConfiguration()
	if self.autoConfiguration then return self.autoConfiguration end
	local called, value = tryCall(self.applicationController,
		{ "getAutoConfiguration", "GetAutoConfiguration" })
	return called and value or nil
end

function SettingsGeneral:getDeviceRating()
	if self.deviceRating then return self.deviceRating end
	local configuration = self:getAutoConfiguration()
	local called, value = tryCall(configuration, { "getDeviceRating", "GetDeviceRating" })
	if called and value then return value end
	local ok, direct = pcall(function() return configuration and configuration.deviceRating end)
	return ok and direct or nil
end

function SettingsGeneral:refreshDeviceData()
	local rating = self:getDeviceRating()
	local called, value = tryCall(rating, { "getDataText", "GetDataText" })
	if not called then
		called, value = tryCall(self:getAutoConfiguration(),
			{ "getDeviceDataText", "GetDeviceDataText", "getDataText", "GetDataText" })
	end
	if not called then
		called, value = tryCall(self.applicationController,
			{ "getDeviceRatingText", "GetDeviceRatingText", "getDeviceDataText", "GetDeviceDataText" })
	end
	if not called or value == nil then
		self.lastError = "Device rating data is unavailable"
		return false
	end
	self:setTextValue(value)
	self.lastError = ""
	return true
end

function SettingsGeneral:setup()
	if self.isInitialising then return false end
	self.isInitialising = true
	self.lastError = ""
	local hasStore = self:getDatabase() ~= nil
	local ok, errorMessage = pcall(function()
		self:refreshDeviceData()
		local storedValue, found = self:readSetting("enableAutoConfig", true)
		self.enableAutoConfig = asBoolean(storedValue, true)
		if hasStore and not found then self:writeBoolean("enableAutoConfig", true) end
		self:setSwitchValue(self.enableAutoConfig)
	end)
	self.isInitialising = false
	if not ok then
		self.lastError = "Unable to set up general settings: " .. tostring(errorMessage)
		print("SettingsGeneral: " .. self.lastError)
		return false
	end
	if not hasStore then
		self.lastError = "No settings store is available; defaults were loaded but cannot be persisted"
		print("SettingsGeneral: " .. self.lastError)
		return false
	end
	return true
end

function SettingsGeneral:toggleAutoConfigSwitch(value)
	if self.isInitialising then return false end
	self.enableAutoConfig = asBoolean(suppliedOr(value,
		self:getSwitchValue(self.enableAutoConfig)), self.enableAutoConfig)
	self:setSwitchValue(self.enableAutoConfig)
	local saved = self:writeBoolean("enableAutoConfig", self.enableAutoConfig)

	local configuration = self:getAutoConfiguration()
	local applied, result = tryCall(configuration,
		{ "setAutoQualityEnabled", "SetAutoQualityEnabled" }, self.enableAutoConfig)
	if not applied then
		applied, result = tryCall(self.applicationController,
			{ "setAutoQualityEnabled", "SetAutoQualityEnabled", "setAutoConfigurationEnabled", "SetAutoConfigurationEnabled" },
			self.enableAutoConfig)
	end
	if not applied or result == false then
		self.lastError = "Auto-configuration was saved but could not be applied"
	end
	if self.onGeneralSettingsChanged then
		local ok, errorMessage = pcall(self.onGeneralSettingsChanged,
			self, "enableAutoConfig", self.enableAutoConfig)
		if not ok then self.lastError = "General settings callback failed: " .. tostring(errorMessage); return false end
	end
	return saved
end

function SettingsGeneral:scheduleDeviceDataRefresh(delay)
	self.refreshDelay = math.max(0.0, math.min(tonumber(delay) or self.refreshDelay, 60.0))
	local now = self:getTime()
	self.refreshAt = now and (now + self.refreshDelay) or nil
	self.refreshPending = true
	return true
end

function SettingsGeneral:runAutoConfiguration()
	local configuration = self:getAutoConfiguration()
	local called, result = tryCall(configuration, { "setupSettings", "SetupSettings" })
	if called then return result ~= false end
	called, result = tryCall(self.applicationController,
		{ "setupAutoConfiguration", "SetupAutoConfiguration", "runAutoConfiguration", "RunAutoConfiguration" })
	if called then return result ~= false end

	-- Lower-level fallback matching AutoConfiguration.SetupSettings().
	local foundQuality, autoQuality = tryCall(configuration, { "getAutoQuality", "GetAutoQuality" })
	if not foundQuality then
		local ok, direct = pcall(function() return configuration and configuration.autoQuality end)
		if ok then autoQuality = direct end
	end
	local ran, runResult = tryCall(autoQuality, { "runAll", "RunAll" })
	if not ran or runResult == false then return false end
	self:writeBoolean("vsync", false)
	self:writeBoolean("fullcontrolsystem", false)
	local applied = tryCall(self.applicationController, { "setVsync", "SetVsync" }, false)
	if not applied then tryCall(self.applicationController, { "setupVsync", "SetupVsync" }) end
	return true
end

function SettingsGeneral:onClickSetupSettings()
	if self.isInitialising then return false end
	self.lastError = ""
	if not self:runAutoConfiguration() then
		self.lastError = "Auto-configuration service is unavailable or failed"
		print("SettingsGeneral: " .. self.lastError)
		return false
	end
	self:scheduleDeviceDataRefresh(self.refreshDelay)
	if self.onGeneralSettingsChanged then
		local ok, errorMessage = pcall(self.onGeneralSettingsChanged,
			self, "setupSettings", true)
		if not ok then self.lastError = "General settings callback failed: " .. tostring(errorMessage); return false end
	end
	return true
end

function SettingsGeneral:updateDataCoroutine()
	return self:scheduleDeviceDataRefresh(self.refreshDelay)
end

function SettingsGeneral:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsBool("enableAutoConfig", self.enableAutoConfig)
	properties:setPropertyAsString("deviceDataText", self.deviceDataText)
	properties:setPropertyAsFloat("refreshDelay", self.refreshDelay)
	properties:setPropertyAsBool("refreshPending", self.refreshPending)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("Run Auto Configuration", false)
	properties:setButtonPressed("Refresh Device Data", false)
end

function SettingsGeneral:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("refreshDelay") then self:setRefreshDelay(properties:getPropertyAsFloat("refreshDelay")) end
	if properties:isButtonPressed("Run Auto Configuration") then self:onClickSetupSettings() end
	if properties:isButtonPressed("Refresh Device Data") then self:refreshDeviceData() end
end

function SettingsGeneral:start() end
function SettingsGeneral:onEnable() self.isEnabled = true; return self:setup() end
function SettingsGeneral:onDisable()
	self.isEnabled = false
	self.refreshPending = false
	self.refreshAt = nil
	return true
end

function SettingsGeneral:update()
	if not self.isEnabled or not self.refreshPending then return end
	local now = self:getTime()
	if self.refreshAt == nil or (now and now >= self.refreshAt) then
		self.refreshPending = false
		self.refreshAt = nil
		self:refreshDeviceData()
	end
end

-- Compatibility aliases for the original C# API and events.
SettingsGeneral.Start = SettingsGeneral.start
SettingsGeneral.Update = SettingsGeneral.update
SettingsGeneral.Setup = SettingsGeneral.setup
SettingsGeneral.OnEnable = SettingsGeneral.onEnable
SettingsGeneral.OnDisable = SettingsGeneral.onDisable
SettingsGeneral.ToggleAutoConfigSwitch = SettingsGeneral.toggleAutoConfigSwitch
SettingsGeneral.UpdateDataCoroutine = SettingsGeneral.updateDataCoroutine
SettingsGeneral.OnClickSetupSettings = SettingsGeneral.onClickSetupSettings
