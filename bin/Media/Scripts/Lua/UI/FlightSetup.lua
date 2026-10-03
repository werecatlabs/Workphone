include("FlightDialog.lua")
include("Settings.lua")

class 'CameraSettingsDialog' (FlightDialog)

function CameraSettingsDialog:__init(component)
	FlightDialog.__init(self, component)
	self.toggleGroup3d = nil
	self.toggleGroupPhoto = nil
	self.cameraMode = "pilot"
	self.sceneType = "3d"
	self.settings = {}
end

function CameraSettingsDialog:setToggleGroup3d(value) self.toggleGroup3d = value end
function CameraSettingsDialog:setToggleGroupPhoto(value) self.toggleGroupPhoto = value end
function CameraSettingsDialog:awake() return true end
function CameraSettingsDialog:setup()
	local called, settings = self:call(self:getApplicationManager(), { "getCameraSettings", "GetCameraSettings" })
	if called and settings then self.settings = settings end
	self:setupCurrentPanel()
	return true
end
function CameraSettingsDialog:start() FlightDialog.start(self); return self:setup() end
function CameraSettingsDialog:setupCurrentPanel()
	self:setControlVisible(self.toggleGroup3d, self.sceneType ~= "photo")
	self:setControlVisible(self.toggleGroupPhoto, self.sceneType == "photo")
	self:setControlText("mode", self.cameraMode)
	return true
end
function CameraSettingsDialog:setCameraMode(mode)
	self.cameraMode = tostring(mode or "pilot")
	self:call(self:getApplicationManager(), { "setCameraMode", "SetCameraMode", "selectCamera", "SelectCamera" }, self.cameraMode)
	self:setupCurrentPanel(); self:emit("cameraChanged", self.cameraMode); return true
end
function CameraSettingsDialog:onClickCameraButton3d(index) self.sceneType = "3d"; return self:setCameraMode(index or "pilot") end
function CameraSettingsDialog:onClickCameraButtonPhoto(index) self.sceneType = "photo"; return self:setCameraMode(index or "photo") end
function CameraSettingsDialog:onSceneLoaded(scene) self.sceneType = tostring(self:read(scene, {}, { "sceneType" }, self.sceneType)); return self:setup() end
function CameraSettingsDialog:onScriptsReloaded() return self:setup() end

CameraSettingsDialog.Awake = CameraSettingsDialog.awake
CameraSettingsDialog.Start = CameraSettingsDialog.start
CameraSettingsDialog.OnDestroy = FlightDialog.onDestroy
CameraSettingsDialog.Setup = CameraSettingsDialog.setup
CameraSettingsDialog.Update = FlightDialog.update
CameraSettingsDialog.OnClickCameraButton3d = CameraSettingsDialog.onClickCameraButton3d
CameraSettingsDialog.OnClickCameraButtonPhoto = CameraSettingsDialog.onClickCameraButtonPhoto
CameraSettingsDialog.SetupCurrentPanel = CameraSettingsDialog.setupCurrentPanel
CameraSettingsDialog.OnSceneLoaded = CameraSettingsDialog.onSceneLoaded
CameraSettingsDialog.OnScriptsReloaded = CameraSettingsDialog.onScriptsReloaded

class 'InputConfiguration' (FlightDialog)

function InputConfiguration:__init(component)
	FlightDialog.__init(self, component)
	self.transmittersToggleButton = nil
	self.settingsToggleButton = nil
	self.inputSettingsPanel = nil
	self.calibrationWizard = nil
	self.inputWizard = nil
	self.currentTx = nil
	self.activePanel = "transmitters"
end

function InputConfiguration:start()
	FlightDialog.start(self)
	local called, transmitter = self:call(self:getInputManager(), { "getCurrentTransmitter", "GetCurrentTransmitter" })
	if called then self.currentTx = transmitter end
	self:showPanel("transmitters")
	return true
end
function InputConfiguration:showPanel(name)
	self.activePanel = tostring(name or "transmitters")
	self:setControlVisible(self.inputSettingsPanel, self.activePanel == "settings")
	self:setControlVisible(self.calibrationWizard, self.activePanel == "calibration")
	self:setControlVisible(self.inputWizard, self.activePanel == "wizard")
	self:emit("panelChanged", self.activePanel)
	return true
end
function InputConfiguration:clickClose() return FlightDialog.clickClose(self) end
function InputConfiguration:clickShowCalibrationWizard() return self:showPanel("calibration") end
function InputConfiguration:clickHideCalibrationWizard() return self:showPanel("transmitters") end
function InputConfiguration:clickShowInputWizard() return self:showPanel("wizard") end
function InputConfiguration:clickHideInputWizard() return self:showPanel("transmitters") end

InputConfiguration.Start = InputConfiguration.start
InputConfiguration.Update = FlightDialog.update
InputConfiguration.ClickClose = InputConfiguration.clickClose
InputConfiguration.ClickShowCalibrationWizard = InputConfiguration.clickShowCalibrationWizard
InputConfiguration.ClickHideCalibrationWizard = InputConfiguration.clickHideCalibrationWizard
InputConfiguration.ClickShowInputWizard = InputConfiguration.clickShowInputWizard
InputConfiguration.ClickHideInputWizard = InputConfiguration.clickHideInputWizard

class 'SetupWizardDialog' (FlightDialog)

function SetupWizardDialog:__init(component)
	FlightDialog.__init(self, component)
	self.pages = {}
	self.pageIndex = 1
	self.options = { language = 1, device = 1, flybarless = 1, txMake = 1, txMode = 1, adapter = 1 }
	self.finished = false
end

function SetupWizardDialog:setPages(value) self.pages = self:list(value) end
function SetupWizardDialog:setupPages() for index, page in ipairs(self.pages) do self:setControlVisible(page, index == self.pageIndex) end; return #self.pages end
function SetupWizardDialog:makePageActive(index)
	self.pageIndex = math.min(math.max(1, math.floor(tonumber(index) or 1)), math.max(1, #self.pages))
	self:setupPages(); self:emit("pageChanged", self.pageIndex); return true
end
function SetupWizardDialog:start() FlightDialog.start(self); self:setupPages(); return true end
function SetupWizardDialog:onEnable() FlightDialog.onEnable(self); self.finished = false; self:setupPages(); return true end
function SetupWizardDialog:onBecameVisible() return self:onEnable() end
function SetupWizardDialog:onLanguageDropDown(value) self.options.language = tonumber(value) or 1; self:emit("languageChanged", self.options.language); return true end
function SetupWizardDialog:onDeviceDropDown(value) self.options.device = tonumber(value) or 1; self:emit("deviceChanged", self.options.device); return true end
function SetupWizardDialog:clickFinish()
	self.finished = true
	self:call(self:getInputManager(), { "applySetupWizard", "ApplySetupWizard", "saveSetup", "SaveSetup" }, self.options)
	self:emit("finished", self.options); return self:clickClose()
end

SetupWizardDialog.SetupPages = SetupWizardDialog.setupPages
SetupWizardDialog.OnEnable = SetupWizardDialog.onEnable
SetupWizardDialog.OnBecameVisible = SetupWizardDialog.onBecameVisible
SetupWizardDialog.Start = SetupWizardDialog.start
SetupWizardDialog.Update = FlightDialog.update
SetupWizardDialog.OnLanguageDropDown = SetupWizardDialog.onLanguageDropDown
SetupWizardDialog.OnDeviceDropDown = SetupWizardDialog.onDeviceDropDown
SetupWizardDialog.ClickClose = FlightDialog.clickClose
SetupWizardDialog.ClickFinish = SetupWizardDialog.clickFinish
SetupWizardDialog.MakePageActive = SetupWizardDialog.makePageActive

class 'SwitchIndicators' (FlightDialog)

function SwitchIndicators:__init(component)
	FlightDialog.__init(self, component)
	self.indicators = {}
	self.switchStatus = {}
	self.updateInterval = 0.10
end

function SwitchIndicators:setIndicators(value) self.indicators = self:list(value) end
function SwitchIndicators:setStatusFromJson(data)
	if type(data) == "table" then self.switchStatus = data
	else
		local called, decoded = self:call(self:getApplicationManager(), { "decodeJson", "DecodeJson", "fromJson", "FromJson" }, tostring(data or ""))
		if called and type(decoded) == "table" then self.switchStatus = decoded else self:setStatus("Invalid switch data", true); return false end
	end
	for name, value in pairs(self.switchStatus) do self:setControlValue(name, self:bool(value, false)) end
	self:emit("statusChanged", self.switchStatus)
	return true
end
function SwitchIndicators:update()
	FlightDialog.update(self)
	if not self:shouldUpdate() then return end
	local called, data = self:call(self:getInputManager(), { "getSwitchStatus", "GetSwitchStatus", "getSwitchStatusJson", "GetSwitchStatusJson" })
	if called and data then self:setStatusFromJson(data) end
end

SwitchIndicators.Start = FlightDialog.start
SwitchIndicators.OnDestroy = FlightDialog.onDestroy
SwitchIndicators.Update = SwitchIndicators.update
SwitchIndicators.SetStatusFromJson = SwitchIndicators.setStatusFromJson

class 'SwitchIndicatorsPanel' (SwitchIndicators)
function SwitchIndicatorsPanel:__init(component) SwitchIndicators.__init(self, component) end
SwitchIndicatorsPanel.SetStatusFromJson = SwitchIndicators.setStatusFromJson

class 'VirtualTransmitter' (FlightDialog)

function VirtualTransmitter:__init(component)
	FlightDialog.__init(self, component)
	self.channels = {}
	self.channelCount = 16
	self.inputEnabled = true
end
function VirtualTransmitter:setChannel(index, value)
	index = math.max(1, math.floor(tonumber(index) or 1)); value = self:limit(value, -1, 1, 0)
	self.channels[index] = value
	self:call(self:getInputManager(), { "setVirtualChannel", "SetVirtualChannel", "setChannel", "SetChannel" }, index - 1, value)
	return value
end
function VirtualTransmitter:getChannel(index) return self.channels[math.max(1, math.floor(tonumber(index) or 1))] or 0 end
function VirtualTransmitter:setInputEnabled(value) self.inputEnabled = value == true; return true end
function VirtualTransmitter:update()
	FlightDialog.update(self)
	if not self.inputEnabled or not self:shouldUpdate() then return end
	local called, values = self:call(self:getInputManager(), { "getVirtualChannels", "GetVirtualChannels" })
	if called and values then self.channels = self:list(values) end
end

VirtualTransmitter.Start = FlightDialog.start
VirtualTransmitter.Update = VirtualTransmitter.update
VirtualTransmitter.SetChannel = VirtualTransmitter.setChannel
VirtualTransmitter.GetChannel = VirtualTransmitter.getChannel

class 'VisualTransmitterDialog' (VirtualTransmitter)
function VisualTransmitterDialog:__init(component) VirtualTransmitter.__init(self, component) end
VisualTransmitterDialog.Update = VirtualTransmitter.update

class 'TransmitterSelect' (FlightDialog)

function TransmitterSelect:__init(component)
	FlightDialog.__init(self, component)
	self.transmitters = {}
	self.selectedIndex = 1
end
function TransmitterSelect:setTransmitters(value) self.transmitters = self:list(value) end
function TransmitterSelect:onSelect(index)
	self.selectedIndex = math.min(math.max(1, math.floor(tonumber(index) or 1)), math.max(1, #self.transmitters))
	local selected = self.transmitters[self.selectedIndex]
	self:call(self:getInputManager(), { "selectTransmitter", "SelectTransmitter", "setCurrentTransmitter", "SetCurrentTransmitter" }, selected)
	self:emit("selected", selected, self.selectedIndex); return selected ~= nil
end
TransmitterSelect.OnSelect = TransmitterSelect.onSelect

class 'TransmittersPanel' (FlightDialog)

function TransmittersPanel:__init(component)
	FlightDialog.__init(self, component)
	self.profiles = {}
	self.selectedIndex = 0
	self.needsReset = true
	self.wizard = nil
	self.setupWizard = nil
end
function TransmittersPanel:refresh()
	local called, profiles = self:call(self:getInputManager(), { "getTransmitterProfiles", "GetTransmitterProfiles" })
	if called then self.profiles = self:list(profiles) end
	self.needsReset = false; self:emit("profilesChanged", self.profiles); return self.profiles
end
function TransmittersPanel:onClickAddTransmitter() self:setControlVisible(self.wizard, true); self:emit("addRequested"); return true end
function TransmittersPanel:selectProfile(index)
	self.selectedIndex = math.min(math.max(1, math.floor(tonumber(index) or 1)), math.max(1, #self.profiles))
	local profile = self.profiles[self.selectedIndex]
	self:call(self:getInputManager(), { "selectTransmitterProfile", "SelectTransmitterProfile" }, profile)
	self:emit("selected", profile); return profile ~= nil
end
function TransmittersPanel:deleteProfile(index)
	local profile = self.profiles[math.floor(tonumber(index) or self.selectedIndex)]
	if not profile then return false end
	self:call(self:getInputManager(), { "deleteTransmitterProfile", "DeleteTransmitterProfile" }, profile)
	table.remove(self.profiles, math.floor(tonumber(index) or self.selectedIndex)); self:emit("profilesChanged", self.profiles); return true
end
function TransmittersPanel:onWizardClose() self:setControlVisible(self.wizard, false); return true end
function TransmittersPanel:onWizardFinish(profile) self:onWizardClose(); table.insert(self.profiles, profile or {}); self:emit("profilesChanged", self.profiles); return true end
function TransmittersPanel:onSetupWizardClose() self:setControlVisible(self.setupWizard, false); return true end
function TransmittersPanel:onSetupWizardFinish(profile) self:onSetupWizardClose(); return self:onWizardFinish(profile) end
function TransmittersPanel:clickImport(path) local called = self:call(self:getInputManager(), { "importTransmitterProfile", "ImportTransmitterProfile" }, path); if called then self:refresh() end; return called end
function TransmittersPanel:clickSave(path) local profile = self.profiles[self.selectedIndex]; return self:call(self:getInputManager(), { "exportTransmitterProfile", "ExportTransmitterProfile" }, profile, path) end
function TransmittersPanel:clickFileDialogSubmit(path) return self:clickImport(path) end

TransmittersPanel.OnClickAddTransmitter = TransmittersPanel.onClickAddTransmitter
TransmittersPanel.SelectProfile = TransmittersPanel.selectProfile
TransmittersPanel.DeleteProfile = TransmittersPanel.deleteProfile
TransmittersPanel.OnWizardClose = TransmittersPanel.onWizardClose
TransmittersPanel.OnWizardFinish = TransmittersPanel.onWizardFinish
TransmittersPanel.OnSetupWizardClose = TransmittersPanel.onSetupWizardClose
TransmittersPanel.OnSetupWizardFinish = TransmittersPanel.onSetupWizardFinish
TransmittersPanel.ClickImport = TransmittersPanel.clickImport
TransmittersPanel.ClickSave = TransmittersPanel.clickSave
TransmittersPanel.ClickFileDialogSubmit = TransmittersPanel.clickFileDialogSubmit

class 'ConeModes' (FlightDialog)

function ConeModes:__init(component) FlightDialog.__init(self, component); self.coneDropdown = nil; self.mode = 0 end
function ConeModes:onConeDropdownValueChanged(value)
	self.mode = math.max(0, math.floor(tonumber(value) or 0))
	self:call(self:getApplicationManager(), { "setConeMode", "SetConeMode" }, self.mode)
	self:emit("modeChanged", self.mode); return true
end
function ConeModes:onQueryResult(result) local value = self:read(result, {}, { "value", "mode" }, 0); return self:onConeDropdownValueChanged(value) end
ConeModes.Start = FlightDialog.start
ConeModes.Update = FlightDialog.update
ConeModes.OnConeDropdownValueChanged = ConeModes.onConeDropdownValueChanged
ConeModes.OnQueryResult = ConeModes.onQueryResult

class 'SystemSettings' (Settings)

function SystemSettings:__init(component) Settings.__init(self, component); self.currentPage = "graphics" end
function SystemSettings:onDropdownChanged(name, value) self.currentPage = tostring(name or self.currentPage); return true end
function SystemSettings:clickClose() return self:exitSettings() end
SystemSettings.Start = UIDialog.start
SystemSettings.Update = Settings.update
SystemSettings.OnDropdownChanged = SystemSettings.onDropdownChanged
SystemSettings.ClickClose = SystemSettings.clickClose
