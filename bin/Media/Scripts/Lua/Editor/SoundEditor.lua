class 'SoundEditor' (BaseEditor)

SoundEditorControlKind =
{
	Value = "value",
	Toggle = "toggle",
	Dropdown = "dropdown",
	Text = "text",
}

SoundEditorOptions =
{
	events =
	{
		"event:/SFX/Test",
		"event:/SFX/Explosion",
		"event:/SFX/Footstep",
		"event:/Music/Menu",
		"event:/Ambience/City",
	},
	parameters =
	{
		"Intensity",
		"Speed",
		"Health",
		"Surface",
		"Distance",
	},
	surfaces =
	{
		"Concrete",
		"Metal",
		"Wood",
		"Grass",
		"Water",
	},
	musicStates =
	{
		"Menu",
		"Explore",
		"Combat",
		"Victory",
		"Defeat",
	},
	buses =
	{
		"bus:/Master",
		"bus:/SFX",
		"bus:/Music",
		"bus:/Ambience",
		"bus:/UI",
	},
	vcas =
	{
		"vca:/Master",
		"vca:/SFX",
		"vca:/Music",
		"vca:/Dialog",
	},
	banks =
	{
		"Master.bank",
		"Master.strings.bank",
		"SFX.bank",
		"Music.bank",
		"Ambience.bank",
	},
	snapshots =
	{
		"snapshot:/Gameplay/Default",
		"snapshot:/Gameplay/Pause",
		"snapshot:/Gameplay/SlowMotion",
		"snapshot:/Mix/Underwater",
		"snapshot:/Mix/LowHealth",
	},
}

SoundEditorDefaults =
{
	event = 0,
	eventPath = "event:/SFX/Test",
	timelinePosition = 0.0,
	timelineSpeed = 1.0,
	volume = 1.0,
	pitch = 1.0,
	pan = 0.0,
	priority = 128.0,
	loopRegion = false,
	oneShot = false,
	voiceStealing = 0,
	parameter = 0,
	parameterValue = 0.0,
	parameterSeekSpeed = 1.0,
	surface = 0,
	musicState = 0,
	distanceParameter = 0.0,
	intensityParameter = 0.0,
	bus = 0,
	vca = 0,
	busVolume = 1.0,
	busMute = false,
	busSolo = false,
	vcaVolume = 1.0,
	reverbSend = 0.0,
	delaySend = 0.0,
	sideChain = 0.0,
	mode = 0,
	spatializer = 1,
	minDistance = 1.0,
	maxDistance = 100.0,
	doppler = 1.0,
	spread = 0.0,
	occlusion = 0.0,
	directivity = 0.0,
	rolloff = 0,
	effectSlot = 0,
	effectType = 0,
	effectWet = 1.0,
	effectBypass = false,
	reverbAmount = 0.0,
	reverbDecay = 1.5,
	reverbRoomSize = 0.5,
	lowEQ = 0.0,
	midEQ = 0.0,
	highEQ = 0.0,
	lowpass = 22000.0,
	highpass = 20.0,
	compressorThreshold = -18.0,
	compressorRatio = 4.0,
	limiterCeiling = -1.0,
	bank = 0,
	bankPath = "media/audio/Desktop/Master.bank",
	liveUpdate = true,
	sampleData = true,
	snapshot = 0,
	snapshotIntensity = 1.0,
	snapshotFadeTime = 1.0,
	dspCPU = 0.0,
	audioMemoryMB = 0.0,
	activeVoices = 0.0,
	virtualVoices = 0.0,
	rawCommand = "event:/SFX/Test play",
}

local SoundEditorUnpack = unpack or table.unpack;
if SoundEditorUnpack == nil then
	SoundEditorUnpack = function(values)
		return nil;
	end
end

function SoundEditor:__init(window)
	print("SoundEditor constructor called");

	self.window = window;
	self.editorWindow = nil;
	self.tabBar = nil;

	self._settings = {};
	for key, value in pairs(SoundEditorDefaults) do
		self._settings[key] = value;
	end
	self._actions = {};
	self._controlBindings = {};
	self._buttonActions = {};
	self._loadedBanks = {};
	self._effects = {};
	self._parameters = {};
	self._soundCache = {};
	self._selectedEventPath = "event:/SFX/Test";
	self._selectedBank = "Master.bank";
	self._selectedSnapshot = "snapshot:/Gameplay/Default";
	self._currentSound = nil;
	self._isPlaying = false;
	self._isPaused = false;
	self._currentStatus = "Ready.";

	self.testText = nil;
	self.statusText = nil;
end

function SoundEditor:__finalize()
	print("SoundEditor __finalize called");

	self.window = nil;
	self.editorWindow = nil;
	self.tabBar = nil;

	self._settings = nil;
	self._actions = nil;
	self._controlBindings = nil;
	self._buttonActions = nil;
	self._loadedBanks = nil;
	self._effects = nil;
	self._parameters = nil;
	self._soundCache = nil;
	self._currentSound = nil;

	self.testText = nil;
	self.statusText = nil;
end

function SoundEditor:_addHeader(ui, typeInfo, parent, label)
	local header = ui:addElement(typeInfo);
	header:setLabel(label);
	parent:addChild(header);
	return header;
end

function SoundEditor:_addText(ui, typeInfo, parent, text, sameLine)
	local control = ui:addElement(typeInfo);
	control:setText(text);
	if sameLine ~= nil then
		control:setSameLine(sameLine);
	end
	parent:addChild(control);
	return control;
end

function SoundEditor:_addButton(ui, typeInfo, parent, label)
	local button = ui:addElement(typeInfo);
	button:setLabel(label);
	parent:addChild(button);
	return button;
end

function SoundEditor:_addToggle(ui, typeInfo, parent, label, defaultValue)
	local toggle = ui:addElement(typeInfo);
	toggle:setLabel(label);
	if defaultValue ~= nil and toggle.setValue then
		toggle:setValue(defaultValue);
	end
	parent:addChild(toggle);
	return toggle;
end

function SoundEditor:_addSlider(ui, typeInfo, parent, label, minValue, maxValue, defaultValue)
	local slider = ui:addElement(typeInfo);
	slider:setLabel(label);
	slider:setMinValue(minValue);
	slider:setMaxValue(maxValue);
	slider:setValue(defaultValue);
	parent:addChild(slider);
	return slider;
end

function SoundEditor:_addDropdown(ui, typeInfo, parent, label, options, selectedOption)
	local dropdown = ui:addElement(typeInfo);
	dropdown:setLabel(label);
	for _, option in ipairs(options) do
		dropdown:addOption(option);
	end
	if selectedOption ~= nil then
		dropdown:setSelectedOption(selectedOption);
	end
	parent:addChild(dropdown);
	return dropdown;
end

function SoundEditor:_bindControl(control, key, kind, options)
	if control == nil then
		return control;
	end

	self._controlBindings = self._controlBindings or {};
	self._controlBindings[control] =
	{
		key = key,
		kind = kind or SoundEditorControlKind.Value,
		options = options,
	};

	if self.window then
		self.window:setHandleEvents(control, true);
	end

	return control;
end

function SoundEditor:_bindButton(control, actionName)
	if control == nil then
		return control;
	end

	self._buttonActions = self._buttonActions or {};
	self._buttonActions[control] = actionName;

	if self.window then
		self.window:setHandleEvents(control, true);
	end

	return control;
end

function SoundEditor:_safeArg(args, index, defaultValue)
	if args == nil then
		return defaultValue;
	end

	local ok, value = pcall(function()
		return args:at(index);
	end);

	if ok and value ~= nil then
		return value;
	end

	return defaultValue;
end

function SoundEditor:_getTextEntryValue(control)
	if control and control.getText then
		return control:getText();
	end

	if control and control.getValue then
		return control:getValue();
	end

	return "";
end

function SoundEditor:_getSelectedOptionValue(control)
	if control and control.getSelectedOption then
		return control:getSelectedOption();
	end

	return 0;
end

function SoundEditor:_getControlValue(control, kind)
	if control == nil then
		return nil;
	end

	if kind == SoundEditorControlKind.Dropdown then
		return self:_getSelectedOptionValue(control);
	elseif kind == SoundEditorControlKind.Text then
		return self:_getTextEntryValue(control);
	elseif kind == SoundEditorControlKind.Toggle then
		return control:getValue();
	end

	return control:getValue();
end

function SoundEditor:_setControlValue(control, kind, value)
	if control == nil then
		return;
	end

	if kind == SoundEditorControlKind.Dropdown then
		control:setSelectedOption(value or 0);
	elseif kind == SoundEditorControlKind.Text then
		control:setText(value or "");
	elseif kind == SoundEditorControlKind.Toggle then
		control:setValue(value == true);
	else
		control:setValue(value or 0.0);
	end
end

function SoundEditor:_getOptionLabel(options, index, fallback)
	if options == nil then
		return fallback;
	end

	local option = options[(index or 0) + 1];
	if option ~= nil then
		return option;
	end

	return fallback;
end

function SoundEditor:_setResult(results, key, value)
	if results then
		results[key] = value;
	end
end

function SoundEditor:_setStatus(text, results)
	self._currentStatus = text or "";
	if self.statusText then
		self.statusText:setText(self._currentStatus);
	end
	self:_setResult(results, "status", self._currentStatus);
	print("SoundEditor status:", self._currentStatus);
end

function SoundEditor:_recordAction(actionName, results)
	self._actions = self._actions or {};
	table.insert(self._actions, actionName);
	self._settings = self._settings or {};
	self._settings.lastAction = actionName;
	self:_setResult(results, "action", actionName);
	self:_setResult(results, "settings", self._settings);
	print("SoundEditor action:", actionName);
end

function SoundEditor:_safeCall(target, methodName, ...)
	if target == nil or methodName == nil then
		return false, nil;
	end

	local okMethod, method = pcall(function()
		return target[methodName];
	end);

	if not okMethod or method == nil then
		return false, nil;
	end

	local args = { ... };
	return pcall(function()
		return method(target, SoundEditorUnpack(args));
	end);
end

function SoundEditor:_getApplicationManager()
	local ok, applicationManager = pcall(function()
		return IApplicationManager.instance();
	end);

	if ok then
		return applicationManager;
	end

	return nil;
end

function SoundEditor:_getSoundManager()
	local applicationManager = self:_getApplicationManager();
	if applicationManager == nil then
		return nil;
	end

	local ok, soundManager = pcall(function()
		return applicationManager:getSoundManager();
	end);

	if ok then
		return soundManager;
	end

	return nil;
end

function SoundEditor:_getResourceDatabase()
	local applicationManager = self:_getApplicationManager();
	if applicationManager == nil then
		return nil;
	end

	local ok, resourceDatabase = pcall(function()
		return applicationManager:getResourceDatabase();
	end);

	if ok then
		return resourceDatabase;
	end

	return nil;
end

function SoundEditor:_syncFromControls()
	self._settings = self._settings or {};
	self._controlBindings = self._controlBindings or {};

	for control, info in pairs(self._controlBindings) do
		if control ~= nil and info ~= nil then
			self._settings[info.key] = self:_getControlValue(control, info.kind);
		end
	end

	self._selectedEventPath = self:_getCurrentEventPath();
	self._selectedBank = self:_getCurrentBankName();
	self._selectedSnapshot = self:_getCurrentSnapshotPath();
end

function SoundEditor:_syncToControls()
	if self._controlBindings == nil or self._settings == nil then
		return;
	end

	for control, info in pairs(self._controlBindings) do
		if control ~= nil and info ~= nil and self._settings[info.key] ~= nil then
			self:_setControlValue(control, info.kind, self._settings[info.key]);
		end
	end
end

function SoundEditor:_getCurrentEventPath()
	local path = self._settings and self._settings.eventPath or nil;
	if path ~= nil and path ~= "" then
		return path;
	end

	return self:_getOptionLabel(SoundEditorOptions.events, self._settings and self._settings.event or 0, self._selectedEventPath);
end

function SoundEditor:_getCurrentBankName()
	return self:_getOptionLabel(SoundEditorOptions.banks, self._settings and self._settings.bank or 0, self._selectedBank);
end

function SoundEditor:_getCurrentBankPath()
	local path = self._settings and self._settings.bankPath or nil;
	if path ~= nil and path ~= "" then
		return path;
	end

	return self:_getCurrentBankName();
end

function SoundEditor:_getCurrentSnapshotPath()
	return self:_getOptionLabel(SoundEditorOptions.snapshots, self._settings and self._settings.snapshot or 0, self._selectedSnapshot);
end

function SoundEditor:_getCurrentParameterName()
	return self:_getOptionLabel(SoundEditorOptions.parameters, self._settings and self._settings.parameter or 0, "Intensity");
end

function SoundEditor:_loadSound(path)
	if path == nil or path == "" then
		return nil, "No sound path selected.";
	end

	self._soundCache = self._soundCache or {};
	if self._soundCache[path] ~= nil then
		return self._soundCache[path], "Loaded cached sound: " .. tostring(path);
	end

	local soundManager = self:_getSoundManager();
	if soundManager ~= nil then
		local ok, sound = self:_safeCall(soundManager, "addSound2", path, self._settings.loopRegion == true);
		if ok and sound ~= nil then
			self._soundCache[path] = sound;
			return sound, "Loaded sound through SoundManager: " .. tostring(path);
		end

		ok, sound = self:_safeCall(soundManager, "addSound3", path);
		if ok and sound ~= nil then
			self._soundCache[path] = sound;
			return sound, "Loaded sound through SoundManager: " .. tostring(path);
		end
	end

	local resourceDatabase = self:_getResourceDatabase();
	if resourceDatabase ~= nil then
		local ok, resource = self:_safeCall(resourceDatabase, "loadResource", path);
		if ok and resource ~= nil then
			self._soundCache[path] = resource;
			return resource, "Loaded resource: " .. tostring(path);
		end
	end

	return nil, "Could not load sound resource: " .. tostring(path);
end

function SoundEditor:_ensureCurrentSound(results)
	self:_syncFromControls();

	if self._currentSound ~= nil then
		return self._currentSound;
	end

	local sound, message = self:_loadSound(self:_getCurrentEventPath());
	if sound ~= nil then
		self._currentSound = sound;
	end

	self:_setStatus(message, results);
	return sound;
end

function SoundEditor:_applyCurrentSoundSettings(sound)
	if sound == nil then
		return;
	end

	local volume = self._settings.volume or 1.0;
	self:_safeCall(sound, "setVolume", volume);

	local loop = self._settings.loopRegion == true and self._settings.oneShot ~= true;
	self:_safeCall(sound, "setLoop", loop);
end

function SoundEditor:_applyManagerSettings()
	local soundManager = self:_getSoundManager();
	if soundManager == nil then
		return false;
	end

	if self._settings.bus == 0 and self._settings.busVolume ~= nil then
		self:_safeCall(soundManager, "setVolume", self._settings.busVolume);
	end

	if self._settings.busMute ~= nil then
		self:_safeCall(soundManager, "setMute", self._settings.busMute == true);
	end

	return true;
end

function SoundEditor:_applyLiveSetting(key, value, results)
	if key == "event" then
		self._settings.eventPath = self:_getOptionLabel(SoundEditorOptions.events, value, self._settings.eventPath);
		if self.eventPathEntry then
			self.eventPathEntry:setText(self._settings.eventPath);
		end
		self._currentSound = nil;
		self:_setStatus("Selected event: " .. tostring(self._settings.eventPath), results);
	elseif key == "eventPath" then
		self._selectedEventPath = value;
		self._currentSound = nil;
		self:_setStatus("Event path set: " .. tostring(value), results);
	elseif key == "volume" or key == "loopRegion" or key == "oneShot" then
		if self._currentSound ~= nil then
			self:_applyCurrentSoundSettings(self._currentSound);
		end
	elseif key == "busVolume" or key == "busMute" then
		if self:_applyManagerSettings() then
			self:_setStatus("Applied mixer setting: " .. tostring(key), results);
		end
	elseif key == "bank" then
		self._selectedBank = self:_getCurrentBankName();
	elseif key == "snapshot" then
		self._selectedSnapshot = self:_getCurrentSnapshotPath();
	end

	if self._settings.liveUpdate == true then
		self:_applyManagerSettings();
	end
end

function SoundEditor:_registerEventControls()
	self._controlBindings = {};
	self._buttonActions = {};

	self:_bindControl(self.eventDropdown, "event", SoundEditorControlKind.Dropdown, SoundEditorOptions.events);
	self:_bindControl(self.eventPathEntry, "eventPath", SoundEditorControlKind.Text);
	self:_bindControl(self.timelinePositionSlider, "timelinePosition", SoundEditorControlKind.Value);
	self:_bindControl(self.timelineSpeedSlider, "timelineSpeed", SoundEditorControlKind.Value);
	self:_bindControl(self.volumeSlider, "volume", SoundEditorControlKind.Value);
	self:_bindControl(self.pitchSlider, "pitch", SoundEditorControlKind.Value);
	self:_bindControl(self.panSlider, "pan", SoundEditorControlKind.Value);
	self:_bindControl(self.prioritySlider, "priority", SoundEditorControlKind.Value);
	self:_bindControl(self.loopingCheckbox, "loopRegion", SoundEditorControlKind.Toggle);
	self:_bindControl(self.oneshotCheckbox, "oneShot", SoundEditorControlKind.Toggle);
	self:_bindControl(self.voiceStealingDropdown, "voiceStealing", SoundEditorControlKind.Dropdown);
	self:_bindControl(self.parameterDropdown, "parameter", SoundEditorControlKind.Dropdown, SoundEditorOptions.parameters);
	self:_bindControl(self.parameterValueSlider, "parameterValue", SoundEditorControlKind.Value);
	self:_bindControl(self.parameterSeekSpeedSlider, "parameterSeekSpeed", SoundEditorControlKind.Value);
	self:_bindControl(self.surfaceDropdown, "surface", SoundEditorControlKind.Dropdown, SoundEditorOptions.surfaces);
	self:_bindControl(self.musicStateDropdown, "musicState", SoundEditorControlKind.Dropdown, SoundEditorOptions.musicStates);
	self:_bindControl(self.distanceParameterSlider, "distanceParameter", SoundEditorControlKind.Value);
	self:_bindControl(self.intensityParameterSlider, "intensityParameter", SoundEditorControlKind.Value);
	self:_bindControl(self.busDropdown, "bus", SoundEditorControlKind.Dropdown, SoundEditorOptions.buses);
	self:_bindControl(self.vcaDropdown, "vca", SoundEditorControlKind.Dropdown, SoundEditorOptions.vcas);
	self:_bindControl(self.busVolumeSlider, "busVolume", SoundEditorControlKind.Value);
	self:_bindControl(self.busMuteCheckbox, "busMute", SoundEditorControlKind.Toggle);
	self:_bindControl(self.busSoloCheckbox, "busSolo", SoundEditorControlKind.Toggle);
	self:_bindControl(self.vcaVolumeSlider, "vcaVolume", SoundEditorControlKind.Value);
	self:_bindControl(self.reverbSendSlider, "reverbSend", SoundEditorControlKind.Value);
	self:_bindControl(self.delaySendSlider, "delaySend", SoundEditorControlKind.Value);
	self:_bindControl(self.sideChainSlider, "sideChain", SoundEditorControlKind.Value);
	self:_bindControl(self.modeDropdown, "mode", SoundEditorControlKind.Dropdown);
	self:_bindControl(self.spatializerDropdown, "spatializer", SoundEditorControlKind.Dropdown);
	self:_bindControl(self.minDistanceSlider, "minDistance", SoundEditorControlKind.Value);
	self:_bindControl(self.maxDistanceSlider, "maxDistance", SoundEditorControlKind.Value);
	self:_bindControl(self.dopplerSlider, "doppler", SoundEditorControlKind.Value);
	self:_bindControl(self.spreadSlider, "spread", SoundEditorControlKind.Value);
	self:_bindControl(self.occlusionSlider, "occlusion", SoundEditorControlKind.Value);
	self:_bindControl(self.directivitySlider, "directivity", SoundEditorControlKind.Value);
	self:_bindControl(self.rolloffDropdown, "rolloff", SoundEditorControlKind.Dropdown);
	self:_bindControl(self.effectSlotDropdown, "effectSlot", SoundEditorControlKind.Dropdown);
	self:_bindControl(self.effectTypeDropdown, "effectType", SoundEditorControlKind.Dropdown);
	self:_bindControl(self.effectWetSlider, "effectWet", SoundEditorControlKind.Value);
	self:_bindControl(self.effectBypassCheckbox, "effectBypass", SoundEditorControlKind.Toggle);
	self:_bindControl(self.reverbAmountSlider, "reverbAmount", SoundEditorControlKind.Value);
	self:_bindControl(self.reverbDecaySlider, "reverbDecay", SoundEditorControlKind.Value);
	self:_bindControl(self.reverbRoomSizeSlider, "reverbRoomSize", SoundEditorControlKind.Value);
	self:_bindControl(self.lowEqSlider, "lowEQ", SoundEditorControlKind.Value);
	self:_bindControl(self.midEqSlider, "midEQ", SoundEditorControlKind.Value);
	self:_bindControl(self.highEqSlider, "highEQ", SoundEditorControlKind.Value);
	self:_bindControl(self.lowpassSlider, "lowpass", SoundEditorControlKind.Value);
	self:_bindControl(self.highpassSlider, "highpass", SoundEditorControlKind.Value);
	self:_bindControl(self.compressorThresholdSlider, "compressorThreshold", SoundEditorControlKind.Value);
	self:_bindControl(self.compressorRatioSlider, "compressorRatio", SoundEditorControlKind.Value);
	self:_bindControl(self.limiterCeilingSlider, "limiterCeiling", SoundEditorControlKind.Value);
	self:_bindControl(self.bankDropdown, "bank", SoundEditorControlKind.Dropdown, SoundEditorOptions.banks);
	self:_bindControl(self.bankPathEntry, "bankPath", SoundEditorControlKind.Text);
	self:_bindControl(self.liveUpdateCheckbox, "liveUpdate", SoundEditorControlKind.Toggle);
	self:_bindControl(self.sampleDataCheckbox, "sampleData", SoundEditorControlKind.Toggle);
	self:_bindControl(self.snapshotDropdown, "snapshot", SoundEditorControlKind.Dropdown, SoundEditorOptions.snapshots);
	self:_bindControl(self.snapshotIntensitySlider, "snapshotIntensity", SoundEditorControlKind.Value);
	self:_bindControl(self.snapshotFadeSlider, "snapshotFadeTime", SoundEditorControlKind.Value);
	self:_bindControl(self.cpuUsageSlider, "dspCPU", SoundEditorControlKind.Value);
	self:_bindControl(self.memoryUsageSlider, "audioMemoryMB", SoundEditorControlKind.Value);
	self:_bindControl(self.voicesSlider, "activeVoices", SoundEditorControlKind.Value);
	self:_bindControl(self.virtualVoicesSlider, "virtualVoices", SoundEditorControlKind.Value);
	self:_bindControl(self.commandEntry, "rawCommand", SoundEditorControlKind.Text);

	self:_bindButton(self.playButton, "playEvent");
	self:_bindButton(self.pauseButton, "pauseEvent");
	self:_bindButton(self.resumeButton, "resumeEvent");
	self:_bindButton(self.stopButton, "stopEvent");
	self:_bindButton(self.stopImmediateButton, "stopEventImmediate");
	self:_bindButton(self.stopAllowFadeoutButton, "stopEventAllowFadeout");
	self:_bindButton(self.refreshEventsButton, "refreshEvents");
	self:_bindButton(self.loadEventButton, "loadEvent");
	self:_bindButton(self.createParameterButton, "createParameter");
	self:_bindButton(self.deleteParameterButton, "deleteParameter");
	self:_bindButton(self.routeToBusButton, "routeEventToBus");
	self:_bindButton(self.addEffectButton, "addOrReplaceEffect");
	self:_bindButton(self.removeEffectButton, "removeEffect");
	self:_bindButton(self.loadBankButton, "loadBank");
	self:_bindButton(self.unloadBankButton, "unloadBank");
	self:_bindButton(self.buildBanksButton, "buildBanks");
	self:_bindButton(self.startSnapshotButton, "startSnapshot");
	self:_bindButton(self.stopSnapshotButton, "stopSnapshot");
	self:_bindButton(self.executeCommandButton, "executeRawCommand");
	self:_bindButton(self.clearDebugButton, "clearDebugValues");
end

function SoundEditor:load()
	print("SoundEditor load start");

	local windowTypeInfo = IUIWindow.typeInfo();
	local dropdownTypeInfo = IUIDropdown.typeInfo();
	local buttonTypeInfo = IUIButton.typeInfo();
	local imageTypeInfo = IUIImage.typeInfo();
	local textTypeInfo = IUIText.typeInfo();
	local checkboxTypeInfo = IUICheckbox.typeInfo();
	local tabBarTypeInfo = IUITabBar.typeInfo();
	local collapsingHeaderTypeInfo = IUICollapsingHeader.typeInfo();
	local sliderPairTypeInfo = IUILabelSliderPair.typeInfo();
	local textEntryTypeInfo = IUITextEntry.typeInfo();
	local toggleSwitchTypeInfo = IUILabelTogglePair.typeInfo();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	local parentWindow = self.window:getParentWindow();

	local editorWindow = ui:addElement(windowTypeInfo);
	editorWindow:setLabel("Sound Event Editor");
	editorWindow:setSize(Vector2F(900.0, 650.0));
	self.editorWindow = editorWindow;
	parentWindow:addChild(self.editorWindow);

	-- FMOD Studio style tab layout.
	self.tabBar = ui:addElement(tabBarTypeInfo);
	self.eventsTab = self.tabBar:addTabItem();
	self.eventsTab:setLabel("Events");
	self.parametersTab = self.tabBar:addTabItem();
	self.parametersTab:setLabel("Parameters");
	self.mixerTab = self.tabBar:addTabItem();
	self.mixerTab:setLabel("Mixer");
	self.spatialTab = self.tabBar:addTabItem();
	self.spatialTab:setLabel("3D");
	self.effectsTab = self.tabBar:addTabItem();
	self.effectsTab:setLabel("Effects");
	self.banksTab = self.tabBar:addTabItem();
	self.banksTab:setLabel("Banks");
	self.snapshotsTab = self.tabBar:addTabItem();
	self.snapshotsTab:setLabel("Snapshots");
	self.advancedTab = self.tabBar:addTabItem();
	self.advancedTab:setLabel("Debug");
	self.editorWindow:addChild(self.tabBar);

	-- Events tab: event browser, transport and timeline audition controls.
	self.eventBrowserHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.eventsTab, "Event Browser");
	self.eventDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.eventBrowserHeader, "Event", {
		"event:/SFX/Test",
		"event:/SFX/Explosion",
		"event:/SFX/Footstep",
		"event:/Music/Menu",
		"event:/Ambience/City"
	}, 0);
	self.eventPathEntry = ui:addElement(textEntryTypeInfo);
	self.eventPathEntry:setLabel("Event Path");
	if self.eventPathEntry.setText then
		self.eventPathEntry:setText(self._selectedEventPath);
	end
	self.eventBrowserHeader:addChild(self.eventPathEntry);
	self.refreshEventsButton = self:_addButton(ui, buttonTypeInfo, self.eventBrowserHeader, "Refresh Events");
	self.loadEventButton = self:_addButton(ui, buttonTypeInfo, self.eventBrowserHeader, "Load Event");

	self.transportHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.eventsTab, "Transport");
	self.playButton = self:_addButton(ui, buttonTypeInfo, self.transportHeader, "Play");
	self.pauseButton = self:_addButton(ui, buttonTypeInfo, self.transportHeader, "Pause");
	self.resumeButton = self:_addButton(ui, buttonTypeInfo, self.transportHeader, "Resume");
	self.stopButton = self:_addButton(ui, buttonTypeInfo, self.transportHeader, "Stop");
	self.stopImmediateButton = self:_addButton(ui, buttonTypeInfo, self.transportHeader, "Stop Immediate");
	self.stopAllowFadeoutButton = self:_addButton(ui, buttonTypeInfo, self.transportHeader, "Stop Fadeout");
	self.timelinePositionSlider = self:_addSlider(ui, sliderPairTypeInfo, self.transportHeader, "Timeline Position ms", 0.0, 300000.0, 0.0);
	self.timelineSpeedSlider = self:_addSlider(ui, sliderPairTypeInfo, self.transportHeader, "Playback Speed", 0.0, 2.0, 1.0);
	self.loopingCheckbox = self:_addToggle(ui, toggleSwitchTypeInfo, self.transportHeader, "Loop Region", false);
	self.oneshotCheckbox = self:_addToggle(ui, toggleSwitchTypeInfo, self.transportHeader, "One Shot", false);

	self.basicHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.eventsTab, "Event Properties");
	self.volumeSlider = self:_addSlider(ui, sliderPairTypeInfo, self.basicHeader, "Volume", 0.0, 1.0, 1.0);
	self.pitchSlider = self:_addSlider(ui, sliderPairTypeInfo, self.basicHeader, "Pitch", 0.5, 2.0, 1.0);
	self.panSlider = self:_addSlider(ui, sliderPairTypeInfo, self.basicHeader, "Pan", -1.0, 1.0, 0.0);
	self.prioritySlider = self:_addSlider(ui, sliderPairTypeInfo, self.basicHeader, "Priority", 0.0, 256.0, 128.0);
	self.voiceStealingDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.basicHeader, "Voice Stealing", {
		"None",
		"Quietest",
		"Oldest",
		"Farthest",
		"Virtualize"
	}, 0);

	-- Parameters tab: game parameters, automation targets and labelled parameter style controls.
	self.parameterHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.parametersTab, "Game Parameters");
	self.parameterDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.parameterHeader, "Parameter", {
		"Intensity",
		"Speed",
		"Health",
		"Surface",
		"Distance"
	}, 0);
	self.parameterValueSlider = self:_addSlider(ui, sliderPairTypeInfo, self.parameterHeader, "Parameter Value", 0.0, 1.0, 0.0);
	self.parameterSeekSpeedSlider = self:_addSlider(ui, sliderPairTypeInfo, self.parameterHeader, "Seek Speed", 0.0, 10.0, 1.0);
	self.createParameterButton = self:_addButton(ui, buttonTypeInfo, self.parameterHeader, "Create Parameter");
	self.deleteParameterButton = self:_addButton(ui, buttonTypeInfo, self.parameterHeader, "Delete Parameter");

	self.builtInParameterHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.parametersTab, "Built-in / Labelled Parameters");
	self.surfaceDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.builtInParameterHeader, "Surface", {
		"Concrete",
		"Metal",
		"Wood",
		"Grass",
		"Water"
	}, 0);
	self.musicStateDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.builtInParameterHeader, "Music State", {
		"Menu",
		"Explore",
		"Combat",
		"Victory",
		"Defeat"
	}, 0);
	self.distanceParameterSlider = self:_addSlider(ui, sliderPairTypeInfo, self.builtInParameterHeader, "Distance Parameter", 0.0, 1000.0, 0.0);
	self.intensityParameterSlider = self:_addSlider(ui, sliderPairTypeInfo, self.builtInParameterHeader, "Intensity", 0.0, 1.0, 0.0);

	-- Mixer tab: buses, VCAs, sends and routing.
	self.busHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.mixerTab, "Mixer Routing");
	self.busDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.busHeader, "Output Bus", {
		"bus:/Master",
		"bus:/SFX",
		"bus:/Music",
		"bus:/Ambience",
		"bus:/UI"
	}, 0);
	self.vcaDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.busHeader, "VCA", {
		"vca:/Master",
		"vca:/SFX",
		"vca:/Music",
		"vca:/Dialog"
	}, 0);
	self.busVolumeSlider = self:_addSlider(ui, sliderPairTypeInfo, self.busHeader, "Bus Volume", 0.0, 1.0, 1.0);
	self.busMuteCheckbox = self:_addToggle(ui, toggleSwitchTypeInfo, self.busHeader, "Mute Bus", false);
	self.busSoloCheckbox = self:_addToggle(ui, toggleSwitchTypeInfo, self.busHeader, "Solo Bus", false);
	self.vcaVolumeSlider = self:_addSlider(ui, sliderPairTypeInfo, self.busHeader, "VCA Volume", 0.0, 1.0, 1.0);

	self.sendHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.mixerTab, "Sends / Returns");
	self.reverbSendSlider = self:_addSlider(ui, sliderPairTypeInfo, self.sendHeader, "Reverb Send", 0.0, 1.0, 0.0);
	self.delaySendSlider = self:_addSlider(ui, sliderPairTypeInfo, self.sendHeader, "Delay Send", 0.0, 1.0, 0.0);
	self.sideChainSlider = self:_addSlider(ui, sliderPairTypeInfo, self.sendHeader, "Side Chain Amount", 0.0, 1.0, 0.0);
	self.routeToBusButton = self:_addButton(ui, buttonTypeInfo, self.sendHeader, "Route Event To Bus");

	-- 3D tab: spatializer controls.
	self.modeHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.spatialTab, "Playback Mode");
	self.modeDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.modeHeader, "Mode", {
		"2D",
		"3D"
	}, 0);
	self.spatializerDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.modeHeader, "Spatializer", {
		"None",
		"FMOD 3D Panner",
		"Object Spatializer",
		"Ambisonics"
	}, 1);

	self.spatialHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.spatialTab, "3D Spatialization");
	self.minDistanceSlider = self:_addSlider(ui, sliderPairTypeInfo, self.spatialHeader, "Min Distance", 0.1, 100.0, 1.0);
	self.maxDistanceSlider = self:_addSlider(ui, sliderPairTypeInfo, self.spatialHeader, "Max Distance", 1.0, 1000.0, 100.0);
	self.dopplerSlider = self:_addSlider(ui, sliderPairTypeInfo, self.spatialHeader, "Doppler Scale", 0.0, 5.0, 1.0);
	self.spreadSlider = self:_addSlider(ui, sliderPairTypeInfo, self.spatialHeader, "Spread", 0.0, 360.0, 0.0);
	self.occlusionSlider = self:_addSlider(ui, sliderPairTypeInfo, self.spatialHeader, "Occlusion", 0.0, 1.0, 0.0);
	self.directivitySlider = self:_addSlider(ui, sliderPairTypeInfo, self.spatialHeader, "Directivity", 0.0, 1.0, 0.0);
	self.rolloffDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.spatialHeader, "Rolloff", {
		"Inverse",
		"Linear",
		"Linear Squared",
		"Custom"
	}, 0);

	-- Effects tab: insert chain similar to FMOD deck/effect rack.
	self.effectRackHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.effectsTab, "Effect Deck");
	self.effectSlotDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.effectRackHeader, "Effect Slot", {
		"Slot 1",
		"Slot 2",
		"Slot 3",
		"Slot 4"
	}, 0);
	self.effectTypeDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.effectRackHeader, "Effect Type", {
		"None",
		"EQ",
		"Compressor",
		"Limiter",
		"Reverb",
		"Delay",
		"Distortion",
		"Lowpass",
		"Highpass"
	}, 0);
	self.addEffectButton = self:_addButton(ui, buttonTypeInfo, self.effectRackHeader, "Add / Replace Effect");
	self.removeEffectButton = self:_addButton(ui, buttonTypeInfo, self.effectRackHeader, "Remove Effect");
	self.effectWetSlider = self:_addSlider(ui, sliderPairTypeInfo, self.effectRackHeader, "Wet", 0.0, 1.0, 1.0);
	self.effectBypassCheckbox = self:_addToggle(ui, toggleSwitchTypeInfo, self.effectRackHeader, "Bypass", false);

	self.reverbHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.effectsTab, "Reverb");
	self.reverbAmountSlider = self:_addSlider(ui, sliderPairTypeInfo, self.reverbHeader, "Reverb Amount", 0.0, 1.0, 0.0);
	self.reverbDecaySlider = self:_addSlider(ui, sliderPairTypeInfo, self.reverbHeader, "Decay", 0.1, 20.0, 1.5);
	self.reverbRoomSizeSlider = self:_addSlider(ui, sliderPairTypeInfo, self.reverbHeader, "Room Size", 0.0, 1.0, 0.5);

	self.eqHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.effectsTab, "EQ / Filter");
	self.lowEqSlider = self:_addSlider(ui, sliderPairTypeInfo, self.eqHeader, "Low EQ", -12.0, 12.0, 0.0);
	self.midEqSlider = self:_addSlider(ui, sliderPairTypeInfo, self.eqHeader, "Mid EQ", -12.0, 12.0, 0.0);
	self.highEqSlider = self:_addSlider(ui, sliderPairTypeInfo, self.eqHeader, "High EQ", -12.0, 12.0, 0.0);
	self.lowpassSlider = self:_addSlider(ui, sliderPairTypeInfo, self.eqHeader, "Lowpass Hz", 20.0, 22000.0, 22000.0);
	self.highpassSlider = self:_addSlider(ui, sliderPairTypeInfo, self.eqHeader, "Highpass Hz", 20.0, 22000.0, 20.0);

	self.dynamicsHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.effectsTab, "Dynamics");
	self.compressorThresholdSlider = self:_addSlider(ui, sliderPairTypeInfo, self.dynamicsHeader, "Compressor Threshold dB", -80.0, 0.0, -18.0);
	self.compressorRatioSlider = self:_addSlider(ui, sliderPairTypeInfo, self.dynamicsHeader, "Compressor Ratio", 1.0, 20.0, 4.0);
	self.limiterCeilingSlider = self:_addSlider(ui, sliderPairTypeInfo, self.dynamicsHeader, "Limiter Ceiling dB", -24.0, 0.0, -1.0);

	-- Banks tab: bank loading/building controls.
	self.bankHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.banksTab, "Banks");
	self.bankDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.bankHeader, "Bank", {
		"Master.bank",
		"Master.strings.bank",
		"SFX.bank",
		"Music.bank",
		"Ambience.bank"
	}, 0);
	self.bankPathEntry = ui:addElement(textEntryTypeInfo);
	self.bankPathEntry:setLabel("Bank Path");
	if self.bankPathEntry.setText then
		self.bankPathEntry:setText("media/audio/Desktop/Master.bank");
	end
	self.bankHeader:addChild(self.bankPathEntry);
	self.loadBankButton = self:_addButton(ui, buttonTypeInfo, self.bankHeader, "Load Bank");
	self.unloadBankButton = self:_addButton(ui, buttonTypeInfo, self.bankHeader, "Unload Bank");
	self.buildBanksButton = self:_addButton(ui, buttonTypeInfo, self.bankHeader, "Build Banks");
	self.liveUpdateCheckbox = self:_addToggle(ui, toggleSwitchTypeInfo, self.bankHeader, "Live Update", true);
	self.sampleDataCheckbox = self:_addToggle(ui, toggleSwitchTypeInfo, self.bankHeader, "Load Sample Data", true);

	-- Snapshots tab: snapshot audition and mix state controls.
	self.snapshotHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.snapshotsTab, "Snapshots");
	self.snapshotDropdown = self:_addDropdown(ui, dropdownTypeInfo, self.snapshotHeader, "Snapshot", {
		"snapshot:/Gameplay/Default",
		"snapshot:/Gameplay/Pause",
		"snapshot:/Gameplay/SlowMotion",
		"snapshot:/Mix/Underwater",
		"snapshot:/Mix/LowHealth"
	}, 0);
	self.snapshotIntensitySlider = self:_addSlider(ui, sliderPairTypeInfo, self.snapshotHeader, "Snapshot Intensity", 0.0, 1.0, 1.0);
	self.snapshotFadeSlider = self:_addSlider(ui, sliderPairTypeInfo, self.snapshotHeader, "Fade Time", 0.0, 10.0, 1.0);
	self.startSnapshotButton = self:_addButton(ui, buttonTypeInfo, self.snapshotHeader, "Start Snapshot");
	self.stopSnapshotButton = self:_addButton(ui, buttonTypeInfo, self.snapshotHeader, "Stop Snapshot");

	-- Debug tab: raw commands and runtime stats.
	self.advancedHeader = self:_addHeader(ui, collapsingHeaderTypeInfo, self.advancedTab, "Debug & Raw Properties");
	self.testText = self:_addText(ui, textTypeInfo, self.advancedHeader, "FMOD Studio style sound event editor", false);
	self.cpuUsageSlider = self:_addSlider(ui, sliderPairTypeInfo, self.advancedHeader, "DSP CPU %", 0.0, 100.0, 0.0);
	self.memoryUsageSlider = self:_addSlider(ui, sliderPairTypeInfo, self.advancedHeader, "Audio Memory MB", 0.0, 2048.0, 0.0);
	self.voicesSlider = self:_addSlider(ui, sliderPairTypeInfo, self.advancedHeader, "Active Voices", 0.0, 256.0, 0.0);
	self.virtualVoicesSlider = self:_addSlider(ui, sliderPairTypeInfo, self.advancedHeader, "Virtual Voices", 0.0, 256.0, 0.0);
	self.commandEntry = ui:addElement(textEntryTypeInfo);
	self.commandEntry:setLabel("Raw Command");
	if self.commandEntry.setText then
		self.commandEntry:setText("event:/SFX/Test play");
	end
	self.advancedHeader:addChild(self.commandEntry);
	self.executeCommandButton = self:_addButton(ui, buttonTypeInfo, self.advancedHeader, "Execute Command");
	self.clearDebugButton = self:_addButton(ui, buttonTypeInfo, self.advancedHeader, "Clear Debug Values");
	self.statusText = self:_addText(ui, textTypeInfo, self.advancedHeader, self._currentStatus or "Ready.", false);

	self:_registerEventControls();
	self:_syncFromControls();
	self:_applyManagerSettings();

	print("SoundEditor load end");
end

function SoundEditor:unload()
	print("SoundEditor unload called");

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	if self._currentSound ~= nil then
		self:_safeCall(self._currentSound, "stop");
	end
	
	if (self.editorWindow) then
		print("SoundEditor unload self.editorWindow");
	
		self.editorWindow:setVisible(false, false);
		self.editorWindow:destroyAllChildren();
		
		local editorParent = self.editorWindow:getParent();
		if (editorParent) then
			editorParent:removeChild(self.editorWindow);			
		end	
		
		ui:removeElement(self.editorWindow);
		
		self.editorWindow = nil;
	end	
	
	self.editorWindow = nil;
	self.tabBar = nil;
	self.testText = nil;
	self.statusText = nil;
	self._controlBindings = {};
	self._buttonActions = {};
	self._soundCache = {};
	self._currentSound = nil;
	self._isPlaying = false;
	self._isPaused = false;

	self.playButton = nil;
	self.pauseButton = nil;
	self.resumeButton = nil;
	self.stopButton = nil;
	self.stopImmediateButton = nil;
	self.stopAllowFadeoutButton = nil;
	self.loadEventButton = nil;
	self.refreshEventsButton = nil;
	self.createParameterButton = nil;
	self.deleteParameterButton = nil;
	self.addEffectButton = nil;
	self.removeEffectButton = nil;
	self.routeToBusButton = nil;
	self.loadBankButton = nil;
	self.unloadBankButton = nil;
	self.buildBanksButton = nil;
	self.startSnapshotButton = nil;
	self.stopSnapshotButton = nil;
	self.executeCommandButton = nil;
	self.clearDebugButton = nil;

	print("SoundEditor unload end");
end

function SoundEditor:show()
	print("SoundEditor show called");

	local parentWindow = self.window:getParentWindow();
	local debugWindow = self.window:getDebugWindow();
	
	if parentWindow then
		parentWindow:setVisible(true, false);
	end
	
	if debugWindow then
		--debugWindow:setVisible(false, false);
		--debugWindow:setVisible(true, false); -- for testing
	end
	
	if self.editorWindow then
		self.editorWindow:setVisible(true, false);
		print("SoundEditor show setVisible");
	end
	
	print("SoundEditor show end");
end

function SoundEditor:hide()
	print("SoundEditor hide called");

	local debugWindow = self.window:getDebugWindow();
		
	if debugWindow then
		debugWindow:setVisible(false, false);
	end
	
	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
	end
	
	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(false, false);
	end
end

function SoundEditor:_performAction(actionName, results)
	self:_syncFromControls();
	self:_recordAction(actionName, results);

	if actionName == "loadEvent" then
		local sound, message = self:_loadSound(self:_getCurrentEventPath());
		self._currentSound = sound;
		self:_setResult(results, "sound", sound);
		self:_setStatus(message, results);
	elseif actionName == "playEvent" then
		local sound = self:_ensureCurrentSound(results);
		if sound ~= nil then
			self:_applyCurrentSoundSettings(sound);
			local ok = self:_safeCall(sound, "play");
			self._isPlaying = ok == true;
			self._isPaused = false;
			self:_setResult(results, "playing", self._isPlaying);
			if ok then
				self:_setStatus("Playing " .. tostring(self:_getCurrentEventPath()), results);
			else
				self:_setStatus("Sound resource does not expose play().", results);
			end
		end
	elseif actionName == "pauseEvent" then
		if self._currentSound ~= nil then
			local ok = self:_safeCall(self._currentSound, "pause");
			self._isPaused = ok == true;
			if ok then
				self:_setStatus("Paused " .. tostring(self:_getCurrentEventPath()), results);
			else
				self:_setStatus("Pause requested; this Lua sound binding does not expose pause().", results);
			end
		else
			self:_setStatus("No sound is loaded.", results);
		end
	elseif actionName == "resumeEvent" then
		local sound = self:_ensureCurrentSound(results);
		if sound ~= nil then
			local ok = self:_safeCall(sound, "play");
			self._isPlaying = ok == true;
			self._isPaused = false;
			self:_setStatus("Resumed " .. tostring(self:_getCurrentEventPath()), results);
		end
	elseif actionName == "stopEvent" or actionName == "stopEventImmediate" or actionName == "stopEventAllowFadeout" then
		if self._currentSound ~= nil then
			self:_safeCall(self._currentSound, "stop");
		end
		self._isPlaying = false;
		self._isPaused = false;
		self:_setStatus("Stopped " .. tostring(self:_getCurrentEventPath()), results);
	elseif actionName == "refreshEvents" then
		self:_setStatus("Event list refreshed (" .. tostring(#SoundEditorOptions.events) .. " built-in entries).", results);
	elseif actionName == "createParameter" then
		local name = self:_getCurrentParameterName();
		self._parameters[name] =
		{
			value = self._settings.parameterValue or 0.0,
			seekSpeed = self._settings.parameterSeekSpeed or 0.0,
		};
		self:_setResult(results, "parameterName", name);
		self:_setStatus("Created/updated parameter: " .. tostring(name), results);
	elseif actionName == "deleteParameter" then
		local name = self:_getCurrentParameterName();
		self._parameters[name] = nil;
		self:_setStatus("Deleted parameter: " .. tostring(name), results);
	elseif actionName == "routeEventToBus" then
		self._settings.eventBus = self:_getOptionLabel(SoundEditorOptions.buses, self._settings.bus, "bus:/Master");
		self:_applyManagerSettings();
		self:_setStatus("Routed event to " .. tostring(self._settings.eventBus), results);
	elseif actionName == "addOrReplaceEffect" then
		local slot = self._settings.effectSlot or 0;
		self._effects[slot] =
		{
			type = self._settings.effectType or 0,
			wet = self._settings.effectWet or 1.0,
			bypass = self._settings.effectBypass == true,
		};
		self:_setStatus("Effect slot " .. tostring(slot + 1) .. " updated.", results);
	elseif actionName == "removeEffect" then
		local slot = self._settings.effectSlot or 0;
		self._effects[slot] = nil;
		self:_setStatus("Effect slot " .. tostring(slot + 1) .. " cleared.", results);
	elseif actionName == "loadBank" then
		local bankPath = self:_getCurrentBankPath();
		local resourceDatabase = self:_getResourceDatabase();
		local loaded = false;
		if resourceDatabase ~= nil then
			local ok, resource = self:_safeCall(resourceDatabase, "loadResource", bankPath);
			loaded = ok and resource ~= nil;
			self:_setResult(results, "bankResource", resource);
		end
		self._loadedBanks[bankPath] = true;
		if loaded then
			self:_setStatus("Loaded bank resource: " .. tostring(bankPath), results);
		else
			self:_setStatus("Bank tracked as loaded: " .. tostring(bankPath), results);
		end
	elseif actionName == "unloadBank" then
		local bankPath = self:_getCurrentBankPath();
		self._loadedBanks[bankPath] = nil;
		self:_setStatus("Unloaded bank from editor state: " .. tostring(bankPath), results);
	elseif actionName == "buildBanks" then
		self:_setStatus("Bank build requested for " .. tostring(self:_getCurrentBankName()), results);
	elseif actionName == "startSnapshot" then
		self._activeSnapshot =
		{
			path = self:_getCurrentSnapshotPath(),
			intensity = self._settings.snapshotIntensity or 1.0,
			fadeTime = self._settings.snapshotFadeTime or 0.0,
		};
		self:_setStatus("Started snapshot: " .. tostring(self._activeSnapshot.path), results);
	elseif actionName == "stopSnapshot" then
		local path = self._activeSnapshot and self._activeSnapshot.path or self:_getCurrentSnapshotPath();
		self._activeSnapshot = nil;
		self:_setStatus("Stopped snapshot: " .. tostring(path), results);
	elseif actionName == "executeRawCommand" then
		self:_executeRawCommand(results);
	elseif actionName == "clearDebugValues" then
		self._settings.dspCPU = 0.0;
		self._settings.audioMemoryMB = 0.0;
		self._settings.activeVoices = 0.0;
		self._settings.virtualVoices = 0.0;
		if self.cpuUsageSlider then self.cpuUsageSlider:setValue(0.0); end
		if self.memoryUsageSlider then self.memoryUsageSlider:setValue(0.0); end
		if self.voicesSlider then self.voicesSlider:setValue(0.0); end
		if self.virtualVoicesSlider then self.virtualVoicesSlider:setValue(0.0); end
		self:_setStatus("Debug values cleared.", results);
	end

	self:_setResult(results, "settings", self._settings);
end

function SoundEditor:_executeRawCommand(results)
	local command = self._settings.rawCommand or "";
	local path, op = string.match(command, "^%s*(%S+)%s+(%S+)%s*$");

	if path ~= nil and string.sub(path, 1, 6) == "event:" then
		self._settings.eventPath = path;
		if self.eventPathEntry then
			self.eventPathEntry:setText(path);
		end
	else
		op = string.match(command, "^%s*(%S+)%s*$");
	end

	if op == "play" then
		self:_performAction("playEvent", results);
	elseif op == "stop" then
		self:_performAction("stopEvent", results);
	elseif op == "pause" then
		self:_performAction("pauseEvent", results);
	elseif op == "resume" then
		self:_performAction("resumeEvent", results);
	elseif op == "load" then
		self:_performAction("loadEvent", results);
	elseif op == "mute" then
		self._settings.busMute = true;
		if self.busMuteCheckbox then self.busMuteCheckbox:setValue(true); end
		self:_applyManagerSettings();
		self:_setStatus("Muted audio.", results);
	elseif op == "unmute" then
		self._settings.busMute = false;
		if self.busMuteCheckbox then self.busMuteCheckbox:setValue(false); end
		self:_applyManagerSettings();
		self:_setStatus("Unmuted audio.", results);
	else
		self:_setStatus("Unknown raw command: " .. tostring(command), results);
	end
end

function SoundEditor:_handleControlChanged(sender, results)
	local binding = self._controlBindings and self._controlBindings[sender] or nil;
	if binding == nil then
		return false;
	end

	local value = self:_getControlValue(sender, binding.kind);
	self._settings[binding.key] = value;
	self:_setResult(results, binding.key, value);
	self:_setResult(results, "settings", self._settings);
	print("SoundEditor setting updated:", binding.key, value);

	self:_applyLiveSetting(binding.key, value, results);
	return true;
end

function SoundEditor:_handleDrop(sender, args, results)
	local dataStr = self:_safeArg(args, 0, nil);
	if dataStr == nil then
		return;
	end

	local filePath = dataStr;
	local ok, cjson = pcall(require, "cjson");
	if ok and cjson ~= nil then
		local decodeOk, decoded = pcall(function()
			return cjson.decode(dataStr);
		end);
		if decodeOk and decoded ~= nil and decoded.filePath ~= nil then
			filePath = decoded.filePath;
		end
	end

	if sender == self.bankPathEntry then
		self.bankPathEntry:setText(filePath);
		self._settings.bankPath = filePath;
		self:_setStatus("Bank path set from drop: " .. tostring(filePath), results);
	else
		if self.eventPathEntry then
			self.eventPathEntry:setText(filePath);
		end
		self._settings.eventPath = filePath;
		self._currentSound = nil;
		self:_setStatus("Event path set from drop: " .. tostring(filePath), results);
	end
end

function SoundEditor:handleEvent(parameters, results)
	if parameters == nil then
		return;
	end

	self._settings = self._settings or {};

	local legacyElement = nil;
	local legacyEventType = nil;
	local hasLegacyEvent = pcall(function()
		legacyElement = parameters.element;
		legacyEventType = parameters.eventType;
	end);

	if hasLegacyEvent and legacyElement ~= nil then
		local element = legacyElement;
		if legacyEventType == "clicked" then
			local actionName = self._buttonActions and self._buttonActions[element] or nil;
			if actionName ~= nil then
				self:_performAction(actionName, results);
			end
		else
			self:_handleControlChanged(element, results);
		end
		return;
	end

	local eventHash = parameters:at(1);
	local args = parameters:at(2);
	local sender = parameters:at(3);
	if sender == nil then
		return;
	end

	if eventHash == IEvent.handleSelection then
		local actionName = self._buttonActions and self._buttonActions[sender] or nil;
		if actionName ~= nil then
			self:_performAction(actionName, results);
		else
			self:_handleControlChanged(sender, results);
		end
	elseif eventHash == IEvent.handleValueChanged or eventHash == IEvent.handlePropertyChanged then
		self:_handleControlChanged(sender, results);
	elseif eventHash == IEvent.handleDrop then
		self:_handleDrop(sender, args, results);
	end
end

function SoundEditor:update()
	if self._currentSound ~= nil then
		local ok, playing = self:_safeCall(self._currentSound, "isPlaying");
		if ok then
			self._isPlaying = playing == true;
			self._settings.activeVoices = self._isPlaying and 1.0 or 0.0;
			if self.voicesSlider then
				self.voicesSlider:setValue(self._settings.activeVoices);
			end
		end
	end
end

function SoundEditor:getProperties(parameters)
	if parameters == nil then
		return;
	end

	local properties = parameters:at(0);
	if properties == nil then
		return;
	end

	self:_syncFromControls();
	for key, value in pairs(self._settings) do
		if type(value) == "boolean" then
			properties:setPropertyAsBool(key, value);
		elseif type(value) == "number" then
			properties:setPropertyAsFloat(key, value);
		elseif type(value) == "string" then
			properties:setPropertyAsString(key, value);
		end
	end
end

function SoundEditor:setProperties(parameters)
	if parameters == nil then
		return;
	end

	local properties = parameters:at(0);
	if properties == nil then
		return;
	end

	self._settings = self._settings or {};
	for key, defaultValue in pairs(SoundEditorDefaults) do
		if properties:hasProperty(key) then
			if type(defaultValue) == "boolean" then
				self._settings[key] = properties:getPropertyAsBool(key);
			elseif type(defaultValue) == "number" then
				self._settings[key] = properties:getPropertyAsFloat(key);
			else
				self._settings[key] = properties:getPropertyAsString(key);
			end
		end
	end

	self:_syncToControls();
	self:_applyManagerSettings();
end
