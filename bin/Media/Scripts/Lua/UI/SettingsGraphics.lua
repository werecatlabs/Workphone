include("UIDialog.lua")

class 'SettingsGraphics' (UIDialog)

local AA_VALUES = { 0, 2, 4, 8 }
local AA_LABELS = { "Off", "Low", "Medium", "High" }
local FRAME_RATES = { 30, 40, 50, 60, 75, 100, 120, 144, 160, 165, 180, 200, 240, 300 }
local QUALITY_LABELS = { "Ultra Low", "Low", "Medium", "High", "Ultra High" }
local QUALITY_VALUES = { "off", "low", "medium", "high", "ultrahigh" }

local function clamp(value, minimum, maximum)
	value = tonumber(value)
	if not value or value ~= value then return minimum end
	if value < minimum then return minimum end
	if value > maximum then return maximum end
	return value
end

local function round(value)
	value = tonumber(value) or 0
	return math.floor(value + 0.5)
end

local function asBoolean(value, defaultValue)
	if value == nil then return defaultValue == true end
	if type(value) == "boolean" then return value end
	if type(value) == "number" then return value ~= 0 end
	value = string.lower(tostring(value))
	if value == "yes" or value == "true" or value == "on" or value == "1" then return true end
	if value == "no" or value == "false" or value == "off" or value == "0" or value == "" then return false end
	return defaultValue == true
end

local function suppliedOr(value, fallback)
	if value ~= nil then return value end
	return fallback
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

local function findIndex(values, wanted, defaultIndex)
	for index, value in ipairs(values) do
		if value == wanted then return index - 1 end
	end
	return defaultIndex or 0
end

local function sqlEscape(value)
	return string.gsub(tostring(value), "'", "''")
end

function SettingsGraphics:__init(component)
	UIDialog.__init(self, component)
	self.component = component

	self.renderQualityDropdown = nil
	self.resolutionDropdown = nil
	self.antiAliasingDropdown = nil
	self.targetFrameRateDropdown = nil
	self.workbenchDropdown = nil
	self.fullscreenSwitch = nil
	self.vsyncSwitch = nil
	self.aaSwitch = nil
	self.simpleLightingSwitch = nil
	self.shadowsSwitch = nil
	self.gpuOptimisationsSwitch = nil
	self.rotorDiscBlurSwitch = nil
	self.enableVRSwitch = nil
	self.resolutionScaleSlider = nil

	self.resolutionEntries = {}
	self.renderQuality = "medium"
	self.resolutionIndex = 0
	self.antiAliasing = 0
	self.targetFrameRate = 60
	self.workbench = 0
	self.fullscreen = false
	self.vsync = true
	self.aa = true
	self.simpleLighting = false
	self.enableShadows = true
	self.gpuOptimisations = true
	self.enableVR = true
	self.rotorDiscBlur = true
	self.resolutionScale = 100.0

	self.isInitialising = false
	self.isEnabled = false
	self.lastError = ""
	self.settingsStore = nil
	self.applicationController = nil
	self.cameraManager = nil
	self.modelController = nil
	self.onSettingsApplied = nil
end

function SettingsGraphics:__finalize()
	pcall(function() self:onDisable() end)
	UIDialog.__finalize(self)
end

function SettingsGraphics:setRenderQualityDropdown(value) self.renderQualityDropdown = value end
function SettingsGraphics:setResolutionDropdown(value) self.resolutionDropdown = value end
function SettingsGraphics:setAntiAliasingDropdown(value) self.antiAliasingDropdown = value end
function SettingsGraphics:setTargetFrameRateDropdown(value) self.targetFrameRateDropdown = value end
function SettingsGraphics:setWorkbenchDropdown(value) self.workbenchDropdown = value end
function SettingsGraphics:setFullscreenSwitch(value) self.fullscreenSwitch = value end
function SettingsGraphics:setVsyncSwitch(value) self.vsyncSwitch = value end
function SettingsGraphics:setAaSwitch(value) self.aaSwitch = value end
function SettingsGraphics:setSimpleLightingSwitch(value) self.simpleLightingSwitch = value end
function SettingsGraphics:setShadowsSwitch(value) self.shadowsSwitch = value end
function SettingsGraphics:setGpuOptimisationsSwitch(value) self.gpuOptimisationsSwitch = value end
function SettingsGraphics:setRotorDiscBlurSwitch(value) self.rotorDiscBlurSwitch = value end
function SettingsGraphics:setEnableVRSwitch(value) self.enableVRSwitch = value end
function SettingsGraphics:setResolutionScaleSlider(value) self.resolutionScaleSlider = value end
function SettingsGraphics:setResolitionScale(value) self.resolutionScaleSlider = value end
function SettingsGraphics:setSettingsStore(value) self.settingsStore = value end
function SettingsGraphics:setApplicationController(value) self.applicationController = value end
function SettingsGraphics:setCameraManager(value) self.cameraManager = value end
function SettingsGraphics:setModelController(value) self.modelController = value end
function SettingsGraphics:setSettingsAppliedCallback(value) self.onSettingsApplied = value end

function SettingsGraphics:getApplicationManager()
	local ok, manager = pcall(function() return IApplicationManager.instance() end)
	return ok and manager or nil
end

function SettingsGraphics:getDatabase()
	if self.settingsStore then return self.settingsStore end
	local ok, database = tryCall(self:getApplicationManager(), { "getDatabase", "GetDatabase" })
	return ok and database or nil
end

function SettingsGraphics:readSetting(key, defaultValue)
	local store = self:getDatabase()
	if not store then return defaultValue, false end

	local called, value = tryCall(store, { "getSetting", "GetSetting", "getSettingAsString", "GetSettingAsString" }, key, defaultValue)
	if not called then called, value = tryCall(store, { "getSetting", "GetSetting", "getSettingAsString", "GetSettingAsString" }, key) end
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

function SettingsGraphics:writeSetting(key, value)
	local store = self:getDatabase()
	if not store then return false end
	local text = tostring(value)
	local called, result = tryCall(store, { "setSetting", "SetSetting", "setSettingAsString", "SetSettingAsString" }, key, text)
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

function SettingsGraphics:writeBoolean(key, value)
	return self:writeSetting(key, value and "1" or "0")
end

function SettingsGraphics:setControlValue(control, value)
	if not control then return false end
	local called = tryCall(control, { "setValue", "SetValue", "setIsOn", "SetIsOn" }, value)
	return called
end

function SettingsGraphics:getControlValue(control, fallback)
	if not control then return fallback end
	local called, value = tryCall(control, { "getValue", "GetValue", "getValueAsFloat", "GetValueAsFloat", "getIsOn", "GetIsOn" })
	if called and value ~= nil then return value end
	local ok, direct = pcall(function() return control.isOn end)
	if ok and direct ~= nil then return direct end
	return fallback
end

function SettingsGraphics:setDropdownIndex(control, index)
	if not control then return false end
	index = math.max(0, round(index))
	local called = tryCall(control, { "setSelectedOption", "SetSelectedOption", "setValue", "SetValue" }, index)
	return called
end

function SettingsGraphics:getDropdownIndex(control, fallback)
	if not control then return fallback or 0 end
	local called, value = tryCall(control, { "getSelectedOption", "GetSelectedOption", "getValue", "GetValue" })
	if called and tonumber(value) then return math.max(0, round(value)) end
	return fallback or 0
end

function SettingsGraphics:setDropdownOptions(control, options)
	if not control then return false end
	local nativeOptions = nil
	local created, parameters = pcall(function() return Parameters() end)
	if created and parameters then
		for _, option in ipairs(options) do parameters:push_back(tostring(option)) end
		nativeOptions = parameters:getAsStringArray()
	end
	local called = false
	if nativeOptions then called = tryCall(control, { "setOptions", "SetOptions" }, nativeOptions) end
	if not called then called = tryCall(control, { "setOptions", "SetOptions" }, options) end
	if called then return true end
	-- Wrappers can expose clearOptions; native Workphone dropdowns normally use setOptions.
	tryCall(control, { "clearOptions", "ClearOptions", "removeAllOptions", "RemoveAllOptions" })
	local added = false
	for _, option in ipairs(options) do
		local ok = tryCall(control, { "addOption", "AddOption" }, option)
		added = added or ok
	end
	return added
end

function SettingsGraphics:getGraphicsWindow()
	local manager = self:getApplicationManager()
	local ok, graphics = tryCall(manager, { "getGraphicsSystem", "GetGraphicsSystem" })
	if ok and graphics then
		local found, window = tryCall(graphics, { "getDefaultWindow", "getRenderWindow" })
		if found and window then return window end
	end
	local found, window = tryCall(manager, { "getWindow", "GetWindow" })
	return found and window or nil
end

function SettingsGraphics:getCurrentResolution()
	local window = self:getGraphicsWindow()
	local ok, size = tryCall(window, { "getSize", "GetSize" })
	if ok and size then
		local widthOk, width = pcall(function() return size.x end)
		local heightOk, height = pcall(function() return size.y end)
		if widthOk and heightOk and tonumber(width) and tonumber(height) then return round(width), round(height) end
	end
	local called, width, height = false, nil, nil
	called, width = tryCall(self.applicationController, { "getScreenWidth", "GetScreenWidth" })
	local heightCalled
	heightCalled, height = tryCall(self.applicationController, { "getScreenHeight", "GetScreenHeight" })
	if called and heightCalled then return round(width), round(height) end
	return nil, nil
end

function SettingsGraphics:setAvailableResolutions(resolutions)
	self.resolutionEntries = {}
	local seen = {}
	for _, resolution in ipairs(resolutions or {}) do
		local width = round(resolution.width or resolution.x or 0)
		local height = round(resolution.height or resolution.y or 0)
		if width > 0 and height > 0 then
			local label = tostring(resolution.label or (width .. " x " .. height))
			local key = width .. "x" .. height
			if not seen[key] then
				seen[key] = true
				table.insert(self.resolutionEntries, { label = label, width = width, height = height })
			end
		end
	end
	local labels = {}
	for _, entry in ipairs(self.resolutionEntries) do table.insert(labels, entry.label) end
	self:setDropdownOptions(self.resolutionDropdown, labels)
	return #self.resolutionEntries
end

function SettingsGraphics:loadAvailableResolutions()
	local called, resolutions = tryCall(self.applicationController, { "getAvailableResolutions", "GetAvailableResolutions" })
	if called and type(resolutions) == "table" then self:setAvailableResolutions(resolutions) end
	local width, height = self:getCurrentResolution()
	if #self.resolutionEntries == 0 and width and height then
		self:setAvailableResolutions({ { width = width, height = height } })
	end
	self.resolutionIndex = 0
	for index, entry in ipairs(self.resolutionEntries) do
		if entry.width == width and entry.height == height then self.resolutionIndex = index - 1 end
	end
	self:setDropdownIndex(self.resolutionDropdown, self.resolutionIndex)
end

function SettingsGraphics:setup()
	if self.isInitialising then return false end
	self.isInitialising = true
	self.lastError = ""

	local ok, errorMessage = pcall(function()
		self.renderQuality = tostring(self:readSetting("renderquality", "medium"))
		if findIndex(QUALITY_VALUES, self.renderQuality, -1) < 0 then self.renderQuality = "medium" end
		local rawAntiAliasing = tostring(self:readSetting("fsaa", 0))
		self.antiAliasing = tonumber(rawAntiAliasing)
		if not self.antiAliasing then
			-- Older builds stored this value as <fsaa aa="N"/>.
			self.antiAliasing = tonumber(string.match(rawAntiAliasing, "aa%s*=%s*['\"](%d+)['\"]")) or 0
			self:writeSetting("fsaa", self.antiAliasing)
		end
		if findIndex(AA_VALUES, self.antiAliasing, -1) < 0 then self.antiAliasing = 0 end
		self.targetFrameRate = tonumber(self:readSetting("framerate", 60)) or 60
		self.workbench = math.max(0, round(self:readSetting("workbench", 0)))
		self.fullscreen = asBoolean(self:readSetting("fullscreen", false), false)
		self.vsync = asBoolean(self:readSetting("vsync", true), true)
		self.aa = asBoolean(self:readSetting("aa", true), true)
		self.simpleLighting = asBoolean(self:readSetting("simple_lighting", false), false)
		self.enableShadows = asBoolean(self:readSetting("enable_shadows", true), true)
		self.gpuOptimisations = asBoolean(self:readSetting("gpu_optimisations", true), true)
		self.enableVR = asBoolean(self:readSetting("enableVR", true), true)
		self.rotorDiscBlur = asBoolean(self:readSetting("rotor_disc_blur", true), true)
		self.resolutionScale = clamp(self:readSetting("resolution_scale", 100.0), 1.0, 200.0)
		local fullscreenRead, actualFullscreen = tryCall(self:getGraphicsWindow(), { "isFullScreen", "IsFullScreen" })
		if fullscreenRead then self.fullscreen = asBoolean(actualFullscreen, self.fullscreen) end

		self:setDropdownOptions(self.renderQualityDropdown, QUALITY_LABELS)
		self:setDropdownIndex(self.renderQualityDropdown, findIndex(QUALITY_VALUES, self.renderQuality, 2))
		self:loadAvailableResolutions()
		self:setDropdownOptions(self.antiAliasingDropdown, AA_LABELS)
		self:setDropdownIndex(self.antiAliasingDropdown, findIndex(AA_VALUES, self.antiAliasing, 0))
		local frameLabels = {}
		for _, rate in ipairs(FRAME_RATES) do table.insert(frameLabels, tostring(rate)) end
		self:setDropdownOptions(self.targetFrameRateDropdown, frameLabels)
		self:setDropdownIndex(self.targetFrameRateDropdown, findIndex(FRAME_RATES, self.targetFrameRate, 3))
		self:setDropdownIndex(self.workbenchDropdown, self.workbench)
		self:setControlValue(self.fullscreenSwitch, self.fullscreen)
		self:setControlValue(self.vsyncSwitch, self.vsync)
		self:setControlValue(self.aaSwitch, self.aa)
		self:setControlValue(self.simpleLightingSwitch, not self.simpleLighting)
		self:setControlValue(self.shadowsSwitch, self.enableShadows)
		self:setControlValue(self.gpuOptimisationsSwitch, not self.gpuOptimisations)
		self:setControlValue(self.enableVRSwitch, self.enableVR)
		self:setControlValue(self.rotorDiscBlurSwitch, self.rotorDiscBlur)
		self:setControlValue(self.resolutionScaleSlider, self.resolutionScale)
	end)

	self.isInitialising = false
	if not ok then
		self.lastError = "Unable to set up graphics settings: " .. tostring(errorMessage)
		print("SettingsGraphics: " .. self.lastError)
		return false
	end
	if not self:getDatabase() then
		self.lastError = "No settings store is available; defaults were loaded but cannot be persisted"
		print("SettingsGraphics: " .. self.lastError)
		return false
	end
	return true
end

function SettingsGraphics:onRenderQualityChanged(value)
	if self.isInitialising then return false end
	local index = tonumber(value) and round(value) or self:getDropdownIndex(self.renderQualityDropdown, 2)
	index = math.max(0, math.min(#QUALITY_VALUES - 1, index))
	self.renderQuality = QUALITY_VALUES[index + 1]
	tryCall(self.applicationController, { "setRenderQuality", "SetRenderQuality" }, self.renderQuality)
	return self:writeSetting("renderquality", self.renderQuality)
end

function SettingsGraphics:onResolutionChanged(value)
	if self.isInitialising then return false end
	local index = tonumber(value) and round(value) or self:getDropdownIndex(self.resolutionDropdown, self.resolutionIndex)
	if index < 0 or index >= #self.resolutionEntries then return false end
	local entry = self.resolutionEntries[index + 1]
	self.resolutionIndex = index
	local applied = tryCall(self.applicationController, { "setResolution", "SetResolution" }, entry.width, entry.height, self.fullscreen)
	if not applied then applied = tryCall(self:getGraphicsWindow(), { "resize", "Resize" }, entry.width, entry.height) end
	if not applied then self.lastError = "The selected resolution could not be applied" end
	return applied
end

function SettingsGraphics:onAntiAliasingChanged(value)
	if self.isInitialising then return false end
	local index = tonumber(value) and round(value) or self:getDropdownIndex(self.antiAliasingDropdown, 0)
	index = math.max(0, math.min(#AA_VALUES - 1, index))
	self.antiAliasing = AA_VALUES[index + 1]
	tryCall(self.applicationController, { "setAntiAliasing", "SetAntiAliasing" }, self.antiAliasing)
	return self:writeSetting("fsaa", self.antiAliasing)
end

function SettingsGraphics:onTargetFrameRateChanged(value)
	if self.isInitialising then return false end
	local index = tonumber(value) and round(value) or self:getDropdownIndex(self.targetFrameRateDropdown, 3)
	index = math.max(0, math.min(#FRAME_RATES - 1, index))
	self.targetFrameRate = FRAME_RATES[index + 1]
	tryCall(self.applicationController, { "setupFPS", "SetupFPS", "setTargetFrameRate", "SetTargetFrameRate" }, self.targetFrameRate)
	return self:writeSetting("framerate", self.targetFrameRate)
end

function SettingsGraphics:onWorkbenchChanged(value)
	if self.isInitialising then return false end
	self.workbench = math.max(0, round(tonumber(value) or self:getDropdownIndex(self.workbenchDropdown, self.workbench)))
	local written = self:writeSetting("workbench", self.workbench)
	tryCall(self.applicationController, { "onWorkbenchSettingChanged", "OnWorkbenchSettingChanged" }, self.workbench)
	return written
end

function SettingsGraphics:onFullscreenToggled(value)
	if self.isInitialising then return false end
	self.fullscreen = asBoolean(suppliedOr(value, self:getControlValue(self.fullscreenSwitch, self.fullscreen)), self.fullscreen)
	local applied = tryCall(self.applicationController, { "setFullscreen", "SetFullscreen" }, self.fullscreen)
	local written = self:writeSetting("fullscreen", self.fullscreen and "yes" or "no")
	if not applied then self.lastError = "Fullscreen was saved but requires an application graphics adapter or restart" end
	return written
end

function SettingsGraphics:onVsyncToggled(value)
	if self.isInitialising then return false end
	self.vsync = asBoolean(suppliedOr(value, self:getControlValue(self.vsyncSwitch, self.vsync)), self.vsync)
	tryCall(self.applicationController, { "setVsync", "SetVsync", "setVSync", "SetVSync" }, self.vsync)
	return self:writeSetting("vsync", self.vsync and "yes" or "no")
end

function SettingsGraphics:onAaSwitch(value)
	if self.isInitialising then return false end
	self.aa = asBoolean(suppliedOr(value, self:getControlValue(self.aaSwitch, self.aa)), self.aa)
	local written = self:writeBoolean("aa", self.aa)
	tryCall(self.cameraManager, { "setDirty", "SetDirty", "markDirty", "MarkDirty" }, true)
	return written
end

function SettingsGraphics:onSimpleLightingSwitch(value)
	if self.isInitialising then return false end
	local switchOn = asBoolean(suppliedOr(value, self:getControlValue(self.simpleLightingSwitch, not self.simpleLighting)), not self.simpleLighting)
	self.simpleLighting = not switchOn
	local written = self:writeBoolean("simple_lighting", self.simpleLighting)
	tryCall(self.applicationController, { "setupGlobalShaderLOD", "SetupGlobalShaderLOD" })
	tryCall(self.applicationController, { "setupShadowQuality", "SetupShadowQuality" })
	return written
end

function SettingsGraphics:onShadowsSwitch(value)
	if self.isInitialising then return false end
	self.enableShadows = asBoolean(suppliedOr(value, self:getControlValue(self.shadowsSwitch, self.enableShadows)), self.enableShadows)
	local written = self:writeBoolean("enable_shadows", self.enableShadows)
	tryCall(self.applicationController, { "setupShadowQuality", "SetupShadowQuality" })
	return written
end

function SettingsGraphics:onGpuOptimisationsSwitch(value)
	if self.isInitialising then return false end
	local switchOn = asBoolean(suppliedOr(value, self:getControlValue(self.gpuOptimisationsSwitch, not self.gpuOptimisations)), not self.gpuOptimisations)
	self.gpuOptimisations = not switchOn
	local written = self:writeBoolean("gpu_optimisations", self.gpuOptimisations)
	local modelController = self.modelController
	if not modelController then local _, found = tryCall(self.applicationController, { "getModelController", "GetModelController" }); modelController = found end
	local _, model = tryCall(modelController, { "getModelAvatar", "GetModelAvatar", "getModel", "GetModel" })
	tryCall(model, { "setupQuality", "SetupQuality" })
	return written
end

function SettingsGraphics:onEnableVR(value)
	if self.isInitialising then return false end
	self.enableVR = asBoolean(suppliedOr(value, self:getControlValue(self.enableVRSwitch, self.enableVR)), self.enableVR)
	tryCall(self.applicationController, { "setVREnabled", "SetVREnabled", "enableVR", "EnableVR" }, self.enableVR)
	return self:writeBoolean("enableVR", self.enableVR)
end

function SettingsGraphics:onRotorDiscBlur(value)
	if self.isInitialising then return false end
	self.rotorDiscBlur = asBoolean(suppliedOr(value, self:getControlValue(self.rotorDiscBlurSwitch, self.rotorDiscBlur)), self.rotorDiscBlur)
	tryCall(self.applicationController, { "setRotorDiscBlurEnabled", "SetRotorDiscBlurEnabled", "enableRotorDiscBlur" }, self.rotorDiscBlur)
	return self:writeBoolean("rotor_disc_blur", self.rotorDiscBlur)
end

function SettingsGraphics:saveResolutionScale()
	self.resolutionScale = clamp(self:getControlValue(self.resolutionScaleSlider, self.resolutionScale), 1.0, 200.0)
	return self:writeSetting("resolution_scale", string.format("%.3f", self.resolutionScale))
end

function SettingsGraphics:getValues()
	return {
		renderQuality = self.renderQuality, antiAliasing = self.antiAliasing,
		targetFrameRate = self.targetFrameRate, workbench = self.workbench,
		fullscreen = self.fullscreen, vsync = self.vsync, aa = self.aa,
		simpleLighting = self.simpleLighting, enableShadows = self.enableShadows,
		gpuOptimisations = self.gpuOptimisations, enableVR = self.enableVR,
		rotorDiscBlur = self.rotorDiscBlur, resolutionScale = self.resolutionScale
	}
end

function SettingsGraphics:onEnable()
	self.isEnabled = true
	return self:setup()
end

function SettingsGraphics:onDisable()
	local saved = true
	if self.isEnabled then saved = self:saveResolutionScale() end
	if self.isEnabled and self.onSettingsApplied then
		local ok, errorMessage = pcall(self.onSettingsApplied, self, self:getValues())
		if not ok then
			self.lastError = "Graphics apply callback failed: " .. tostring(errorMessage)
			saved = false
		end
	end
	self.isEnabled = false
	return saved
end

function SettingsGraphics:start() return self:onEnable() end
function SettingsGraphics:update() end

function SettingsGraphics:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("renderQuality", self.renderQuality)
	properties:setPropertyAsInt("antiAliasing", self.antiAliasing)
	properties:setPropertyAsInt("targetFrameRate", self.targetFrameRate)
	properties:setPropertyAsInt("workbench", self.workbench)
	properties:setPropertyAsBool("fullscreen", self.fullscreen)
	properties:setPropertyAsBool("vsync", self.vsync)
	properties:setPropertyAsBool("aa", self.aa)
	properties:setPropertyAsBool("simpleLighting", self.simpleLighting)
	properties:setPropertyAsBool("enableShadows", self.enableShadows)
	properties:setPropertyAsBool("gpuOptimisations", self.gpuOptimisations)
	properties:setPropertyAsBool("enableVR", self.enableVR)
	properties:setPropertyAsBool("rotorDiscBlur", self.rotorDiscBlur)
	properties:setPropertyAsFloat("resolutionScale", self.resolutionScale)
	properties:setButtonPressed("Reload", false)
end

function SettingsGraphics:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("resolutionScale") then
		self.resolutionScale = clamp(properties:getPropertyAsFloat("resolutionScale"), 1.0, 200.0)
		self:setControlValue(self.resolutionScaleSlider, self.resolutionScale)
	end
	if properties:isButtonPressed("Reload") then self:setup() end
end

-- Compatibility aliases for the original C# event names and common script casing.
SettingsGraphics.Start = SettingsGraphics.start
SettingsGraphics.OnEnable = SettingsGraphics.onEnable
SettingsGraphics.OnDisable = SettingsGraphics.onDisable
SettingsGraphics.Setup = SettingsGraphics.setup
SettingsGraphics.Update = SettingsGraphics.update
SettingsGraphics.OnRenderQualityChanged = SettingsGraphics.onRenderQualityChanged
SettingsGraphics.OnResolutionChanged = SettingsGraphics.onResolutionChanged
SettingsGraphics.OnAntiAliasingChanged = SettingsGraphics.onAntiAliasingChanged
SettingsGraphics.OnTargetFrameRateChanged = SettingsGraphics.onTargetFrameRateChanged
SettingsGraphics.OnWorkbenchChanged = SettingsGraphics.onWorkbenchChanged
SettingsGraphics.OnFullscreenToggled = SettingsGraphics.onFullscreenToggled
SettingsGraphics.OnVsyncToggled = SettingsGraphics.onVsyncToggled
SettingsGraphics.OnAaSwitch = SettingsGraphics.onAaSwitch
SettingsGraphics.OnSimpleLightingSwitch = SettingsGraphics.onSimpleLightingSwitch
SettingsGraphics.OnShadowsSwitch = SettingsGraphics.onShadowsSwitch
SettingsGraphics.OnGpuOptimisationsSwitch = SettingsGraphics.onGpuOptimisationsSwitch
SettingsGraphics.OnEnableVR = SettingsGraphics.onEnableVR
SettingsGraphics.OnRotorDiscBlur = SettingsGraphics.onRotorDiscBlur
