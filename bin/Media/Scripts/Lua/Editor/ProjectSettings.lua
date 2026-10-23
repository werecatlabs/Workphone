class 'ProjectSettings' (BaseEditor)

ProjectSettingsTypes =
{
	None = 0,

	-- Application
	ProjectName = 1,
	Company = 2,	
	ProductName = 3,
	Version = 4,
	BundleId = 5,
	DefaultLanguage = 6,
	StartupScene = 7,
	SceneList = 8,
	AutoLoadLastScene = 9,

	-- Platform / renderer selection
	ActivePlatform = 20,
	PcRenderer = 21,
	MacRenderer = 22,
	IosRenderer = 23,
	AndroidRenderer = 24,
	RendererMode = 25,
	ShaderModel = 26,
	ShaderLanguage = 27,
	UseValidationLayers = 28,
	AllowRendererFallback = 29,
	ApplyPcProfile = 30,
	ApplyMacProfile = 31,
	ApplyIosProfile = 32,
	ApplyAndroidProfile = 33,

	-- Graphics / display
	Resolution = 40,
	Fullscreen = 41,
	VSync = 42,
	FrameRateLimit = 43,
	RenderScale = 44,
	AntiAliasing = 45,
	DynamicResolution = 46,
	HDR = 47,
	ColorSpace = 48,
	TextureFiltering = 49,
	Anisotropy = 50,
	PresentMode = 51,

	-- Render pipeline / lighting
	RenderPipeline = 60,
	ShadowQuality = 61,
	ShadowDistance = 62,
	ShadowResolution = 63,
	LightingMode = 64,
	GI = 65,
	ReflectionProbes = 66,
	PostProcessing = 67,
	Bloom = 68,
	MotionBlur = 69,
	AmbientOcclusion = 70,
	Tonemapping = 71,

	-- Quality / assets
	QualityPreset = 80,
	TextureQuality = 81,
	MaxTextureSize = 82,
	MeshQuality = 83,
	LodBias = 84,
	ParticleQuality = 85,
	TerrainQuality = 86,
	StreamingBudgetMb = 87,
	AssetCompression = 88,
	TextureCompressionPC = 89,
	TextureCompressionMac = 90,
	TextureCompressionIOS = 91,
	TextureCompressionAndroid = 92,

	-- Physics
	Gravity = 110,
	SimulationRate = 111,
	SolverIterations = 112,
	CollisionLayers = 113,
	Broadphase = 114,
	ContinuousCollision = 115,
	PhysicsSubsteps = 116,
	PhysicsThreads = 117,
	DeterministicPhysics = 118,
	PhysicsDebugDraw = 119,

	-- Audio
	MasterVolume = 130,
	MusicVolume = 131,
	SfxVolume = 132,
	VoiceVolume = 133,
	Spatialization = 134,
	AudioBackend = 135,
	OutputDevice = 136,
	AudioSampleRate = 137,
	AudioBufferSize = 138,
	AudioVoiceLimit = 139,
	AudioStreaming = 140,
	AudioOcclusion = 141,

	-- Input
	InputSystem = 150,
	GamepadSupport = 151,
	TouchSupport = 152,
	MouseCapture = 153,
	InputRebinds = 154,
	GestureRecognition = 155,
	Haptics = 156,

	-- Build / packaging
	BuildConfig = 170,
	ArchitecturePC = 171,
	ArchitectureMac = 172,
	ArchitectureIOS = 173,
	ArchitectureAndroid = 174,
	MinIOSVersion = 175,
	MinAndroidSdk = 176,
	TargetAndroidSdk = 177,
	UseAndroidAppBundle = 178,
	SigningProfile = 179,
	StripSymbols = 180,
	GenerateDebugSymbols = 181,
	IncludeEditorData = 182,
	UseAssetBundles = 183,
	BuildCompression = 184,

	-- Tasks / performance
	ThreadCount = 200,
	BackgroundTasks = 201,
	TaskPriority = 202,
	JobSystem = 203,
	WorkerAffinity = 204,
	RenderThread = 205,
	GpuParticles = 206,
	OcclusionCulling = 207,
	StaticBatching = 208,
	DynamicBatching = 209,
	MemoryBudgetMb = 210,
	GpuMemoryBudgetMb = 211,

	-- Scripting / networking / services
	ScriptBackend = 230,
	LuaEnabled = 231,
	HotReloadScripts = 232,
	ReflectionMode = 233,
	NetworkBackend = 234,
	EnableMultiplayer = 235,
	CloudSave = 236,
	Analytics = 237,
	CrashReporting = 238,

	-- Diagnostics / editor
	LogLevel = 250,
	ProfilerEnabled = 251,
	GpuProfiler = 252,
	ValidationWarnings = 253,
	Telemetry = 254,
	EditorTheme = 255,
	Autosave = 256,
	AutosaveInterval = 257,
	ShowAdvancedWarnings = 258,

	-- Actions
	Apply = 1000,
	Save = 1001,
	Reset = 1002,
	Validate = 1003,
	ExportJson = 1004,
	ImportJson = 1005,
	BuildSelected = 1006,
	ReloadRenderer = 1007,
}

local ProjectSettingsDefaults =
{
	projectName = "Lioncat Project",
	company = "",
	productName = "Lioncat Game",
	version = "0.1.0",
	bundleId = "com.company.lioncatgame",
	defaultLanguage = 0,
	startupScene = "",
	sceneList = "",
	autoLoadLastScene = false,

	activePlatform = 0,
	rendererPC = 0,
	rendererMac = 1,
	rendererIOS = 1,
	rendererAndroid = 1,
	rendererMode = 0,
	shaderModel = 2,
	shaderLanguage = 0,
	useValidationLayers = false,
	allowRendererFallback = true,

	resolution = 0,
	fullscreen = false,
	vsync = true,
	frameRateLimit = 60,
	renderScale = 1.0,
	aa = 2,
	dynamicResolution = false,
	hdr = true,
	colorSpace = 1,
	textureFiltering = 1,
	anisotropy = 8,
	presentMode = 0,

	renderPipeline = 1,
	shadowQuality = 2,
	shadowDistance = 80,
	shadowResolution = 2,
	lightingMode = 1,
	gi = 0,
	reflectionProbes = true,
	postProcessing = true,
	bloom = true,
	motionBlur = false,
	ambientOcclusion = true,
	tonemapping = 1,

	qualityPreset = 2,
	textureQuality = 2,
	maxTextureSize = 4096,
	meshQuality = 2,
	lodBias = 1.0,
	particleQuality = 2,
	terrainQuality = 2,
	streamingBudgetMb = 512,
	assetCompression = 1,
	textureCompressionPC = 1,
	textureCompressionMac = 1,
	textureCompressionIOS = 2,
	textureCompressionAndroid = 3,

	gravity = -9.8,
	simulationRate = 60,
	solverIterations = 8,
	collisionLayers = "Default,Player,Enemy,World,Trigger,UI",
	broadphase = 1,
	continuousCollision = true,
	physicsSubsteps = 1,
	physicsThreads = 2,
	deterministicPhysics = false,
	physicsDebugDraw = false,

	masterVolume = 80,
	musicVolume = 60,
	sfxVolume = 70,
	voiceVolume = 80,
	spatialization = true,
	audioBackend = 0,
	outputDevice = 0,
	audioSampleRate = 48000,
	audioBufferSize = 1024,
	audioVoiceLimit = 128,
	audioStreaming = true,
	audioOcclusion = false,

	inputSystem = 0,
	gamepadSupport = true,
	touchSupport = true,
	mouseCapture = true,
	inputRebinds = true,
	gestureRecognition = true,
	haptics = true,

	buildConfig = 1,
	architecturePC = 1,
	architectureMac = 1,
	architectureIOS = 0,
	architectureAndroid = 1,
	minIOSVersion = 14,
	minAndroidSdk = 26,
	targetAndroidSdk = 35,
	useAndroidAppBundle = true,
	signingProfile = "Default",
	stripSymbols = true,
	generateDebugSymbols = true,
	includeEditorData = false,
	useAssetBundles = true,
	buildCompression = 1,

	threadCount = 4,
	backgroundTasks = true,
	taskPriority = 5,
	jobSystem = 0,
	workerAffinity = 0,
	renderThread = true,
	gpuParticles = true,
	occlusionCulling = true,
	staticBatching = true,
	dynamicBatching = true,
	memoryBudgetMb = 2048,
	gpuMemoryBudgetMb = 1024,

	scriptBackend = 0,
	luaEnabled = true,
	hotReloadScripts = true,
	reflectionMode = 1,
	networkBackend = 0,
	enableMultiplayer = false,
	cloudSave = false,
	analytics = false,
	crashReporting = true,

	logLevel = 2,
	profilerEnabled = true,
	gpuProfiler = false,
	validationWarnings = true,
	telemetry = false,
	editorTheme = 1,
	autosave = true,
	autosaveInterval = 10,
	showAdvancedWarnings = true,
}

local ProjectSettingsStoreMap =
{
	projectName = { name = "project.name", type = "string" },
	company = { name = "project.company", type = "string" },
	productName = { name = "project.productName", type = "string" },
	version = { name = "project.version", type = "string" },
	bundleId = { name = "project.bundleId", type = "string" },
	defaultLanguage = { name = "project.defaultLanguage", type = "int" },
	startupScene = { name = "project.startupScene", type = "string" },
	sceneList = { name = "project.sceneList", type = "string" },
	autoLoadLastScene = { name = "project.autoLoadLastScene", type = "bool" },

	activePlatform = { name = "platform.active", type = "int" },
	rendererPC = { name = "graphics.renderer.pc", type = "int" },
	rendererMac = { name = "graphics.renderer.mac", type = "int" },
	rendererIOS = { name = "graphics.renderer.ios", type = "int" },
	rendererAndroid = { name = "graphics.renderer.android", type = "int" },
	rendererMode = { name = "graphics.renderer.mode", type = "int" },
	shaderModel = { name = "graphics.shaderModel", type = "int" },
	shaderLanguage = { name = "graphics.shaderLanguage", type = "int" },
	useValidationLayers = { name = "graphics.validationLayers", type = "bool" },
	allowRendererFallback = { name = "graphics.allowRendererFallback", type = "bool" },

	resolution = { name = "graphics.resolution", type = "int" },
	fullscreen = { name = "graphics.fullscreen", type = "bool" },
	vsync = { name = "graphics.vsync", type = "bool" },
	frameRateLimit = { name = "graphics.frameRateLimit", type = "int" },
	renderScale = { name = "graphics.renderScale", type = "float" },
	aa = { name = "graphics.antiAliasing", type = "int" },
	dynamicResolution = { name = "graphics.dynamicResolution", type = "bool" },
	hdr = { name = "graphics.hdr", type = "bool" },
	colorSpace = { name = "graphics.colorSpace", type = "int" },
	textureFiltering = { name = "graphics.textureFiltering", type = "int" },
	anisotropy = { name = "graphics.anisotropy", type = "int" },
	presentMode = { name = "graphics.presentMode", type = "int" },

	renderPipeline = { name = "graphics.renderPipeline", type = "int" },
	shadowQuality = { name = "graphics.shadowQuality", type = "int" },
	shadowDistance = { name = "graphics.shadowDistance", type = "float" },
	shadowResolution = { name = "graphics.shadowResolution", type = "int" },
	lightingMode = { name = "graphics.lightingMode", type = "int" },
	gi = { name = "graphics.gi", type = "int" },
	reflectionProbes = { name = "graphics.reflectionProbes", type = "bool" },
	postProcessing = { name = "graphics.postProcessing", type = "bool" },
	bloom = { name = "graphics.bloom", type = "bool" },
	motionBlur = { name = "graphics.motionBlur", type = "bool" },
	ambientOcclusion = { name = "graphics.ambientOcclusion", type = "bool" },
	tonemapping = { name = "graphics.tonemapping", type = "int" },

	qualityPreset = { name = "quality.preset", type = "int" },
	textureQuality = { name = "quality.texture", type = "int" },
	maxTextureSize = { name = "quality.maxTextureSize", type = "int" },
	meshQuality = { name = "quality.mesh", type = "int" },
	lodBias = { name = "quality.lodBias", type = "float" },
	particleQuality = { name = "quality.particles", type = "int" },
	terrainQuality = { name = "quality.terrain", type = "int" },
	streamingBudgetMb = { name = "quality.streamingBudgetMb", type = "int" },
	assetCompression = { name = "assets.compression", type = "int" },
	textureCompressionPC = { name = "assets.textureCompression.pc", type = "int" },
	textureCompressionMac = { name = "assets.textureCompression.mac", type = "int" },
	textureCompressionIOS = { name = "assets.textureCompression.ios", type = "int" },
	textureCompressionAndroid = { name = "assets.textureCompression.android", type = "int" },

	gravity = { name = "physics.gravity", type = "float" },
	simulationRate = { name = "physics.simulationRate", type = "int" },
	solverIterations = { name = "physics.solverIterations", type = "int" },
	collisionLayers = { name = "physics.collisionLayers", type = "string" },
	broadphase = { name = "physics.broadphase", type = "int" },
	continuousCollision = { name = "physics.continuousCollision", type = "bool" },
	physicsSubsteps = { name = "physics.substeps", type = "int" },
	physicsThreads = { name = "physics.threads", type = "int" },
	deterministicPhysics = { name = "physics.deterministic", type = "bool" },
	physicsDebugDraw = { name = "physics.debugDraw", type = "bool" },

	masterVolume = { name = "sound.masterVolume", type = "int" },
	musicVolume = { name = "sound.musicVolume", type = "int" },
	sfxVolume = { name = "sound.sfxVolume", type = "int" },
	voiceVolume = { name = "sound.voiceVolume", type = "int" },
	spatialization = { name = "sound.spatialization", type = "bool" },
	audioBackend = { name = "sound.backend", type = "int" },
	outputDevice = { name = "sound.outputDevice", type = "int" },
	audioSampleRate = { name = "sound.sampleRate", type = "int" },
	audioBufferSize = { name = "sound.bufferSize", type = "int" },
	audioVoiceLimit = { name = "sound.voiceLimit", type = "int" },
	audioStreaming = { name = "sound.streaming", type = "bool" },
	audioOcclusion = { name = "sound.occlusion", type = "bool" },

	inputSystem = { name = "input.system", type = "int" },
	gamepadSupport = { name = "input.gamepad", type = "bool" },
	touchSupport = { name = "input.touch", type = "bool" },
	mouseCapture = { name = "input.mouseCapture", type = "bool" },
	inputRebinds = { name = "input.rebinds", type = "bool" },
	gestureRecognition = { name = "input.gestures", type = "bool" },
	haptics = { name = "input.haptics", type = "bool" },

	buildConfig = { name = "build.config", type = "int" },
	architecturePC = { name = "build.architecture.pc", type = "int" },
	architectureMac = { name = "build.architecture.mac", type = "int" },
	architectureIOS = { name = "build.architecture.ios", type = "int" },
	architectureAndroid = { name = "build.architecture.android", type = "int" },
	minIOSVersion = { name = "build.ios.minVersion", type = "int" },
	minAndroidSdk = { name = "build.android.minSdk", type = "int" },
	targetAndroidSdk = { name = "build.android.targetSdk", type = "int" },
	useAndroidAppBundle = { name = "build.android.appBundle", type = "bool" },
	signingProfile = { name = "build.signingProfile", type = "string" },
	stripSymbols = { name = "build.stripSymbols", type = "bool" },
	generateDebugSymbols = { name = "build.debugSymbols", type = "bool" },
	includeEditorData = { name = "build.includeEditorData", type = "bool" },
	useAssetBundles = { name = "build.assetBundles", type = "bool" },
	buildCompression = { name = "build.compression", type = "int" },

	threadCount = { name = "tasks.threadCount", type = "int" },
	backgroundTasks = { name = "tasks.backgroundTasks", type = "bool" },
	taskPriority = { name = "tasks.priority", type = "int" },
	jobSystem = { name = "tasks.jobSystem", type = "int" },
	workerAffinity = { name = "tasks.workerAffinity", type = "int" },
	renderThread = { name = "tasks.renderThread", type = "bool" },
	gpuParticles = { name = "performance.gpuParticles", type = "bool" },
	occlusionCulling = { name = "performance.occlusionCulling", type = "bool" },
	staticBatching = { name = "performance.staticBatching", type = "bool" },
	dynamicBatching = { name = "performance.dynamicBatching", type = "bool" },
	memoryBudgetMb = { name = "performance.memoryBudgetMb", type = "int" },
	gpuMemoryBudgetMb = { name = "performance.gpuMemoryBudgetMb", type = "int" },

	scriptBackend = { name = "scripting.backend", type = "int" },
	luaEnabled = { name = "scripting.lua", type = "bool" },
	hotReloadScripts = { name = "scripting.hotReload", type = "bool" },
	reflectionMode = { name = "scripting.reflection", type = "int" },
	networkBackend = { name = "network.backend", type = "int" },
	enableMultiplayer = { name = "network.multiplayer", type = "bool" },
	cloudSave = { name = "services.cloudSave", type = "bool" },
	analytics = { name = "services.analytics", type = "bool" },
	crashReporting = { name = "services.crashReporting", type = "bool" },

	logLevel = { name = "diagnostics.logLevel", type = "int" },
	profilerEnabled = { name = "diagnostics.profiler", type = "bool" },
	gpuProfiler = { name = "diagnostics.gpuProfiler", type = "bool" },
	validationWarnings = { name = "diagnostics.validationWarnings", type = "bool" },
	telemetry = { name = "diagnostics.telemetry", type = "bool" },
	editorTheme = { name = "editor.theme", type = "int" },
	autosave = { name = "editor.autosave", type = "bool" },
	autosaveInterval = { name = "editor.autosaveInterval", type = "int" },
	showAdvancedWarnings = { name = "editor.advancedWarnings", type = "bool" },
}

local PlatformNames = { "PC", "macOS (build unavailable)", "iOS (build unavailable)", "Android (build unavailable)" };
local PcRendererNames = { "Auto", "DirectX 11", "DirectX 12 (unavailable)", "Vulkan (unavailable)", "OpenGL (unavailable)", "Software (CPU)" };
local MacRendererNames = { "Auto", "Metal", "Vulkan / MoltenVK", "OpenGL" };
local IosRendererNames = { "Auto", "Metal", "OpenGL ES" };
local AndroidRendererNames = { "Auto", "Vulkan", "OpenGL ES" };

local function addText(ui, parent, text)
	local textTypeInfo = IUIText.typeInfo();
	local label = ui:addElement(textTypeInfo);
	label:setText(text);
	label:setSameLine(false);
	parent:addChild(label);
	return label;
end

local function addButton(ui, parent, label, elementId, sameLine)
	local buttonTypeInfo = IUIButton.typeInfo();
	local button = ui:addElement(buttonTypeInfo);
	button:setLabel(label);
	button:setElementId(elementId);
	button:setSameLine(sameLine == true);
	parent:addChild(button);
	return button;
end

local function addToggle(ui, parent, label, elementId, value)
	local toggleTypeInfo = IUILabelTogglePair.typeInfo();
	local toggle = ui:addElement(toggleTypeInfo);
	toggle:setLabel(label);
	toggle:setElementId(elementId);
	toggle:setValue(value == true);
	toggle:setSameLine(false);
	parent:addChild(toggle);
	return toggle;
end

local function addSlider(ui, parent, label, elementId, minValue, maxValue, value)
	local sliderTypeInfo = IUILabelSliderPair.typeInfo();
	local slider = ui:addElement(sliderTypeInfo);
	slider:setLabel(label);
	slider:setElementId(elementId);
	slider:setMinValue(minValue);
	slider:setMaxValue(maxValue);
	slider:setValue(value);
	slider:setSameLine(false);
	parent:addChild(slider);
	return slider;
end

local function addEntry(ui, parent, label, elementId)
	local textEntryTypeInfo = IUITextEntry.typeInfo();
	local entry = ui:addElement(textEntryTypeInfo);
	entry:setLabel(label);
	entry:setElementId(elementId);
	entry:setSameLine(false);
	parent:addChild(entry);
	return entry;
end

local function addDropdown(ui, parent, label, elementId, options, selected)
	local dropdownTypeInfo = IUIDropdown.typeInfo();
	local dropdown = ui:addElement(dropdownTypeInfo);
	dropdown:setLabel(label);
	dropdown:setElementId(elementId);
	for _, option in ipairs(options) do
		dropdown:addOption(option);
	end
	dropdown:setSelectedOption(selected or 0);
	dropdown:setSameLine(false);
	parent:addChild(dropdown);
	return dropdown;
end

local function round(value)
	return math.floor(value + 0.5);
end

local function safeOption(options, index)
	local i = (index or 0) + 1;
	return options[i] or options[1] or "";
end

function ProjectSettings:__init(window)
	print("ProjectSettings constructor called");

	self.window = window;
	self.editorWindow = nil;
	self.tabBar = nil;
	self.statusText = nil;
	self.rendererSummaryText = nil;
	self.settings = {};
	self.controlBindings = {};
	self.actionLog = {};
	self.capabilities = {};
	self.unavailableReasons = {};
	self.syncingControls = false;
	self.statusMessage = "Ready.";
end

function ProjectSettings:__finalize()
	print("ProjectSettings __finalize called");
	self:_clearRefs();
	self.window = nil;
end

function ProjectSettings:_clearRefs()
	self.editorWindow = nil;
	self.tabBar = nil;
	self.statusText = nil;
	self.rendererSummaryText = nil;
	self.controlBindings = {};

	-- Element references are cleared by name so Lua GC can release UI wrappers.
	for key, _ in pairs(ProjectSettingsDefaults) do
		self[key .. "Control"] = nil;
	end

	self.applyButton = nil;
	self.saveButton = nil;
	self.resetButton = nil;
	self.validateButton = nil;
	self.exportJsonButton = nil;
	self.importJsonButton = nil;
	self.buildSelectedButton = nil;
	self.reloadRendererButton = nil;
	self.applyPcProfileButton = nil;
	self.applyMacProfileButton = nil;
	self.applyIosProfileButton = nil;
	self.applyAndroidProfileButton = nil;
end

function ProjectSettings:bind(control, key, kind)
	if control == nil then
		return control;
	end

	table.insert(self.controlBindings, { control = control, key = key, kind = kind, label = control:getLabel() });
	self[key .. "Control"] = control;
	return control;
end

local function readProperty(properties, name, valueType)
	if valueType == "bool" then
		return properties:getPropertyAsBool(name);
	elseif valueType == "int" then
		return properties:getPropertyAsInt(name);
	elseif valueType == "float" then
		return properties:getPropertyAsFloat(name);
	end
	return properties:getPropertyAsString(name);
end

local function writeProperty(properties, name, value, valueType)
	if valueType == "bool" then
		properties:setPropertyAsBool(name, value == true);
	elseif valueType == "int" then
		properties:setPropertyAsInt(name, round(value));
	elseif valueType == "float" then
		properties:setPropertyAsFloat(name, value);
	else
		properties:setPropertyAsString(name, value or "");
	end
end

function ProjectSettings:setStatus(message)
	self.statusMessage = message;
	if self.statusText ~= nil then
		self.statusText:setText(message);
	end
end

function ProjectSettings:readSettingsProperties(properties)
	for key, map in pairs(ProjectSettingsStoreMap) do
		if properties:hasProperty(map.name) then
			self.settings[key] = readProperty(properties, map.name, map.type);
		elseif properties:hasProperty(key) then
			-- Older window snapshots used the Lua field names.
			self.settings[key] = readProperty(properties, key, map.type);
		end
		if properties:hasProperty("capability." .. map.name) then
			self.capabilities[key] = properties:getPropertyAsBool("capability." .. map.name);
		end
		if properties:hasProperty("unavailable." .. map.name) then
			self.unavailableReasons[key] = properties:getPropertyAsString("unavailable." .. map.name);
		end
	end
end

function ProjectSettings:writeSettingsProperties(properties)
	for key, map in pairs(ProjectSettingsStoreMap) do
		writeProperty(properties, map.name, self.settings[key], map.type);
	end
end

function ProjectSettings:loadSettings()
	self.capabilities = {};
	self.unavailableReasons = {};
	for key, defaultValue in pairs(ProjectSettingsDefaults) do
		self.settings[key] = defaultValue;
	end
	if not self:performAction("readProjectSettings", false) then
		for key, _ in pairs(ProjectSettingsDefaults) do
			self.capabilities[key] = false;
			self.unavailableReasons[key] = "Open a project before editing project settings.";
		end
	end
end

function ProjectSettings:addHeader(ui, tab, label)
	local collapsingHeaderTypeInfo = IUICollapsingHeader.typeInfo();
	local header = ui:addElement(collapsingHeaderTypeInfo);
	header:setLabel(label);
	tab:addChild(header);
	return header;
end

function ProjectSettings:load()
	print("ProjectSettings load start");

	local windowTypeInfo = IUIWindow.typeInfo();
	local tabBarTypeInfo = IUITabBar.typeInfo();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	local parentWindow = self.window:getParentWindow();

	self:loadSettings();
	self.controlBindings = {};

	self.editorWindow = ui:addElement(windowTypeInfo);
	self.editorWindow:setLabel("Project Settings");
	self.editorWindow:setSize(Vector2F(860.0, 720.0));
	parentWindow:addChild(self.editorWindow);

	self.tabBar = ui:addElement(tabBarTypeInfo);
	local applicationTab = self.tabBar:addTabItem(); applicationTab:setLabel("Application");
	local platformTab = self.tabBar:addTabItem(); platformTab:setLabel("Platforms");
	local graphicsTab = self.tabBar:addTabItem(); graphicsTab:setLabel("Graphics");
	local qualityTab = self.tabBar:addTabItem(); qualityTab:setLabel("Quality");
	local physicsTab = self.tabBar:addTabItem(); physicsTab:setLabel("Physics");
	local soundTab = self.tabBar:addTabItem(); soundTab:setLabel("Audio");
	local inputTab = self.tabBar:addTabItem(); inputTab:setLabel("Input");
	local buildTab = self.tabBar:addTabItem(); buildTab:setLabel("Build");
	local performanceTab = self.tabBar:addTabItem(); performanceTab:setLabel("Performance");
	local servicesTab = self.tabBar:addTabItem(); servicesTab:setLabel("Code/Services");
	local diagnosticsTab = self.tabBar:addTabItem(); diagnosticsTab:setLabel("Diagnostics");
	local advancedTab = self.tabBar:addTabItem(); advancedTab:setLabel("Advanced");
	self.editorWindow:addChild(self.tabBar);

	-- Application ----------------------------------------------------------
	local identityHeader = self:addHeader(ui, applicationTab, "Project Identity");
	self:bind(addEntry(ui, identityHeader, "Project Name", ProjectSettingsTypes.ProjectName), "projectName", "text"):setText(self.settings.projectName);
	self:bind(addEntry(ui, identityHeader, "Company", ProjectSettingsTypes.Company), "company", "text"):setText(self.settings.company);
	self:bind(addEntry(ui, identityHeader, "Product Name", ProjectSettingsTypes.ProductName), "productName", "text"):setText(self.settings.productName);
	self:bind(addEntry(ui, identityHeader, "Version", ProjectSettingsTypes.Version), "version", "text"):setText(self.settings.version);
	self:bind(addEntry(ui, identityHeader, "Bundle Identifier", ProjectSettingsTypes.BundleId), "bundleId", "text"):setText(self.settings.bundleId);
	self:bind(addDropdown(ui, identityHeader, "Default Language", ProjectSettingsTypes.DefaultLanguage,
		{ "English", "French", "German", "Japanese", "Spanish", "Italian", "Portuguese" }, self.settings.defaultLanguage), "defaultLanguage", "dropdown");

	local scenesHeader = self:addHeader(ui, applicationTab, "Scenes");
	self:bind(addEntry(ui, scenesHeader, "Startup Scene", ProjectSettingsTypes.StartupScene), "startupScene", "text"):setText(self.settings.startupScene);
	self:bind(addEntry(ui, scenesHeader, "Scene List", ProjectSettingsTypes.SceneList), "sceneList", "text"):setText(self.settings.sceneList);
	self:bind(addToggle(ui, scenesHeader, "Auto Load Last Editor Scene", ProjectSettingsTypes.AutoLoadLastScene, self.settings.autoLoadLastScene), "autoLoadLastScene", "toggle");

	-- Platforms / renderers -----------------------------------------------
	local targetHeader = self:addHeader(ui, platformTab, "Target Platform");
	self:bind(addDropdown(ui, targetHeader, "Active Platform", ProjectSettingsTypes.ActivePlatform, PlatformNames, self.settings.activePlatform), "activePlatform", "dropdown");
	addText(ui, targetHeader, "Renderer choices are stored per platform. Runtime can use Auto, or a concrete backend per platform.");

	local rendererHeader = self:addHeader(ui, platformTab, "Renderer Backends");
	self:bind(addDropdown(ui, rendererHeader, "PC Renderer", ProjectSettingsTypes.PcRenderer, PcRendererNames, self.settings.rendererPC), "rendererPC", "dropdown");
	self:bind(addDropdown(ui, rendererHeader, "macOS Renderer", ProjectSettingsTypes.MacRenderer, MacRendererNames, self.settings.rendererMac), "rendererMac", "dropdown");
	self:bind(addDropdown(ui, rendererHeader, "iOS Renderer", ProjectSettingsTypes.IosRenderer, IosRendererNames, self.settings.rendererIOS), "rendererIOS", "dropdown");
	self:bind(addDropdown(ui, rendererHeader, "Android Renderer", ProjectSettingsTypes.AndroidRenderer, AndroidRendererNames, self.settings.rendererAndroid), "rendererAndroid", "dropdown");
	self:bind(addDropdown(ui, rendererHeader, "Renderer Mode", ProjectSettingsTypes.RendererMode,
		{ "Auto Select", "Force Selected", "Prefer High Performance", "Prefer Compatibility" }, self.settings.rendererMode), "rendererMode", "dropdown");
	self:bind(addDropdown(ui, rendererHeader, "Shader Model", ProjectSettingsTypes.ShaderModel,
		{ "SM 4.x / ES 3.0", "SM 5.x / ES 3.1", "SM 6.x / Metal 2", "Experimental" }, self.settings.shaderModel), "shaderModel", "dropdown");
	self:bind(addDropdown(ui, rendererHeader, "Shader Language", ProjectSettingsTypes.ShaderLanguage,
		{ "Auto", "HLSL", "MSL", "GLSL", "SPIR-V" }, self.settings.shaderLanguage), "shaderLanguage", "dropdown");
	self:bind(addToggle(ui, rendererHeader, "Use Validation Layers", ProjectSettingsTypes.UseValidationLayers, self.settings.useValidationLayers), "useValidationLayers", "toggle");
	self:bind(addToggle(ui, rendererHeader, "Allow Renderer Fallback", ProjectSettingsTypes.AllowRendererFallback, self.settings.allowRendererFallback), "allowRendererFallback", "toggle");

	local profilesHeader = self:addHeader(ui, platformTab, "Platform Profiles");
	addText(ui, profilesHeader, "Profiles stage target defaults for review. The current toolchain builds PC projects.");
	self.applyPcProfileButton = addButton(ui, profilesHeader, "Use PC Profile", ProjectSettingsTypes.ApplyPcProfile, false);
	self.applyMacProfileButton = addButton(ui, profilesHeader, "Use macOS Profile", ProjectSettingsTypes.ApplyMacProfile, true);
	self.applyIosProfileButton = addButton(ui, profilesHeader, "Use iOS Profile", ProjectSettingsTypes.ApplyIosProfile, false);
	self.applyAndroidProfileButton = addButton(ui, profilesHeader, "Use Android Profile", ProjectSettingsTypes.ApplyAndroidProfile, true);
	self.rendererSummaryText = addText(ui, profilesHeader, "Renderer summary will appear here.");

	-- Graphics -------------------------------------------------------------
	local displayHeader = self:addHeader(ui, graphicsTab, "Display");
	addText(ui, displayHeader, "Game graphics changes apply to the next standalone launch. Unavailable controls need engine support.");
	addText(ui, displayHeader, "Post effects require the Software game renderer.");
	self:bind(addDropdown(ui, displayHeader, "Resolution", ProjectSettingsTypes.Resolution,
		{ "1920x1080", "1280x720", "2560x1440", "3840x2160", "Device Native", "Custom" }, self.settings.resolution), "resolution", "dropdown");
	self:bind(addToggle(ui, displayHeader, "Fullscreen", ProjectSettingsTypes.Fullscreen, self.settings.fullscreen), "fullscreen", "toggle");
	self:bind(addToggle(ui, displayHeader, "VSync", ProjectSettingsTypes.VSync, self.settings.vsync), "vsync", "toggle");
	self:bind(addSlider(ui, displayHeader, "Frame Rate Limit", ProjectSettingsTypes.FrameRateLimit, 15.0, 240.0, self.settings.frameRateLimit), "frameRateLimit", "intSlider");
	self:bind(addSlider(ui, displayHeader, "Render Scale", ProjectSettingsTypes.RenderScale, 0.25, 2.0, self.settings.renderScale), "renderScale", "floatSlider");
	self:bind(addDropdown(ui, displayHeader, "Present Mode", ProjectSettingsTypes.PresentMode,
		{ "Auto", "FIFO / VSync", "Mailbox", "Immediate" }, self.settings.presentMode), "presentMode", "dropdown");

	local renderingHeader = self:addHeader(ui, graphicsTab, "Rendering");
	self:bind(addDropdown(ui, renderingHeader, "Render Pipeline", ProjectSettingsTypes.RenderPipeline,
		{ "Forward", "Forward+", "Deferred", "Mobile Forward" }, self.settings.renderPipeline), "renderPipeline", "dropdown");
	self:bind(addDropdown(ui, renderingHeader, "Anti-Aliasing", ProjectSettingsTypes.AntiAliasing,
		{ "None", "FXAA (unavailable)", "TAA", "MSAA 2x (unavailable)", "MSAA 4x (unavailable)", "MSAA 8x (unavailable)" }, self.settings.aa), "aa", "dropdown");
	self:bind(addToggle(ui, renderingHeader, "Dynamic Resolution", ProjectSettingsTypes.DynamicResolution, self.settings.dynamicResolution), "dynamicResolution", "toggle");
	self:bind(addToggle(ui, renderingHeader, "HDR", ProjectSettingsTypes.HDR, self.settings.hdr), "hdr", "toggle");
	self:bind(addDropdown(ui, renderingHeader, "Color Space", ProjectSettingsTypes.ColorSpace,
		{ "Gamma", "Linear" }, self.settings.colorSpace), "colorSpace", "dropdown");
	self:bind(addDropdown(ui, renderingHeader, "Texture Filtering", ProjectSettingsTypes.TextureFiltering,
		{ "Point", "Bilinear", "Trilinear" }, self.settings.textureFiltering), "textureFiltering", "dropdown");
	self:bind(addSlider(ui, renderingHeader, "Anisotropy", ProjectSettingsTypes.Anisotropy, 1.0, 16.0, self.settings.anisotropy), "anisotropy", "intSlider");

	local lightingHeader = self:addHeader(ui, graphicsTab, "Lighting / Shadows / Post FX");
	self:bind(addDropdown(ui, lightingHeader, "Shadows", ProjectSettingsTypes.ShadowQuality,
		{ "Off", "On" }, self.settings.shadowQuality > 0 and 1 or 0), "shadowQuality", "dropdown");
	self:bind(addSlider(ui, lightingHeader, "Shadow Distance", ProjectSettingsTypes.ShadowDistance, 0.0, 300.0, self.settings.shadowDistance), "shadowDistance", "floatSlider");
	self:bind(addDropdown(ui, lightingHeader, "Shadow Resolution", ProjectSettingsTypes.ShadowResolution,
		{ "512", "1024", "2048", "4096" }, self.settings.shadowResolution), "shadowResolution", "dropdown");
	self:bind(addDropdown(ui, lightingHeader, "Lighting Mode", ProjectSettingsTypes.LightingMode,
		{ "Unlit", "Per Vertex", "Per Pixel", "Clustered" }, self.settings.lightingMode), "lightingMode", "dropdown");
	self:bind(addDropdown(ui, lightingHeader, "Global Illumination", ProjectSettingsTypes.GI,
		{ "Off", "Baked", "Realtime", "Hybrid" }, self.settings.gi), "gi", "dropdown");
	self:bind(addToggle(ui, lightingHeader, "Reflection Probes", ProjectSettingsTypes.ReflectionProbes, self.settings.reflectionProbes), "reflectionProbes", "toggle");
	self:bind(addToggle(ui, lightingHeader, "Post Processing", ProjectSettingsTypes.PostProcessing, self.settings.postProcessing), "postProcessing", "toggle");
	self:bind(addToggle(ui, lightingHeader, "Bloom", ProjectSettingsTypes.Bloom, self.settings.bloom), "bloom", "toggle");
	self:bind(addToggle(ui, lightingHeader, "Motion Blur", ProjectSettingsTypes.MotionBlur, self.settings.motionBlur), "motionBlur", "toggle");
	self:bind(addToggle(ui, lightingHeader, "Ambient Occlusion", ProjectSettingsTypes.AmbientOcclusion, self.settings.ambientOcclusion), "ambientOcclusion", "toggle");
	self:bind(addDropdown(ui, lightingHeader, "Tonemapping", ProjectSettingsTypes.Tonemapping,
		{ "None", "ACES", "Reinhard", "Filmic" }, self.settings.tonemapping), "tonemapping", "dropdown");

	-- Quality / assets -----------------------------------------------------
	local qualityHeader = self:addHeader(ui, qualityTab, "Quality Presets");
	self:bind(addDropdown(ui, qualityHeader, "Quality Preset", ProjectSettingsTypes.QualityPreset,
		{ "Low", "Medium", "High", "Ultra", "Custom" }, self.settings.qualityPreset), "qualityPreset", "dropdown");
	self:bind(addDropdown(ui, qualityHeader, "Texture Quality", ProjectSettingsTypes.TextureQuality,
		{ "Quarter", "Half", "Full", "Ultra" }, self.settings.textureQuality), "textureQuality", "dropdown");
	self:bind(addSlider(ui, qualityHeader, "Max Texture Size", ProjectSettingsTypes.MaxTextureSize, 256.0, 8192.0, self.settings.maxTextureSize), "maxTextureSize", "intSlider");
	self:bind(addDropdown(ui, qualityHeader, "Mesh Quality", ProjectSettingsTypes.MeshQuality,
		{ "Low", "Medium", "High", "Ultra" }, self.settings.meshQuality), "meshQuality", "dropdown");
	self:bind(addSlider(ui, qualityHeader, "LOD Bias", ProjectSettingsTypes.LodBias, 0.1, 4.0, self.settings.lodBias), "lodBias", "floatSlider");
	self:bind(addDropdown(ui, qualityHeader, "Particle Quality", ProjectSettingsTypes.ParticleQuality,
		{ "Low", "Medium", "High", "Ultra" }, self.settings.particleQuality), "particleQuality", "dropdown");
	self:bind(addDropdown(ui, qualityHeader, "Terrain Quality", ProjectSettingsTypes.TerrainQuality,
		{ "Low", "Medium", "High", "Ultra" }, self.settings.terrainQuality), "terrainQuality", "dropdown");
	self:bind(addSlider(ui, qualityHeader, "Streaming Budget MB", ProjectSettingsTypes.StreamingBudgetMb, 64.0, 4096.0, self.settings.streamingBudgetMb), "streamingBudgetMb", "intSlider");

	local compressionHeader = self:addHeader(ui, qualityTab, "Asset Compression");
	self:bind(addDropdown(ui, compressionHeader, "Asset Compression", ProjectSettingsTypes.AssetCompression,
		{ "None", "Fast", "Balanced", "Maximum" }, self.settings.assetCompression), "assetCompression", "dropdown");
	self:bind(addDropdown(ui, compressionHeader, "PC Texture Compression", ProjectSettingsTypes.TextureCompressionPC,
		{ "None", "BC1/BC3", "BC7", "ASTC" }, self.settings.textureCompressionPC), "textureCompressionPC", "dropdown");
	self:bind(addDropdown(ui, compressionHeader, "macOS Texture Compression", ProjectSettingsTypes.TextureCompressionMac,
		{ "None", "BC1/BC3", "BC7", "ASTC" }, self.settings.textureCompressionMac), "textureCompressionMac", "dropdown");
	self:bind(addDropdown(ui, compressionHeader, "iOS Texture Compression", ProjectSettingsTypes.TextureCompressionIOS,
		{ "None", "PVRTC", "ASTC", "ETC2" }, self.settings.textureCompressionIOS), "textureCompressionIOS", "dropdown");
	self:bind(addDropdown(ui, compressionHeader, "Android Texture Compression", ProjectSettingsTypes.TextureCompressionAndroid,
		{ "None", "ETC2", "ASTC", "BC7", "Device Default" }, self.settings.textureCompressionAndroid), "textureCompressionAndroid", "dropdown");

	-- Physics --------------------------------------------------------------
	local physicsHeader = self:addHeader(ui, physicsTab, "Simulation");
	self:bind(addSlider(ui, physicsHeader, "Gravity Y", ProjectSettingsTypes.Gravity, -50.0, 0.0, self.settings.gravity), "gravity", "floatSlider");
	self:bind(addSlider(ui, physicsHeader, "Simulation Rate", ProjectSettingsTypes.SimulationRate, 30.0, 240.0, self.settings.simulationRate), "simulationRate", "intSlider");
	self:bind(addSlider(ui, physicsHeader, "Solver Iterations", ProjectSettingsTypes.SolverIterations, 1.0, 64.0, self.settings.solverIterations), "solverIterations", "intSlider");
	self:bind(addSlider(ui, physicsHeader, "Substeps", ProjectSettingsTypes.PhysicsSubsteps, 1.0, 8.0, self.settings.physicsSubsteps), "physicsSubsteps", "intSlider");
	self:bind(addSlider(ui, physicsHeader, "Physics Threads", ProjectSettingsTypes.PhysicsThreads, 1.0, 16.0, self.settings.physicsThreads), "physicsThreads", "intSlider");
	self:bind(addDropdown(ui, physicsHeader, "Broadphase", ProjectSettingsTypes.Broadphase,
		{ "Sweep And Prune", "Multi Box Prune", "Automatic Box Prune", "GPU Broadphase" }, self.settings.broadphase), "broadphase", "dropdown");
	self:bind(addToggle(ui, physicsHeader, "Continuous Collision", ProjectSettingsTypes.ContinuousCollision, self.settings.continuousCollision), "continuousCollision", "toggle");
	self:bind(addToggle(ui, physicsHeader, "Deterministic Physics", ProjectSettingsTypes.DeterministicPhysics, self.settings.deterministicPhysics), "deterministicPhysics", "toggle");
	self:bind(addToggle(ui, physicsHeader, "Physics Debug Draw", ProjectSettingsTypes.PhysicsDebugDraw, self.settings.physicsDebugDraw), "physicsDebugDraw", "toggle");
	self:bind(addEntry(ui, physicsHeader, "Collision Layers", ProjectSettingsTypes.CollisionLayers), "collisionLayers", "text"):setText(self.settings.collisionLayers);

	-- Audio ----------------------------------------------------------------
	local soundHeader = self:addHeader(ui, soundTab, "Mixer / Runtime Audio");
	self:bind(addSlider(ui, soundHeader, "Master Volume", ProjectSettingsTypes.MasterVolume, 0.0, 100.0, self.settings.masterVolume), "masterVolume", "intSlider");
	self:bind(addSlider(ui, soundHeader, "Music Volume", ProjectSettingsTypes.MusicVolume, 0.0, 100.0, self.settings.musicVolume), "musicVolume", "intSlider");
	self:bind(addSlider(ui, soundHeader, "SFX Volume", ProjectSettingsTypes.SfxVolume, 0.0, 100.0, self.settings.sfxVolume), "sfxVolume", "intSlider");
	self:bind(addSlider(ui, soundHeader, "Voice Volume", ProjectSettingsTypes.VoiceVolume, 0.0, 100.0, self.settings.voiceVolume), "voiceVolume", "intSlider");
	self:bind(addDropdown(ui, soundHeader, "Audio Backend", ProjectSettingsTypes.AudioBackend,
		{ "Default", "FMOD Studio", "OpenAL", "Platform Native" }, self.settings.audioBackend), "audioBackend", "dropdown");
	self:bind(addDropdown(ui, soundHeader, "Output Device", ProjectSettingsTypes.OutputDevice,
		{ "Default", "Headphones", "Speakers", "Controller", "Mobile Speaker" }, self.settings.outputDevice), "outputDevice", "dropdown");
	self:bind(addToggle(ui, soundHeader, "Enable Spatialization", ProjectSettingsTypes.Spatialization, self.settings.spatialization), "spatialization", "toggle");
	self:bind(addToggle(ui, soundHeader, "Audio Streaming", ProjectSettingsTypes.AudioStreaming, self.settings.audioStreaming), "audioStreaming", "toggle");
	self:bind(addToggle(ui, soundHeader, "Audio Occlusion", ProjectSettingsTypes.AudioOcclusion, self.settings.audioOcclusion), "audioOcclusion", "toggle");
	self:bind(addSlider(ui, soundHeader, "Sample Rate", ProjectSettingsTypes.AudioSampleRate, 22050.0, 96000.0, self.settings.audioSampleRate), "audioSampleRate", "intSlider");
	self:bind(addSlider(ui, soundHeader, "Buffer Size", ProjectSettingsTypes.AudioBufferSize, 128.0, 4096.0, self.settings.audioBufferSize), "audioBufferSize", "intSlider");
	self:bind(addSlider(ui, soundHeader, "Voice Limit", ProjectSettingsTypes.AudioVoiceLimit, 16.0, 512.0, self.settings.audioVoiceLimit), "audioVoiceLimit", "intSlider");

	-- Input ----------------------------------------------------------------
	local inputHeader = self:addHeader(ui, inputTab, "Input Devices");
	self:bind(addDropdown(ui, inputHeader, "Input System", ProjectSettingsTypes.InputSystem,
		{ "Engine Input", "SDL", "Platform Native", "Hybrid" }, self.settings.inputSystem), "inputSystem", "dropdown");
	self:bind(addToggle(ui, inputHeader, "Gamepad Support", ProjectSettingsTypes.GamepadSupport, self.settings.gamepadSupport), "gamepadSupport", "toggle");
	self:bind(addToggle(ui, inputHeader, "Touch Support", ProjectSettingsTypes.TouchSupport, self.settings.touchSupport), "touchSupport", "toggle");
	self:bind(addToggle(ui, inputHeader, "Mouse Capture", ProjectSettingsTypes.MouseCapture, self.settings.mouseCapture), "mouseCapture", "toggle");
	self:bind(addToggle(ui, inputHeader, "Runtime Rebinding", ProjectSettingsTypes.InputRebinds, self.settings.inputRebinds), "inputRebinds", "toggle");
	self:bind(addToggle(ui, inputHeader, "Gesture Recognition", ProjectSettingsTypes.GestureRecognition, self.settings.gestureRecognition), "gestureRecognition", "toggle");
	self:bind(addToggle(ui, inputHeader, "Haptics", ProjectSettingsTypes.Haptics, self.settings.haptics), "haptics", "toggle");

	-- Build ----------------------------------------------------------------
	local buildHeader = self:addHeader(ui, buildTab, "Build Configuration");
	self:bind(addDropdown(ui, buildHeader, "Build Config", ProjectSettingsTypes.BuildConfig,
		{ "Debug", "Development", "Release", "Shipping" }, self.settings.buildConfig), "buildConfig", "dropdown");
	self:bind(addDropdown(ui, buildHeader, "PC Architecture", ProjectSettingsTypes.ArchitecturePC,
		{ "x86 (unavailable)", "x64", "ARM64 (unavailable)" }, self.settings.architecturePC), "architecturePC", "dropdown");
	self:bind(addDropdown(ui, buildHeader, "macOS Architecture", ProjectSettingsTypes.ArchitectureMac,
		{ "Intel", "Apple Silicon", "Universal" }, self.settings.architectureMac), "architectureMac", "dropdown");
	self:bind(addDropdown(ui, buildHeader, "iOS Architecture", ProjectSettingsTypes.ArchitectureIOS,
		{ "ARM64" }, self.settings.architectureIOS), "architectureIOS", "dropdown");
	self:bind(addDropdown(ui, buildHeader, "Android Architecture", ProjectSettingsTypes.ArchitectureAndroid,
		{ "armeabi-v7a", "arm64-v8a", "x86", "x86_64", "Universal" }, self.settings.architectureAndroid), "architectureAndroid", "dropdown");
	self:bind(addSlider(ui, buildHeader, "Minimum iOS Version", ProjectSettingsTypes.MinIOSVersion, 12.0, 18.0, self.settings.minIOSVersion), "minIOSVersion", "intSlider");
	self:bind(addSlider(ui, buildHeader, "Minimum Android SDK", ProjectSettingsTypes.MinAndroidSdk, 21.0, 35.0, self.settings.minAndroidSdk), "minAndroidSdk", "intSlider");
	self:bind(addSlider(ui, buildHeader, "Target Android SDK", ProjectSettingsTypes.TargetAndroidSdk, 26.0, 36.0, self.settings.targetAndroidSdk), "targetAndroidSdk", "intSlider");
	self:bind(addToggle(ui, buildHeader, "Use Android App Bundle", ProjectSettingsTypes.UseAndroidAppBundle, self.settings.useAndroidAppBundle), "useAndroidAppBundle", "toggle");
	self:bind(addEntry(ui, buildHeader, "Signing Profile", ProjectSettingsTypes.SigningProfile), "signingProfile", "text"):setText(self.settings.signingProfile);
	self:bind(addToggle(ui, buildHeader, "Strip Symbols", ProjectSettingsTypes.StripSymbols, self.settings.stripSymbols), "stripSymbols", "toggle");
	self:bind(addToggle(ui, buildHeader, "Generate Debug Symbols", ProjectSettingsTypes.GenerateDebugSymbols, self.settings.generateDebugSymbols), "generateDebugSymbols", "toggle");
	self:bind(addToggle(ui, buildHeader, "Include Editor Data", ProjectSettingsTypes.IncludeEditorData, self.settings.includeEditorData), "includeEditorData", "toggle");
	self:bind(addToggle(ui, buildHeader, "Use Asset Bundles", ProjectSettingsTypes.UseAssetBundles, self.settings.useAssetBundles), "useAssetBundles", "toggle");
	self:bind(addDropdown(ui, buildHeader, "Build Compression", ProjectSettingsTypes.BuildCompression,
		{ "None", "Fast", "Balanced", "Maximum" }, self.settings.buildCompression), "buildCompression", "dropdown");

	-- Performance ----------------------------------------------------------
	local performanceHeader = self:addHeader(ui, performanceTab, "Runtime Performance");
	self:bind(addSlider(ui, performanceHeader, "Thread Count", ProjectSettingsTypes.ThreadCount, 1.0, 32.0, self.settings.threadCount), "threadCount", "intSlider");
	self:bind(addToggle(ui, performanceHeader, "Enable Background Tasks", ProjectSettingsTypes.BackgroundTasks, self.settings.backgroundTasks), "backgroundTasks", "toggle");
	self:bind(addSlider(ui, performanceHeader, "Task Priority", ProjectSettingsTypes.TaskPriority, 0.0, 10.0, self.settings.taskPriority), "taskPriority", "intSlider");
	self:bind(addDropdown(ui, performanceHeader, "Job System", ProjectSettingsTypes.JobSystem,
		{ "Engine Thread Pool", "Platform Jobs", "Single Thread", "External" }, self.settings.jobSystem), "jobSystem", "dropdown");
	self:bind(addDropdown(ui, performanceHeader, "Worker Affinity", ProjectSettingsTypes.WorkerAffinity,
		{ "Auto", "Performance Cores", "Efficiency Cores", "Pinned" }, self.settings.workerAffinity), "workerAffinity", "dropdown");
	self:bind(addToggle(ui, performanceHeader, "Render Thread", ProjectSettingsTypes.RenderThread, self.settings.renderThread), "renderThread", "toggle");
	self:bind(addToggle(ui, performanceHeader, "GPU Particles", ProjectSettingsTypes.GpuParticles, self.settings.gpuParticles), "gpuParticles", "toggle");
	self:bind(addToggle(ui, performanceHeader, "Occlusion Culling", ProjectSettingsTypes.OcclusionCulling, self.settings.occlusionCulling), "occlusionCulling", "toggle");
	self:bind(addToggle(ui, performanceHeader, "Static Batching", ProjectSettingsTypes.StaticBatching, self.settings.staticBatching), "staticBatching", "toggle");
	self:bind(addToggle(ui, performanceHeader, "Dynamic Batching", ProjectSettingsTypes.DynamicBatching, self.settings.dynamicBatching), "dynamicBatching", "toggle");
	self:bind(addSlider(ui, performanceHeader, "Memory Budget MB", ProjectSettingsTypes.MemoryBudgetMb, 256.0, 8192.0, self.settings.memoryBudgetMb), "memoryBudgetMb", "intSlider");
	self:bind(addSlider(ui, performanceHeader, "GPU Memory Budget MB", ProjectSettingsTypes.GpuMemoryBudgetMb, 128.0, 8192.0, self.settings.gpuMemoryBudgetMb), "gpuMemoryBudgetMb", "intSlider");

	-- Scripting / services -------------------------------------------------
	local scriptingHeader = self:addHeader(ui, servicesTab, "Scripting");
	self:bind(addDropdown(ui, scriptingHeader, "Script Backend", ProjectSettingsTypes.ScriptBackend,
		{ "Lua", "C++ Native", "Hybrid Lua/C++", "External VM" }, self.settings.scriptBackend), "scriptBackend", "dropdown");
	self:bind(addToggle(ui, scriptingHeader, "Lua Enabled", ProjectSettingsTypes.LuaEnabled, self.settings.luaEnabled), "luaEnabled", "toggle");
	self:bind(addToggle(ui, scriptingHeader, "Hot Reload Scripts", ProjectSettingsTypes.HotReloadScripts, self.settings.hotReloadScripts), "hotReloadScripts", "toggle");
	self:bind(addDropdown(ui, scriptingHeader, "Reflection Mode", ProjectSettingsTypes.ReflectionMode,
		{ "Off", "Runtime", "Generated", "Runtime + Generated" }, self.settings.reflectionMode), "reflectionMode", "dropdown");

	local servicesHeader = self:addHeader(ui, servicesTab, "Networking / Services");
	self:bind(addDropdown(ui, servicesHeader, "Network Backend", ProjectSettingsTypes.NetworkBackend,
		{ "None", "Engine Sockets", "Boost.Asio", "Platform Service" }, self.settings.networkBackend), "networkBackend", "dropdown");
	self:bind(addToggle(ui, servicesHeader, "Enable Multiplayer", ProjectSettingsTypes.EnableMultiplayer, self.settings.enableMultiplayer), "enableMultiplayer", "toggle");
	self:bind(addToggle(ui, servicesHeader, "Cloud Save", ProjectSettingsTypes.CloudSave, self.settings.cloudSave), "cloudSave", "toggle");
	self:bind(addToggle(ui, servicesHeader, "Analytics", ProjectSettingsTypes.Analytics, self.settings.analytics), "analytics", "toggle");
	self:bind(addToggle(ui, servicesHeader, "Crash Reporting", ProjectSettingsTypes.CrashReporting, self.settings.crashReporting), "crashReporting", "toggle");

	-- Diagnostics ----------------------------------------------------------
	local diagnosticsHeader = self:addHeader(ui, diagnosticsTab, "Diagnostics / Editor");
	self:bind(addDropdown(ui, diagnosticsHeader, "Log Level", ProjectSettingsTypes.LogLevel,
		{ "Trace", "Debug", "Info", "Warning", "Error", "Fatal" }, self.settings.logLevel), "logLevel", "dropdown");
	self:bind(addToggle(ui, diagnosticsHeader, "Profiler Enabled", ProjectSettingsTypes.ProfilerEnabled, self.settings.profilerEnabled), "profilerEnabled", "toggle");
	self:bind(addToggle(ui, diagnosticsHeader, "GPU Profiler", ProjectSettingsTypes.GpuProfiler, self.settings.gpuProfiler), "gpuProfiler", "toggle");
	self:bind(addToggle(ui, diagnosticsHeader, "Validation Warnings", ProjectSettingsTypes.ValidationWarnings, self.settings.validationWarnings), "validationWarnings", "toggle");
	self:bind(addToggle(ui, diagnosticsHeader, "Telemetry", ProjectSettingsTypes.Telemetry, self.settings.telemetry), "telemetry", "toggle");
	self:bind(addDropdown(ui, diagnosticsHeader, "Editor Theme", ProjectSettingsTypes.EditorTheme,
		{ "Classic", "Dark", "Light", "High Contrast" }, self.settings.editorTheme), "editorTheme", "dropdown");
	self:bind(addToggle(ui, diagnosticsHeader, "Autosave", ProjectSettingsTypes.Autosave, self.settings.autosave), "autosave", "toggle");
	self:bind(addSlider(ui, diagnosticsHeader, "Autosave Interval Minutes", ProjectSettingsTypes.AutosaveInterval, 1.0, 60.0, self.settings.autosaveInterval), "autosaveInterval", "intSlider");
	self:bind(addToggle(ui, diagnosticsHeader, "Show Advanced Warnings", ProjectSettingsTypes.ShowAdvancedWarnings, self.settings.showAdvancedWarnings), "showAdvancedWarnings", "toggle");

	-- Advanced / actions ---------------------------------------------------
	local actionsHeader = self:addHeader(ui, advancedTab, "Actions");
	addText(ui, actionsHeader, "Apply updates the current project. Save writes the project file. Startup and backend settings take effect after restart.");
	self.applyButton = addButton(ui, actionsHeader, "Apply", ProjectSettingsTypes.Apply, false);
	self.saveButton = addButton(ui, actionsHeader, "Save", ProjectSettingsTypes.Save, true);
	self.resetButton = addButton(ui, actionsHeader, "Reset", ProjectSettingsTypes.Reset, true);
	self.validateButton = addButton(ui, actionsHeader, "Validate", ProjectSettingsTypes.Validate, false);
	self.reloadRendererButton = addButton(ui, actionsHeader, "Game renderer applies on next launch", ProjectSettingsTypes.ReloadRenderer, true);
	self.reloadRendererButton:setEnabled(false, false);
	self.buildSelectedButton = addButton(ui, actionsHeader, "Build Desktop Plugin", ProjectSettingsTypes.BuildSelected, true);
	self.exportJsonButton = addButton(ui, actionsHeader, "Export JSON", ProjectSettingsTypes.ExportJson, false);
	self.importJsonButton = addButton(ui, actionsHeader, "Import JSON", ProjectSettingsTypes.ImportJson, true);
	self.statusText = addText(ui, actionsHeader, self.statusMessage);

	self:registerEventControls();
	self:syncToControls();

	print("ProjectSettings load end");
end

function ProjectSettings:registerEventControls()
	for _, binding in ipairs(self.controlBindings) do
		if binding.control ~= nil then
			self.window:setHandleEvents(binding.control, true);
		end
	end

	local buttons =
	{
		self.applyButton, self.saveButton, self.resetButton, self.validateButton,
		self.exportJsonButton, self.importJsonButton, self.buildSelectedButton,
		self.reloadRendererButton, self.applyPcProfileButton, self.applyMacProfileButton,
		self.applyIosProfileButton, self.applyAndroidProfileButton
	};

	for _, button in ipairs(buttons) do
		if button ~= nil then
			self.window:setHandleEvents(button, true);
		end
	end
end

function ProjectSettings:getControlValue(control, kind)
	if kind == "text" then
		return control:getText();
	elseif kind == "dropdown" then
		return control:getSelectedOption();
	elseif kind == "toggle" then
		return control:getValue();
	elseif kind == "intSlider" then
		return round(control:getValue());
	elseif kind == "floatSlider" then
		return control:getValue();
	end

	return nil;
end

function ProjectSettings:setControlValue(control, kind, value)
	if control == nil then
		return;
	end

	if kind == "text" then
		control:setText(value or "");
	elseif kind == "dropdown" then
		control:setSelectedOption(round(value or 0));
	elseif kind == "toggle" then
		control:setValue(value == true);
	elseif kind == "intSlider" or kind == "floatSlider" then
		control:setValue(value or 0);
	end
end

function ProjectSettings:syncFromControls()
	for _, binding in ipairs(self.controlBindings) do
		if binding.control ~= nil and self.capabilities[binding.key] ~= false then
			self.settings[binding.key] = self:getControlValue(binding.control, binding.kind);
		end
	end
end

function ProjectSettings:syncToControls()
	self.syncingControls = true;
	for _, binding in ipairs(self.controlBindings) do
		local value = self.settings[binding.key];
		if binding.key == "shadowQuality" then value = value > 0 and 1 or 0; end
		self:setControlValue(binding.control, binding.kind, value);
		local available = self.capabilities[binding.key] ~= false;
		binding.control:setEnabled(available, false);
		local suffix = "";
		if not available then
			suffix = " (Unavailable)";
		end
		binding.control:setLabel(binding.label .. suffix);
	end
	self.syncingControls = false;
	self:updateRendererSummary();
end

function ProjectSettings:saveSettings()
	return self:performAction("saveProjectSettings");
end

function ProjectSettings:updateRendererSummary()
	if self.rendererSummaryText == nil then
		return;
	end

	local activePlatform = safeOption(PlatformNames, self.settings.activePlatform);
	local pcRenderer = safeOption(PcRendererNames, self.settings.rendererPC);
	local macRenderer = safeOption(MacRendererNames, self.settings.rendererMac);
	local iosRenderer = safeOption(IosRendererNames, self.settings.rendererIOS);
	local androidRenderer = safeOption(AndroidRendererNames, self.settings.rendererAndroid);

	self.rendererSummaryText:setText(string.format(
		"Active: %s | PC: %s | macOS: %s | iOS: %s | Android: %s",
		activePlatform, pcRenderer, macRenderer, iosRenderer, androidRenderer));
end

function ProjectSettings:validateSettings()
	return self:performAction("validateProjectSettings");
end

function ProjectSettings:applyPlatformProfile(platform)
	-- 0=PC, 1=macOS, 2=iOS, 3=Android
	self:syncFromControls();
	local preserved = {};
	for key, value in pairs(self.settings) do
		if self.capabilities[key] == false then preserved[key] = value; end
	end
	self.settings.activePlatform = platform;

	if platform == 0 then
		self.settings.rendererPC = 0; -- Auto: DirectX/Vulkan chosen by runtime
		self.settings.renderPipeline = 1;
		self.settings.qualityPreset = 2;
		self.settings.renderScale = 1.0;
		self.settings.aa = 2;
		self.settings.shadowQuality = 3;
		self.settings.shadowDistance = 120;
		self.settings.maxTextureSize = 4096;
		self.settings.textureCompressionPC = 2;
		self.settings.frameRateLimit = 144;
		self.settings.memoryBudgetMb = 4096;
		self.settings.gpuMemoryBudgetMb = 2048;
	elseif platform == 1 then
		self.settings.rendererMac = 1; -- Metal
		self.settings.renderPipeline = 1;
		self.settings.qualityPreset = 2;
		self.settings.renderScale = 1.0;
		self.settings.aa = 2;
		self.settings.shadowQuality = 3;
		self.settings.maxTextureSize = 4096;
		self.settings.textureCompressionMac = 2;
		self.settings.architectureMac = 2;
		self.settings.memoryBudgetMb = 4096;
		self.settings.gpuMemoryBudgetMb = 2048;
	elseif platform == 2 then
		self.settings.rendererIOS = 1; -- Metal
		self.settings.renderPipeline = 3;
		self.settings.qualityPreset = 1;
		self.settings.renderScale = 0.85;
		self.settings.aa = 0;
		self.settings.shadowQuality = 2;
		self.settings.shadowDistance = 50;
		self.settings.maxTextureSize = 2048;
		self.settings.textureCompressionIOS = 2;
		self.settings.frameRateLimit = 60;
		self.settings.memoryBudgetMb = 1024;
		self.settings.gpuMemoryBudgetMb = 512;
	elseif platform == 3 then
		self.settings.rendererAndroid = 1; -- Vulkan, with OpenGL ES fallback controlled by allowRendererFallback
		self.settings.renderPipeline = 3;
		self.settings.qualityPreset = 1;
		self.settings.renderScale = 0.75;
		self.settings.aa = 0;
		self.settings.shadowQuality = 1;
		self.settings.shadowDistance = 40;
		self.settings.maxTextureSize = 2048;
		self.settings.textureCompressionAndroid = 2;
		self.settings.frameRateLimit = 60;
		self.settings.memoryBudgetMb = 1024;
		self.settings.gpuMemoryBudgetMb = 512;
		self.settings.useAndroidAppBundle = true;
	end

	for key, value in pairs(preserved) do self.settings[key] = value; end
	self:syncToControls();
	self:setStatus(safeOption(PlatformNames, platform) .. " profile staged. Review settings, then Apply or Save. The current toolchain builds PC projects.");
end

function ProjectSettings:applySettings(updateStatus)
	return self:performAction("applyProjectSettings", updateStatus);
end

function ProjectSettings:resetToDefaults()
	for key, value in pairs(ProjectSettingsDefaults) do
		if self.capabilities[key] ~= false then self.settings[key] = value; end
	end

	self:syncToControls();
	self:setStatus("Defaults restored for available settings. Apply or Save to keep these changes.");
end

function ProjectSettings:unload()
	print("ProjectSettings unload called");

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
	end

	self:_clearRefs();
	print("ProjectSettings unload end");
end

function ProjectSettings:getProperties(parameters)
	local properties = parameters:at(0);
	self:syncFromControls();
	self:writeSettingsProperties(properties);
end

function ProjectSettings:setProperties(parameters)
	local properties = parameters:at(0);
	self:readSettingsProperties(properties);
	self:syncToControls();
end

function ProjectSettings:show()
	print("ProjectSettings show called");
	if self.editorWindow then
		self.editorWindow:setVisible(true, false);
	end
end

function ProjectSettings:hide()
	print("ProjectSettings hide called");
	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
	end
end

function ProjectSettings:performAction(action, updateStatus)
	table.insert(self.actionLog, action);
	local ok, request = pcall(function()
		local applicationManager = IApplicationManager.instance();
		if applicationManager == nil or self.window == nil then
			error("The Editor project settings service is unavailable.");
		end
		local factory = applicationManager:getFactoryManager();
		if factory == nil then
			error("The Editor object factory is unavailable.");
		end
		local properties = factory:make_object("Properties");
		if properties == nil then
			error("Could not create the project settings request.");
		end
		properties:setPropertyAsString("action", action);
		if action ~= "readProjectSettings" and action ~= "importProjectSettingsJson" then
			self:syncFromControls();
			self:writeSettingsProperties(properties);
		end
		self.window:handleEvent(IEvent.UI, StringUtil.getHash32("projectSettingsAction"),
			Parameters(), nil, properties, nil);
		return properties;
	end);
	if not ok then
		self:setStatus("Project settings failed: " .. tostring(request));
		print(self.statusMessage);
		return false;
	end
	if not request:hasProperty("success") then
		self:setStatus("Project settings action is unavailable. Restart the Editor after updating its binaries.");
		print(self.statusMessage);
		return false;
	end
	local success = request:getPropertyAsBool("success");
	if success then
		self:readSettingsProperties(request);
		self:syncToControls();
	end
	if updateStatus ~= false or not success then
		local message = request:getPropertyAsString("status");
		if message == nil or message == "" then
			message = success and "Project settings updated." or "Project settings action failed.";
		end
		self:setStatus(message);
		if not success then print("Project settings: " .. message); end
	end
	return success;
end

function ProjectSettings:handleEvent(parameters, results)
	if parameters == nil or self.syncingControls then
		return;
	end

	local eventHash = parameters:at(1);
	local sender = parameters:at(3);
	if sender == nil then
		return;
	end

	local elementId = sender:getElementId();
	if eventHash == IEvent.handleSelection then
		if elementId == ProjectSettingsTypes.Apply then
			self:applySettings(true);
		elseif elementId == ProjectSettingsTypes.Save then
			self:saveSettings();
		elseif elementId == ProjectSettingsTypes.Reset then
			self:resetToDefaults();
		elseif elementId == ProjectSettingsTypes.Validate then
			self:validateSettings();
		elseif elementId == ProjectSettingsTypes.ReloadRenderer then
			self:performAction("reloadRenderer");
		elseif elementId == ProjectSettingsTypes.BuildSelected then
			self:performAction("buildSelectedPlatform");
		elseif elementId == ProjectSettingsTypes.ExportJson then
			self:performAction("exportProjectSettingsJson");
		elseif elementId == ProjectSettingsTypes.ImportJson then
			self:performAction("importProjectSettingsJson");
		elseif elementId == ProjectSettingsTypes.ApplyPcProfile then
			self:applyPlatformProfile(0);
		elseif elementId == ProjectSettingsTypes.ApplyMacProfile then
			self:applyPlatformProfile(1);
		elseif elementId == ProjectSettingsTypes.ApplyIosProfile then
			self:applyPlatformProfile(2);
		elseif elementId == ProjectSettingsTypes.ApplyAndroidProfile then
			self:applyPlatformProfile(3);
		else
			self:handleControlChanged(sender);
		end
	elseif eventHash == IEvent.handleValueChanged or eventHash == IEvent.handlePropertyChanged then
		self:handleControlChanged(sender);
	end
end

function ProjectSettings:handleControlChanged(sender)
	local senderId = sender:getElementId();
	for _, binding in ipairs(self.controlBindings) do
		if binding.control:getElementId() == senderId then
			if self.capabilities[binding.key] == false then
				self:setStatus(self.unavailableReasons[binding.key] or "This setting is unavailable in the current Editor build.");
				self:syncToControls();
				return;
			end
			self:syncFromControls();
			self:updateRendererSummary();
			self:setStatus("Unsaved changes. Apply updates the current project; Save writes the project file.");
			return;
		end
	end
end
