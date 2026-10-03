include("FlightHUD.lua")
include("FlightSetup.lua")
include("FlightOnline.lua")
include("FlightMedia.lua")
include("FlightWorkbench.lua")
include("FlightAliases.lua")

class 'UIManager' (BaseComponent)

local function call(object, lowerName, upperName, ...)
	if not object then return nil end
	local ok, fn = pcall(function() return object[lowerName] or object[upperName] end)
	if ok and fn then
		local called, result = pcall(fn, object, ...)
		if called then return result end
	end
	return nil
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

local function vectorItems(value)
	local result = {}
	if not value then return result end
	if type(value) == "table" then
		for _, item in ipairs(value) do table.insert(result, item) end
		return result
	end
	local called, count = tryCall(value, { "size", "Size", "count", "Count", "getSize", "GetSize" })
	if not called then count = readMember(value, {}, { "Count", "count", "Length", "length" }, 0) end
	for index = 0, math.max(0, math.floor(tonumber(count) or 0)) - 1 do
		local found, item = tryCall(value, { "at", "At", "get", "Get", "getItem", "GetItem" }, index)
		if not found then found, item = pcall(function() return value[index] end) end
		if found and item ~= nil then table.insert(result, item) end
	end
	return result
end

local function makeColour(red, green, blue, alpha)
	local ok, colour = pcall(function() return ColourF(red, green, blue, alpha) end)
	if ok then return colour end
	return { r = red, g = green, b = blue, a = alpha }
end

local function dialogVisible(dialog)
	if not dialog then return false end
	local value = call(dialog, "isVisible", "IsVisible")
	if value ~= nil then return value end
	value = readMember(dialog, {}, { "visible" }, nil)
	if value ~= nil then return value == true end
	local actor = call(dialog, "getActor", "GetActor")
	local called, enabled = tryCall(actor, { "isEnabled", "IsEnabled", "isVisible", "IsVisible" })
	return called and enabled == true
end

local function currentTime()
	local manager = UIManager._instance and UIManager._instance.applicationController or nil
	if not manager then
		local ok, value = pcall(function() return IApplicationManager.instance() end)
		if ok then manager = value end
	end
	local _, timer = tryCall(manager, { "getTimer", "GetTimer" })
	local called, value = tryCall(timer,
		{ "getTimeSinceLevelLoad", "GetTimeSinceLevelLoad", "getTime", "GetTime" })
	if called and tonumber(value) then return tonumber(value) end
	local ok, fallback = pcall(function() return os.clock() end)
	return ok and fallback or 0.0
end

local function uiField(name, label, value, property)
	return { name = name, label = label, value = value, property = property }
end

local function uiAction(name, label, callback, danger)
	return { name = name, label = label, fn = callback, danger = danger == true }
end

-- These names mirror the serialized controller references in Unity's
-- UIManager. Clearing them before committing a rebuild prevents references to
-- actors from an older generated hierarchy surviving when optional UI is off.
local GENERATED_REFERENCE_FIELDS = {
	"modelEditor", "trainingHud", "demoHud", "trainingMenu", "statsMenu",
	"airFoilPanel", "modelSettings", "quickSettings", "systemPerformance",
	"quickFixedWing", "quickTail", "devMenu", "tooltip", "cgModeInfo",
	"startMenu", "startMenuDrone", "raceSettingsMenu", "loadingScreen",
	"splashScreen", "componentSelector", "componentConfig", "componentChange",
	"switchIndicators", "onlineMenu", "setupWizard", "scenerySelector",
	"cameraSettings", "inputConfiguration", "chat", "chatDialog",
	"workbenchControls", "flightRecorder", "mp3PlayerDialog",
	"virtualTransmitter", "visualTransmitterDialog", "systemSettings", "settings",
	"fileBrowser", "modelHanger", "hud", "coneModes", "informationText",
	"sceneEditorManagerGUI", "spectatorUI", "transmitterSelect",
	"messageBoxConfirm", "txIndicatorPanel", "transmittersPanel",
	"colourPicker", "fixedWingDialog", "screenFade", "canvas", "bgCanvas",
	"htmlCanvas", "visualTxCanvas", "raceCanvas"
}

-- Extended panels share one shell renderer, but carry controller-specific
-- controls and actions. Keeping this data outside generate() makes it possible
-- to audit the complete UI without walking a large procedural function.
local EXTENDED_DIALOG_SPECS = {
	{
		key = "training_hud", actor = "TrainingHUD", className = "TrainingHud",
		title = "TRAINING HUD", subtitle = "Lesson objectives and instructor guidance",
		property = "trainingHud", layer = "hud", width = 760.0, height = 430.0,
		position = { x = -530.0, y = -260.0 }, noClose = true, noStatus = true,
		fields = { uiField("objective", "OBJECTIVE", "Awaiting lesson"), uiField("progress", "PROGRESS", "0%") }
	},
	{
		key = "demo_hud", actor = "DemoHUD", className = "DemoHud",
		title = "DEMO HUD", subtitle = "Demonstration flight status",
		property = "demoHud", layer = "hud", width = 620.0, height = 400.0,
		position = { x = 610.0, y = -280.0 }, noClose = true, noStatus = true,
		fields = { uiField("time", "ELAPSED", "00:00", "timeText"), uiField("mode", "MODE", "Demonstration") }
	},
	{
		key = "online_menu", actor = "OnlineMenu", className = "OnlineMenu",
		title = "ONLINE FLYING", subtitle = "Sessions, regions, and multiplayer rooms",
		property = "onlineMenu", fullscreen = true, width = 1420.0, height = 860.0,
		fields = { uiField("connection", "CONNECTION", "Offline"), uiField("region", "REGION", "Auto"), uiField("pilot", "PILOT", "Pilot"), uiField("sessions", "SESSIONS", "No sessions") },
		actions = { uiAction("Refresh", "Refresh", "UpdateRoomList"), uiAction("Create", "Create", "ClickCreateSession"), uiAction("Race", "Create Race", "ClickCreateRace"), uiAction("Join", "Join", "ClickJoinSession") }
	},
	{
		key = "race_settings", actor = "RaceSettings", className = "RaceSettings",
		title = "RACE SETTINGS", subtitle = "Course rules and competitors",
		property = "raceSettingsMenu", fullscreen = true, width = 1160.0, height = 760.0,
		fields = { uiField("laps", "LAPS", "3"), uiField("opponents", "OPPONENTS", "3"), uiField("raceType", "RACE TYPE", "1"), uiField("difficulty", "AI DIFFICULTY", "1") },
		actions = { uiAction("Lap", "+ Lap", "AddLaps"), uiAction("Opponent", "+ Opponent", "AddOpponent"), uiAction("RaceType", "Race Type", "AddRaceType"), uiAction("Difficulty", "Difficulty", "AddAiDifficulty"), uiAction("Apply", "Apply", "ClickBack") }
	},
	{
		key = "input_configuration", actor = "InputConfiguration", className = "InputConfiguration",
		title = "RADIO & INPUT", subtitle = "Transmitter calibration, channels, rates, and expo",
		property = "inputConfiguration", fullscreen = true, width = 1460.0, height = 880.0,
		fields = { uiField("device", "ACTIVE DEVICE", "No transmitter"), uiField("calibration", "CALIBRATION", "Required"), uiField("axes", "CHANNELS", "0 detected"), uiField("status", "INPUT STATUS", "Waiting for device") },
		actions = { uiAction("Calibrate", "Calibrate", "ClickShowCalibrationWizard"), uiAction("Wizard", "Setup Wizard", "ClickShowInputWizard") }
	},
	{
		key = "training_menu", actor = "TrainingMenu", className = "TrainingMenu",
		title = "FLIGHT TRAINING", subtitle = "Lessons and assisted practice",
		property = "trainingMenu", fullscreen = true, width = 1320.0, height = 820.0,
		fields = { uiField("lesson", "SELECTED LESSON", "None"), uiField("difficulty", "ASSISTANCE", "Standard"), uiField("progress", "COURSE PROGRESS", "0%"), uiField("status", "INSTRUCTOR", "Select a lesson") },
		actions = { uiAction("StartLesson", "Start Lesson", "StartLesson") }
	},
	{
		key = "stats_menu", actor = "StatsMenu", className = "StatsMenu",
		title = "FLIGHT STATISTICS", subtitle = "Session telemetry and pilot history",
		property = "statsMenu", fullscreen = true, width = 1260.0, height = 800.0,
		fields = { uiField("flightTime", "FLIGHT TIME", "00:00:00"), uiField("distance", "DISTANCE", "0 km"), uiField("landings", "LANDINGS", "0"), uiField("crashes", "CRASHES", "0") },
		actions = { uiAction("Refresh", "Refresh", "refresh") }
	},
	{
		key = "quick_settings", actor = "QuickSettings", title = "QUICK SETTINGS",
		subtitle = "Frequently used simulator controls", property = "quickSettings",
		width = 920.0, height = 650.0,
		fields = { uiField("wind", "WIND", "Calm"), uiField("camera", "CAMERA", "Pilot"), uiField("assistance", "ASSISTANCE", "Standard"), uiField("volume", "MASTER VOLUME", "100%") }
	},
	{
		key = "quick_fixed_wing", actor = "QuickFixedWing", className = "FixedWingDialog",
		title = "QUICK FIXED-WING", subtitle = "Rates, expo, flaps, and assistance",
		property = "quickFixedWing", width = 960.0, height = 680.0,
		fields = { uiField("rates", "RATES", "100%"), uiField("expo", "EXPO", "0%"), uiField("flaps", "FLAPS", "0%"), uiField("stability", "STABILITY", "Off") },
		actions = { uiAction("Apply", "Apply", "Apply") }
	},
	{
		key = "quick_tail", actor = "QuickTail", className = "FixedWingDialog",
		title = "QUICK TAIL SETUP", subtitle = "Tail, gyro, and yaw response",
		property = "quickTail", width = 920.0, height = 650.0,
		fields = { uiField("tailRate", "TAIL RATE", "100%"), uiField("gyro", "GYRO", "Normal"), uiField("yawExpo", "YAW EXPO", "0%"), uiField("hold", "HEADING HOLD", "Off") },
		actions = { uiAction("Apply", "Apply", "Apply") }
	},
	{
		key = "system_performance", actor = "SystemPerformance", title = "SYSTEM PERFORMANCE",
		subtitle = "Frame timing and graphics diagnostics", property = "systemPerformance",
		width = 980.0, height = 670.0,
		fields = { uiField("fps", "FRAME RATE", "-- fps"), uiField("frameTime", "FRAME TIME", "-- ms"), uiField("quality", "QUALITY", "Auto"), uiField("latency", "INPUT LATENCY", "-- ms") }
	},
	{
		key = "airfoil_panel", actor = "AirFoilPanel", className = "AirFoilPanel",
		title = "AIRFOIL ANALYSIS", subtitle = "Lift, drag, incidence, and control surfaces",
		property = "airFoilPanel", width = 1180.0, height = 780.0,
		fields = { uiField("airfoil", "AIRFOIL", "Default"), uiField("lift", "LIFT", "--"), uiField("drag", "DRAG", "--"), uiField("incidence", "INCIDENCE", "0 deg") },
		actions = { uiAction("Open", "Open", "ClickOpen"), uiAction("Save", "Save", "ClickSave"), uiAction("Reset", "Reset", "ClickReset") }
	},
	{
		key = "tooltip", actor = "Tooltip", className = "Tooltip", title = "FLIGHT HELP",
		subtitle = "Context-sensitive simulator help", property = "tooltip", layer = "overlay",
		width = 620.0, height = 400.0, position = { x = 600.0, y = 260.0 }, noClose = true, noStatus = true,
		fields = { uiField("text", "TIP", "") }
	},
	{
		key = "camera_settings", actor = "CameraSettings", className = "CameraSettingsDialog",
		title = "CAMERA SETTINGS", subtitle = "Pilot, chase, fixed, and cinematic cameras",
		property = "cameraSettings", width = 1080.0, height = 720.0,
		fields = { uiField("mode", "CAMERA MODE", "Pilot"), uiField("fieldOfView", "FIELD OF VIEW", "70 deg"), uiField("smoothing", "SMOOTHING", "50%"), uiField("shake", "CAMERA SHAKE", "On") },
		actions = { uiAction("Camera3D", "3D Camera", "OnClickCameraButton3d"), uiAction("CameraPhoto", "Photo Camera", "OnClickCameraButtonPhoto") }
	},
	{
		key = "setup_wizard", actor = "SetupWizard", className = "SetupWizardDialog",
		title = "SETUP WIZARD", subtitle = "Guided simulator and transmitter setup",
		property = "setupWizard", fullscreen = true, width = 1260.0, height = 820.0,
		fields = { uiField("page", "STEP", "1"), uiField("language", "LANGUAGE", "Auto"), uiField("device", "INPUT DEVICE", "Detecting"), uiField("summary", "SETUP STATUS", "Not complete") },
		actions = { uiAction("Finish", "Finish", "ClickFinish") }
	},
	{
		key = "chat", actor = "Chat", className = "ChatDialog", title = "PILOT CHAT",
		subtitle = "Multiplayer messages", property = "chat", aliases = { "chatDialog" }, layer = "hud",
		width = 650.0, height = 600.0, position = { x = 600.0, y = 170.0 },
		fields = { uiField("messages", "MESSAGES", "No messages"), uiField("input", "MESSAGE", "") }
	},
	{
		key = "workbench_controls", actor = "WorkbenchControls", title = "WORKBENCH CONTROLS",
		subtitle = "Aircraft editing shortcuts", property = "workbenchControls", width = 900.0, height = 640.0,
		fields = { uiField("select", "SELECT", "Left mouse"), uiField("orbit", "ORBIT", "Right mouse"), uiField("pan", "PAN", "Middle mouse"), uiField("focus", "FOCUS", "F") }
	},
	{
		key = "flight_recorder", actor = "FlightRecorder", className = "FlightRecorder",
		title = "FLIGHT RECORDER", subtitle = "Record, replay, and export flights",
		property = "flightRecorder", width = 1040.0, height = 700.0,
		fields = { uiField("recording", "RECORDING", "Stopped"), uiField("elapsed", "ELAPSED", "00:00"), uiField("file", "ACTIVE FILE", "None"), uiField("playback", "PLAYBACK", "0%") },
		actions = { uiAction("Record", "Record", "OnClickRecord"), uiAction("Play", "Play", "OnClickPlay"), uiAction("Pause", "Pause", "OnClickPause"), uiAction("Stop", "Stop", "OnClickStop") }
	},
	{
		key = "mp3_player", actor = "Mp3Player", className = "Mp3PlayerDialog",
		title = "AUDIO PLAYER", subtitle = "Flight soundtrack", property = "mp3PlayerDialog",
		width = 820.0, height = 570.0,
		fields = { uiField("track", "TRACK", "None"), uiField("elapsed", "TIME", "00:00"), uiField("volume", "VOLUME", "100%"), uiField("state", "STATE", "Stopped") },
		actions = { uiAction("Play", "Play", "OnClickPlay"), uiAction("Pause", "Pause", "OnClickPause"), uiAction("Stop", "Stop", "OnClickStop") }
	},
	{
		key = "virtual_transmitter", actor = "VirtualTransmitter", className = "VirtualTransmitter",
		title = "VIRTUAL TRANSMITTER", subtitle = "Touch and mouse flight controls",
		property = "virtualTransmitter", layer = "hud", width = 900.0, height = 430.0,
		position = { x = 0.0, y = 290.0 }, noClose = true, noStatus = true,
		fields = { uiField("channel1", "AILERON", "0%"), uiField("channel2", "ELEVATOR", "0%"), uiField("channel3", "THROTTLE", "0%"), uiField("channel4", "RUDDER", "0%") }
	},
	{
		key = "visual_transmitter_dialog", actor = "VisualTransmitterDialog", className = "VisualTransmitterDialog",
		title = "RADIO MONITOR", subtitle = "Live transmitter channel output",
		property = "visualTransmitterDialog", layer = "hud", width = 860.0, height = 470.0,
		position = { x = 0.0, y = 270.0 },
		fields = { uiField("channel1", "CH 1", "0%"), uiField("channel2", "CH 2", "0%"), uiField("channel3", "CH 3", "0%"), uiField("channel4", "CH 4", "0%") }
	},
	{
		key = "tx_indicator_panel", actor = "TxIndicatorPanel", className = "SwitchIndicatorsPanel",
		title = "CHANNEL MONITOR", subtitle = "Switch and auxiliary channel status",
		property = "txIndicatorPanel", layer = "hud", width = 820.0, height = 520.0,
		position = { x = 500.0, y = 210.0 },
		fields = { uiField("gear", "GEAR", "Off"), uiField("dualRate", "DUAL RATE", "Off"), uiField("throttleHold", "THROTTLE HOLD", "Off"), uiField("flightMode", "FLIGHT MODE", "Normal") }
	},
	{
		key = "transmitters_panel", actor = "TransmittersPanel", className = "TransmittersPanel",
		title = "TRANSMITTER PROFILES", subtitle = "Saved radio and controller profiles",
		property = "transmittersPanel", width = 1120.0, height = 740.0,
		fields = { uiField("profile", "ACTIVE PROFILE", "None"), uiField("device", "DEVICE", "Disconnected"), uiField("channels", "CHANNELS", "0"), uiField("calibration", "CALIBRATION", "Required") },
		actions = { uiAction("Add", "Add Radio", "OnClickAddTransmitter"), uiAction("Import", "Import", "ClickImport"), uiAction("Save", "Export", "ClickSave") }
	},
	{
		key = "component_selector", actor = "ComponentSelector", className = "ComponentSelector",
		title = "COMPONENT SELECTOR", subtitle = "Choose aircraft components",
		property = "componentSelector", fullscreen = true, width = 1460.0, height = 880.0,
		fields = { uiField("model", "AIRCRAFT", "None"), uiField("component", "COMPONENT", "None"), uiField("category", "CATEGORY", "All"), uiField("compatibility", "COMPATIBILITY", "Unknown") },
		actions = { uiAction("Refresh", "Refresh", "Refresh"), uiAction("Fly", "Fly", "ClickFly"), uiAction("Reset", "Reset", "ClickReset") }
	},
	{
		key = "component_config", actor = "ComponentConfig", className = "ComponentConfig",
		title = "COMPONENT CONFIGURATION", subtitle = "Tune the selected component",
		property = "componentConfig", fullscreen = true, width = 1320.0, height = 820.0,
		fields = { uiField("component", "COMPONENT", "None"), uiField("mass", "MASS", "--"), uiField("position", "POSITION", "--"), uiField("status", "CHANGES", "No changes") },
		actions = { uiAction("Apply", "Apply", "ClickApply"), uiAction("Cancel", "Cancel", "ClickCancel", true) }, noClose = true
	},
	{
		key = "component_change", actor = "ComponentChange", className = "ComponentChange",
		title = "CHANGE COMPONENT", subtitle = "Select a compatible replacement",
		property = "componentChange", fullscreen = true, width = 1320.0, height = 820.0,
		fields = { uiField("current", "CURRENT", "None"), uiField("replacement", "REPLACEMENT", "None"), uiField("category", "CATEGORY", "All"), uiField("status", "COMPATIBILITY", "Select a component") },
		actions = { uiAction("Apply", "Apply", "ClickApply"), uiAction("Cancel", "Cancel", "ClickCancel", true) }, noClose = true
	},
	{
		key = "switch_indicators", actor = "SwitchIndicators", className = "SwitchIndicators",
		title = "TRANSMITTER SWITCHES", subtitle = "Live switch and channel indicators",
		property = "switchIndicators", width = 920.0, height = 650.0,
		fields = { uiField("gear", "GEAR", "Off"), uiField("rates", "RATES", "Low"), uiField("hold", "HOLD", "Off"), uiField("mode", "MODE", "Normal") }
	},
	{
		key = "transmitter_select", actor = "TransmitterSelect", className = "TransmitterSelect",
		title = "TRANSMITTER SELECT", subtitle = "Choose and configure an input device",
		property = "transmitterSelect", fullscreen = true, width = 1120.0, height = 720.0,
		fields = { uiField("device", "DEVICE", "None"), uiField("connection", "CONNECTION", "Disconnected"), uiField("channels", "CHANNELS", "0"), uiField("profile", "PROFILE", "Default") }
	},
	{
		key = "file_browser", actor = "FileBrowser", className = "FileBrowserDialog",
		title = "FILE BROWSER", subtitle = "Import and export simulator content",
		property = "fileBrowser", fullscreen = true, width = 1380.0, height = 860.0,
		fields = { uiField("directory", "LOCATION", ""), uiField("selection", "SELECTED", "None"), uiField("filter", "FILE TYPE", "All files"), uiField("entries", "CONTENTS", "Empty") },
		actions = { uiAction("Submit", "Select", "ClickSubmit"), uiAction("Cancel", "Cancel", "ClickCancel", true) }, noClose = true
	},
	{
		key = "model_settings", actor = "ModelSettings", className = "ModelSettings",
		title = "AIRCRAFT SETTINGS", subtitle = "Physics, assistance, and model options",
		property = "modelSettings", width = 1180.0, height = 780.0,
		fields = { uiField("model", "AIRCRAFT", "None"), uiField("physics", "PHYSICS", "Realistic"), uiField("assistance", "ASSISTANCE", "Standard"), uiField("damage", "DAMAGE", "Enabled") },
		actions = { uiAction("Apply", "Apply", "Apply") }
	},
	{
		key = "colour_picker", actor = "ColourPicker", className = "ColourPicker",
		title = "COLOUR PICKER", subtitle = "Aircraft paint and UI colour selection",
		property = "colourPicker", width = 780.0, height = 620.0,
		fields = { uiField("hue", "HUE", "0 deg"), uiField("saturation", "SATURATION", "100%"), uiField("value", "BRIGHTNESS", "100%"), uiField("hex", "COLOUR", "#FFFFFF") }
	},
	{
		key = "dev_menu", actor = "DevMenu", className = "DevPanel",
		title = "DEVELOPER TOOLS", subtitle = "Diagnostics and simulation controls",
		property = "devMenu", width = 1160.0, height = 760.0,
		fields = { uiField("scene", "SCENE", "--"), uiField("model", "MODEL", "--"), uiField("fps", "FRAME RATE", "--"), uiField("state", "SIM STATE", "--") }
	},
	{
		key = "cg_mode_info", actor = "CGModeInfo", className = "CGModeInfo",
		title = "CENTRE OF GRAVITY", subtitle = "Balance and mass information",
		property = "cgModeInfo", layer = "hud", width = 720.0, height = 480.0,
		position = { x = 540.0, y = -210.0 },
		fields = { uiField("cg", "CG", "--"), uiField("mass", "MASS", "--"), uiField("forward", "FORWARD LIMIT", "--"), uiField("aft", "AFT LIMIT", "--") }
	},
	{
		key = "cone_modes", actor = "ConeModes", className = "ConeModes",
		title = "CONE MODES", subtitle = "Aerobatic training markers",
		property = "coneModes", layer = "hud", width = 680.0, height = 430.0,
		position = { x = -560.0, y = 240.0 },
		fields = { uiField("mode", "MODE", "Off"), uiField("distance", "DISTANCE", "--"), uiField("alignment", "ALIGNMENT", "--") }
	},
	{
		key = "fixed_wing", actor = "FixedWing", className = "FixedWingDialog",
		title = "FIXED-WING SETUP", subtitle = "Wing, tail, and control surface setup",
		property = "fixedWingDialog", fullscreen = true, width = 1420.0, height = 860.0,
		fields = { uiField("wing", "WING", "Default"), uiField("tail", "TAIL", "Default"), uiField("controlSurfaces", "CONTROL SURFACES", "Configured"), uiField("balance", "BALANCE", "Unknown") },
		actions = { uiAction("Apply", "Apply", "Apply") }
	},
	{
		key = "scene_editor_gui", actor = "SceneEditorGUI", className = "SceneEditorManagerGUI",
		title = "SCENERY EDITOR", subtitle = "Place, configure, and save scenery objects",
		property = "sceneEditorManagerGUI", fullscreen = true, width = 1520.0, height = 900.0,
		fields = { uiField("scene", "SCENERY", "Untitled"), uiField("selection", "SELECTED OBJECT", "None"), uiField("mode", "EDITOR MODE", "View"), uiField("terrain", "TERRAIN", "Not generated") },
		actions = { uiAction("Play", "Play", "OnClickPlay"), uiAction("Edit", "Edit", "OnClickEdit"), uiAction("Terrain", "Terrain", "GenerateTerrain") }
	},
	{
		key = "spectator_ui", actor = "SpectatorUI", title = "SPECTATOR CONTROLS",
		subtitle = "Observe pilots and switch cameras", property = "spectatorUI",
		layer = "hud", width = 780.0, height = 430.0, position = { x = 0.0, y = 280.0 },
		fields = { uiField("pilot", "PILOT", "None"), uiField("camera", "CAMERA", "Track"), uiField("position", "POSITION", "--") }
	},
	{
		key = "message_box_confirm", actor = "MessageBoxConfirm", className = "MessageBoxConfirm",
		title = "CONFIRM", subtitle = "Confirm the requested simulator action",
		property = "messageBoxConfirm", layer = "overlay", width = 720.0, height = 440.0,
		noClose = true, fields = { uiField("message", "MESSAGE", "Are you sure?") },
		actions = { uiAction("Confirm", "Confirm", "OnConfirmButton"), uiAction("Cancel", "Cancel", "OnCancelButton", true) }
	}
}

function UIManager:__init(component)
	BaseComponent.__init(self, component)
	self.component = component
	self.dialogs = {}
	self.fullscreenDialogs = {}
	self.information = {}
	self.informationText = nil
	self.loadingScreen = nil
	self.splashScreen = nil
	self.screenFade = nil
	self.applicationController = nil
	self.uiStateManager = nil
	self.isFullscreenDialogDisplayed = false
	self.isMouseOver = false
	self.zorderDirty = false
	self.selectedComponentType = 0
	self.selectedComponentConfig = -1
	self.fadeSpeed = 0.5
	self.fadeOnStart = true
	self.fadeOnExit = true
	self.fadeSequence = nil
	self.messageExpiresAt = nil
	self.fullscreenChangedCallbacks = {}
	self.generatedRoot = nil
	self.generatedActors = {}
	self.generatedControls = {}
	self.generationWarnings = {}
	self.lastError = ""
	self.referenceWidth = 1920.0
	self.referenceHeight = 1080.0
	self.materialPath = "DefaultUI.mat"
	self.autoGenerate = true
	self.generateExtendedDialogs = true
	self.startMenu = nil
	self.systemSettings = nil
	self.scenerySelector = nil
	self.modelHanger = nil
	self.modelEditor = nil
	self.hud = nil
	UIManager._instance = self
end

function UIManager:__finalize()
	if UIManager._instance == self then UIManager._instance = nil end
	BaseComponent.__finalize(self)
end

function UIManager.instance()
	return UIManager._instance
end

function UIManager:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsFloat("fadeSpeed", self.fadeSpeed)
	properties:setPropertyAsBool("fadeOnStart", self.fadeOnStart)
	properties:setPropertyAsBool("fadeOnExit", self.fadeOnExit)
	properties:setPropertyAsInt("selectedComponentType", self.selectedComponentType)
	properties:setPropertyAsInt("selectedComponentConfig", self.selectedComponentConfig)
	properties:setPropertyAsFloat("referenceWidth", self.referenceWidth)
	properties:setPropertyAsFloat("referenceHeight", self.referenceHeight)
	properties:setPropertyAsString("materialPath", self.materialPath)
	properties:setPropertyAsBool("autoGenerate", self.autoGenerate)
	properties:setPropertyAsBool("generateExtendedDialogs", self.generateExtendedDialogs)
	properties:setPropertyAsInt("generatedDialogCount", (function()
		local count, seen = 0, {}
		for _, dialog in pairs(self.dialogs) do
			if dialog and not seen[dialog] then seen[dialog] = true; count = count + 1 end
		end
		return count
	end)())
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("Generate", false);
end

function UIManager:setProperties(parameters)
	local properties = parameters:at(0)
	self.fadeSpeed = properties:getPropertyAsFloat("fadeSpeed")
	self.fadeOnStart = properties:getPropertyAsBool("fadeOnStart")
	self.fadeOnExit = properties:getPropertyAsBool("fadeOnExit")
	self.selectedComponentType = properties:getPropertyAsInt("selectedComponentType")
	self.selectedComponentConfig = properties:getPropertyAsInt("selectedComponentConfig")
	if properties:hasProperty("referenceWidth") then self.referenceWidth = math.max(640.0, properties:getPropertyAsFloat("referenceWidth")) end
	if properties:hasProperty("referenceHeight") then self.referenceHeight = math.max(360.0, properties:getPropertyAsFloat("referenceHeight")) end
	if properties:hasProperty("materialPath") then self.materialPath = properties:getPropertyAsString("materialPath") end
	if properties:hasProperty("autoGenerate") then self.autoGenerate = properties:getPropertyAsBool("autoGenerate") end
	if properties:hasProperty("generateExtendedDialogs") then self.generateExtendedDialogs = properties:getPropertyAsBool("generateExtendedDialogs") end
	if properties:isButtonPressed("Generate") then
		self:generate();
	end
end

function UIManager:generate()
	self.lastError = ""
	local warnings = {}
	local applicationManager = self.applicationController
	if not applicationManager then
		local ok, value = pcall(function() return IApplicationManager.instance() end)
		if ok then applicationManager = value end
	end
	local managerFound, gameManager = tryCall(applicationManager, { "getGameManager", "GetGameManager" })
	local rootFound, root = tryCall(self.component, { "getActor", "GetActor" })
	if not rootFound then rootFound, root = tryCall(self, { "getActor", "GetActor" }) end
	if not managerFound or not gameManager or not rootFound or not root then
		self.lastError = "UI generation requires a game manager and owning actor"
		print("UIManager:generate aborted: " .. self.lastError)
		return nil
	end

	local typeOk, userComponentTypeId = pcall(function() return UserComponent.typeInfo() end)
	if not typeOk then userComponentTypeId = nil end
	local previousVisibility = {}
	for name, dialog in pairs(self.dialogs) do previousVisibility[name] = dialogVisible(dialog) end

	local oldRoots = {}
	local function rememberRoot(candidate)
		if not candidate then return end
		for _, existing in ipairs(oldRoots) do if existing == candidate then return end end
		table.insert(oldRoots, candidate)
	end
	rememberRoot(self.generatedRoot)
	local childrenFound, children = tryCall(root, { "getChildren", "GetChildren" })
	if childrenFound then
		for _, child in ipairs(vectorItems(children)) do
			local _, name = tryCall(child, { "getName", "GetName" })
			if name == "__GameUIGenerated" then rememberRoot(child) end
		end
	end

	local newRoot = nil
	local newDialogs = {}
	local newFullscreenDialogs = {}
	local newActors = {}
	local newControls = {}
	local newReferences = {}

	local function warn(message)
		table.insert(warnings, tostring(message))
	end

	local function addComponent(actor, className, required)
		local called, component = tryCall(actor, { "addComponent", "AddComponent" }, className)
		if not called or not component then
			if required then error("could not add required " .. className .. " component") end
			warn("could not add " .. className .. " to " .. tostring(readMember(actor, { "getName", "GetName" }, {}, "actor")))
			return nil
		end
		return component
	end

	local function createActor(parent, name, required)
		local called, actor = tryCall(gameManager, { "createActor", "CreateActor" })
		if not called or not actor then
			if required then error("could not create required actor " .. name) end
			warn("could not create actor " .. name)
			return nil
		end
		tryCall(actor, { "setName", "SetName" }, name)
		local added = tryCall(parent, { "addChild", "AddChild" }, actor)
		if not added then
			tryCall(gameManager, { "destroyActor", "DestroyActor" }, actor, true)
			if required then error("could not parent required actor " .. name) end
			warn("could not parent actor " .. name)
			return nil
		end
		return actor
	end

	local function addTransform(actor, position, size, zOrder, required)
		local transform = addComponent(actor, "LayoutTransform", required)
		if not transform then return nil end
		local positioned = tryCall(transform, { "setPosition", "SetPosition" }, position)
		local sized = tryCall(transform, { "setSize", "SetSize" }, size)
		tryCall(transform, { "setZOrder", "SetZOrder" }, zOrder or 0)
		if required and (not positioned or not sized) then error("could not configure required layout transform") end
		return transform
	end

	local function addMaterial(actor)
		if self.materialPath == "" then return nil end
		local material = addComponent(actor, "Material", false)
		if material then tryCall(material, { "setMaterialPath", "SetMaterialPath" }, self.materialPath) end
		return material
	end

	local colours = {
		background = makeColour(0.008, 0.016, 0.030, 1.0),
		layer = makeColour(0.014, 0.030, 0.052, 0.96),
		panel = makeColour(0.025, 0.055, 0.090, 0.98),
		panelSoft = makeColour(0.035, 0.075, 0.115, 0.94),
		control = makeColour(0.060, 0.135, 0.210, 1.0),
		controlHighlight = makeColour(0.100, 0.330, 0.520, 1.0),
		controlPressed = makeColour(0.040, 0.230, 0.390, 1.0),
		disabled = makeColour(0.100, 0.120, 0.140, 0.55),
		primary = makeColour(0.930, 0.970, 1.0, 1.0),
		secondary = makeColour(0.530, 0.700, 0.840, 1.0),
		accent = makeColour(0.120, 0.720, 0.900, 1.0),
		good = makeColour(0.180, 0.820, 0.520, 1.0),
		warning = makeColour(1.000, 0.670, 0.180, 1.0),
		danger = makeColour(0.700, 0.150, 0.170, 1.0),
		transparent = makeColour(0.0, 0.0, 0.0, 0.0)
	}

	local function createPanel(parent, name, position, size, colour, zOrder, required)
		local actor = createActor(parent, name, required)
		if not actor then return nil, nil end
		addTransform(actor, position, size, zOrder or 0, required)
		local image = addComponent(actor, "Image", required)
		if image then tryCall(image, { "setColour", "SetColour", "setColor", "SetColor" }, colour or colours.panel) end
		addMaterial(actor)
		return actor, image
	end

	local function createText(parent, name, textValue, position, size, colour, zOrder)
		local actor = createActor(parent, name, false)
		if not actor then return nil, nil end
		addTransform(actor, position, size, zOrder or 3, false)
		local text = addComponent(actor, "Text", false)
		if text then
			tryCall(text, { "setText", "SetText" }, tostring(textValue or ""))
			tryCall(text, { "setColour", "SetColour", "setColor", "SetColor" }, colour or colours.primary)
			tryCall(text, { "setHorizontalAlignment", "SetHorizontalAlignment" }, 1)
			tryCall(text, { "setVerticalAlignment", "SetVerticalAlignment" }, 1)
		end
		return actor, text
	end

	local function wireButton(button, targetComponent, functionName)
		if not button or not targetComponent or not functionName then return false end
		local eventsFound, events = tryCall(button, { "getEvents", "GetEvents" })
		if not eventsFound then return false end
		local wired = false
		for _, event in ipairs(vectorItems(events)) do
			local _, eventHash = tryCall(event, { "getEventHash", "GetEventHash" })
			local activateHash = nil
			pcall(function() activateHash = IEvent.ACTIVATE_HASH end)
			if not activateHash or eventHash == activateHash then
				local listenersFound, listeners = tryCall(event, { "getListeners", "GetListeners" })
				if listenersFound then
					for _, listener in ipairs(vectorItems(listeners)) do
						local componentSet = tryCall(listener, { "setComponent", "SetComponent" }, targetComponent)
						local functionSet = tryCall(listener, { "setFunction", "SetFunction" }, functionName)
						wired = wired or (componentSet and functionSet)
					end
				end
			end
		end
		return wired
	end

	local function createButton(parent, name, label, position, size, targetComponent, functionName, colour)
		local actor = createActor(parent, name, false)
		if not actor then return nil, nil end
		local wired = targetComponent == nil or functionName == nil
		addTransform(actor, position, size or Vector2F(220.0, 58.0), 2, false)
		local image = addComponent(actor, "Image", false)
		if image then tryCall(image, { "setColour", "SetColour" }, colour or colours.control) end
		local button = addComponent(actor, "Button", false)
		if button then
			tryCall(button, { "setTextStr", "SetTextStr" }, tostring(label or name))
			tryCall(button, { "setTextSize", "SetTextSize" }, 22)
			tryCall(button, { "setNormalColour", "SetNormalColour" }, colour or colours.control)
			tryCall(button, { "setHighlightedColour", "SetHighlightedColour" }, colours.controlHighlight)
			tryCall(button, { "setPressedColour", "SetPressedColour" }, colours.controlPressed)
			tryCall(button, { "setDisabledColour", "SetDisabledColour" }, colours.disabled)
			if targetComponent and functionName then
				wired = wireButton(button, targetComponent, functionName)
				if not wired then warn(name .. " has no activation listener") end
			end
		end
		addMaterial(actor)
		local buttonWidth = tonumber(readMember(size, {}, { "x" }, 220.0)) or 220.0
		local buttonHeight = tonumber(readMember(size, {}, { "y" }, 58.0)) or 58.0
		createText(actor, "Label", label, Vector2F(0.0, 0.0),
			Vector2F(buttonWidth - 14.0, buttonHeight - 8.0), colours.primary, 3)
		return actor, button, wired
	end

	local function addScriptComponent(actor, className, required)
		local component = nil
		if userComponentTypeId then
			local called, value = tryCall(actor, { "addComponentById", "AddComponentById" }, userComponentTypeId)
			if called then component = value end
		end
		if not component then component = addComponent(actor, "UserComponent", required) end
		if not component then return nil end
		local named = tryCall(component, { "setClassName", "SetClassName" }, className)
		if not named then
			if required then error("could not assign required script class " .. className) end
			warn("could not assign script class " .. className)
			return nil
		end
		local scriptFound, script = tryCall(component, { "getScriptClass", "GetScriptClass" })
		return { actor = actor, component = component, script = scriptFound and script or component, className = className }
	end

	local function addDialogAlias(dialogs, fullscreen, name, record, isFullscreen)
		if not name or name == "" or not record then return end
		dialogs[name] = record.script
		fullscreen[name] = isFullscreen == true
	end

	local function createDialogRecord(parent, key, actorName, className, isFullscreen, required)
		if newActors[key] then error("duplicate generated dialog key " .. tostring(key)) end
		local actor = createActor(parent, actorName, required)
		if not actor then return nil end
		addTransform(actor, Vector2F(0.0, 0.0), Vector2F(self.referenceWidth, self.referenceHeight), 0, required)
		local record = addScriptComponent(actor, className, required)
		if not record then
			tryCall(gameManager, { "destroyActor", "DestroyActor" }, actor, true)
			return nil
		end
		record.key = key
		record.fullscreen = isFullscreen == true
		record.required = required == true
		newActors[key] = actor
		addDialogAlias(newDialogs, newFullscreenDialogs, key, record, isFullscreen)
		addDialogAlias(newDialogs, newFullscreenDialogs, actorName, record, isFullscreen)
		return record
	end

	local function bindControl(record, name, control, property)
		if not record or not control or not name then return false end
		local bound = tryCall(record.script, { "bindControl", "BindControl" }, name, control)
		newControls[record.key .. "." .. name] = control
		if not newControls[name] then newControls[name] = control end
		if property and property ~= "" then pcall(function() record.script[property] = control end) end
		return bound
	end

	local function createDialogShell(record, title, subtitle, actions, width, height, fields, position, noStatus)
		width = width or 1260.0
		height = height or 820.0
		fields = fields or {}
		actions = actions or {}
		position = position or { x = 0.0, y = 0.0 }
		if record.fullscreen then
			createPanel(record.actor, "ModalBackdrop", Vector2F(0.0, 0.0),
				Vector2F(self.referenceWidth, self.referenceHeight), colours.layer, 0, record.required)
		end
		local card = createPanel(record.actor, "DialogCard", Vector2F(position.x or 0.0, position.y or 0.0),
			Vector2F(width, height), colours.panel, 1, record.required)
		if not card then return false end
		local _, titleText = createText(card, "Title", title, Vector2F(0.0, -height * 0.5 + 62.0),
			Vector2F(width - 100.0, 70.0), colours.primary, 3)
		createText(card, "Subtitle", subtitle or "", Vector2F(0.0, -height * 0.5 + 112.0),
			Vector2F(width - 120.0, 42.0), colours.secondary, 3)
		local content = createPanel(card, "Content", Vector2F(0.0, 15.0),
			Vector2F(width - 90.0, height - 270.0), colours.layer, 2, record.required)
		if record.required and not titleText then error("required title was not created for " .. record.key) end
		if content then
			local contentWidth = width - 90.0
			local contentHeight = height - 270.0
			local columns = #fields > 1 and 2 or 1
			local rows = math.max(1, math.ceil(#fields / columns))
			local gap = 18.0
			local fieldWidth = (contentWidth - gap * (columns + 1)) / columns
			local footerHeight = noStatus and 0.0 or 58.0
			local fieldHeight = math.max(58.0,
				math.min(112.0, (contentHeight - footerHeight - gap * (rows + 1)) / rows))
			local hasStatusField = false
			for index, field in ipairs(fields) do
				local column = (index - 1) % columns
				local row = math.floor((index - 1) / columns)
				local x = (column - (columns - 1) * 0.5) * (fieldWidth + gap)
				local y = -contentHeight * 0.5 + gap + fieldHeight * 0.5 + row * (fieldHeight + gap)
				local fieldActor = createPanel(content, field.name .. "Field", Vector2F(x, y),
					Vector2F(fieldWidth, fieldHeight), colours.panelSoft, 2, false)
				if fieldActor then
					createText(fieldActor, "Label", field.label, Vector2F(0.0, -fieldHeight * 0.22),
						Vector2F(fieldWidth - 28.0, 30.0), colours.secondary, 3)
					local _, valueText = createText(fieldActor, "Value", field.value or "",
						Vector2F(0.0, fieldHeight * 0.19), Vector2F(fieldWidth - 28.0, 42.0), colours.primary, 3)
					bindControl(record, field.name, valueText, field.property)
					hasStatusField = hasStatusField or field.name == "status"
				end
			end
			if not hasStatusField and not noStatus then
				local _, statusText = createText(content, "Status", "Ready - awaiting simulation data",
					Vector2F(0.0, contentHeight * 0.5 - 31.0),
					Vector2F(contentWidth - 60.0, 42.0), colours.secondary, 3)
				bindControl(record, "status", statusText)
			end
		end
		local count = #actions
		local spacing = math.min(220.0, (width - 100.0) / math.max(1, count))
		for index, action in ipairs(actions) do
			local x = (index - (count + 1) * 0.5) * spacing
			local _, button, wired = createButton(card, action.name .. "Button", action.label, Vector2F(x, height * 0.5 - 68.0),
				Vector2F(math.min(200.0, spacing - 12.0), 56.0), record.component, action.fn,
				action.danger and colours.danger or colours.control)
			if record.required and (not button or not wired) then error("required action " .. action.name .. " was not wired for " .. record.key) end
			bindControl(record, action.name .. "Button", button)
		end
		return true, content
	end

	local function prepareDialog(record)
		if not record then return end
		if record.prepared then return true end
		tryCall(record.script, { "setApplicationController", "SetApplicationController" }, applicationManager)
		tryCall(record.script, { "setUIManager", "SetUIManager" }, self)
		tryCall(record.script, { "setUIStateManager", "SetUIStateManager" }, self.uiStateManager)
		tryCall(record.script, { "setDialogName", "SetDialogName" }, record.key)
		tryCall(record.script, { "setDialogReference", "SetDialogReference" }, record.key)
		record.prepared = true
		return true
	end

	local function initialiseDialog(record, visible)
		if not record then return end
		prepareDialog(record)
		tryCall(record.script, { "start", "Start", "initialise", "Initialise" })
		if visible then
			local shown = tryCall(record.script, { "show", "Show" }, true)
			if not shown then tryCall(record.actor, { "setEnabled", "SetEnabled" }, true) end
		else
			local hidden = tryCall(record.script, { "hide", "Hide" }, true)
			if not hidden then tryCall(record.actor, { "setEnabled", "SetEnabled" }, false) end
		end
	end

	local function createGenericDialog(parent, spec)
		local record = createDialogRecord(parent, spec.key, spec.actor, spec.className or "FlightDialog",
			spec.fullscreen, spec.required == true)
		if not record then return nil end
		prepareDialog(record)
		local actions = {}
		for _, action in ipairs(spec.actions or {}) do table.insert(actions, action) end
		if not spec.noClose then table.insert(actions, { name = "Close", label = "Close", fn = "Hide", danger = true }) end
		local built = createDialogShell(record, spec.title, spec.subtitle, actions, spec.width, spec.height,
			spec.fields, spec.position, spec.noStatus)
		if not built then
			if spec.required then error("could not build required dialog " .. spec.key) end
			warn("dialog shell was incomplete for " .. spec.key)
		end
		initialiseDialog(record, false)
		if spec.property then newReferences[spec.property] = record.script end
		for _, alias in ipairs(spec.aliases or {}) do newReferences[alias] = record.script end
		return record
	end

	local function createInstrument(parent, name, label, value, position, accent)
		local card = createPanel(parent, name, position, Vector2F(218.0, 88.0), colours.panelSoft, 2, false)
		if not card then return nil end
		createText(card, "Label", label, Vector2F(0.0, -20.0), Vector2F(196.0, 30.0), colours.secondary, 3)
		local _, valueText = createText(card, "Value", value, Vector2F(0.0, 19.0), Vector2F(196.0, 42.0), accent or colours.primary, 3)
		newControls[name] = valueText
		return valueText
	end

	local function destroyGenerated(candidate)
		if not candidate then return true end
		local destroyed = tryCall(gameManager, { "destroyActor", "DestroyActor" }, candidate, true)
		if not destroyed then
			tryCall(root, { "removeChild", "RemoveChild" }, candidate)
			tryCall(candidate, { "setEnabled", "SetEnabled" }, false)
		end
		return destroyed
	end

	local buildOk, buildResult = pcall(function()
		newRoot = createActor(root, "__GameUIGenerated", true)
		tryCall(newRoot, { "setEnabled", "SetEnabled" }, false)
		addTransform(newRoot, Vector2F(0.0, 0.0), Vector2F(self.referenceWidth, self.referenceHeight), 0, true)
		addComponent(newRoot, "Layout", false)

		local backgroundLayer = createPanel(newRoot, "BackgroundLayer", Vector2F(0.0, 0.0),
			Vector2F(self.referenceWidth, self.referenceHeight), colours.transparent, 0, true)
		local menuLayer = createActor(newRoot, "MenuLayer", true)
		local dialogLayer = createActor(newRoot, "DialogLayer", true)
		local hudLayer = createActor(newRoot, "HudLayer", true)
		local overlayLayer = createActor(newRoot, "OverlayLayer", true)
		for index, layer in ipairs({ menuLayer, dialogLayer, hudLayer, overlayLayer }) do
			addTransform(layer, Vector2F(0.0, 0.0), Vector2F(self.referenceWidth, self.referenceHeight), index, true)
		end
		newActors.root = newRoot
		newActors.background = backgroundLayer
		newActors.menu_layer = menuLayer
		newActors.dialog_layer = dialogLayer
		newActors.hud_layer = hudLayer
		newActors.overlay_layer = overlayLayer
		newReferences.canvas = newRoot
		newReferences.bgCanvas = backgroundLayer
		newReferences.htmlCanvas = dialogLayer

		-- Start menu and system settings use their full Lua generators.
		local startRecord = createDialogRecord(menuLayer, "start_menu", "StartMenu", "StartMenu", true, true)
		prepareDialog(startRecord)
		local startCalled, generatedStart = tryCall(startRecord.script, { "generate", "Generate" })
		if not startCalled or not generatedStart then error("StartMenu generation failed") end
		newReferences.startMenu = startRecord.script

		local droneStartRecord = createDialogRecord(menuLayer, "start_menu_drone", "StartMenuDrone", "StartMenu", true, true)
		prepareDialog(droneStartRecord)
		pcall(function()
			droneStartRecord.script.title = "LIONCAT DRONE"
			droneStartRecord.script.subtitle = "Drone Simulation"
		end)
		local droneCalled, generatedDrone = tryCall(droneStartRecord.script, { "generate", "Generate" })
		if not droneCalled or not generatedDrone then error("StartMenuDrone generation failed") end
		newReferences.startMenuDrone = droneStartRecord.script

		local settingsRecord = createDialogRecord(dialogLayer, "settings", "Settings", "SystemSettings", true, true)
		prepareDialog(settingsRecord)
		local settingsCalled, generatedSettings = tryCall(settingsRecord.script, { "generate", "Generate" })
		if not settingsCalled or not generatedSettings then error("Settings generation failed") end
		newReferences.systemSettings = settingsRecord.script
		newReferences.settings = settingsRecord.script
		tryCall(startRecord.script, { "setSettingsController", "SetSettingsController" }, settingsRecord.script)
		tryCall(startRecord.script, { "setNavigationController", "SetNavigationController" }, self.uiStateManager or self)
		tryCall(droneStartRecord.script, { "setSettingsController", "SetSettingsController" }, settingsRecord.script)
		tryCall(droneStartRecord.script, { "setNavigationController", "SetNavigationController" }, self.uiStateManager or self)
		tryCall(settingsRecord.script, { "setHomeController", "SetHomeController" }, self)
		local graphicsButton = readMember(settingsRecord.script, {}, { "graphicsButton" }, nil)
		local audioButton = readMember(settingsRecord.script, {}, { "audioButton" }, nil)
		local exitButton = readMember(settingsRecord.script, {}, { "exitButton" }, nil)
		wireButton(graphicsButton, settingsRecord.component, "showGraphics")
		wireButton(audioButton, settingsRecord.component, "showAudio")
		wireButton(exitButton, settingsRecord.component, "exitSettings")
		initialiseDialog(startRecord, false)
		initialiseDialog(droneStartRecord, false)
		initialiseDialog(settingsRecord, false)

		-- Scenery selector shell, backed by the converted selector controller.
		local sceneRecord = createDialogRecord(dialogLayer, "scenery_selector", "ScenerySelector", "ScenerySelector", true, true)
		local sceneShell, sceneContent = createDialogShell(sceneRecord, "FLIGHT LOCATION", "Choose scenery, weather profile, and flight environment", {
			{ name = "All", label = "All", fn = "ClickAllScenery" },
			{ name = "Photo", label = "Photo", fn = "ClickPhotoScenery" },
			{ name = "ThreeD", label = "3D", fn = "Click3dScenery" },
			{ name = "Race", label = "Race", fn = "ClickRaceTrackScenery" },
			{ name = "Fly", label = "Load", fn = "ClickSelectScenery" },
			{ name = "Close", label = "Close", fn = "ClickClose", danger = true }
		}, 1540.0, 900.0)
		if not sceneShell or not sceneContent then error("ScenerySelector shell generation failed") end
		local builtInScenes = createPanel(sceneContent, "BuiltInSceneryPanel", Vector2F(-480.0, -15.0),
			Vector2F(440.0, 500.0), colours.panelSoft, 2, true)
		local customScenes = createPanel(sceneContent, "CustomSceneryPanel", Vector2F(0.0, -15.0),
			Vector2F(440.0, 500.0), colours.panelSoft, 2, true)
		local onlineScenes = createPanel(sceneContent, "OnlineSceneryPanel", Vector2F(480.0, -15.0),
			Vector2F(440.0, 500.0), colours.panelSoft, 2, true)
		local builtInSceneContent = createActor(builtInScenes, "Items", true)
		local customSceneContent = createActor(customScenes, "Items", true)
		local onlineSceneContent = createActor(onlineScenes, "Items", true)
		addTransform(builtInSceneContent, Vector2F(0.0, 30.0), Vector2F(400.0, 390.0), 3, true)
		addTransform(customSceneContent, Vector2F(0.0, 30.0), Vector2F(400.0, 390.0), 3, true)
		addTransform(onlineSceneContent, Vector2F(0.0, 30.0), Vector2F(400.0, 390.0), 3, true)
		for _, panelInfo in ipairs({
			{ actor = builtInScenes, content = builtInSceneContent, title = "BUILT-IN LOCATIONS", key = "built_in_scenery" },
			{ actor = customScenes, content = customSceneContent, title = "MY LOCATIONS", key = "custom_scenery" },
			{ actor = onlineScenes, content = onlineSceneContent, title = "ONLINE LOCATIONS", key = "online_scenery" }
		}) do
			if panelInfo.actor then
				createText(panelInfo.actor, "Title", panelInfo.title, Vector2F(0.0, -210.0),
					Vector2F(400.0, 42.0), colours.accent, 3)
				createText(panelInfo.content or panelInfo.actor, "EmptyState", "Scenery cards load at runtime", Vector2F(0.0, 0.0),
					Vector2F(380.0, 42.0), colours.secondary, 3)
				newActors[panelInfo.key] = panelInfo.actor
				newActors[panelInfo.key .. "_content"] = panelInfo.content
			end
		end
		tryCall(sceneRecord.script, { "setScenerySelectPanel", "SetScenerySelectPanel" }, builtInScenes)
		tryCall(sceneRecord.script, { "setBuiltInSelectPanel", "SetBuiltInSelectPanel" }, builtInScenes)
		tryCall(sceneRecord.script, { "setCustomSelectPanel", "SetCustomSelectPanel" }, customScenes)
		tryCall(sceneRecord.script, { "setOnlineScenesPanel", "SetOnlineScenesPanel" }, onlineScenes)
		tryCall(sceneRecord.script, { "setRaceSelectPanel", "SetRaceSelectPanel" }, onlineScenes)
		tryCall(sceneRecord.script, { "setScrollContent", "SetScrollContent" }, builtInSceneContent)
		tryCall(sceneRecord.script, { "setCustomScrollContent", "SetCustomScrollContent" }, customSceneContent)
		tryCall(sceneRecord.script, { "setOnlineScrollContent", "SetOnlineScrollContent" }, onlineSceneContent)
		newReferences.scenerySelector = sceneRecord.script
		initialiseDialog(sceneRecord, false)

		-- Aircraft hanger with three panel controllers and launch actions.
		local vehicleRecord = createDialogRecord(dialogLayer, "model_hanger", "ModelHanger", "ModelHangerDialog", true, true)
		local vehicleShell, vehicleContent = createDialogShell(vehicleRecord, "AIRCRAFT HANGAR", "Select an aircraft and choose how to enter the simulator", {
			{ name = "All", label = "All", fn = "ClickAllModels" },
			{ name = "Helicopters", label = "Helicopters", fn = "ClickHelicopters" },
			{ name = "Planes", label = "Planes", fn = "ClickPlanes" },
			{ name = "Drones", label = "Drones", fn = "ClickDrones" },
			{ name = "Cars", label = "Ground", fn = "ClickCars" },
			{ name = "Close", label = "Close", fn = "ClickClose", danger = true }
		}, 1600.0, 920.0)
		if not vehicleShell or not vehicleContent then error("ModelHanger shell generation failed") end
		local builtInPanelActor = createPanel(vehicleContent, "BuiltInModelsPanel", Vector2F(-500.0, 0.0),
			Vector2F(500.0, 510.0), colours.panelSoft, 2, true)
		local myModelsPanelActor = createPanel(vehicleContent, "MyModelsPanel", Vector2F(0.0, 0.0),
			Vector2F(470.0, 510.0), colours.panelSoft, 2, true)
		local onlinePanelActor = createPanel(vehicleContent, "OnlineModelsPanel", Vector2F(500.0, 0.0),
			Vector2F(500.0, 510.0), colours.panelSoft, 2, true)
		createText(builtInPanelActor, "Title", "BUILT-IN AIRCRAFT", Vector2F(0.0, -210.0), Vector2F(440.0, 46.0), colours.accent, 3)
		createText(myModelsPanelActor, "Title", "MY AIRCRAFT", Vector2F(0.0, -210.0), Vector2F(410.0, 46.0), colours.accent, 3)
		createText(onlinePanelActor, "Title", "WORKSHOP", Vector2F(0.0, -210.0), Vector2F(440.0, 46.0), colours.accent, 3)
		local builtInPanel = addScriptComponent(builtInPanelActor, "VehiclesPanel", true)
		local myModelsPanel = addScriptComponent(myModelsPanelActor, "VehiclesPanel", true)
		local onlinePanel = addScriptComponent(onlinePanelActor, "VehiclesPanel", true)
		for _, panelInfo in ipairs({
			{ record = builtInPanel, key = "built_in_models" },
			{ record = myModelsPanel, key = "my_models" },
			{ record = onlinePanel, key = "online_models" }
		}) do
			if panelInfo.record then
				panelInfo.record.key = panelInfo.key
				prepareDialog(panelInfo.record)
				tryCall(panelInfo.record.script, { "start", "Start", "initialise", "Initialise" })
				newActors[panelInfo.key] = panelInfo.record.actor
			end
		end
		if builtInPanel then tryCall(vehicleRecord.script, { "setModelSelectPanel", "SetModelSelectPanel" }, builtInPanel.script) end
		if myModelsPanel then tryCall(vehicleRecord.script, { "setMyModelsPanel", "SetMyModelsPanel" }, myModelsPanel.script) end
		if onlinePanel then tryCall(vehicleRecord.script, { "setOnlineModelsPanel", "SetOnlineModelsPanel" }, onlinePanel.script) end
		local _, modelNameText = createText(vehicleContent, "SelectedAircraftName", "No aircraft selected",
			Vector2F(0.0, 205.0), Vector2F(700.0, 50.0), colours.primary, 3)
		local _, workbenchButton, workbenchWired = createButton(vehicleContent, "WorkbenchButton", "Open Workbench",
			Vector2F(-140.0, 250.0), Vector2F(250.0, 58.0), vehicleRecord.component, "ClickSelectModelWB", colours.control)
		local _, flyButton, flyWired = createButton(vehicleContent, "FlyButton", "Fly",
			Vector2F(140.0, 250.0), Vector2F(250.0, 58.0), vehicleRecord.component, "ClickSelectModel", colours.good)
		if not modelNameText or not workbenchButton or not flyButton or not workbenchWired or not flyWired then
			error("ModelHanger launch controls were not completely generated")
		end
		tryCall(vehicleRecord.script, { "setModelNameText", "SetModelNameText" }, modelNameText)
		tryCall(vehicleRecord.script, { "setWorkbenchButton", "SetWorkbenchButton" }, workbenchButton)
		tryCall(vehicleRecord.script, { "setFlyButton", "SetFlyButton" }, flyButton)
		newReferences.modelHanger = vehicleRecord.script
		initialiseDialog(vehicleRecord, false)

		local editorRecord = createDialogRecord(dialogLayer, "model_editor", "ModelEditor", "ModelEditor", true, false)
		if editorRecord then
			createDialogShell(editorRecord, "AIRCRAFT WORKBENCH", "Configure airframe, power system, radio, and flight setup",
				{
					uiAction("Reload", "Reload", "OnClickReload"),
					uiAction("Update", "Apply Changes", "OnClickUpdate"),
					uiAction("Close", "Close", "Hide", true)
				}, 1500.0, 900.0, {
					uiField("selectedObject", "SELECTED OBJECT", "None", "selectedObjectText"),
					uiField("properties", "PROPERTIES", "No component selected", "propertiesText"),
					uiField("angularLimits", "ANGULAR LIMITS", "--", "angularLimitsText"),
					uiField("validation", "MODEL STATUS", "Ready")
				})
			newReferences.modelEditor = editorRecord.script
			initialiseDialog(editorRecord, false)
		end

		-- In-flight HUD: intentionally sparse in the centre to preserve visibility.
		local hudRecord = createDialogRecord(hudLayer, "hud", "FlightHUD", "Hud", false, true)
		newReferences.hud = hudRecord.script
		local topBar = createPanel(hudRecord.actor, "TopStatusBar", Vector2F(0.0, -496.0),
			Vector2F(1880.0, 72.0), colours.layer, 1, false)
		local _, modeText = createText(topBar, "Mode", "FLIGHT READY", Vector2F(-760.0, 0.0), Vector2F(260.0, 44.0), colours.good, 3)
		local _, aircraftText = createText(topBar, "Aircraft", "NO AIRCRAFT", Vector2F(-80.0, 0.0), Vector2F(620.0, 44.0), colours.primary, 3)
		local _, clockText = createText(topBar, "Clock", "SIM 00:00", Vector2F(640.0, 0.0), Vector2F(260.0, 44.0), colours.secondary, 3)
		local _, fpsText = createText(topBar, "FrameRate", "FPS --", Vector2F(850.0, 0.0), Vector2F(130.0, 44.0), colours.secondary, 3)
		bindControl(hudRecord, "Mode", modeText)
		bindControl(hudRecord, "Aircraft", aircraftText)
		bindControl(hudRecord, "Clock", clockText)
		bindControl(hudRecord, "FrameRate", fpsText)
		tryCall(hudRecord.script, { "setControlsFpsText", "SetControlsFpsText" }, fpsText)
		createInstrument(hudRecord.actor, "Altitude", "ALTITUDE", "0 m", Vector2F(825.0, -360.0), colours.accent)
		createInstrument(hudRecord.actor, "Airspeed", "AIR SPEED", "0 km/h", Vector2F(825.0, -258.0), colours.accent)
		createInstrument(hudRecord.actor, "Heading", "HEADING", "000 deg", Vector2F(825.0, -156.0), colours.primary)
		createInstrument(hudRecord.actor, "Battery", "BATTERY", "100%", Vector2F(825.0, -54.0), colours.good)
		createInstrument(hudRecord.actor, "Signal", "RADIO LINK", "READY", Vector2F(825.0, 48.0), colours.good)
		for _, name in ipairs({ "Altitude", "Airspeed", "Heading", "Battery", "Signal" }) do
			bindControl(hudRecord, name, newControls[name])
		end
		local hudActions = {
			{ "Aircraft", "Aircraft", "handleAircraftClicked" },
			{ "Scenery", "Scenery", "handleSceneryClicked" },
			{ "Radio", "Radio", "handleRadioClicked" },
			{ "Camera", "Camera", "handleCameraClicked" },
			{ "Recorder", "Recorder", "handleRecorderClicked" },
			{ "Settings", "Settings", "handleSettingsClicked" },
			{ "Pause", "Pause", "handlePauseClicked" }
		}
		for index, action in ipairs(hudActions) do
			local _, button, wired = createButton(hudRecord.actor, action[1] .. "Button", action[2],
				Vector2F(-830.0, -360.0 + (index - 1) * 78.0), Vector2F(210.0, 60.0), self.component, action[3], colours.control)
			if not button or not wired then error("HUD action " .. action[1] .. " was not wired") end
		end
		createText(hudRecord.actor, "ControlHint", "ESC  MENU     F1  HELP     F5  RESET     SPACE  PAUSE",
			Vector2F(0.0, 500.0), Vector2F(920.0, 38.0), colours.secondary, 3)
		initialiseDialog(hudRecord, false)

		if self.generateExtendedDialogs then
			local targetLayers = {
				dialog = dialogLayer,
				hud = hudLayer,
				overlay = overlayLayer,
				menu = menuLayer
			}
			for _, spec in ipairs(EXTENDED_DIALOG_SPECS) do
				createGenericDialog(targetLayers[spec.layer or "dialog"] or dialogLayer, spec)
			end
			newReferences.visualTxCanvas = newActors.visual_transmitter_dialog
			newReferences.raceCanvas = newActors.race_settings
		end
		-- Overlay layer: loading, splash, notifications, and fade surface.
		local loadingRecord = createDialogRecord(overlayLayer, "loading", "LoadingScreen", "UnityDialog", true, true)
		local loadingPanel = createPanel(loadingRecord.actor, "LoadingBackdrop", Vector2F(0.0, 0.0),
			Vector2F(self.referenceWidth, self.referenceHeight), colours.background, 1, true)
		createText(loadingPanel, "Title", "LOADING FLIGHT", Vector2F(0.0, -30.0), Vector2F(800.0, 80.0), colours.primary, 3)
		createText(loadingPanel, "Status", "Preparing aircraft and scenery...", Vector2F(0.0, 45.0), Vector2F(800.0, 48.0), colours.secondary, 3)
		initialiseDialog(loadingRecord, false)
		newReferences.loadingScreen = loadingRecord.script

		local splashRecord = createDialogRecord(overlayLayer, "splash", "SplashScreen", "UnityDialog", true, true)
		local splashPanel = createPanel(splashRecord.actor, "SplashBackdrop", Vector2F(0.0, 0.0),
			Vector2F(self.referenceWidth, self.referenceHeight), colours.background, 1, true)
		createText(splashPanel, "Title", "LIONCAT", Vector2F(0.0, -25.0), Vector2F(1000.0, 120.0), colours.primary, 3)
		createText(splashPanel, "Subtitle", "FLIGHT SIMULATION", Vector2F(0.0, 70.0), Vector2F(800.0, 54.0), colours.accent, 3)
		initialiseDialog(splashRecord, false)
		newReferences.splashScreen = splashRecord.script

		local infoActor, informationText = createText(overlayLayer, "InformationText", "",
			Vector2F(0.0, -420.0), Vector2F(1100.0, 64.0), colours.primary, 20)
		local informationRecord = infoActor and addScriptComponent(infoActor, "InformationText", false) or nil
		if informationRecord then
			informationRecord.key = "information_text"
			tryCall(informationRecord.script, { "bindControl", "BindControl" }, "text", informationText)
			initialiseDialog(informationRecord, false)
			newReferences.informationText = informationRecord.script
		else
			if infoActor then tryCall(infoActor, { "setEnabled", "SetEnabled" }, false) end
			newReferences.informationText = informationText
		end

		local fadeActor, fadeImage = createPanel(overlayLayer, "ScreenFade", Vector2F(0.0, 0.0),
			Vector2F(self.referenceWidth, self.referenceHeight), colours.background, 30, false)
		local fadeRecord = fadeActor and addScriptComponent(fadeActor, "UnityDialog", false) or nil
		if fadeRecord then
			fadeRecord.key = "screen_fade"
			prepareDialog(fadeRecord)
			tryCall(fadeRecord.script, { "setFadeTarget", "SetFadeTarget" }, fadeImage)
			tryCall(fadeRecord.script, { "setFadeOnShow", "SetFadeOnShow" }, true)
			tryCall(fadeRecord.script, { "setFadeOnHide", "SetFadeOnHide" }, true)
			initialiseDialog(fadeRecord, false)
			newReferences.screenFade = fadeRecord.script
		else
			if fadeActor then tryCall(fadeActor, { "setEnabled", "SetEnabled" }, false) end
			newReferences.screenFade = fadeImage
		end

		return {
			root = newRoot,
			dialogs = newDialogs,
			fullscreen = newFullscreenDialogs,
			actors = newActors,
			controls = newControls,
			references = newReferences,
			startRecord = startRecord
		}
	end)

	if not buildOk or not buildResult then
		self.lastError = "UI generation failed: " .. tostring(buildResult)
		if newRoot then destroyGenerated(newRoot) end
		self.generationWarnings = warnings
		for _, warning in ipairs(warnings) do print("UIManager warning: " .. warning) end
		print("UIManager:generate aborted: " .. self.lastError)
		return nil
	end

	-- Commit only after the complete required hierarchy exists.
	self.generatedRoot = buildResult.root
	self.dialogs = buildResult.dialogs
	self.fullscreenDialogs = buildResult.fullscreen
	self.generatedActors = buildResult.actors
	self.generatedControls = buildResult.controls
	for _, name in ipairs(GENERATED_REFERENCE_FIELDS) do self[name] = nil end
	for name, value in pairs(buildResult.references) do self[name] = value end
	self.startMenu = buildResult.references.startMenu
	self.systemSettings = buildResult.references.systemSettings
	self.scenerySelector = buildResult.references.scenerySelector
	self.modelHanger = buildResult.references.modelHanger
	self.modelEditor = buildResult.references.modelEditor
	self.hud = buildResult.references.hud
	self.loadingScreen = buildResult.references.loadingScreen
	self.splashScreen = buildResult.references.splashScreen
	self.informationText = buildResult.references.informationText
	self.screenFade = buildResult.references.screenFade
	tryCall(buildResult.root, { "setEnabled", "SetEnabled" }, true)

	local desiredVisibility = {}
	for name, dialog in pairs(self.dialogs) do
		local shouldShow = previousVisibility[name]
		if shouldShow == nil and not next(previousVisibility) then
			shouldShow = name == "start_menu" or name == "StartMenu"
		end
		if desiredVisibility[dialog] == nil then desiredVisibility[dialog] = shouldShow == true
		else desiredVisibility[dialog] = desiredVisibility[dialog] or shouldShow == true end
	end
	for dialog, shouldShow in pairs(desiredVisibility) do
		if shouldShow then tryCall(dialog, { "show", "Show" }, true)
		else tryCall(dialog, { "hide", "Hide" }, true) end
	end

	for _, oldRoot in ipairs(oldRoots) do
		if oldRoot ~= buildResult.root and not destroyGenerated(oldRoot) then
			warn("a previous generated UI root could not be fully destroyed")
		end
	end

	self.generationWarnings = warnings
	self.lastError = ""
	self:updateFullscreenDialogFlag()
	for _, warning in ipairs(warnings) do print("UIManager warning: " .. warning) end
	return buildResult.root
end

function UIManager:getGeneratedActor(name)
	return self.generatedActors[name]
end

function UIManager:showExclusiveDialog(name)
	local target = self.dialogs[name]
	if not target then return false end
	local visited = {}
	for dialogName, dialog in pairs(self.dialogs) do
		if self.fullscreenDialogs[dialogName] and dialog ~= target and not visited[dialog] then
			visited[dialog] = true
			tryCall(dialog, { "hide", "Hide" }, true)
		end
	end
	tryCall(target, { "show", "Show" }, true)
	self:updateFullscreenDialogFlag()
	return true
end

function UIManager:setHomeState(state)
	local visited = {}
	for _, dialog in pairs(self.dialogs) do
		if not visited[dialog] then
			visited[dialog] = true
			tryCall(dialog, { "hide", "Hide" }, true)
		end
	end
	if self.startMenu then tryCall(self.startMenu, { "show", "Show" }, true) end
	self:updateFullscreenDialogFlag()
	return true
end

function UIManager:showFlightUI()
	if self.startMenu then tryCall(self.startMenu, { "hide", "Hide" }, true) end
	local visited = {}
	for name, dialog in pairs(self.dialogs) do
		if self.fullscreenDialogs[name] and dialog ~= self.hud and not visited[dialog] then
			visited[dialog] = true
			tryCall(dialog, { "hide", "Hide" }, true)
		end
	end
	if self.hud then tryCall(self.hud, { "show", "Show" }, true) end
	self:updateFullscreenDialogFlag()
	return self.hud ~= nil
end

function UIManager:hideFlightUI()
	if not self.hud then return false end
	tryCall(self.hud, { "hide", "Hide" }, true)
	return true
end

function UIManager:handleAircraftClicked() return self:showExclusiveDialog("model_hanger") end
function UIManager:handleSceneryClicked() return self:showExclusiveDialog("scenery_selector") end
function UIManager:handleRadioClicked() return self:showExclusiveDialog("input_configuration") end
function UIManager:handleCameraClicked() return self:showExclusiveDialog("camera_settings") end
function UIManager:handleRecorderClicked() return self:showExclusiveDialog("flight_recorder") end
function UIManager:handleSettingsClicked() return self:showExclusiveDialog("settings") end
function UIManager:handlePauseClicked() return self:setHomeState(0) end

function UIManager:setInstrumentValue(name, value, colour)
	local control = self.generatedControls[name]
	if not control then return false end
	local updated = tryCall(control, { "setText", "SetText" }, tostring(value or ""))
	if colour then tryCall(control, { "setColour", "SetColour", "setColor", "SetColor" }, colour) end
	return updated
end

function UIManager:updateFlightTelemetry(values)
	if type(values) ~= "table" then return false end
	local delegated = tryCall(self.hud, { "setTelemetry", "SetTelemetry" }, values)
	local mappings = {
		altitude = "Altitude",
		airspeed = "Airspeed",
		heading = "Heading",
		battery = "Battery",
		signal = "Signal",
		mode = "Mode",
		aircraft = "Aircraft",
		clock = "Clock",
		fps = "FrameRate"
	}
	local updated = delegated == true
	for source, target in pairs(mappings) do
		if values[source] ~= nil and (not delegated or source == "mode" or source == "aircraft" or
			source == "clock" or source == "fps") then
			local displayValue = values[source]
			if source == "fps" then displayValue = string.format("FPS %.0f", tonumber(displayValue) or 0) end
			updated = self:setInstrumentValue(target, displayValue) or updated
		end
	end
	return updated
end

function UIManager:clickModelIcon(item)
	if not self.modelHanger or not dialogVisible(self.modelHanger) then return false end
	local called, result = tryCall(self.modelHanger, { "clickModelIcon", "ClickModelIcon" }, item)
	return called and result ~= false
end

function UIManager:clickComponentItem(item)
	if not item then return false end
	if self.componentChange and dialogVisible(self.componentChange) then
		local called, result = tryCall(self.componentChange, { "clickComponent", "ClickComponent" }, item)
		return called and result ~= false
	end
	if not self.componentSelector or not dialogVisible(self.componentSelector) then return false end

	local referenceId = readMember(item,
		{ "getComponentId", "GetComponentId", "getComponentReferenceId", "GetComponentReferenceId" },
		{ "componentId", "componentReferenceId", "id" }, nil)
	if referenceId ~= nil and self.applicationController then
		local written = tryCall(self.applicationController,
			{ "setSelectedComponentRefId", "SetSelectedComponentRefId" }, referenceId)
		if not written then pcall(function() self.applicationController.selectedComponentRefId = referenceId end) end
	end
	tryCall(self.componentSelector, { "hide", "Hide", "onHidePanel", "OnHidePanel" }, true)
	if self.componentConfig then
		tryCall(self.componentConfig, { "setSelectedComponent", "SetSelectedComponent" }, item)
		tryCall(self.componentConfig, { "show", "Show" }, true)
	end
	tryCall(self.applicationController, { "selectComponent", "SelectComponent" }, item)
	return self.componentConfig ~= nil
end

function UIManager:clickChangeComponent()
	if self.componentConfig then tryCall(self.componentConfig, { "hide", "Hide" }, true) end
	if not self.componentChange then return false end
	tryCall(self.componentChange, { "show", "Show" }, true)
	return true
end

function UIManager:start()
	self.information = {}
	if self.autoGenerate and not self.generatedRoot then self:generate() end
	if self.fadeOnStart then self:screenFadeOut(self.fadeSpeed) end
end

function UIManager:update()
	if self.messageExpiresAt then
		if currentTime() >= self.messageExpiresAt then
			self.messageExpiresAt = nil
			self:hideMessageBox()
			self:checkRaceInfoList()
		end
	end
	if self.fadeSequence then
		local phase = self.fadeSequence.phase
		if phase == "fade_in" then
			local showing = readMember(self.screenFade, { "getIsShowing", "GetIsShowing" }, { "isShowing" }, false)
			if not showing then
				self:showLoadingScreen()
				local callback = self.fadeSequence.callback
				if callback then
					local ok, errorMessage = pcall(callback, 0)
					if not ok then self.lastError = "Fade callback failed: " .. tostring(errorMessage) end
				end
				self.fadeSequence.phase = "fade_out"
				self:screenFadeOut(self.fadeSequence.speed)
			end
		elseif phase == "fade_out" then
			local hiding = readMember(self.screenFade, { "getIsHiding", "GetIsHiding" }, { "isHiding" }, false)
			if not hiding then self.fadeSequence = nil end
		end
	end
	self:updateFullscreenDialogFlag()
end

-- Typed object properties are not exposed by the current Lua Properties binding,
-- so scene/bootstrap code supplies runtime dependencies with these methods.
function UIManager:setApplicationController(value) self.applicationController = value end
function UIManager:setUIStateManager(value) self.uiStateManager = value end
function UIManager:setInformationText(value) self.informationText = value end
function UIManager:setLoadingScreen(value) self.loadingScreen = value end
function UIManager:setSplashScreen(value) self.splashScreen = value end

function UIManager:registerDialog(name, dialog, fullscreen)
	if not name or name == "" or not dialog then return false end
	self.dialogs[name] = dialog
	self.fullscreenDialogs[name] = fullscreen == true
	self:updateFullscreenDialogFlag()
	return true
end

function UIManager:unregisterDialog(name)
	self.dialogs[name] = nil
	self.fullscreenDialogs[name] = nil
	self:updateFullscreenDialogFlag()
end

function UIManager:getDialog(name) return self.dialogs[name] end
function UIManager:getHtmlDialog(name) return self.dialogs[name] end

function UIManager:moveInHierarchy(gameObject, delta)
	if not gameObject then return false end
	delta = math.floor(tonumber(delta) or 0)
	local _, transform = tryCall(gameObject, { "getTransform", "GetTransform" })
	if not transform then
		local _, candidate = tryCall(gameObject, { "getComponent", "GetComponent" }, "LayoutTransform")
		transform = candidate
	end
	local called, index = tryCall(transform or gameObject,
		{ "getSiblingIndex", "GetSiblingIndex", "getZOrder", "GetZOrder" })
	if not called then return false end
	local moved = tryCall(transform or gameObject,
		{ "setSiblingIndex", "SetSiblingIndex", "setZOrder", "SetZOrder" },
		math.max(0, math.floor(tonumber(index) or 0) + delta))
	return moved
end

function UIManager:createDialog(dialogName, url, position, size, anchor)
	dialogName = tostring(dialogName or "")
	url = tostring(url or "")
	if dialogName == "" or url == "" then return nil end
	if self.dialogs[dialogName] then return self.dialogs[dialogName] end
	if not self.generatedRoot and not self:generate() then return nil end

	local parent = self.generatedActors.dialog_layer or self.generatedRoot
	local applicationManager = self.applicationController
	if not applicationManager then
		local ok, value = pcall(function() return IApplicationManager.instance() end)
		if ok then applicationManager = value end
	end
	local _, gameManager = tryCall(applicationManager, { "getGameManager", "GetGameManager" })
	local created, actor = tryCall(gameManager, { "createActor", "CreateActor" })
	if not created or not actor then return nil end

	local function fail()
		tryCall(gameManager, { "destroyActor", "DestroyActor" }, actor, true)
		return nil
	end

	tryCall(actor, { "setName", "SetName" }, "DynamicDialog_" .. dialogName)
	if not tryCall(parent, { "addChild", "AddChild" }, actor) then return fail() end
	local _, transform = tryCall(actor, { "addComponent", "AddComponent" }, "LayoutTransform")
	if not transform then return fail() end
	local x = tonumber(readMember(position, {}, { "x" }, 0.0)) or 0.0
	local y = tonumber(readMember(position, {}, { "y" }, 0.0)) or 0.0
	local width = math.max(240.0, tonumber(readMember(size, {}, { "x" }, 720.0)) or 720.0)
	local height = math.max(160.0, tonumber(readMember(size, {}, { "y" }, 480.0)) or 480.0)
	tryCall(transform, { "setPosition", "SetPosition" }, Vector2F(x, y))
	tryCall(transform, { "setSize", "SetSize" }, Vector2F(width, height))
	tryCall(transform, { "setZOrder", "SetZOrder" }, 10)

	local _, image = tryCall(actor, { "addComponent", "AddComponent" }, "Image")
	if image then tryCall(image, { "setColour", "SetColour", "setColor", "SetColor" }, makeColour(0.025, 0.055, 0.090, 0.98)) end
	local _, text = tryCall(actor, { "addComponent", "AddComponent" }, "Text")
	if text then
		tryCall(text, { "setText", "SetText" }, dialogName)
		tryCall(text, { "setColour", "SetColour", "setColor", "SetColor" }, makeColour(0.93, 0.97, 1.0, 1.0))
	end

	local typeOk, userComponentTypeId = pcall(function() return UserComponent.typeInfo() end)
	local component = nil
	if typeOk then
		local _, value = tryCall(actor, { "addComponentById", "AddComponentById" }, userComponentTypeId)
		component = value
	end
	if not component then
		local _, fallback = tryCall(actor, { "addComponent", "AddComponent" }, "UserComponent")
		component = fallback
	end
	if not component or not tryCall(component, { "setClassName", "SetClassName" }, "FlightDialog") then return fail() end
	local scriptFound, script = tryCall(component, { "getScriptClass", "GetScriptClass" })
	script = scriptFound and script or component
	tryCall(script, { "setApplicationController", "SetApplicationController" }, applicationManager)
	tryCall(script, { "setUIManager", "SetUIManager" }, self)
	tryCall(script, { "setUIStateManager", "SetUIStateManager" }, self.uiStateManager)
	tryCall(script, { "setDialogName", "SetDialogName" }, dialogName)
	tryCall(script, { "setDialogReference", "SetDialogReference" }, dialogName)
	tryCall(script, { "setValue", "SetValue" }, "url", url)
	tryCall(script, { "setValue", "SetValue" }, "anchor", tostring(anchor or "center"))
	tryCall(script, { "start", "Start", "initialise", "Initialise" })
	tryCall(script, { "hide", "Hide" }, true)
	self.generatedActors[dialogName] = actor
	self:registerDialog(dialogName, script, false)
	self.zorderDirty = true
	return script
end

function UIManager:showDialog(name)
	local dialog = self.dialogs[name]
	if not dialog then return false end
	call(dialog, "show", "Show")
	self:updateFullscreenDialogFlag()
	return true
end

function UIManager:hideDialog(name)
	local dialog = self.dialogs[name]
	if not dialog then return false end
	call(dialog, "hide", "Hide")
	self:updateFullscreenDialogFlag()
	return true
end

function UIManager:isDialogVisible(name)
	return dialogVisible(self.dialogs[name])
end

function UIManager:hideAllDialogs()
	local visited = {}
	for _, dialog in pairs(self.dialogs) do
		if dialog and not visited[dialog] then visited[dialog] = true; call(dialog, "hide", "Hide") end
	end
	self:updateFullscreenDialogFlag()
end

function UIManager:destroyAllDialogs()
	if self.generatedRoot then
		local applicationManager = self.applicationController
		if not applicationManager then
			local ok, value = pcall(function() return IApplicationManager.instance() end)
			if ok then applicationManager = value end
		end
		local _, gameManager = tryCall(applicationManager, { "getGameManager", "GetGameManager" })
		local destroyed = tryCall(gameManager, { "destroyActor", "DestroyActor" }, self.generatedRoot, true)
		if not destroyed then tryCall(self.generatedRoot, { "setEnabled", "SetEnabled" }, false) end
	else
		local visited = {}
		for _, dialog in pairs(self.dialogs) do
			if dialog and not visited[dialog] then
				visited[dialog] = true
				if call(dialog, "destroy", "Destroy") == nil then
					local actor = call(dialog, "getActor", "GetActor")
					if actor then tryCall(actor, { "setEnabled", "SetEnabled" }, false) end
				end
			end
		end
	end
	self.dialogs = {}
	self.fullscreenDialogs = {}
	self.generatedRoot = nil
	self.generatedActors = {}
	self.generatedControls = {}
	for _, name in ipairs(GENERATED_REFERENCE_FIELDS) do self[name] = nil end
	self:updateFullscreenDialogFlag()
end

function UIManager:addFullscreenChangedListener(callback)
	if callback then table.insert(self.fullscreenChangedCallbacks, callback) end
end

function UIManager:setFullscreenDialogDisplayed(value)
	value = value == true
	if self.isFullscreenDialogDisplayed == value then return end
	self.isFullscreenDialogDisplayed = value
	for _, callback in ipairs(self.fullscreenChangedCallbacks) do
		local ok, errorMessage = pcall(callback, value)
		if not ok then self.lastError = "Fullscreen listener failed: " .. tostring(errorMessage) end
	end
	call(self.applicationController, "setPauseMenuActive", "SetPauseMenuActive", value)
end

function UIManager:updateFullscreenDialogFlag()
	local displayed = false
	for name, isFullscreen in pairs(self.fullscreenDialogs) do
		if isFullscreen and dialogVisible(self.dialogs[name]) then
			displayed = true
			break
		end
	end
	self:setFullscreenDialogDisplayed(displayed)
	return displayed
end

function UIManager:updateCachedData() self:updateFullscreenDialogFlag() end
function UIManager:windowResize() end
function UIManager:paintDialog(name) end

function UIManager:updateResources()
	local visited = {}
	for _, dialog in pairs(self.dialogs) do
		if dialog and not visited[dialog] then
			visited[dialog] = true
			tryCall(dialog, { "updateResources", "UpdateResources", "refresh", "Refresh" })
		end
	end
	return true
end

function UIManager:setupGui()
	return self:generate()
end

function UIManager:setupDropdowns()
	local visited = {}
	for _, dialog in pairs(self.dialogs) do
		if dialog and not visited[dialog] then
			visited[dialog] = true
			tryCall(dialog, { "populateAllRegionDropDowns", "PopulateAllRegionDropDowns",
				"setupDropdowns", "SetupDropdowns" })
		end
	end
	return true
end

function UIManager:clickFullscreenMenuButton(action)
	if action == "gotomainmenu" then
		call(self.applicationController, "clickGotoMainMenu", "ClickGotoMainMenu")
		return
	end
	if action == "gototoscene" or action == "gotoworkshop" then self:showLoadingScreen() end
	call(self.applicationController, "sendPluginEvent", "SendPluginEvent", "fullscreenMenuButton", action)
end

function UIManager:handleFlightPanelAction(action, toggledState)
	if action == "visual_tx" then
		call(self.uiStateManager, "setVisualTxVisible", "SetVisualTxVisbile", toggledState)
	elseif toggledState then
		self:showDialog(action)
	else
		self:hideDialog(action)
	end
	local eventName = toggledState and "showDialog" or "hideDialog"
	call(self.applicationController, "sendPluginEvent", "SendPluginEvent", eventName, action)
end

function UIManager:showLoadingScreen() call(self.loadingScreen, "show", "Show") end
function UIManager:hideLoadingScreen() call(self.loadingScreen, "hide", "Hide") end

function UIManager:clearInformation() self.information = {} end

function UIManager:setInformationDisplay(info, isError)
	if not self.informationText then return false end
	call(self.informationText, "setText", "SetText", info or "")
	call(self.informationText, "setError", "SetError", isError == true)
	local actor = call(self.informationText, "getActor", "GetActor")
	if actor then tryCall(actor, { "setEnabled", "SetEnabled" }, info ~= nil and info ~= "") end
	return true
end

function UIManager:showMessageBox(info, time)
	self:setInformationDisplay(info, false)
	if time and time > 0 then
		self.messageExpiresAt = currentTime() + time
	else
		self.messageExpiresAt = nil
	end
end

function UIManager:hideMessageBox()
	self.messageExpiresAt = nil
	self:setInformationDisplay("", false)
end

function UIManager:showInformation(info, time)
	if not info or info == "" then return end
	if self.messageExpiresAt then
		table.insert(self.information, { text = info, time = time or 2.0, error = false })
	else
		self:showMessageBox(info, time or 2.0)
	end
end

function UIManager:showError(info, time)
	if self.messageExpiresAt then
		table.insert(self.information, { text = info, time = time or 2.0, error = true })
	else
		self:setInformationDisplay(info, true)
		self.messageExpiresAt = currentTime() + (time or 2.0)
	end
end

function UIManager:checkRaceInfoList()
	local entry = table.remove(self.information)
	if not entry then return end
	if entry.error then self:showError(entry.text, entry.time)
	else self:showInformation(entry.text, entry.time) end
end

function UIManager:screenFadeOut(speed)
	call(self.screenFade, "show", "Show", true)
	call(self.screenFade, "fadeOut", "FadeOut", speed or self.fadeSpeed)
end

function UIManager:screenFadeIn(speed, loadScene, scene)
	call(self.screenFade, "fadeIn", "FadeIn", speed or self.fadeSpeed)
	if loadScene and scene then
		self:showLoadingScreen()
		local app = self.applicationController
		if not app then
			local ok, value = pcall(function() return IApplicationManager.instance() end)
			if ok then app = value end
		end
		local _, sceneManager = tryCall(app, { "getSceneManager", "GetSceneManager" })
		local loaded = tryCall(sceneManager, { "loadScene", "LoadScene" }, scene)
		if not loaded then self.lastError = "Could not load scene " .. tostring(scene) end
	end
end

function UIManager:screenFadeInAndOut(speed)
	self:screenFadeOut(speed or self.fadeSpeed)
	return true
end

function UIManager:screenFadeOutAndIn(speed, callback)
	self.fadeSequence = {
		phase = "fade_in",
		speed = tonumber(speed) or self.fadeSpeed,
		callback = callback
	}
	self:screenFadeIn(self.fadeSequence.speed)
	return true
end

function UIManager.fadeCallback(callback, value)
	if type(callback) ~= "function" then return false end
	local ok, result = pcall(callback, value or 0)
	return ok, result
end

function UIManager:setScreenFade(value) self.screenFade = value end
function UIManager:modelReady(value) self:hideLoadingScreen() end
function UIManager:uiReady(value) self:updateCachedData() end
function UIManager:sceneReady(scene, mode) self:updateCachedData() end
function UIManager:sceneWasLoaded(scene, mode) self:updateCachedData() end
function UIManager:sceneWasUnloaded(scene) self:hideAllDialogs() end
function UIManager:onScriptsReloaded() self:updateCachedData() end
function UIManager:onModelLoaded()
	tryCall(self.componentSelector, { "onModelLoaded", "OnModelLoaded" })
	call(self.applicationController, "onModelLoaded", "OnModelLoaded")
end
function UIManager:onModelComponentChanged()
	tryCall(self.componentSelector, { "onModelComponentChanged", "OnModelComponentChanged" })
	call(self.applicationController, "onModelComponentChanged", "OnModelComponentChanged")
end

-- Compatibility aliases for bindings ported directly from the C# UI events.
UIManager.Start = UIManager.start
UIManager.Generate = UIManager.generate
UIManager.GetDialog = UIManager.getDialog
UIManager.GetHtmlDialog = UIManager.getHtmlDialog
UIManager.CreateDialog = UIManager.createDialog
UIManager.MoveInHierarchy = UIManager.moveInHierarchy
UIManager.GetGeneratedActor = UIManager.getGeneratedActor
UIManager.ShowDialog = UIManager.showDialog
UIManager.HideDialog = UIManager.hideDialog
UIManager.IsDialogVisible = UIManager.isDialogVisible
UIManager.HideAllDialogs = UIManager.hideAllDialogs
UIManager.DestroyAllDialogs = UIManager.destroyAllDialogs
UIManager.ShowExclusiveDialog = UIManager.showExclusiveDialog
UIManager.SetHomeState = UIManager.setHomeState
UIManager.ShowFlightUI = UIManager.showFlightUI
UIManager.HideFlightUI = UIManager.hideFlightUI
UIManager.UpdateFullscreenDialogFlag = UIManager.updateFullscreenDialogFlag
UIManager.UpdateCachedData = UIManager.updateCachedData
UIManager.UpdateResources = UIManager.updateResources
UIManager.WindowResize = UIManager.windowResize
UIManager.PaintDialog = UIManager.paintDialog
UIManager.SetupGui = UIManager.setupGui
UIManager.SetupDropdowns = UIManager.setupDropdowns
UIManager.ClickFullscreenMenuButton = UIManager.clickFullscreenMenuButton
UIManager.HandleFlightPanelAction = UIManager.handleFlightPanelAction
UIManager.HandleAircraftClicked = UIManager.handleAircraftClicked
UIManager.HandleSceneryClicked = UIManager.handleSceneryClicked
UIManager.HandleRadioClicked = UIManager.handleRadioClicked
UIManager.HandleCameraClicked = UIManager.handleCameraClicked
UIManager.HandleRecorderClicked = UIManager.handleRecorderClicked
UIManager.HandleSettingsClicked = UIManager.handleSettingsClicked
UIManager.HandlePauseClicked = UIManager.handlePauseClicked
UIManager.ShowLoadingScreen = UIManager.showLoadingScreen
UIManager.HideLoadingScreen = UIManager.hideLoadingScreen
UIManager.ClearInformation = UIManager.clearInformation
UIManager.ShowMessageBox = UIManager.showMessageBox
UIManager.HideMessageBox = UIManager.hideMessageBox
UIManager.ShowInformation = UIManager.showInformation
UIManager.ShowError = UIManager.showError
UIManager.CheckRaceInfoList = UIManager.checkRaceInfoList
UIManager.ScreenFadeOut = UIManager.screenFadeOut
UIManager.ScreenFadeIn = UIManager.screenFadeIn
UIManager.SetInstrumentValue = UIManager.setInstrumentValue
UIManager.UpdateFlightTelemetry = UIManager.updateFlightTelemetry
UIManager.ClickModelIcon = UIManager.clickModelIcon
UIManager.ClickComponentItem = UIManager.clickComponentItem
UIManager.ClickChangeComponent = UIManager.clickChangeComponent
UIManager.ModelReady = UIManager.modelReady
UIManager.UIReady = UIManager.uiReady
UIManager.SceneReady = UIManager.sceneReady
UIManager.SceneWasLoaded = UIManager.sceneWasLoaded
UIManager.SceneWasUnloaded = UIManager.sceneWasUnloaded
UIManager.OnScriptsReloaded = UIManager.onScriptsReloaded
UIManager.OnModelLoaded = UIManager.onModelLoaded
UIManager.OnModelComponentChanged = UIManager.onModelComponentChanged
UIManager.ScreenFadeInAndOut = UIManager.screenFadeInAndOut
UIManager.ScreenFadeOutAndIn = UIManager.screenFadeOutAndIn
UIManager.FadeCallback = UIManager.fadeCallback
