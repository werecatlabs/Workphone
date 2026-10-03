class 'MaterialEditor' (BaseEditor)

MaterialEditorTypes =
{
	MATERIAL_TYPE = 10,

	-- Texture slot ids. Keep the original ids for compatibility.
	Albedo = 0,
	Normal = 1,
	Metallic = 2,
	Roughness = 12,
	Emission = 13,
	AmbientOcclusion = 14,
	Height = 15,
	Opacity = 16,
	Mask = 17,
	DetailAlbedo = 18,
	DetailNormal = 19,
	ClearCoat = 20,
	Anisotropy = 21,

	-- Common material widget ids.
	MetalnessSliderId = 100,
	RoughnessSliderId = 101,
	MetallicSourceId = 102,

	TransparentId = 103,
	CutoutId = 104,
	EmissionId = 105,
	RefractionId = 106,

	MetallicColourId = 107,
	EmissionColourId = 108,
	AlbedoColourId = 109,

	FilePathId = 110,
	MaterialTypeId = 111,
	MaterialNameId = 112,
	ShaderPathId = 113,
	PresetId = 114,

	RenderModeId = 120,
	WorkflowId = 121,
	AlphaClipId = 122,
	OpacityId = 123,
	DoubleSidedId = 124,
	ReceiveShadowsId = 125,
	CastShadowsId = 126,
	CullModeId = 127,
	RefractionAmountId = 128,
	RefractionIorId = 129,

	SpecularSliderId = 130,
	NormalStrengthId = 131,
	AoStrengthId = 132,
	HeightScaleId = 133,
	ParallaxStepsId = 134,
	EmissionIntensityId = 135,
	ClearCoatAmountId = 136,
	ClearCoatRoughnessId = 137,
	AnisotropyAmountId = 138,

	-- Main UV transform.
	UVTilingXId = 200,
	UVTilingYId = 201,
	UVOffsetXId = 202,
	UVOffsetYId = 203,
	UVRotationId = 204,
	UVSetId = 205,
	UVProjectionId = 206,
	UVTriplanarScaleId = 207,
	UVWrapUId = 208,
	UVWrapVId = 209,
	UVFilterId = 210,
	UVAnisoId = 211,

	-- Detail UV transform.
	DetailTilingXId = 220,
	DetailTilingYId = 221,
	DetailOffsetXId = 222,
	DetailOffsetYId = 223,
	DetailRotationId = 224,
	DetailBlendModeId = 225,
	DetailStrengthId = 226,
	DetailNormalStrengthId = 227,

	-- Texture packing/import.
	RoughnessSourceId = 300,
	AoSourceId = 301,
	OpacitySourceId = 302,
	HeightSourceId = 303,
	GenerateMipmapsId = 304,
	SrgbId = 305,
	NormalMapId = 306,
	TextureStreamingId = 307,
	MaxTextureSizeId = 308,
	PcCompressionId = 309,
	MacCompressionId = 310,
	IosCompressionId = 311,
	AndroidCompressionId = 312,

	-- Render state.
	BlendModeId = 400,
	SrcBlendId = 401,
	DstBlendId = 402,
	DepthWriteId = 403,
	DepthTestId = 404,
	RenderQueueId = 405,
	SortPriorityId = 406,
	StencilRefId = 407,
	StencilReadMaskId = 408,
	StencilWriteMaskId = 409,
	GpuInstancingId = 410,
	SrpBatcherId = 411,
	ReceiveDecalsId = 412,

	-- Layering/variants.
	LayerBlendId = 500,
	LayerMaskStrengthId = 501,
	MaterialVariantId = 502,
	KeywordEntryId = 503,
	TechniqueDropdownId = 504,

	-- Preview/debug/action buttons.
	PreviewShapeId = 600,
	PreviewEnvironmentId = 601,
	PreviewExposureId = 602,
	PreviewRotationId = 603,
	ShowUvCheckerId = 604,
	ShowWireframeId = 605,
	ShowTangentsId = 606,
	ShowMipLevelsId = 607,

	ApplyId = 700,
	SaveId = 701,
	ReloadShaderId = 702,
	ValidateId = 703,
	ResetUvId = 704,
	CopyUvId = 705,
	PasteUvId = 706,
	DuplicateId = 707,
	CreateInstanceId = 708,
	ExportJsonId = 709,
	ImportJsonId = 710,
}

local MaterialEditorTextureSlots =
{
	Albedo = 0,
	Normal = 1,
	Metallic = 2,
	Roughness = 3,
	DetailWeight = 4,
	DetailAlbedo = 5,
	DetailNormal = 9,
	Emission = 13,

	-- Editor/custom-shader slots. These are persisted by Workphone materials,
	-- but are not sent to Ogre's fixed PBS texture enum by default.
	AmbientOcclusion = 22,
	Height = 23,
	Opacity = 24,
	Mask = 25,
	ClearCoat = 26,
	Anisotropy = 27,
}

local MaterialEditorDefaults =
{
	filePath = "None",
	materialName = "",
	shaderPath = "",
	materialType = 0,
	preset = 0,
	renderMode = 0,
	workflow = 0,

	transparent = false,
	cutout = false,
	emissionEnabled = false,
	refractionEnabled = false,
	alphaClip = 0.5,
	opacity = 1.0,
	doubleSided = false,
	receiveShadows = true,
	castShadows = true,
	cullMode = 0,
	refractionAmount = 0.0,
	refractionIor = 1.45,

	metalness = 0.0,
	roughness = 0.5,
	specular = 0.5,
	normalStrength = 1.0,
	aoStrength = 1.0,
	heightScale = 0.02,
	parallaxSteps = 16,
	emissionIntensity = 1.0,
	clearCoat = 0.0,
	clearCoatRoughness = 0.1,
	anisotropy = 0.0,

	uvTilingX = 1.0,
	uvTilingY = 1.0,
	uvOffsetX = 0.0,
	uvOffsetY = 0.0,
	uvRotation = 0.0,
	uvSet = 0,
	uvProjection = 0,
	uvTriplanarScale = 1.0,
	uvWrapU = 0,
	uvWrapV = 0,
	uvFilter = 1,
	uvAniso = 1,

	detailTilingX = 1.0,
	detailTilingY = 1.0,
	detailOffsetX = 0.0,
	detailOffsetY = 0.0,
	detailRotation = 0.0,
	detailBlendMode = 0,
	detailStrength = 0.0,
	detailNormalStrength = 1.0,

	metallicSource = 0,
	roughnessSource = 0,
	aoSource = 0,
	opacitySource = 0,
	heightSource = 0,
	generateMipmaps = true,
	srgb = true,
	normalMap = false,
	textureStreaming = true,
	maxTextureSize = 2048,
	pcCompression = 0,
	macCompression = 0,
	iosCompression = 0,
	androidCompression = 0,

	blendMode = 0,
	srcBlend = 0,
	dstBlend = 0,
	depthWrite = true,
	depthTest = 2,
	renderQueue = 2000,
	sortPriority = 0,
	stencilRef = 0,
	stencilReadMask = 255,
	stencilWriteMask = 255,
	gpuInstancing = true,
	srpBatcher = true,
	receiveDecals = true,

	layerBlend = 0,
	layerMaskStrength = 1.0,
	materialVariant = 0,
	keywords = "",
	technique = 0,

	previewShape = 0,
	previewEnvironment = 0,
	previewExposure = 1.0,
	previewRotation = 0.0,
	showUvChecker = false,
	showWireframe = false,
	showTangents = false,
	showMipLevels = false,
}

local MaterialTypeOptions =
{
	"Standard",
	"Standard Specular",
	"Standard Triplanar",
	"Terrain Standard",
	"Terrain Specular",
	"Terrain Diffuse",
	"Skybox",
	"Skybox Cubemap",
	"UI",
	"Custom",
}

local MaterialEditorUIntKeys =
{
	materialType = true,
	preset = true,
	renderMode = true,
	workflow = true,
	cullMode = true,
	uvSet = true,
	uvProjection = true,
	uvWrapU = true,
	uvWrapV = true,
	uvFilter = true,
	detailBlendMode = true,
	metallicSource = true,
	roughnessSource = true,
	aoSource = true,
	opacitySource = true,
	heightSource = true,
	blendMode = true,
	srcBlend = true,
	dstBlend = true,
	depthTest = true,
	layerBlend = true,
	materialVariant = true,
	technique = true,
	pcCompression = true,
	macCompression = true,
	iosCompression = true,
	androidCompression = true,
	previewShape = true,
	previewEnvironment = true,
}

local MaterialEditorRenderModes =
{
	Opaque = 0,
	Cutout = 1,
	Fade = 2,
	Transparent = 3,
	Additive = 4,
	Multiply = 5,
	Premultiplied = 6,
}

local MaterialEditorLoggedFailures = {};

local function copyTable(source)
	local result = {};
	for key, value in pairs(source) do
		result[key] = value;
	end
	return result;
end

local function normaliseDropdownIndex(value, optionCount)
	if type(value) ~= "number" or value ~= value or value == math.huge or value == -math.huge then
		return 0;
	end

	value = math.floor(value);
	if value < 0 or (optionCount ~= nil and optionCount > 0 and value >= optionCount) then
		return 0;
	end

	return value;
end

local function safeCall(target, methodName, ...)
	if target == nil or methodName == nil then
		return false, nil;
	end

	local okIndex, method = pcall(function()
		return target[methodName];
	end);

	if not okIndex or type(method) ~= "function" then
		return false, nil;
	end

	local okCall, result = pcall(method, target, ...);
	if not okCall then
		local key = tostring(target).."."..tostring(methodName);
		if MaterialEditorLoggedFailures[key] ~= true then
			MaterialEditorLoggedFailures[key] = true;
			print("MaterialEditor optional call failed: "..tostring(methodName).." - "..tostring(result));
		end
		return false, nil;
	end

	return true, result;
end

local function safeGet(target, methodName, defaultValue)
	local ok, result = safeCall(target, methodName);
	if ok and result ~= nil then
		return result;
	end
	return defaultValue;
end

local function safeArg(args, index, defaultValue)
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

local function safeParameterAt(parameters, index, defaultValue)
	if parameters == nil then
		return defaultValue;
	end

	local ok, value = pcall(function()
		return parameters:at(index);
	end);

	if ok and value ~= nil then
		return value;
	end

	return defaultValue;
end

local function safeVector2(x, y)
	local ok, value = pcall(function()
		return Vector2F(x, y);
	end);

	if ok then
		return value;
	end

	return nil;
end

local function readVector2Axis(value, axisName, defaultValue)
	local ok, axisValue = safeCall(value, axisName);
	if ok and axisValue ~= nil then
		return axisValue;
	end

	return defaultValue;
end

local function normaliseColourValue(value)
	if value == nil then
		return nil;
	end

	if type(value) ~= "table" then
		return value;
	end

	local r = value.r or value[1] or value.x;
	local g = value.g or value[2] or value.y;
	local b = value.b or value[3] or value.z;
	local a = value.a or value[4] or value.w or 1.0;

	if r == nil or g == nil or b == nil then
		return nil;
	end

	local ok, colour = pcall(function()
		return ColourF(r, g, b, a);
	end);

	if ok then
		return colour;
	end

	return nil;
end

local function containsText(text, value)
	if text == nil then
		return false;
	end

	local lowerText = string.lower(text);
	local lowerValue = string.lower(value);
	return string.find(lowerText, lowerValue, 1, true) ~= nil;
end

local function isTexturePath(path)
	if path == nil then return false; end
	return containsText(path, ".png") or
		containsText(path, ".tga") or
		containsText(path, ".tif") or
		containsText(path, ".tiff") or
		containsText(path, ".jpeg") or
		containsText(path, ".jpg") or
		containsText(path, ".dds") or
		containsText(path, ".ktx") or
		containsText(path, ".ktx2") or
		containsText(path, ".basis") or
		containsText(path, ".webp");
end

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

local function addEntry(ui, parent, label, elementId, text)
	local textEntryTypeInfo = IUITextEntry.typeInfo();
	local entry = ui:addElement(textEntryTypeInfo);
	entry:setLabel(label);
	entry:setElementId(elementId);
	entry:setText(text or "");
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

local function addColour(ui, parent, label, elementId)
	local colourPickerTypeInfo = IUIColourPicker.typeInfo();
	local colour = ui:addElement(colourPickerTypeInfo);
	colour:setLabel(label);
	colour:setElementId(elementId);
	colour:setSameLine(false);
	parent:addChild(colour);
	return colour;
end

function MaterialEditor:__init(window)
	print("MaterialEditor constructor called");
	BaseEditor:__init(self, window);

	self.window = window;
	self.editorWindow = nil;
	self.tabBar = nil;
	self.statusText = nil;

	self.material = nil;
	self.materialComponent = nil;
	self.materialType = nil;
	self.selectedMaterialFile = nil;
	self.selectedMaterial = nil;

	self.textureSlots = {};
	self.textureSlotIds = {};
	self.controls = {};
	self.controlBindings = {};
	self.controlBindingsById = {};
	self.materialSettings = copyTable(MaterialEditorDefaults);

	self.albedo = nil;
	self.normal = nil;
	self.metallic = nil;
	self.roughness = nil;
	self.ambientOcclusion = nil;
	self.height = nil;
	self.opacity = nil;
	self.emission = nil;
	self.mask = nil;
	self.detailAlbedo = nil;
	self.detailNormal = nil;
	self.clearCoat = nil;
	self.anisotropyMap = nil;
end

function MaterialEditor:__finalize()
	print("MaterialEditor __finalize called");
	BaseEditor:__finalize();
	self:_clearRefs();
	self.window = nil;
end

function MaterialEditor:_clearRefs()
	self.editorWindow = nil;
	self.tabBar = nil;
	self.statusText = nil;

	self.selectedMaterialFile = nil;
	self.selectedMaterial = nil;
	self.materialNameEntry = nil;
	self.shaderPathEntry = nil;
	self.presetDropdown = nil;

	self.transparent = nil;
	self.cutout = nil;
	self.emissionToggle = nil;
	self.refractionToggle = nil;
	self.doubleSidedToggle = nil;
	self.receiveShadowsToggle = nil;
	self.castShadowsToggle = nil;
	self.cullModeDropdown = nil;
	self.renderModeDropdown = nil;
	self.workflowDropdown = nil;
	self.alphaClipSlider = nil;
	self.opacitySlider = nil;
	self.refractionAmountSlider = nil;
	self.refractionIorSlider = nil;

	self.metalnesssSlider = nil;
	self.roughnessSlider = nil;
	self.metallicSourceDropDown = nil;
	self.roughnessSourceDropdown = nil;
	self.aoSourceDropdown = nil;
	self.opacitySourceDropdown = nil;
	self.heightSourceDropdown = nil;

	self.metallicColour = nil;
	self.emissionColour = nil;
	self.albedoColour = nil;
	self.specularSlider = nil;
	self.normalStrengthSlider = nil;
	self.aoStrengthSlider = nil;
	self.heightScaleSlider = nil;
	self.parallaxStepsSlider = nil;
	self.emissionIntensitySlider = nil;
	self.clearCoatAmountSlider = nil;
	self.clearCoatRoughnessSlider = nil;
	self.anisotropySlider = nil;

	self.uvTilingXSlider = nil;
	self.uvTilingYSlider = nil;
	self.uvOffsetXSlider = nil;
	self.uvOffsetYSlider = nil;
	self.uvRotationSlider = nil;
	self.uvSetDropdown = nil;
	self.uvProjectionDropdown = nil;
	self.uvTriplanarScaleSlider = nil;
	self.uvWrapUDropdown = nil;
	self.uvWrapVDropdown = nil;
	self.uvFilterDropdown = nil;
	self.uvAnisoSlider = nil;
	self.detailTilingXSlider = nil;
	self.detailTilingYSlider = nil;
	self.detailOffsetXSlider = nil;
	self.detailOffsetYSlider = nil;
	self.detailRotationSlider = nil;
	self.detailBlendDropdown = nil;
	self.detailStrengthSlider = nil;
	self.detailNormalStrengthSlider = nil;
	self.resetUvButton = nil;
	self.copyUvButton = nil;
	self.pasteUvButton = nil;

	self.blendModeDropdown = nil;
	self.srcBlendDropdown = nil;
	self.dstBlendDropdown = nil;
	self.depthWriteToggle = nil;
	self.depthTestDropdown = nil;
	self.renderQueueSlider = nil;
	self.sortPrioritySlider = nil;
	self.stencilRefSlider = nil;
	self.stencilReadMaskSlider = nil;
	self.stencilWriteMaskSlider = nil;
	self.gpuInstancingToggle = nil;
	self.srpBatcherToggle = nil;
	self.receiveDecalsToggle = nil;

	self.layerBlendDropdown = nil;
	self.layerMaskStrengthSlider = nil;
	self.materialVariantDropdown = nil;
	self.keywordEntry = nil;
	self.techniqueDropdown = nil;
	self.duplicateButton = nil;
	self.createInstanceButton = nil;

	self.generateMipmapsToggle = nil;
	self.srgbToggle = nil;
	self.normalMapToggle = nil;
	self.textureStreamingToggle = nil;
	self.maxTextureSizeSlider = nil;
	self.pcCompressionDropdown = nil;
	self.macCompressionDropdown = nil;
	self.iosCompressionDropdown = nil;
	self.androidCompressionDropdown = nil;

	self.previewShapeDropdown = nil;
	self.previewEnvironmentDropdown = nil;
	self.previewExposureSlider = nil;
	self.previewRotationSlider = nil;
	self.showUvCheckerToggle = nil;
	self.showWireframeToggle = nil;
	self.showTangentsToggle = nil;
	self.showMipLevelsToggle = nil;

	self.applyButton = nil;
	self.saveButton = nil;
	self.reloadShaderButton = nil;
	self.validateButton = nil;
	self.exportJsonButton = nil;
	self.importJsonButton = nil;

	self.textureSlots = {};
	self.textureSlotIds = {};
	self.controls = {};
	self.controlBindings = {};
	self.controlBindingsById = {};

	self.albedo = nil;
	self.normal = nil;
	self.metallic = nil;
	self.roughness = nil;
	self.ambientOcclusion = nil;
	self.height = nil;
	self.opacity = nil;
	self.emission = nil;
	self.mask = nil;
	self.detailAlbedo = nil;
	self.detailNormal = nil;
	self.clearCoat = nil;
	self.anisotropyMap = nil;
end

function MaterialEditor:registerControl(control, key, valueType, setterName, optionCount)
	if control == nil then
		return;
	end

	self.controls[#self.controls + 1] = control;
	local binding =
	{
		control = control,
		key = key,
		valueType = valueType,
		setterName = setterName,
		optionCount = optionCount,
	};

	self.controlBindings[control] = binding;

	local elementId = control:getElementId();
	if elementId ~= nil then
		self.controlBindingsById[elementId] = binding;
	end

	if self.window ~= nil then
		self.window:setHandleEvents(control, true);
	end

	return control;
end

function MaterialEditor:addTextureSlot(parent, key, elementId, label, sameLine)
	local slot = ResourceSelect(self.window, parent, elementId, label);
	slot:setSameLine(sameLine == true);
	slot:setLabel(label);

	self.textureSlots[key] = slot;
	self.textureSlotIds[elementId] = slot;

	if key == "albedo" then self.albedo = slot; end
	if key == "normal" then self.normal = slot; end
	if key == "metallic" then self.metallic = slot; end
	if key == "roughness" then self.roughness = slot; end
	if key == "ambientOcclusion" then self.ambientOcclusion = slot; end
	if key == "height" then self.height = slot; end
	if key == "opacity" then self.opacity = slot; end
	if key == "emission" then self.emission = slot; end
	if key == "mask" then self.mask = slot; end
	if key == "detailAlbedo" then self.detailAlbedo = slot; end
	if key == "detailNormal" then self.detailNormal = slot; end
	if key == "clearCoat" then self.clearCoat = slot; end
	if key == "anisotropy" then self.anisotropyMap = slot; end

	return slot;
end

function MaterialEditor:addBoundSlider(ui, parent, label, elementId, key, minValue, maxValue, setterName)
	local value = self.materialSettings[key] or 0.0;
	local control = addSlider(ui, parent, label, elementId, minValue, maxValue, value);
	self:registerControl(control, key, "float", setterName);
	return control;
end

function MaterialEditor:addBoundToggle(ui, parent, label, elementId, key, setterName)
	local value = self.materialSettings[key] == true;
	local control = addToggle(ui, parent, label, elementId, value);
	self:registerControl(control, key, "bool", setterName);
	return control;
end

function MaterialEditor:addBoundDropdown(ui, parent, label, elementId, key, options, setterName)
	local value = normaliseDropdownIndex(self.materialSettings[key] or 0, #options);
	self.materialSettings[key] = value;
	local control = addDropdown(ui, parent, label, elementId, options, value);
	self:registerControl(control, key, "dropdown", setterName, #options);
	return control;
end

function MaterialEditor:addBoundEntry(ui, parent, label, elementId, key, setterName)
	local value = self.materialSettings[key] or "";
	local control = addEntry(ui, parent, label, elementId, value);
	self:registerControl(control, key, "string", setterName);
	return control;
end

function MaterialEditor:addBoundColour(ui, parent, label, elementId, key, setterName)
	local control = addColour(ui, parent, label, elementId);
	self:registerControl(control, key, "colour", setterName);
	return control;
end

function MaterialEditor:storeEditorSetting(key, value, valueType)
	if self.material == nil or key == nil or value == nil then
		return;
	end

	if key == "filePath" then
		return;
	end

	local luaType = type(value);
	if valueType == "dropdown" or MaterialEditorUIntKeys[key] == true then
		safeCall(self.material, "setEditorUInt", key, value);
	elseif luaType == "boolean" then
		safeCall(self.material, "setEditorBool", key, value);
	elseif luaType == "number" then
		safeCall(self.material, "setEditorFloat", key, value);
	elseif luaType == "string" then
		safeCall(self.material, "setEditorString", key, value);
	end
end

function MaterialEditor:loadEditorSettingsFromMaterial()
	if self.material == nil then
		return;
	end

	for key, defaultValue in pairs(MaterialEditorDefaults) do
		local defaultType = type(defaultValue);
		if defaultType == "boolean" then
			local ok, value = safeCall(self.material, "getEditorBool", key, defaultValue);
			if ok and value ~= nil then
				self.materialSettings[key] = value;
			end
		elseif defaultType == "number" then
			if MaterialEditorUIntKeys[key] == true then
				local ok, value = safeCall(self.material, "getEditorUInt", key, defaultValue);
				if ok and value ~= nil then
					self.materialSettings[key] = value;
				end
			else
				local ok, value = safeCall(self.material, "getEditorFloat", key, defaultValue);
				if ok and value ~= nil then
					self.materialSettings[key] = value;
				end
			end
		elseif defaultType == "string" then
			local ok, value = safeCall(self.material, "getEditorString", key, defaultValue);
			if ok and value ~= nil then
				self.materialSettings[key] = value;
			end
		end
	end
end

function MaterialEditor:syncSurfaceSettingsFromMaterial()
	if self.material == nil then
		return;
	end

	local renderMode = safeGet(self.material, "getRenderMode", self.materialSettings.renderMode or MaterialEditorRenderModes.Opaque);
	local passTransparent = safeGet(self.material, "isTransparent", false);
	local passCutout = safeGet(self.material, "isCutout", false);
	local transparent = false;
	local cutout = false;

	if renderMode == MaterialEditorRenderModes.Cutout then
		cutout = true;
	elseif renderMode ~= nil and renderMode >= MaterialEditorRenderModes.Fade then
		transparent = true;
	elseif passCutout == true then
		renderMode = MaterialEditorRenderModes.Cutout;
		cutout = true;
	elseif passTransparent == true then
		renderMode = MaterialEditorRenderModes.Transparent;
		transparent = true;
	else
		renderMode = MaterialEditorRenderModes.Opaque;
	end

	self.materialSettings.renderMode = renderMode;
	self.materialSettings.transparent = transparent;
	self.materialSettings.cutout = cutout;
end

function MaterialEditor:applySurfaceMode(transparent, cutout, preferredRenderMode)
	if self.material == nil then
		return;
	end

	transparent = transparent == true;
	cutout = cutout == true;

	if cutout then
		transparent = false;
	elseif transparent then
		cutout = false;
	end

	local renderMode = MaterialEditorRenderModes.Opaque;
	if cutout then
		renderMode = MaterialEditorRenderModes.Cutout;
	elseif transparent then
		if preferredRenderMode ~= nil and
			preferredRenderMode >= MaterialEditorRenderModes.Fade and
			preferredRenderMode <= MaterialEditorRenderModes.Premultiplied then
			renderMode = preferredRenderMode;
		else
		renderMode = MaterialEditorRenderModes.Transparent;
	end
	end

	self.materialSettings.renderMode = renderMode;
	self.materialSettings.transparent = transparent;
	self.materialSettings.cutout = cutout;

	self:storeEditorSetting("renderMode", renderMode, "dropdown");
	self:storeEditorSetting("transparent", transparent, "bool");
	self:storeEditorSetting("cutout", cutout, "bool");
	safeCall(self.material, "setRenderMode", renderMode);
end

function MaterialEditor:load()
	print("MaterialEditor load called");

	self:_clearRefs();
	self.materialSettings = copyTable(MaterialEditorDefaults);

	local windowTypeInfo = IUIWindow.typeInfo();
	local tabBarTypeInfo = IUITabBar.typeInfo();
	local collapsingHeaderTypeInfo = IUICollapsingHeader.typeInfo();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	local parentWindow = self.window:getParentWindow();
	if not parentWindow then
		return;
	end

	parentWindow:setSize(Vector2F(760.0, 760.0));

	self.editorWindow = ui:addElement(windowTypeInfo);
	self.editorWindow:setLabel("Material Editor");
	self.editorWindow:setSize(Vector2F(760.0, 760.0));
	parentWindow:addChild(self.editorWindow);

	self.tabBar = ui:addElement(tabBarTypeInfo);
	local materialTab = self.tabBar:addTabItem(); materialTab:setLabel("Material");
	local textureTab = self.tabBar:addTabItem(); textureTab:setLabel("Textures");
	local uvTab = self.tabBar:addTabItem(); uvTab:setLabel("UV");
	local lightingTab = self.tabBar:addTabItem(); lightingTab:setLabel("Lighting");
	local renderStateTab = self.tabBar:addTabItem(); renderStateTab:setLabel("Render State");
	local layersTab = self.tabBar:addTabItem(); layersTab:setLabel("Layers");
	local importTab = self.tabBar:addTabItem(); importTab:setLabel("Import");
	local previewTab = self.tabBar:addTabItem(); previewTab:setLabel("Preview");
	local debugTab = self.tabBar:addTabItem(); debugTab:setLabel("Debug");
	self.editorWindow:addChild(self.tabBar);

	-- Material tab.
	local assetHeader = ui:addElement(collapsingHeaderTypeInfo);
	assetHeader:setLabel("Asset");
	materialTab:addChild(assetHeader);

	self.selectedMaterialFile = self:addBoundEntry(ui, assetHeader, "File", MaterialEditorTypes.FilePathId, "filePath", nil);
	self.selectedMaterialFile:setText("None");
	self.window:setDroppable(self.selectedMaterialFile, true);

	self.materialNameEntry = self:addBoundEntry(ui, assetHeader, "Material Name", MaterialEditorTypes.MaterialNameId, "materialName", "setName");
	self.shaderPathEntry = self:addBoundEntry(ui, assetHeader, "Shader", MaterialEditorTypes.ShaderPathId, "shaderPath", "setShaderPath");

	self.selectedMaterial = self:addBoundDropdown(ui, assetHeader, "Workflow Type", MaterialEditorTypes.MaterialTypeId, "materialType",
		MaterialTypeOptions, "setMaterialType");

	self.presetDropdown = self:addBoundDropdown(ui, assetHeader, "Preset", MaterialEditorTypes.PresetId, "preset",
		{ "Standard PBR", "Glass", "Cutout Foliage", "Emissive", "UI Transparent", "Decal", "Terrain Layer" }, nil);

	local surfaceHeader = ui:addElement(collapsingHeaderTypeInfo);
	surfaceHeader:setLabel("Surface");
	materialTab:addChild(surfaceHeader);

	self.renderModeDropdown = self:addBoundDropdown(ui, surfaceHeader, "Render Mode", MaterialEditorTypes.RenderModeId, "renderMode",
		{ "Opaque", "Cutout", "Fade", "Transparent", "Additive", "Multiply", "Premultiplied" }, "setRenderMode");
	self.workflowDropdown = self:addBoundDropdown(ui, surfaceHeader, "PBR Workflow", MaterialEditorTypes.WorkflowId, "workflow",
		{ "Metallic/Roughness", "Specular/Gloss", "Packed ORM", "Custom Shader" }, "setWorkflow");

	self.transparent = self:addBoundToggle(ui, surfaceHeader, "Transparent", MaterialEditorTypes.TransparentId, "transparent", "setTransparent");
	self.cutout = self:addBoundToggle(ui, surfaceHeader, "Cutout", MaterialEditorTypes.CutoutId, "cutout", "setCutout");
	self.emissionToggle = self:addBoundToggle(ui, surfaceHeader, "Emission", MaterialEditorTypes.EmissionId, "emissionEnabled", "setEmissionEnabled");
	self.refractionToggle = self:addBoundToggle(ui, surfaceHeader, "Refraction", MaterialEditorTypes.RefractionId, "refractionEnabled", "setRefractionEnabled");
	self.doubleSidedToggle = self:addBoundToggle(ui, surfaceHeader, "Double Sided", MaterialEditorTypes.DoubleSidedId, "doubleSided", "setDoubleSided");
	self.receiveShadowsToggle = self:addBoundToggle(ui, surfaceHeader, "Receive Shadows", MaterialEditorTypes.ReceiveShadowsId, "receiveShadows", "setReceiveShadows");
	self.castShadowsToggle = self:addBoundToggle(ui, surfaceHeader, "Cast Shadows", MaterialEditorTypes.CastShadowsId, "castShadows", "setCastShadows");
	self.cullModeDropdown = self:addBoundDropdown(ui, surfaceHeader, "Cull Mode", MaterialEditorTypes.CullModeId, "cullMode",
		{ "Back", "Front", "Off" }, "setCullMode");

	self.alphaClipSlider = self:addBoundSlider(ui, surfaceHeader, "Alpha Clip", MaterialEditorTypes.AlphaClipId, "alphaClip", 0.0, 1.0, "setAlphaClip");
	self.opacitySlider = self:addBoundSlider(ui, surfaceHeader, "Opacity", MaterialEditorTypes.OpacityId, "opacity", 0.0, 1.0, "setOpacity");
	self.refractionAmountSlider = self:addBoundSlider(ui, surfaceHeader, "Refraction Amount", MaterialEditorTypes.RefractionAmountId, "refractionAmount", 0.0, 1.0, "setRefractionAmount");
	self.refractionIorSlider = self:addBoundSlider(ui, surfaceHeader, "IOR", MaterialEditorTypes.RefractionIorId, "refractionIor", 1.0, 2.5, "setIndexOfRefraction");

	-- Textures tab.
	local baseMapsHeader = ui:addElement(collapsingHeaderTypeInfo);
	baseMapsHeader:setLabel("Base Maps");
	textureTab:addChild(baseMapsHeader);

	self:addTextureSlot(baseMapsHeader, "albedo", MaterialEditorTypes.Albedo, "Albedo", false, MaterialEditorTextureSlots.Albedo);
	self:addTextureSlot(baseMapsHeader, "normal", MaterialEditorTypes.Normal, "Normal", true, MaterialEditorTextureSlots.Normal);
	self:addTextureSlot(baseMapsHeader, "metallic", MaterialEditorTypes.Metallic, "Metallic", false, MaterialEditorTextureSlots.Metallic);
	self:addTextureSlot(baseMapsHeader, "roughness", MaterialEditorTypes.Roughness, "Roughness", true, MaterialEditorTextureSlots.Roughness);
	self:addTextureSlot(baseMapsHeader, "ambientOcclusion", MaterialEditorTypes.AmbientOcclusion, "Ambient Occlusion", false, MaterialEditorTextureSlots.AmbientOcclusion);
	self:addTextureSlot(baseMapsHeader, "height", MaterialEditorTypes.Height, "Height", true, MaterialEditorTextureSlots.Height);
	self:addTextureSlot(baseMapsHeader, "opacity", MaterialEditorTypes.Opacity, "Opacity", false, MaterialEditorTextureSlots.Opacity);
	self:addTextureSlot(baseMapsHeader, "emission", MaterialEditorTypes.Emission, "Emission", true, MaterialEditorTextureSlots.Emission);

	local advancedMapsHeader = ui:addElement(collapsingHeaderTypeInfo);
	advancedMapsHeader:setLabel("Advanced Maps");
	textureTab:addChild(advancedMapsHeader);

	self:addTextureSlot(advancedMapsHeader, "mask", MaterialEditorTypes.Mask, "Packed Mask", false, MaterialEditorTextureSlots.Mask);
	self:addTextureSlot(advancedMapsHeader, "detailAlbedo", MaterialEditorTypes.DetailAlbedo, "Detail Albedo", true, MaterialEditorTextureSlots.DetailAlbedo);
	self:addTextureSlot(advancedMapsHeader, "detailNormal", MaterialEditorTypes.DetailNormal, "Detail Normal", false, MaterialEditorTextureSlots.DetailNormal);
	self:addTextureSlot(advancedMapsHeader, "clearCoat", MaterialEditorTypes.ClearCoat, "Clear Coat", true, MaterialEditorTextureSlots.ClearCoat);
	self:addTextureSlot(advancedMapsHeader, "anisotropy", MaterialEditorTypes.Anisotropy, "Anisotropy", false, MaterialEditorTextureSlots.Anisotropy);

	local channelHeader = ui:addElement(collapsingHeaderTypeInfo);
	channelHeader:setLabel("Texture Channel Packing");
	textureTab:addChild(channelHeader);

	self.metallicSourceDropDown = self:addBoundDropdown(ui, channelHeader, "Metallic Source", MaterialEditorTypes.MetallicSourceId, "metallicSource",
		{ "Metallic R", "Metallic A", "Albedo A", "Packed Mask R" }, "setMetallicSource");
	self.roughnessSourceDropdown = self:addBoundDropdown(ui, channelHeader, "Roughness Source", MaterialEditorTypes.RoughnessSourceId, "roughnessSource",
		{ "Roughness R", "Roughness A", "Metallic A", "Packed Mask G", "Invert Smoothness A" }, "setRoughnessSource");
	self.aoSourceDropdown = self:addBoundDropdown(ui, channelHeader, "AO Source", MaterialEditorTypes.AoSourceId, "aoSource",
		{ "AO R", "Packed Mask B", "Vertex Color A", "None" }, "setAmbientOcclusionSource");
	self.opacitySourceDropdown = self:addBoundDropdown(ui, channelHeader, "Opacity Source", MaterialEditorTypes.OpacitySourceId, "opacitySource",
		{ "Albedo A", "Opacity R", "Packed Mask A", "Constant" }, "setOpacitySource");
	self.heightSourceDropdown = self:addBoundDropdown(ui, channelHeader, "Height Source", MaterialEditorTypes.HeightSourceId, "heightSource",
		{ "Height R", "Packed Mask A", "Normal A" }, "setHeightSource");

	-- UV tab.
	local mainUvHeader = ui:addElement(collapsingHeaderTypeInfo);
	mainUvHeader:setLabel("Main UV Transform");
	uvTab:addChild(mainUvHeader);

	self.uvTilingXSlider = self:addBoundSlider(ui, mainUvHeader, "Tiling X", MaterialEditorTypes.UVTilingXId, "uvTilingX", -32.0, 32.0, "setUVTilingX");
	self.uvTilingYSlider = self:addBoundSlider(ui, mainUvHeader, "Tiling Y", MaterialEditorTypes.UVTilingYId, "uvTilingY", -32.0, 32.0, "setUVTilingY");
	self.uvOffsetXSlider = self:addBoundSlider(ui, mainUvHeader, "Offset X", MaterialEditorTypes.UVOffsetXId, "uvOffsetX", -10.0, 10.0, "setUVOffsetX");
	self.uvOffsetYSlider = self:addBoundSlider(ui, mainUvHeader, "Offset Y", MaterialEditorTypes.UVOffsetYId, "uvOffsetY", -10.0, 10.0, "setUVOffsetY");
	self.uvRotationSlider = self:addBoundSlider(ui, mainUvHeader, "Rotation", MaterialEditorTypes.UVRotationId, "uvRotation", -180.0, 180.0, "setUVRotation");
	self.uvSetDropdown = self:addBoundDropdown(ui, mainUvHeader, "UV Set", MaterialEditorTypes.UVSetId, "uvSet",
		{ "UV0", "UV1", "UV2", "UV3" }, "setUVSet");
	self.uvProjectionDropdown = self:addBoundDropdown(ui, mainUvHeader, "Projection", MaterialEditorTypes.UVProjectionId, "uvProjection",
		{ "Mesh UV", "World Box Projection", "Object Box Projection", "Planar XZ", "Planar XY", "Planar YZ", "Screen Space" }, "setUVProjection");
	self.uvTriplanarScaleSlider = self:addBoundSlider(ui, mainUvHeader, "Projection Scale", MaterialEditorTypes.UVTriplanarScaleId, "uvTriplanarScale", 0.01, 100.0, "setTriplanarScale");

	local samplerHeader = ui:addElement(collapsingHeaderTypeInfo);
	samplerHeader:setLabel("Sampler");
	uvTab:addChild(samplerHeader);

	self.uvWrapUDropdown = self:addBoundDropdown(ui, samplerHeader, "Wrap U", MaterialEditorTypes.UVWrapUId, "uvWrapU",
		{ "Repeat", "Clamp", "Mirror", "Mirror Once", "Border" }, "setWrapU");
	self.uvWrapVDropdown = self:addBoundDropdown(ui, samplerHeader, "Wrap V", MaterialEditorTypes.UVWrapVId, "uvWrapV",
		{ "Repeat", "Clamp", "Mirror", "Mirror Once", "Border" }, "setWrapV");
	self.uvFilterDropdown = self:addBoundDropdown(ui, samplerHeader, "Filter", MaterialEditorTypes.UVFilterId, "uvFilter",
		{ "Point", "Bilinear", "Trilinear", "Anisotropic" }, "setFilterMode");
	self.uvAnisoSlider = self:addBoundSlider(ui, samplerHeader, "Anisotropic Level", MaterialEditorTypes.UVAnisoId, "uvAniso", 1.0, 16.0, "setAnisotropicLevel");

	local detailUvHeader = ui:addElement(collapsingHeaderTypeInfo);
	detailUvHeader:setLabel("Detail UV Transform");
	uvTab:addChild(detailUvHeader);

	self.detailTilingXSlider = self:addBoundSlider(ui, detailUvHeader, "Detail Tiling X", MaterialEditorTypes.DetailTilingXId, "detailTilingX", -64.0, 64.0, "setDetailTilingX");
	self.detailTilingYSlider = self:addBoundSlider(ui, detailUvHeader, "Detail Tiling Y", MaterialEditorTypes.DetailTilingYId, "detailTilingY", -64.0, 64.0, "setDetailTilingY");
	self.detailOffsetXSlider = self:addBoundSlider(ui, detailUvHeader, "Detail Offset X", MaterialEditorTypes.DetailOffsetXId, "detailOffsetX", -10.0, 10.0, "setDetailOffsetX");
	self.detailOffsetYSlider = self:addBoundSlider(ui, detailUvHeader, "Detail Offset Y", MaterialEditorTypes.DetailOffsetYId, "detailOffsetY", -10.0, 10.0, "setDetailOffsetY");
	self.detailRotationSlider = self:addBoundSlider(ui, detailUvHeader, "Detail Rotation", MaterialEditorTypes.DetailRotationId, "detailRotation", -180.0, 180.0, "setDetailRotation");
	self.detailBlendDropdown = self:addBoundDropdown(ui, detailUvHeader, "Detail Blend", MaterialEditorTypes.DetailBlendModeId, "detailBlendMode",
		{ "Overlay", "Multiply", "Add", "Normal Blend", "Height Blend" }, "setDetailBlendMode");
	self.detailStrengthSlider = self:addBoundSlider(ui, detailUvHeader, "Detail Strength", MaterialEditorTypes.DetailStrengthId, "detailStrength", 0.0, 2.0, "setDetailStrength");
	self.detailNormalStrengthSlider = self:addBoundSlider(ui, detailUvHeader, "Detail Normal Strength", MaterialEditorTypes.DetailNormalStrengthId, "detailNormalStrength", 0.0, 4.0, "setDetailNormalStrength");

	self.resetUvButton = addButton(ui, uvTab, "Reset UV", MaterialEditorTypes.ResetUvId, false);
	self.copyUvButton = addButton(ui, uvTab, "Copy UV", MaterialEditorTypes.CopyUvId, true);
	self.pasteUvButton = addButton(ui, uvTab, "Paste UV", MaterialEditorTypes.PasteUvId, true);
	self:registerControl(self.resetUvButton, "resetUv", "button", nil);
	self:registerControl(self.copyUvButton, "copyUv", "button", nil);
	self:registerControl(self.pasteUvButton, "pasteUv", "button", nil);

	-- Lighting tab.
	local pbrHeader = ui:addElement(collapsingHeaderTypeInfo);
	pbrHeader:setLabel("PBR Scalars");
	lightingTab:addChild(pbrHeader);

	self.metalnesssSlider = self:addBoundSlider(ui, pbrHeader, "Metalness", MaterialEditorTypes.MetalnessSliderId, "metalness", 0.0, 1.0, "setMetalness");
	self.roughnessSlider = self:addBoundSlider(ui, pbrHeader, "Roughness", MaterialEditorTypes.RoughnessSliderId, "roughness", 0.0, 1.0, "setRoughness");
	self.specularSlider = self:addBoundSlider(ui, pbrHeader, "Specular", MaterialEditorTypes.SpecularSliderId, "specular", 0.0, 1.0, "setSpecularAmount");
	self.normalStrengthSlider = self:addBoundSlider(ui, pbrHeader, "Normal Strength", MaterialEditorTypes.NormalStrengthId, "normalStrength", 0.0, 4.0, "setNormalStrength");
	self.aoStrengthSlider = self:addBoundSlider(ui, pbrHeader, "AO Strength", MaterialEditorTypes.AoStrengthId, "aoStrength", 0.0, 4.0, "setAmbientOcclusionStrength");
	self.heightScaleSlider = self:addBoundSlider(ui, pbrHeader, "Height Scale", MaterialEditorTypes.HeightScaleId, "heightScale", -1.0, 1.0, "setHeightScale");
	self.parallaxStepsSlider = self:addBoundSlider(ui, pbrHeader, "Parallax Steps", MaterialEditorTypes.ParallaxStepsId, "parallaxSteps", 1.0, 64.0, "setParallaxSteps");

	self.albedoColour = self:addBoundColour(ui, pbrHeader, "Albedo Colour", MaterialEditorTypes.AlbedoColourId, "albedoColour", "setDiffuse");
	self.metallicColour = self:addBoundColour(ui, pbrHeader, "Metallic Colour", MaterialEditorTypes.MetallicColourId, "metallicColour", "setDiffuse");

	local emissionHeader = ui:addElement(collapsingHeaderTypeInfo);
	emissionHeader:setLabel("Emission / Advanced Lighting");
	lightingTab:addChild(emissionHeader);

	self.emissionColour = self:addBoundColour(ui, emissionHeader, "Emission Colour", MaterialEditorTypes.EmissionColourId, "emissionColour", "setEmissive");
	self.emissionIntensitySlider = self:addBoundSlider(ui, emissionHeader, "Emission Intensity", MaterialEditorTypes.EmissionIntensityId, "emissionIntensity", 0.0, 100.0, "setEmissionIntensity");
	self.clearCoatAmountSlider = self:addBoundSlider(ui, emissionHeader, "Clear Coat", MaterialEditorTypes.ClearCoatAmountId, "clearCoat", 0.0, 1.0, "setClearCoat");
	self.clearCoatRoughnessSlider = self:addBoundSlider(ui, emissionHeader, "Clear Coat Roughness", MaterialEditorTypes.ClearCoatRoughnessId, "clearCoatRoughness", 0.0, 1.0, "setClearCoatRoughness");
	self.anisotropySlider = self:addBoundSlider(ui, emissionHeader, "Anisotropy", MaterialEditorTypes.AnisotropyAmountId, "anisotropy", -1.0, 1.0, "setAnisotropy");

	-- Render state tab.
	local blendHeader = ui:addElement(collapsingHeaderTypeInfo);
	blendHeader:setLabel("Blend / Depth / Queue");
	renderStateTab:addChild(blendHeader);

	self.blendModeDropdown = self:addBoundDropdown(ui, blendHeader, "Blend Mode", MaterialEditorTypes.BlendModeId, "blendMode",
		{ "Opaque", "Alpha", "Premultiplied", "Additive", "Multiply", "Custom" }, "setBlendMode");
	self.srcBlendDropdown = self:addBoundDropdown(ui, blendHeader, "Source Blend", MaterialEditorTypes.SrcBlendId, "srcBlend",
		{ "One", "Zero", "Src Alpha", "One Minus Src Alpha", "Dst Color", "One Minus Dst Color" }, "setSrcBlend");
	self.dstBlendDropdown = self:addBoundDropdown(ui, blendHeader, "Destination Blend", MaterialEditorTypes.DstBlendId, "dstBlend",
		{ "Zero", "One", "Src Alpha", "One Minus Src Alpha", "Dst Alpha", "One Minus Dst Alpha" }, "setDstBlend");
	self.depthWriteToggle = self:addBoundToggle(ui, blendHeader, "Depth Write", MaterialEditorTypes.DepthWriteId, "depthWrite", "setDepthWrite");
	self.depthTestDropdown = self:addBoundDropdown(ui, blendHeader, "Depth Test", MaterialEditorTypes.DepthTestId, "depthTest",
		{ "Never", "Less", "Less Equal", "Equal", "Greater Equal", "Greater", "Always" }, "setDepthTest");
	self.renderQueueSlider = self:addBoundSlider(ui, blendHeader, "Render Queue", MaterialEditorTypes.RenderQueueId, "renderQueue", 1000.0, 5000.0, "setRenderQueue");
	self.sortPrioritySlider = self:addBoundSlider(ui, blendHeader, "Sort Priority", MaterialEditorTypes.SortPriorityId, "sortPriority", -100.0, 100.0, "setSortPriority");

	local stencilHeader = ui:addElement(collapsingHeaderTypeInfo);
	stencilHeader:setLabel("Stencil / Batching");
	renderStateTab:addChild(stencilHeader);

	self.stencilRefSlider = self:addBoundSlider(ui, stencilHeader, "Stencil Ref", MaterialEditorTypes.StencilRefId, "stencilRef", 0.0, 255.0, "setStencilRef");
	self.stencilReadMaskSlider = self:addBoundSlider(ui, stencilHeader, "Stencil Read Mask", MaterialEditorTypes.StencilReadMaskId, "stencilReadMask", 0.0, 255.0, "setStencilReadMask");
	self.stencilWriteMaskSlider = self:addBoundSlider(ui, stencilHeader, "Stencil Write Mask", MaterialEditorTypes.StencilWriteMaskId, "stencilWriteMask", 0.0, 255.0, "setStencilWriteMask");
	self.gpuInstancingToggle = self:addBoundToggle(ui, stencilHeader, "GPU Instancing", MaterialEditorTypes.GpuInstancingId, "gpuInstancing", "setGpuInstancing");
	self.srpBatcherToggle = self:addBoundToggle(ui, stencilHeader, "SRP/HLMS Batcher", MaterialEditorTypes.SrpBatcherId, "srpBatcher", "setBatchingEnabled");
	self.receiveDecalsToggle = self:addBoundToggle(ui, stencilHeader, "Receive Decals", MaterialEditorTypes.ReceiveDecalsId, "receiveDecals", "setReceiveDecals");

	-- Layers tab.
	local layersHeader = ui:addElement(collapsingHeaderTypeInfo);
	layersHeader:setLabel("Layered Material / Variants");
	layersTab:addChild(layersHeader);

	self.layerBlendDropdown = self:addBoundDropdown(ui, layersHeader, "Layer Blend", MaterialEditorTypes.LayerBlendId, "layerBlend",
		{ "None", "Mask", "Height Blend", "Vertex Color", "Slope", "Triplanar" }, "setLayerBlendMode");
	self.layerMaskStrengthSlider = self:addBoundSlider(ui, layersHeader, "Layer Mask Strength", MaterialEditorTypes.LayerMaskStrengthId, "layerMaskStrength", 0.0, 4.0, "setLayerMaskStrength");
	self.materialVariantDropdown = self:addBoundDropdown(ui, layersHeader, "Variant", MaterialEditorTypes.MaterialVariantId, "materialVariant",
		{ "Default", "Low", "Medium", "High", "Mobile", "Editor Preview", "Runtime Instance" }, "setVariant");
	self.keywordEntry = self:addBoundEntry(ui, layersHeader, "Shader Keywords", MaterialEditorTypes.KeywordEntryId, "keywords", "setKeywords");
	self.techniqueDropdown = self:addBoundDropdown(ui, layersHeader, "Technique", MaterialEditorTypes.TechniqueDropdownId, "technique",
		{ "Auto", "Forward", "Deferred", "Depth Only", "Shadow Caster", "Unlit", "UI" }, "setTechnique");

	self.duplicateButton = addButton(ui, layersTab, "Duplicate Material", MaterialEditorTypes.DuplicateId, false);
	self.createInstanceButton = addButton(ui, layersTab, "Create Runtime Instance", MaterialEditorTypes.CreateInstanceId, true);
	self:registerControl(self.duplicateButton, "duplicate", "button", nil);
	self:registerControl(self.createInstanceButton, "createInstance", "button", nil);

	-- Import tab.
	local importHeader = ui:addElement(collapsingHeaderTypeInfo);
	importHeader:setLabel("Texture Import");
	importTab:addChild(importHeader);

	self.generateMipmapsToggle = self:addBoundToggle(ui, importHeader, "Generate Mipmaps", MaterialEditorTypes.GenerateMipmapsId, "generateMipmaps", "setGenerateMipmaps");
	self.srgbToggle = self:addBoundToggle(ui, importHeader, "sRGB / Color Texture", MaterialEditorTypes.SrgbId, "srgb", "setSrgb");
	self.normalMapToggle = self:addBoundToggle(ui, importHeader, "Treat Selected Texture As Normal", MaterialEditorTypes.NormalMapId, "normalMap", "setNormalMap");
	self.textureStreamingToggle = self:addBoundToggle(ui, importHeader, "Texture Streaming", MaterialEditorTypes.TextureStreamingId, "textureStreaming", "setTextureStreaming");
	self.maxTextureSizeSlider = self:addBoundSlider(ui, importHeader, "Max Texture Size", MaterialEditorTypes.MaxTextureSizeId, "maxTextureSize", 128.0, 8192.0, "setMaxTextureSize");

	local platformHeader = ui:addElement(collapsingHeaderTypeInfo);
	platformHeader:setLabel("Platform Compression");
	importTab:addChild(platformHeader);

	self.pcCompressionDropdown = self:addBoundDropdown(ui, platformHeader, "PC Compression", MaterialEditorTypes.PcCompressionId, "pcCompression",
		{ "Auto", "BC1", "BC3", "BC5", "BC7", "Uncompressed" }, "setPcTextureCompression");
	self.macCompressionDropdown = self:addBoundDropdown(ui, platformHeader, "macOS Compression", MaterialEditorTypes.MacCompressionId, "macCompression",
		{ "Auto", "BC7", "ASTC", "PVRTC", "Uncompressed" }, "setMacTextureCompression");
	self.iosCompressionDropdown = self:addBoundDropdown(ui, platformHeader, "iOS Compression", MaterialEditorTypes.IosCompressionId, "iosCompression",
		{ "Auto", "ASTC 4x4", "ASTC 6x6", "ASTC 8x8", "PVRTC", "Uncompressed" }, "setIosTextureCompression");
	self.androidCompressionDropdown = self:addBoundDropdown(ui, platformHeader, "Android Compression", MaterialEditorTypes.AndroidCompressionId, "androidCompression",
		{ "Auto", "ASTC", "ETC2", "Basis Universal", "Uncompressed" }, "setAndroidTextureCompression");

	-- Preview tab.
	local previewHeader = ui:addElement(collapsingHeaderTypeInfo);
	previewHeader:setLabel("Preview");
	previewTab:addChild(previewHeader);

	self.previewShapeDropdown = self:addBoundDropdown(ui, previewHeader, "Preview Shape", MaterialEditorTypes.PreviewShapeId, "previewShape",
		{ "Sphere", "Cube", "Plane", "Cylinder", "Suzanne", "Selected Mesh", "Custom Mesh" }, nil);
	self.previewEnvironmentDropdown = self:addBoundDropdown(ui, previewHeader, "Environment", MaterialEditorTypes.PreviewEnvironmentId, "previewEnvironment",
		{ "Studio", "Outdoor", "Night", "Neutral Grey", "HDRI", "Transparent" }, nil);
	self.previewExposureSlider = self:addBoundSlider(ui, previewHeader, "Exposure", MaterialEditorTypes.PreviewExposureId, "previewExposure", 0.0, 8.0, nil);
	self.previewRotationSlider = self:addBoundSlider(ui, previewHeader, "Rotation", MaterialEditorTypes.PreviewRotationId, "previewRotation", -180.0, 180.0, nil);
	self.showUvCheckerToggle = self:addBoundToggle(ui, previewHeader, "UV Checker Overlay", MaterialEditorTypes.ShowUvCheckerId, "showUvChecker", nil);
	self.showWireframeToggle = self:addBoundToggle(ui, previewHeader, "Wireframe Overlay", MaterialEditorTypes.ShowWireframeId, "showWireframe", nil);
	self.showTangentsToggle = self:addBoundToggle(ui, previewHeader, "Show Tangents", MaterialEditorTypes.ShowTangentsId, "showTangents", nil);
	self.showMipLevelsToggle = self:addBoundToggle(ui, previewHeader, "Show Mip Levels", MaterialEditorTypes.ShowMipLevelsId, "showMipLevels", nil);

	-- Debug/actions tab.
	local actionsHeader = ui:addElement(collapsingHeaderTypeInfo);
	actionsHeader:setLabel("Actions");
	debugTab:addChild(actionsHeader);

	self.applyButton = addButton(ui, actionsHeader, "Apply", MaterialEditorTypes.ApplyId, false);
	self.saveButton = addButton(ui, actionsHeader, "Save", MaterialEditorTypes.SaveId, true);
	self.reloadShaderButton = addButton(ui, actionsHeader, "Reload Shader", MaterialEditorTypes.ReloadShaderId, true);
	self.validateButton = addButton(ui, actionsHeader, "Validate", MaterialEditorTypes.ValidateId, false);
	self.exportJsonButton = addButton(ui, actionsHeader, "Export JSON", MaterialEditorTypes.ExportJsonId, true);
	self.importJsonButton = addButton(ui, actionsHeader, "Import JSON", MaterialEditorTypes.ImportJsonId, true);

	self:registerControl(self.applyButton, "apply", "button", nil);
	self:registerControl(self.saveButton, "save", "button", nil);
	self:registerControl(self.reloadShaderButton, "reloadShader", "button", nil);
	self:registerControl(self.validateButton, "validate", "button", nil);
	self:registerControl(self.exportJsonButton, "exportJson", "button", nil);
	self:registerControl(self.importJsonButton, "importJson", "button", nil);

	local debugInfoHeader = ui:addElement(collapsingHeaderTypeInfo);
	debugInfoHeader:setLabel("Debug");
	debugTab:addChild(debugInfoHeader);

	self.statusText = addText(ui, debugInfoHeader, "Ready.");

	self:syncToControls();
	self:updateSelection();
end

function MaterialEditor:unload()
	print("MaterialEditor unload called");

	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:destroyAllChildren();
	end

	self:_clearRefs();
	self.material = nil;
	self.materialComponent = nil;
	self.materialType = nil;
end

function MaterialEditor:show()
	print("MaterialEditor show called");

	local parentWindow = self.window:getParentWindow();
	local debugWindow = self.window:getDebugWindow();

	if parentWindow then
		parentWindow:setVisible(true, false);
	end

	if debugWindow then
		debugWindow:setVisible(false, false);
	end

	if self.editorWindow then
		self.editorWindow:setVisible(true, false);
	end
end

function MaterialEditor:hide()
	print("MaterialEditor hide called");

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

function MaterialEditor:updateSelection()
	print("MaterialEditor updateSelection called");

	self.material = nil;
	self.materialComponent = nil;
	self.materialSettings = copyTable(MaterialEditorDefaults);

	if self.selectedMaterialFile then
		self.selectedMaterialFile:setText("None");
	end

	local applicationManager = IApplicationManager.instance();
	local resourceDatabase = applicationManager:getResourceDatabase();
	local selectionManager = applicationManager:getSelectionManager();
	local selection = selectionManager:getSelection();

	local materialComponentTypeInfo = Material.typeInfo();
	local materialTypeInfo = IMaterial.typeInfo();
	local fileSelectionTypeInfo = FileSelection.typeInfo();

	local size = selection:size();
	for i = 0, size - 1 do
		local item = selection:at(i);

		if item:derived(materialTypeInfo) then
			self.material = item;
		elseif item:derived(materialComponentTypeInfo) then
			self.material = item:getMaterial();
			self.materialComponent = item;
		elseif item:derived(fileSelectionTypeInfo) then
			local path = item:getFilePath();
			if containsText(path, ".mat") then
				self.material = resourceDatabase:loadResource(path);
			end
		else
			print("type:"..item:getTypeInfo());
		end
	end

	self:updateSelectedMaterial();
end

function MaterialEditor:updateSelectedMaterial()
	if self.material then
		local filePath = safeGet(self.material, "getFilePath", "None");
		self.materialSettings.filePath = filePath;

		if self.selectedMaterialFile then
			self.selectedMaterialFile:setText(filePath);
		end

		self.materialType = safeGet(self.material, "getMaterialType", self.materialSettings.materialType);
		self.materialSettings.materialType = self.materialType or 0;

		self:loadEditorSettingsFromMaterial();

		self.materialSettings.filePath = filePath;
		self.materialType = safeGet(self.material, "getMaterialType", self.materialSettings.materialType);
		self.materialSettings.materialType = self.materialType or 0;
		self.materialSettings.materialName = safeGet(self.material, "getName", self.materialSettings.materialName);
		self.materialSettings.workflow = safeGet(self.material, "getWorkflow", self.materialSettings.workflow);
		self:syncSurfaceSettingsFromMaterial();
		self.materialSettings.emissionEnabled = safeGet(self.material, "isEmissionEnabled", self.materialSettings.emissionEnabled);
		self.materialSettings.refractionEnabled = safeGet(self.material, "isRefractionEnabled", self.materialSettings.refractionEnabled);

		self.materialSettings.metalness = safeGet(self.material, "getMetalness", self.materialSettings.metalness);
		self.materialSettings.roughness = safeGet(self.material, "getRoughness", self.materialSettings.roughness);
		self.materialSettings.specular = safeGet(self.material, "getSpecularAmount", self.materialSettings.specular);
		self.materialSettings.normalStrength = safeGet(self.material, "getNormalStrength", self.materialSettings.normalStrength);
		self.materialSettings.opacity = safeGet(self.material, "getOpacity", self.materialSettings.opacity);
		self.materialSettings.alphaClip = safeGet(self.material, "getAlphaClip", self.materialSettings.alphaClip);
		self.materialSettings.blendMode = safeGet(self.material, "getBlendMode", self.materialSettings.blendMode);
		self.materialSettings.depthWrite = safeGet(self.material, "getDepthWrite", self.materialSettings.depthWrite);
		self.materialSettings.depthTest = safeGet(self.material, "getDepthTest", self.materialSettings.depthTest);
		self.materialSettings.cullMode = safeGet(self.material, "getCullMode", self.materialSettings.cullMode);
		self.materialSettings.doubleSided = self.materialSettings.cullMode == 2;

		local okUvTiling, uvTiling = safeCall(self.material, "getUVTiling");
		if okUvTiling and uvTiling ~= nil then
			self.materialSettings.uvTilingX = readVector2Axis(uvTiling, "X", self.materialSettings.uvTilingX);
			self.materialSettings.uvTilingY = readVector2Axis(uvTiling, "Y", self.materialSettings.uvTilingY);
		end

		local okUvOffset, uvOffset = safeCall(self.material, "getUVOffset");
		if okUvOffset and uvOffset ~= nil then
			self.materialSettings.uvOffsetX = readVector2Axis(uvOffset, "X", self.materialSettings.uvOffsetX);
			self.materialSettings.uvOffsetY = readVector2Axis(uvOffset, "Y", self.materialSettings.uvOffsetY);
		end

		self.materialSettings.uvRotation = safeGet(self.material, "getUVRotation", self.materialSettings.uvRotation);
		self.materialSettings.uvSet = safeGet(self.material, "getUVSet", self.materialSettings.uvSet);
		self.materialSettings.uvProjection = safeGet(self.material, "getUVProjection", self.materialSettings.uvProjection);
		self.materialSettings.uvTriplanarScale = safeGet(self.material, "getTriplanarScale", self.materialSettings.uvTriplanarScale);

		if self.albedoColour then
			local diffuseColour = safeGet(self.material, "getDiffuse", nil);
			if diffuseColour then
				safeCall(self.albedoColour, "setColour", diffuseColour);
				if self.metallicColour then
					safeCall(self.metallicColour, "setColour", diffuseColour);
				end
			end
		end

		if self.emissionColour then
			local emissionColour = safeGet(self.material, "getEmissive", nil);
			if emissionColour then
				safeCall(self.emissionColour, "setColour", emissionColour);
			end
		end
	end

	for _, slot in pairs(self.textureSlots) do
		if slot then
			slot:setMaterial(self.material);
		end
	end

	self:syncToControls();
end

function MaterialEditor:syncDerivedSettingsFromMaterial()
	if self.material == nil then
		return;
	end

	self:syncSurfaceSettingsFromMaterial();
	self.materialSettings.emissionEnabled = safeGet(self.material, "isEmissionEnabled", self.materialSettings.emissionEnabled);
	self.materialSettings.refractionEnabled = safeGet(self.material, "isRefractionEnabled", self.materialSettings.refractionEnabled);
	self.materialSettings.opacity = safeGet(self.material, "getOpacity", self.materialSettings.opacity);
	self.materialSettings.blendMode = safeGet(self.material, "getBlendMode", self.materialSettings.blendMode);
	self.materialSettings.depthWrite = safeGet(self.material, "getDepthWrite", self.materialSettings.depthWrite);
	self.materialSettings.depthTest = safeGet(self.material, "getDepthTest", self.materialSettings.depthTest);
	self.materialSettings.cullMode = safeGet(self.material, "getCullMode", self.materialSettings.cullMode);
	self.materialSettings.doubleSided = self.materialSettings.cullMode == 2;
end

function MaterialEditor:syncToControls()
	for control, binding in pairs(self.controlBindings) do
		if control ~= nil and binding ~= nil then
			local value = self.materialSettings[binding.key];

			if binding.valueType == "float" then
				if value ~= nil then safeCall(control, "setValue", value); end
			elseif binding.valueType == "bool" then
				safeCall(control, "setValue", value == true);
			elseif binding.valueType == "dropdown" then
				value = normaliseDropdownIndex(value or 0, binding.optionCount);
				self.materialSettings[binding.key] = value;
				safeCall(control, "setSelectedOption", value);
			elseif binding.valueType == "string" then
				safeCall(control, "setText", value or "");
			elseif binding.valueType == "colour" then
				local colour = normaliseColourValue(value);
				if colour ~= nil then safeCall(control, "setColour", colour); end
			end
		end
	end
end

function MaterialEditor:getControlValue(control, binding, args)
	if control == nil or binding == nil then
		return nil;
	end

	if binding.valueType == "float" then
		local argValue = safeArg(args, 0, nil);
		if argValue ~= nil then return argValue; end
		local ok, value = safeCall(control, "getValue");
		if ok and value ~= nil then return value; end
	elseif binding.valueType == "bool" then
		local argValue = safeArg(args, 1, nil);
		if argValue ~= nil then return argValue; end
		argValue = safeArg(args, 0, nil);
		if argValue ~= nil then return argValue; end
		local ok, value = safeCall(control, "getValue");
		if ok and value ~= nil then return value; end
	elseif binding.valueType == "dropdown" then
		local ok, value = safeCall(control, "getSelectedOption");
		if not ok or value == nil then
			value = safeArg(args, 0, nil);
		end
		if value ~= nil then
			return normaliseDropdownIndex(value, binding.optionCount);
		end
		return nil;
	elseif binding.valueType == "string" then
		local ok, value = safeCall(control, "getText");
		if ok and value ~= nil then return value; end
		return safeArg(args, 0, nil);
	elseif binding.valueType == "colour" then
		if args ~= nil then
			local firstValue = safeArg(args, 0, nil);
			if firstValue ~= nil and type(firstValue) ~= "number" then
				local colour = normaliseColourValue(firstValue);
				if colour ~= nil then return colour; end
				return firstValue;
			end

			local r = firstValue;
			local g = safeArg(args, 1, nil);
			local b = safeArg(args, 2, nil);
			local a = safeArg(args, 3, 1.0);
			if r ~= nil and g ~= nil and b ~= nil then
				return normaliseColourValue({ r = r, g = g, b = b, a = a });
			end
		end
		local ok, value = safeCall(control, "getColour");
		if ok and value ~= nil then return value; end
	end

	return nil;
end

function MaterialEditor:applySettingToMaterial(binding, value)
	if self.material == nil or binding == nil then
		return;
	end

	self:storeEditorSetting(binding.key, value, binding.valueType);

	if binding.key == "doubleSided" then
		if value == true then
			safeCall(self.material, "setCullMode", 2);
		else
			safeCall(self.material, "setCullMode", self.materialSettings.cullMode or 0);
		end
		return;
	elseif binding.key == "transparent" then
		local transparent = value == true;
		local cutout = false;
		if transparent == false then
			cutout = self.materialSettings.cutout == true;
		end
		self:applySurfaceMode(transparent, cutout);
		return;
	elseif binding.key == "cutout" then
		local cutout = value == true;
		local transparent = false;
		if cutout == false then
			transparent = self.materialSettings.transparent == true;
		end
		self:applySurfaceMode(transparent, cutout);
		return;
	elseif binding.key == "renderMode" then
		safeCall(self.material, "setRenderMode", value);
		self:syncSurfaceSettingsFromMaterial();
		self:storeEditorSetting("transparent", self.materialSettings.transparent, "bool");
		self:storeEditorSetting("cutout", self.materialSettings.cutout, "bool");
		return;
	elseif binding.key == "cullMode" and value ~= 2 then
		self.materialSettings.doubleSided = false;
		self:storeEditorSetting("doubleSided", false, "bool");
	elseif binding.key == "uvTilingX" or binding.key == "uvTilingY" then
		local tiling = safeVector2(self.materialSettings.uvTilingX, self.materialSettings.uvTilingY);
		if tiling ~= nil then
			safeCall(self.material, "setUVTiling", tiling);
		end
		return;
	elseif binding.key == "uvOffsetX" or binding.key == "uvOffsetY" then
		local offset = safeVector2(self.materialSettings.uvOffsetX, self.materialSettings.uvOffsetY);
		if offset ~= nil then
			safeCall(self.material, "setUVOffset", offset);
		end
		return;
	elseif binding.key == "detailTilingX" or binding.key == "detailTilingY" then
		safeCall(self.material, "setEditorFloat", "detailTilingX", self.materialSettings.detailTilingX);
		safeCall(self.material, "setEditorFloat", "detailTilingY", self.materialSettings.detailTilingY);
		return;
	elseif binding.key == "detailOffsetX" or binding.key == "detailOffsetY" then
		safeCall(self.material, "setEditorFloat", "detailOffsetX", self.materialSettings.detailOffsetX);
		safeCall(self.material, "setEditorFloat", "detailOffsetY", self.materialSettings.detailOffsetY);
		return;
	end

	if binding.setterName == nil then
		return;
	end

	if binding.valueType == "colour" and value ~= nil then
		local colour = normaliseColourValue(value);
		if colour ~= nil then safeCall(self.material, binding.setterName, colour); end
		return;
	end

	safeCall(self.material, binding.setterName, value);
end

function MaterialEditor:onSettingChanged(control, args, elementId)
	local binding = self.controlBindings[control];
	if binding == nil and elementId ~= nil and self.controlBindingsById ~= nil then
		binding = self.controlBindingsById[elementId];
	end

	if binding == nil then
		return;
	end

	local valueControl = binding.control or control;
	local value = self:getControlValue(valueControl, binding, args);
	if value == nil then
		return;
	end

	self.materialSettings[binding.key] = value;
	self:applySettingToMaterial(binding, value);

	if binding.key == "renderMode" or
		binding.key == "opacity" or
		binding.key == "transparent" or
		binding.key == "cutout" or
		binding.key == "blendMode" or
		binding.key == "depthWrite" or
		binding.key == "depthTest" or
		binding.key == "cullMode" or
		binding.key == "doubleSided" then
		self:syncDerivedSettingsFromMaterial();
		self:syncToControls();
	end

	if self.statusText then
		self.statusText:setText("Changed: "..binding.key);
	end
end

function MaterialEditor:resetUVSettings()
	self.materialSettings.uvTilingX = MaterialEditorDefaults.uvTilingX;
	self.materialSettings.uvTilingY = MaterialEditorDefaults.uvTilingY;
	self.materialSettings.uvOffsetX = MaterialEditorDefaults.uvOffsetX;
	self.materialSettings.uvOffsetY = MaterialEditorDefaults.uvOffsetY;
	self.materialSettings.uvRotation = MaterialEditorDefaults.uvRotation;
	self.materialSettings.uvSet = MaterialEditorDefaults.uvSet;
	self.materialSettings.uvProjection = MaterialEditorDefaults.uvProjection;
	self.materialSettings.uvTriplanarScale = MaterialEditorDefaults.uvTriplanarScale;

	self.materialSettings.detailTilingX = MaterialEditorDefaults.detailTilingX;
	self.materialSettings.detailTilingY = MaterialEditorDefaults.detailTilingY;
	self.materialSettings.detailOffsetX = MaterialEditorDefaults.detailOffsetX;
	self.materialSettings.detailOffsetY = MaterialEditorDefaults.detailOffsetY;
	self.materialSettings.detailRotation = MaterialEditorDefaults.detailRotation;

	self:syncToControls();

	if self.material then
		local tiling = safeVector2(self.materialSettings.uvTilingX, self.materialSettings.uvTilingY);
		if tiling ~= nil then
			safeCall(self.material, "setUVTiling", tiling);
		end

		local offset = safeVector2(self.materialSettings.uvOffsetX, self.materialSettings.uvOffsetY);
		if offset ~= nil then
			safeCall(self.material, "setUVOffset", offset);
		end

		safeCall(self.material, "setUVRotation", self.materialSettings.uvRotation);
		safeCall(self.material, "setUVSet", self.materialSettings.uvSet);
		safeCall(self.material, "setUVProjection", self.materialSettings.uvProjection);
		safeCall(self.material, "setTriplanarScale", self.materialSettings.uvTriplanarScale);
		self:storeEditorSetting("detailTilingX", self.materialSettings.detailTilingX, "float");
		self:storeEditorSetting("detailTilingY", self.materialSettings.detailTilingY, "float");
		self:storeEditorSetting("detailOffsetX", self.materialSettings.detailOffsetX, "float");
		self:storeEditorSetting("detailOffsetY", self.materialSettings.detailOffsetY, "float");
		self:storeEditorSetting("detailRotation", self.materialSettings.detailRotation, "float");
		safeCall(self.material, "save");
	end

	if self.statusText then
		self.statusText:setText("UV settings reset.");
	end
end

function MaterialEditor:copyUVSettings()
	self.copiedUV =
	{
		uvTilingX = self.materialSettings.uvTilingX,
		uvTilingY = self.materialSettings.uvTilingY,
		uvOffsetX = self.materialSettings.uvOffsetX,
		uvOffsetY = self.materialSettings.uvOffsetY,
		uvRotation = self.materialSettings.uvRotation,
		uvSet = self.materialSettings.uvSet,
		uvProjection = self.materialSettings.uvProjection,
		uvTriplanarScale = self.materialSettings.uvTriplanarScale,
		detailTilingX = self.materialSettings.detailTilingX,
		detailTilingY = self.materialSettings.detailTilingY,
		detailOffsetX = self.materialSettings.detailOffsetX,
		detailOffsetY = self.materialSettings.detailOffsetY,
		detailRotation = self.materialSettings.detailRotation,
	};

	if self.statusText then
		self.statusText:setText("Copied UV settings.");
	end
end

function MaterialEditor:pasteUVSettings()
	if self.copiedUV == nil then
		if self.statusText then
			self.statusText:setText("No copied UV settings.");
		end
		return;
	end

	for key, value in pairs(self.copiedUV) do
		self.materialSettings[key] = value;
	end

	self:syncToControls();

	if self.material then
		local tiling = safeVector2(self.materialSettings.uvTilingX, self.materialSettings.uvTilingY);
		if tiling ~= nil then
			safeCall(self.material, "setUVTiling", tiling);
		end

		local offset = safeVector2(self.materialSettings.uvOffsetX, self.materialSettings.uvOffsetY);
		if offset ~= nil then
			safeCall(self.material, "setUVOffset", offset);
		end

		safeCall(self.material, "setUVRotation", self.materialSettings.uvRotation);
		for key, value in pairs(self.copiedUV) do
			local valueType = "float";
			if MaterialEditorUIntKeys[key] == true then
				valueType = "dropdown";
			end
			self:storeEditorSetting(key, value, valueType);
		end
		safeCall(self.material, "setUVSet", self.materialSettings.uvSet);
		safeCall(self.material, "setUVProjection", self.materialSettings.uvProjection);
		safeCall(self.material, "setTriplanarScale", self.materialSettings.uvTriplanarScale);
		safeCall(self.material, "save");
	end

	if self.statusText then
		self.statusText:setText("Pasted UV settings.");
	end
end

function MaterialEditor:applyAllSettings()
	for _, control in ipairs(self.controls) do
		local binding = self.controlBindings[control];
		if binding ~= nil and binding.valueType ~= "button" then
			local value = self:getControlValue(control, binding, nil);
			if value ~= nil then
				self.materialSettings[binding.key] = value;
				self:applySettingToMaterial(binding, value);
			end
		end
	end

	-- Render mode and the two convenience toggles describe the same state. Apply
	-- them once so stale controls cannot overwrite each other while saving.
	self:applySurfaceMode(self.materialSettings.transparent,
		self.materialSettings.cutout, self.materialSettings.renderMode);
	self:syncToControls();


	if self.statusText then
		self.statusText:setText("Applied material settings.");
	end
end

function MaterialEditor:saveMaterial()
	self:applyAllSettings();

	if self.material then
		safeCall(self.material, "save");
	end

	if self.statusText then
		self.statusText:setText("Saved material.");
	end
end

function MaterialEditor:validateMaterial()
	local warnings = {};

	if self.material == nil then
		warnings[#warnings + 1] = "No material selected.";
	end

	if self.materialSettings.uvTilingX == 0.0 or self.materialSettings.uvTilingY == 0.0 then
		warnings[#warnings + 1] = "UV tiling has a zero axis.";
	end

	if self.materialSettings.transparent and self.materialSettings.depthWrite then
		warnings[#warnings + 1] = "Transparent material has depth write enabled.";
	end

	if self.materialSettings.normalMap == true and self.normal == nil then
		warnings[#warnings + 1] = "Normal map mode enabled but no normal slot is available.";
	end

	if #warnings == 0 then
		if self.statusText then
			self.statusText:setText("Validation passed.");
		end
	else
		local message = "Validation: "..table.concat(warnings, " ");
		print(message);
		if self.statusText then
			self.statusText:setText(message);
		end
	end
end

function MaterialEditor:handleAction(elementId)
	if elementId == MaterialEditorTypes.ApplyId then
		self:applyAllSettings();
	elseif elementId == MaterialEditorTypes.SaveId then
		self:saveMaterial();
	elseif elementId == MaterialEditorTypes.ReloadShaderId then
		if self.material then safeCall(self.material, "reloadShader"); end
		if self.statusText then self.statusText:setText("Reload shader requested."); end
	elseif elementId == MaterialEditorTypes.ValidateId then
		self:validateMaterial();
	elseif elementId == MaterialEditorTypes.ResetUvId then
		self:resetUVSettings();
	elseif elementId == MaterialEditorTypes.CopyUvId then
		self:copyUVSettings();
	elseif elementId == MaterialEditorTypes.PasteUvId then
		self:pasteUVSettings();
	elseif elementId == MaterialEditorTypes.DuplicateId then
		if self.material then safeCall(self.material, "duplicate"); end
		if self.statusText then self.statusText:setText("Duplicate material requested."); end
	elseif elementId == MaterialEditorTypes.CreateInstanceId then
		if self.material then safeCall(self.material, "createInstance"); end
		if self.statusText then self.statusText:setText("Create runtime instance requested."); end
	elseif elementId == MaterialEditorTypes.ExportJsonId then
		if self.material then safeCall(self.material, "exportJson"); end
		if self.statusText then self.statusText:setText("Export JSON requested."); end
	elseif elementId == MaterialEditorTypes.ImportJsonId then
		if self.material then safeCall(self.material, "importJson"); end
		if self.statusText then self.statusText:setText("Import JSON requested."); end
	end
end

function MaterialEditor:getCurrentMaterial()
	if self.material ~= nil then
		return self.material;
	end

	if self.materialComponent ~= nil then
		local okMaterial, material = safeCall(self.materialComponent, "getMaterial");
		if okMaterial and material ~= nil then
			self.material = material;
		end
	end

	return self.material;
end

function MaterialEditor:handleDrop(args, elementId)
	print("MaterialEditor handleDrop called");

	local okJson, cjson = pcall(require, "cjson");
	if not okJson or cjson == nil then
		if self.statusText then
			self.statusText:setText("Unable to decode drop data.");
		end
		return;
	end

	local dataStr = safeArg(args, 0, nil);
	if dataStr == nil then
		return;
	end

	print(dataStr);

	local okDecode, decoded = pcall(function()
		return cjson.decode(dataStr);
	end);

	if not okDecode or decoded == nil then
		if self.statusText then
			self.statusText:setText("Invalid drop data.");
		end
		return;
	end

	local path = decoded.filePath or decoded.path;
	if path == nil then
		return;
	end

	local applicationManager = IApplicationManager.instance();
	local resourceDatabase = applicationManager:getResourceDatabase();

	if isTexturePath(path) then
		local texture = resourceDatabase:loadResource(path);
		local slot = self.textureSlotIds[elementId];
		if slot ~= nil then
			local material = self:getCurrentMaterial();
			slot:setMaterial(material);

			if material ~= nil and slot:setTexture(texture) then
				if self.statusText then
					self.statusText:setText("Texture assigned: "..path);
				end
			else
				if self.statusText then
					self.statusText:setText("No material selected for texture assignment.");
				end
			end
		end
	elseif containsText(path, ".mat") then
		if self.selectedMaterialFile then
			self.selectedMaterialFile:setText(path);
		end

		if self.materialComponent then
			self.materialComponent:setMaterialPath(path);
			self.material = self.materialComponent:getMaterial();
		else
			self.material = resourceDatabase:loadResource(path);
		end

		self:updateSelectedMaterial();

		if self.statusText then
			self.statusText:setText("Material loaded: "..path);
		end
	end
end

function MaterialEditor:handleEvent(parameters, results)
--function MaterialEditor:handleEvent(eventType, eventValue, parameters, sender, object, event)
	print("MaterialEditor handleEvent called: ");

	if self.material == nil then
		print("MaterialEditor handleEvent material nil");
	else
		print("MaterialEditor handleEvent material: "..tostring(safeGet(self.material, "getName", "<unnamed>")));
	end

	if self.window == nil then
		print("MaterialEditor window nil");
		return;
	end

	if  not self.window:isWindowVisible() then
		print("MaterialEditor not visible window: "..tostring(safeGet(self.window, "getName", "<unnamed>")));
		return;
	end

	if parameters == nil then
		print("MaterialEditor parameters nil");
		return;
	end

	local eventHash = parameters:at(1);
	local args = parameters:at(2);
	local sender = parameters:at(3);
	if sender == nil then
		print("MaterialEditor sender nil");
		return;
	end

	local okElementId, elementId = safeCall(sender, "getElementId");
	if not okElementId or elementId == nil then
		print("MaterialEditor sender has no element id");
		return;
	end

	if eventHash == IEvent.handleSelection then
		print("MaterialEditor handleSelection called");
		local binding = self.controlBindings[sender];
		if binding == nil and self.controlBindingsById ~= nil then
			binding = self.controlBindingsById[elementId];
		end

		if binding ~= nil then
			self:onSettingChanged(sender, args, elementId);
		else
		self:handleAction(elementId);
		end
	elseif eventHash == IEvent.handleValueChanged then
		print("MaterialEditor handleValueChanged called");
		self:onSettingChanged(sender, args, elementId);
	elseif eventHash == IEvent.handleDrag then
		print("MaterialEditor handleDrag called");
	elseif eventHash == IEvent.handleDrop then
		self:handleDrop(args, elementId);
	end

	for _, slot in pairs(self.textureSlots) do
		if slot then
			slot:handleEvent(parameters, results);
		end
	end

	if self.material then
		self.material:save();
	end
end

function MaterialEditor:getProperties(parameters)
	local properties = safeParameterAt(parameters, 0, nil);
	if properties == nil then
		return;
	end

	for key, value in pairs(self.materialSettings) do
		if type(value) == "boolean" then
			properties:setPropertyAsBool(key, value);
		elseif type(value) == "number" then
			properties:setPropertyAsFloat(key, value);
		elseif type(value) == "string" then
			properties:setPropertyAsString(key, value);
		end
	end
end

function MaterialEditor:setProperties(parameters)
	local properties = safeParameterAt(parameters, 0, nil);
	if properties == nil then
		return;
	end

	for key, defaultValue in pairs(MaterialEditorDefaults) do
		if properties:hasProperty(key) then
			if type(defaultValue) == "boolean" then
				self.materialSettings[key] = properties:getPropertyAsBool(key);
			elseif type(defaultValue) == "number" then
				self.materialSettings[key] = properties:getPropertyAsFloat(key);
			elseif type(defaultValue) == "string" then
				self.materialSettings[key] = properties:getPropertyAsString(key);
			end
		end
	end

	self:syncToControls();
	self:applyAllSettings();
end
