include("UIDialog.lua")

class 'SettingsAudio' (UIDialog)

local function clamp(value, minimum, maximum)
	value = tonumber(value)
	if not value or value ~= value then return minimum end
	if value < minimum then return minimum end
	if value > maximum then return maximum end
	return value
end

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

function SettingsAudio:__init(component)
	UIDialog.__init(self, component)
	self.component = component

	self.masterVolumeSlider = nil
	self.ambientVolumeSlider = nil
	self.mainBladesVolumeSlider = nil
	self.tailBladesVolumeSlider = nil
	self.motorVolumeSlider = nil

	self.masterVolume = 50.0
	self.ambientVolume = 50.0
	self.mainBladesVolume = 50.0
	self.tailBladesVolume = 50.0
	self.motorVolume = 50.0

	self.writeInterval = 3.0
	self.nextUpdateTime = 0.0
	self.isDirty = false
	self.isInitialising = false
	self.isEnabled = false
	self.lastError = ""
	self.retryBlockedWithoutTimer = false

	-- Optional project-specific adapters. The engine database and sound manager
	-- are used automatically when these are not supplied.
	self.settingsStore = nil
	self.settingsManager = nil
	self.applicationController = nil
	self.modelController = nil
	self.onValuesApplied = nil
end

function SettingsAudio:__finalize()
	if self.isDirty then pcall(function() self:flush() end) end
	UIDialog.__finalize(self)
end

function SettingsAudio:getBindings()
	return {
		{ key = "mastervolume", field = "masterVolume", slider = self.masterVolumeSlider },
		{ key = "ambientvolume", field = "ambientVolume", slider = self.ambientVolumeSlider },
		{ key = "bladevolume", field = "mainBladesVolume", slider = self.mainBladesVolumeSlider },
		{ key = "tailbladevolume", field = "tailBladesVolume", slider = self.tailBladesVolumeSlider },
		{ key = "motorvolume", field = "motorVolume", slider = self.motorVolumeSlider }
	}
end

function SettingsAudio:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsFloat("masterVolume", self.masterVolume)
	properties:setPropertyAsFloat("ambientVolume", self.ambientVolume)
	properties:setPropertyAsFloat("mainBladesVolume", self.mainBladesVolume)
	properties:setPropertyAsFloat("tailBladesVolume", self.tailBladesVolume)
	properties:setPropertyAsFloat("motorVolume", self.motorVolume)
	properties:setPropertyAsFloat("writeInterval", self.writeInterval)
	properties:setButtonPressed("Reload", false)
	properties:setButtonPressed("Apply", false)
end

function SettingsAudio:setProperties(parameters)
	local properties = parameters:at(0)
	local changed = false
	for _, binding in ipairs(self:getBindings()) do
		if properties:hasProperty(binding.field) then
			local value = clamp(properties:getPropertyAsFloat(binding.field), 0.0, 100.0)
			if self[binding.field] ~= value then
				self[binding.field] = value
				changed = true
			end
		end
	end
	if properties:hasProperty("writeInterval") then
		self.writeInterval = clamp(properties:getPropertyAsFloat("writeInterval"), 0.0, 3600.0)
	end
	if changed and not self.isInitialising then
		self.isDirty = true
		self.retryBlockedWithoutTimer = false
	end
	if properties:isButtonPressed("Reload") then self:setup() end
	if properties:isButtonPressed("Apply") then
		self.isDirty = true
		self:flush()
	end
end

function SettingsAudio:setMasterVolumeSlider(value) self.masterVolumeSlider = value end
function SettingsAudio:setAmbientVolumeSlider(value) self.ambientVolumeSlider = value end
function SettingsAudio:setMainBladesVolumeSlider(value) self.mainBladesVolumeSlider = value end
function SettingsAudio:setTailBladesVolumeSlider(value) self.tailBladesVolumeSlider = value end
function SettingsAudio:setMotorVolumeSlider(value) self.motorVolumeSlider = value end
function SettingsAudio:setSettingsStore(value) self.settingsStore = value end
function SettingsAudio:setSettingsManager(value) self.settingsManager = value end
function SettingsAudio:setApplicationController(value) self.applicationController = value end
function SettingsAudio:setModelController(value) self.modelController = value end
function SettingsAudio:setValuesAppliedCallback(value) self.onValuesApplied = value end

function SettingsAudio:getApplicationManager()
	local ok, manager = pcall(function() return IApplicationManager.instance() end)
	return ok and manager or nil
end

function SettingsAudio:getDatabase()
	if self.settingsStore then return self.settingsStore end
	local manager = self:getApplicationManager()
	local ok, database = tryCall(manager, { "getDatabase", "GetDatabase" })
	return ok and database or nil
end

function SettingsAudio:getTime()
	local manager = self:getApplicationManager()
	local ok, timer = tryCall(manager, { "getTimer", "GetTimer" })
	if not ok or not timer then return nil end
	local hasTime, value = tryCall(timer, { "getTimeSinceLevelLoad", "getTimeSinceSceneLoad" })
	return hasTime and tonumber(value) or nil
end

function SettingsAudio:readSetting(key, defaultValue)
	local store = self:getDatabase()
	if not store then return defaultValue, false end

	local called, value = tryCall(store,
		{ "getSettingAsFloat", "GetSettingAsFloat" }, key, defaultValue)
	if not called then
		called, value = tryCall(store, { "getSettingAsFloat", "GetSettingAsFloat" }, key)
	end
	if called and value ~= nil then return clamp(value, 0.0, 100.0), true end

	local sql = "SELECT value FROM settings WHERE param='" .. key .. "' LIMIT 1"
	local queried, query = tryCall(store, { "executeQuery", "ExecuteQuery" }, sql)
	if not queried or not query then return defaultValue, false end

	local ok, result = pcall(function()
		if query:eof() then return defaultValue end
		return query:getFieldValueAsFloat("value")
	end)
	if not ok then return defaultValue, false end
	return clamp(result, 0.0, 100.0), true
end

function SettingsAudio:writeSetting(key, value)
	local store = self:getDatabase()
	if not store then return false end
	value = clamp(value, 0.0, 100.0)

	local called, result = tryCall(store,
		{ "setSettingAsFloat", "SetSettingAsFloat" }, key, value)
	if called then return result ~= false end

	local valueText = string.format("%.6f", value)
	local updateSql = "UPDATE settings SET value='" .. valueText .. "' WHERE param='" .. key .. "'"
	local updated, affected = tryCall(store, { "executeDML", "ExecuteDML" }, updateSql)
	if not updated then return false end
	if tonumber(affected) and tonumber(affected) < 0 then return false end

	if tonumber(affected) == 0 then
		local insertSql = "INSERT INTO settings (param, value) VALUES ('" .. key .. "', '" .. valueText .. "')"
		local inserted, insertResult = tryCall(store, { "executeDML", "ExecuteDML" }, insertSql)
		return inserted and (not tonumber(insertResult) or tonumber(insertResult) >= 0)
	end
	return true
end

function SettingsAudio:setSliderValue(slider, value)
	if not slider then return false end
	local called = tryCall(slider, { "setValue", "SetValue" }, clamp(value, 0.0, 100.0))
	return called
end

function SettingsAudio:getSliderValue(slider, fallback)
	if not slider then return clamp(fallback, 0.0, 100.0) end
	local called, value = tryCall(slider,
		{ "getValueAsFloat", "GetValueAsFloat", "getValue", "GetValue" })
	if not called then return clamp(fallback, 0.0, 100.0) end
	return clamp(value, 0.0, 100.0)
end

function SettingsAudio:setup()
	if self.isInitialising then return false end
	self.isInitialising = true
	self.lastError = ""

	local hasStore = self:getDatabase() ~= nil
	local ok, errorMessage = pcall(function()
		for _, binding in ipairs(self:getBindings()) do
			local value = self:readSetting(binding.key, 50.0)
			self[binding.field] = value
			self:setSliderValue(binding.slider, value)
		end
	end)

	self.isInitialising = false
	self.isDirty = false
	local now = self:getTime()
	self.nextUpdateTime = (now or 0.0) + self.writeInterval
	if not ok then
		self.lastError = "Unable to set up audio settings: " .. tostring(errorMessage)
		print("SettingsAudio: " .. self.lastError)
		return false
	end
	if not hasStore then
		self.lastError = "No settings store is available; defaults were loaded but cannot be persisted"
		print("SettingsAudio: " .. self.lastError)
		return false
	end
	return true
end

function SettingsAudio:volumeSliderChanged()
	if self.isInitialising then return end
	self.isDirty = true
	self.retryBlockedWithoutTimer = false
end

function SettingsAudio:writeValues()
	local allWritten = true
	for _, binding in ipairs(self:getBindings()) do
		local value = self:getSliderValue(binding.slider, self[binding.field])
		self[binding.field] = value
		if not self:writeSetting(binding.key, value) then allWritten = false end
	end
	if not allWritten then self.lastError = "One or more audio settings could not be saved" end
	return allWritten
end

function SettingsAudio:updateVolume()
	tryCall(self.settingsManager, { "readSettings", "ReadSettings" })

	local applicationManager = self:getApplicationManager()
	local masterPercent = clamp(self.masterVolume, 0.0, 100.0)
	local masterNormalised = masterPercent / 100.0

	-- Project adapters retain the original C# 0-100 contract.
	tryCall(self.applicationController, { "setVolume", "SetVolume" }, masterPercent)

	-- Workphone's SoundManager uses a normalized 0-1 volume.
	local hasSoundManager, soundManager = tryCall(applicationManager,
		{ "getSoundManager", "GetSoundManager" })
	if hasSoundManager and soundManager then
		tryCall(soundManager, { "setVolume", "SetVolume" }, masterNormalised)
	end

	local modelController = self.modelController
	if not modelController then
		local _, value = tryCall(self.applicationController,
			{ "getModelController", "GetModelController" })
		modelController = value
	end
	local _, model = tryCall(modelController,
		{ "getModelAvatar", "GetModelAvatar", "getModel", "GetModel" })
	tryCall(model, { "setupVolume", "SetupVolume" })

	if self.onValuesApplied then
		local ok, errorMessage = pcall(self.onValuesApplied, self, self:getValues())
		if not ok then
			self.lastError = "Audio apply callback failed: " .. tostring(errorMessage)
			return false
		end
	end
	return true
end

function SettingsAudio:flush()
	if self.isInitialising then return false end
	if not self.isDirty then return true end
	if not self:writeValues() then return false end
	if not self:updateVolume() then return false end
	self.isDirty = false
	self.retryBlockedWithoutTimer = false
	local now = self:getTime()
	self.nextUpdateTime = (now or 0.0) + self.writeInterval
	return true
end

function SettingsAudio:getValues()
	return {
		masterVolume = self.masterVolume,
		ambientVolume = self.ambientVolume,
		mainBladesVolume = self.mainBladesVolume,
		tailBladesVolume = self.tailBladesVolume,
		motorVolume = self.motorVolume
	}
end

function SettingsAudio:setValue(name, value, markDirty)
	local aliases = {
		mastervolume = "masterVolume",
		ambientvolume = "ambientVolume",
		bladevolume = "mainBladesVolume",
		tailbladevolume = "tailBladesVolume",
		motorvolume = "motorVolume"
	}
	local field = aliases[name] or name
	if self[field] == nil then return false end
	self[field] = clamp(value, 0.0, 100.0)
	for _, binding in ipairs(self:getBindings()) do
		if binding.field == field then self:setSliderValue(binding.slider, self[field]) end
	end
	if markDirty ~= false and not self.isInitialising then self.isDirty = true end
	self.retryBlockedWithoutTimer = false
	return true
end

function SettingsAudio:start()
	self:onEnable()
end

function SettingsAudio:onEnable()
	self.isEnabled = true
	return self:setup()
end

function SettingsAudio:onDisable()
	local flushed = true
	if self.isDirty then flushed = self:flush() end
	self.isEnabled = false
	return flushed
end

function SettingsAudio:update()
	if not self.isEnabled or not self.isDirty or self.isInitialising then return end
	local now = self:getTime()
	if not now and self.retryBlockedWithoutTimer then return end
	if not now or now >= self.nextUpdateTime then
		local flushed = self:flush()
		if not flushed then
			-- Avoid hammering a missing/locked database every frame.
			self.nextUpdateTime = (now or 0.0) + self.writeInterval
			if not now then self.retryBlockedWithoutTimer = true end
		end
	end
end

-- Compatibility aliases for methods/events ported directly from C#.
SettingsAudio.Start = SettingsAudio.start
SettingsAudio.OnEnable = SettingsAudio.onEnable
SettingsAudio.OnDisable = SettingsAudio.onDisable
SettingsAudio.Setup = SettingsAudio.setup
SettingsAudio.VolumeSliderChanged = SettingsAudio.volumeSliderChanged
SettingsAudio.WriteValues = SettingsAudio.writeValues
SettingsAudio.UpdateVolume = SettingsAudio.updateVolume
SettingsAudio.Flush = SettingsAudio.flush
