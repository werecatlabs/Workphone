include("UIDialog.lua")

class 'SettingsSimulation' (UIDialog)

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

local function finiteNumber(value, defaultValue)
	value = tonumber(value)
	if not value or value ~= value or value == math.huge or value == -math.huge then
		return defaultValue
	end
	return value
end

local function clamp(value, minimum, maximum, defaultValue)
	value = finiteNumber(value, defaultValue or minimum)
	if value < minimum then return minimum end
	if value > maximum then return maximum end
	return value
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

local function numberText(value)
	return string.format("%.9g", finiteNumber(value, 0.0))
end

function SettingsSimulation:__init(component)
	UIDialog.__init(self, component)
	self.component = component

	self.fullControlModeSwitch = nil
	self.aggressiveCpuUtilisationSwitch = nil
	self.simulationSpeedSlider = nil
	self.bladeOpacitySlider = nil
	self.bladeSlapSlider = nil
	self.crashResetTimeSlider = nil
	self.crashToleranceSlider = nil
	self.defaultFlybarlessDropdown = nil

	self.fullControlSystem = false
	self.aggressiveCpuUtilisation = false
	self.simulationSpeed = 100.0
	self.bladeDiskOpacity = 1.0
	self.bladeSlapFrequency = 100.0
	self.crashResetTime = 5.0
	self.crashTolerance = 100.0
	self.defaultResponseMode = 4

	self.simulationSpeedMin = 1.0
	self.simulationSpeedMax = 1000.0
	self.bladeSlapFrequencyMax = 1000.0
	self.crashResetTimeMax = 300.0
	self.crashToleranceMax = 1000.0
	self.responseModeCount = 64

	self.isInitialising = false
	self.isEnabled = false
	self.crashToleranceDirty = false
	self.lastError = ""
	self.settingsStore = nil
	self.applicationController = nil
	self.onSimulationSettingsChanged = nil
end

function SettingsSimulation:__finalize()
	if self.isEnabled and self.crashToleranceDirty then pcall(function() self:saveCrashTolerance() end) end
	UIDialog.__finalize(self)
end

function SettingsSimulation:setFullControlModeSwitch(value) self.fullControlModeSwitch = value end
function SettingsSimulation:setAggressiveCpuUtilisationSwitch(value) self.aggressiveCpuUtilisationSwitch = value end
function SettingsSimulation:setSimulationSpeedSlider(value) self.simulationSpeedSlider = value end
function SettingsSimulation:setBladeOpacitySlider(value) self.bladeOpacitySlider = value end
function SettingsSimulation:setBladeSlapSlider(value) self.bladeSlapSlider = value end
function SettingsSimulation:setCrashResetTimeSlider(value) self.crashResetTimeSlider = value end
function SettingsSimulation:setCrashToleranceSlider(value) self.crashToleranceSlider = value end
function SettingsSimulation:setDefaultFlybarlessDropdown(value) self.defaultFlybarlessDropdown = value end
function SettingsSimulation:setSettingsStore(value) self.settingsStore = value end
function SettingsSimulation:setApplicationController(value) self.applicationController = value end
function SettingsSimulation:setSimulationSettingsChangedCallback(value) self.onSimulationSettingsChanged = value end
function SettingsSimulation:setResponseModeCount(value) self.responseModeCount = math.max(1, math.floor(tonumber(value) or 64)) end

function SettingsSimulation:getApplicationManager()
	local ok, manager = pcall(function() return IApplicationManager.instance() end)
	return ok and manager or nil
end

function SettingsSimulation:getDatabase()
	if self.settingsStore then return self.settingsStore end
	local ok, database = tryCall(self:getApplicationManager(), { "getDatabase", "GetDatabase" })
	return ok and database or nil
end

function SettingsSimulation:readSetting(key, defaultValue)
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

function SettingsSimulation:writeSetting(key, value)
	local store = self:getDatabase()
	if not store then return false end
	local text = tostring(value)
	local called, result = tryCall(store,
		{ "setSetting", "SetSetting", "setSettingAsString", "SetSettingAsString" }, key, text)
	if called then return result ~= false end

	local escapedKey = sqlEscape(key)
	local escapedValue = sqlEscape(text)
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

function SettingsSimulation:writeBoolean(key, value)
	return self:writeSetting(key, value and "1" or "0")
end

function SettingsSimulation:setControlValue(control, value)
	if not control then return false end
	local called = tryCall(control,
		{ "setValue", "SetValue", "setIsOn", "SetIsOn", "setValueAsString", "SetValueAsString" }, value)
	if called then return true end
	local readable, current = pcall(function() return control.isOn end)
	if readable and current ~= nil then return pcall(function() control.isOn = value == true end) end
	return false
end

function SettingsSimulation:getControlValue(control, fallback)
	if not control then return fallback end
	local called, value = tryCall(control,
		{ "getValue", "GetValue", "getValueAsFloat", "GetValueAsFloat", "getIsOn", "GetIsOn" })
	if called and value ~= nil then return value end
	local ok, direct = pcall(function() return control.isOn end)
	if ok and direct ~= nil then return direct end
	return fallback
end

function SettingsSimulation:setDropdownIndex(control, index)
	if not control then return false end
	local called = tryCall(control,
		{ "setSelectedOption", "SetSelectedOption", "setValue", "SetValue" }, index)
	if called then return true end
	local readable, current = pcall(function() return control.value end)
	if readable and current ~= nil then return pcall(function() control.value = index end) end
	return false
end

function SettingsSimulation:getDropdownIndex(control, fallback)
	if not control then return fallback or 0 end
	local called, value = tryCall(control,
		{ "getSelectedOption", "GetSelectedOption", "getValue", "GetValue" })
	if called and tonumber(value) then return math.max(0, math.floor(tonumber(value) + 0.5)) end
	local readable, direct = pcall(function() return control.value end)
	if readable and tonumber(direct) then return math.max(0, math.floor(tonumber(direct) + 0.5)) end
	return fallback or 0
end

function SettingsSimulation:getResponseModeCount()
	local called, options = tryCall(self.defaultFlybarlessDropdown, { "getOptions", "GetOptions" })
	if called and options then
		local hasSize, size = tryCall(options, { "size", "Size", "getSize", "GetSize" })
		if hasSize and tonumber(size) and tonumber(size) > 0 then return math.floor(tonumber(size)) end
		if type(options) == "table" and #options > 0 then return #options end
	end
	return self.responseModeCount
end

function SettingsSimulation:notifyChanged(key, value)
	tryCall(self.applicationController, { "handleSetting", "HandleSetting" }, key, tostring(value))
	if self.onSimulationSettingsChanged then
		local ok, errorMessage = pcall(self.onSimulationSettingsChanged, self, key, value, self:getValues())
		if not ok then
			self.lastError = "Simulation settings callback failed: " .. tostring(errorMessage)
			return false
		end
	end
	return true
end

function SettingsSimulation:setup()
	if self.isInitialising then return false end
	self.isInitialising = true
	self.lastError = ""
	local hasStore = self:getDatabase() ~= nil

	local ok, errorMessage = pcall(function()
		self.simulationSpeed = clamp(self:readSetting("simspeed", 100.0),
			self.simulationSpeedMin, self.simulationSpeedMax, 100.0)
		self.bladeDiskOpacity = clamp(self:readSetting("bladediskopacity", 1.0), 0.0, 1.0, 1.0)
		self.bladeSlapFrequency = clamp(self:readSetting("bladeslapfrequency", 100.0),
			0.0, self.bladeSlapFrequencyMax, 100.0)
		self.crashResetTime = clamp(self:readSetting("crashresettime", 5.0),
			0.0, self.crashResetTimeMax, 5.0)
		self.crashTolerance = clamp(self:readSetting("crash_tolerance", 100.0),
			0.0, self.crashToleranceMax, 100.0)
		self.defaultResponseMode = math.floor(clamp(self:readSetting("defaultresponsemode", 4),
			1, self:getResponseModeCount(), 4) + 0.5)
		self.fullControlSystem = asBoolean(self:readSetting("fullcontrolsystem", false), false)
		self.aggressiveCpuUtilisation = asBoolean(
			self:readSetting("aggressiveCpuUtilisation", false), false)

		self:setControlValue(self.simulationSpeedSlider, self.simulationSpeed)
		self:setControlValue(self.bladeOpacitySlider, self.bladeDiskOpacity * 100.0)
		self:setControlValue(self.bladeSlapSlider, self.bladeSlapFrequency)
		self:setControlValue(self.crashResetTimeSlider, self.crashResetTime)
		self:setControlValue(self.crashToleranceSlider, self.crashTolerance)
		self:setControlValue(self.fullControlModeSwitch, self.fullControlSystem)
		self:setControlValue(self.aggressiveCpuUtilisationSwitch, self.aggressiveCpuUtilisation)
		self:setDropdownIndex(self.defaultFlybarlessDropdown, self.defaultResponseMode - 1)
	end)

	self.isInitialising = false
	self.crashToleranceDirty = false
	if not ok then
		self.lastError = "Unable to set up simulation settings: " .. tostring(errorMessage)
		print("SettingsSimulation: " .. self.lastError)
		return false
	end
	if not hasStore then
		self.lastError = "No settings store is available; defaults were loaded but cannot be persisted"
		print("SettingsSimulation: " .. self.lastError)
		return false
	end
	return true
end

function SettingsSimulation:onControlSystemSwitch(value)
	if self.isInitialising then return false end
	self.fullControlSystem = asBoolean(suppliedOr(value,
		self:getControlValue(self.fullControlModeSwitch, self.fullControlSystem)), self.fullControlSystem)
	local saved = self:writeBoolean("fullcontrolsystem", self.fullControlSystem)
	self:notifyChanged("fullcontrolsystem", self.fullControlSystem)
	return saved
end

function SettingsSimulation:onAggressiveCpuUtilisationSwitch(value)
	if self.isInitialising then return false end
	self.aggressiveCpuUtilisation = asBoolean(suppliedOr(value,
		self:getControlValue(self.aggressiveCpuUtilisationSwitch, self.aggressiveCpuUtilisation)),
		self.aggressiveCpuUtilisation)
	local text = self.aggressiveCpuUtilisation and "true" or "false"
	local saved = self:writeSetting("aggressiveCpuUtilisation", text)
	local sent, sendResult = tryCall(self.applicationController,
		{ "sendPluginEvent", "SendPluginEvent" }, "aggressiveCpuUtilisation", text)
	if not sent or sendResult == false then
		tryCall(self.applicationController,
			{ "sendPluginEvent", "SendPluginEvent" }, "aggressiveCpuUtilisation", { arg1 = text })
	end
	self:notifyChanged("aggressiveCpuUtilisation", self.aggressiveCpuUtilisation)
	return saved
end

function SettingsSimulation:onSimspeed(value)
	if self.isInitialising then return false end
	self.simulationSpeed = clamp(suppliedOr(value,
		self:getControlValue(self.simulationSpeedSlider, self.simulationSpeed)),
		self.simulationSpeedMin, self.simulationSpeedMax, self.simulationSpeed)
	self:setControlValue(self.simulationSpeedSlider, self.simulationSpeed)
	local saved = self:writeSetting("simspeed", numberText(self.simulationSpeed))
	self:notifyChanged("simspeed", self.simulationSpeed)
	return saved
end

function SettingsSimulation:onBladediskopacity(value)
	if self.isInitialising then return false end
	local sliderValue = clamp(suppliedOr(value,
		self:getControlValue(self.bladeOpacitySlider, self.bladeDiskOpacity * 100.0)),
		0.0, 100.0, self.bladeDiskOpacity * 100.0)
	self.bladeDiskOpacity = sliderValue / 100.0
	self:setControlValue(self.bladeOpacitySlider, sliderValue)
	local saved = self:writeSetting("bladediskopacity", numberText(self.bladeDiskOpacity))
	self:notifyChanged("bladediskopacity", self.bladeDiskOpacity)
	return saved
end

function SettingsSimulation:onBladeslapfrequency(value)
	if self.isInitialising then return false end
	self.bladeSlapFrequency = clamp(suppliedOr(value,
		self:getControlValue(self.bladeSlapSlider, self.bladeSlapFrequency)),
		0.0, self.bladeSlapFrequencyMax, self.bladeSlapFrequency)
	self:setControlValue(self.bladeSlapSlider, self.bladeSlapFrequency)
	local saved = self:writeSetting("bladeslapfrequency", numberText(self.bladeSlapFrequency))
	self:notifyChanged("bladeslapfrequency", self.bladeSlapFrequency)
	return saved
end

function SettingsSimulation:onCrashresettime(value)
	if self.isInitialising then return false end
	self.crashResetTime = clamp(suppliedOr(value,
		self:getControlValue(self.crashResetTimeSlider, self.crashResetTime)),
		0.0, self.crashResetTimeMax, self.crashResetTime)
	self:setControlValue(self.crashResetTimeSlider, self.crashResetTime)
	local saved = self:writeSetting("crashresettime", numberText(self.crashResetTime))
	self:notifyChanged("crashresettime", self.crashResetTime)
	return saved
end

function SettingsSimulation:onDefaultresponsemode(value)
	if self.isInitialising then return false end
	local index = tonumber(value)
	if index == nil then index = self:getDropdownIndex(self.defaultFlybarlessDropdown, self.defaultResponseMode - 1) end
	index = math.floor(clamp(index, 0, self:getResponseModeCount() - 1, self.defaultResponseMode - 1) + 0.5)
	self.defaultResponseMode = index + 1
	self:setDropdownIndex(self.defaultFlybarlessDropdown, index)
	local saved = self:writeSetting("defaultresponsemode", self.defaultResponseMode)
	self:notifyChanged("defaultresponsemode", self.defaultResponseMode)
	return saved
end

function SettingsSimulation:onCrashToleranceChanged(value)
	if self.isInitialising then return false end
	self.crashTolerance = clamp(suppliedOr(value,
		self:getControlValue(self.crashToleranceSlider, self.crashTolerance)),
		0.0, self.crashToleranceMax, self.crashTolerance)
	self:setControlValue(self.crashToleranceSlider, self.crashTolerance)
	self.crashToleranceDirty = true
	return true
end

function SettingsSimulation:saveCrashTolerance()
	self.crashTolerance = clamp(self:getControlValue(self.crashToleranceSlider, self.crashTolerance),
		0.0, self.crashToleranceMax, self.crashTolerance)
	local saved = self:writeSetting("crash_tolerance", numberText(self.crashTolerance))
	if saved then self.crashToleranceDirty = false end
	self:notifyChanged("crash_tolerance", self.crashTolerance)
	return saved
end

function SettingsSimulation:onEnableDirectControls(value)
	if self.isInitialising then return false end
	local enabled = asBoolean(value, true)
	local called, result = tryCall(self.applicationController,
		{ "setDirectControlsEnabled", "SetDirectControlsEnabled", "enableDirectControls", "EnableDirectControls" },
		enabled)
	if self.onSimulationSettingsChanged then
		local ok, errorMessage = pcall(self.onSimulationSettingsChanged, self, "directcontrols", enabled, self:getValues())
		if not ok then self.lastError = "Simulation settings callback failed: " .. tostring(errorMessage); return false end
	end
	return called and result ~= false
end

function SettingsSimulation:onQueryResult(tag, queryResult)
	local value = queryResult
	if type(queryResult) == "table" then
		value = queryResult.value or (queryResult.rows and queryResult.rows[1] and queryResult.rows[1].value)
	elseif type(queryResult) == "string" then
		value = string.match(queryResult, '"value"%s*:%s*"([^"]+)"') or queryResult
	end
	if tag == "fullcontrolsystem" then
		-- Preserve the legacy callback's inverted representation.
		self.fullControlSystem = not asBoolean(value, true)
		self:setControlValue(self.fullControlModeSwitch, self.fullControlSystem)
	elseif tag == "simspeed" then
		self.simulationSpeed = clamp(value, self.simulationSpeedMin, self.simulationSpeedMax, self.simulationSpeed)
		self:setControlValue(self.simulationSpeedSlider, self.simulationSpeed)
	elseif tag == "bladediskopacity" then
		self.bladeDiskOpacity = clamp(value, 0.0, 1.0, self.bladeDiskOpacity)
		self:setControlValue(self.bladeOpacitySlider, self.bladeDiskOpacity * 100.0)
	elseif tag == "bladeslapfrequency" then
		self.bladeSlapFrequency = clamp(value, 0.0, self.bladeSlapFrequencyMax, self.bladeSlapFrequency)
		self:setControlValue(self.bladeSlapSlider, self.bladeSlapFrequency)
	elseif tag == "crashresettime" then
		self.crashResetTime = clamp(value, 0.0, self.crashResetTimeMax, self.crashResetTime)
		self:setControlValue(self.crashResetTimeSlider, self.crashResetTime)
	elseif tag == "defaultresponsemode" then
		self.defaultResponseMode = math.floor(clamp(value, 1, self:getResponseModeCount(), self.defaultResponseMode) + 0.5)
		self:setDropdownIndex(self.defaultFlybarlessDropdown, self.defaultResponseMode - 1)
	else
		return false
	end
	return true
end

function SettingsSimulation:getValues()
	return {
		fullControlSystem = self.fullControlSystem,
		aggressiveCpuUtilisation = self.aggressiveCpuUtilisation,
		simulationSpeed = self.simulationSpeed,
		bladeDiskOpacity = self.bladeDiskOpacity,
		bladeOpacityPercent = self.bladeDiskOpacity * 100.0,
		bladeSlapFrequency = self.bladeSlapFrequency,
		crashResetTime = self.crashResetTime,
		crashTolerance = self.crashTolerance,
		defaultResponseMode = self.defaultResponseMode,
		defaultResponseModeIndex = self.defaultResponseMode - 1
	}
end

function SettingsSimulation:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsBool("fullControlSystem", self.fullControlSystem)
	properties:setPropertyAsBool("aggressiveCpuUtilisation", self.aggressiveCpuUtilisation)
	properties:setPropertyAsFloat("simulationSpeed", self.simulationSpeed)
	properties:setPropertyAsFloat("bladeDiskOpacity", self.bladeDiskOpacity)
	properties:setPropertyAsFloat("bladeSlapFrequency", self.bladeSlapFrequency)
	properties:setPropertyAsFloat("crashResetTime", self.crashResetTime)
	properties:setPropertyAsFloat("crashTolerance", self.crashTolerance)
	properties:setPropertyAsInt("defaultResponseMode", self.defaultResponseMode)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("Reload", false)
	properties:setButtonPressed("Apply Crash Tolerance", false)
end

function SettingsSimulation:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("crashTolerance") then
		self:onCrashToleranceChanged(properties:getPropertyAsFloat("crashTolerance"))
	end
	if properties:isButtonPressed("Reload") then self:setup() end
	if properties:isButtonPressed("Apply Crash Tolerance") then self:saveCrashTolerance() end
end

function SettingsSimulation:start() end
function SettingsSimulation:update() end
function SettingsSimulation:onEnable() self.isEnabled = true; return self:setup() end
function SettingsSimulation:onDisable()
	local saved = true
	if self.isEnabled then saved = self:saveCrashTolerance() end
	self.isEnabled = false
	return saved
end

-- Compatibility aliases for the original C# method and event names.
SettingsSimulation.Start = SettingsSimulation.start
SettingsSimulation.Update = SettingsSimulation.update
SettingsSimulation.Setup = SettingsSimulation.setup
SettingsSimulation.OnEnable = SettingsSimulation.onEnable
SettingsSimulation.OnDisable = SettingsSimulation.onDisable
SettingsSimulation.OnControlSystemSwitch = SettingsSimulation.onControlSystemSwitch
SettingsSimulation.OnAggressiveCpuUtilisationSwitch = SettingsSimulation.onAggressiveCpuUtilisationSwitch
SettingsSimulation.OnSimspeed = SettingsSimulation.onSimspeed
SettingsSimulation.OnBladediskopacity = SettingsSimulation.onBladediskopacity
SettingsSimulation.OnBladeslapfrequency = SettingsSimulation.onBladeslapfrequency
SettingsSimulation.OnCrashresettime = SettingsSimulation.onCrashresettime
SettingsSimulation.OnDefaultresponsemode = SettingsSimulation.onDefaultresponsemode
SettingsSimulation.OnEnableDirectControls = SettingsSimulation.onEnableDirectControls
SettingsSimulation.OnQueryResult = SettingsSimulation.onQueryResult
