class 'TerrainEditor' (BaseEditor)

TerrainEditorTypes =
{
	None = 0,

	-- Resource slots
	HeightMap = 101,
	LayerBaseTexture = 102,
	LayerNormalTexture = 103,
	LayerRoughnessTexture = 104,
	LayerMaskTexture = 105,
	LayerHeightTexture = 106,
	LayerAOTexture = 107,
	LayerSplatTexture = 108,
	BiomeMaskTexture = 109,
	WaterNormalTexture = 110,
	RoadMaskTexture = 111,
	ErosionMaskTexture = 112,

	-- Terrain layer buttons
	AddLayer = 201,
	RemoveLayer = 202,
	Refresh = 203,
	DuplicateLayer = 204,
	MoveLayerUp = 205,
	MoveLayerDown = 206,
	BakeLayerMaps = 207,
	ClearLayerMask = 208,

	SelectedLayer = 301,

	-- Terrain shape / heightmap
	TerrainSizeX = 320,
	TerrainSizeY = 321,
	TerrainSizeZ = 322,
	HeightScale = 323,
	HeightmapResolution = 324,
	TerrainResolution = 325,
	ChunkSize = 326,
	WorldOriginX = 327,
	WorldOriginY = 328,
	WorldOriginZ = 329,
	RebuildTerrain = 330,
	ImportHeightmap = 331,
	ExportHeightmap = 332,
	BakeNormals = 333,
	BakeHoles = 334,
	FitTerrainToSelection = 335,

	-- Sculpting
	BrushMode = 350,
	BrushSize = 351,
	BrushStrength = 352,
	BrushFalloff = 353,
	BrushOpacity = 354,
	BrushSpacing = 355,
	TargetHeight = 356,
	SculptUseTablet = 357,
	SculptMirrorX = 358,
	SculptMirrorZ = 359,
	SculptStampPath = 360,
	SculptApplyRaise = 361,
	SculptApplyLower = 362,
	SculptApplySmooth = 363,
	SculptApplyFlatten = 364,
	SculptApplyTerrace = 365,
	SculptApplyNoise = 366,
	SculptApplyStamp = 367,
	SculptClear = 368,
	BrushCentreX = 9001,
	BrushCentreZ = 9002,

	-- Layer paint / material
	LayerBlendMode = 370,
	LayerUVSet = 371,
	LayerProjection = 372,
	LayerTilingX = 373,
	LayerTilingY = 374,
	LayerOffsetX = 375,
	LayerOffsetY = 376,
	LayerRotation = 377,
	LayerTriplanar = 378,
	LayerTriplanarScale = 379,
	LayerNormalStrength = 380,
	LayerRoughness = 381,
	LayerMetallic = 382,
	LayerAO = 383,
	LayerHeightBlend = 384,
	LayerHeightContrast = 385,
	LayerSlopeMin = 386,
	LayerSlopeMax = 387,
	LayerHeightMin = 388,
	LayerHeightMax = 389,
	LayerPaintSize = 390,
	LayerPaintStrength = 391,
	LayerPaintHardness = 392,
	AutoPaintLayer = 393,
	ClearLayerPaint = 394,
	NormalizeLayerWeights = 395,

	-- Foliage - trees
	TreeEnabled = 401,
	TreeDensity = 402,
	TreePreviewCount = 403,
	TreeGeneratedCount = 404,
	TreePrefab = 405,
	AddTreeLayer = 406,
	RemoveTreeLayer = 407,
	SelectedTreeLayer = 408,
	TreeLayerTexture = 409,
	TreeLayerDensity = 410,
	TreeLayerPrefab = 411,
	TreeSlopeMin = 412,
	TreeSlopeMax = 413,
	TreeHeightMin = 414,
	TreeHeightMax = 415,
	TreeMinScale = 416,
	TreeMaxScale = 417,
	TreeRandomYaw = 418,
	TreeAlignToNormal = 419,
	TreeCastShadows = 420,
	TreeGpuInstancing = 421,
	TreeWind = 422,
	TreeCollision = 423,

	-- Foliage - grass
	GrassEnabled = 501,
	GrassDensity = 502,
	AddGrassLayer = 503,
	RemoveGrassLayer = 504,
	SelectedGrassLayer = 505,
	GrassLayerTexture = 506,
	GrassLayerDensity = 507,
	GrassLayerPrefab = 508,
	GrassSlopeMin = 509,
	GrassSlopeMax = 510,
	GrassHeightMin = 511,
	GrassHeightMax = 512,
	GrassMinScale = 513,
	GrassMaxScale = 514,
	GrassRandomYaw = 515,
	GrassAlignToNormal = 516,
	GrassWind = 517,
	GrassGpuInstancing = 518,
	GrassCastShadows = 519,

	-- Procedural generation
	GeneratorPreset = 600,
	GeneratorSeed = 601,
	NoiseType = 602,
	NoiseScale = 603,
	NoiseOctaves = 604,
	NoisePersistence = 605,
	NoiseLacunarity = 606,
	MountainAmount = 607,
	ValleyAmount = 608,
	PlateauAmount = 609,
	TerraceSteps = 610,
	GenerateHeight = 611,
	GenerateMasks = 612,
	GenerateBiome = 613,
	RandomizeSeed = 614,

	-- Erosion
	HydraulicIterations = 650,
	RainAmount = 651,
	SedimentCapacity = 652,
	ThermalIterations = 653,
	TalusAngle = 654,
	ErosionBrushRadius = 655,
	RunHydraulicErosion = 656,
	RunThermalErosion = 657,
	BakeErosionMasks = 658,

	-- Water / roads
	WaterEnabled = 700,
	WaterHeight = 701,
	WaterDepth = 702,
	WaterFoam = 703,
	WaterReflection = 704,
	WaterRefraction = 705,
	WaterFlowSpeed = 706,
	GenerateWaterPlane = 707,
	RoadSplinePath = 720,
	RoadWidth = 721,
	RoadShoulderWidth = 722,
	RoadFlattenStrength = 723,
	RoadSnapToTerrain = 724,
	GenerateRoad = 725,
	ClearRoads = 726,

	-- LOD / streaming / performance
	LodCount = 800,
	Lod0Distance = 801,
	Lod1Distance = 802,
	Lod2Distance = 803,
	Lod3Distance = 804,
	LodMorph = 805,
	ChunkStreaming = 806,
	StreamingRadius = 807,
	AsyncBuild = 808,
	OcclusionCulling = 809,
	GpuTerrain = 810,
	GpuFoliage = 811,
	TextureStreamingBudget = 812,
	MaxVisibleTrees = 813,
	MaxVisibleGrass = 814,
	RebuildLod = 815,
	BakeImpostors = 816,

	-- Collision / nav / lighting
	CollisionEnabled = 900,
	CollisionResolution = 901,
	CollisionLayer = 902,
	BuildCollision = 903,
	BuildNavMesh = 904,
	NavWalkableSlope = 905,
	NavAgentRadius = 906,
	LightmapResolution = 920,
	TerrainReceivesShadows = 921,
	TerrainCastsShadows = 922,
	BakeLightmapUVs = 923,
	BakeAmbientOcclusion = 924,

	-- Tools / debug
	ValidateTerrain = 1000,
	SaveTerrain = 1001,
	ReloadTerrain = 1002,
	ExportMesh = 1003,
	ExportHeightData = 1004,
	ImportTerrainJson = 1005,
	ExportTerrainJson = 1006,
	TerrainOutputPath = 1007,
	ExportTerrainRecipe = 1008,
	ShowBounds = 1010,
	ShowChunks = 1011,
	ShowNormals = 1012,
	ShowSlope = 1013,
	ShowLayerWeights = 1014,
	ShowFoliageCells = 1015,
	ShowOverdraw = 1016,
	ProfilerEnabled = 1017,
}

local TerrainEditorDefaults =
{
	terrainSizeX = 512.0,
	terrainSizeY = 128.0,
	terrainSizeZ = 512.0,
	heightScale = 100.0,
	heightmapResolution = 1024,
	terrainResolution = 1024,
	chunkSize = 64,
	worldOriginX = 0.0,
	worldOriginY = 0.0,
	worldOriginZ = 0.0,

	brushMode = 0,
	brushSize = 20.0,
	brushCentreX = 0.0,
	brushCentreZ = 0.0,
	brushStrength = 0.35,
	brushFalloff = 0.5,
	brushOpacity = 1.0,
	brushSpacing = 0.1,
	targetHeight = 0.0,
	sculptUseTablet = false,
	sculptMirrorX = false,
	sculptMirrorZ = false,
	sculptStampPath = "",

	layerBlendMode = 0,
	layerUVSet = 0,
	layerProjection = 0,
	layerTilingX = 8.0,
	layerTilingY = 8.0,
	layerOffsetX = 0.0,
	layerOffsetY = 0.0,
	layerRotation = 0.0,
	layerTriplanar = false,
	layerTriplanarScale = 1.0,
	layerNormalStrength = 1.0,
	layerRoughness = 0.75,
	layerMetallic = 0.0,
	layerAO = 1.0,
	layerHeightBlend = 0.0,
	layerHeightContrast = 1.0,
	layerSlopeMin = 0.0,
	layerSlopeMax = 90.0,
	layerHeightMin = -100.0,
	layerHeightMax = 100.0,
	layerPaintSize = 12.0,
	layerPaintStrength = 0.5,
	layerPaintHardness = 0.6,

	treeSlopeMin = 0.0,
	treeSlopeMax = 35.0,
	treeHeightMin = -100.0,
	treeHeightMax = 1000.0,
	treeMinScale = 0.8,
	treeMaxScale = 1.4,
	treeRandomYaw = true,
	treeAlignToNormal = true,
	treeCastShadows = true,
	treeGpuInstancing = true,
	treeWind = true,
	treeCollision = false,

	grassSlopeMin = 0.0,
	grassSlopeMax = 45.0,
	grassHeightMin = -100.0,
	grassHeightMax = 1000.0,
	grassMinScale = 0.7,
	grassMaxScale = 1.25,
	grassRandomYaw = true,
	grassAlignToNormal = true,
	grassWind = true,
	grassGpuInstancing = true,
	grassCastShadows = false,

	generatorPreset = 0,
	generatorSeed = 12345,
	noiseType = 0,
	noiseScale = 120.0,
	noiseOctaves = 5,
	noisePersistence = 0.5,
	noiseLacunarity = 2.0,
	mountainAmount = 0.6,
	valleyAmount = 0.25,
	plateauAmount = 0.1,
	terraceSteps = 6,

	hydraulicIterations = 50000,
	rainAmount = 0.3,
	sedimentCapacity = 0.6,
	thermalIterations = 25000,
	talusAngle = 35.0,
	erosionBrushRadius = 4.0,

	waterEnabled = false,
	waterHeight = 0.0,
	waterDepth = 8.0,
	waterFoam = 0.5,
	waterReflection = true,
	waterRefraction = true,
	waterFlowSpeed = 1.0,
	roadSplinePath = "",
	roadWidth = 6.0,
	roadShoulderWidth = 2.0,
	roadFlattenStrength = 0.75,
	roadSnapToTerrain = true,

	lodCount = 4,
	lod0Distance = 64.0,
	lod1Distance = 128.0,
	lod2Distance = 256.0,
	lod3Distance = 512.0,
	lodMorph = true,
	chunkStreaming = true,
	streamingRadius = 1024.0,
	asyncBuild = true,
	occlusionCulling = true,
	gpuTerrain = false,
	gpuFoliage = true,
	textureStreamingBudget = 512.0,
	maxVisibleTrees = 10000,
	maxVisibleGrass = 50000,

	collisionEnabled = true,
	collisionResolution = 512,
	collisionLayer = "Terrain",
	navWalkableSlope = 45.0,
	navAgentRadius = 0.5,
	lightmapResolution = 512,
	terrainReceivesShadows = true,
	terrainCastsShadows = false,

	showBounds = false,
	showChunks = false,
	showNormals = false,
	showSlope = false,
	showLayerWeights = false,
	showFoliageCells = false,
	showOverdraw = false,
	profilerEnabled = false,
	outputFile = "",
}

local TerrainHeightmapResolutions = { 256, 512, 1024, 2048, 4096, 8192 };
local TerrainControlResolutions = { 256, 512, 1024, 2048, 4096 };
local TerrainChunkSizes = { 16, 32, 64, 128, 256 };
local TerrainCollisionResolutions = { 128, 256, 512, 1024, 2048, 4096 };

local function copyDefaults()
	local copy = {};
	for key, value in pairs(TerrainEditorDefaults) do
		copy[key] = value;
	end
	return copy;
end

local function copyTable(source)
	local copy = {};
	for key, value in pairs(source or {}) do
		copy[key] = value;
	end
	return copy;
end

local function terrainRound(value)
	return math.floor((tonumber(value) or 0) + 0.5);
end

local function terrainJsonEscape(value)
	value = tostring(value or "");
	value = value:gsub("\\", "\\\\");
	value = value:gsub("\"", "\\\"");
	value = value:gsub("\n", "\\n");
	value = value:gsub("\r", "\\r");
	value = value:gsub("\t", "\\t");
	return value;
end

local function terrainSafeFilePart(value)
	value = tostring(value or "terrain");
	value = value:gsub("[^%w%._%-]+", "_");
	value = value:gsub("^_+", "");
	value = value:gsub("_+$", "");
	if value == "" then
		value = "terrain";
	end
	return value;
end

local function resolveOptionValue(options, value, fallback)
	local numeric = tonumber(value);
	if numeric ~= nil then
		local rounded = terrainRound(numeric);
		if rounded >= 0 and rounded < #options then
			return options[rounded + 1];
		end
		for _, optionValue in ipairs(options) do
			if rounded == optionValue then
				return optionValue;
			end
		end
	end

	return fallback or options[1];
end

local function resolveOptionIndex(options, value, fallbackIndex)
	local numeric = tonumber(value);
	if numeric ~= nil then
		local rounded = terrainRound(numeric);
		for index, optionValue in ipairs(options) do
			if rounded == optionValue then
				return index - 1;
			end
		end
		if rounded >= 0 and rounded < #options then
			return rounded;
		end
	end

	return fallbackIndex or 0;
end

local function addText(ui, parent, text, sameLine)
	local textTypeInfo = IUIText.typeInfo();
	local element = ui:addElement(textTypeInfo);
	element:setText(text);
	element:setSameLine(sameLine == true);
	parent:addChild(element);
	return element;
end

local function addSection(ui, parent, label)
	return addText(ui, parent, "-- " .. label .. " --", false);
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

local function addDropdown(ui, parent, label, elementId, options, selected)
	local dropdownTypeInfo = IUIDropdown.typeInfo();
	local dropdown = ui:addElement(dropdownTypeInfo);
	dropdown:setLabel(label);
	dropdown:setElementId(elementId);
	if options ~= nil then
		for _, option in ipairs(options) do
			dropdown:addOption(option);
		end
		if selected ~= nil then
			dropdown:setSelectedOption(selected);
		else
			dropdown:setSelectedOption(0);
		end
	end
	parent:addChild(dropdown);
	return dropdown;
end

local function addToggle(ui, parent, label, elementId, value)
	local toggleTypeInfo = IUILabelTogglePair.typeInfo();
	local toggle = ui:addElement(toggleTypeInfo);
	toggle:setLabel(label);
	toggle:setElementId(elementId);
	toggle:setValue(value == true);
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
	parent:addChild(slider);
	return slider;
end

local function addTextInput(ui, parent, label, elementId, value)
	local textInputTypeInfo = IUILabelTextInputPair.typeInfo();
	local textInput = ui:addElement(textInputTypeInfo);
	textInput:setLabel(label);
	textInput:setElementId(elementId);
	if value ~= nil then
		textInput:setValue(value);
	end
	parent:addChild(textInput);
	return textInput;
end

local function isTexturePath(path)
	if path == nil then
		return false;
	end

	local lowerPath = string.lower(path);
	return StringUtil.contains(lowerPath, ".png") or
		StringUtil.contains(lowerPath, ".tga") or
		StringUtil.contains(lowerPath, ".tiff") or
		StringUtil.contains(lowerPath, ".jpeg") or
		StringUtil.contains(lowerPath, ".jpg") or
		StringUtil.contains(lowerPath, ".exr") or
		StringUtil.contains(lowerPath, ".hdr") or
		StringUtil.contains(lowerPath, ".dds") or
		StringUtil.contains(lowerPath, ".ktx") or
		StringUtil.contains(lowerPath, ".ktx2");
end

local function setResourcePreview(resourceSelect, texture)
	local applicationManager = IApplicationManager.instance();
	local resourceDatabase = applicationManager:getResourceDatabase();
	local previewTexture = texture;

	if previewTexture == nil then
		previewTexture = resourceDatabase:loadResource("checker.png");
	end

	if resourceSelect and resourceSelect.image then
		resourceSelect.image:setTexture(previewTexture);
	end
end

local function safeCall(object, methodName, ...)
	if object == nil then
		return false;
	end

	local okLookup, method = pcall(function() return object[methodName]; end);
	if not okLookup or type(method) ~= "function" then
		return false;
	end

	local okCall, err = pcall(method, object, ...);
	if not okCall then
		print("TerrainEditor optional call failed: " .. methodName .. " - " .. tostring(err));
		return false, nil;
	end

	return true, err;
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

function TerrainEditor:__init(window)
	print("TerrainEditor constructor called");

	BaseEditor:__init(self, window);

	self.window = window;
	self.settings = copyDefaults();
	self.controlBindings = {};
	self.eventControls = {};
	self.resourceSlots = {};
	self.resourcePaths = {};
	self._refNames = {};
	self._exportedFiles = {};
	self._currentStatus = "Select a TerrainSystem to edit.";
	self._lastValidation = "";

	self.terrain = nil;
	self.selectedLayerIndex = 0;
	self.selectedTreeLayerIndex = 0;
	self.selectedGrassLayerIndex = 0;
end

function TerrainEditor:__finalize()
	print("TerrainEditor __finalize called");
	BaseEditor:__finalize();
	self:_clearRefs();
	self.window = nil;
end

function TerrainEditor:ref(name, value)
	self[name] = value;
	self._refNames = self._refNames or {};
	self._refNames[name] = true;
	return value;
end

function TerrainEditor:bind(control, key, valueType)
	if control == nil then
		return control;
	end

	self.eventControls = self.eventControls or {};
	table.insert(self.eventControls, control);

	if self.window ~= nil then
		self.window:setHandleEvents(control, true);
	end

	if key ~= nil then
		self.controlBindings = self.controlBindings or {};
		self.controlBindings[control:getElementId()] = { key = key, valueType = valueType or "value", control = control };
	end

	return control;
end

function TerrainEditor:bindRef(name, control, key, valueType)
	self:ref(name, control);
	return self:bind(control, key, valueType);
end

function TerrainEditor:addResourceSlot(name, uiParent, elementId, label)
	local slot = ResourceSelect(self.window, uiParent, elementId, label);
	slot:setSameLine(false);
	slot:setLabel(label);
	self:ref(name, slot);
	self.resourceSlots = self.resourceSlots or {};
	self.resourceSlots[elementId] = { name = name, slot = slot };
	return slot;
end

function TerrainEditor:_clearRefs()
	if self._refNames ~= nil then
		for name, _ in pairs(self._refNames) do
			self[name] = nil;
		end
	end

	self._refNames = {};
	self.controlBindings = {};
	self.eventControls = {};
	self.resourceSlots = {};
	self.resourcePaths = {};

	self.terrain = nil;
	self.selectedLayerIndex = 0;
	self.selectedTreeLayerIndex = 0;
	self.selectedGrassLayerIndex = 0;
end

function TerrainEditor:getOptionValuesForKey(key)
	if key == "heightmapResolution" then
		return TerrainHeightmapResolutions;
	elseif key == "terrainResolution" then
		return TerrainControlResolutions;
	elseif key == "chunkSize" then
		return TerrainChunkSizes;
	elseif key == "collisionResolution" then
		return TerrainCollisionResolutions;
	end

	return nil;
end

function TerrainEditor:normalizeControlValue(key, value, valueType)
	if valueType == "selected" then
		local options = self:getOptionValuesForKey(key);
		if options ~= nil then
			return resolveOptionValue(options, value, TerrainEditorDefaults[key]);
		end
	elseif valueType == "int" then
		return terrainRound(value);
	end

	return value;
end

function TerrainEditor:getControlValue(control, valueType)
	if control == nil then
		return nil;
	end

	if valueType == "selected" then
		return control:getSelectedOption();
	elseif valueType == "toggle" then
		return control:getValue();
	elseif valueType == "text" then
		return control:getValue();
	elseif valueType == "entry" then
		return control:getText();
	elseif valueType == "int" then
		return math.floor(control:getValue() + 0.5);
	end

	return control:getValue();
end

function TerrainEditor:setControlValue(control, valueType, value, key)
	if control == nil then
		return false;
	end

	if valueType == "selected" then
		local selected = terrainRound(value);
		local options = self:getOptionValuesForKey(key);
		if options ~= nil then
			selected = resolveOptionIndex(options, value, selected);
		end
		return safeCall(control, "setSelectedOption", selected);
	elseif valueType == "toggle" then
		return safeCall(control, "setValue", value == true);
	elseif valueType == "text" then
		local ok = safeCall(control, "setValue", tostring(value or ""));
		if not ok then
			return safeCall(control, "setText", tostring(value or ""));
		end
		return true;
	elseif valueType == "entry" then
		return safeCall(control, "setText", tostring(value or ""));
	elseif valueType == "int" then
		return safeCall(control, "setValue", terrainRound(value));
	end

	return safeCall(control, "setValue", tonumber(value) or 0.0);
end

function TerrainEditor:syncFromControls()
	self.settings = self.settings or copyDefaults();

	for _, binding in pairs(self.controlBindings or {}) do
		if binding.key ~= nil and binding.control ~= nil then
			local ok, value = pcall(function()
				return self:getControlValue(binding.control, binding.valueType);
			end);
			if ok and value ~= nil then
				self.settings[binding.key] = self:normalizeControlValue(binding.key, value, binding.valueType);
			end
		end
	end
end

function TerrainEditor:getProperties(parameters)
	if parameters == nil then
		return;
	end

	local okProperties, properties = pcall(function() return parameters:at(0); end);
	if not okProperties or properties == nil then
		return;
	end

	self:syncFromControls();

	for key, value in pairs(self.settings or {}) do
		if type(value) == "boolean" then
			properties:setPropertyAsBool(key, value);
		elseif type(value) == "number" then
			properties:setPropertyAsFloat(key, value);
		else
			properties:setPropertyAsString(key, tostring(value or ""));
		end
	end

	properties:setPropertyAsFloat("terrain.selectedLayer", self.selectedLayerIndex or 0);
	properties:setPropertyAsFloat("terrain.selectedTreeLayer", self.selectedTreeLayerIndex or 0);
	properties:setPropertyAsFloat("terrain.selectedGrassLayer", self.selectedGrassLayerIndex or 0);
	properties:setPropertyAsString("terrain.status", tostring(self._currentStatus or ""));
	properties:setPropertyAsString("terrain.lastValidation", tostring(self._lastValidation or ""));
	properties:setPropertyAsString("terrain.lastActionName", tostring(self.lastAction and self.lastAction.name or ""));
	properties:setPropertyAsFloat("terrain.lastActionId", self.lastAction and self.lastAction.id or -1);

	properties:setPropertyAsString("terrain.export.terrain", tostring(self._exportedFiles and self._exportedFiles.terrain or ""));
	properties:setPropertyAsString("terrain.export.height", tostring(self._exportedFiles and self._exportedFiles.height or ""));
	properties:setPropertyAsString("terrain.export.mesh", tostring(self._exportedFiles and self._exportedFiles.mesh or ""));
end

function TerrainEditor:setProperties(parameters)
	if parameters == nil then
		return;
	end

	local okProperties, properties = pcall(function() return parameters:at(0); end);
	if not okProperties or properties == nil then
		return;
	end

	self.settings = self.settings or copyDefaults();
	for key, defaultValue in pairs(TerrainEditorDefaults) do
		if properties:hasProperty(key) then
			if type(defaultValue) == "boolean" then
				self.settings[key] = properties:getPropertyAsBool(key);
			elseif type(defaultValue) == "number" then
				self.settings[key] = properties:getPropertyAsFloat(key);
			else
				self.settings[key] = properties:getPropertyAsString(key);
			end
		end
	end

	if properties:hasProperty("terrain.selectedLayer") then
		self.selectedLayerIndex = terrainRound(properties:getPropertyAsFloat("terrain.selectedLayer"));
	end
	if properties:hasProperty("terrain.selectedTreeLayer") then
		self.selectedTreeLayerIndex = terrainRound(properties:getPropertyAsFloat("terrain.selectedTreeLayer"));
	end
	if properties:hasProperty("terrain.selectedGrassLayer") then
		self.selectedGrassLayerIndex = terrainRound(properties:getPropertyAsFloat("terrain.selectedGrassLayer"));
	end
	if properties:hasProperty("terrain.status") then
		self._currentStatus = properties:getPropertyAsString("terrain.status");
	end
	if properties:hasProperty("terrain.lastValidation") then
		self._lastValidation = properties:getPropertyAsString("terrain.lastValidation");
	end

	self.lastAction = self.lastAction or {};
	if properties:hasProperty("terrain.lastActionName") then
		self.lastAction.name = properties:getPropertyAsString("terrain.lastActionName");
	end
	if properties:hasProperty("terrain.lastActionId") then
		self.lastAction.id = terrainRound(properties:getPropertyAsFloat("terrain.lastActionId"));
	end

	self._exportedFiles = self._exportedFiles or {};
	if properties:hasProperty("terrain.export.terrain") then
		self._exportedFiles.terrain = properties:getPropertyAsString("terrain.export.terrain");
	end
	if properties:hasProperty("terrain.export.height") then
		self._exportedFiles.height = properties:getPropertyAsString("terrain.export.height");
	end
	if properties:hasProperty("terrain.export.mesh") then
		self._exportedFiles.mesh = properties:getPropertyAsString("terrain.export.mesh");
	end

	self:syncToControls();
	if self._currentStatus ~= nil and self._currentStatus ~= "" then
		self:setStatus(self._currentStatus);
	else
		self:setStatus("Terrain editor properties restored.");
	end
end

function TerrainEditor:syncToControls()
	for _, binding in pairs(self.controlBindings or {}) do
		if binding.key ~= nil and binding.control ~= nil and self.settings[binding.key] ~= nil then
			self:setControlValue(binding.control, binding.valueType, self.settings[binding.key], binding.key);
		end
	end
end

function TerrainEditor:setStatus(text)
	self._currentStatus = tostring(text or "");
	if self.statusText then
		self.statusText:setText(self._currentStatus);
	end
end

function TerrainEditor:load()
	print("TerrainEditor load start");

	local windowTypeInfo = IUIWindow.typeInfo();
	local tabBarTypeInfo = IUITabBar.typeInfo();
	local collapsingHeaderTypeInfo = IUICollapsingHeader.typeInfo();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	local parentWindow = self.window:getParentWindow();

	if parentWindow == nil then
		print("TerrainEditor load failed: parent window is nil");
		return;
	end

	parentWindow:setSize(Vector2F(780.0, 840.0));

	self:ref("editorWindow", ui:addElement(windowTypeInfo));
	self.editorWindow:setLabel("Terrain Editor");
	self.editorWindow:setSize(Vector2F(780.0, 840.0));
	parentWindow:addChild(self.editorWindow);

	self:ref("statusText", addText(ui, self.editorWindow, "Select a TerrainSystem to edit.", false));

	self:ref("tabBar", ui:addElement(tabBarTypeInfo));
	self.editorWindow:addChild(self.tabBar);

	local terrainTab = self.tabBar:addTabItem(); terrainTab:setLabel("Terrain");
	local sculptTab = self.tabBar:addTabItem(); sculptTab:setLabel("Sculpt");
	local layersTab = self.tabBar:addTabItem(); layersTab:setLabel("Paint/Layers");
	local proceduralTab = self.tabBar:addTabItem(); proceduralTab:setLabel("Procedural");
	local foliageTab = self.tabBar:addTabItem(); foliageTab:setLabel("Foliage");
	local waterRoadsTab = self.tabBar:addTabItem(); waterRoadsTab:setLabel("Water/Roads");
	local lodTab = self.tabBar:addTabItem(); lodTab:setLabel("LOD/Streaming");
	local collisionTab = self.tabBar:addTabItem(); collisionTab:setLabel("Collision/Nav");
	local debugTab = self.tabBar:addTabItem(); debugTab:setLabel("Tools/Debug");

	-- Terrain tab
	local heightHeader = ui:addElement(collapsingHeaderTypeInfo);
	heightHeader:setLabel("Height Map & Terrain Shape");
	terrainTab:addChild(heightHeader);

	self:addResourceSlot("heightMap", heightHeader, TerrainEditorTypes.HeightMap, "Height Map");
	addText(ui, heightHeader, "Drop a height map image onto the preview. Supports PNG/TGA/TIFF/JPEG plus HDR/EXR/DDS/KTX when your resource loader supports them.", false);

	self:bindRef("terrainSizeXSlider", addSlider(ui, heightHeader, "Terrain Size X", TerrainEditorTypes.TerrainSizeX, 1.0, 8192.0, self.settings.terrainSizeX), "terrainSizeX");
	self:bindRef("terrainSizeYSlider", addSlider(ui, heightHeader, "Terrain Size Y", TerrainEditorTypes.TerrainSizeY, 1.0, 2048.0, self.settings.terrainSizeY), "terrainSizeY");
	self:bindRef("terrainSizeZSlider", addSlider(ui, heightHeader, "Terrain Size Z", TerrainEditorTypes.TerrainSizeZ, 1.0, 8192.0, self.settings.terrainSizeZ), "terrainSizeZ");
	self:bindRef("heightScaleSlider", addSlider(ui, heightHeader, "Height Scale", TerrainEditorTypes.HeightScale, 0.01, 1024.0, self.settings.heightScale), "heightScale");
	self:bindRef("heightmapResolutionDropdown", addDropdown(ui, heightHeader, "Heightmap Resolution", TerrainEditorTypes.HeightmapResolution,
		{ "256", "512", "1024", "2048", "4096", "8192" }, 2), "heightmapResolution", "selected");
	self:bindRef("terrainResolutionDropdown", addDropdown(ui, heightHeader, "Control/Blend Resolution", TerrainEditorTypes.TerrainResolution,
		{ "256", "512", "1024", "2048", "4096" }, 2), "terrainResolution", "selected");
	self:bindRef("chunkSizeDropdown", addDropdown(ui, heightHeader, "Chunk Size", TerrainEditorTypes.ChunkSize,
		{ "16", "32", "64", "128", "256" }, 2), "chunkSize", "selected");
	self:bindRef("worldOriginXSlider", addSlider(ui, heightHeader, "World Origin X", TerrainEditorTypes.WorldOriginX, -100000.0, 100000.0, self.settings.worldOriginX), "worldOriginX");
	self:bindRef("worldOriginYSlider", addSlider(ui, heightHeader, "World Origin Y", TerrainEditorTypes.WorldOriginY, -100000.0, 100000.0, self.settings.worldOriginY), "worldOriginY");
	self:bindRef("worldOriginZSlider", addSlider(ui, heightHeader, "World Origin Z", TerrainEditorTypes.WorldOriginZ, -100000.0, 100000.0, self.settings.worldOriginZ), "worldOriginZ");

	local terrainActionsHeader = ui:addElement(collapsingHeaderTypeInfo);
	terrainActionsHeader:setLabel("Build Actions");
	terrainTab:addChild(terrainActionsHeader);
	self:bindRef("rebuildTerrainButton", addButton(ui, terrainActionsHeader, "Rebuild Terrain", TerrainEditorTypes.RebuildTerrain, false));
	self:bindRef("importHeightmapButton", addButton(ui, terrainActionsHeader, "Import Heightmap", TerrainEditorTypes.ImportHeightmap, true));
	self:bindRef("exportHeightmapButton", addButton(ui, terrainActionsHeader, "Export Height Samples (JSON)", TerrainEditorTypes.ExportHeightmap, true));
	self:bindRef("bakeNormalsButton", addButton(ui, terrainActionsHeader, "Bake Normals", TerrainEditorTypes.BakeNormals, false));
	self:bindRef("bakeHolesButton", addButton(ui, terrainActionsHeader, "Bake Holes", TerrainEditorTypes.BakeHoles, true));
	self:bindRef("fitTerrainButton", addButton(ui, terrainActionsHeader, "Fit To Selection", TerrainEditorTypes.FitTerrainToSelection, true));

	-- Sculpt tab
	local brushHeader = ui:addElement(collapsingHeaderTypeInfo);
	brushHeader:setLabel("Brush");
	sculptTab:addChild(brushHeader);
	self:bindRef("brushModeDropdown", addDropdown(ui, brushHeader, "Mode", TerrainEditorTypes.BrushMode,
		{ "Raise/Lower", "Smooth", "Flatten", "Set Height", "Noise", "Terrace", "Erode", "Stamp" }, self.settings.brushMode), "brushMode", "selected");
	self:bindRef("brushSizeSlider", addSlider(ui, brushHeader, "Brush Size", TerrainEditorTypes.BrushSize, 0.1, 512.0, self.settings.brushSize), "brushSize");
	self:bindRef("brushCentreXSlider", addSlider(ui, brushHeader, "Brush Centre X (local metres)", TerrainEditorTypes.BrushCentreX, -8192.0, 8192.0, self.settings.brushCentreX), "brushCentreX");
	self:bindRef("brushCentreZSlider", addSlider(ui, brushHeader, "Brush Centre Z (local metres)", TerrainEditorTypes.BrushCentreZ, -8192.0, 8192.0, self.settings.brushCentreZ), "brushCentreZ");
	addText(ui, brushHeader, "Raise, lower, smooth and flatten apply one undoable action at this centre. Brush Size is the radius. Smooth/flatten strength is 0–1; falloff is smooth. Viewport strokes, tablet and mirror modifiers are unavailable.", false);
	self:bindRef("brushStrengthSlider", addSlider(ui, brushHeader, "Strength", TerrainEditorTypes.BrushStrength, 0.0, 2.0, self.settings.brushStrength), "brushStrength");
	self:bindRef("brushFalloffSlider", addSlider(ui, brushHeader, "Falloff (fixed smooth)", TerrainEditorTypes.BrushFalloff, 0.0, 1.0, self.settings.brushFalloff), "brushFalloff");
	safeCall(self.brushFalloffSlider, "setEnabled", false, true);
	self:bindRef("brushOpacitySlider", addSlider(ui, brushHeader, "Opacity", TerrainEditorTypes.BrushOpacity, 0.0, 1.0, self.settings.brushOpacity), "brushOpacity");
	self:bindRef("brushSpacingSlider", addSlider(ui, brushHeader, "Spacing", TerrainEditorTypes.BrushSpacing, 0.0, 1.0, self.settings.brushSpacing), "brushSpacing");
	self:bindRef("targetHeightSlider", addSlider(ui, brushHeader, "Target Height", TerrainEditorTypes.TargetHeight, -1024.0, 1024.0, self.settings.targetHeight), "targetHeight");
	self:bindRef("sculptUseTabletToggle", addToggle(ui, brushHeader, "Use Tablet Pressure", TerrainEditorTypes.SculptUseTablet, self.settings.sculptUseTablet), "sculptUseTablet", "toggle");
	self:bindRef("sculptMirrorXToggle", addToggle(ui, brushHeader, "Mirror X", TerrainEditorTypes.SculptMirrorX, self.settings.sculptMirrorX), "sculptMirrorX", "toggle");
	self:bindRef("sculptMirrorZToggle", addToggle(ui, brushHeader, "Mirror Z", TerrainEditorTypes.SculptMirrorZ, self.settings.sculptMirrorZ), "sculptMirrorZ", "toggle");
	self:bindRef("sculptStampInput", addTextInput(ui, brushHeader, "Stamp Brush", TerrainEditorTypes.SculptStampPath, self.settings.sculptStampPath), "sculptStampPath", "text");

	local sculptActionsHeader = ui:addElement(collapsingHeaderTypeInfo);
	sculptActionsHeader:setLabel("Brush Actions");
	sculptTab:addChild(sculptActionsHeader);
	self:bindRef("sculptRaiseButton", addButton(ui, sculptActionsHeader, "Raise", TerrainEditorTypes.SculptApplyRaise, false));
	self:bindRef("sculptLowerButton", addButton(ui, sculptActionsHeader, "Lower", TerrainEditorTypes.SculptApplyLower, true));
	self:bindRef("sculptSmoothButton", addButton(ui, sculptActionsHeader, "Smooth", TerrainEditorTypes.SculptApplySmooth, true));
	self:bindRef("sculptFlattenButton", addButton(ui, sculptActionsHeader, "Flatten", TerrainEditorTypes.SculptApplyFlatten, false));
	self:bindRef("sculptTerraceButton", addButton(ui, sculptActionsHeader, "Terrace", TerrainEditorTypes.SculptApplyTerrace, true));
	self:bindRef("sculptNoiseButton", addButton(ui, sculptActionsHeader, "Noise", TerrainEditorTypes.SculptApplyNoise, true));
	self:bindRef("sculptStampButton", addButton(ui, sculptActionsHeader, "Stamp", TerrainEditorTypes.SculptApplyStamp, false));
	self:bindRef("sculptClearButton", addButton(ui, sculptActionsHeader, "Clear Sculpt Mask", TerrainEditorTypes.SculptClear, true));

	-- Paint / layers tab
	local layerHeader = ui:addElement(collapsingHeaderTypeInfo);
	layerHeader:setLabel("Terrain Layers");
	layersTab:addChild(layerHeader);
	self:bindRef("addLayerButton", addButton(ui, layerHeader, "Add Layer", TerrainEditorTypes.AddLayer, false));
	self:bindRef("removeLayerButton", addButton(ui, layerHeader, "Remove Layer", TerrainEditorTypes.RemoveLayer, true));
	self:bindRef("refreshButton", addButton(ui, layerHeader, "Refresh", TerrainEditorTypes.Refresh, true));
	self:bindRef("duplicateLayerButton", addButton(ui, layerHeader, "Duplicate", TerrainEditorTypes.DuplicateLayer, false));
	self:bindRef("moveLayerUpButton", addButton(ui, layerHeader, "Move Up", TerrainEditorTypes.MoveLayerUp, true));
	self:bindRef("moveLayerDownButton", addButton(ui, layerHeader, "Move Down", TerrainEditorTypes.MoveLayerDown, true));
	self:bindRef("selectedLayerDropdown", addDropdown(ui, layerHeader, "Selected Layer", TerrainEditorTypes.SelectedLayer));
	self:ref("layerInfoText", addText(ui, layerHeader, "No terrain selected.", false));

	local layerMapsHeader = ui:addElement(collapsingHeaderTypeInfo);
	layerMapsHeader:setLabel("Layer Texture Maps");
	layersTab:addChild(layerMapsHeader);
	self:addResourceSlot("layerTexture", layerMapsHeader, TerrainEditorTypes.LayerBaseTexture, "Base/Albedo");
	self:addResourceSlot("layerNormalTexture", layerMapsHeader, TerrainEditorTypes.LayerNormalTexture, "Normal");
	self:addResourceSlot("layerRoughnessTexture", layerMapsHeader, TerrainEditorTypes.LayerRoughnessTexture, "Roughness");
	self:addResourceSlot("layerAOTexture", layerMapsHeader, TerrainEditorTypes.LayerAOTexture, "AO");
	self:addResourceSlot("layerHeightTexture", layerMapsHeader, TerrainEditorTypes.LayerHeightTexture, "Height");
	self:addResourceSlot("layerMaskTexture", layerMapsHeader, TerrainEditorTypes.LayerMaskTexture, "Mask");
	self:addResourceSlot("layerSplatTexture", layerMapsHeader, TerrainEditorTypes.LayerSplatTexture, "Splat/Control Map");
	addText(ui, layerMapsHeader, "Drop textures onto a slot. Existing base texture assignment still uses TerrainLayer:setBaseTexture(). Other maps are stored and use optional bindings when available.", false);

	local layerUVHeader = ui:addElement(collapsingHeaderTypeInfo);
	layerUVHeader:setLabel("Layer UV / Material");
	layersTab:addChild(layerUVHeader);
	self:bindRef("layerBlendModeDropdown", addDropdown(ui, layerUVHeader, "Blend Mode", TerrainEditorTypes.LayerBlendMode,
		{ "Alpha", "Height Blend", "Slope Blend", "Additive", "Multiply" }, self.settings.layerBlendMode), "layerBlendMode", "selected");
	self:bindRef("layerUVSetDropdown", addDropdown(ui, layerUVHeader, "UV Set", TerrainEditorTypes.LayerUVSet,
		{ "UV0", "UV1", "World XZ", "Generated" }, self.settings.layerUVSet), "layerUVSet", "selected");
	self:bindRef("layerProjectionDropdown", addDropdown(ui, layerUVHeader, "Projection", TerrainEditorTypes.LayerProjection,
		{ "UV", "World Planar", "Triplanar", "Cylindrical" }, self.settings.layerProjection), "layerProjection", "selected");
	self:bindRef("layerTilingXSlider", addSlider(ui, layerUVHeader, "UV Tiling X", TerrainEditorTypes.LayerTilingX, 0.001, 512.0, self.settings.layerTilingX), "layerTilingX");
	self:bindRef("layerTilingYSlider", addSlider(ui, layerUVHeader, "UV Tiling Y", TerrainEditorTypes.LayerTilingY, 0.001, 512.0, self.settings.layerTilingY), "layerTilingY");
	self:bindRef("layerOffsetXSlider", addSlider(ui, layerUVHeader, "UV Offset X", TerrainEditorTypes.LayerOffsetX, -100.0, 100.0, self.settings.layerOffsetX), "layerOffsetX");
	self:bindRef("layerOffsetYSlider", addSlider(ui, layerUVHeader, "UV Offset Y", TerrainEditorTypes.LayerOffsetY, -100.0, 100.0, self.settings.layerOffsetY), "layerOffsetY");
	self:bindRef("layerRotationSlider", addSlider(ui, layerUVHeader, "UV Rotation", TerrainEditorTypes.LayerRotation, -360.0, 360.0, self.settings.layerRotation), "layerRotation");
	self:bindRef("layerTriplanarToggle", addToggle(ui, layerUVHeader, "Use Triplanar", TerrainEditorTypes.LayerTriplanar, self.settings.layerTriplanar), "layerTriplanar", "toggle");
	self:bindRef("layerTriplanarScaleSlider", addSlider(ui, layerUVHeader, "Triplanar Scale", TerrainEditorTypes.LayerTriplanarScale, 0.001, 512.0, self.settings.layerTriplanarScale), "layerTriplanarScale");
	self:bindRef("layerNormalStrengthSlider", addSlider(ui, layerUVHeader, "Normal Strength", TerrainEditorTypes.LayerNormalStrength, 0.0, 4.0, self.settings.layerNormalStrength), "layerNormalStrength");
	self:bindRef("layerRoughnessSlider", addSlider(ui, layerUVHeader, "Roughness", TerrainEditorTypes.LayerRoughness, 0.0, 1.0, self.settings.layerRoughness), "layerRoughness");
	self:bindRef("layerMetallicSlider", addSlider(ui, layerUVHeader, "Metallic", TerrainEditorTypes.LayerMetallic, 0.0, 1.0, self.settings.layerMetallic), "layerMetallic");
	self:bindRef("layerAOSlider", addSlider(ui, layerUVHeader, "AO Strength", TerrainEditorTypes.LayerAO, 0.0, 4.0, self.settings.layerAO), "layerAO");
	self:bindRef("layerHeightBlendSlider", addSlider(ui, layerUVHeader, "Height Blend", TerrainEditorTypes.LayerHeightBlend, 0.0, 1.0, self.settings.layerHeightBlend), "layerHeightBlend");
	self:bindRef("layerHeightContrastSlider", addSlider(ui, layerUVHeader, "Height Contrast", TerrainEditorTypes.LayerHeightContrast, 0.0, 8.0, self.settings.layerHeightContrast), "layerHeightContrast");

	local autoPaintHeader = ui:addElement(collapsingHeaderTypeInfo);
	autoPaintHeader:setLabel("Auto Paint / Brush");
	layersTab:addChild(autoPaintHeader);
	self:bindRef("layerSlopeMinSlider", addSlider(ui, autoPaintHeader, "Slope Min", TerrainEditorTypes.LayerSlopeMin, 0.0, 90.0, self.settings.layerSlopeMin), "layerSlopeMin");
	self:bindRef("layerSlopeMaxSlider", addSlider(ui, autoPaintHeader, "Slope Max", TerrainEditorTypes.LayerSlopeMax, 0.0, 90.0, self.settings.layerSlopeMax), "layerSlopeMax");
	self:bindRef("layerHeightMinSlider", addSlider(ui, autoPaintHeader, "Height Min", TerrainEditorTypes.LayerHeightMin, -4096.0, 4096.0, self.settings.layerHeightMin), "layerHeightMin");
	self:bindRef("layerHeightMaxSlider", addSlider(ui, autoPaintHeader, "Height Max", TerrainEditorTypes.LayerHeightMax, -4096.0, 4096.0, self.settings.layerHeightMax), "layerHeightMax");
	self:bindRef("layerPaintSizeSlider", addSlider(ui, autoPaintHeader, "Paint Brush Size", TerrainEditorTypes.LayerPaintSize, 0.1, 512.0, self.settings.layerPaintSize), "layerPaintSize");
	self:bindRef("layerPaintStrengthSlider", addSlider(ui, autoPaintHeader, "Paint Strength", TerrainEditorTypes.LayerPaintStrength, 0.0, 1.0, self.settings.layerPaintStrength), "layerPaintStrength");
	self:bindRef("layerPaintHardnessSlider", addSlider(ui, autoPaintHeader, "Paint Hardness", TerrainEditorTypes.LayerPaintHardness, 0.0, 1.0, self.settings.layerPaintHardness), "layerPaintHardness");
	self:bindRef("autoPaintLayerButton", addButton(ui, autoPaintHeader, "Auto Paint Selected Layer", TerrainEditorTypes.AutoPaintLayer, false));
	self:bindRef("clearLayerPaintButton", addButton(ui, autoPaintHeader, "Clear Layer Paint", TerrainEditorTypes.ClearLayerPaint, true));
	self:bindRef("normalizeLayerWeightsButton", addButton(ui, autoPaintHeader, "Normalize Weights", TerrainEditorTypes.NormalizeLayerWeights, true));
	self:bindRef("bakeLayerMapsButton", addButton(ui, autoPaintHeader, "Bake Layer Maps", TerrainEditorTypes.BakeLayerMaps, false));
	self:bindRef("clearLayerMaskButton", addButton(ui, autoPaintHeader, "Clear Mask", TerrainEditorTypes.ClearLayerMask, true));

	-- Procedural tab
	local generationHeader = ui:addElement(collapsingHeaderTypeInfo);
	generationHeader:setLabel("Terrain Generator");
	proceduralTab:addChild(generationHeader);
	self:bindRef("generatorPresetDropdown", addDropdown(ui, generationHeader, "Preset", TerrainEditorTypes.GeneratorPreset,
		{ "Rolling Hills", "Mountain Range", "Island", "Canyons", "Plateau", "City Terrain", "Race Track Base" }, self.settings.generatorPreset), "generatorPreset", "selected");
	self:bindRef("generatorSeedSlider", addSlider(ui, generationHeader, "Seed", TerrainEditorTypes.GeneratorSeed, 0.0, 999999.0, self.settings.generatorSeed), "generatorSeed", "int");
	self:bindRef("noiseTypeDropdown", addDropdown(ui, generationHeader, "Noise Type", TerrainEditorTypes.NoiseType,
		{ "Perlin", "Simplex", "Ridged", "Voronoi", "Domain Warp", "Hybrid" }, self.settings.noiseType), "noiseType", "selected");
	self:bindRef("noiseScaleSlider", addSlider(ui, generationHeader, "Noise Scale", TerrainEditorTypes.NoiseScale, 1.0, 4096.0, self.settings.noiseScale), "noiseScale");
	self:bindRef("noiseOctavesSlider", addSlider(ui, generationHeader, "Octaves", TerrainEditorTypes.NoiseOctaves, 1.0, 12.0, self.settings.noiseOctaves), "noiseOctaves", "int");
	self:bindRef("noisePersistenceSlider", addSlider(ui, generationHeader, "Persistence", TerrainEditorTypes.NoisePersistence, 0.0, 1.0, self.settings.noisePersistence), "noisePersistence");
	self:bindRef("noiseLacunaritySlider", addSlider(ui, generationHeader, "Lacunarity", TerrainEditorTypes.NoiseLacunarity, 1.0, 8.0, self.settings.noiseLacunarity), "noiseLacunarity");
	self:bindRef("mountainAmountSlider", addSlider(ui, generationHeader, "Mountain Amount", TerrainEditorTypes.MountainAmount, 0.0, 1.0, self.settings.mountainAmount), "mountainAmount");
	self:bindRef("valleyAmountSlider", addSlider(ui, generationHeader, "Valley Amount", TerrainEditorTypes.ValleyAmount, 0.0, 1.0, self.settings.valleyAmount), "valleyAmount");
	self:bindRef("plateauAmountSlider", addSlider(ui, generationHeader, "Plateau Amount", TerrainEditorTypes.PlateauAmount, 0.0, 1.0, self.settings.plateauAmount), "plateauAmount");
	self:bindRef("terraceStepsSlider", addSlider(ui, generationHeader, "Terrace Steps", TerrainEditorTypes.TerraceSteps, 1.0, 32.0, self.settings.terraceSteps), "terraceSteps", "int");
	self:bindRef("generateHeightButton", addButton(ui, generationHeader, "Generate Height", TerrainEditorTypes.GenerateHeight, false));
	self:bindRef("generateMasksButton", addButton(ui, generationHeader, "Generate Masks", TerrainEditorTypes.GenerateMasks, true));
	self:bindRef("generateBiomeButton", addButton(ui, generationHeader, "Generate Biome", TerrainEditorTypes.GenerateBiome, true));
	self:bindRef("randomizeSeedButton", addButton(ui, generationHeader, "Randomize Seed", TerrainEditorTypes.RandomizeSeed, false));

	local erosionHeader = ui:addElement(collapsingHeaderTypeInfo);
	erosionHeader:setLabel("Erosion");
	proceduralTab:addChild(erosionHeader);
	self:addResourceSlot("erosionMaskTexture", erosionHeader, TerrainEditorTypes.ErosionMaskTexture, "Erosion Mask");
	self:bindRef("hydraulicIterationsSlider", addSlider(ui, erosionHeader, "Hydraulic Iterations", TerrainEditorTypes.HydraulicIterations, 0.0, 500000.0, self.settings.hydraulicIterations), "hydraulicIterations", "int");
	self:bindRef("rainAmountSlider", addSlider(ui, erosionHeader, "Rain Amount", TerrainEditorTypes.RainAmount, 0.0, 1.0, self.settings.rainAmount), "rainAmount");
	self:bindRef("sedimentCapacitySlider", addSlider(ui, erosionHeader, "Sediment Capacity", TerrainEditorTypes.SedimentCapacity, 0.0, 2.0, self.settings.sedimentCapacity), "sedimentCapacity");
	self:bindRef("thermalIterationsSlider", addSlider(ui, erosionHeader, "Thermal Iterations", TerrainEditorTypes.ThermalIterations, 0.0, 500000.0, self.settings.thermalIterations), "thermalIterations", "int");
	self:bindRef("talusAngleSlider", addSlider(ui, erosionHeader, "Talus Angle", TerrainEditorTypes.TalusAngle, 0.0, 90.0, self.settings.talusAngle), "talusAngle");
	self:bindRef("erosionBrushRadiusSlider", addSlider(ui, erosionHeader, "Brush Radius", TerrainEditorTypes.ErosionBrushRadius, 1.0, 128.0, self.settings.erosionBrushRadius), "erosionBrushRadius");
	self:bindRef("runHydraulicErosionButton", addButton(ui, erosionHeader, "Run Hydraulic", TerrainEditorTypes.RunHydraulicErosion, false));
	self:bindRef("runThermalErosionButton", addButton(ui, erosionHeader, "Run Thermal", TerrainEditorTypes.RunThermalErosion, true));
	self:bindRef("bakeErosionMasksButton", addButton(ui, erosionHeader, "Bake Erosion Masks", TerrainEditorTypes.BakeErosionMasks, true));

	-- Foliage tab
	local treeHeader = ui:addElement(collapsingHeaderTypeInfo);
	treeHeader:setLabel("Trees");
	foliageTab:addChild(treeHeader);
	self:bindRef("treeEnabledToggle", addToggle(ui, treeHeader, "Enable Trees", TerrainEditorTypes.TreeEnabled, false));
	self:bindRef("treeDensitySlider", addSlider(ui, treeHeader, "Tree Density", TerrainEditorTypes.TreeDensity, 0.0, 1000.0, 0.0));
	self:bindRef("treePreviewCountSlider", addSlider(ui, treeHeader, "Preview Tree Count", TerrainEditorTypes.TreePreviewCount, 0.0, 5000.0, 10.0));
	self:bindRef("treeGeneratedCountSlider", addSlider(ui, treeHeader, "Generated Tree Count", TerrainEditorTypes.TreeGeneratedCount, 0.0, 50000.0, 1000.0));
	self:bindRef("treePrefabInput", addTextInput(ui, treeHeader, "Default Tree Prefab", TerrainEditorTypes.TreePrefab, ""));
	self:bindRef("addTreeLayerButton", addButton(ui, treeHeader, "Add Tree Layer", TerrainEditorTypes.AddTreeLayer, false));
	self:bindRef("removeTreeLayerButton", addButton(ui, treeHeader, "Remove Tree Layer", TerrainEditorTypes.RemoveTreeLayer, true));
	self:bindRef("selectedTreeLayerDropdown", addDropdown(ui, treeHeader, "Selected Tree Layer", TerrainEditorTypes.SelectedTreeLayer));
	self:ref("treeInfoText", addText(ui, treeHeader, "No terrain selected.", false));
	self:addResourceSlot("treeLayerTexture", treeHeader, TerrainEditorTypes.TreeLayerTexture, "Tree Layer Texture/Mask");
	self:bindRef("treeLayerDensitySlider", addSlider(ui, treeHeader, "Tree Layer Density", TerrainEditorTypes.TreeLayerDensity, 0.0, 10000.0, 100.0));
	self:bindRef("treeLayerPrefabInput", addTextInput(ui, treeHeader, "Tree Layer Prefab", TerrainEditorTypes.TreeLayerPrefab, ""));
	self:bindRef("treeSlopeMinSlider", addSlider(ui, treeHeader, "Slope Min", TerrainEditorTypes.TreeSlopeMin, 0.0, 90.0, self.settings.treeSlopeMin), "treeSlopeMin");
	self:bindRef("treeSlopeMaxSlider", addSlider(ui, treeHeader, "Slope Max", TerrainEditorTypes.TreeSlopeMax, 0.0, 90.0, self.settings.treeSlopeMax), "treeSlopeMax");
	self:bindRef("treeHeightMinSlider", addSlider(ui, treeHeader, "Height Min", TerrainEditorTypes.TreeHeightMin, -4096.0, 4096.0, self.settings.treeHeightMin), "treeHeightMin");
	self:bindRef("treeHeightMaxSlider", addSlider(ui, treeHeader, "Height Max", TerrainEditorTypes.TreeHeightMax, -4096.0, 4096.0, self.settings.treeHeightMax), "treeHeightMax");
	self:bindRef("treeMinScaleSlider", addSlider(ui, treeHeader, "Min Scale", TerrainEditorTypes.TreeMinScale, 0.01, 10.0, self.settings.treeMinScale), "treeMinScale");
	self:bindRef("treeMaxScaleSlider", addSlider(ui, treeHeader, "Max Scale", TerrainEditorTypes.TreeMaxScale, 0.01, 10.0, self.settings.treeMaxScale), "treeMaxScale");
	self:bindRef("treeRandomYawToggle", addToggle(ui, treeHeader, "Random Yaw", TerrainEditorTypes.TreeRandomYaw, self.settings.treeRandomYaw), "treeRandomYaw", "toggle");
	self:bindRef("treeAlignToNormalToggle", addToggle(ui, treeHeader, "Align To Normal", TerrainEditorTypes.TreeAlignToNormal, self.settings.treeAlignToNormal), "treeAlignToNormal", "toggle");
	self:bindRef("treeCastShadowsToggle", addToggle(ui, treeHeader, "Cast Shadows", TerrainEditorTypes.TreeCastShadows, self.settings.treeCastShadows), "treeCastShadows", "toggle");
	self:bindRef("treeGpuInstancingToggle", addToggle(ui, treeHeader, "GPU Instancing", TerrainEditorTypes.TreeGpuInstancing, self.settings.treeGpuInstancing), "treeGpuInstancing", "toggle");
	self:bindRef("treeWindToggle", addToggle(ui, treeHeader, "Wind", TerrainEditorTypes.TreeWind, self.settings.treeWind), "treeWind", "toggle");
	self:bindRef("treeCollisionToggle", addToggle(ui, treeHeader, "Collision", TerrainEditorTypes.TreeCollision, self.settings.treeCollision), "treeCollision", "toggle");

	local grassHeader = ui:addElement(collapsingHeaderTypeInfo);
	grassHeader:setLabel("Grass / Detail Meshes");
	foliageTab:addChild(grassHeader);
	self:bindRef("grassEnabledToggle", addToggle(ui, grassHeader, "Enable Grass", TerrainEditorTypes.GrassEnabled, false));
	self:bindRef("grassDensitySlider", addSlider(ui, grassHeader, "Grass Density", TerrainEditorTypes.GrassDensity, 0.0, 10000.0, 0.0));
	self:bindRef("addGrassLayerButton", addButton(ui, grassHeader, "Add Grass Layer", TerrainEditorTypes.AddGrassLayer, false));
	self:bindRef("removeGrassLayerButton", addButton(ui, grassHeader, "Remove Grass Layer", TerrainEditorTypes.RemoveGrassLayer, true));
	self:bindRef("selectedGrassLayerDropdown", addDropdown(ui, grassHeader, "Selected Grass Layer", TerrainEditorTypes.SelectedGrassLayer));
	self:ref("grassInfoText", addText(ui, grassHeader, "No terrain selected.", false));
	self:addResourceSlot("grassLayerTexture", grassHeader, TerrainEditorTypes.GrassLayerTexture, "Grass Layer Texture/Mask");
	self:bindRef("grassLayerDensitySlider", addSlider(ui, grassHeader, "Grass Layer Density", TerrainEditorTypes.GrassLayerDensity, 0.0, 10000.0, 100.0));
	self:bindRef("grassLayerPrefabInput", addTextInput(ui, grassHeader, "Grass Layer Prefab", TerrainEditorTypes.GrassLayerPrefab, ""));
	self:bindRef("grassSlopeMinSlider", addSlider(ui, grassHeader, "Slope Min", TerrainEditorTypes.GrassSlopeMin, 0.0, 90.0, self.settings.grassSlopeMin), "grassSlopeMin");
	self:bindRef("grassSlopeMaxSlider", addSlider(ui, grassHeader, "Slope Max", TerrainEditorTypes.GrassSlopeMax, 0.0, 90.0, self.settings.grassSlopeMax), "grassSlopeMax");
	self:bindRef("grassHeightMinSlider", addSlider(ui, grassHeader, "Height Min", TerrainEditorTypes.GrassHeightMin, -4096.0, 4096.0, self.settings.grassHeightMin), "grassHeightMin");
	self:bindRef("grassHeightMaxSlider", addSlider(ui, grassHeader, "Height Max", TerrainEditorTypes.GrassHeightMax, -4096.0, 4096.0, self.settings.grassHeightMax), "grassHeightMax");
	self:bindRef("grassMinScaleSlider", addSlider(ui, grassHeader, "Min Scale", TerrainEditorTypes.GrassMinScale, 0.01, 10.0, self.settings.grassMinScale), "grassMinScale");
	self:bindRef("grassMaxScaleSlider", addSlider(ui, grassHeader, "Max Scale", TerrainEditorTypes.GrassMaxScale, 0.01, 10.0, self.settings.grassMaxScale), "grassMaxScale");
	self:bindRef("grassRandomYawToggle", addToggle(ui, grassHeader, "Random Yaw", TerrainEditorTypes.GrassRandomYaw, self.settings.grassRandomYaw), "grassRandomYaw", "toggle");
	self:bindRef("grassAlignToNormalToggle", addToggle(ui, grassHeader, "Align To Normal", TerrainEditorTypes.GrassAlignToNormal, self.settings.grassAlignToNormal), "grassAlignToNormal", "toggle");
	self:bindRef("grassWindToggle", addToggle(ui, grassHeader, "Wind", TerrainEditorTypes.GrassWind, self.settings.grassWind), "grassWind", "toggle");
	self:bindRef("grassGpuInstancingToggle", addToggle(ui, grassHeader, "GPU Instancing", TerrainEditorTypes.GrassGpuInstancing, self.settings.grassGpuInstancing), "grassGpuInstancing", "toggle");
	self:bindRef("grassCastShadowsToggle", addToggle(ui, grassHeader, "Cast Shadows", TerrainEditorTypes.GrassCastShadows, self.settings.grassCastShadows), "grassCastShadows", "toggle");
	self:addResourceSlot("biomeMaskTexture", foliageTab, TerrainEditorTypes.BiomeMaskTexture, "Biome Mask");

	-- Water and roads tab
	local waterHeader = ui:addElement(collapsingHeaderTypeInfo);
	waterHeader:setLabel("Water");
	waterRoadsTab:addChild(waterHeader);
	self:addResourceSlot("waterNormalTexture", waterHeader, TerrainEditorTypes.WaterNormalTexture, "Water Normal");
	self:bindRef("waterEnabledToggle", addToggle(ui, waterHeader, "Enable Water", TerrainEditorTypes.WaterEnabled, self.settings.waterEnabled), "waterEnabled", "toggle");
	self:bindRef("waterHeightSlider", addSlider(ui, waterHeader, "Water Height", TerrainEditorTypes.WaterHeight, -4096.0, 4096.0, self.settings.waterHeight), "waterHeight");
	self:bindRef("waterDepthSlider", addSlider(ui, waterHeader, "Depth", TerrainEditorTypes.WaterDepth, 0.0, 1024.0, self.settings.waterDepth), "waterDepth");
	self:bindRef("waterFoamSlider", addSlider(ui, waterHeader, "Foam", TerrainEditorTypes.WaterFoam, 0.0, 1.0, self.settings.waterFoam), "waterFoam");
	self:bindRef("waterReflectionToggle", addToggle(ui, waterHeader, "Reflections", TerrainEditorTypes.WaterReflection, self.settings.waterReflection), "waterReflection", "toggle");
	self:bindRef("waterRefractionToggle", addToggle(ui, waterHeader, "Refraction", TerrainEditorTypes.WaterRefraction, self.settings.waterRefraction), "waterRefraction", "toggle");
	self:bindRef("waterFlowSpeedSlider", addSlider(ui, waterHeader, "Flow Speed", TerrainEditorTypes.WaterFlowSpeed, -20.0, 20.0, self.settings.waterFlowSpeed), "waterFlowSpeed");
	self:bindRef("generateWaterPlaneButton", addButton(ui, waterHeader, "Generate Water Plane", TerrainEditorTypes.GenerateWaterPlane, false));

	local roadsHeader = ui:addElement(collapsingHeaderTypeInfo);
	roadsHeader:setLabel("Roads / Rivers / Splines");
	waterRoadsTab:addChild(roadsHeader);
	self:addResourceSlot("roadMaskTexture", roadsHeader, TerrainEditorTypes.RoadMaskTexture, "Road/River Mask");
	self:bindRef("roadSplineInput", addTextInput(ui, roadsHeader, "Road Spline Path", TerrainEditorTypes.RoadSplinePath, self.settings.roadSplinePath), "roadSplinePath", "text");
	self:bindRef("roadWidthSlider", addSlider(ui, roadsHeader, "Road Width", TerrainEditorTypes.RoadWidth, 0.1, 128.0, self.settings.roadWidth), "roadWidth");
	self:bindRef("roadShoulderWidthSlider", addSlider(ui, roadsHeader, "Shoulder Width", TerrainEditorTypes.RoadShoulderWidth, 0.0, 64.0, self.settings.roadShoulderWidth), "roadShoulderWidth");
	self:bindRef("roadFlattenStrengthSlider", addSlider(ui, roadsHeader, "Flatten Strength", TerrainEditorTypes.RoadFlattenStrength, 0.0, 1.0, self.settings.roadFlattenStrength), "roadFlattenStrength");
	self:bindRef("roadSnapToTerrainToggle", addToggle(ui, roadsHeader, "Snap To Terrain", TerrainEditorTypes.RoadSnapToTerrain, self.settings.roadSnapToTerrain), "roadSnapToTerrain", "toggle");
	self:bindRef("generateRoadButton", addButton(ui, roadsHeader, "Generate Road", TerrainEditorTypes.GenerateRoad, false));
	self:bindRef("clearRoadsButton", addButton(ui, roadsHeader, "Clear Roads", TerrainEditorTypes.ClearRoads, true));

	-- LOD / streaming tab
	local lodHeader = ui:addElement(collapsingHeaderTypeInfo);
	lodHeader:setLabel("LOD / Streaming / Platform Performance");
	lodTab:addChild(lodHeader);
	self:bindRef("lodCountSlider", addSlider(ui, lodHeader, "LOD Count", TerrainEditorTypes.LodCount, 1.0, 8.0, self.settings.lodCount), "lodCount", "int");
	self:bindRef("lod0DistanceSlider", addSlider(ui, lodHeader, "LOD0 Distance", TerrainEditorTypes.Lod0Distance, 1.0, 4096.0, self.settings.lod0Distance), "lod0Distance");
	self:bindRef("lod1DistanceSlider", addSlider(ui, lodHeader, "LOD1 Distance", TerrainEditorTypes.Lod1Distance, 1.0, 8192.0, self.settings.lod1Distance), "lod1Distance");
	self:bindRef("lod2DistanceSlider", addSlider(ui, lodHeader, "LOD2 Distance", TerrainEditorTypes.Lod2Distance, 1.0, 16384.0, self.settings.lod2Distance), "lod2Distance");
	self:bindRef("lod3DistanceSlider", addSlider(ui, lodHeader, "LOD3 Distance", TerrainEditorTypes.Lod3Distance, 1.0, 32768.0, self.settings.lod3Distance), "lod3Distance");
	self:bindRef("lodMorphToggle", addToggle(ui, lodHeader, "LOD Morph", TerrainEditorTypes.LodMorph, self.settings.lodMorph), "lodMorph", "toggle");
	self:bindRef("chunkStreamingToggle", addToggle(ui, lodHeader, "Chunk Streaming", TerrainEditorTypes.ChunkStreaming, self.settings.chunkStreaming), "chunkStreaming", "toggle");
	self:bindRef("streamingRadiusSlider", addSlider(ui, lodHeader, "Streaming Radius", TerrainEditorTypes.StreamingRadius, 64.0, 65536.0, self.settings.streamingRadius), "streamingRadius");
	self:bindRef("asyncBuildToggle", addToggle(ui, lodHeader, "Async Build", TerrainEditorTypes.AsyncBuild, self.settings.asyncBuild), "asyncBuild", "toggle");
	self:bindRef("occlusionCullingToggle", addToggle(ui, lodHeader, "Occlusion Culling", TerrainEditorTypes.OcclusionCulling, self.settings.occlusionCulling), "occlusionCulling", "toggle");
	self:bindRef("gpuTerrainToggle", addToggle(ui, lodHeader, "GPU Terrain", TerrainEditorTypes.GpuTerrain, self.settings.gpuTerrain), "gpuTerrain", "toggle");
	self:bindRef("gpuFoliageToggle", addToggle(ui, lodHeader, "GPU Foliage", TerrainEditorTypes.GpuFoliage, self.settings.gpuFoliage), "gpuFoliage", "toggle");
	self:bindRef("textureStreamingBudgetSlider", addSlider(ui, lodHeader, "Texture Streaming MB", TerrainEditorTypes.TextureStreamingBudget, 16.0, 8192.0, self.settings.textureStreamingBudget), "textureStreamingBudget");
	self:bindRef("maxVisibleTreesSlider", addSlider(ui, lodHeader, "Max Visible Trees", TerrainEditorTypes.MaxVisibleTrees, 0.0, 500000.0, self.settings.maxVisibleTrees), "maxVisibleTrees", "int");
	self:bindRef("maxVisibleGrassSlider", addSlider(ui, lodHeader, "Max Visible Grass", TerrainEditorTypes.MaxVisibleGrass, 0.0, 1000000.0, self.settings.maxVisibleGrass), "maxVisibleGrass", "int");
	self:bindRef("rebuildLodButton", addButton(ui, lodHeader, "Rebuild LOD", TerrainEditorTypes.RebuildLod, false));
	self:bindRef("bakeImpostorsButton", addButton(ui, lodHeader, "Bake Impostors", TerrainEditorTypes.BakeImpostors, true));

	-- Collision / navigation / lighting tab
	local collisionHeader = ui:addElement(collapsingHeaderTypeInfo);
	collisionHeader:setLabel("Collision");
	collisionTab:addChild(collisionHeader);
	self:bindRef("collisionEnabledToggle", addToggle(ui, collisionHeader, "Enable Collision", TerrainEditorTypes.CollisionEnabled, self.settings.collisionEnabled), "collisionEnabled", "toggle");
	self:bindRef("collisionResolutionDropdown", addDropdown(ui, collisionHeader, "Collision Resolution", TerrainEditorTypes.CollisionResolution,
		{ "128", "256", "512", "1024", "2048", "4096" }, 2), "collisionResolution", "selected");
	self:bindRef("collisionLayerInput", addTextInput(ui, collisionHeader, "Collision Layer", TerrainEditorTypes.CollisionLayer, self.settings.collisionLayer), "collisionLayer", "text");
	self:bindRef("buildCollisionButton", addButton(ui, collisionHeader, "Build Collision", TerrainEditorTypes.BuildCollision, false));

	local navHeader = ui:addElement(collapsingHeaderTypeInfo);
	navHeader:setLabel("Navigation");
	collisionTab:addChild(navHeader);
	self:bindRef("navWalkableSlopeSlider", addSlider(ui, navHeader, "Walkable Slope", TerrainEditorTypes.NavWalkableSlope, 0.0, 90.0, self.settings.navWalkableSlope), "navWalkableSlope");
	self:bindRef("navAgentRadiusSlider", addSlider(ui, navHeader, "Agent Radius", TerrainEditorTypes.NavAgentRadius, 0.01, 20.0, self.settings.navAgentRadius), "navAgentRadius");
	self:bindRef("buildNavMeshButton", addButton(ui, navHeader, "Build NavMesh", TerrainEditorTypes.BuildNavMesh, false));

	local lightingHeader = ui:addElement(collapsingHeaderTypeInfo);
	lightingHeader:setLabel("Lighting / GI");
	collisionTab:addChild(lightingHeader);
	self:bindRef("lightmapResolutionSlider", addSlider(ui, lightingHeader, "Lightmap Resolution", TerrainEditorTypes.LightmapResolution, 16.0, 4096.0, self.settings.lightmapResolution), "lightmapResolution", "int");
	self:bindRef("terrainReceivesShadowsToggle", addToggle(ui, lightingHeader, "Receive Shadows", TerrainEditorTypes.TerrainReceivesShadows, self.settings.terrainReceivesShadows), "terrainReceivesShadows", "toggle");
	self:bindRef("terrainCastsShadowsToggle", addToggle(ui, lightingHeader, "Cast Shadows", TerrainEditorTypes.TerrainCastsShadows, self.settings.terrainCastsShadows), "terrainCastsShadows", "toggle");
	self:bindRef("bakeLightmapUVsButton", addButton(ui, lightingHeader, "Bake Lightmap UVs", TerrainEditorTypes.BakeLightmapUVs, false));
	self:bindRef("bakeAOButton", addButton(ui, lightingHeader, "Bake Ambient Occlusion", TerrainEditorTypes.BakeAmbientOcclusion, true));

	-- Tools / debug tab
	local toolsHeader = ui:addElement(collapsingHeaderTypeInfo);
	toolsHeader:setLabel("Asset Tools");
	debugTab:addChild(toolsHeader);
	self:bindRef("outputPathInput", addTextInput(ui, toolsHeader, "Output Path", TerrainEditorTypes.TerrainOutputPath, self.settings.outputFile), "outputFile", "text");
	self:bindRef("validateTerrainButton", addButton(ui, toolsHeader, "Validate", TerrainEditorTypes.ValidateTerrain, false));
	self:bindRef("saveTerrainButton", addButton(ui, toolsHeader, "Save", TerrainEditorTypes.SaveTerrain, true));
	self:bindRef("reloadTerrainButton", addButton(ui, toolsHeader, "Reload", TerrainEditorTypes.ReloadTerrain, true));
	self:bindRef("exportMeshButton", addButton(ui, toolsHeader, "Export Mesh", TerrainEditorTypes.ExportMesh, false));
	self:bindRef("exportHeightDataButton", addButton(ui, toolsHeader, "Export Height Data", TerrainEditorTypes.ExportHeightData, true));
	self:bindRef("importJsonButton", addButton(ui, toolsHeader, "Import JSON", TerrainEditorTypes.ImportTerrainJson, false));
	self:bindRef("exportJsonButton", addButton(ui, toolsHeader, "Export JSON", TerrainEditorTypes.ExportTerrainJson, true));
	self:bindRef("exportRecipeButton", addButton(ui, toolsHeader, "Export Recipe Settings", TerrainEditorTypes.ExportTerrainRecipe, false));

	local debugHeader = ui:addElement(collapsingHeaderTypeInfo);
	debugHeader:setLabel("Debug Visualisation");
	debugTab:addChild(debugHeader);
	self:bindRef("showBoundsToggle", addToggle(ui, debugHeader, "Show Bounds", TerrainEditorTypes.ShowBounds, self.settings.showBounds), "showBounds", "toggle");
	self:bindRef("showChunksToggle", addToggle(ui, debugHeader, "Show Chunks", TerrainEditorTypes.ShowChunks, self.settings.showChunks), "showChunks", "toggle");
	self:bindRef("showNormalsToggle", addToggle(ui, debugHeader, "Show Normals", TerrainEditorTypes.ShowNormals, self.settings.showNormals), "showNormals", "toggle");
	self:bindRef("showSlopeToggle", addToggle(ui, debugHeader, "Show Slope", TerrainEditorTypes.ShowSlope, self.settings.showSlope), "showSlope", "toggle");
	self:bindRef("showLayerWeightsToggle", addToggle(ui, debugHeader, "Show Layer Weights", TerrainEditorTypes.ShowLayerWeights, self.settings.showLayerWeights), "showLayerWeights", "toggle");
	self:bindRef("showFoliageCellsToggle", addToggle(ui, debugHeader, "Show Foliage Cells", TerrainEditorTypes.ShowFoliageCells, self.settings.showFoliageCells), "showFoliageCells", "toggle");
	self:bindRef("showOverdrawToggle", addToggle(ui, debugHeader, "Show Overdraw", TerrainEditorTypes.ShowOverdraw, self.settings.showOverdraw), "showOverdraw", "toggle");
	self:bindRef("profilerEnabledToggle", addToggle(ui, debugHeader, "Terrain Profiler", TerrainEditorTypes.ProfilerEnabled, self.settings.profilerEnabled), "profilerEnabled", "toggle");

	self:syncToControls();
	self:updateSelection();

	print("TerrainEditor load end");
end

function TerrainEditor:unload()
	print("TerrainEditor unload called");

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
		self.editorWindow:destroyAllChildren();

		local parent = self.editorWindow:getParent();
		if parent then
			parent:removeChild(self.editorWindow);
		end

		ui:removeElement(self.editorWindow);
	end

	self:_clearRefs();
end

function TerrainEditor:show()
	print("TerrainEditor show called");

	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(true, false);
	end

	if self.editorWindow then
		self.editorWindow:setVisible(true, false);
	end

	local debugWindow = self.window:getDebugWindow();
	if debugWindow then
		debugWindow:setVisible(false, false);
	end
end

function TerrainEditor:hide()
	print("TerrainEditor hide called");

	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
	end

	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(false, false);
	end

	local debugWindow = self.window:getDebugWindow();
	if debugWindow then
		debugWindow:setVisible(false, false);
	end
end

function TerrainEditor:update()
end

function TerrainEditor:getLayerCount()
	if self.terrain == nil then
		return 0;
	end

	local configuredCount = self.terrain:getNumLayers();
	local actualCount = self.terrain:calculateNumLayers();

	if actualCount > configuredCount then
		return actualCount;
	end

	return configuredCount;
end

function TerrainEditor:getSelectedLayer()
	if self.terrain == nil then
		return nil;
	end

	local layerCount = self:getLayerCount();
	if layerCount <= 0 or self.selectedLayerIndex >= layerCount then
		return nil;
	end

	return self.terrain:getSubComponentByIndex(self.selectedLayerIndex);
end

function TerrainEditor:updateLayerControls()
	if self.selectedLayerDropdown == nil then
		return;
	end

	local options = Parameters();
	local layerCount = self:getLayerCount();

	if layerCount <= 0 then
		options:push_back("No layers");
		self.selectedLayerIndex = 0;
	else
		if self.selectedLayerIndex >= layerCount then
			self.selectedLayerIndex = layerCount - 1;
		end

		for i = 0, layerCount - 1 do
			options:push_back("Layer " .. i);
		end
	end

	self.selectedLayerDropdown:setOptions(options:getAsStringArray());
	self.selectedLayerDropdown:setSelectedOption(self.selectedLayerIndex);

	if self.layerInfoText then
		if self.terrain then
			self.layerInfoText:setText("Layers: " .. layerCount .. "    Selected: " .. self.selectedLayerIndex);
		else
			self.layerInfoText:setText("No terrain selected.");
		end
	end

	local layer = self:getSelectedLayer();
	if layer then
		setResourcePreview(self.layerTexture, layer:getBaseTexture());
	else
		setResourcePreview(self.layerTexture, nil);
	end

	self:applySelectedLayerMaterialSettings(false);
end

function TerrainEditor:getFoliageTypeInfo(kind)
	if kind == "tree" then
		return TerrainTreeLayer.typeInfo();
	end

	return TerrainGrassLayer.typeInfo();
end

function TerrainEditor:getFoliageLayers(kind)
	local layers = {};
	if self.terrain == nil then
		return layers;
	end

	local typeInfo = self:getFoliageTypeInfo(kind);
	local size = self.terrain:getNumSubComponents();

	for i = 0, size - 1 do
		local subComponent = self.terrain:getSubComponentByIndex(i);
		if subComponent and subComponent:derived(typeInfo) then
			table.insert(layers, subComponent);
		end
	end

	return layers;
end

function TerrainEditor:getSelectedFoliageLayer(kind)
	local layers = self:getFoliageLayers(kind);
	local selectedIndex = self.selectedTreeLayerIndex;

	if kind == "grass" then
		selectedIndex = self.selectedGrassLayerIndex;
	end

	if #layers <= 0 or selectedIndex >= #layers then
		return nil;
	end

	return layers[selectedIndex + 1];
end

function TerrainEditor:updateFoliageLayerIndices(kind)
	local layers = self:getFoliageLayers(kind);
	for i = 1, #layers do
		layers[i]:setIndex(i - 1);
	end
end

function TerrainEditor:updateFoliageDropdown(dropdown, layers, selectedIndex, emptyLabel)
	if dropdown == nil then
		return selectedIndex;
	end

	local options = Parameters();
	if #layers <= 0 then
		options:push_back(emptyLabel);
		selectedIndex = 0;
	else
		if selectedIndex >= #layers then
			selectedIndex = #layers - 1;
		end

		for i = 0, #layers - 1 do
			options:push_back("Layer " .. i);
		end
	end

	dropdown:setOptions(options:getAsStringArray());
	dropdown:setSelectedOption(selectedIndex);
	return selectedIndex;
end

function TerrainEditor:updateFoliageControls()
	if self.terrain == nil then
		if self.treeInfoText then self.treeInfoText:setText("No terrain selected."); end
		if self.grassInfoText then self.grassInfoText:setText("No terrain selected."); end
		setResourcePreview(self.treeLayerTexture, nil);
		setResourcePreview(self.grassLayerTexture, nil);
		return;
	end

	if self.treeEnabledToggle then self.treeEnabledToggle:setValue(self.terrain:getTreesEnabled()); end
	if self.treeDensitySlider then self.treeDensitySlider:setValue(self.terrain:getTreeDensity()); end
	if self.treePreviewCountSlider then self.treePreviewCountSlider:setValue(self.terrain:getPreviewTreeCount()); end
	if self.treeGeneratedCountSlider then self.treeGeneratedCountSlider:setValue(self.terrain:getGeneratedTreeCount()); end
	if self.treePrefabInput then self.treePrefabInput:setValue(self.terrain:getTreePrefabName()); end
	if self.grassEnabledToggle then self.grassEnabledToggle:setValue(self.terrain:getGrassEnabled()); end
	if self.grassDensitySlider then self.grassDensitySlider:setValue(self.terrain:getGrassDensity()); end

	local treeLayers = self:getFoliageLayers("tree");
	self.selectedTreeLayerIndex = self:updateFoliageDropdown(self.selectedTreeLayerDropdown, treeLayers, self.selectedTreeLayerIndex, "No tree layers");
	local treeLayer = self:getSelectedFoliageLayer("tree");
	if self.treeInfoText then self.treeInfoText:setText("Tree layers: " .. #treeLayers .. "    Selected: " .. self.selectedTreeLayerIndex); end
	if treeLayer then
		setResourcePreview(self.treeLayerTexture, treeLayer:getBaseTexture());
		if self.treeLayerDensitySlider then self.treeLayerDensitySlider:setValue(treeLayer:getDensity()); end
		if self.treeLayerPrefabInput then self.treeLayerPrefabInput:setValue(treeLayer:getPrefabPath()); end
	else
		setResourcePreview(self.treeLayerTexture, nil);
		if self.treeLayerDensitySlider then self.treeLayerDensitySlider:setValue(0); end
		if self.treeLayerPrefabInput then self.treeLayerPrefabInput:setValue(""); end
	end

	local grassLayers = self:getFoliageLayers("grass");
	self.selectedGrassLayerIndex = self:updateFoliageDropdown(self.selectedGrassLayerDropdown, grassLayers, self.selectedGrassLayerIndex, "No grass layers");
	local grassLayer = self:getSelectedFoliageLayer("grass");
	if self.grassInfoText then self.grassInfoText:setText("Grass layers: " .. #grassLayers .. "    Selected: " .. self.selectedGrassLayerIndex); end
	if grassLayer then
		setResourcePreview(self.grassLayerTexture, grassLayer:getBaseTexture());
		if self.grassLayerDensitySlider then self.grassLayerDensitySlider:setValue(grassLayer:getDensity()); end
		if self.grassLayerPrefabInput then self.grassLayerPrefabInput:setValue(grassLayer:getPrefabPath()); end
	else
		setResourcePreview(self.grassLayerTexture, nil);
		if self.grassLayerDensitySlider then self.grassLayerDensitySlider:setValue(0); end
		if self.grassLayerPrefabInput then self.grassLayerPrefabInput:setValue(""); end
	end
end

function TerrainEditor:updateSelection()
	print("TerrainEditor updateSelection called");

	self.terrain = nil;

	local applicationManager = IApplicationManager.instance();
	local selectionManager = applicationManager:getSelectionManager();
	local selection = selectionManager:getSelection();
	local terrainSystemTypeInfo = TerrainSystem.typeInfo();

	local size = selection:size();
	for i = 0, size - 1 do
		local item = selection:at(i);
		if item:derived(terrainSystemTypeInfo) then
			self.terrain = item;
			break;
		end
	end

	if self.heightMap then
		self.heightMap:setTerrain(self.terrain);
	end

	if self.terrain then
		self:syncTerrainDataSettings();
		self:setStatus("Editing selected terrain. Height brush actions support Undo/Redo; Save and Reload use versioned sample data. Other tools report availability when selected.");
		setResourcePreview(self.heightMap, self.terrain:getHeightMap());
	else
		self:setStatus("Select a TerrainSystem to edit.");
		setResourcePreview(self.heightMap, nil);
	end

	self:updateLayerControls();
	--self:updateFoliageControls();
end

function TerrainEditor:addLayer()
	if self.terrain == nil then
		return;
	end

	local layer = self.terrain:addLayer();
	if layer then
		local layerCount = self:getLayerCount();
		self.selectedLayerIndex = layerCount - 1;
		self.terrain:resizeLayermap();
	end

	self:updateLayerControls();
	self:setStatus("Added terrain layer.");
end

function TerrainEditor:removeSelectedLayer()
	if self.terrain == nil then
		return;
	end

	local layerCount = self:getLayerCount();
	if layerCount <= 0 then
		return;
	end

	self.terrain:setNumLayers(layerCount - 1);
	self.terrain:resizeLayermap();

	if self.selectedLayerIndex > 0 then
		self.selectedLayerIndex = self.selectedLayerIndex - 1;
	end

	self:updateLayerControls();
	self:setStatus("Removed terrain layer.");
end

function TerrainEditor:setHeightMapTexture(texture)
	if self.terrain == nil or texture == nil then
		return;
	end

	self.terrain:setHeightMap(texture);
	setResourcePreview(self.heightMap, texture);
	self:setStatus("Height map assigned.");
end

function TerrainEditor:setSelectedLayerTexture(texture)
	local layer = self:getSelectedLayer();
	if layer == nil or texture == nil then
		return;
	end

	layer:setBaseTexture(texture);
	setResourcePreview(self.layerTexture, texture);

	if self.terrain then
		self.terrain:resizeLayermap();
		self.terrain:updateLayers();
	end

	self:setStatus("Layer base texture assigned.");
end

function TerrainEditor:setOptionalLayerTexture(elementId, texture)
	local layer = self:getSelectedLayer();
	local slotInfo = self.resourceSlots and self.resourceSlots[elementId] or nil;
	if slotInfo and slotInfo.slot then
		setResourcePreview(slotInfo.slot, texture);
	end

	if layer == nil or texture == nil then
		return;
	end

	if elementId == TerrainEditorTypes.LayerNormalTexture then
		safeCall(layer, "setNormalTexture", texture);
	elseif elementId == TerrainEditorTypes.LayerRoughnessTexture then
		safeCall(layer, "setRoughnessTexture", texture);
	elseif elementId == TerrainEditorTypes.LayerMaskTexture then
		safeCall(layer, "setMaskTexture", texture);
	elseif elementId == TerrainEditorTypes.LayerHeightTexture then
		safeCall(layer, "setHeightTexture", texture);
	elseif elementId == TerrainEditorTypes.LayerAOTexture then
		safeCall(layer, "setAOTexture", texture);
	elseif elementId == TerrainEditorTypes.LayerSplatTexture then
		safeCall(layer, "setSplatTexture", texture);
	end

	if self.terrain then
		safeCall(self.terrain, "updateLayers");
	end
end

function TerrainEditor:applySelectedLayerMaterialSettings(updateStatus)
	local layer = self:getSelectedLayer();
	if layer == nil then
		return;
	end

	local tiling = safeVector2(self.settings.layerTilingX, self.settings.layerTilingY);
	local offset = safeVector2(self.settings.layerOffsetX, self.settings.layerOffsetY);

	if tiling then
		safeCall(layer, "setUVTiling", tiling);
		safeCall(layer, "setTextureTiling", tiling);
	end
	if offset then
		safeCall(layer, "setUVOffset", offset);
		safeCall(layer, "setTextureOffset", offset);
	end

	safeCall(layer, "setUVRotation", self.settings.layerRotation);
	safeCall(layer, "setTriplanar", self.settings.layerTriplanar);
	safeCall(layer, "setTriplanarScale", self.settings.layerTriplanarScale);
	safeCall(layer, "setNormalStrength", self.settings.layerNormalStrength);
	safeCall(layer, "setRoughness", self.settings.layerRoughness);
	safeCall(layer, "setMetallic", self.settings.layerMetallic);
	safeCall(layer, "setAO", self.settings.layerAO);
	safeCall(layer, "setHeightBlend", self.settings.layerHeightBlend);
	safeCall(layer, "setHeightContrast", self.settings.layerHeightContrast);

	if self.terrain then
		safeCall(self.terrain, "updateLayers");
	end

	if updateStatus ~= false then
		self:setStatus("Layer UV/material settings updated.");
	end
end

function TerrainEditor:addTreeLayer()
	if self.terrain == nil then
		return;
	end

	local layer = self.terrain:addTreeLayer();
	if layer then
		local layers = self:getFoliageLayers("tree");
		self.selectedTreeLayerIndex = #layers - 1;
		self:updateFoliageLayerIndices("tree");
	end

	self:updateFoliageControls();
	self:setStatus("Added tree layer.");
end

function TerrainEditor:addGrassLayer()
	if self.terrain == nil then
		return;
	end

	local layer = self.terrain:addGrassLayer();
	if layer then
		local layers = self:getFoliageLayers("grass");
		self.selectedGrassLayerIndex = #layers - 1;
		self:updateFoliageLayerIndices("grass");
	end

	self:updateFoliageControls();
	self:setStatus("Added grass layer.");
end

function TerrainEditor:removeSelectedFoliageLayer(kind)
	if self.terrain == nil then
		return;
	end

	local layer = self:getSelectedFoliageLayer(kind);
	if layer == nil then
		return;
	end

	self.terrain:removeSubComponent(layer);

	if kind == "tree" then
		if self.selectedTreeLayerIndex > 0 then self.selectedTreeLayerIndex = self.selectedTreeLayerIndex - 1; end
	else
		if self.selectedGrassLayerIndex > 0 then self.selectedGrassLayerIndex = self.selectedGrassLayerIndex - 1; end
	end

	self:updateFoliageLayerIndices(kind);
	self:updateFoliageControls();
	self:setStatus("Removed " .. kind .. " layer.");
end

function TerrainEditor:setSelectedFoliageTexture(kind, texture)
	local layer = self:getSelectedFoliageLayer(kind);
	if layer == nil or texture == nil then
		return;
	end

	layer:setBaseTexture(texture);

	if kind == "tree" then
		setResourcePreview(self.treeLayerTexture, texture);
	else
		setResourcePreview(self.grassLayerTexture, texture);
	end

	if self.terrain then
		self.terrain:updateLayers();
	end
end

function TerrainEditor:setSelectedFoliageDensity(kind, density)
	local layer = self:getSelectedFoliageLayer(kind);
	if layer then
		layer:setDensity(density);
	end
end

function TerrainEditor:setSelectedFoliagePrefab(kind, prefabPath)
	local layer = self:getSelectedFoliageLayer(kind);
	if layer then
		layer:setPrefabPath(prefabPath);
	end
end

function TerrainEditor:getHeightmapResolution()
	return resolveOptionValue(TerrainHeightmapResolutions, self.settings.heightmapResolution, TerrainEditorDefaults.heightmapResolution);
end

function TerrainEditor:getTerrainResolution()
	return resolveOptionValue(TerrainControlResolutions, self.settings.terrainResolution, TerrainEditorDefaults.terrainResolution);
end

function TerrainEditor:getChunkSize()
	return resolveOptionValue(TerrainChunkSizes, self.settings.chunkSize, TerrainEditorDefaults.chunkSize);
end

function TerrainEditor:getCollisionResolution()
	return resolveOptionValue(TerrainCollisionResolutions, self.settings.collisionResolution, TerrainEditorDefaults.collisionResolution);
end

function TerrainEditor:getGeneratorType()
	local preset = terrainRound(self.settings.generatorPreset or 0);
	if preset == 1 or preset == 3 then
		return "mountains";
	elseif preset == 2 then
		return "island";
	end

	return "gradient";
end

function TerrainEditor:applyGeneratedTerrainSettings()
	if self.terrain == nil then
		return false;
	end

	local resolution = self:getHeightmapResolution();
	safeCall(self.terrain, "setGeneratedHeightMapWidth", resolution);
	safeCall(self.terrain, "setGeneratedHeightMapHeight", resolution);
	safeCall(self.terrain, "setGeneratedHeightMapType", self:getGeneratorType());
	return true;
end

function TerrainEditor:syncTerrainDataSettings()
	if self.terrain == nil then return; end
	local sizeOk, size = safeCall(self.terrain, "getHeightMapSize");
	if sizeOk and size then
		self._terrainWidth, self._terrainDepth = size:X(), size:Y();
		-- The generator dropdown cannot describe arbitrary or rectangular authored grids.
		-- Only replace its setting when the current grid matches an offered option.
		if self._terrainWidth == self._terrainDepth then
			for _, resolution in ipairs(TerrainHeightmapResolutions) do
				if resolution == self._terrainWidth then self.settings.heightmapResolution = resolution; break; end
			end
		end
	end
	local scaleOk, scale = safeCall(self.terrain, "getHeightScale");
	if scaleOk then self.settings.heightScale = scale; end
	self:syncToControls();
end

function TerrainEditor:getOutputPath(extension)
	self:syncFromControls();

	local path = tostring(self.settings.outputFile or "");
	if path == "" or path == "None" then
		path = "terrain_" .. terrainSafeFilePart(tostring(self.settings.generatorSeed or "asset"));
	end

	local lowerPath = string.lower(path);
	local knownExtensions = { ".terrain.json", ".height.json", ".mesh.json", ".json" };
	for _, knownExtension in ipairs(knownExtensions) do
		if string.sub(lowerPath, -#knownExtension) == knownExtension then
			path = string.sub(path, 1, #path - #knownExtension);
			break;
		end
	end

	local lastSlash = path:match("^.*()/") or 0;
	local lastBackslash = path:match("^.*()\\") or 0;
	local lastSeparator = math.max(lastSlash, lastBackslash);
	local lastDot = path:match("^.*()%.") or 0;

	if lastDot > lastSeparator then
		path = path:sub(1, lastDot - 1) .. extension;
	else
		path = path .. extension;
	end

	self.settings.outputFile = path;
	if self.outputPathInput then
		self.outputPathInput:setValue(path);
	end

	return path;
end

function TerrainEditor:writeTextFile(path, contents)
	local okApp, applicationManager = pcall(function() return IApplicationManager.instance(); end);
	if okApp and applicationManager then
		local okFs, fileSystem = pcall(function() return applicationManager:getFileSystem(); end);
		if okFs and fileSystem then
			local okWrite = pcall(function() fileSystem:writeAllText(path, contents); end);
			if okWrite then
				return true;
			end
		end
	end

	if io and io.open then
		local file, err = io.open(path, "w");
		if file then
			file:write(contents);
			file:close();
			return true;
		end
		return false, err;
	end

	return false, "No writable file API available.";
end

function TerrainEditor:readTextFile(path)
	local okApp, applicationManager = pcall(function() return IApplicationManager.instance(); end);
	if okApp and applicationManager then
		local okFs, fileSystem = pcall(function() return applicationManager:getFileSystem(); end);
		if okFs and fileSystem then
			local okRead, contents = pcall(function() return fileSystem:readAllText(path); end);
			if okRead then
				return true, contents;
			end
		end
	end

	if io and io.open then
		local file, err = io.open(path, "r");
		if file then
			local contents = file:read("*a");
			file:close();
			return true, contents;
		end
		return false, err;
	end

	return false, "No readable file API available.";
end

function TerrainEditor:encodeJsonValue(value)
	local valueType = type(value);
	if valueType == "string" then
		return "\"" .. terrainJsonEscape(value) .. "\"";
	elseif valueType == "boolean" then
		return value and "true" or "false";
	elseif valueType == "number" then
		return tostring(value);
	elseif value == nil then
		return "null";
	end

	return "\"" .. terrainJsonEscape(tostring(value)) .. "\"";
end

function TerrainEditor:appendSettingsJson(lines, indent, settings)
	local keys = {};
	for key, _ in pairs(settings or {}) do
		keys[#keys + 1] = key;
	end
	table.sort(keys);

	for i = 1, #keys do
		local key = keys[i];
		local comma = i < #keys and "," or "";
		lines[#lines + 1] = indent .. "\"" .. terrainJsonEscape(key) .. "\": " .. self:encodeJsonValue(settings[key]) .. comma;
	end
end

function TerrainEditor:getTerrainSummary()
	local summary =
	{
		layers = self:getLayerCount(),
		treeLayers = #(self:getFoliageLayers("tree") or {}),
		grassLayers = #(self:getFoliageLayers("grass") or {}),
		selectedLayer = self.selectedLayerIndex or 0,
		selectedTreeLayer = self.selectedTreeLayerIndex or 0,
		selectedGrassLayer = self.selectedGrassLayerIndex or 0,
		heightmapResolution = self:getHeightmapResolution(),
		terrainResolution = self:getTerrainResolution(),
		chunkSize = self:getChunkSize(),
		collisionResolution = self:getCollisionResolution(),
	};

	return summary;
end

function TerrainEditor:buildRecipeText(kind)
	self:syncFromControls();

	local summary = self:getTerrainSummary();
	local lines = {};
	lines[#lines + 1] = "{";
	lines[#lines + 1] = "  \"type\": \"" .. terrainJsonEscape(kind or "TerrainRecipe") .. "\",";
	lines[#lines + 1] = "  \"version\": 1,";
	lines[#lines + 1] = "  \"generatorType\": \"" .. terrainJsonEscape(self:getGeneratorType()) .. "\",";
	lines[#lines + 1] = "  \"settings\": {";
	self:appendSettingsJson(lines, "    ", self.settings);
	lines[#lines + 1] = "  },";
	lines[#lines + 1] = "  \"summary\": {";
	lines[#lines + 1] = "    \"layers\": " .. tostring(summary.layers) .. ",";
	lines[#lines + 1] = "    \"treeLayers\": " .. tostring(summary.treeLayers) .. ",";
	lines[#lines + 1] = "    \"grassLayers\": " .. tostring(summary.grassLayers) .. ",";
	lines[#lines + 1] = "    \"selectedLayer\": " .. tostring(summary.selectedLayer) .. ",";
	lines[#lines + 1] = "    \"selectedTreeLayer\": " .. tostring(summary.selectedTreeLayer) .. ",";
	lines[#lines + 1] = "    \"selectedGrassLayer\": " .. tostring(summary.selectedGrassLayer) .. ",";
	lines[#lines + 1] = "    \"heightmapResolution\": " .. tostring(summary.heightmapResolution) .. ",";
	lines[#lines + 1] = "    \"terrainResolution\": " .. tostring(summary.terrainResolution) .. ",";
	lines[#lines + 1] = "    \"chunkSize\": " .. tostring(summary.chunkSize) .. ",";
	lines[#lines + 1] = "    \"collisionResolution\": " .. tostring(summary.collisionResolution);
	lines[#lines + 1] = "  },";
	lines[#lines + 1] = "  \"lastAction\": {";
	lines[#lines + 1] = "    \"id\": " .. tostring(self.lastAction and self.lastAction.id or -1) .. ",";
	lines[#lines + 1] = "    \"name\": \"" .. terrainJsonEscape(self.lastAction and self.lastAction.name or "") .. "\"";
	lines[#lines + 1] = "  }";
	lines[#lines + 1] = "}";

	return table.concat(lines, "\n");
end

function TerrainEditor:writeRecipe(kind, exportedKey)
	-- Append a distinct suffix and retain the sample path used by Save/Reload.
	local samplePath = tostring(self.settings.outputFile or "");
	if samplePath == "" or samplePath == "None" then samplePath = "terrain"; end
	local path = samplePath .. ".recipe.json";
	local ok, err = self:writeTextFile(path, self:buildRecipeText(kind));
	if ok then
		self._exportedFiles = self._exportedFiles or {};
		self._exportedFiles[exportedKey or "terrain"] = path;
		self:setStatus("Wrote terrain recipe settings: " .. path);
		return true;
	end

	self:setStatus("Failed to write terrain data: " .. tostring(err));
	return false;
end

function TerrainEditor:importRecipe(path)
	path = path or self:getOutputPath(".terrain.json");

	local okRead, contentsOrErr = self:readTextFile(path);
	if not okRead then
		self:setStatus("Failed to read terrain JSON: " .. tostring(contentsOrErr));
		return false;
	end

	local okJson, cjson = pcall(require, "cjson");
	if not okJson or cjson == nil then
		self:setStatus("Cannot import terrain JSON because cjson is unavailable.");
		return false;
	end

	local okDecode, decoded = pcall(function() return cjson.decode(contentsOrErr); end);
	if not okDecode or decoded == nil then
		self:setStatus("Failed to decode terrain JSON.");
		return false;
	end
	if decoded.format == "workphone.terrain" then
		return self:runTerrainOperation("importTerrainData", "Imported terrain samples: " .. path, contentsOrErr);
	end

	if decoded.settings ~= nil then
		self.settings = self.settings or copyDefaults();
		for key, value in pairs(decoded.settings) do
			if TerrainEditorDefaults[key] ~= nil then
				self.settings[key] = value;
			end
		end
		self:syncToControls();
		self:setStatus("Imported terrain recipe settings: " .. path .. ". Current samples are unchanged.");
		return true;
	end

	self:setStatus("Terrain JSON did not contain editor settings.");
	return false;
end

function TerrainEditor:validateTerrain()
	local issues = {};
	self:syncFromControls();

	if self.terrain == nil then
		issues[#issues + 1] = "No TerrainSystem selected";
	end
	if tonumber(self.settings.heightScale or 0) <= 0 then
		issues[#issues + 1] = "Height scale must be greater than zero";
	end
	if self:getHeightmapResolution() < 2 then
		issues[#issues + 1] = "Heightmap resolution is too low";
	end
	if (tonumber(self.settings.layerSlopeMin) or 0) > (tonumber(self.settings.layerSlopeMax) or 0) then
		issues[#issues + 1] = "Layer slope min is greater than max";
	end
	if (tonumber(self.settings.treeSlopeMin) or 0) > (tonumber(self.settings.treeSlopeMax) or 0) then
		issues[#issues + 1] = "Tree slope min is greater than max";
	end
	if (tonumber(self.settings.grassSlopeMin) or 0) > (tonumber(self.settings.grassSlopeMax) or 0) then
		issues[#issues + 1] = "Grass slope min is greater than max";
	end

	if #issues == 0 then
		self._lastValidation = "OK";
		self:setStatus("Terrain validation passed.");
		return true;
	end

	self._lastValidation = table.concat(issues, "; ");
	self:setStatus("Terrain validation failed: " .. self._lastValidation);
	return false;
end

function TerrainEditor:runTerrainOperation(method, successMessage, ...)
	if self.terrain == nil then
		self:setStatus("Select a TerrainSystem first.");
		return false;
	end
	local ok, error = safeCall(self.terrain, method, ...);
	if not ok or type(error) ~= "string" then
		self:setStatus("Terrain operation unavailable or failed: " .. method .. ".");
		return false;
	end
	if error ~= "" then
		self:setStatus("Terrain operation failed: " .. error);
		return false;
	end
	if method == "loadTerrainDataFile" or method == "importTerrainData" then self:syncTerrainDataSettings(); end
	self:setStatus(successMessage);
	return true;
end

function TerrainEditor:performAction(elementId)
	local actionNames =
	{
		[TerrainEditorTypes.DuplicateLayer] = "duplicate selected layer",
		[TerrainEditorTypes.MoveLayerUp] = "move layer up",
		[TerrainEditorTypes.MoveLayerDown] = "move layer down",
		[TerrainEditorTypes.BakeLayerMaps] = "bake layer maps",
		[TerrainEditorTypes.ClearLayerMask] = "clear layer mask",
		[TerrainEditorTypes.RebuildTerrain] = "rebuild terrain",
		[TerrainEditorTypes.ImportHeightmap] = "import heightmap",
		[TerrainEditorTypes.ExportHeightmap] = "export heightmap",
		[TerrainEditorTypes.BakeNormals] = "bake terrain normals",
		[TerrainEditorTypes.BakeHoles] = "bake terrain holes",
		[TerrainEditorTypes.FitTerrainToSelection] = "fit terrain to selection",
		[TerrainEditorTypes.SculptApplyRaise] = "apply raise brush",
		[TerrainEditorTypes.SculptApplyLower] = "apply lower brush",
		[TerrainEditorTypes.SculptApplySmooth] = "apply smooth brush",
		[TerrainEditorTypes.SculptApplyFlatten] = "apply flatten brush",
		[TerrainEditorTypes.SculptApplyTerrace] = "apply terrace brush",
		[TerrainEditorTypes.SculptApplyNoise] = "apply noise brush",
		[TerrainEditorTypes.SculptApplyStamp] = "apply stamp brush",
		[TerrainEditorTypes.SculptClear] = "clear sculpt mask",
		[TerrainEditorTypes.AutoPaintLayer] = "auto paint selected layer",
		[TerrainEditorTypes.ClearLayerPaint] = "clear selected layer paint",
		[TerrainEditorTypes.NormalizeLayerWeights] = "normalize layer weights",
		[TerrainEditorTypes.GenerateHeight] = "generate height terrain",
		[TerrainEditorTypes.GenerateMasks] = "generate terrain masks",
		[TerrainEditorTypes.GenerateBiome] = "generate biome placement",
		[TerrainEditorTypes.RandomizeSeed] = "randomize procedural seed",
		[TerrainEditorTypes.RunHydraulicErosion] = "run hydraulic erosion",
		[TerrainEditorTypes.RunThermalErosion] = "run thermal erosion",
		[TerrainEditorTypes.BakeErosionMasks] = "bake erosion masks",
		[TerrainEditorTypes.GenerateWaterPlane] = "generate water plane",
		[TerrainEditorTypes.GenerateRoad] = "generate road spline",
		[TerrainEditorTypes.ClearRoads] = "clear roads",
		[TerrainEditorTypes.RebuildLod] = "rebuild terrain LOD",
		[TerrainEditorTypes.BakeImpostors] = "bake foliage impostors",
		[TerrainEditorTypes.BuildCollision] = "build collision",
		[TerrainEditorTypes.BuildNavMesh] = "build navmesh",
		[TerrainEditorTypes.BakeLightmapUVs] = "bake lightmap UVs",
		[TerrainEditorTypes.BakeAmbientOcclusion] = "bake terrain AO",
		[TerrainEditorTypes.ValidateTerrain] = "validate terrain",
		[TerrainEditorTypes.SaveTerrain] = "save terrain",
		[TerrainEditorTypes.ReloadTerrain] = "reload terrain",
		[TerrainEditorTypes.ExportMesh] = "export terrain mesh",
		[TerrainEditorTypes.ExportHeightData] = "export height data",
		[TerrainEditorTypes.ImportTerrainJson] = "import terrain JSON",
		[TerrainEditorTypes.ExportTerrainJson] = "export terrain JSON",
		[TerrainEditorTypes.ExportTerrainRecipe] = "export terrain recipe settings",
	};

	if elementId == TerrainEditorTypes.RandomizeSeed then
		self.settings.generatorSeed = math.random(0, 999999);
		if self.generatorSeedSlider then
			self.generatorSeedSlider:setValue(self.settings.generatorSeed);
		end
	end

	local actionName = actionNames[elementId];
	if actionName == nil then
		return false;
	end

	self:syncFromControls();
	self.lastAction = { id = elementId, name = actionName, settings = copyTable(self.settings) };

	local brushModes = {
		[TerrainEditorTypes.SculptApplyRaise] = "raise",
		[TerrainEditorTypes.SculptApplyLower] = "lower",
		[TerrainEditorTypes.SculptApplySmooth] = "smooth",
		[TerrainEditorTypes.SculptApplyFlatten] = "flatten",
	};
	local brushMode = brushModes[elementId];
	if brushMode then
		if self.settings.sculptUseTablet or self.settings.sculptMirrorX or self.settings.sculptMirrorZ then
			self:setStatus("Tablet and mirror brush modifiers are unavailable.");
			return false;
		end
		return self:runTerrainOperation("applyHeightBrush", "Applied " .. brushMode .. " brush. Use Edit > Undo/Redo.",
			brushMode, tonumber(self.settings.brushCentreX) or 0, tonumber(self.settings.brushCentreZ) or 0,
			tonumber(self.settings.brushSize) or 0,
			(tonumber(self.settings.brushStrength) or 0) * (tonumber(self.settings.brushOpacity) or 1),
			tonumber(self.settings.targetHeight) or 0);
	elseif elementId == TerrainEditorTypes.SaveTerrain or elementId == TerrainEditorTypes.ExportTerrainJson or elementId == TerrainEditorTypes.ExportHeightData or
		elementId == TerrainEditorTypes.ExportHeightmap then
		local extension = (elementId == TerrainEditorTypes.SaveTerrain or elementId == TerrainEditorTypes.ExportTerrainJson) and ".terrain.json" or ".height.json";
		local path = self:getOutputPath(extension);
		local saved = self:runTerrainOperation("saveTerrainDataFile", "Saved versioned terrain samples: " .. path, path);
		if saved then self._exportedFiles.terrain = path; end
		return saved;
	elseif elementId == TerrainEditorTypes.ReloadTerrain then
		local path = tostring(self.settings.outputFile or "");
		if path == "" then self:setStatus("Set the path of a saved terrain sample file first."); return false; end
		return self:runTerrainOperation("loadTerrainDataFile", "Reloaded terrain samples: " .. path, path);
	elseif elementId == TerrainEditorTypes.ImportTerrainJson then
		local path = tostring(self.settings.outputFile or "");
		if path == "" then self:setStatus("Set the JSON file path first."); return false; end
		return self:importRecipe(path);
	elseif elementId == TerrainEditorTypes.ExportTerrainRecipe then
		return self:writeRecipe("TerrainRecipe", "recipe");
	elseif elementId == TerrainEditorTypes.ValidateTerrain then
		return self:validateTerrain();
	elseif elementId == TerrainEditorTypes.RandomizeSeed then
		self:setStatus("Updated recipe seed; current native height presets do not consume this setting.");
		return true;
	elseif self.terrain and elementId == TerrainEditorTypes.RebuildTerrain then
		local ok, failure = safeCall(self.terrain, "rebuild");
		if ok then self:setStatus("Terrain rebuilt from retained height samples."); return true; end
		self:setStatus("Terrain rebuild failed: " .. tostring(failure)); return false;
	elseif self.terrain and elementId == TerrainEditorTypes.GenerateHeight then
		self:applyGeneratedTerrainSettings();
		local beforeOk, before = safeCall(self.terrain, "getTerrainRevision");
		local generated = safeCall(self.terrain, "generateHeightMap");
		local afterOk, after = safeCall(self.terrain, "getTerrainRevision");
		if generated and beforeOk and afterOk and after ~= before then
			self:syncTerrainDataSettings();
			self:setStatus("Generated terrain height data."); return true;
		end
		self:setStatus("Terrain generation did not publish new height data."); return false;
	end
	self:setStatus("Unavailable: " .. actionName .. ". No terrain data was changed.");
	return false;
end

function TerrainEditor:updateBoundSetting(elementId, sender)
	local binding = self.controlBindings and self.controlBindings[elementId] or nil;
	if binding == nil then
		return false;
	end

	local value = self:getControlValue(sender, binding.valueType);
	self.settings[binding.key] = self:normalizeControlValue(binding.key, value, binding.valueType);

	if self.terrain and elementId == TerrainEditorTypes.HeightScale then
		local requestedScale = tonumber(self.settings.heightScale);
		local applied = safeCall(self.terrain, "setHeightScale", requestedScale);
		self:syncTerrainDataSettings();
		if not applied or self.settings.heightScale ~= requestedScale then
			self:setStatus("Terrain height scale was rejected; existing samples are retained."); return false;
		end
		self:setStatus("Updated terrain height scale; authored grid dimensions are unchanged.");
	elseif self.terrain and elementId == TerrainEditorTypes.HeightmapResolution then
		local resolution = self:getHeightmapResolution();
		local applied = safeCall(self.terrain, "setHeightMapSize", Vector2I(resolution, resolution));
		self:syncTerrainDataSettings();
		if not applied or self._terrainWidth ~= resolution or self._terrainDepth ~= resolution then
			self:setStatus("Terrain grid resize was rejected; existing samples are retained."); return false;
		end
		self:setStatus("Applied explicit terrain grid resize.");
	elseif elementId == TerrainEditorTypes.GeneratorPreset then
		self:applyGeneratedTerrainSettings();
		self:setStatus("Updated generator settings; use Generate to replace height samples.");
	elseif elementId >= TerrainEditorTypes.LayerBlendMode and elementId <= TerrainEditorTypes.LayerHeightMax then
		self:applySelectedLayerMaterialSettings(true);
	else
		self:setStatus("Updated terrain setting: " .. binding.key);
	end

	return true;
end

function TerrainEditor:handleDrop(sender, args)
	print("TerrainEditor handleDrop called");

	if args == nil then
		return;
	end

	local dataStr = nil;
	if args.at then
		local okData, valueData = pcall(function() return args:at(0); end);
		if okData then
			dataStr = valueData;
		end
	else
		dataStr = args.filePath or args.path or args[1];
	end

	if dataStr == nil then
		return;
	end

	local path = nil;
	if type(dataStr) == "table" then
		path = dataStr.filePath or dataStr.path;
	elseif type(dataStr) == "string" then
		local okJson, cjson = pcall(require, "cjson");
		if okJson and cjson ~= nil then
			local okDecode, decoded = pcall(function() return cjson.decode(dataStr); end);
			if okDecode and decoded ~= nil then
				path = decoded.filePath or decoded.path;
			end
		end

		if path == nil then
			path = dataStr:match("\"filePath\"%s*:%s*\"([^\"]+)\"") or
				dataStr:match("\"path\"%s*:%s*\"([^\"]+)\"") or dataStr;
		end
	end

	if path == nil or path == "" then
		return;
	end

	local okId, elementId = safeCall(sender, "getElementId");
	if not okId or elementId == nil then
		return;
	end

	self.resourcePaths = self.resourcePaths or {};
	self.resourcePaths[elementId] = path;

	local lowerPath = string.lower(path);
	if StringUtil.contains(lowerPath, ".json") then
		self.settings.outputFile = path;
		if self.outputPathInput then
			self.outputPathInput:setValue(path);
		end
		self:setStatus("Terrain JSON path set: " .. path);
		return;
	end

	if not isTexturePath(path) then
		return;
	end

	local applicationManager = IApplicationManager.instance();
	local resourceDatabase = applicationManager:getResourceDatabase();
	local texture = resourceDatabase:loadResource(path);

	if elementId == TerrainEditorTypes.HeightMap then
		self:setHeightMapTexture(texture);
	elseif elementId == TerrainEditorTypes.LayerBaseTexture then
		self:setSelectedLayerTexture(texture);
	elseif elementId == TerrainEditorTypes.LayerNormalTexture or elementId == TerrainEditorTypes.LayerRoughnessTexture or
		elementId == TerrainEditorTypes.LayerMaskTexture or elementId == TerrainEditorTypes.LayerHeightTexture or
		elementId == TerrainEditorTypes.LayerAOTexture or elementId == TerrainEditorTypes.LayerSplatTexture then
		self:setOptionalLayerTexture(elementId, texture);
	elseif elementId == TerrainEditorTypes.TreeLayerTexture then
		self:setSelectedFoliageTexture("tree", texture);
	elseif elementId == TerrainEditorTypes.GrassLayerTexture then
		self:setSelectedFoliageTexture("grass", texture);
	else
		local slotInfo = self.resourceSlots and self.resourceSlots[elementId] or nil;
		if slotInfo and slotInfo.slot then
			setResourcePreview(slotInfo.slot, texture);
			self:setStatus("Assigned resource slot: " .. slotInfo.name);
		end
	end
end

function TerrainEditor:handleEvent(parameters, results)
	print("TerrainEditor handleEvent called");

	local visibleOk, visible = safeCall(self.window, "isWindowVisible");
	if visibleOk and not visible then
		return;
	end

	if parameters == nil then
		print("TerrainEditor parameters nil");
		return;
	end

	local eventHash = nil;
	local args = nil;
	local sender = nil;
	local elementId = nil;
	local legacyEventType = nil;

	if parameters.at then
		local okEvent, valueEvent = pcall(function() return parameters:at(1); end);
		local okArgs, valueArgs = pcall(function() return parameters:at(2); end);
		local okSender, valueSender = pcall(function() return parameters:at(3); end);
		if okEvent then eventHash = valueEvent; end
		if okArgs then args = valueArgs; end
		if okSender then sender = valueSender; end
	else
		eventHash = parameters.eventHash or parameters.eventType;
		args = parameters.args or parameters.data;
		sender = parameters.sender or parameters.element;
		elementId = parameters.elementId;
		legacyEventType = parameters.eventType;
	end

	if sender ~= nil then
		local okId, valueId = safeCall(sender, "getElementId");
		if okId then
			elementId = valueId;
		elseif type(sender) == "number" then
			elementId = sender;
		end
	end

	if elementId == nil then
		return;
	end

	local isSelection = eventHash == IEvent.handleSelection
		or legacyEventType == "clicked"
		or legacyEventType == "selection";
	local isValueChanged = eventHash == IEvent.handleValueChanged
		or eventHash == IEvent.handlePropertyChanged
		or legacyEventType == "changed"
		or legacyEventType == "valueChanged";
	local isDrop = eventHash == IEvent.handleDrop
		or legacyEventType == "drop";

	if isSelection then
		if elementId == TerrainEditorTypes.AddLayer then
			self:addLayer();
		elseif elementId == TerrainEditorTypes.RemoveLayer then
			self:removeSelectedLayer();
		elseif elementId == TerrainEditorTypes.Refresh then
			self:updateSelection();
		elseif elementId == TerrainEditorTypes.AddTreeLayer then
			self:addTreeLayer();
		elseif elementId == TerrainEditorTypes.RemoveTreeLayer then
			self:removeSelectedFoliageLayer("tree");
		elseif elementId == TerrainEditorTypes.AddGrassLayer then
			self:addGrassLayer();
		elseif elementId == TerrainEditorTypes.RemoveGrassLayer then
			self:removeSelectedFoliageLayer("grass");
		elseif self.layerTexture and elementId == self.layerTexture.deleteHash then
			local layer = self:getSelectedLayer();
			if layer then layer:setBaseTexture(nil); self:updateLayerControls(); end
		elseif self.treeLayerTexture and elementId == self.treeLayerTexture.deleteHash then
			local layer = self:getSelectedFoliageLayer("tree");
			if layer then layer:setBaseTexture(nil); self:updateFoliageControls(); end
		elseif self.grassLayerTexture and elementId == self.grassLayerTexture.deleteHash then
			local layer = self:getSelectedFoliageLayer("grass");
			if layer then layer:setBaseTexture(nil); self:updateFoliageControls(); end
		elseif self.heightMap and elementId == self.heightMap.deleteHash then
			if self.terrain then self.terrain:setHeightMap(nil); setResourcePreview(self.heightMap, nil); end
		else
			self:performAction(elementId);
		end
	elseif isValueChanged then
		if elementId == TerrainEditorTypes.SelectedLayer then
			local okSelected, selected = safeCall(sender, "getSelectedOption");
			self.selectedLayerIndex = okSelected and selected or self.selectedLayerIndex;
			self:updateLayerControls();
		elseif elementId == TerrainEditorTypes.TreeEnabled then
			local okValue, value = safeCall(sender, "getValue");
			if self.terrain and okValue then self.terrain:setTreesEnabled(value); end
		elseif elementId == TerrainEditorTypes.TreeDensity then
			local okValue, value = safeCall(sender, "getValue");
			if self.terrain and okValue then self.terrain:setTreeDensity(value); end
		elseif elementId == TerrainEditorTypes.TreePreviewCount then
			local okValue, value = safeCall(sender, "getValue");
			if self.terrain and okValue then self.terrain:setPreviewTreeCount(value); end
		elseif elementId == TerrainEditorTypes.TreeGeneratedCount then
			local okValue, value = safeCall(sender, "getValue");
			if self.terrain and okValue then self.terrain:setGeneratedTreeCount(value); end
		elseif elementId == TerrainEditorTypes.TreePrefab then
			local okValue, value = safeCall(sender, "getValue");
			if self.terrain and okValue then self.terrain:setTreePrefabName(value); end
		elseif elementId == TerrainEditorTypes.SelectedTreeLayer then
			local okSelected, selected = safeCall(sender, "getSelectedOption");
			self.selectedTreeLayerIndex = okSelected and selected or self.selectedTreeLayerIndex;
			self:updateFoliageControls();
		elseif elementId == TerrainEditorTypes.TreeLayerDensity then
			local okValue, value = safeCall(sender, "getValue");
			if okValue then self:setSelectedFoliageDensity("tree", value); end
		elseif elementId == TerrainEditorTypes.TreeLayerPrefab then
			local okValue, value = safeCall(sender, "getValue");
			if okValue then self:setSelectedFoliagePrefab("tree", value); end
		elseif elementId == TerrainEditorTypes.GrassEnabled then
			local okValue, value = safeCall(sender, "getValue");
			if self.terrain and okValue then self.terrain:setGrassEnabled(value); end
		elseif elementId == TerrainEditorTypes.GrassDensity then
			local okValue, value = safeCall(sender, "getValue");
			if self.terrain and okValue then self.terrain:setGrassDensity(value); end
		elseif elementId == TerrainEditorTypes.SelectedGrassLayer then
			local okSelected, selected = safeCall(sender, "getSelectedOption");
			self.selectedGrassLayerIndex = okSelected and selected or self.selectedGrassLayerIndex;
			self:updateFoliageControls();
		elseif elementId == TerrainEditorTypes.GrassLayerDensity then
			local okValue, value = safeCall(sender, "getValue");
			if okValue then self:setSelectedFoliageDensity("grass", value); end
		elseif elementId == TerrainEditorTypes.GrassLayerPrefab then
			local okValue, value = safeCall(sender, "getValue");
			if okValue then self:setSelectedFoliagePrefab("grass", value); end
		else
			self:updateBoundSetting(elementId, sender);
		end
	elseif isDrop then
		self:handleDrop(sender, args);
	end

	if self.resourceSlots ~= nil then
		for _, slotInfo in pairs(self.resourceSlots) do
			if slotInfo.slot and (elementId == slotInfo.slot.index or elementId == slotInfo.slot.deleteHash) then
				slotInfo.slot:handleEvent(parameters, results);
			end
		end
	end
end
