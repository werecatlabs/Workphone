include("UIDialog.lua")

class 'SettingsRegion' (UIDialog)

local KEYBOARD_OPTIONS = { "QWERTY", "AZERTY" }
local SPEED_UNIT_VALUES = { "mph", "kmh" }

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

local function sqlEscape(value)
	return string.gsub(tostring(value), "'", "''")
end

local function normaliseText(value, defaultValue)
	if value == nil then return defaultValue or "" end
	value = tostring(value)
	if value == "" then return defaultValue or "" end
	return value
end

local function clampIndex(value, count, defaultValue)
	value = math.floor((tonumber(value) or defaultValue or 0) + 0.5)
	if count <= 0 then return 0 end
	if value < 0 then return 0 end
	if value >= count then return count - 1 end
	return value
end

function SettingsRegion:__init(component)
	UIDialog.__init(self, component)
	self.component = component

	self.languageDropdown = nil
	self.keyboardLayoutDropdown = nil
	self.speedUnitsDropdown = nil
	self.languageOptions = {}

	self.languageIndex = 0
	self.languageCode = "en"
	self.keyboardLayout = "qwerty"
	self.speedUnits = "mph"
	self.isInitialising = false
	self.isEnabled = false
	self.lastError = ""

	-- Optional project adapters. Persistence falls back to the engine database.
	self.settingsStore = nil
	self.applicationController = nil
	self.uiManager = nil
	self.languageTextSetter = nil
	self.onRegionalSettingsChanged = nil
end

function SettingsRegion:__finalize()
	self.isEnabled = false
	UIDialog.__finalize(self)
end

function SettingsRegion:setLanguageDropdown(value) self.languageDropdown = value end
function SettingsRegion:setKeyboardLayoutDropdown(value) self.keyboardLayoutDropdown = value end
function SettingsRegion:setSpeedUnitsDropdown(value) self.speedUnitsDropdown = value end
function SettingsRegion:setSettingsStore(value) self.settingsStore = value end
function SettingsRegion:setApplicationController(value) self.applicationController = value end
function SettingsRegion:setUIManager(value) self.uiManager = value end
function SettingsRegion:setLanguageTextSetter(value) self.languageTextSetter = value end
function SettingsRegion:setRegionalSettingsChangedCallback(value) self.onRegionalSettingsChanged = value end

function SettingsRegion:getApplicationManager()
	local ok, manager = pcall(function() return IApplicationManager.instance() end)
	return ok and manager or nil
end

function SettingsRegion:getDatabase()
	if self.settingsStore then return self.settingsStore end
	local ok, database = tryCall(self:getApplicationManager(), { "getDatabase", "GetDatabase" })
	return ok and database or nil
end

function SettingsRegion:readSetting(key, defaultValue)
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

function SettingsRegion:writeSetting(key, value)
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

function SettingsRegion:setDropdownOptions(control, options)
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
	tryCall(control, { "clearOptions", "ClearOptions", "removeAllOptions", "RemoveAllOptions" })
	local added = false
	for _, option in ipairs(options) do
		local ok = tryCall(control, { "addOption", "AddOption" }, option)
		added = added or ok
	end
	return added
end

function SettingsRegion:setDropdownIndex(control, index)
	if not control then return false end
	local called = tryCall(control, { "setSelectedOption", "SetSelectedOption", "setValue", "SetValue" }, index)
	return called
end

function SettingsRegion:getDropdownIndex(control, fallback)
	if not control then return fallback or 0 end
	local called, value = tryCall(control,
		{ "getSelectedOption", "GetSelectedOption", "getValue", "GetValue" })
	if called and tonumber(value) then return math.max(0, math.floor(tonumber(value) + 0.5)) end
	return fallback or 0
end

function SettingsRegion:setSelectedCaption(control, value)
	if not control then return false end
	local called = tryCall(control,
		{ "setSelectedText", "SetSelectedText", "setCaption", "SetCaption" }, value)
	return called
end

function SettingsRegion:loadLanguages()
	self.languageOptions = {}
	local store = self:getDatabase()
	if store then
		local queried, query = tryCall(store, { "executeQuery", "ExecuteQuery" },
			"SELECT iso_code, display_name, english_name, enabled FROM languages ORDER BY id")
		if queried and query then
			local seen = {}
			local ok, errorMessage = pcall(function()
				while not query:eof() do
					local enabled = tonumber(query:getFieldValueAsInt("enabled")) == 1
					local code = normaliseText(query:getFieldValue("iso_code"), "")
					if enabled and code ~= "" and not seen[string.lower(code)] then
						local englishName = normaliseText(query:getFieldValue("english_name"), code)
						local displayName = normaliseText(query:getFieldValue("display_name"), englishName)
						table.insert(self.languageOptions, {
							language = englishName,
							code = code,
							displayName = displayName,
							enabled = true
						})
						seen[string.lower(code)] = true
					end
					query:nextRow()
				end
			end)
			if not ok then self.lastError = "Unable to read languages: " .. tostring(errorMessage) end
		end
	end

	if #self.languageOptions == 0 then
		table.insert(self.languageOptions, {
			language = "English", code = "en", displayName = "English", enabled = true
		})
	end
	local labels = {}
	for _, option in ipairs(self.languageOptions) do table.insert(labels, option.language) end
	self:setDropdownOptions(self.languageDropdown, labels)
	return #self.languageOptions
end

function SettingsRegion:findLanguageIndex(code)
	code = string.lower(normaliseText(code, "en"))
	for index, option in ipairs(self.languageOptions) do
		if string.lower(option.code) == code then return index - 1 end
	end
	for index, option in ipairs(self.languageOptions) do
		if string.lower(option.code) == "en" then return index - 1 end
	end
	return 0
end

function SettingsRegion:setup()
	if self.isInitialising then return false end
	self.isInitialising = true
	self.lastError = ""
	local hasStore = self:getDatabase() ~= nil

	local ok, errorMessage = pcall(function()
		self:loadLanguages()
		local languageCode, foundLanguage = self:readSetting("language", "")
		if not foundLanguage or normaliseText(languageCode, "") == "" then
			languageCode = self:readSetting("languageCode", "en")
		end
		self.languageIndex = self:findLanguageIndex(languageCode)
		local language = self.languageOptions[self.languageIndex + 1]
		self.languageCode = language and language.code or "en"
		self:setDropdownIndex(self.languageDropdown, self.languageIndex)
		if language then self:setSelectedCaption(self.languageDropdown, language.displayName) end

		self.keyboardLayout = string.lower(normaliseText(self:readSetting("keyboard", "qwerty"), "qwerty"))
		if self.keyboardLayout ~= "qwerty" and self.keyboardLayout ~= "azerty" then
			self.keyboardLayout = "qwerty"
		end
		self:setDropdownOptions(self.keyboardLayoutDropdown, KEYBOARD_OPTIONS)
		self:setDropdownIndex(self.keyboardLayoutDropdown, self.keyboardLayout == "qwerty" and 0 or 1)

		self.speedUnits = string.lower(normaliseText(self:readSetting("unitsspeed", "mph"), "mph"))
		if self.speedUnits ~= "mph" and self.speedUnits ~= "kmh" then self.speedUnits = "mph" end
		self:setDropdownIndex(self.speedUnitsDropdown, self.speedUnits == "mph" and 0 or 1)
	end)

	self.isInitialising = false
	if not ok then
		self.lastError = "Unable to set up regional settings: " .. tostring(errorMessage)
		print("SettingsRegion: " .. self.lastError)
		return false
	end
	if not hasStore then
		self.lastError = "No settings store is available; defaults were loaded but cannot be persisted"
		print("SettingsRegion: " .. self.lastError)
		return false
	end
	return true
end

function SettingsRegion:getUIManager()
	if self.uiManager then return self.uiManager end
	local ok, manager = pcall(function()
		if UIManager and UIManager.instance then return UIManager.instance() end
		return nil
	end)
	return ok and manager or nil
end

function SettingsRegion:applyLanguage(option)
	local setter = self.languageTextSetter
	local ui = self:getUIManager()
	if not setter and ui then
		local found, value = tryCall(ui, { "getLanguageTextSetter", "GetLanguageTextSetter" })
		if found then setter = value end
		if not setter then
			local ok, direct = pcall(function() return ui.languageTextSetter end)
			if ok then setter = direct end
		end
	end
	tryCall(setter, { "setLanguage", "SetLanguage" }, option.code)

	local updated = tryCall(self.applicationController,
		{ "updateSettings", "UpdateSettings" }, "language", option.code)
	if not updated then tryCall(self.applicationController, { "setLanguage", "SetLanguage" }, option.code) end
	tryCall(self.applicationController,
		{ "setPreference", "SetPreference", "setPlayerPreference", "SetPlayerPreference" },
		"languageCode", option.code)
	if self.onRegionalSettingsChanged then
		local ok, errorMessage = pcall(self.onRegionalSettingsChanged, self, "language", option.code, option)
		if not ok then
			self.lastError = "Regional settings callback failed: " .. tostring(errorMessage)
			return false
		end
	end
	return true
end

function SettingsRegion:onSelectLanguage(value)
	if self.isInitialising or #self.languageOptions == 0 then return false end
	local selected = tonumber(value)
	if selected == nil then selected = self:getDropdownIndex(self.languageDropdown, self.languageIndex) end
	selected = clampIndex(selected, #self.languageOptions, self.languageIndex)
	local option = self.languageOptions[selected + 1]
	if not option or not option.enabled then return false end

	self.languageIndex = selected
	self.languageCode = option.code
	self:setDropdownIndex(self.languageDropdown, selected)
	self:setSelectedCaption(self.languageDropdown, option.displayName)
	local saved = self:writeSetting("language", option.code)
	-- Retain compatibility with the original PlayerPrefs key for non-Unity consumers.
	self:writeSetting("languageCode", option.code)
	local applied = self:applyLanguage(option)
	return saved and applied
end

function SettingsRegion:onSelectKeyboardLayout(value)
	if self.isInitialising then return false end
	local selected = tonumber(value)
	if selected == nil then selected = self:getDropdownIndex(self.keyboardLayoutDropdown, 0) end
	selected = clampIndex(selected, #KEYBOARD_OPTIONS, 0)
	self.keyboardLayout = string.lower(KEYBOARD_OPTIONS[selected + 1])
	self:setDropdownIndex(self.keyboardLayoutDropdown, selected)
	local saved = self:writeSetting("keyboard", self.keyboardLayout)
	tryCall(self.applicationController,
		{ "updateSettings", "UpdateSettings" }, "keyboard", self.keyboardLayout)
	if self.onRegionalSettingsChanged then
		local ok, errorMessage = pcall(self.onRegionalSettingsChanged, self, "keyboard", self.keyboardLayout)
		if not ok then self.lastError = "Regional settings callback failed: " .. tostring(errorMessage); return false end
	end
	return saved
end

function SettingsRegion:onUnitsSpeed(value)
	if self.isInitialising then return false end
	local selected = tonumber(value)
	if selected == nil then selected = self:getDropdownIndex(self.speedUnitsDropdown, 0) end
	selected = clampIndex(selected, #SPEED_UNIT_VALUES, 0)
	self.speedUnits = SPEED_UNIT_VALUES[selected + 1]
	self:setDropdownIndex(self.speedUnitsDropdown, selected)
	local saved = self:writeSetting("unitsspeed", self.speedUnits)
	local applied = tryCall(self.applicationController,
		{ "setSpeedUnitsIndex", "SetSpeedUnitsIndex" }, selected)
	if not applied then
		tryCall(self.applicationController, { "setSpeedUnits", "SetSpeedUnits" }, self.speedUnits)
	end
	if self.onRegionalSettingsChanged then
		local ok, errorMessage = pcall(self.onRegionalSettingsChanged, self, "unitsspeed", self.speedUnits, selected)
		if not ok then self.lastError = "Regional settings callback failed: " .. tostring(errorMessage); return false end
	end
	return saved
end

function SettingsRegion:onQueryResult(tag, queryResult)
	if tag ~= "keyboard" then return false end
	local value = queryResult
	if type(queryResult) == "table" then
		value = queryResult.value or (queryResult.rows and queryResult.rows[1] and queryResult.rows[1].value)
	elseif type(queryResult) == "string" then
		value = string.match(queryResult, '"value"%s*:%s*"([^"]+)"') or queryResult
	end
	value = string.lower(normaliseText(value, "qwerty"))
	self.keyboardLayout = value == "qwerty" and "qwerty" or "azerty"
	self:setDropdownIndex(self.keyboardLayoutDropdown, self.keyboardLayout == "qwerty" and 0 or 1)
	return true
end

function SettingsRegion:getValues()
	local language = self.languageOptions[self.languageIndex + 1]
	return {
		language = language and language.language or "English",
		languageCode = self.languageCode,
		keyboardLayout = self.keyboardLayout,
		speedUnits = self.speedUnits
	}
end

function SettingsRegion:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("languageCode", self.languageCode)
	properties:setPropertyAsString("keyboardLayout", self.keyboardLayout)
	properties:setPropertyAsString("speedUnits", self.speedUnits)
	properties:setButtonPressed("Reload", false)
end

function SettingsRegion:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("languageCode") then self.languageCode = properties:getPropertyAsString("languageCode") end
	if properties:hasProperty("keyboardLayout") then self.keyboardLayout = properties:getPropertyAsString("keyboardLayout") end
	if properties:hasProperty("speedUnits") then self.speedUnits = properties:getPropertyAsString("speedUnits") end
	if properties:isButtonPressed("Reload") then self:setup() end
end

function SettingsRegion:start() end
function SettingsRegion:update() end
function SettingsRegion:onEnable() self.isEnabled = true; return self:setup() end
function SettingsRegion:onDisable() self.isEnabled = false; return true end

-- Compatibility aliases for the original C# event names.
SettingsRegion.Start = SettingsRegion.start
SettingsRegion.Update = SettingsRegion.update
SettingsRegion.OnEnable = SettingsRegion.onEnable
SettingsRegion.OnDisable = SettingsRegion.onDisable
SettingsRegion.Setup = SettingsRegion.setup
SettingsRegion.OnQueryResult = SettingsRegion.onQueryResult
SettingsRegion.OnUnitsSpeed = SettingsRegion.onUnitsSpeed
SettingsRegion.OnSelectLanguage = SettingsRegion.onSelectLanguage
SettingsRegion.OnSelectKeyboardLayout = SettingsRegion.onSelectKeyboardLayout
