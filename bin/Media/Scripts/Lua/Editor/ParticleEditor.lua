class 'ParticleEditor' (BaseEditor)

--[[
	Production particle editor UI.

	This keeps the original editor style but expands it into a full production tool:
	- system/playback controls
	- renderer/material controls
	- emitter authoring
	- particle modules
	- over-lifetime controls
	- forces/collision/noise
	- sub emitters/trails/lights
	- LOD/performance/platform quality
	- preview, asset, import/export, debug/profiling

	The editor stores changed values in self.settings and emits actions in self.actions.
	Hook ParticleEditor:onSettingChanged() and ParticleEditor:performAction() into the C++
	particle system when the engine-side API is ready.
--]]

ParticleEditorControlKind =
{
	Value          = "value",
	Toggle         = "toggle",
	Dropdown       = "dropdown",
	Text           = "text",
	Colour3        = "colour3",
	Colour4        = "colour4",
}

local ParticleEditorTemplateNames =
{
	[0] = "",
	[1] = "Fire",
	[2] = "Smoke",
	[3] = "Explosion",
	[4] = "Sparks",
	[5] = "Magic",
	[6] = "Weather",
	[7] = "Trail",
	[8] = "Impact",
	[9] = "",
};

local function safeCall(object, methodName, ...)
	if object == nil then
		return false, nil;
	end

	local okLookup, method = pcall(function() return object[methodName]; end);
	if not okLookup or type(method) ~= "function" then
		return false, nil;
	end

	local okCall, result = pcall(method, object, ...);
	if not okCall then
		print("ParticleEditor optional call failed: " .. tostring(methodName) .. " - " .. tostring(result));
		return false, nil;
	end

	return true, result;
end

local function safeGet(object, methodName, defaultValue)
	local ok, value = safeCall(object, methodName);
	if ok and value ~= nil then
		return value;
	end
	return defaultValue;
end

local function safeVector2(x, y)
	local ok, value = pcall(function() return Vector2F(x, y); end);
	if ok then
		return value;
	end
	return nil;
end

local function safeVector3(x, y, z)
	local ok, value = pcall(function() return Vector3F(x, y, z); end);
	if ok then
		return value;
	end
	return nil;
end

local function clamp(value, minValue, maxValue)
	if value == nil then
		return minValue;
	end
	if value < minValue then
		return minValue;
	end
	if value > maxValue then
		return maxValue;
	end
	return value;
end

function ParticleEditor:__init(window)
	print("ParticleEditor constructor called");

	self.window = window;
	self.editorWindow = nil;

	self.settings = {};
	self.controlBindings = {};
	self.buttonActions = {};
	self.emitters = {};
	self.affectors = {};
	self.actions = {};

	self.selectedEmitterIndex = 1;
	self.selectedAffectorIndex = 1;
	self.isPlaying = false;
	self.isPaused = false;
	self.previewTime = 0.0;
	self.selectedParticleComponent = nil;
	self.selectedActor = nil;

	self.ui = nil;
	self.types = nil;
end

function ParticleEditor:__finalize()
	print("ParticleEditor __finalize called");

	self.window = nil;
	self.editorWindow = nil;
	self.settings = nil;
	self.controlBindings = nil;
	self.buttonActions = nil;
	self.emitters = nil;
	self.affectors = nil;
	self.actions = nil;
	self.selectedParticleComponent = nil;
	self.selectedActor = nil;
	self.ui = nil;
	self.types = nil;
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Small UI factory helpers
-- ─────────────────────────────────────────────────────────────────────────────

function ParticleEditor:registerControl(element, key, kind)
	if element == nil then
		return element;
	end

	self.controlBindings[element] =
	{
		key = key,
		kind = kind,
	};

	if self.window then
		self.window:setHandleEvents(element, true);
	end

	return element;
end

function ParticleEditor:registerButton(element, actionName)
	if element == nil then
		return element;
	end

	self.buttonActions[element] = actionName;

	if self.window then
		self.window:setHandleEvents(element, true);
	end

	return element;
end

function ParticleEditor:addHeader(parent, label)
	local header = self.ui:addElement(self.types.collapsingHeader);
	header:setLabel(label);
	parent:addChild(header);
	return header;
end

function ParticleEditor:addLabel(parent, name, text)
	local label = self.ui:addElement(self.types.text);
	label:setLabel(text or name);
	parent:addChild(label);
	self[name] = label;
	return label;
end

function ParticleEditor:addTextEntry(parent, name, label, defaultText, settingKey)
	local entry = self.ui:addElement(self.types.textEntry);
	entry:setLabel(label);
	entry:setText(defaultText or "");
	parent:addChild(entry);
	self[name] = entry;
	self.settings[settingKey or name] = defaultText or "";
	self:registerControl(entry, settingKey or name, ParticleEditorControlKind.Text);
	return entry;
end

function ParticleEditor:addButton(parent, name, label, actionName)
	local button = self.ui:addElement(self.types.button);
	button:setLabel(label);
	parent:addChild(button);
	self[name] = button;
	self:registerButton(button, actionName or name);
	return button;
end

function ParticleEditor:addToggle(parent, name, label, defaultValue, settingKey)
	local toggle = self.ui:addElement(self.types.toggle);
	toggle:setLabel(label);
	if defaultValue ~= nil then
		pcall(function()
			toggle:setValue(defaultValue);
		end);
	end
	parent:addChild(toggle);
	self[name] = toggle;
	self.settings[settingKey or name] = defaultValue or false;
	self:registerControl(toggle, settingKey or name, ParticleEditorControlKind.Toggle);
	return toggle;
end

function ParticleEditor:addSlider(parent, name, label, minValue, maxValue, defaultValue, settingKey)
	local slider = self.ui:addElement(self.types.sliderPair);
	slider:setLabel(label);
	slider:setMinValue(minValue);
	slider:setMaxValue(maxValue);
	slider:setValue(defaultValue);
	parent:addChild(slider);
	self[name] = slider;
	self.settings[settingKey or name] = defaultValue;
	self:registerControl(slider, settingKey or name, ParticleEditorControlKind.Value);
	return slider;
end

function ParticleEditor:addDropdown(parent, name, label, options, selectedOption, settingKey)
	local dropdown = self.ui:addElement(self.types.dropdown);
	dropdown:setLabel(label);
	for _, option in ipairs(options) do
		dropdown:addOption(option);
	end
	dropdown:setSelectedOption(selectedOption or 0);
	parent:addChild(dropdown);
	self[name] = dropdown;
	self.settings[settingKey or name] = selectedOption or 0;
	self:registerControl(dropdown, settingKey or name, ParticleEditorControlKind.Dropdown);
	return dropdown;
end

function ParticleEditor:addColour(parent, name, label, settingKey)
	local colour = self.ui:addElement(self.types.colourPicker);
	colour:setLabel(label);
	parent:addChild(colour);
	self[name] = colour;
	self.settings[settingKey or name] = {1.0, 1.0, 1.0, 1.0};
	self:registerControl(colour, settingKey or name, ParticleEditorControlKind.Colour4);
	return colour;
end

function ParticleEditor:safeArg(args, index, defaultValue)
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

function ParticleEditor:readControlValue(element, kind, args)
	if kind == ParticleEditorControlKind.Dropdown then
		return safeGet(element, "getSelectedOption", self:safeArg(args, 0, 0));
	elseif kind == ParticleEditorControlKind.Text then
		return safeGet(element, "getText", self:safeArg(args, 0, ""));
	elseif kind == ParticleEditorControlKind.Colour3 then
		return
		{
			self:safeArg(args, 0, 1.0),
			self:safeArg(args, 1, 1.0),
			self:safeArg(args, 2, 1.0),
		};
	elseif kind == ParticleEditorControlKind.Colour4 then
		return
		{
			self:safeArg(args, 0, 1.0),
			self:safeArg(args, 1, 1.0),
			self:safeArg(args, 2, 1.0),
			self:safeArg(args, 3, 1.0),
		};
	end

	return safeGet(element, "getValue", self:safeArg(args, 0, nil));
end

function ParticleEditor:writeResult(results, key, value)
	if results == nil then
		return;
	end

	pcall(function()
		results[key] = value;
	end);
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Load / unload / visibility
-- ─────────────────────────────────────────────────────────────────────────────

function ParticleEditor:load()
	print("ParticleEditor load start");

	local applicationManager = IApplicationManager.instance();
	self.ui = applicationManager:getUI();

	self.types =
	{
		window           = IUIWindow.typeInfo(),
		dropdown         = IUIDropdown.typeInfo(),
		button           = IUIButton.typeInfo(),
		text             = IUIText.typeInfo(),
		textEntry        = IUITextEntry.typeInfo(),
		toggle           = IUILabelTogglePair.typeInfo(),
		sliderPair       = IUILabelSliderPair.typeInfo(),
		collapsingHeader = IUICollapsingHeader.typeInfo(),
		tabBar           = IUITabBar.typeInfo(),
		colourPicker     = IUIColourPicker.typeInfo(),
	};

	local parentWindow = self.window:getParentWindow();
	if parentWindow == nil then
		print("ParticleEditor load failed: parentWindow nil");
		return;
	end

	parentWindow:setSize(Vector2F(820.0, 760.0));

	self.editorWindow = self.ui:addElement(self.types.window);
	self.editorWindow:setLabel("Particle Editor");
	self.editorWindow:setSize(Vector2F(820.0, 760.0));
	parentWindow:addChild(self.editorWindow);

	self.tabBar = self.ui:addElement(self.types.tabBar);
	self.systemTab     = self.tabBar:addTabItem(); self.systemTab:setLabel("System");
	self.rendererTab   = self.tabBar:addTabItem(); self.rendererTab:setLabel("Renderer");
	self.emittersTab   = self.tabBar:addTabItem(); self.emittersTab:setLabel("Emitters");
	self.modulesTab    = self.tabBar:addTabItem(); self.modulesTab:setLabel("Modules");
	self.lifetimeTab   = self.tabBar:addTabItem(); self.lifetimeTab:setLabel("Over Life");
	self.physicsTab    = self.tabBar:addTabItem(); self.physicsTab:setLabel("Physics");
	self.subEmittersTab = self.tabBar:addTabItem(); self.subEmittersTab:setLabel("Sub Emitters");
	self.performanceTab = self.tabBar:addTabItem(); self.performanceTab:setLabel("LOD/Perf");
	self.previewTab    = self.tabBar:addTabItem(); self.previewTab:setLabel("Preview");
	self.assetTab      = self.tabBar:addTabItem(); self.assetTab:setLabel("Asset");
	self.debugTab      = self.tabBar:addTabItem(); self.debugTab:setLabel("Debug");
	self.editorWindow:addChild(self.tabBar);

	self:buildSystemTab();
	self:buildRendererTab();
	self:buildEmittersTab();
	self:buildModulesTab();
	self:buildLifetimeTab();
	self:buildPhysicsTab();
	self:buildSubEmittersTab();
	self:buildPerformanceTab();
	self:buildPreviewTab();
	self:buildAssetTab();
	self:buildDebugTab();

	self:updateEmitterSummary();
	self:updateAffectorSummary();
	self:updateStatsLabel();
	self:updateSelection();

	print("ParticleEditor load end");
end

function ParticleEditor:unload()
	print("ParticleEditor unload called");

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
		self.editorWindow:destroyAllChildren();

		local editorParent = self.editorWindow:getParent();
		if editorParent then
			editorParent:removeChild(self.editorWindow);
		end

		ui:removeElement(self.editorWindow);
		self.editorWindow = nil;
	end

	self.controlBindings = {};
	self.buttonActions = {};
	self.ui = nil;
	self.types = nil;

	local fields =
	{
		"tabBar", "systemTab", "rendererTab", "emittersTab", "modulesTab", "lifetimeTab",
		"physicsTab", "subEmittersTab", "performanceTab", "previewTab", "assetTab", "debugTab",
		"systemStatsLabel", "emitterSummaryLabel", "affectorSummaryLabel", "debugStatsLabel"
	};

	for _, field in ipairs(fields) do
		self[field] = nil;
	end
end

function ParticleEditor:show()
	print("ParticleEditor show called");

	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(true, false);
	end

	if self.editorWindow then
		self.editorWindow:setVisible(true, false);
	end
end

function ParticleEditor:hide()
	print("ParticleEditor hide called");

	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
	end

	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(false, false);
	end
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Tabs
-- ─────────────────────────────────────────────────────────────────────────────

function ParticleEditor:buildSystemTab()
	local assetHeader = self:addHeader(self.systemTab, "Particle System");
	self:addTextEntry(assetHeader, "systemNameEntry", "System Name", "NewParticleSystem", "system.name");
	self:addTextEntry(assetHeader, "systemGuidEntry", "GUID", "", "system.guid");
	self:addDropdown(assetHeader, "systemTemplateDropdown", "Template", {"Empty", "Fire", "Smoke", "Explosion", "Sparks", "Magic", "Weather", "Trail", "Impact", "Custom"}, 0, "system.template");
	self:addToggle(assetHeader, "enabledToggle", "Enabled", true, "system.enabled");
	self:addToggle(assetHeader, "loopingToggle", "Looping", true, "system.looping");
	self:addToggle(assetHeader, "prewarmToggle", "Prewarm", false, "system.prewarm");
	self:addToggle(assetHeader, "playOnAwakeToggle", "Play On Awake", true, "system.playOnAwake");
	self:addToggle(assetHeader, "liveUpdateToggle", "Live Update While Editing", true, "system.liveUpdate");

	local timingHeader = self:addHeader(self.systemTab, "Timing");
	self:addSlider(timingHeader, "durationSlider", "Duration", 0.01, 120.0, 5.0, "system.duration");
	self:addSlider(timingHeader, "startDelaySlider", "Start Delay", 0.0, 30.0, 0.0, "system.startDelay");
	self:addSlider(timingHeader, "simulationSpeedSlider", "Simulation Speed", 0.0, 10.0, 1.0, "system.simulationSpeed");
	self:addSlider(timingHeader, "fixedTimeStepSlider", "Fixed Time Step", 0.001, 0.1, 0.016, "system.fixedTimeStep");
	self:addDropdown(timingHeader, "simulationSpaceDropdown", "Simulation Space", {"Local", "World", "Custom Transform"}, 0, "system.simulationSpace");
	self:addDropdown(timingHeader, "updateModeDropdown", "Update Mode", {"Game Time", "Unscaled Time", "Fixed Update", "Manual"}, 0, "system.updateMode");

	local limitsHeader = self:addHeader(self.systemTab, "Limits");
	self:addSlider(limitsHeader, "maxParticlesSlider", "Max Particles", 1, 200000, 1000, "system.maxParticles");
	self:addSlider(limitsHeader, "maxAliveSecondsSlider", "Max Alive Seconds", 0.1, 300.0, 30.0, "system.maxAliveSeconds");
	self:addDropdown(limitsHeader, "overflowModeDropdown", "Overflow Mode", {"Kill Oldest", "Kill Youngest", "Do Not Spawn", "Clamp Rate"}, 0, "system.overflowMode");
	self:addToggle(limitsHeader, "autoCullToggle", "Auto Cull When Offscreen", true, "system.autoCull");
	self:addSlider(limitsHeader, "cullGraceSlider", "Cull Grace Time", 0.0, 30.0, 3.0, "system.cullGraceTime");

	local playbackHeader = self:addHeader(self.systemTab, "Playback");
	self:addButton(playbackHeader, "playButton", "Play", "play");
	self:addButton(playbackHeader, "pauseButton", "Pause", "pause");
	self:addButton(playbackHeader, "resumeButton", "Resume", "resume");
	self:addButton(playbackHeader, "stopButton", "Stop", "stop");
	self:addButton(playbackHeader, "restartButton", "Restart", "restart");
	self:addButton(playbackHeader, "singleStepButton", "Single Step", "singleStep");

	self.systemStatsLabel = self:addLabel(self.systemTab, "systemStatsLabel", "Alive: --   Emitted: --   Culled: --   Draw Calls: --");
end

function ParticleEditor:buildRendererTab()
	local rendererHeader = self:addHeader(self.rendererTab, "Renderer");
	self:addDropdown(rendererHeader, "rendererModeDropdown", "Render Mode", {"Billboard", "Stretched Billboard", "Mesh", "Ribbon", "Trail", "Decal", "Light", "GPU Point Sprite"}, 0, "renderer.mode");
	self:addDropdown(rendererHeader, "billboardAlignmentDropdown", "Billboard Alignment", {"View", "World", "Local", "Velocity", "Facing Camera Position"}, 0, "renderer.billboardAlignment");
	self:addDropdown(rendererHeader, "sortModeDropdown", "Sort Mode", {"None", "By Distance", "Oldest First", "Youngest First", "Custom"}, 1, "renderer.sortMode");
	self:addDropdown(rendererHeader, "blendModeDropdown", "Blend Mode", {"Alpha", "Additive", "Premultiplied", "Multiply", "Soft Additive", "Opaque"}, 1, "renderer.blendMode");
	self:addDropdown(rendererHeader, "depthModeDropdown", "Depth Mode", {"Read/Write", "Read Only", "Ignore Depth", "Soft Particles"}, 0, "renderer.depthMode");
	self:addToggle(rendererHeader, "castShadowsToggle", "Cast Shadows", false, "renderer.castShadows");
	self:addToggle(rendererHeader, "receiveShadowsToggle", "Receive Shadows", false, "renderer.receiveShadows");
	self:addToggle(rendererHeader, "motionVectorsToggle", "Motion Vectors", false, "renderer.motionVectors");

	local materialHeader = self:addHeader(self.rendererTab, "Material / Atlas");
	self:addTextEntry(materialHeader, "materialPathEntry", "Material", "", "renderer.material");
	self:addTextEntry(materialHeader, "texturePathEntry", "Texture", "", "renderer.texture");
	self:addDropdown(materialHeader, "shaderDropdown", "Shader", {"Unlit Particle", "Lit Particle", "Distortion", "Flipbook", "Soft Particle", "Custom"}, 0, "renderer.shader");
	self:addSlider(materialHeader, "softParticleDistanceSlider", "Soft Particle Distance", 0.0, 10.0, 1.0, "renderer.softParticleDistance");
	self:addSlider(materialHeader, "cameraFadeNearSlider", "Camera Fade Near", 0.0, 10.0, 0.2, "renderer.cameraFadeNear");
	self:addSlider(materialHeader, "cameraFadeFarSlider", "Camera Fade Far", 0.0, 50.0, 2.0, "renderer.cameraFadeFar");
	self:addSlider(materialHeader, "atlasColumnsSlider", "Atlas Columns", 1, 32, 1, "renderer.atlasColumns");
	self:addSlider(materialHeader, "atlasRowsSlider", "Atlas Rows", 1, 32, 1, "renderer.atlasRows");
	self:addSlider(materialHeader, "frameRateSlider", "Flipbook FPS", 0.0, 120.0, 24.0, "renderer.flipbookFps");
	self:addToggle(materialHeader, "randomStartFrameToggle", "Random Start Frame", false, "renderer.randomStartFrame");

	local lightingHeader = self:addHeader(self.rendererTab, "Lighting");
	self:addSlider(lightingHeader, "normalStrengthSlider", "Normal Strength", 0.0, 5.0, 1.0, "renderer.normalStrength");
	self:addSlider(lightingHeader, "metallicSlider", "Metallic", 0.0, 1.0, 0.0, "renderer.metallic");
	self:addSlider(lightingHeader, "roughnessSlider", "Roughness", 0.0, 1.0, 0.5, "renderer.roughness");
	self:addSlider(lightingHeader, "emissiveStrengthSlider", "Emissive Strength", 0.0, 100.0, 1.0, "renderer.emissiveStrength");
	self:addColour(lightingHeader, "emissiveColourPicker", "Emissive Colour", "renderer.emissiveColour");
end

function ParticleEditor:buildEmittersTab()
	local listHeader = self:addHeader(self.emittersTab, "Emitter List");
	self:addDropdown(listHeader, "emitterListDropdown", "Selected Emitter", {"Emitter 1"}, 0, "emitter.selected");
	self:addButton(listHeader, "addEmitterButton", "Add Emitter", "addEmitter");
	self:addButton(listHeader, "duplicateEmitterButton", "Duplicate Emitter", "duplicateEmitter");
	self:addButton(listHeader, "removeEmitterButton", "Remove Emitter", "removeEmitter");
	self:addButton(listHeader, "moveEmitterUpButton", "Move Up", "moveEmitterUp");
	self:addButton(listHeader, "moveEmitterDownButton", "Move Down", "moveEmitterDown");
	self.emitterSummaryLabel = self:addLabel(listHeader, "emitterSummaryLabel", "Emitters: 1");

	local shapeHeader = self:addHeader(self.emittersTab, "Emitter Shape");
	self:addDropdown(shapeHeader, "emitterTypeDropdown", "Type", {"Point", "Box", "Sphere", "Hemisphere", "Cone", "Cylinder", "Circle", "Mesh Surface", "Mesh Volume", "Spline", "Skinned Mesh", "Texture Mask"}, 0, "emitter.type");
	self:addDropdown(shapeHeader, "emitFromDropdown", "Emit From", {"Volume", "Surface", "Edge", "Vertex", "Shell", "Base", "Random"}, 0, "emitter.emitFrom");
	self:addSlider(shapeHeader, "shapeRadiusSlider", "Radius", 0.0, 100.0, 1.0, "emitter.shape.radius");
	self:addSlider(shapeHeader, "shapeRadiusThicknessSlider", "Radius Thickness", 0.0, 1.0, 1.0, "emitter.shape.radiusThickness");
	self:addSlider(shapeHeader, "shapeAngleSlider", "Cone Angle", 0.0, 180.0, 25.0, "emitter.shape.angle");
	self:addSlider(shapeHeader, "shapeArcSlider", "Arc", 0.0, 360.0, 360.0, "emitter.shape.arc");
	self:addSlider(shapeHeader, "shapeLengthSlider", "Length", 0.0, 100.0, 1.0, "emitter.shape.length");
	self:addSlider(shapeHeader, "shapeWidthSlider", "Width", 0.0, 100.0, 1.0, "emitter.shape.width");
	self:addSlider(shapeHeader, "shapeHeightSlider", "Height", 0.0, 100.0, 1.0, "emitter.shape.height");
	self:addTextEntry(shapeHeader, "shapeMeshEntry", "Mesh Source", "", "emitter.shape.mesh");

	local emissionHeader = self:addHeader(self.emittersTab, "Emission");
	self:addSlider(emissionHeader, "rateOverTimeSlider", "Rate Over Time", 0.0, 100000.0, 100.0, "emitter.rateOverTime");
	self:addSlider(emissionHeader, "rateOverDistanceSlider", "Rate Over Distance", 0.0, 10000.0, 0.0, "emitter.rateOverDistance");
	self:addSlider(emissionHeader, "burstCountSlider", "Burst Count", 0, 10000, 10, "emitter.burst.count");
	self:addSlider(emissionHeader, "burstTimeSlider", "Burst Time", 0.0, 120.0, 0.0, "emitter.burst.time");
	self:addSlider(emissionHeader, "burstCyclesSlider", "Burst Cycles", 1, 100, 1, "emitter.burst.cycles");
	self:addSlider(emissionHeader, "burstIntervalSlider", "Burst Interval", 0.01, 30.0, 1.0, "emitter.burst.interval");
	self:addDropdown(emissionHeader, "emissionModeDropdown", "Emission Mode", {"Continuous", "Burst", "Distance", "Scripted", "Event Driven"}, 0, "emitter.emissionMode");

	local initialHeader = self:addHeader(self.emittersTab, "Initial Particle State");
	self:addSlider(initialHeader, "lifetimeSlider", "Lifetime", 0.01, 300.0, 5.0, "emitter.lifetime");
	self:addSlider(initialHeader, "lifetimeRandomSlider", "Lifetime Random", 0.0, 1.0, 0.0, "emitter.lifetimeRandom");
	self:addSlider(initialHeader, "startSizeSlider", "Start Size", 0.001, 100.0, 1.0, "emitter.startSize");
	self:addSlider(initialHeader, "startSizeRandomSlider", "Start Size Random", 0.0, 1.0, 0.0, "emitter.startSizeRandom");
	self:addSlider(initialHeader, "startRotationSlider", "Start Rotation", -360.0, 360.0, 0.0, "emitter.startRotation");
	self:addSlider(initialHeader, "startRotationRandomSlider", "Start Rotation Random", 0.0, 1.0, 0.0, "emitter.startRotationRandom");
	self:addColour(initialHeader, "startColorPicker", "Start Colour", "emitter.startColour");
	self:addSlider(initialHeader, "velocitySlider", "Start Speed", 0.0, 1000.0, 10.0, "emitter.startSpeed");
	self:addSlider(initialHeader, "velocityRandomSlider", "Speed Random", 0.0, 1.0, 0.0, "emitter.speedRandom");
	self:addSlider(initialHeader, "spreadSlider", "Direction Spread", 0.0, 180.0, 45.0, "emitter.spread");
	self:addSlider(initialHeader, "inheritVelocitySlider", "Inherit Velocity", -10.0, 10.0, 0.0, "emitter.inheritVelocity");

	local spawnHeader = self:addHeader(self.emittersTab, "Spawn Transform");
	self:addSlider(spawnHeader, "spawnOffsetXSlider", "Offset X", -100.0, 100.0, 0.0, "emitter.offset.x");
	self:addSlider(spawnHeader, "spawnOffsetYSlider", "Offset Y", -100.0, 100.0, 0.0, "emitter.offset.y");
	self:addSlider(spawnHeader, "spawnOffsetZSlider", "Offset Z", -100.0, 100.0, 0.0, "emitter.offset.z");
	self:addSlider(spawnHeader, "spawnRotationXSlider", "Rotation X", -360.0, 360.0, 0.0, "emitter.rotation.x");
	self:addSlider(spawnHeader, "spawnRotationYSlider", "Rotation Y", -360.0, 360.0, 0.0, "emitter.rotation.y");
	self:addSlider(spawnHeader, "spawnRotationZSlider", "Rotation Z", -360.0, 360.0, 0.0, "emitter.rotation.z");
end

function ParticleEditor:buildModulesTab()
	local modulesHeader = self:addHeader(self.modulesTab, "Enabled Modules");
	self:addToggle(modulesHeader, "colourModuleToggle", "Colour Over Lifetime", true, "module.colour.enabled");
	self:addToggle(modulesHeader, "sizeModuleToggle", "Size Over Lifetime", true, "module.size.enabled");
	self:addToggle(modulesHeader, "velocityModuleToggle", "Velocity Over Lifetime", false, "module.velocity.enabled");
	self:addToggle(modulesHeader, "rotationModuleToggle", "Rotation Over Lifetime", false, "module.rotation.enabled");
	self:addToggle(modulesHeader, "noiseModuleToggle", "Noise", false, "module.noise.enabled");
	self:addToggle(modulesHeader, "forceModuleToggle", "Force Field", false, "module.force.enabled");
	self:addToggle(modulesHeader, "collisionModuleToggle", "Collision", false, "module.collision.enabled");
	self:addToggle(modulesHeader, "triggerModuleToggle", "Triggers", false, "module.triggers.enabled");
	self:addToggle(modulesHeader, "lightsModuleToggle", "Lights", false, "module.lights.enabled");
	self:addToggle(modulesHeader, "trailsModuleToggle", "Trails", false, "module.trails.enabled");
	self:addToggle(modulesHeader, "subEmitterModuleToggle", "Sub Emitters", false, "module.subEmitters.enabled");
	self:addToggle(modulesHeader, "customDataModuleToggle", "Custom Data Streams", false, "module.customData.enabled");

	local affectorHeader = self:addHeader(self.modulesTab, "Affector Stack");
	self:addDropdown(affectorHeader, "affectorListDropdown", "Selected Affector", {"None"}, 0, "affector.selected");
	self:addDropdown(affectorHeader, "affectorTypeDropdown", "Affector Type", {"Gravity", "Linear Force", "Vortex", "Attractor", "Repulsor", "Colour", "Scale", "Drag", "Turbulence", "Kill", "Custom Script"}, 0, "affector.type");
	self:addDropdown(affectorHeader, "affectorBlendModeDropdown", "Blend Mode", {"Add", "Multiply", "Override", "Min", "Max", "Curve Weighted"}, 0, "affector.blendMode");
	self:addSlider(affectorHeader, "affectorStrengthSlider", "Strength", -1000.0, 1000.0, 10.0, "affector.strength");
	self:addSlider(affectorHeader, "affectorDurationSlider", "Duration", 0.0, 300.0, 5.0, "affector.duration");
	self:addSlider(affectorHeader, "affectorFalloffSlider", "Falloff", 0.0, 10.0, 1.0, "affector.falloff");
	self:addTextEntry(affectorHeader, "affectorMaskEntry", "Mask / Layer", "Default", "affector.mask");
	self:addButton(affectorHeader, "addAffectorButton", "Add Affector", "addAffector");
	self:addButton(affectorHeader, "removeAffectorButton", "Remove Affector", "removeAffector");
	self:addButton(affectorHeader, "moveAffectorUpButton", "Move Up", "moveAffectorUp");
	self:addButton(affectorHeader, "moveAffectorDownButton", "Move Down", "moveAffectorDown");
	self.affectorSummaryLabel = self:addLabel(affectorHeader, "affectorSummaryLabel", "Affectors: 0");

	local randomHeader = self:addHeader(self.modulesTab, "Randomisation");
	self:addSlider(randomHeader, "randomSeedSlider", "Random Seed", 0, 999999, 12345, "random.seed");
	self:addToggle(randomHeader, "autoRandomSeedToggle", "Auto Random Seed", true, "random.autoSeed");
	self:addSlider(randomHeader, "positionJitterSlider", "Position Jitter", 0.0, 100.0, 0.0, "random.positionJitter");
	self:addSlider(randomHeader, "directionJitterSlider", "Direction Jitter", 0.0, 180.0, 0.0, "random.directionJitter");
	self:addSlider(randomHeader, "colourJitterSlider", "Colour Jitter", 0.0, 1.0, 0.0, "random.colourJitter");
	self:addSlider(randomHeader, "alphaJitterSlider", "Alpha Jitter", 0.0, 1.0, 0.0, "random.alphaJitter");

	local streamsHeader = self:addHeader(self.modulesTab, "Vertex Streams / Custom Data");
	self:addToggle(streamsHeader, "streamPositionToggle", "Position", true, "stream.position");
	self:addToggle(streamsHeader, "streamColourToggle", "Colour", true, "stream.colour");
	self:addToggle(streamsHeader, "streamUvToggle", "UV", true, "stream.uv");
	self:addToggle(streamsHeader, "streamVelocityToggle", "Velocity", false, "stream.velocity");
	self:addToggle(streamsHeader, "streamAgeToggle", "Age / Lifetime", false, "stream.age");
	self:addToggle(streamsHeader, "streamCustom1Toggle", "Custom 1", false, "stream.custom1");
	self:addToggle(streamsHeader, "streamCustom2Toggle", "Custom 2", false, "stream.custom2");
	self:addDropdown(streamsHeader, "customDataModeDropdown", "Custom Data Mode", {"None", "Vector", "Colour", "Curve Sample", "Script Value"}, 0, "stream.customDataMode");
end

function ParticleEditor:buildLifetimeTab()
	local colourHeader = self:addHeader(self.lifetimeTab, "Colour / Alpha Over Lifetime");
	self:addColour(colourHeader, "colourStartPicker", "Start", "life.colour.start");
	self:addColour(colourHeader, "colourMidPicker", "Middle", "life.colour.mid");
	self:addColour(colourHeader, "colourEndPicker", "End", "life.colour.end");
	self:addSlider(colourHeader, "alphaStartSlider", "Alpha Start", 0.0, 1.0, 1.0, "life.alpha.start");
	self:addSlider(colourHeader, "alphaMidSlider", "Alpha Middle", 0.0, 1.0, 1.0, "life.alpha.mid");
	self:addSlider(colourHeader, "alphaEndSlider", "Alpha End", 0.0, 1.0, 0.0, "life.alpha.end");
	self:addDropdown(colourHeader, "gradientPresetDropdown", "Gradient Preset", {"Custom", "Fade Out", "Fire", "Smoke", "Magic", "Spark", "Electric", "Water"}, 0, "life.gradientPreset");

	local sizeHeader = self:addHeader(self.lifetimeTab, "Size Over Lifetime");
	self:addDropdown(sizeHeader, "sizeCurveDropdown", "Size Curve", {"Constant", "Linear", "Ease In", "Ease Out", "Bell", "Pulse", "Custom"}, 1, "life.size.curve");
	self:addSlider(sizeHeader, "sizeStartSlider", "Size Start", 0.0, 100.0, 1.0, "life.size.start");
	self:addSlider(sizeHeader, "sizeMidSlider", "Size Middle", 0.0, 100.0, 1.0, "life.size.mid");
	self:addSlider(sizeHeader, "sizeEndSlider", "Size End", 0.0, 100.0, 0.0, "life.size.end");
	self:addSlider(sizeHeader, "sizeXMultiplierSlider", "X Multiplier", 0.0, 10.0, 1.0, "life.size.x");
	self:addSlider(sizeHeader, "sizeYMultiplierSlider", "Y Multiplier", 0.0, 10.0, 1.0, "life.size.y");
	self:addSlider(sizeHeader, "sizeZMultiplierSlider", "Z Multiplier", 0.0, 10.0, 1.0, "life.size.z");

	local velocityHeader = self:addHeader(self.lifetimeTab, "Velocity / Rotation Over Lifetime");
	self:addSlider(velocityHeader, "velocityLifeXSlider", "Velocity X", -1000.0, 1000.0, 0.0, "life.velocity.x");
	self:addSlider(velocityHeader, "velocityLifeYSlider", "Velocity Y", -1000.0, 1000.0, 0.0, "life.velocity.y");
	self:addSlider(velocityHeader, "velocityLifeZSlider", "Velocity Z", -1000.0, 1000.0, 0.0, "life.velocity.z");
	self:addSlider(velocityHeader, "speedModifierSlider", "Speed Modifier", -10.0, 10.0, 0.0, "life.speedModifier");
	self:addSlider(velocityHeader, "angularVelocityXSlider", "Angular Velocity X", -1440.0, 1440.0, 0.0, "life.angularVelocity.x");
	self:addSlider(velocityHeader, "angularVelocityYSlider", "Angular Velocity Y", -1440.0, 1440.0, 0.0, "life.angularVelocity.y");
	self:addSlider(velocityHeader, "angularVelocityZSlider", "Angular Velocity Z", -1440.0, 1440.0, 0.0, "life.angularVelocity.z");

	local textureHeader = self:addHeader(self.lifetimeTab, "Texture Sheet Animation");
	self:addDropdown(textureHeader, "textureAnimationModeDropdown", "Mode", {"Whole Sheet", "Single Row", "Random Row", "Custom Frames"}, 0, "life.texture.mode");
	self:addSlider(textureHeader, "textureStartFrameSlider", "Start Frame", 0, 1024, 0, "life.texture.startFrame");
	self:addSlider(textureHeader, "textureCyclesSlider", "Cycles", 0.0, 20.0, 1.0, "life.texture.cycles");
	self:addSlider(textureHeader, "textureFrameBlendSlider", "Frame Blend", 0.0, 1.0, 0.0, "life.texture.frameBlend");
end

function ParticleEditor:buildPhysicsTab()
	local forcesHeader = self:addHeader(self.physicsTab, "Forces");
	self:addSlider(forcesHeader, "gravityXSlider", "Gravity X", -100.0, 100.0, 0.0, "physics.gravity.x");
	self:addSlider(forcesHeader, "gravityYSlider", "Gravity Y", -100.0, 100.0, -9.81, "physics.gravity.y");
	self:addSlider(forcesHeader, "gravityZSlider", "Gravity Z", -100.0, 100.0, 0.0, "physics.gravity.z");
	self:addSlider(forcesHeader, "forceXSlider", "Force X", -1000.0, 1000.0, 0.0, "physics.force.x");
	self:addSlider(forcesHeader, "forceYSlider", "Force Y", -1000.0, 1000.0, 0.0, "physics.force.y");
	self:addSlider(forcesHeader, "forceZSlider", "Force Z", -1000.0, 1000.0, 0.0, "physics.force.z");
	self:addSlider(forcesHeader, "dragSlider", "Linear Drag", 0.0, 100.0, 0.0, "physics.drag");
	self:addSlider(forcesHeader, "dampingSlider", "Damping", 0.0, 1.0, 0.0, "physics.damping");

	local noiseHeader = self:addHeader(self.physicsTab, "Noise / Turbulence");
	self:addDropdown(noiseHeader, "noiseQualityDropdown", "Quality", {"Low", "Medium", "High", "Curl", "Texture 3D"}, 1, "noise.quality");
	self:addSlider(noiseHeader, "noiseStrengthSlider", "Strength", 0.0, 1000.0, 0.0, "noise.strength");
	self:addSlider(noiseHeader, "noiseFrequencySlider", "Frequency", 0.0, 100.0, 1.0, "noise.frequency");
	self:addSlider(noiseHeader, "noiseScrollXSlider", "Scroll X", -100.0, 100.0, 0.0, "noise.scroll.x");
	self:addSlider(noiseHeader, "noiseScrollYSlider", "Scroll Y", -100.0, 100.0, 0.0, "noise.scroll.y");
	self:addSlider(noiseHeader, "noiseScrollZSlider", "Scroll Z", -100.0, 100.0, 0.0, "noise.scroll.z");
	self:addSlider(noiseHeader, "noiseOctavesSlider", "Octaves", 1, 8, 1, "noise.octaves");
	self:addSlider(noiseHeader, "noiseRemapMinSlider", "Remap Min", -1.0, 1.0, -1.0, "noise.remapMin");
	self:addSlider(noiseHeader, "noiseRemapMaxSlider", "Remap Max", -1.0, 1.0, 1.0, "noise.remapMax");

	local collisionHeader = self:addHeader(self.physicsTab, "Collision");
	self:addDropdown(collisionHeader, "collisionModeDropdown", "Mode", {"None", "World Geometry", "Depth Buffer", "Physics Scene", "Signed Distance Field", "Custom Callback"}, 0, "collision.mode");
	self:addSlider(collisionHeader, "collisionRadiusSlider", "Particle Radius", 0.0, 10.0, 0.1, "collision.radius");
	self:addSlider(collisionHeader, "collisionDampenSlider", "Dampen", 0.0, 1.0, 0.2, "collision.dampen");
	self:addSlider(collisionHeader, "collisionBounceSlider", "Bounce", 0.0, 1.0, 0.5, "collision.bounce");
	self:addSlider(collisionHeader, "collisionLifetimeLossSlider", "Lifetime Loss", 0.0, 1.0, 0.0, "collision.lifetimeLoss");
	self:addToggle(collisionHeader, "collisionKillToggle", "Kill On Collision", false, "collision.kill");
	self:addToggle(collisionHeader, "collisionSendEventsToggle", "Send Collision Events", false, "collision.sendEvents");

	local triggerHeader = self:addHeader(self.physicsTab, "Triggers / Zones");
	self:addDropdown(triggerHeader, "triggerModeDropdown", "Mode", {"None", "Inside", "Outside", "Enter", "Exit"}, 0, "trigger.mode");
	self:addDropdown(triggerHeader, "triggerActionDropdown", "Action", {"Ignore", "Kill", "Callback", "Spawn Sub Emitter", "Change Colour", "Change Velocity"}, 0, "trigger.action");
	self:addTextEntry(triggerHeader, "triggerLayerEntry", "Trigger Layer", "Particles", "trigger.layer");
end

function ParticleEditor:buildSubEmittersTab()
	local subHeader = self:addHeader(self.subEmittersTab, "Sub Emitters");
	self:addDropdown(subHeader, "birthSubEmitterDropdown", "On Birth", {"None", "Spark", "Smoke Puff", "Flash", "Custom"}, 0, "sub.birth");
	self:addDropdown(subHeader, "deathSubEmitterDropdown", "On Death", {"None", "Spark", "Smoke Puff", "Explosion", "Custom"}, 0, "sub.death");
	self:addDropdown(subHeader, "collisionSubEmitterDropdown", "On Collision", {"None", "Spark", "Dust", "Debris", "Custom"}, 0, "sub.collision");
	self:addSlider(subHeader, "subEmitterProbabilitySlider", "Spawn Probability", 0.0, 1.0, 1.0, "sub.probability");
	self:addToggle(subHeader, "inheritSubEmitterColourToggle", "Inherit Colour", true, "sub.inheritColour");
	self:addToggle(subHeader, "inheritSubEmitterVelocityToggle", "Inherit Velocity", true, "sub.inheritVelocity");
	self:addButton(subHeader, "addSubEmitterButton", "Add Sub Emitter", "addSubEmitter");
	self:addButton(subHeader, "removeSubEmitterButton", "Remove Sub Emitter", "removeSubEmitter");

	local trailsHeader = self:addHeader(self.subEmittersTab, "Trails / Ribbons");
	self:addToggle(trailsHeader, "trailsEnabledToggle", "Trails Enabled", false, "trails.enabled");
	self:addDropdown(trailsHeader, "trailModeDropdown", "Trail Mode", {"Per Particle", "Ribbon Strip", "Connected Path", "Camera Facing"}, 0, "trails.mode");
	self:addSlider(trailsHeader, "trailLifetimeSlider", "Trail Lifetime", 0.01, 60.0, 1.0, "trails.lifetime");
	self:addSlider(trailsHeader, "trailWidthSlider", "Trail Width", 0.001, 20.0, 0.25, "trails.width");
	self:addSlider(trailsHeader, "trailMinVertexDistanceSlider", "Min Vertex Distance", 0.001, 10.0, 0.1, "trails.minVertexDistance");
	self:addColour(trailsHeader, "trailColourPicker", "Trail Colour", "trails.colour");
	self:addToggle(trailsHeader, "trailTextureTilingToggle", "Texture Tiling", true, "trails.textureTiling");

	local lightsHeader = self:addHeader(self.subEmittersTab, "Particle Lights");
	self:addToggle(lightsHeader, "lightsEnabledToggle", "Lights Enabled", false, "lights.enabled");
	self:addSlider(lightsHeader, "lightRatioSlider", "Light Ratio", 0.0, 1.0, 0.1, "lights.ratio");
	self:addSlider(lightsHeader, "lightRangeSlider", "Range", 0.0, 100.0, 5.0, "lights.range");
	self:addSlider(lightsHeader, "lightIntensitySlider", "Intensity", 0.0, 100.0, 1.0, "lights.intensity");
	self:addColour(lightsHeader, "lightColourPicker", "Light Colour", "lights.colour");
	self:addToggle(lightsHeader, "lightUseParticleColourToggle", "Use Particle Colour", true, "lights.useParticleColour");
end

function ParticleEditor:buildPerformanceTab()
	local lodHeader = self:addHeader(self.performanceTab, "LOD");
	self:addToggle(lodHeader, "lodEnabledToggle", "LOD Enabled", true, "lod.enabled");
	self:addSlider(lodHeader, "lod0DistanceSlider", "LOD0 Distance", 0.0, 10000.0, 25.0, "lod.lod0Distance");
	self:addSlider(lodHeader, "lod1DistanceSlider", "LOD1 Distance", 0.0, 10000.0, 75.0, "lod.lod1Distance");
	self:addSlider(lodHeader, "lod2DistanceSlider", "LOD2 Distance", 0.0, 10000.0, 200.0, "lod.lod2Distance");
	self:addSlider(lodHeader, "lod1EmissionScaleSlider", "LOD1 Emission Scale", 0.0, 1.0, 0.5, "lod.lod1EmissionScale");
	self:addSlider(lodHeader, "lod2EmissionScaleSlider", "LOD2 Emission Scale", 0.0, 1.0, 0.1, "lod.lod2EmissionScale");
	self:addToggle(lodHeader, "lodDisableLightsToggle", "Disable Lights On LOD1+", true, "lod.disableLights");
	self:addToggle(lodHeader, "lodDisableCollisionToggle", "Disable Collision On LOD1+", true, "lod.disableCollision");

	local perfHeader = self:addHeader(self.performanceTab, "Performance Budgets");
	self:addDropdown(perfHeader, "simulationBackendDropdown", "Simulation Backend", {"CPU", "GPU Compute", "GPU Transform Feedback", "Hybrid", "Auto"}, 0, "perf.backend");
	self:addSlider(perfHeader, "cpuBudgetSlider", "CPU Budget ms", 0.0, 33.0, 2.0, "perf.cpuBudgetMs");
	self:addSlider(perfHeader, "gpuBudgetSlider", "GPU Budget ms", 0.0, 33.0, 2.0, "perf.gpuBudgetMs");
	self:addSlider(perfHeader, "drawCallBudgetSlider", "Draw Call Budget", 1, 1000, 8, "perf.drawCallBudget");
	self:addSlider(perfHeader, "overdrawBudgetSlider", "Overdraw Budget", 0.0, 100.0, 10.0, "perf.overdrawBudget");
	self:addToggle(perfHeader, "batchingToggle", "Batch Compatible Systems", true, "perf.batching");
	self:addToggle(perfHeader, "gpuInstancingToggle", "GPU Instancing", true, "perf.gpuInstancing");
	self:addToggle(perfHeader, "boundsAutoUpdateToggle", "Auto Bounds", true, "perf.autoBounds");
	self:addSlider(perfHeader, "boundsPaddingSlider", "Bounds Padding", 0.0, 1000.0, 5.0, "perf.boundsPadding");

	local platformHeader = self:addHeader(self.performanceTab, "Platform Quality");
	self:addDropdown(platformHeader, "qualityProfileDropdown", "Quality Profile", {"Ultra", "High", "Medium", "Low", "Mobile", "Switch/Handheld", "Custom"}, 2, "quality.profile");
	self:addSlider(platformHeader, "mobileParticleScaleSlider", "Mobile Particle Scale", 0.0, 1.0, 0.5, "quality.mobileParticleScale");
	self:addSlider(platformHeader, "lowEndParticleScaleSlider", "Low-End Particle Scale", 0.0, 1.0, 0.25, "quality.lowEndParticleScale");
	self:addToggle(platformHeader, "allowHalfResolutionToggle", "Allow Half Resolution", true, "quality.allowHalfResolution");
	self:addToggle(platformHeader, "stripEditorDataToggle", "Strip Editor Data On Build", true, "quality.stripEditorData");
end

function ParticleEditor:buildPreviewTab()
	local previewHeader = self:addHeader(self.previewTab, "Preview Scene");
	self:addDropdown(previewHeader, "previewSceneDropdown", "Scene", {"Empty", "Ground Plane", "Indoor Room", "Outdoor Sun", "Dark HDRI", "Gameplay Camera", "Custom"}, 1, "preview.scene");
	self:addDropdown(previewHeader, "previewCameraDropdown", "Camera", {"Orbit", "Game Camera", "Front", "Top", "Side", "Custom"}, 0, "preview.camera");
	self:addSlider(previewHeader, "previewCameraDistanceSlider", "Camera Distance", 0.1, 1000.0, 10.0, "preview.cameraDistance");
	self:addSlider(previewHeader, "previewTimeScaleSlider", "Time Scale", 0.0, 4.0, 1.0, "preview.timeScale");
	self:addToggle(previewHeader, "previewGridToggle", "Show Grid", true, "preview.showGrid");
	self:addToggle(previewHeader, "previewBoundsToggle", "Show Bounds", true, "preview.showBounds");
	self:addToggle(previewHeader, "previewVelocityToggle", "Show Velocity Vectors", false, "preview.showVelocity");
	self:addToggle(previewHeader, "previewOverdrawToggle", "Show Overdraw", false, "preview.showOverdraw");
	self:addToggle(previewHeader, "previewWireframeToggle", "Wireframe", false, "preview.wireframe");

	local previewActionsHeader = self:addHeader(self.previewTab, "Preview Actions");
	self:addButton(previewActionsHeader, "focusPreviewButton", "Frame System", "previewFrameSystem");
	self:addButton(previewActionsHeader, "resetPreviewCameraButton", "Reset Camera", "previewResetCamera");
	self:addButton(previewActionsHeader, "captureThumbnailButton", "Capture Thumbnail", "previewCaptureThumbnail");
	self:addButton(previewActionsHeader, "captureGifButton", "Capture GIF", "previewCaptureGif");
	self:addButton(previewActionsHeader, "spawnTestImpactButton", "Spawn Test Impact", "previewSpawnImpact");
	self:addButton(previewActionsHeader, "clearPreviewButton", "Clear Preview", "previewClear");

	local authoringHeader = self:addHeader(self.previewTab, "Authoring Helpers");
	self:addTextEntry(authoringHeader, "notesEntry", "Notes", "", "asset.notes");
	self:addDropdown(authoringHeader, "usageTagDropdown", "Usage Tag", {"Gameplay", "Environment", "UI", "Cinematic", "Weapon", "Vehicle", "Weather", "Magic", "Debug"}, 0, "asset.usageTag");
	self:addDropdown(authoringHeader, "scaleReferenceDropdown", "Scale Reference", {"Human", "Vehicle", "Room", "Weapon", "World", "Custom"}, 0, "asset.scaleReference");
end

function ParticleEditor:buildAssetTab()
	local assetHeader = self:addHeader(self.assetTab, "Asset");
	self:addTextEntry(assetHeader, "assetPathEntry", "Particle Asset", "", "asset.path");
	self:addTextEntry(assetHeader, "materialOverrideEntry", "Material Override", "", "asset.materialOverride");
	self:addTextEntry(assetHeader, "meshOverrideEntry", "Mesh Override", "", "asset.meshOverride");
	self:addTextEntry(assetHeader, "textureOverrideEntry", "Texture Override", "", "asset.textureOverride");
	self:addDropdown(assetHeader, "assetVersionDropdown", "Asset Version", {"Working", "Preview", "Approved", "Deprecated"}, 0, "asset.versionState");
	self:addButton(assetHeader, "newAssetButton", "New", "assetNew");
	self:addButton(assetHeader, "loadAssetButton", "Load", "assetLoad");
	self:addButton(assetHeader, "saveAssetButton", "Save", "assetSave");
	self:addButton(assetHeader, "saveAsAssetButton", "Save As", "assetSaveAs");
	self:addButton(assetHeader, "revertAssetButton", "Revert", "assetRevert");

	local importExportHeader = self:addHeader(self.assetTab, "Import / Export");
	self:addDropdown(importExportHeader, "exportFormatDropdown", "Export Format", {"Engine JSON", "Ogre Particle Script", "Unity-like JSON", "Binary Runtime", "CSV Debug"}, 0, "asset.exportFormat");
	self:addButton(importExportHeader, "importButton", "Import", "assetImport");
	self:addButton(importExportHeader, "exportButton", "Export", "assetExport");
	self:addButton(importExportHeader, "buildRuntimeButton", "Build Runtime Asset", "assetBuildRuntime");
	self:addButton(importExportHeader, "validateAssetButton", "Validate", "assetValidate");
	self:addButton(importExportHeader, "findReferencesButton", "Find References", "assetFindReferences");

	local presetsHeader = self:addHeader(self.assetTab, "Presets");
	self:addDropdown(presetsHeader, "presetDropdown", "Preset", {"Fire Small", "Fire Large", "Smoke Soft", "Explosion", "Muzzle Flash", "Sparks", "Magic Aura", "Rain", "Snow", "Dust Trail", "Custom"}, 0, "asset.preset");
	self:addButton(presetsHeader, "applyPresetButton", "Apply Preset", "presetApply");
	self:addButton(presetsHeader, "savePresetButton", "Save Preset", "presetSave");
	self:addButton(presetsHeader, "deletePresetButton", "Delete Preset", "presetDelete");
end

function ParticleEditor:buildDebugTab()
	local profilerHeader = self:addHeader(self.debugTab, "Profiler");
	self:addToggle(profilerHeader, "profilerEnabledToggle", "Profiler Enabled", true, "debug.profilerEnabled");
	self:addToggle(profilerHeader, "showRuntimeStatsToggle", "Show Runtime Stats", true, "debug.showRuntimeStats");
	self:addToggle(profilerHeader, "showSpawnEventsToggle", "Show Spawn Events", false, "debug.showSpawnEvents");
	self:addToggle(profilerHeader, "showCollisionEventsToggle", "Show Collision Events", false, "debug.showCollisionEvents");
	self:addToggle(profilerHeader, "showGpuBufferToggle", "Show GPU Buffers", false, "debug.showGpuBuffers");
	self:addButton(profilerHeader, "profileFrameButton", "Profile Frame", "debugProfileFrame");
	self:addButton(profilerHeader, "profileTenSecondsButton", "Profile 10 Seconds", "debugProfileTenSeconds");
	self:addButton(profilerHeader, "dumpStateButton", "Dump State", "debugDumpState");
	self:addButton(profilerHeader, "resetCountersButton", "Reset Counters", "debugResetCounters");

	local validationHeader = self:addHeader(self.debugTab, "Validation");
	self:addToggle(validationHeader, "validateOnSaveToggle", "Validate On Save", true, "debug.validateOnSave");
	self:addToggle(validationHeader, "warnOverdrawToggle", "Warn Overdraw", true, "debug.warnOverdraw");
	self:addToggle(validationHeader, "warnMissingAssetsToggle", "Warn Missing Assets", true, "debug.warnMissingAssets");
	self:addToggle(validationHeader, "warnMobileCostToggle", "Warn Mobile Cost", true, "debug.warnMobileCost");
	self:addSlider(validationHeader, "warningParticleCountSlider", "Warning Particle Count", 1, 200000, 10000, "debug.warningParticleCount");
	self:addSlider(validationHeader, "warningOverdrawSlider", "Warning Overdraw", 0.0, 100.0, 20.0, "debug.warningOverdraw");

	local rawHeader = self:addHeader(self.debugTab, "Raw Commands");
	self:addTextEntry(rawHeader, "rawCommandEntry", "Command", "", "debug.rawCommand");
	self:addButton(rawHeader, "executeCommandButton", "Execute", "debugExecuteCommand");
	self:addButton(rawHeader, "copyJsonButton", "Copy JSON", "debugCopyJson");
	self:addButton(rawHeader, "pasteJsonButton", "Paste JSON", "debugPasteJson");

	self.debugStatsLabel = self:addLabel(self.debugTab, "debugStatsLabel", "CPU: -- ms   GPU: -- ms   Alive: --   Spawned/s: --   Overdraw: --");
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Selection / updates
-- ─────────────────────────────────────────────────────────────────────────────

function ParticleEditor:updateSelection()
	print("ParticleEditor updateSelection called");

	self.selectedParticleComponent = nil;
	self.selectedActor = nil;

	local applicationManager = IApplicationManager.instance();
	local selectionManager = applicationManager:getSelectionManager();
	if selectionManager == nil then
		return;
	end

	local selection = selectionManager:getSelection();
	if selection == nil then
		return;
	end

	local particleTypeInfo = ParticleSystem.typeInfo();
	local actorTypeInfo = nil;
	pcall(function() actorTypeInfo = IActor.typeInfo(); end);

	local size = selection:size();
	for i = 0, size - 1 do
		local item = selection:at(i);
		if item ~= nil and item:derived(particleTypeInfo) then
			self.selectedParticleComponent = item;
			print("ParticleEditor selected ParticleSystem component");
			break;
		elseif item ~= nil and actorTypeInfo ~= nil and item:derived(actorTypeInfo) then
			self.selectedActor = item;
			local ok, component = safeCall(item, "getComponent", "ParticleSystem");
			if ok and component ~= nil then
				self.selectedParticleComponent = component;
				print("ParticleEditor selected actor ParticleSystem component");
				break;
			end
		end
	end

	if self.selectedParticleComponent == nil and size > 0 then
		local item = selection:at(0);
		print("ParticleEditor selected item type: " .. tostring(item:getTypeInfo()));
	end

	self:syncFromComponent();
end

function ParticleEditor:ensureParticleComponent()
	if self.selectedParticleComponent ~= nil then
		return self.selectedParticleComponent;
	end

	self:updateSelection();
	if self.selectedParticleComponent ~= nil then
		return self.selectedParticleComponent;
	end

	if self.selectedActor ~= nil then
		local ok, component = safeCall(self.selectedActor, "addComponent", "ParticleSystem");
		if ok and component ~= nil then
			self.selectedParticleComponent = component;
			self:applyAllSettings();
			return component;
		end
	end

	return nil;
end

function ParticleEditor:setControlValue(control, value)
	if control == nil or value == nil then
		return;
	end

	if safeCall(control, "setValue", value) then
		return;
	end

	if safeCall(control, "setSelectedOption", value) then
		return;
	end

	if safeCall(control, "setText", tostring(value)) then
		return;
	end
end

function ParticleEditor:setSettingAndControl(controlName, key, value)
	self.settings[key] = value;
	self:setControlValue(self[controlName], value);
end

function ParticleEditor:syncFromComponent()
	local component = self.selectedParticleComponent;
	if component == nil then
		return;
	end

	self.settings["system.playOnAwake"] = safeGet(component, "getPlayOnLoad", self.settings["system.playOnAwake"]);
	self.settings["system.looping"] = safeGet(component, "isLooping", self.settings["system.looping"]);
	self.settings["system.duration"] = safeGet(component, "getDuration", self.settings["system.duration"]);
	self.settings["emitter.rateOverTime"] = safeGet(component, "getRate", self.settings["emitter.rateOverTime"]);
	self.settings["emitter.lifetime"] = safeGet(component, "getLifetime", self.settings["emitter.lifetime"]);
	self.settings["emitter.startSpeed"] = safeGet(component, "getShapeSize", self.settings["emitter.startSpeed"]);
	self.settings["emitter.speedRandom"] = safeGet(component, "getShapeSizeVariance", self.settings["emitter.speedRandom"]);
	self.settings["emitter.shape.angle"] = safeGet(component, "getAngle", self.settings["emitter.shape.angle"]);
	self.settings["emitter.spread"] = safeGet(component, "getAngleVariance", self.settings["emitter.spread"]);
	self.settings["emitter.type"] = safeGet(component, "getShapeType", self.settings["emitter.type"]);

	self:setControlValue(self.playOnAwakeToggle, self.settings["system.playOnAwake"]);
	self:setControlValue(self.loopingToggle, self.settings["system.looping"]);
	self:setControlValue(self.durationSlider, self.settings["system.duration"]);
	self:setControlValue(self.rateOverTimeSlider, self.settings["emitter.rateOverTime"]);
	self:setControlValue(self.lifetimeSlider, self.settings["emitter.lifetime"]);
	self:setControlValue(self.velocitySlider, self.settings["emitter.startSpeed"]);
	self:setControlValue(self.velocityRandomSlider, self.settings["emitter.speedRandom"]);
	self:setControlValue(self.shapeAngleSlider, self.settings["emitter.shape.angle"]);
	self:setControlValue(self.spreadSlider, self.settings["emitter.spread"]);
	self:setControlValue(self.emitterTypeDropdown, self.settings["emitter.type"]);

	self:updateStatsLabel();
end

function ParticleEditor:updateSelectedMaterial()
	print("ParticleEditor updateSelectedMaterial called");
	self:performAction("updateSelectedMaterial");
end

function ParticleEditor:applySettingToComponent(key, value)
	local component = self.selectedParticleComponent;
	if component == nil then
		return;
	end

	if key == "system.playOnAwake" then
		safeCall(component, "setPlayOnLoad", value == true);
	elseif key == "system.looping" then
		safeCall(component, "setLooping", value == true);
	elseif key == "system.duration" then
		safeCall(component, "setDuration", value or 0.0);
	elseif key == "system.template" then
		local templateName = ParticleEditorTemplateNames[value or 0] or "";
		safeCall(component, "setTemplateName", templateName);
		safeCall(component, "rebuild");
	elseif key == "asset.path" then
		safeCall(component, "setTemplateName", value or "");
		safeCall(component, "rebuild");
	elseif key == "emitter.rateOverTime" then
		safeCall(component, "setRate", value or 0.0);
	elseif key == "emitter.lifetime" or key == "emitter.lifetimeRandom" then
		local lifetime = self.settings["emitter.lifetime"] or 0.0;
		local random = clamp(self.settings["emitter.lifetimeRandom"] or 0.0, 0.0, 1.0);
		safeCall(component, "setLifetime", lifetime);
		local range = safeVector2(lifetime * (1.0 - random), lifetime);
		if range then
			safeCall(component, "setStartLifetime", range);
		end
	elseif key == "emitter.startSize" or key == "emitter.startSizeRandom" then
		local size = self.settings["emitter.startSize"] or 0.0;
		local random = clamp(self.settings["emitter.startSizeRandom"] or 0.0, 0.0, 1.0);
		local range = safeVector2(size * (1.0 - random), size);
		if range then
			safeCall(component, "setStartSize", range);
		end
	elseif key == "emitter.startSpeed" or key == "emitter.speedRandom" or key == "emitter.shape.radius" then
		local speed = self.settings["emitter.startSpeed"] or self.settings["emitter.shape.radius"] or 0.0;
		local random = clamp(self.settings["emitter.speedRandom"] or 0.0, 0.0, 1.0);
		safeCall(component, "setShapeSize", speed);
		safeCall(component, "setShapeSizeVariance", speed * random);
	elseif key == "emitter.shape.angle" then
		safeCall(component, "setAngle", value or 0.0);
	elseif key == "emitter.spread" then
		safeCall(component, "setAngleVariance", value or 0.0);
	elseif key == "emitter.type" then
		safeCall(component, "setShapeType", value or 0);
	elseif key == "life.size.x" or key == "life.size.y" or key == "life.size.z" then
		local scale = safeVector3(self.settings["life.size.x"] or 1.0, self.settings["life.size.y"] or 1.0, self.settings["life.size.z"] or 1.0);
		if scale then
			safeCall(component, "setScale", scale);
		end
	elseif key == "renderer.material" or key == "asset.materialOverride" then
		local ok, renderSystem = safeCall(component, "getParticleSystem");
		if ok and renderSystem ~= nil and value ~= nil and value ~= "" then
			safeCall(renderSystem, "setMaterialName", value);
		end
	end
end

function ParticleEditor:applyAllSettings()
	if self.selectedParticleComponent == nil then
		return;
	end

	for key, value in pairs(self.settings) do
		self:applySettingToComponent(key, value);
	end
end

function ParticleEditor:onSettingChanged(key, value)
	print("ParticleEditor setting changed: " .. tostring(key) .. " = " .. tostring(value));

	if key == "emitter.selected" then
		self.selectedEmitterIndex = value + 1;
	elseif key == "system.maxParticles" or key == "emitter.rateOverTime" or key == "emitter.lifetime" then
		self:updateStatsLabel();
	end

	if self.selectedParticleComponent == nil then
		self:ensureParticleComponent();
	end

	self:applySettingToComponent(key, value);

	if self.settings["system.liveUpdate"] then
		self:performAction("liveUpdate");
	end
end

function ParticleEditor:updateStatsLabel()
	local maxParticles = self.settings["system.maxParticles"] or 0;
	local rate = self.settings["emitter.rateOverTime"] or 0;
	local lifetime = self.settings["emitter.lifetime"] or 0;
	local estimatedAlive = math.min(maxParticles, rate * lifetime);
	local drawCalls = 1;

	if self.settings["trails.enabled"] then
		drawCalls = drawCalls + 1;
	end
	if self.settings["lights.enabled"] then
		drawCalls = drawCalls + 1;
	end

	if self.systemStatsLabel then
		self.systemStatsLabel:setLabel(string.format("Alive Est: %.0f   Rate: %.1f/s   Lifetime: %.2fs   Draw Calls Est: %d", estimatedAlive, rate, lifetime, drawCalls));
	end

	if self.debugStatsLabel then
		self.debugStatsLabel:setLabel(string.format("CPU: -- ms   GPU: -- ms   Max: %d   Spawned/s: %.1f   Draw Calls Est: %d", maxParticles, rate, drawCalls));
	end
end

function ParticleEditor:updateEmitterSummary()
	if #self.emitters == 0 then
		table.insert(self.emitters, { name = "Emitter 1", type = "Point" });
	end

	if self.emitterSummaryLabel then
		self.emitterSummaryLabel:setLabel("Emitters: " .. tostring(#self.emitters) .. "   Selected: " .. tostring(self.selectedEmitterIndex));
	end
end

function ParticleEditor:updateAffectorSummary()
	if self.affectorSummaryLabel then
		self.affectorSummaryLabel:setLabel("Affectors: " .. tostring(#self.affectors));
	end
end

function ParticleEditor:performAction(actionName)
	print("ParticleEditor action: " .. tostring(actionName));
	table.insert(self.actions, actionName);

	if actionName == "play" then
		self:ensureParticleComponent();
		self:applyAllSettings();
		if self.selectedParticleComponent then
			safeCall(self.selectedParticleComponent, "play");
		end
		self.isPlaying = true;
		self.isPaused = false;
	elseif actionName == "pause" then
		if self.selectedParticleComponent then
			safeCall(self.selectedParticleComponent, "pause");
		end
		self.isPaused = true;
	elseif actionName == "resume" then
		if self.selectedParticleComponent then
			safeCall(self.selectedParticleComponent, "resume");
		end
		self.isPlaying = true;
		self.isPaused = false;
	elseif actionName == "stop" then
		if self.selectedParticleComponent then
			safeCall(self.selectedParticleComponent, "stop");
		end
		self.isPlaying = false;
		self.isPaused = false;
		self.previewTime = 0.0;
	elseif actionName == "restart" then
		self:ensureParticleComponent();
		if self.selectedParticleComponent then
			self:applyAllSettings();
			safeCall(self.selectedParticleComponent, "rebuild");
			safeCall(self.selectedParticleComponent, "play");
		end
		self.isPlaying = true;
		self.isPaused = false;
		self.previewTime = 0.0;
	elseif actionName == "liveUpdate" then
		if self.selectedParticleComponent then
			self:applyAllSettings();
		end
	elseif actionName == "singleStep" then
		if self.selectedParticleComponent then
			safeCall(self.selectedParticleComponent, "pause");
		end
		self.isPaused = true;
	elseif actionName == "addEmitter" then
		table.insert(self.emitters, { name = "Emitter " .. tostring(#self.emitters + 1), type = "Point" });
		self.selectedEmitterIndex = #self.emitters;
		self:updateEmitterSummary();
	elseif actionName == "duplicateEmitter" then
		local src = self.emitters[self.selectedEmitterIndex] or { name = "Emitter", type = "Point" };
		table.insert(self.emitters, { name = src.name .. " Copy", type = src.type });
		self.selectedEmitterIndex = #self.emitters;
		self:updateEmitterSummary();
	elseif actionName == "removeEmitter" then
		if #self.emitters > 1 then
			table.remove(self.emitters, self.selectedEmitterIndex);
			if self.selectedEmitterIndex > #self.emitters then
				self.selectedEmitterIndex = #self.emitters;
			end
		end
		self:updateEmitterSummary();
	elseif actionName == "addAffector" then
		table.insert(self.affectors, { name = "Affector " .. tostring(#self.affectors + 1) });
		self:updateAffectorSummary();
	elseif actionName == "removeAffector" then
		if #self.affectors > 0 then
			table.remove(self.affectors, #self.affectors);
		end
		self:updateAffectorSummary();
	elseif actionName == "debugResetCounters" then
		self.previewTime = 0.0;
		self:updateStatsLabel();
	elseif actionName == "presetApply" then
		self:applyPreset(self.settings["asset.preset"] or 0);
	elseif actionName == "assetNew" then
		self:ensureParticleComponent();
		if self.selectedParticleComponent then
			safeCall(self.selectedParticleComponent, "setTemplateName", "");
			safeCall(self.selectedParticleComponent, "rebuild");
			self:applyAllSettings();
		end
	elseif actionName == "assetLoad" or actionName == "assetImport" then
		self:loadSettingsFromFile(self.settings["asset.path"]);
	elseif actionName == "assetSave" or actionName == "assetSaveAs" or actionName == "assetExport" then
		self:saveSettingsToFile(self.settings["asset.path"]);
	elseif actionName == "assetValidate" then
		self:validateSettings();
	elseif actionName == "assetBuildRuntime" then
		self:ensureParticleComponent();
		if self.selectedParticleComponent then
			self:applyAllSettings();
			safeCall(self.selectedParticleComponent, "rebuild");
		end
	end
end

function ParticleEditor:applyPreset(presetIndex)
	print("ParticleEditor applyPreset: " .. tostring(presetIndex));

	-- The dropdown is intentionally index-based to match the rest of the engine UI.
	-- Keep this light: it updates the UI controls that are most useful for previews.
	if presetIndex == 1 then -- Fire Small
		self:setSettingAndControl("rateOverTimeSlider", "emitter.rateOverTime", 250.0);
		self:setSettingAndControl("lifetimeSlider", "emitter.lifetime", 1.2);
		self:setSettingAndControl("startSizeSlider", "emitter.startSize", 0.6);
		self:setSettingAndControl("velocitySlider", "emitter.startSpeed", 2.5);
	elseif presetIndex == 2 then -- Fire Large
		self:setSettingAndControl("rateOverTimeSlider", "emitter.rateOverTime", 700.0);
		self:setSettingAndControl("lifetimeSlider", "emitter.lifetime", 2.5);
		self:setSettingAndControl("startSizeSlider", "emitter.startSize", 2.0);
		self:setSettingAndControl("velocitySlider", "emitter.startSpeed", 5.0);
	elseif presetIndex == 3 then -- Smoke Soft
		self:setSettingAndControl("rateOverTimeSlider", "emitter.rateOverTime", 80.0);
		self:setSettingAndControl("lifetimeSlider", "emitter.lifetime", 8.0);
		self:setSettingAndControl("startSizeSlider", "emitter.startSize", 1.5);
		self:setSettingAndControl("velocitySlider", "emitter.startSpeed", 1.5);
	elseif presetIndex == 4 then -- Explosion
		self:setSettingAndControl("rateOverTimeSlider", "emitter.rateOverTime", 0.0);
		self:setSettingAndControl("burstCountSlider", "emitter.burst.count", 250.0);
		self:setSettingAndControl("lifetimeSlider", "emitter.lifetime", 2.0);
		self:setSettingAndControl("velocitySlider", "emitter.startSpeed", 35.0);
	end

	self:updateStatsLabel();
	self:performAction("liveUpdate");
end

function ParticleEditor:validateSettings()
	local maxParticles = self.settings["system.maxParticles"] or 0;
	local rate = self.settings["emitter.rateOverTime"] or 0.0;
	local lifetime = self.settings["emitter.lifetime"] or 0.0;

	if maxParticles <= 0 or lifetime <= 0.0 then
		print("ParticleEditor validation failed: particle count and lifetime must be positive");
		return false;
	end

	if rate * lifetime > maxParticles then
		print("ParticleEditor validation warning: estimated alive particles exceeds max particles");
	end

	return true;
end

function ParticleEditor:saveSettingsToFile(path)
	if path == nil or path == "" then
		print("ParticleEditor save skipped: no asset path");
		return false;
	end

	if self.settings["debug.validateOnSave"] and not self:validateSettings() then
		return false;
	end

	local okJson, cjson = pcall(require, "cjson");
	if not okJson or cjson == nil then
		print("ParticleEditor save skipped: cjson is unavailable");
		return false;
	end

	local okEncode, encoded = pcall(function()
		return cjson.encode({ settings = self.settings, emitters = self.emitters, affectors = self.affectors });
	end);
	if not okEncode then
		print("ParticleEditor save failed: " .. tostring(encoded));
		return false;
	end

	local file = io.open(path, "w");
	if file == nil then
		print("ParticleEditor save failed: could not open " .. tostring(path));
		return false;
	end

	file:write(encoded);
	file:close();
	print("ParticleEditor saved " .. tostring(path));
	return true;
end

function ParticleEditor:loadSettingsFromFile(path)
	if path == nil or path == "" then
		print("ParticleEditor load skipped: no asset path");
		return false;
	end

	local file = io.open(path, "r");
	if file == nil then
		print("ParticleEditor load failed: could not open " .. tostring(path));
		return false;
	end

	local data = file:read("*a");
	file:close();

	local okJson, cjson = pcall(require, "cjson");
	if not okJson or cjson == nil then
		print("ParticleEditor load skipped: cjson is unavailable");
		return false;
	end

	local okDecode, decoded = pcall(function() return cjson.decode(data); end);
	if not okDecode or decoded == nil then
		print("ParticleEditor load failed: invalid JSON");
		return false;
	end

	if decoded.settings then
		for key, value in pairs(decoded.settings) do
			self.settings[key] = value;
		end
	end
	if decoded.emitters then self.emitters = decoded.emitters; end
	if decoded.affectors then self.affectors = decoded.affectors; end

	self:updateEmitterSummary();
	self:updateAffectorSummary();
	self:updateStatsLabel();
	self:ensureParticleComponent();
	self:applyAllSettings();
	print("ParticleEditor loaded " .. tostring(path));
	return true;
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Event handling
-- ─────────────────────────────────────────────────────────────────────────────

function ParticleEditor:handleEvent(parameters, results)
	print("ParticleEditor handleEvent called");

	if parameters == nil then
		print("ParticleEditor parameters nil");
		return;
	end

	local eventHash = parameters:at(1);
	local args = parameters:at(2);
	local sender = parameters:at(3);

	if sender == nil then
		return;
	end

	if eventHash == IEvent.handleValueChanged then
		local binding = self.controlBindings[sender];
		if binding then
			local value = self:readControlValue(sender, binding.kind, args);
			self.settings[binding.key] = value;
			self:writeResult(results, binding.key, value);
			self:onSettingChanged(binding.key, value);
		end
	elseif eventHash == IEvent.handleSelection then
		local actionName = self.buttonActions[sender];
		if actionName then
			self:writeResult(results, "action", actionName);
			self:performAction(actionName);
		end
	elseif eventHash == IEvent.handleDrop then
		local dataStr = self:safeArg(args, 0, nil);
		if dataStr then
			local ok, cjson = pcall(require, "cjson");
			if ok and cjson then
				local decoded = cjson.decode(dataStr);
				if decoded and decoded.filePath then
					print("ParticleEditor drop: " .. decoded.filePath);
					if self.assetPathEntry then
						self.assetPathEntry:setText(decoded.filePath);
					end
					self.settings["asset.path"] = decoded.filePath;
					self:onSettingChanged("asset.path", decoded.filePath);
				end
			end
		end
	end
end
