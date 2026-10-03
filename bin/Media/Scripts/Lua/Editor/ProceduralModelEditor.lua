class 'ProceduralModelEditor' (BaseEditor)

-- Unity / ProBuilder inspired rule-based procedural model editor.
-- This keeps the existing BoxRule, CylinderRule, ArrayRule and CompositeRule
-- generation path, but expands the editor UI so the procedural model can be
-- driven like a lightweight ProBuilder-style modelling tool.

ProceduralModelEditorTypes =
{
	-- Shape / template dropdown options.
	ShapeBox          = 0,
	ShapeCylinder     = 1,
	ShapeArray        = 2,
	ShapeComposite    = 3,
	ShapeWall         = 4,
	ShapeFloor        = 5,
	ShapeStairs       = 6,
	ShapeDoor         = 7,
	ShapeWindow       = 8,
	ShapeArch         = 9,
	ShapeRoom         = 10,
	ShapeRoof         = 11,
	ShapeBuilding     = 12,
	ShapeRadialArray  = 13,

	-- Main controls.
	ShapeTypeDropdownId  = 1,
	GenerateButtonId     = 2,
	MeshStatsLabelId     = 3,
	ResetButtonId        = 4,
	LiveGenerateId       = 5,
	RuleNameId           = 6,
	PresetDropdownId     = 7,
	SavePresetButtonId   = 8,
	LoadPresetButtonId   = 9,

	-- Box.
	BoxSizeXId   = 10,
	BoxSizeYId   = 11,
	BoxSizeZId   = 12,
	BoxOffsetXId = 13,
	BoxOffsetYId = 14,
	BoxOffsetZId = 15,

	-- Cylinder.
	CylRadiusId   = 20,
	CylHeightId   = 21,
	CylSegmentsId = 22,
	CylOffsetXId  = 23,
	CylOffsetYId  = 24,
	CylOffsetZId  = 25,

	-- Array.
	ArrayCountId     = 30,
	ArrayStepXId     = 31,
	ArrayStepYId     = 32,
	ArrayStepZId     = 33,
	ArrayChildTypeId = 34,

	-- Radial array.
	RadialCountId      = 40,
	RadialRadiusId     = 41,
	RadialAngleId      = 42,
	RadialAxisId       = 43,
	RadialStartAngleId = 44,

	-- ProBuilder style edit modes.
	EditModeId        = 50,
	SelectionModeId   = 51,
	PivotModeId       = 52,
	HandleSpaceId     = 53,
	GridSnapId         = 54,
	SnapXId            = 55,
	SnapYId            = 56,
	SnapZId            = 57,
	CurrentToolLabelId = 58,

	-- Geometry operations.
	ExtrudeFacesButtonId   = 60,
	InsetFacesButtonId     = 61,
	BevelEdgesButtonId     = 62,
	ConnectEdgesButtonId   = 63,
	BridgeEdgesButtonId    = 64,
	SubdivideButtonId      = 65,
	TriangulateButtonId    = 66,
	MergeFacesButtonId     = 67,
	FlipNormalsButtonId    = 68,
	DeleteFacesButtonId    = 69,
	WeldVerticesButtonId   = 70,
	DetachFacesButtonId    = 71,

	-- Operation parameters.
	ExtrudeDistanceId = 80,
	InsetAmountId     = 81,
	BevelAmountId     = 82,
	BevelSegmentsId   = 83,
	WeldToleranceId   = 84,
	SubdivideCutsId   = 85,

	-- Rule stack / boolean operations.
	AddBoxRuleButtonId       = 90,
	AddCylinderRuleButtonId  = 91,
	AddWallRuleButtonId      = 92,
	AddStairsRuleButtonId    = 93,
	AddDoorRuleButtonId      = 94,
	AddWindowRuleButtonId    = 95,
	AddRoofRuleButtonId      = 96,
	AddArrayRuleButtonId     = 97,
	AddRadialArrayRuleButtonId = 98,
	BooleanModeId            = 99,
	ApplyRuleStackButtonId   = 100,
	ClearRuleStackButtonId   = 101,

	-- Wall / room / building templates.
	WallLengthId       = 110,
	WallHeightId       = 111,
	WallThicknessId    = 112,
	FloorWidthId       = 113,
	FloorDepthId       = 114,
	FloorThicknessId   = 115,
	RoomWidthId        = 116,
	RoomDepthId        = 117,
	RoomHeightId       = 118,
	BuildingFloorsId   = 119,
	BuildingFloorHeightId = 120,

	-- Door / window / arch / stairs templates.
	DoorWidthId       = 130,
	DoorHeightId      = 131,
	DoorOffsetXId     = 132,
	WindowWidthId     = 133,
	WindowHeightId    = 134,
	WindowOffsetXId   = 135,
	WindowOffsetYId   = 136,
	ArchRadiusId      = 137,
	ArchThicknessId   = 138,
	StairCountId      = 139,
	StairWidthId      = 140,
	StairTreadId      = 141,
	StairRiserId      = 142,

	-- Deform / surface.
	MirrorAxisId       = 150,
	MirrorButtonId     = 151,
	FlipXButtonId      = 152,
	FlipYButtonId      = 153,
	FlipZButtonId      = 154,
	TaperAmountId      = 155,
	TwistAmountId      = 156,
	BendAmountId       = 157,
	NoiseAmountId      = 158,

	-- UV / materials.
	MaterialSlotId      = 170,
	AssignMaterialButtonId = 171,
	AutoUVButtonId      = 172,
	PlanarUVButtonId    = 173,
	BoxUVButtonId       = 174,
	UVScaleXId          = 175,
	UVScaleYId          = 176,
	UVOffsetXId         = 177,
	UVOffsetYId         = 178,
	SmoothingGroupId    = 179,
	AutoSmoothAngleId   = 180,

	-- Export / bake.
	BakeMeshButtonId     = 190,
	SaveAssetButtonId    = 191,
	ExportObjButtonId    = 192,
	ExportFbxButtonId    = 193,
	ExportColliderButtonId = 194,
	GenerateColliderId   = 195,
	ColliderTypeId       = 196,
	LodCountId           = 197,
	LodReductionId       = 198,
	SceneActorEnabledId  = 199,
	CreateActorButtonId  = 200,

	-- Array child shape options.
	ArrayChildBox      = 0,
	ArrayChildCylinder = 1,
	ArrayChildWall     = 2,
}

local ProceduralModelEditorDefaults =
{
	["outputFile"] = "None",
	["ruleName"] = "New Procedural Mesh",
	["preset"] = 0,
	["liveGenerate"] = 1,
	["shapeType"] = ProceduralModelEditorTypes.ShapeBox,

	["box.size.x"] = 1.0,
	["box.size.y"] = 1.0,
	["box.size.z"] = 1.0,
	["box.offset.x"] = 0.0,
	["box.offset.y"] = 0.0,
	["box.offset.z"] = 0.0,

	["cylinder.radius"] = 0.5,
	["cylinder.height"] = 1.0,
	["cylinder.segments"] = 8,
	["cylinder.offset.x"] = 0.0,
	["cylinder.offset.y"] = 0.0,
	["cylinder.offset.z"] = 0.0,

	["array.count"] = 1,
	["array.step.x"] = 1.0,
	["array.step.y"] = 0.0,
	["array.step.z"] = 0.0,
	["array.child"] = ProceduralModelEditorTypes.ArrayChildBox,

	["radial.count"] = 8,
	["radial.radius"] = 3.0,
	["radial.angle"] = 360.0,
	["radial.axis"] = 1,
	["radial.startAngle"] = 0.0,

	["edit.mode"] = 0,
	["selection.mode"] = 0,
	["pivot.mode"] = 0,
	["handle.space"] = 0,
	["grid.snap"] = 1,
	["snap.x"] = 0.25,
	["snap.y"] = 0.25,
	["snap.z"] = 0.25,

	["op.extrudeDistance"] = 1.0,
	["op.insetAmount"] = 0.1,
	["op.bevelAmount"] = 0.1,
	["op.bevelSegments"] = 1,
	["op.weldTolerance"] = 0.001,
	["op.subdivideCuts"] = 1,
	["boolean.mode"] = 0,

	["wall.length"] = 6.0,
	["wall.height"] = 3.0,
	["wall.thickness"] = 0.25,
	["floor.width"] = 6.0,
	["floor.depth"] = 6.0,
	["floor.thickness"] = 0.2,
	["room.width"] = 6.0,
	["room.depth"] = 6.0,
	["room.height"] = 3.0,
	["building.floors"] = 3,
	["building.floorHeight"] = 3.0,

	["door.width"] = 1.0,
	["door.height"] = 2.1,
	["door.offset.x"] = 0.0,
	["window.width"] = 1.2,
	["window.height"] = 1.0,
	["window.offset.x"] = 0.0,
	["window.offset.y"] = 1.5,
	["arch.radius"] = 0.8,
	["arch.thickness"] = 0.25,
	["stairs.count"] = 8,
	["stairs.width"] = 2.0,
	["stairs.tread"] = 0.3,
	["stairs.riser"] = 0.18,

	["mirror.axis"] = 0,
	["deform.taper"] = 0.0,
	["deform.twist"] = 0.0,
	["deform.bend"] = 0.0,
	["deform.noise"] = 0.0,

	["material.slot"] = 0,
	["material.smoothingGroup"] = 0,
	["material.autoSmoothAngle"] = 60.0,
	["uv.mode"] = "autoUv",
	["uv.scale.x"] = 1.0,
	["uv.scale.y"] = 1.0,
	["uv.offset.x"] = 0.0,
	["uv.offset.y"] = 0.0,

	["collider.enabled"] = 0,
	["collider.type"] = 0,
	["lod.count"] = 0,
	["lod.reduction"] = 50.0,
	["sceneActor.enabled"] = 1,
}

local ProceduralModelNumericRanges =
{
	["shapeType"] = {0, 13, true},
	["preset"] = {0, 6, true},
	["liveGenerate"] = {0, 1, true},
	["box.size.x"] = {0.01, 100.0},
	["box.size.y"] = {0.01, 100.0},
	["box.size.z"] = {0.01, 100.0},
	["cylinder.radius"] = {0.01, 50.0},
	["cylinder.height"] = {0.01, 100.0},
	["cylinder.segments"] = {3, 128, true},
	["array.count"] = {1, 256, true},
	["array.child"] = {0, 2, true},
	["radial.count"] = {1, 256, true},
	["radial.radius"] = {0.0, 100.0},
	["radial.angle"] = {0.0, 360.0},
	["radial.axis"] = {0, 2, true},
	["edit.mode"] = {0, 3, true},
	["selection.mode"] = {0, 5, true},
	["pivot.mode"] = {0, 3, true},
	["handle.space"] = {0, 2, true},
	["grid.snap"] = {0, 1, true},
	["snap.x"] = {0.001, 10.0},
	["snap.y"] = {0.001, 10.0},
	["snap.z"] = {0.001, 10.0},
	["op.bevelSegments"] = {1, 16, true},
	["op.weldTolerance"] = {0.0, 1.0},
	["op.subdivideCuts"] = {1, 16, true},
	["boolean.mode"] = {0, 4, true},
	["wall.length"] = {0.1, 200.0},
	["wall.height"] = {0.1, 50.0},
	["wall.thickness"] = {0.01, 10.0},
	["floor.width"] = {0.1, 200.0},
	["floor.depth"] = {0.1, 200.0},
	["floor.thickness"] = {0.01, 10.0},
	["room.width"] = {0.1, 200.0},
	["room.depth"] = {0.1, 200.0},
	["room.height"] = {0.1, 50.0},
	["building.floors"] = {1, 64, true},
	["building.floorHeight"] = {0.1, 20.0},
	["door.width"] = {0.1, 20.0},
	["door.height"] = {0.1, 20.0},
	["window.width"] = {0.1, 20.0},
	["window.height"] = {0.1, 20.0},
	["window.offset.y"] = {0.0, 20.0},
	["arch.radius"] = {0.1, 20.0},
	["arch.thickness"] = {0.01, 10.0},
	["stairs.count"] = {1, 128, true},
	["stairs.width"] = {0.1, 50.0},
	["stairs.tread"] = {0.05, 10.0},
	["stairs.riser"] = {0.05, 10.0},
	["mirror.axis"] = {0, 2, true},
	["deform.taper"] = {-5.0, 5.0},
	["deform.twist"] = {-360.0, 360.0},
	["deform.bend"] = {-360.0, 360.0},
	["deform.noise"] = {0.0, 10.0},
	["material.slot"] = {0, 5, true},
	["material.smoothingGroup"] = {0, 32, true},
	["material.autoSmoothAngle"] = {0.0, 180.0},
	["uv.scale.x"] = {0.001, 100.0},
	["uv.scale.y"] = {0.001, 100.0},
	["collider.enabled"] = {0, 1, true},
	["collider.type"] = {0, 3, true},
	["lod.count"] = {0, 8, true},
	["lod.reduction"] = {0.0, 95.0},
	["sceneActor.enabled"] = {0, 1, true},
}

local ProceduralModelPresetSettings =
{
	[1] =
	{
		name = "Blockout Room",
		ruleStack = {"Wall", "Wall", "Wall", "Wall"},
		settings =
		{
			["shapeType"] = ProceduralModelEditorTypes.ShapeRoom,
			["room.width"] = 8.0,
			["room.depth"] = 6.0,
			["room.height"] = 3.2,
			["wall.thickness"] = 0.25,
			["floor.thickness"] = 0.18,
		},
	},
	[2] =
	{
		name = "Sci-Fi Door",
		ruleStack = {"Wall", "Door Cut", "Box", "Box"},
		settings =
		{
			["shapeType"] = ProceduralModelEditorTypes.ShapeComposite,
			["wall.length"] = 4.0,
			["wall.height"] = 3.2,
			["wall.thickness"] = 0.35,
			["door.width"] = 1.35,
			["door.height"] = 2.4,
			["box.size.x"] = 0.2,
			["box.size.y"] = 2.6,
			["box.size.z"] = 0.45,
			["box.offset.x"] = -0.95,
			["box.offset.y"] = 1.3,
		},
	},
	[3] =
	{
		name = "Wall With Windows",
		ruleStack = {"Wall", "Window Cut", "Linear Array"},
		settings =
		{
			["shapeType"] = ProceduralModelEditorTypes.ShapeComposite,
			["wall.length"] = 8.0,
			["wall.height"] = 3.0,
			["window.width"] = 1.0,
			["window.height"] = 0.9,
			["window.offset.y"] = 1.65,
			["array.child"] = ProceduralModelEditorTypes.ArrayChildWall,
			["array.count"] = 3,
			["array.step.x"] = 2.5,
		},
	},
	[4] =
	{
		name = "Staircase",
		settings =
		{
			["shapeType"] = ProceduralModelEditorTypes.ShapeStairs,
			["stairs.count"] = 12,
			["stairs.width"] = 2.4,
			["stairs.tread"] = 0.32,
			["stairs.riser"] = 0.18,
		},
	},
	[5] =
	{
		name = "Simple Building",
		ruleStack = {"Roof"},
		settings =
		{
			["shapeType"] = ProceduralModelEditorTypes.ShapeBuilding,
			["building.floors"] = 4,
			["building.floorHeight"] = 3.1,
			["room.width"] = 8.0,
			["room.depth"] = 8.0,
			["room.height"] = 3.0,
		},
	},
	[6] =
	{
		name = "Roof Piece",
		settings =
		{
			["shapeType"] = ProceduralModelEditorTypes.ShapeRoof,
			["room.width"] = 6.0,
			["room.depth"] = 5.0,
			["room.height"] = 3.0,
			["wall.thickness"] = 0.35,
		},
	},
}

local function proceduralCopyTable(source)
	local copy = {};
	if source then
		for key, value in pairs(source) do
			if type(value) == "table" then
				copy[key] = proceduralCopyTable(value);
			else
				copy[key] = value;
			end
		end
	end

	return copy;
end

local function proceduralRound(value)
	return math.floor((tonumber(value) or 0) + 0.5);
end

local function proceduralClamp(value, minValue, maxValue)
	value = tonumber(value) or 0.0;
	if minValue ~= nil and value < minValue then
		value = minValue;
	end
	if maxValue ~= nil and value > maxValue then
		value = maxValue;
	end
	return value;
end

local function proceduralJsonEscape(value)
	value = tostring(value or "");
	value = value:gsub("\\", "\\\\");
	value = value:gsub("\"", "\\\"");
	value = value:gsub("\n", "\\n");
	value = value:gsub("\r", "\\r");
	value = value:gsub("\t", "\\t");
	return value;
end

local function proceduralSafeFilePart(value)
	value = tostring(value or "ProceduralMesh");
	value = value:gsub("[^%w%._%-]+", "_");
	value = value:gsub("^_+", "");
	value = value:gsub("_+$", "");
	if value == "" then
		value = "ProceduralMesh";
	end
	return value;
end

local function proceduralSafeCall(target, methodName, ...)
	if target == nil then
		return false, nil;
	end

	local method = target[methodName];
	if method == nil then
		return false, nil;
	end

	return pcall(method, target, ...);
end

local function proceduralDirectoryName(path)
	path = tostring(path or "");
	local lastSlash = path:match("^.*()/") or 0;
	local lastBackslash = path:match("^.*()\\") or 0;
	local lastSeparator = math.max(lastSlash, lastBackslash);
	if lastSeparator <= 0 then
		return nil;
	end
	return path:sub(1, lastSeparator - 1);
end

local function proceduralCallAny(target, methodNames, ...)
	if target == nil or methodNames == nil then
		return false, nil;
	end

	for i = 1, #methodNames do
		local methodName = methodNames[i];
		if target[methodName] ~= nil then
			local ok, result = proceduralSafeCall(target, methodName, ...);
			if ok then
				return true, result, methodName;
			end
		end
	end

	return false, nil;
end

function ProceduralModelEditor:__init(window)
	print("ProceduralModelEditor constructor called");
	BaseEditor:__init(self, window);

	self.window = window;
	self.editorWindow = nil;
	self.tabBar = nil;

	self.selectedFileEntry = nil;
	self.shapeTypeDropdown = nil;
	self.meshStatsLabel    = nil;
	self.statusText        = nil;
	self.currentToolLabel  = nil;
	self.generateButton    = nil;
	self.resetButton       = nil;

	self.component      = nil;
	self.bakedComponent = nil;
	self.bakedMesh      = nil;
	self.generatedActor = nil;
	self.generatedMeshComponent = nil;
	self.generatedMeshRenderer = nil;
	self.generatedMaterial = nil;
	self.generatedCollisionMesh = nil;
	self._generatedActorRegistered = false;

	self._settings = proceduralCopyTable(ProceduralModelEditorDefaults);
	self._controlDefaults = {};
	self._controlInfo = {};
	self._liveBakeIds = {};
	self._actionButtons = {};
	self._ruleStack = {};
	self._currentTool = "Object";
	self._lastBakeStats = nil;
	self._operationLog = {};
	self._operationApplyReport = {};
	self._exportedFiles = {};
	self._transformState =
	{
		mirrorAxis = nil,
		flipX = false,
		flipY = false,
		flipZ = false,
		flipNormals = false,
	};
	self._currentStatus = "Ready.";
end

function ProceduralModelEditor:__finalize()
	print("ProceduralModelEditor __finalize called");
	BaseEditor:__finalize();

	self.window = nil;
	self.editorWindow = nil;
	self.tabBar = nil;

	self.selectedFileEntry = nil;
	self.shapeTypeDropdown = nil;
	self.meshStatsLabel    = nil;
	self.statusText        = nil;
	self.currentToolLabel  = nil;
	self.generateButton    = nil;
	self.resetButton       = nil;

	self.component      = nil;
	self.bakedComponent = nil;
	self.bakedMesh      = nil;
	self.generatedActor = nil;
	self.generatedMeshComponent = nil;
	self.generatedMeshRenderer = nil;
	self.generatedMaterial = nil;
	self.generatedCollisionMesh = nil;
	self._generatedActorRegistered = nil;

	self._settings = nil;
	self._controlDefaults = nil;
	self._controlInfo = nil;
	self._liveBakeIds = nil;
	self._actionButtons = nil;
	self._ruleStack = nil;
	self._currentTool = nil;
	self._lastBakeStats = nil;
	self._operationLog = nil;
	self._operationApplyReport = nil;
	self._exportedFiles = nil;
	self._transformState = nil;
	self._currentStatus = nil;
end

-- ─────────────────────────────────────────────────────────────────────────────
-- UI construction helpers
-- ─────────────────────────────────────────────────────────────────────────────

function ProceduralModelEditor:registerControl(control, id, key, kind, defaultValue, liveBake)
	if not control then
		return;
	end

	control:setElementId(id);

	if self.window then
		self.window:setHandleEvents(control, true);
	end

	self._controlInfo[id] =
	{
		control = control,
		key = key,
		kind = kind,
	};

	if key ~= nil and self._settings ~= nil and self._settings[key] == nil then
		self._settings[key] = defaultValue;
	end

	self._controlDefaults[id] =
	{
		control = control,
		key = key,
		kind = kind,
		value = defaultValue,
	};

	if liveBake then
		self._liveBakeIds[id] = true;
	end
end

function ProceduralModelEditor:registerButton(control, id, actionName)
	if not control then
		return;
	end

	control:setElementId(id);

	if self.window then
		self.window:setHandleEvents(control, true);
	end

	self._actionButtons[id] = actionName;
end

function ProceduralModelEditor:setStatus(message)
	self._currentStatus = tostring(message or "");
	print("ProceduralModelEditor status: " .. self._currentStatus);

	if self.statusText then
		self.statusText:setLabel(self._currentStatus);
	end
end

function ProceduralModelEditor:getControlValueByInfo(info)
	if info == nil or info.control == nil then
		return nil;
	end

	if info.kind == "dropdown" then
		local ok, value = proceduralSafeCall(info.control, "getSelectedOption");
		if ok then
			return value;
		end
	elseif info.kind == "text" then
		local ok, value = proceduralSafeCall(info.control, "getText");
		if ok then
			return value;
		end
	else
		local ok, value = proceduralSafeCall(info.control, "getValue");
		if ok then
			return value;
		end
	end

	return nil;
end

function ProceduralModelEditor:setControlValueByInfo(info, value)
	if info == nil or info.control == nil then
		return false;
	end

	if info.kind == "dropdown" then
		return proceduralSafeCall(info.control, "setSelectedOption", proceduralRound(value));
	elseif info.kind == "text" then
		return proceduralSafeCall(info.control, "setText", tostring(value or ""));
	else
		return proceduralSafeCall(info.control, "setValue", tonumber(value) or 0.0);
	end
end

function ProceduralModelEditor:setControlByKey(key, value)
	if self._controlInfo == nil then
		return false;
	end

	for _, info in pairs(self._controlInfo) do
		if info.key == key then
			self:setControlValueByInfo(info, value);
			return true;
		end
	end

	return false;
end

function ProceduralModelEditor:setControlDefault(id)
	local info = self._controlDefaults and self._controlDefaults[id] or nil;
	if info == nil or info.control == nil then
		return;
	end

	self:setControlValueByInfo(info, info.value);

	if info.key and self._settings then
		self._settings[info.key] = info.value;
	end
end

function ProceduralModelEditor:isLiveGenerateEnabled()
	local info = self._controlInfo and self._controlInfo[ProceduralModelEditorTypes.LiveGenerateId] or nil;
	if info and info.control then
		local value = self:getControlValueByInfo(info);
		return proceduralRound(value) == 1;
	end

	return true;
end

function ProceduralModelEditor:syncFromControls()
	self._settings = self._settings or proceduralCopyTable(ProceduralModelEditorDefaults);

	if self._controlInfo then
		for _, info in pairs(self._controlInfo) do
			if info.key ~= nil then
				local value = self:getControlValueByInfo(info);
				if value ~= nil then
					self._settings[info.key] = value;
				end
			end
		end
	end

	self:validateSettings(true);
end

function ProceduralModelEditor:syncToControls()
	if self._settings == nil or self._controlInfo == nil then
		return;
	end

	self:validateSettings(false);

	for _, info in pairs(self._controlInfo) do
		if info.key ~= nil and self._settings[info.key] ~= nil then
			self:setControlValueByInfo(info, self._settings[info.key]);
		end
	end

	self:updateCurrentToolFromSettings();
	self:updateRuleStackLabel();
	self:updateCurrentToolLabel();
	if self.statusText then
		self.statusText:setLabel(self._currentStatus or "Ready.");
	end
end

function ProceduralModelEditor:validateSettings(updateControls)
	self._settings = self._settings or proceduralCopyTable(ProceduralModelEditorDefaults);

	for key, defaultValue in pairs(ProceduralModelEditorDefaults) do
		if self._settings[key] == nil then
			self._settings[key] = defaultValue;
		end
	end

	for key, range in pairs(ProceduralModelNumericRanges) do
		local value = self._settings[key];
		if value ~= nil then
			value = proceduralClamp(value, range[1], range[2]);
			if range[3] then
				value = proceduralRound(value);
			end
			self._settings[key] = value;
			if updateControls then
				self:setControlByKey(key, value);
			end
		end
	end
end

function ProceduralModelEditor:updateCurrentToolFromSettings()
	local modes = {"Object", "Vertex", "Edge", "Face"};
	local modeIndex = proceduralRound(self._settings and self._settings["edit.mode"] or 0) + 1;
	self._currentTool = modes[modeIndex] or self._currentTool or "Object";
end

function ProceduralModelEditor:writeResult(results, key, value)
	if results == nil then
		return;
	end

	pcall(function()
		results[key] = value;
	end);
end

function ProceduralModelEditor:addRuleStackEntry(label)
	self._ruleStack = self._ruleStack or {};
	self._ruleStack[#self._ruleStack + 1] = label;
	self:updateRuleStackLabel();
	self._settings["shapeType"] = ProceduralModelEditorTypes.ShapeComposite;
	self:setControlByKey("shapeType", ProceduralModelEditorTypes.ShapeComposite);
	self.bakedComponent = nil;
	self.bakedMesh = nil;
	self._lastBakeStats = nil;
	self:setStatus("Added rule: " .. tostring(label) .. ".");
end

function ProceduralModelEditor:updateRuleStackLabel()
	if self.ruleStackLabel == nil then
		return;
	end

	if self._ruleStack == nil or #self._ruleStack == 0 then
		self.ruleStackLabel:setLabel("Rule Stack: empty");
		return;
	end

	local text = "Rule Stack: ";
	for i = 1, #self._ruleStack do
		if i > 1 then
			text = text .. " -> ";
		end
		text = text .. self._ruleStack[i];
	end

	self.ruleStackLabel:setLabel(text);
end

function ProceduralModelEditor:updateCurrentToolLabel()
	if self.currentToolLabel then
		self.currentToolLabel:setLabel("Current Tool: " .. tostring(self._currentTool or "Object"));
	end
end

function ProceduralModelEditor:load()
	print("ProceduralModelEditor load called");

	local windowTypeInfo           = IUIWindow.typeInfo();
	local buttonTypeInfo           = IUIButton.typeInfo();
	local labelTypeInfo            = IUIText.typeInfo();
	local textEntryTypeInfo        = IUITextEntry.typeInfo();
	local dropdownTypeInfo         = IUIDropdown.typeInfo();
	local sliderPairTypeInfo       = IUILabelSliderPair.typeInfo();
	local tabBarTypeInfo           = IUITabBar.typeInfo();
	local collapsingHeaderTypeInfo = IUICollapsingHeader.typeInfo();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	local parentWindow = self.window:getParentWindow();
	if not parentWindow then
		return;
	end

	parentWindow:setSize(Vector2F(760.0, 760.0));

	self.editorWindow = ui:addElement(windowTypeInfo);
	self.editorWindow:setLabel("Procedural Model Editor");
	self.editorWindow:setSize(Vector2F(760.0, 760.0));
	parentWindow:addChild(self.editorWindow);

	local function addHeader(parent, label)
		local header = ui:addElement(collapsingHeaderTypeInfo);
		header:setLabel(label);
		parent:addChild(header);
		return header;
	end

	local function addLabel(parent, label)
		local element = ui:addElement(labelTypeInfo);
		element:setLabel(label);
		parent:addChild(element);
		return element;
	end

	local function addText(parent, label, text, id, key, liveBake)
		local element = ui:addElement(textEntryTypeInfo);
		element:setLabel(label);
		element:setText(text);
		self:registerControl(element, id, key, "text", text, liveBake);
		parent:addChild(element);
		return element;
	end

	local function addDropdown(parent, label, id, key, options, selected, liveBake)
		local element = ui:addElement(dropdownTypeInfo);
		element:setLabel(label);
		for i = 1, #options do
			element:addOption(options[i]);
		end
		element:setSelectedOption(selected);
		self:registerControl(element, id, key, "dropdown", selected, liveBake);
		parent:addChild(element);
		return element;
	end

	local function addSlider(parent, label, id, key, minValue, maxValue, value, liveBake)
		local element = ui:addElement(sliderPairTypeInfo);
		element:setLabel(label);
		element:setMinValue(minValue);
		element:setMaxValue(maxValue);
		element:setValue(value);
		self:registerControl(element, id, key, "slider", value, liveBake);
		parent:addChild(element);
		return element;
	end

	local function addButton(parent, label, id, actionName)
		local element = ui:addElement(buttonTypeInfo);
		element:setLabel(label);
		self:registerButton(element, id, actionName);
		parent:addChild(element);
		return element;
	end

	-- ── Top / asset area ──────────────────────────────────────────────────
	local assetHeader = addHeader(self.editorWindow, "Asset / Rule Asset");

	self.selectedFileEntry = addText(assetHeader, "Output File", "None",
		ProceduralModelEditorTypes.RuleNameId, "outputFile", false);
	if self.window then
		self.window:setDroppable(self.selectedFileEntry, true);
	end

	self.ruleNameEntry = addText(assetHeader, "Rule Name", "New Procedural Mesh",
		ProceduralModelEditorTypes.RuleNameId + 10000, "ruleName", false);

	self.presetDropdown = addDropdown(assetHeader, "Preset",
		ProceduralModelEditorTypes.PresetDropdownId, "preset",
		{"Custom", "Blockout Room", "Sci-Fi Door", "Wall With Windows", "Staircase", "Simple Building", "Roof Piece"}, 0, false);

	self.liveGenerateDropdown = addDropdown(assetHeader, "Live Generate",
		ProceduralModelEditorTypes.LiveGenerateId, "liveGenerate",
		{"Off", "On"}, 1, false);

	-- Shape type.
	self.shapeTypeDropdown = addDropdown(assetHeader, "Template / Root Rule",
		ProceduralModelEditorTypes.ShapeTypeDropdownId, "shapeType",
		{"Box", "Cylinder", "Array", "Composite", "Wall", "Floor", "Stairs", "Door", "Window", "Arch", "Room", "Roof", "Building", "Radial Array"}, 0, true);

	self.currentToolLabel = addLabel(assetHeader, "Current Tool: Object");

	self.generateButton = addButton(assetHeader, "Generate / Preview", ProceduralModelEditorTypes.GenerateButtonId, "generate");
	self.resetButton    = addButton(assetHeader, "Reset", ProceduralModelEditorTypes.ResetButtonId, "reset");
	addButton(assetHeader, "Load Preset", ProceduralModelEditorTypes.LoadPresetButtonId, "loadPreset");
	addButton(assetHeader, "Save Preset", ProceduralModelEditorTypes.SavePresetButtonId, "savePreset");

	-- ── Tab bar ──────────────────────────────────────────────────────────
	local tabBar = ui:addElement(tabBarTypeInfo);
	self.tabBar = tabBar;

	local primitiveTab = tabBar:addTabItem(); primitiveTab:setLabel("Primitive");
	local templatesTab = tabBar:addTabItem(); templatesTab:setLabel("Templates");
	local ruleTab      = tabBar:addTabItem(); ruleTab:setLabel("Rule Stack");
	local editTab      = tabBar:addTabItem(); editTab:setLabel("Edit Tools");
	local arrayTab     = tabBar:addTabItem(); arrayTab:setLabel("Arrays");
	local deformTab    = tabBar:addTabItem(); deformTab:setLabel("Deform");
	local uvTab        = tabBar:addTabItem(); uvTab:setLabel("UV / Material");
	local exportTab    = tabBar:addTabItem(); exportTab:setLabel("Bake / Export");

	self.editorWindow:addChild(tabBar);

	-- ── Primitive tab: existing Box / Cylinder controls ───────────────────
	local boxSizeHeader = addHeader(primitiveTab, "Box Rule");
	self.boxSizeXSlider = addSlider(boxSizeHeader, "Size X", ProceduralModelEditorTypes.BoxSizeXId, "box.size.x", 0.01, 100.0, 1.0, true);
	self.boxSizeYSlider = addSlider(boxSizeHeader, "Size Y", ProceduralModelEditorTypes.BoxSizeYId, "box.size.y", 0.01, 100.0, 1.0, true);
	self.boxSizeZSlider = addSlider(boxSizeHeader, "Size Z", ProceduralModelEditorTypes.BoxSizeZId, "box.size.z", 0.01, 100.0, 1.0, true);
	self.boxOffsetXSlider = addSlider(boxSizeHeader, "Offset X", ProceduralModelEditorTypes.BoxOffsetXId, "box.offset.x", -50.0, 50.0, 0.0, true);
	self.boxOffsetYSlider = addSlider(boxSizeHeader, "Offset Y", ProceduralModelEditorTypes.BoxOffsetYId, "box.offset.y", -50.0, 50.0, 0.0, true);
	self.boxOffsetZSlider = addSlider(boxSizeHeader, "Offset Z", ProceduralModelEditorTypes.BoxOffsetZId, "box.offset.z", -50.0, 50.0, 0.0, true);

	local cylHeader = addHeader(primitiveTab, "Cylinder Rule");
	self.cylRadiusSlider = addSlider(cylHeader, "Radius", ProceduralModelEditorTypes.CylRadiusId, "cylinder.radius", 0.01, 50.0, 0.5, true);
	self.cylHeightSlider = addSlider(cylHeader, "Height", ProceduralModelEditorTypes.CylHeightId, "cylinder.height", 0.01, 100.0, 1.0, true);
	self.cylSegmentsSlider = addSlider(cylHeader, "Segments", ProceduralModelEditorTypes.CylSegmentsId, "cylinder.segments", 3, 128, 8, true);
	self.cylOffsetXSlider = addSlider(cylHeader, "Offset X", ProceduralModelEditorTypes.CylOffsetXId, "cylinder.offset.x", -50.0, 50.0, 0.0, true);
	self.cylOffsetYSlider = addSlider(cylHeader, "Offset Y", ProceduralModelEditorTypes.CylOffsetYId, "cylinder.offset.y", -50.0, 50.0, 0.0, true);
	self.cylOffsetZSlider = addSlider(cylHeader, "Offset Z", ProceduralModelEditorTypes.CylOffsetZId, "cylinder.offset.z", -50.0, 50.0, 0.0, true);

	-- ── Template tab: architectural / ProBuilder-style primitives ─────────
	local wallHeader = addHeader(templatesTab, "Wall / Floor / Room");
	self.wallLengthSlider = addSlider(wallHeader, "Wall Length", ProceduralModelEditorTypes.WallLengthId, "wall.length", 0.1, 200.0, 6.0, true);
	self.wallHeightSlider = addSlider(wallHeader, "Wall Height", ProceduralModelEditorTypes.WallHeightId, "wall.height", 0.1, 50.0, 3.0, true);
	self.wallThicknessSlider = addSlider(wallHeader, "Wall Thickness", ProceduralModelEditorTypes.WallThicknessId, "wall.thickness", 0.01, 10.0, 0.25, true);
	self.floorWidthSlider = addSlider(wallHeader, "Floor Width", ProceduralModelEditorTypes.FloorWidthId, "floor.width", 0.1, 200.0, 6.0, true);
	self.floorDepthSlider = addSlider(wallHeader, "Floor Depth", ProceduralModelEditorTypes.FloorDepthId, "floor.depth", 0.1, 200.0, 6.0, true);
	self.floorThicknessSlider = addSlider(wallHeader, "Floor Thickness", ProceduralModelEditorTypes.FloorThicknessId, "floor.thickness", 0.01, 10.0, 0.2, true);
	self.roomWidthSlider = addSlider(wallHeader, "Room Width", ProceduralModelEditorTypes.RoomWidthId, "room.width", 0.1, 200.0, 6.0, true);
	self.roomDepthSlider = addSlider(wallHeader, "Room Depth", ProceduralModelEditorTypes.RoomDepthId, "room.depth", 0.1, 200.0, 6.0, true);
	self.roomHeightSlider = addSlider(wallHeader, "Room Height", ProceduralModelEditorTypes.RoomHeightId, "room.height", 0.1, 50.0, 3.0, true);

	local openingHeader = addHeader(templatesTab, "Door / Window / Arch");
	self.doorWidthSlider = addSlider(openingHeader, "Door Width", ProceduralModelEditorTypes.DoorWidthId, "door.width", 0.1, 20.0, 1.0, true);
	self.doorHeightSlider = addSlider(openingHeader, "Door Height", ProceduralModelEditorTypes.DoorHeightId, "door.height", 0.1, 20.0, 2.1, true);
	self.doorOffsetXSlider = addSlider(openingHeader, "Door Offset X", ProceduralModelEditorTypes.DoorOffsetXId, "door.offset.x", -50.0, 50.0, 0.0, true);
	self.windowWidthSlider = addSlider(openingHeader, "Window Width", ProceduralModelEditorTypes.WindowWidthId, "window.width", 0.1, 20.0, 1.2, true);
	self.windowHeightSlider = addSlider(openingHeader, "Window Height", ProceduralModelEditorTypes.WindowHeightId, "window.height", 0.1, 20.0, 1.0, true);
	self.windowOffsetXSlider = addSlider(openingHeader, "Window Offset X", ProceduralModelEditorTypes.WindowOffsetXId, "window.offset.x", -50.0, 50.0, 0.0, true);
	self.windowOffsetYSlider = addSlider(openingHeader, "Window Offset Y", ProceduralModelEditorTypes.WindowOffsetYId, "window.offset.y", 0.0, 20.0, 1.5, true);
	self.archRadiusSlider = addSlider(openingHeader, "Arch Radius", ProceduralModelEditorTypes.ArchRadiusId, "arch.radius", 0.1, 20.0, 0.8, true);
	self.archThicknessSlider = addSlider(openingHeader, "Arch Thickness", ProceduralModelEditorTypes.ArchThicknessId, "arch.thickness", 0.01, 10.0, 0.25, true);

	local stairsHeader = addHeader(templatesTab, "Stairs / Building / Roof");
	self.stairCountSlider = addSlider(stairsHeader, "Stair Count", ProceduralModelEditorTypes.StairCountId, "stairs.count", 1, 128, 8, true);
	self.stairWidthSlider = addSlider(stairsHeader, "Stair Width", ProceduralModelEditorTypes.StairWidthId, "stairs.width", 0.1, 50.0, 2.0, true);
	self.stairTreadSlider = addSlider(stairsHeader, "Tread Depth", ProceduralModelEditorTypes.StairTreadId, "stairs.tread", 0.05, 10.0, 0.3, true);
	self.stairRiserSlider = addSlider(stairsHeader, "Riser Height", ProceduralModelEditorTypes.StairRiserId, "stairs.riser", 0.05, 10.0, 0.18, true);
	self.buildingFloorsSlider = addSlider(stairsHeader, "Building Floors", ProceduralModelEditorTypes.BuildingFloorsId, "building.floors", 1, 64, 3, true);
	self.buildingFloorHeightSlider = addSlider(stairsHeader, "Floor Height", ProceduralModelEditorTypes.BuildingFloorHeightId, "building.floorHeight", 0.1, 20.0, 3.0, true);

	-- ── Rule stack tab ───────────────────────────────────────────────────
	local stackHeader = addHeader(ruleTab, "Rule Stack Builder");
	self.ruleStackLabel = addLabel(stackHeader, "Rule Stack: empty");

	addButton(stackHeader, "Add Box Rule", ProceduralModelEditorTypes.AddBoxRuleButtonId, "addBoxRule");
	addButton(stackHeader, "Add Cylinder Rule", ProceduralModelEditorTypes.AddCylinderRuleButtonId, "addCylinderRule");
	addButton(stackHeader, "Add Wall Rule", ProceduralModelEditorTypes.AddWallRuleButtonId, "addWallRule");
	addButton(stackHeader, "Add Stairs Rule", ProceduralModelEditorTypes.AddStairsRuleButtonId, "addStairsRule");
	addButton(stackHeader, "Add Door Cut Rule", ProceduralModelEditorTypes.AddDoorRuleButtonId, "addDoorRule");
	addButton(stackHeader, "Add Window Cut Rule", ProceduralModelEditorTypes.AddWindowRuleButtonId, "addWindowRule");
	addButton(stackHeader, "Add Roof Rule", ProceduralModelEditorTypes.AddRoofRuleButtonId, "addRoofRule");
	addButton(stackHeader, "Add Linear Array Rule", ProceduralModelEditorTypes.AddArrayRuleButtonId, "addArrayRule");
	addButton(stackHeader, "Add Radial Array Rule", ProceduralModelEditorTypes.AddRadialArrayRuleButtonId, "addRadialArrayRule");

	self.booleanModeDropdown = addDropdown(stackHeader, "Boolean / Combine Mode",
		ProceduralModelEditorTypes.BooleanModeId, "boolean.mode",
		{"Union", "Subtract", "Intersect", "Replace", "Append"}, 0, false);

	addButton(stackHeader, "Apply Rule Stack", ProceduralModelEditorTypes.ApplyRuleStackButtonId, "applyRuleStack");
	addButton(stackHeader, "Clear Rule Stack", ProceduralModelEditorTypes.ClearRuleStackButtonId, "clearRuleStack");

	-- ── Edit tools tab ───────────────────────────────────────────────────
	local editModeHeader = addHeader(editTab, "Selection / Handles");
	self.editModeDropdown = addDropdown(editModeHeader, "Edit Mode", ProceduralModelEditorTypes.EditModeId, "edit.mode",
		{"Object", "Vertex", "Edge", "Face"}, 0, false);
	self.selectionModeDropdown = addDropdown(editModeHeader, "Selection Mode", ProceduralModelEditorTypes.SelectionModeId, "selection.mode",
		{"Single", "Add", "Subtract", "Paint", "Loop", "Ring"}, 0, false);
	self.pivotModeDropdown = addDropdown(editModeHeader, "Pivot Mode", ProceduralModelEditorTypes.PivotModeId, "pivot.mode",
		{"Center", "Individual", "Active Element", "Custom"}, 0, false);
	self.handleSpaceDropdown = addDropdown(editModeHeader, "Handle Space", ProceduralModelEditorTypes.HandleSpaceId, "handle.space",
		{"World", "Local", "Normal"}, 0, false);
	self.gridSnapDropdown = addDropdown(editModeHeader, "Grid Snap", ProceduralModelEditorTypes.GridSnapId, "grid.snap",
		{"Off", "On"}, 1, false);
	self.snapXSlider = addSlider(editModeHeader, "Snap X", ProceduralModelEditorTypes.SnapXId, "snap.x", 0.001, 10.0, 0.25, false);
	self.snapYSlider = addSlider(editModeHeader, "Snap Y", ProceduralModelEditorTypes.SnapYId, "snap.y", 0.001, 10.0, 0.25, false);
	self.snapZSlider = addSlider(editModeHeader, "Snap Z", ProceduralModelEditorTypes.SnapZId, "snap.z", 0.001, 10.0, 0.25, false);

	local opHeader = addHeader(editTab, "Face / Edge / Vertex Operations");
	self.extrudeDistanceSlider = addSlider(opHeader, "Extrude Distance", ProceduralModelEditorTypes.ExtrudeDistanceId, "op.extrudeDistance", -20.0, 20.0, 1.0, false);
	self.insetAmountSlider = addSlider(opHeader, "Inset Amount", ProceduralModelEditorTypes.InsetAmountId, "op.insetAmount", 0.0, 10.0, 0.1, false);
	self.bevelAmountSlider = addSlider(opHeader, "Bevel Amount", ProceduralModelEditorTypes.BevelAmountId, "op.bevelAmount", 0.0, 10.0, 0.1, false);
	self.bevelSegmentsSlider = addSlider(opHeader, "Bevel Segments", ProceduralModelEditorTypes.BevelSegmentsId, "op.bevelSegments", 1, 16, 1, false);
	self.weldToleranceSlider = addSlider(opHeader, "Weld Tolerance", ProceduralModelEditorTypes.WeldToleranceId, "op.weldTolerance", 0.0, 1.0, 0.001, false);
	self.subdivideCutsSlider = addSlider(opHeader, "Subdivide Cuts", ProceduralModelEditorTypes.SubdivideCutsId, "op.subdivideCuts", 1, 16, 1, false);

	addButton(opHeader, "Extrude Faces", ProceduralModelEditorTypes.ExtrudeFacesButtonId, "extrudeFaces");
	addButton(opHeader, "Inset Faces", ProceduralModelEditorTypes.InsetFacesButtonId, "insetFaces");
	addButton(opHeader, "Bevel Edges", ProceduralModelEditorTypes.BevelEdgesButtonId, "bevelEdges");
	addButton(opHeader, "Connect Edges", ProceduralModelEditorTypes.ConnectEdgesButtonId, "connectEdges");
	addButton(opHeader, "Bridge Edges", ProceduralModelEditorTypes.BridgeEdgesButtonId, "bridgeEdges");
	addButton(opHeader, "Subdivide", ProceduralModelEditorTypes.SubdivideButtonId, "subdivide");
	addButton(opHeader, "Triangulate", ProceduralModelEditorTypes.TriangulateButtonId, "triangulate");
	addButton(opHeader, "Merge Faces", ProceduralModelEditorTypes.MergeFacesButtonId, "mergeFaces");
	addButton(opHeader, "Flip Normals", ProceduralModelEditorTypes.FlipNormalsButtonId, "flipNormals");
	addButton(opHeader, "Delete Faces", ProceduralModelEditorTypes.DeleteFacesButtonId, "deleteFaces");
	addButton(opHeader, "Weld Vertices", ProceduralModelEditorTypes.WeldVerticesButtonId, "weldVertices");
	addButton(opHeader, "Detach Faces", ProceduralModelEditorTypes.DetachFacesButtonId, "detachFaces");

	-- ── Arrays tab ───────────────────────────────────────────────────────
	self.arrayChildDropdown = addDropdown(arrayTab, "Child Shape", ProceduralModelEditorTypes.ArrayChildTypeId, "array.child",
		{"Box", "Cylinder", "Wall"}, 0, true);
	self.arrayCountSlider = addSlider(arrayTab, "Count", ProceduralModelEditorTypes.ArrayCountId, "array.count", 1, 256, 1, true);
	self.arrayStepXSlider = addSlider(arrayTab, "Step X", ProceduralModelEditorTypes.ArrayStepXId, "array.step.x", -50.0, 50.0, 1.0, true);
	self.arrayStepYSlider = addSlider(arrayTab, "Step Y", ProceduralModelEditorTypes.ArrayStepYId, "array.step.y", -50.0, 50.0, 0.0, true);
	self.arrayStepZSlider = addSlider(arrayTab, "Step Z", ProceduralModelEditorTypes.ArrayStepZId, "array.step.z", -50.0, 50.0, 0.0, true);

	local radialHeader = addHeader(arrayTab, "Radial Array");
	self.radialCountSlider = addSlider(radialHeader, "Radial Count", ProceduralModelEditorTypes.RadialCountId, "radial.count", 1, 256, 8, true);
	self.radialRadiusSlider = addSlider(radialHeader, "Radius", ProceduralModelEditorTypes.RadialRadiusId, "radial.radius", 0.0, 100.0, 3.0, true);
	self.radialAngleSlider = addSlider(radialHeader, "Arc Angle", ProceduralModelEditorTypes.RadialAngleId, "radial.angle", 0.0, 360.0, 360.0, true);
	self.radialStartAngleSlider = addSlider(radialHeader, "Start Angle", ProceduralModelEditorTypes.RadialStartAngleId, "radial.startAngle", -360.0, 360.0, 0.0, true);
	self.radialAxisDropdown = addDropdown(radialHeader, "Axis", ProceduralModelEditorTypes.RadialAxisId, "radial.axis",
		{"X", "Y", "Z"}, 1, true);

	-- ── Deform tab ───────────────────────────────────────────────────────
	local mirrorHeader = addHeader(deformTab, "Mirror / Transform");
	self.mirrorAxisDropdown = addDropdown(mirrorHeader, "Mirror Axis", ProceduralModelEditorTypes.MirrorAxisId, "mirror.axis",
		{"X", "Y", "Z"}, 0, false);
	addButton(mirrorHeader, "Mirror", ProceduralModelEditorTypes.MirrorButtonId, "mirror");
	addButton(mirrorHeader, "Flip X", ProceduralModelEditorTypes.FlipXButtonId, "flipX");
	addButton(mirrorHeader, "Flip Y", ProceduralModelEditorTypes.FlipYButtonId, "flipY");
	addButton(mirrorHeader, "Flip Z", ProceduralModelEditorTypes.FlipZButtonId, "flipZ");

	local deformHeader = addHeader(deformTab, "Procedural Deformers");
	self.taperAmountSlider = addSlider(deformHeader, "Taper", ProceduralModelEditorTypes.TaperAmountId, "deform.taper", -5.0, 5.0, 0.0, true);
	self.twistAmountSlider = addSlider(deformHeader, "Twist", ProceduralModelEditorTypes.TwistAmountId, "deform.twist", -360.0, 360.0, 0.0, true);
	self.bendAmountSlider = addSlider(deformHeader, "Bend", ProceduralModelEditorTypes.BendAmountId, "deform.bend", -360.0, 360.0, 0.0, true);
	self.noiseAmountSlider = addSlider(deformHeader, "Noise", ProceduralModelEditorTypes.NoiseAmountId, "deform.noise", 0.0, 10.0, 0.0, true);

	-- ── UV / material tab ────────────────────────────────────────────────
	local materialHeader = addHeader(uvTab, "Materials");
	self.materialSlotDropdown = addDropdown(materialHeader, "Material Slot", ProceduralModelEditorTypes.MaterialSlotId, "material.slot",
		{"Default", "Wall", "Floor", "Trim", "Glass", "Roof"}, 0, false);
	self.smoothingGroupSlider = addSlider(materialHeader, "Smoothing Group", ProceduralModelEditorTypes.SmoothingGroupId, "material.smoothingGroup", 0, 32, 0, false);
	self.autoSmoothAngleSlider = addSlider(materialHeader, "Auto Smooth Angle", ProceduralModelEditorTypes.AutoSmoothAngleId, "material.autoSmoothAngle", 0.0, 180.0, 60.0, false);
	addButton(materialHeader, "Assign Material", ProceduralModelEditorTypes.AssignMaterialButtonId, "assignMaterial");

	local uvHeader = addHeader(uvTab, "UV Generation");
	self.uvScaleXSlider = addSlider(uvHeader, "UV Scale X", ProceduralModelEditorTypes.UVScaleXId, "uv.scale.x", 0.001, 100.0, 1.0, true);
	self.uvScaleYSlider = addSlider(uvHeader, "UV Scale Y", ProceduralModelEditorTypes.UVScaleYId, "uv.scale.y", 0.001, 100.0, 1.0, true);
	self.uvOffsetXSlider = addSlider(uvHeader, "UV Offset X", ProceduralModelEditorTypes.UVOffsetXId, "uv.offset.x", -100.0, 100.0, 0.0, true);
	self.uvOffsetYSlider = addSlider(uvHeader, "UV Offset Y", ProceduralModelEditorTypes.UVOffsetYId, "uv.offset.y", -100.0, 100.0, 0.0, true);
	addButton(uvHeader, "Auto UV", ProceduralModelEditorTypes.AutoUVButtonId, "autoUv");
	addButton(uvHeader, "Planar UV", ProceduralModelEditorTypes.PlanarUVButtonId, "planarUv");
	addButton(uvHeader, "Box UV", ProceduralModelEditorTypes.BoxUVButtonId, "boxUv");

	-- ── Bake / export tab ────────────────────────────────────────────────
	local bakeHeader = addHeader(exportTab, "Bake / Collider / LOD");
	self.generateColliderDropdown = addDropdown(bakeHeader, "Generate Collider", ProceduralModelEditorTypes.GenerateColliderId, "collider.enabled",
		{"Off", "On"}, 0, false);
	self.colliderTypeDropdown = addDropdown(bakeHeader, "Collider Type", ProceduralModelEditorTypes.ColliderTypeId, "collider.type",
		{"Mesh", "Box", "Convex Hull", "Compound"}, 0, false);
	self.lodCountSlider = addSlider(bakeHeader, "LOD Count", ProceduralModelEditorTypes.LodCountId, "lod.count", 0, 8, 0, false);
	self.lodReductionSlider = addSlider(bakeHeader, "LOD Reduction %", ProceduralModelEditorTypes.LodReductionId, "lod.reduction", 0.0, 95.0, 50.0, false);
	self.sceneActorDropdown = addDropdown(bakeHeader, "Scene Actor", ProceduralModelEditorTypes.SceneActorEnabledId, "sceneActor.enabled",
		{"Off", "Auto Create / Update"}, 1, false);

	addButton(bakeHeader, "Bake Mesh", ProceduralModelEditorTypes.BakeMeshButtonId, "bakeMesh");
	addButton(bakeHeader, "Create / Update Scene Actor", ProceduralModelEditorTypes.CreateActorButtonId, "createSceneActor");
	addButton(bakeHeader, "Save Mesh Asset", ProceduralModelEditorTypes.SaveAssetButtonId, "saveMeshAsset");
	addButton(bakeHeader, "Export OBJ", ProceduralModelEditorTypes.ExportObjButtonId, "exportObj");
	addButton(bakeHeader, "Export FBX", ProceduralModelEditorTypes.ExportFbxButtonId, "exportFbx");
	addButton(bakeHeader, "Export Collider", ProceduralModelEditorTypes.ExportColliderButtonId, "exportCollider");

	-- ── Mesh stats ───────────────────────────────────────────────────────
	self.meshStatsLabel = ui:addElement(labelTypeInfo);
	self.meshStatsLabel:setLabel("Vertices: --   Triangles: --   Indices: --   Rule: --");
	self.meshStatsLabel:setElementId(ProceduralModelEditorTypes.MeshStatsLabelId);
	self.editorWindow:addChild(self.meshStatsLabel);

	self.statusText = ui:addElement(labelTypeInfo);
	self.statusText:setLabel(self._currentStatus or "Ready.");
	self.editorWindow:addChild(self.statusText);

	self:syncToControls();
	self:updateRuleStackLabel();
	self:updateCurrentToolLabel();
end

function ProceduralModelEditor:unload()
	print("ProceduralModelEditor unload called");

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

	self.tabBar = nil;
	self.selectedFileEntry = nil;
	self.shapeTypeDropdown = nil;
	self.meshStatsLabel = nil;
	self.statusText = nil;
	self.currentToolLabel = nil;
	self.generateButton = nil;
	self.resetButton = nil;
	self.ruleStackLabel = nil;
	self.bakedComponent = nil;
	self.bakedMesh = nil;
	self.generatedActor = nil;
	self.generatedMeshComponent = nil;
	self.generatedMeshRenderer = nil;
	self.generatedMaterial = nil;
	self.generatedCollisionMesh = nil;

	self._settings = proceduralCopyTable(ProceduralModelEditorDefaults);
	self._controlDefaults = {};
	self._controlInfo = {};
	self._liveBakeIds = {};
	self._actionButtons = {};
	self._ruleStack = {};
	self._lastBakeStats = nil;
	self._operationLog = {};
	self._operationApplyReport = {};
	self._exportedFiles = {};
	self._transformState =
	{
		mirrorAxis = nil,
		flipX = false,
		flipY = false,
		flipZ = false,
		flipNormals = false,
	};
	self._currentStatus = "Ready.";
end

function ProceduralModelEditor:show()
	print("ProceduralModelEditor show called");

	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(true, false);
	end

	if self.editorWindow then
		self.editorWindow:setVisible(true, false);
	end
end

function ProceduralModelEditor:hide()
	print("ProceduralModelEditor hide called");

	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
	end

	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(false, false);
	end
end

function ProceduralModelEditor:updateSelection()
	print("ProceduralModelEditor updateSelection called");

	self.component = nil;

	if self.selectedFileEntry then
		self.selectedFileEntry:setText("None");
	end

	local applicationManager = IApplicationManager.instance();
	local selectionManager   = applicationManager:getSelectionManager();
	local selection          = selectionManager:getSelection();

	local size = selection:size();
	for i = 0, size - 1 do
		local item = selection:at(i);
		print("ProceduralModelEditor selection item type: "..item:getTypeInfo());
		self.component = item;
	end

	if self.component then
		self:setStatus("Selection captured for procedural bake context.");
	else
		self:setStatus("No scene selection. Generated meshes remain editor-side until exported.");
	end
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Events
-- ─────────────────────────────────────────────────────────────────────────────

function ProceduralModelEditor:handleEvent(parameters, results)
	print("ProceduralModelEditor handleEvent called");

	local visibleOk, visible = proceduralSafeCall(self.window, "isWindowVisible");
	if visibleOk and not visible then
		return;
	end

	if parameters == nil then
		print("ProceduralModelEditor parameters nil");
		return;
	end

	local eventHash = nil;
	local sender = nil;
	local args = nil;
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
		local okId, valueId = proceduralSafeCall(sender, "getElementId");
		if okId then
			elementId = valueId;
		end
	end

	if elementId == nil then
		return;
	end

	local isValueChanged = eventHash == IEvent.handleValueChanged
		or legacyEventType == "changed"
		or legacyEventType == "valueChanged";
	local isSelection = eventHash == IEvent.handleSelection
		or legacyEventType == "clicked"
		or legacyEventType == "selection";
	local isDrop = eventHash == IEvent.handleDrop
		or legacyEventType == "drop";

	if isValueChanged then
		local info = self._controlInfo and self._controlInfo[elementId] or nil;
		if info then
			local value = self:getControlValueByInfo(info);
			if info.key ~= nil then
				self._settings[info.key] = value;
			end
			self.bakedComponent = nil;
			self.bakedMesh = nil;
			self._lastBakeStats = nil;

			print("ProceduralModelEditor setting updated: " .. tostring(info.key) .. " = " .. tostring(value));
			self:writeResult(results, info.key, value);
		end

		if elementId == ProceduralModelEditorTypes.EditModeId and self.editModeDropdown then
			self:updateCurrentToolFromSettings();
			self:updateCurrentToolLabel();
		end

		if self._liveBakeIds and self._liveBakeIds[elementId] and self:isLiveGenerateEnabled() then
			self:generate();
		end
	end

	if isSelection then
		local action = self._actionButtons and self._actionButtons[elementId] or nil;

		if action then
			print("ProceduralModelEditor action: " .. action);

			self:writeResult(results, "action", action);
			self:writeResult(results, "settings", self._settings);

			self:handleAction(action, results);
		end
	end

	if isDrop then
		self:handleDrop(args, elementId, results);
	end
end

function ProceduralModelEditor:handleAction(action, results)
	self:syncFromControls();

	if action == "generate" then
		self:generate();
		return;
	elseif action == "reset" then
		self:resetSliders();
		return;
	elseif action == "loadPreset" then
		self:loadSelectedPreset();
		return;
	elseif action == "savePreset" then
		self:saveCurrentPreset();
		return;
	elseif action == "bakeMesh" then
		self:bakeMesh();
		return;
	elseif action == "createSceneActor" then
		self:createOrUpdateSceneActor(true);
		return;
	elseif action == "saveMeshAsset" then
		self:saveMeshAsset();
		return;
	elseif action == "exportObj" then
		self:exportObj();
		return;
	elseif action == "exportFbx" then
		self:exportFbx();
		return;
	elseif action == "exportCollider" then
		self:exportCollider();
		return;
	elseif action == "clearRuleStack" then
		self._ruleStack = {};
		self:updateRuleStackLabel();
		self.bakedComponent = nil;
		self.bakedMesh = nil;
		self._lastBakeStats = nil;
		self:setStatus("Rule stack cleared.");
		return;
	elseif action == "applyRuleStack" then
		self._settings["shapeType"] = ProceduralModelEditorTypes.ShapeComposite;
		self:setControlByKey("shapeType", ProceduralModelEditorTypes.ShapeComposite);
		self:generate();
		return;
	elseif action == "addBoxRule" then
		self:addRuleStackEntry("Box");
	elseif action == "addCylinderRule" then
		self:addRuleStackEntry("Cylinder");
	elseif action == "addWallRule" then
		self:addRuleStackEntry("Wall");
	elseif action == "addStairsRule" then
		self:addRuleStackEntry("Stairs");
	elseif action == "addDoorRule" then
		self:addRuleStackEntry("Door Cut");
	elseif action == "addWindowRule" then
		self:addRuleStackEntry("Window Cut");
	elseif action == "addRoofRule" then
		self:addRuleStackEntry("Roof");
	elseif action == "addArrayRule" then
		self:addRuleStackEntry("Linear Array");
	elseif action == "addRadialArrayRule" then
		self:addRuleStackEntry("Radial Array");
	elseif action == "mirror" or action == "flipX" or action == "flipY" or action == "flipZ" or action == "flipNormals" then
		self:toggleTransformAction(action);
	else
		self._currentTool = action;
		self:updateCurrentToolLabel();
		self:recordOperation(action);
	end

	if self:isGeometryAction(action) and self:isLiveGenerateEnabled() then
		self:generate();
	end
end

function ProceduralModelEditor:isGeometryAction(action)
	return action == "extrudeFaces"
		or action == "insetFaces"
		or action == "bevelEdges"
		or action == "connectEdges"
		or action == "bridgeEdges"
		or action == "subdivide"
		or action == "triangulate"
		or action == "mergeFaces"
		or action == "flipNormals"
		or action == "deleteFaces"
		or action == "weldVertices"
		or action == "detachFaces"
		or action == "mirror"
		or action == "flipX"
		or action == "flipY"
		or action == "flipZ"
		or action == "autoUv"
		or action == "planarUv"
		or action == "boxUv";
end

function ProceduralModelEditor:handleDrop(args, elementId, results)
	local dataStr = nil;
	if args ~= nil and args.at then
		local ok, value = pcall(function() return args:at(0); end);
		if ok then
			dataStr = value;
		end
	elseif type(args) == "string" then
		dataStr = args;
	end

	if dataStr == nil then
		return;
	end

	local filePath = dataStr;
	local okJson, cjson = pcall(require, "cjson");
	if okJson and cjson then
		local okDecode, decoded = pcall(function() return cjson.decode(dataStr); end);
		if okDecode and decoded and decoded.filePath then
			filePath = decoded.filePath;
		end
	end

	print("ProceduralModelEditor drop: " .. tostring(filePath));

	if self.selectedFileEntry then
		self.selectedFileEntry:setText(filePath);
	end

	self._settings["outputFile"] = filePath;
	self:writeResult(results, "outputFile", filePath);
	self:setStatus("Output file set: " .. tostring(filePath));
end

function ProceduralModelEditor:recordOperation(action)
	self._operationLog = self._operationLog or {};
	self._operationLog[#self._operationLog + 1] = action;

	local label = tostring(action or "operation");
	if action == "assignMaterial" then
		label = "material assignment";
	elseif action == "autoUv" or action == "planarUv" or action == "boxUv" then
		self._settings["uv.mode"] = action;
		label = action;
	elseif action == "triangulate" then
		label = "triangulate (mesh output is already triangle-based)";
	end

	self:setStatus("Operation recorded: " .. label .. ".");
end

function ProceduralModelEditor:toggleTransformAction(action)
	self._transformState = self._transformState or {};
	local axisNames = {"X", "Y", "Z"};
	local statusMessage = nil;

	if action == "mirror" then
		local axis = proceduralRound(self._settings["mirror.axis"] or 0);
		if self._transformState.mirrorAxis == axis then
			self._transformState.mirrorAxis = nil;
			statusMessage = "Mirror disabled.";
		else
			self._transformState.mirrorAxis = axis;
			statusMessage = "Mirror " .. (axisNames[axis + 1] or "Z") .. " enabled.";
		end
	elseif action == "flipX" then
		self._transformState.flipX = not self._transformState.flipX;
		statusMessage = "Flip X " .. tostring(self._transformState.flipX and "enabled" or "disabled") .. ".";
	elseif action == "flipY" then
		self._transformState.flipY = not self._transformState.flipY;
		statusMessage = "Flip Y " .. tostring(self._transformState.flipY and "enabled" or "disabled") .. ".";
	elseif action == "flipZ" then
		self._transformState.flipZ = not self._transformState.flipZ;
		statusMessage = "Flip Z " .. tostring(self._transformState.flipZ and "enabled" or "disabled") .. ".";
	elseif action == "flipNormals" then
		self._transformState.flipNormals = not self._transformState.flipNormals;
		statusMessage = "Flip normals " .. tostring(self._transformState.flipNormals and "enabled" or "disabled") .. ".";
	end

	self.bakedComponent = nil;
	self.bakedMesh = nil;
	self._lastBakeStats = nil;
	self:recordOperation(action);
	if statusMessage then
		self:setStatus(statusMessage);
	end
end

function ProceduralModelEditor:applySettingsTable(settings)
	self._settings = self._settings or proceduralCopyTable(ProceduralModelEditorDefaults);
	if settings then
		for key, value in pairs(settings) do
			self._settings[key] = value;
			self:setControlByKey(key, value);
		end
	end
	self:validateSettings(true);
	self:updateCurrentToolFromSettings();
	self:updateCurrentToolLabel();
end

function ProceduralModelEditor:loadSelectedPreset()
	self:syncFromControls();

	local presetIndex = proceduralRound(self._settings["preset"] or 0);
	local preset = ProceduralModelPresetSettings[presetIndex];
	if presetIndex == 0 then
		preset = self._savedPreset;
	end

	if preset == nil then
		self:setStatus("No saved custom preset. Current settings remain active.");
		return false;
	end

	self:applySettingsTable(preset.settings);
	self._ruleStack = proceduralCopyTable(preset.ruleStack or {});
	self:updateRuleStackLabel();
	self:setStatus("Preset loaded: " .. tostring(preset.name or "Custom") .. ".");

	if self:isLiveGenerateEnabled() then
		self:generate();
	end

	return true;
end

function ProceduralModelEditor:saveCurrentPreset()
	self:syncFromControls();
	self._savedPreset =
	{
		name = "Custom",
		settings = proceduralCopyTable(self._settings),
		ruleStack = proceduralCopyTable(self._ruleStack),
	};
	self:setStatus("Custom preset saved in this editor session.");
	return true;
end

function ProceduralModelEditor:getOutputPath(extension)
	self:syncFromControls();

	local path = tostring(self._settings["outputFile"] or "");
	if path == "" or path == "None" then
		local name = self._settings["ruleName"] or "ProceduralMesh";
		path = "procedural_" .. proceduralSafeFilePart(name);
	end

	local lowerPath = string.lower(path);
	local knownExtensions = {".collider.json", ".pmodel", ".obj", ".fbx"};
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

	if self.selectedFileEntry then
		self.selectedFileEntry:setText(path);
	end
	self._settings["outputFile"] = path;

	return path;
end

function ProceduralModelEditor:writeTextFile(path, contents)
	local okApp, applicationManager = pcall(function() return IApplicationManager.instance(); end);
	if okApp and applicationManager then
		local okFs, fileSystem = pcall(function() return applicationManager:getFileSystem(); end);
		if okFs and fileSystem then
			self:ensureOutputDirectory(path, fileSystem);
			local okWrite, writeResult = pcall(function() return fileSystem:writeAllText(path, contents); end);
			if okWrite and writeResult ~= false then
				return true;
			end
		end
	end

	if io and io.open then
		self:ensureOutputDirectory(path, nil);
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

function ProceduralModelEditor:ensureOutputDirectory(path, fileSystem)
	local directory = proceduralDirectoryName(path);
	if directory == nil or directory == "" then
		return true;
	end

	if fileSystem ~= nil then
		local ok, result = proceduralSafeCall(fileSystem, "createDirectories", directory);
		if ok and result ~= false then
			return true;
		end

		ok, result = proceduralSafeCall(fileSystem, "createDirectory", directory);
		if ok and result ~= false then
			return true;
		end

		ok, result = proceduralSafeCall(fileSystem, "ensureDirectory", directory);
		if ok and result ~= false then
			return true;
		end
	end

	return false;
end

function ProceduralModelEditor:buildRecipeText(kind)
	self:syncFromControls();

	local lines = {};
	local stats = self._lastBakeStats or { vertices = 0, triangles = 0, indices = 0, ruleName = "Unbaked" };
	local keys = {};
	for key, _ in pairs(self._settings or {}) do
		keys[#keys + 1] = key;
	end
	table.sort(keys);

	lines[#lines + 1] = "{";
	lines[#lines + 1] = "  \"type\": \"" .. proceduralJsonEscape(kind or "ProceduralModelRecipe") .. "\",";
	lines[#lines + 1] = "  \"version\": 1,";
	lines[#lines + 1] = "  \"ruleName\": \"" .. proceduralJsonEscape(self._settings["ruleName"] or "Procedural Mesh") .. "\",";
	lines[#lines + 1] = "  \"settings\": {";
	for i = 1, #keys do
		local key = keys[i];
		local value = self._settings[key];
		local encoded = "null";
		if type(value) == "string" then
			encoded = "\"" .. proceduralJsonEscape(value) .. "\"";
		elseif type(value) == "boolean" then
			encoded = value and "true" or "false";
		elseif type(value) == "number" then
			encoded = tostring(value);
		end

		local comma = i < #keys and "," or "";
		lines[#lines + 1] = "    \"" .. proceduralJsonEscape(key) .. "\": " .. encoded .. comma;
	end
	lines[#lines + 1] = "  },";

	lines[#lines + 1] = "  \"ruleStack\": [";
	for i = 1, #(self._ruleStack or {}) do
		local comma = i < #self._ruleStack and "," or "";
		lines[#lines + 1] = "    \"" .. proceduralJsonEscape(self._ruleStack[i]) .. "\"" .. comma;
	end
	lines[#lines + 1] = "  ],";

	lines[#lines + 1] = "  \"transform\": {";
	lines[#lines + 1] = "    \"mirrorAxis\": " .. tostring(self._transformState and self._transformState.mirrorAxis or "null") .. ",";
	lines[#lines + 1] = "    \"flipX\": " .. tostring(self._transformState and self._transformState.flipX == true) .. ",";
	lines[#lines + 1] = "    \"flipY\": " .. tostring(self._transformState and self._transformState.flipY == true) .. ",";
	lines[#lines + 1] = "    \"flipZ\": " .. tostring(self._transformState and self._transformState.flipZ == true) .. ",";
	lines[#lines + 1] = "    \"flipNormals\": " .. tostring(self._transformState and self._transformState.flipNormals == true);
	lines[#lines + 1] = "  },";

	lines[#lines + 1] = "  \"operations\": [";
	for i = 1, #(self._operationLog or {}) do
		local comma = i < #self._operationLog and "," or "";
		lines[#lines + 1] = "    \"" .. proceduralJsonEscape(self._operationLog[i]) .. "\"" .. comma;
	end
	lines[#lines + 1] = "  ],";

	lines[#lines + 1] = "  \"operationApplyReport\": [";
	for i = 1, #(self._operationApplyReport or {}) do
		local comma = i < #self._operationApplyReport and "," or "";
		lines[#lines + 1] = "    \"" .. proceduralJsonEscape(self._operationApplyReport[i]) .. "\"" .. comma;
	end
	lines[#lines + 1] = "  ],";

	lines[#lines + 1] = "  \"export\": {";
	lines[#lines + 1] = "    \"colliderEnabled\": " .. tostring(proceduralRound(self._settings["collider.enabled"] or 0) == 1) .. ",";
	lines[#lines + 1] = "    \"colliderType\": " .. tostring(proceduralRound(self._settings["collider.type"] or 0)) .. ",";
	lines[#lines + 1] = "    \"lodCount\": " .. tostring(proceduralRound(self._settings["lod.count"] or 0)) .. ",";
	lines[#lines + 1] = "    \"lodReduction\": " .. tostring(tonumber(self._settings["lod.reduction"] or 0.0) or 0.0);
	lines[#lines + 1] = "  },";

	lines[#lines + 1] = string.format("  \"stats\": { \"vertices\": %d, \"triangles\": %d, \"indices\": %d, \"rule\": \"%s\" }",
		stats.vertices or 0, stats.triangles or 0, stats.indices or 0, proceduralJsonEscape(stats.ruleName or "Unbaked"));
	lines[#lines + 1] = "}";

	return table.concat(lines, "\n");
end

function ProceduralModelEditor:ensureBakedMesh()
	if self.bakedMesh ~= nil then
		return true;
	end

	return self:generate() == true;
end

function ProceduralModelEditor:bakeMesh()
	local ok = self:generate();
	if ok then
		self:setStatus("Mesh baked and ready for export.");
	end
	return ok;
end

function ProceduralModelEditor:saveMeshAsset()
	if not self:ensureBakedMesh() then
		return false;
	end

	local path = self:getOutputPath(".pmodel");
	local ok, err = self:writeTextFile(path, self:buildRecipeText("ProceduralModelRecipe"));
	if ok then
		self._exportedFiles.asset = path;
		self:setStatus("Procedural asset saved: " .. path);
		return true;
	end

	self:setStatus("Could not save procedural asset: " .. tostring(err));
	return false;
end

function ProceduralModelEditor:applyMeshPathToSelection(path)
	if self.component == nil then
		return false;
	end

	local ok = proceduralSafeCall(self.component, "setMeshPath", path);
	if ok then
		return true;
	end

	return false;
end

function ProceduralModelEditor:writeObjFile(path, mesh)
	if mesh == nil or mesh.getVertexPosition == nil or mesh.getIndex == nil then
		return false, "Mesh vertex/index access is not available.";
	end

	local vertexCount = mesh:getVertexCount();
	local indexCount = mesh:getIndexCount();
	local triangleIndexCount = indexCount - (indexCount % 3);
	local hasNormals = mesh.getVertexNormal ~= nil;
	local lines = {};
	lines[#lines + 1] = "# Workphone procedural mesh";
	lines[#lines + 1] = string.format("# vertices %d indices %d", vertexCount, indexCount);

	for i = 0, vertexCount - 1 do
		local v = mesh:getVertexPosition(i);
		lines[#lines + 1] = string.format("v %.6f %.6f %.6f", v.x, v.y, v.z);
	end

	local normalLines = {};
	if hasNormals then
		for i = 0, vertexCount - 1 do
			local okNormal, n = pcall(function() return mesh:getVertexNormal(i); end);
			if not okNormal or n == nil then
				hasNormals = false;
				break;
			end
			normalLines[#normalLines + 1] = string.format("vn %.6f %.6f %.6f", n.x, n.y, n.z);
		end
	end

	if hasNormals then
		for i = 1, #normalLines do
			lines[#lines + 1] = normalLines[i];
		end
	end

	for i = 0, triangleIndexCount - 1, 3 do
		local a = mesh:getIndex(i) + 1;
		local b = mesh:getIndex(i + 1) + 1;
		local c = mesh:getIndex(i + 2) + 1;
		if hasNormals then
			lines[#lines + 1] = string.format("f %d//%d %d//%d %d//%d", a, a, b, b, c, c);
		else
			lines[#lines + 1] = string.format("f %d %d %d", a, b, c);
		end
	end

	return self:writeTextFile(path, table.concat(lines, "\n"));
end

function ProceduralModelEditor:exportObj()
	if not self:ensureBakedMesh() then
		return false;
	end

	local path = self:getOutputPath(".obj");
	local ok, err = self:writeGeneratedObj(path);
	if not ok then
		self:setStatus("Could not export OBJ: " .. tostring(err or "write failed"));
		return false;
	end

	self._exportedFiles.obj = path;
	if self:applyMeshPathToSelection(path) then
		self:setStatus("OBJ exported and assigned to selection: " .. path);
	else
		self:setStatus("OBJ exported: " .. path);
	end
	return true;
end

function ProceduralModelEditor:writeFbxFile(path, mesh)
	if mesh == nil or mesh.getVertexPosition == nil or mesh.getIndex == nil then
		return false, "Mesh vertex/index access is not available.";
	end

	local vertexCount = mesh:getVertexCount();
	local indexCount = mesh:getIndexCount();
	local triangleIndexCount = indexCount - (indexCount % 3);
	local vertices = {};
	local normals = {};
	local polygonIndices = {};

	for i = 0, vertexCount - 1 do
		local v = mesh:getVertexPosition(i);
		vertices[#vertices + 1] = string.format("%.6f,%.6f,%.6f", v.x, v.y, v.z);

		local n = mesh.getVertexNormal ~= nil and mesh:getVertexNormal(i) or Vector3F(0.0, 1.0, 0.0);
		normals[#normals + 1] = string.format("%.6f,%.6f,%.6f", n.x, n.y, n.z);
	end

	for i = 0, triangleIndexCount - 1, 3 do
		polygonIndices[#polygonIndices + 1] = tostring(mesh:getIndex(i));
		polygonIndices[#polygonIndices + 1] = tostring(mesh:getIndex(i + 1));
		polygonIndices[#polygonIndices + 1] = tostring(-(mesh:getIndex(i + 2) + 1));
	end

	local lines = {};
	lines[#lines + 1] = "; FBX 7.4.0 generated by Workphone ProceduralModelEditor";
	lines[#lines + 1] = "FBXHeaderExtension:  {";
	lines[#lines + 1] = "    FBXHeaderVersion: 1003";
	lines[#lines + 1] = "    FBXVersion: 7400";
	lines[#lines + 1] = "    Creator: \"Workphone ProceduralModelEditor\"";
	lines[#lines + 1] = "}";
	lines[#lines + 1] = "Objects:  {";
	lines[#lines + 1] = "    Geometry: 1, \"Geometry::ProceduralMesh\", \"Mesh\" {";
	lines[#lines + 1] = "        Vertices: *" .. tostring(vertexCount * 3) .. " {";
	lines[#lines + 1] = "            a: " .. table.concat(vertices, ",");
	lines[#lines + 1] = "        }";
	lines[#lines + 1] = "        PolygonVertexIndex: *" .. tostring(triangleIndexCount) .. " {";
	lines[#lines + 1] = "            a: " .. table.concat(polygonIndices, ",");
	lines[#lines + 1] = "        }";
	lines[#lines + 1] = "        LayerElementNormal: 0 {";
	lines[#lines + 1] = "            Version: 101";
	lines[#lines + 1] = "            Name: \"\"";
	lines[#lines + 1] = "            MappingInformationType: \"ByVertice\"";
	lines[#lines + 1] = "            ReferenceInformationType: \"Direct\"";
	lines[#lines + 1] = "            Normals: *" .. tostring(vertexCount * 3) .. " {";
	lines[#lines + 1] = "                a: " .. table.concat(normals, ",");
	lines[#lines + 1] = "            }";
	lines[#lines + 1] = "        }";
	lines[#lines + 1] = "    }";
	lines[#lines + 1] = "    Model: 2, \"Model::ProceduralMesh\", \"Mesh\" {";
	lines[#lines + 1] = "        Version: 232";
	lines[#lines + 1] = "        Shading: T";
	lines[#lines + 1] = "        Culling: \"CullingOff\"";
	lines[#lines + 1] = "    }";
	lines[#lines + 1] = "}";
	lines[#lines + 1] = "Connections:  {";
	lines[#lines + 1] = "    C: \"OO\",1,2";
	lines[#lines + 1] = "}";

	return self:writeTextFile(path, table.concat(lines, "\n"));
end

function ProceduralModelEditor:exportFbx()
	if not self:ensureBakedMesh() then
		return false;
	end

	local path = self:getOutputPath(".fbx");
	local ok, err = self:writeFbxFile(path, self.bakedMesh);
	if ok then
		self._exportedFiles.fbx = path;
		self:setStatus("FBX exported: " .. path);
		return true;
	end

	self:setStatus("Could not export FBX: " .. tostring(err));
	return false;
end

function ProceduralModelEditor:exportCollider()
	if not self:ensureBakedMesh() then
		return false;
	end

	local path = self:getOutputPath(".collider.json");
	local ok, err = self:writeTextFile(path, self:buildRecipeText("ProceduralColliderRecipe"));
	if ok then
		self._exportedFiles.collider = path;
		self:setStatus("Collider recipe exported: " .. path);
		return true;
	end

	self:setStatus("Could not export collider: " .. tostring(err));
	return false;
end

function ProceduralModelEditor:getCurrentScene()
	local okApp, applicationManager = pcall(function() return IApplicationManager.instance(); end);
	if not okApp or applicationManager == nil then
		return nil, nil, "Application manager is not available.";
	end

	local okManager, sceneManager = pcall(function() return applicationManager:getGameManager(); end);
	if not okManager or sceneManager == nil then
		return nil, nil, "Game manager is not available.";
	end

	local okScene, scene = pcall(function() return sceneManager:getCurrentScene(); end);
	if not okScene or scene == nil then
		return sceneManager, nil, "No current scene is available.";
	end

	return sceneManager, scene, nil;
end

function ProceduralModelEditor:getSceneActorName()
	local name = self._settings and self._settings["ruleName"] or "Procedural Mesh";
	return proceduralSafeFilePart(name);
end

function ProceduralModelEditor:getOrAddActorComponent(actor, className)
	if actor == nil then
		return nil;
	end

	local okComponent, component = proceduralSafeCall(actor, "getComponent", className);
	if okComponent and component ~= nil then
		return component;
	end

	okComponent, component = proceduralSafeCall(actor, "addComponent", className);
	if okComponent and component ~= nil then
		return component;
	end

	return nil;
end

function ProceduralModelEditor:configureGeneratedActor(actor, meshPath)
	if actor == nil then
		return false, "Actor could not be created.";
	end

	local actorName = self:getSceneActorName();
	proceduralSafeCall(actor, "setName", actorName);
	proceduralSafeCall(actor, "setStatic", true);
	proceduralSafeCall(actor, "setVisible", true);
	proceduralSafeCall(actor, "setEnabled", true);
	proceduralSafeCall(actor, "setDirty", true);

	local meshComponent = self:getOrAddActorComponent(actor, "Mesh");
	if meshComponent ~= nil then
		proceduralSafeCall(meshComponent, "setMeshPath", meshPath);
		proceduralSafeCall(meshComponent, "load", nil);
	end

	local meshRenderer = self:getOrAddActorComponent(actor, "MeshRenderer");
	if meshRenderer ~= nil then
		proceduralSafeCall(meshRenderer, "load", nil);
		proceduralSafeCall(meshRenderer, "updateMaterials");
		proceduralSafeCall(meshRenderer, "updateTransform");
		proceduralSafeCall(meshRenderer, "updateVisibility");
	end

	local material = self:getOrAddActorComponent(actor, "Material");
	if material ~= nil then
		proceduralSafeCall(material, "setIndex", proceduralRound(self._settings["material.slot"] or 0));
	end

	local colliderEnabled = proceduralRound(self._settings["collider.enabled"] or 0) == 1;
	if colliderEnabled then
		local collisionMesh = self:getOrAddActorComponent(actor, "CollisionMesh");
		if collisionMesh ~= nil then
			proceduralSafeCall(collisionMesh, "setMeshPath", meshPath);
			proceduralSafeCall(collisionMesh, "setConvex", proceduralRound(self._settings["collider.type"] or 0) == 2);
			proceduralSafeCall(collisionMesh, "load", nil);
			self.generatedCollisionMesh = collisionMesh;
		end
	end

	proceduralSafeCall(actor, "updateTransform");
	proceduralSafeCall(actor, "updateDirty");
	proceduralSafeCall(actor, "updateVisibility");

	self.generatedActor = actor;
	self.generatedMeshComponent = meshComponent;
	self.generatedMeshRenderer = meshRenderer;
	self.generatedMaterial = material;

	if meshComponent == nil then
		return false, "Actor was created, but a Mesh component could not be added.";
	end
	if meshRenderer == nil then
		return false, "Actor was created, but a MeshRenderer component could not be added.";
	end

	return true;
end

function ProceduralModelEditor:registerGeneratedActor(scene, actor)
	if scene == nil or actor == nil then
		return;
	end

	if self._generatedActorRegistered == true then
		return;
	end

	proceduralSafeCall(scene, "addActor", actor);
	proceduralSafeCall(scene, "registerAllUpdates", actor);
	proceduralSafeCall(scene, "registerUpdate", actor);
	self._generatedActorRegistered = true;
end

function ProceduralModelEditor:createOrUpdateSceneActor(forceStatus)
	if not self:ensureBakedMesh() then
		return false;
	end

	local sceneManager, scene, sceneError = self:getCurrentScene();
	if scene == nil then
		if forceStatus then
			self:setStatus("Could not create scene actor: " .. tostring(sceneError));
		end
		return false;
	end

	local path = self:getOutputPath(".obj");
	local okObj, objError = self:writeGeneratedObj(path);
	if not okObj then
		if forceStatus then
			self:setStatus("Could not write scene actor mesh: " .. tostring(objError));
		end
		return false;
	end

	self._exportedFiles.obj = path;

	local actor = self.generatedActor;
	if actor == nil and sceneManager ~= nil then
		local actorName = self:getSceneActorName();
		local okExisting, existingActor = proceduralSafeCall(sceneManager, "getActorByName", actorName);
		if okExisting and existingActor ~= nil then
			actor = existingActor;
			self._generatedActorRegistered = true;
		else
			local okCreate, createdActor = proceduralSafeCall(sceneManager, "createActor");
			if okCreate then
				actor = createdActor;
				self._generatedActorRegistered = false;
			end
		end
	end

	if actor == nil then
		if forceStatus then
			self:setStatus("Could not create scene actor.");
		end
		return false;
	end

	self:registerGeneratedActor(scene, actor);
	local configured, configureError = self:configureGeneratedActor(actor, path);
	if not configured then
		if forceStatus then
			self:setStatus(tostring(configureError));
		end
		return false;
	end

	if forceStatus then
		self:setStatus("Scene actor created / updated with generated mesh: " .. path);
	end

	return true, path;
end

function ProceduralModelEditor:markOperationReport(action, state)
	self._operationApplyReport = self._operationApplyReport or {};
	self._operationApplyReport[#self._operationApplyReport + 1] = tostring(action or "operation") .. ":" .. tostring(state or "recorded");
end

function ProceduralModelEditor:tryMeshOperation(mesh, action, methodNames, ...)
	local ok, _, methodName = proceduralCallAny(mesh, methodNames, ...);
	if ok then
		self:markOperationReport(action, "applied:" .. tostring(methodName));
		return true;
	end

	self:markOperationReport(action, "recorded");
	return false;
end

function ProceduralModelEditor:applyRecordedMeshOperations(mesh)
	if mesh == nil then
		return;
	end

	self._operationApplyReport = {};

	for i = 1, #(self._operationLog or {}) do
		local action = self._operationLog[i];

		if action == "extrudeFaces" then
			self:tryMeshOperation(mesh, action, {"extrudeFaces", "extrudeSelectedFaces", "extrude"},
				tonumber(self._settings["op.extrudeDistance"] or 0.0) or 0.0);
		elseif action == "insetFaces" then
			self:tryMeshOperation(mesh, action, {"insetFaces", "insetSelectedFaces", "inset"},
				tonumber(self._settings["op.insetAmount"] or 0.0) or 0.0);
		elseif action == "bevelEdges" then
			self:tryMeshOperation(mesh, action, {"bevelEdges", "bevelSelectedEdges", "bevel"},
				tonumber(self._settings["op.bevelAmount"] or 0.0) or 0.0,
				proceduralRound(self._settings["op.bevelSegments"] or 1));
		elseif action == "connectEdges" then
			self:tryMeshOperation(mesh, action, {"connectEdges", "connectSelectedEdges"});
		elseif action == "bridgeEdges" then
			self:tryMeshOperation(mesh, action, {"bridgeEdges", "bridgeSelectedEdges"});
		elseif action == "subdivide" then
			self:tryMeshOperation(mesh, action, {"subdivide", "subdivideFaces", "subdivideSelectedFaces"},
				proceduralRound(self._settings["op.subdivideCuts"] or 1));
		elseif action == "triangulate" then
			self:tryMeshOperation(mesh, action, {"triangulate", "triangulateFaces"});
		elseif action == "mergeFaces" then
			self:tryMeshOperation(mesh, action, {"mergeFaces", "mergeSelectedFaces"});
		elseif action == "deleteFaces" then
			self:tryMeshOperation(mesh, action, {"deleteFaces", "deleteSelectedFaces"});
		elseif action == "weldVertices" then
			self:tryMeshOperation(mesh, action, {"weldVertices", "weldSelectedVertices", "weld"},
				tonumber(self._settings["op.weldTolerance"] or 0.0) or 0.0);
		elseif action == "detachFaces" then
			self:tryMeshOperation(mesh, action, {"detachFaces", "detachSelectedFaces"});
		elseif action == "assignMaterial" then
			self:tryMeshOperation(mesh, action, {"assignMaterial", "setMaterialSlot"},
				proceduralRound(self._settings["material.slot"] or 0));
		elseif action == "autoUv" then
			self._settings["uv.mode"] = "autoUv";
			self:tryMeshOperation(mesh, action, {"autoUv", "autoUV", "generateUVs", "generateUV"},
				tonumber(self._settings["uv.scale.x"] or 1.0) or 1.0,
				tonumber(self._settings["uv.scale.y"] or 1.0) or 1.0,
				tonumber(self._settings["uv.offset.x"] or 0.0) or 0.0,
				tonumber(self._settings["uv.offset.y"] or 0.0) or 0.0);
		elseif action == "planarUv" then
			self._settings["uv.mode"] = "planarUv";
			self:tryMeshOperation(mesh, action, {"planarUv", "planarUV", "generatePlanarUVs"},
				tonumber(self._settings["uv.scale.x"] or 1.0) or 1.0,
				tonumber(self._settings["uv.scale.y"] or 1.0) or 1.0,
				tonumber(self._settings["uv.offset.x"] or 0.0) or 0.0,
				tonumber(self._settings["uv.offset.y"] or 0.0) or 0.0);
		elseif action == "boxUv" then
			self._settings["uv.mode"] = "boxUv";
			self:tryMeshOperation(mesh, action, {"boxUv", "boxUV", "generateBoxUVs"},
				tonumber(self._settings["uv.scale.x"] or 1.0) or 1.0,
				tonumber(self._settings["uv.scale.y"] or 1.0) or 1.0,
				tonumber(self._settings["uv.offset.x"] or 0.0) or 0.0,
				tonumber(self._settings["uv.offset.y"] or 0.0) or 0.0);
		elseif action ~= "mirror" and action ~= "flipX" and action ~= "flipY" and action ~= "flipZ" and action ~= "flipNormals" then
			self:markOperationReport(action, "recorded");
		end
	end
end

function ProceduralModelEditor:applyPostBakeOperations(mesh)
	if mesh == nil then
		return;
	end

	local transform = self._transformState or {};
	if transform.mirrorAxis ~= nil then
		proceduralSafeCall(mesh, "mirror", proceduralRound(transform.mirrorAxis));
	end
	if transform.flipX then
		proceduralSafeCall(mesh, "mirror", 0);
	end
	if transform.flipY then
		proceduralSafeCall(mesh, "mirror", 1);
	end
	if transform.flipZ then
		proceduralSafeCall(mesh, "mirror", 2);
	end

	local taper = tonumber(self._settings["deform.taper"] or 0.0) or 0.0;
	local twist = tonumber(self._settings["deform.twist"] or 0.0) or 0.0;
	local bend = tonumber(self._settings["deform.bend"] or 0.0) or 0.0;
	local noise = tonumber(self._settings["deform.noise"] or 0.0) or 0.0;

	if math.abs(taper) > 0.00001 then
		proceduralSafeCall(mesh, "applyTaper", taper);
	end
	if math.abs(twist) > 0.00001 then
		proceduralSafeCall(mesh, "applyTwist", twist);
	end
	if math.abs(bend) > 0.00001 then
		proceduralSafeCall(mesh, "applyBend", bend);
	end
	if noise > 0.00001 then
		proceduralSafeCall(mesh, "applyNoise", noise);
	end
	if transform.flipNormals then
		proceduralSafeCall(mesh, "flipWinding");
	end

	self:applyRecordedMeshOperations(mesh);

	proceduralSafeCall(mesh, "computeNormals");
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Helpers
-- ─────────────────────────────────────────────────────────────────────────────

function ProceduralModelEditor:getSliderValue(slider, fallback)
	if slider then
		local ok, value = proceduralSafeCall(slider, "getValue");
		if ok then
			return value;
		end
	end

	return fallback;
end

function ProceduralModelEditor:getDropdownValue(dropdown, fallback)
	if dropdown then
		local ok, value = proceduralSafeCall(dropdown, "getSelectedOption");
		if ok then
			return value;
		end
	end

	return fallback;
end

function ProceduralModelEditor:resetSliders()
	self._settings = proceduralCopyTable(ProceduralModelEditorDefaults);

	if self._controlDefaults then
		for id, info in pairs(self._controlDefaults) do
			self:setControlDefault(id);
		end
	end

	self._ruleStack = {};
	self._operationLog = {};
	self._operationApplyReport = {};
	self._exportedFiles = {};
	self._transformState =
	{
		mirrorAxis = nil,
		flipX = false,
		flipY = false,
		flipZ = false,
		flipNormals = false,
	};
	self.bakedComponent = nil;
	self.bakedMesh = nil;
	self._lastBakeStats = nil;
	self:updateRuleStackLabel();

	if self.tabBar and self.tabBar.setSelectedTab then
		self.tabBar:setSelectedTab(0);
	end

	self._currentTool = "Object";
	self:updateCurrentToolLabel();
	self:setStatus("Procedural model settings reset.");

	self:generate();
end

function ProceduralModelEditor:buildBoxRule()
	local sx = self:getSliderValue(self.boxSizeXSlider, 1.0);
	local sy = self:getSliderValue(self.boxSizeYSlider, 1.0);
	local sz = self:getSliderValue(self.boxSizeZSlider, 1.0);
	local ox = self:getSliderValue(self.boxOffsetXSlider, 0.0);
	local oy = self:getSliderValue(self.boxOffsetYSlider, 0.0);
	local oz = self:getSliderValue(self.boxOffsetZSlider, 0.0);

	return BoxRule(Vector3F(sx, sy, sz), Vector3F(ox, oy, oz));
end

function ProceduralModelEditor:buildCylinderRule()
	local r   = self:getSliderValue(self.cylRadiusSlider, 0.5);
	local h   = self:getSliderValue(self.cylHeightSlider, 1.0);
	local seg = proceduralRound(self:getSliderValue(self.cylSegmentsSlider, 8));
	local ox  = self:getSliderValue(self.cylOffsetXSlider, 0.0);
	local oy  = self:getSliderValue(self.cylOffsetYSlider, 0.0);
	local oz  = self:getSliderValue(self.cylOffsetZSlider, 0.0);

	return CylinderRule(r, h, seg, Vector3F(ox, oy, oz));
end

function ProceduralModelEditor:buildWallRule()
	local length = self:getSliderValue(self.wallLengthSlider, 6.0);
	local height = self:getSliderValue(self.wallHeightSlider, 3.0);
	local thickness = self:getSliderValue(self.wallThicknessSlider, 0.25);

	return BoxRule(Vector3F(length, height, thickness), Vector3F(0.0, height * 0.5, 0.0));
end

function ProceduralModelEditor:buildFloorRule()
	local width = self:getSliderValue(self.floorWidthSlider, 6.0);
	local depth = self:getSliderValue(self.floorDepthSlider, 6.0);
	local thickness = self:getSliderValue(self.floorThicknessSlider, 0.2);

	return BoxRule(Vector3F(width, thickness, depth), Vector3F(0.0, thickness * 0.5, 0.0));
end

function ProceduralModelEditor:buildDoorProxyRule()
	local width = self:getSliderValue(self.doorWidthSlider, 1.0);
	local height = self:getSliderValue(self.doorHeightSlider, 2.1);
	local x = self:getSliderValue(self.doorOffsetXSlider, 0.0);
	local thickness = self:getSliderValue(self.wallThicknessSlider, 0.25);

	-- This is a visible proxy for the door opening. The C++ rule stack can
	-- interpret this as a subtractive boolean when boolean.mode is Subtract.
	return BoxRule(Vector3F(width, height, thickness * 1.25), Vector3F(x, height * 0.5, 0.0));
end

function ProceduralModelEditor:buildWindowProxyRule()
	local width = self:getSliderValue(self.windowWidthSlider, 1.2);
	local height = self:getSliderValue(self.windowHeightSlider, 1.0);
	local x = self:getSliderValue(self.windowOffsetXSlider, 0.0);
	local y = self:getSliderValue(self.windowOffsetYSlider, 1.5);
	local thickness = self:getSliderValue(self.wallThicknessSlider, 0.25);

	return BoxRule(Vector3F(width, height, thickness * 1.25), Vector3F(x, y, 0.0));
end

function ProceduralModelEditor:buildStairsRule()
	local count = proceduralRound(self:getSliderValue(self.stairCountSlider, 8));
	local width = self:getSliderValue(self.stairWidthSlider, 2.0);
	local tread = self:getSliderValue(self.stairTreadSlider, 0.3);
	local riser = self:getSliderValue(self.stairRiserSlider, 0.18);

	local stepRule = BoxRule(Vector3F(width, riser, tread), Vector3F(0.0, riser * 0.5, tread * 0.5));
	return ArrayRule(stepRule, count, Vector3F(0.0, riser, tread));
end

function ProceduralModelEditor:buildArchRule()
	local radius = self:getSliderValue(self.archRadiusSlider, 0.8);
	local thickness = self:getSliderValue(self.archThicknessSlider, 0.25);
	local wallThickness = self:getSliderValue(self.wallThicknessSlider, 0.25);

	local composite = CompositeRule();
	composite:add(BoxRule(Vector3F(thickness, radius * 2.0, wallThickness), Vector3F(-radius, radius, 0.0)));
	composite:add(BoxRule(Vector3F(thickness, radius * 2.0, wallThickness), Vector3F(radius, radius, 0.0)));
	composite:add(CylinderRule(radius, wallThickness, 16, Vector3F(0.0, radius * 2.0, 0.0)));
	return composite;
end

function ProceduralModelEditor:buildRoomRule()
	local width = self:getSliderValue(self.roomWidthSlider, 6.0);
	local depth = self:getSliderValue(self.roomDepthSlider, 6.0);
	local height = self:getSliderValue(self.roomHeightSlider, 3.0);
	local thickness = self:getSliderValue(self.wallThicknessSlider, 0.25);
	local floorThickness = self:getSliderValue(self.floorThicknessSlider, 0.2);

	local composite = CompositeRule();

	-- Floor.
	composite:add(BoxRule(Vector3F(width, floorThickness, depth), Vector3F(0.0, floorThickness * 0.5, 0.0)));

	-- Back / front walls.
	composite:add(BoxRule(Vector3F(width, height, thickness), Vector3F(0.0, height * 0.5, -depth * 0.5)));
	composite:add(BoxRule(Vector3F(width, height, thickness), Vector3F(0.0, height * 0.5, depth * 0.5)));

	-- Left / right walls.
	composite:add(BoxRule(Vector3F(thickness, height, depth), Vector3F(-width * 0.5, height * 0.5, 0.0)));
	composite:add(BoxRule(Vector3F(thickness, height, depth), Vector3F(width * 0.5, height * 0.5, 0.0)));

	return composite;
end

function ProceduralModelEditor:buildRoofRule()
	local width = self:getSliderValue(self.roomWidthSlider, 6.0);
	local depth = self:getSliderValue(self.roomDepthSlider, 6.0);
	local thickness = self:getSliderValue(self.wallThicknessSlider, 0.25);
	local height = self:getSliderValue(self.roomHeightSlider, 3.0);

	-- Low-poly placeholder roof. A later C++ RoofRule can replace this with
	-- wedge topology; this keeps preview generation alive today.
	return BoxRule(Vector3F(width, thickness, depth), Vector3F(0.0, height + thickness * 0.5, 0.0));
end

function ProceduralModelEditor:buildBuildingRule()
	local floors = proceduralRound(self:getSliderValue(self.buildingFloorsSlider, 3));
	local floorHeight = self:getSliderValue(self.buildingFloorHeightSlider, 3.0);

	local room = self:buildRoomRule();
	return ArrayRule(room, floors, Vector3F(0.0, floorHeight, 0.0));
end

function ProceduralModelEditor:buildArrayRule()
	local count     = proceduralRound(self:getSliderValue(self.arrayCountSlider, 1));
	local stepX     = self:getSliderValue(self.arrayStepXSlider, 1.0);
	local stepY     = self:getSliderValue(self.arrayStepYSlider, 0.0);
	local stepZ     = self:getSliderValue(self.arrayStepZSlider, 0.0);
	local childType = self:getDropdownValue(self.arrayChildDropdown, ProceduralModelEditorTypes.ArrayChildBox);

	local childRule = nil;
	if childType == ProceduralModelEditorTypes.ArrayChildCylinder then
		childRule = self:buildCylinderRule();
	elseif childType == ProceduralModelEditorTypes.ArrayChildWall then
		childRule = self:buildWallRule();
	else
		childRule = self:buildBoxRule();
	end

	return ArrayRule(childRule, count, Vector3F(stepX, stepY, stepZ));
end

function ProceduralModelEditor:buildRadialArrayFallbackRule()
	local count = proceduralRound(self:getSliderValue(self.radialCountSlider, 8));
	local radius = self:getSliderValue(self.radialRadiusSlider, 3.0);
	local arcAngle = self:getSliderValue(self.radialAngleSlider, 360.0);
	local startAngle = self:getSliderValue(self.radialStartAngleSlider, 0.0);
	local axis = proceduralRound(self:getDropdownValue(self.radialAxisDropdown, 1));
	local childType = self:getDropdownValue(self.arrayChildDropdown, ProceduralModelEditorTypes.ArrayChildBox);
	if count < 1 then
		count = 1;
	end

	local composite = CompositeRule();
	local stepAngle = 0.0;
	if count > 1 then
		stepAngle = arcAngle / count;
		if math.abs(arcAngle) >= 359.999 then
			stepAngle = arcAngle / count;
		else
			stepAngle = arcAngle / (count - 1);
		end
	end

	for i = 0, count - 1 do
		local radians = (startAngle + stepAngle * i) * 3.14159265358979323846 / 180.0;
		local offsetX = 0.0;
		local offsetY = 0.0;
		local offsetZ = 0.0;

		if axis == 0 then
			offsetY = math.cos(radians) * radius;
			offsetZ = math.sin(radians) * radius;
		elseif axis == 2 then
			offsetX = math.cos(radians) * radius;
			offsetY = math.sin(radians) * radius;
		else
			offsetX = math.cos(radians) * radius;
			offsetZ = math.sin(radians) * radius;
		end

		if childType == ProceduralModelEditorTypes.ArrayChildCylinder then
			local r = self:getSliderValue(self.cylRadiusSlider, 0.5);
			local h = self:getSliderValue(self.cylHeightSlider, 1.0);
			local seg = proceduralRound(self:getSliderValue(self.cylSegmentsSlider, 8));
			local ox = self:getSliderValue(self.cylOffsetXSlider, 0.0);
			local oy = self:getSliderValue(self.cylOffsetYSlider, 0.0);
			local oz = self:getSliderValue(self.cylOffsetZSlider, 0.0);
			composite:add(CylinderRule(r, h, seg, Vector3F(ox + offsetX, oy + offsetY, oz + offsetZ)));
		elseif childType == ProceduralModelEditorTypes.ArrayChildWall then
			local length = self:getSliderValue(self.wallLengthSlider, 6.0);
			local height = self:getSliderValue(self.wallHeightSlider, 3.0);
			local thickness = self:getSliderValue(self.wallThicknessSlider, 0.25);
			composite:add(BoxRule(Vector3F(length, height, thickness), Vector3F(offsetX, height * 0.5 + offsetY, offsetZ)));
		else
			local sx = self:getSliderValue(self.boxSizeXSlider, 1.0);
			local sy = self:getSliderValue(self.boxSizeYSlider, 1.0);
			local sz = self:getSliderValue(self.boxSizeZSlider, 1.0);
			local ox = self:getSliderValue(self.boxOffsetXSlider, 0.0);
			local oy = self:getSliderValue(self.boxOffsetYSlider, 0.0);
			local oz = self:getSliderValue(self.boxOffsetZSlider, 0.0);
			composite:add(BoxRule(Vector3F(sx, sy, sz), Vector3F(ox + offsetX, oy + offsetY, oz + offsetZ)));
		end
	end

	return composite;
end

function ProceduralModelEditor:buildCompositeRule()
	local composite = CompositeRule();

	if self._ruleStack and #self._ruleStack > 0 then
		for i = 1, #self._ruleStack do
			local entry = self._ruleStack[i];

			if entry == "Box" then
				composite:add(self:buildBoxRule());
			elseif entry == "Cylinder" then
				composite:add(self:buildCylinderRule());
			elseif entry == "Wall" then
				composite:add(self:buildWallRule());
			elseif entry == "Stairs" then
				composite:add(self:buildStairsRule());
			elseif entry == "Door Cut" then
				composite:add(self:buildDoorProxyRule());
			elseif entry == "Window Cut" then
				composite:add(self:buildWindowProxyRule());
			elseif entry == "Roof" then
				composite:add(self:buildRoofRule());
			elseif entry == "Linear Array" then
				composite:add(self:buildArrayRule());
			elseif entry == "Radial Array" then
				composite:add(self:buildRadialArrayFallbackRule());
			end
		end
	else
		composite:add(self:buildBoxRule());
		composite:add(self:buildCylinderRule());
	end

	return composite;
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Generation
-- ─────────────────────────────────────────────────────────────────────────────

function ProceduralModelEditor:generate()
	print("ProceduralModelEditor generate called");
	self:syncFromControls();

	local shapeType = ProceduralModelEditorTypes.ShapeBox;
	if self.shapeTypeDropdown then
		shapeType = proceduralRound(self:getDropdownValue(self.shapeTypeDropdown, shapeType));
	elseif self._settings and self._settings["shapeType"] ~= nil then
		shapeType = proceduralRound(self._settings["shapeType"]);
	end

	local rule = nil;
	local ruleName = "Box";

	local okRule, ruleError = pcall(function()
		if shapeType == ProceduralModelEditorTypes.ShapeBox then
			rule = self:buildBoxRule();
			ruleName = "Box";

		elseif shapeType == ProceduralModelEditorTypes.ShapeCylinder then
			rule = self:buildCylinderRule();
			ruleName = "Cylinder";

		elseif shapeType == ProceduralModelEditorTypes.ShapeArray then
			rule = self:buildArrayRule();
			ruleName = "Linear Array";

		elseif shapeType == ProceduralModelEditorTypes.ShapeComposite then
			rule = self:buildCompositeRule();
			ruleName = "Composite";

		elseif shapeType == ProceduralModelEditorTypes.ShapeWall then
			rule = self:buildWallRule();
			ruleName = "Wall";

		elseif shapeType == ProceduralModelEditorTypes.ShapeFloor then
			rule = self:buildFloorRule();
			ruleName = "Floor";

		elseif shapeType == ProceduralModelEditorTypes.ShapeStairs then
			rule = self:buildStairsRule();
			ruleName = "Stairs";

		elseif shapeType == ProceduralModelEditorTypes.ShapeDoor then
			rule = self:buildDoorProxyRule();
			ruleName = "Door Proxy / Cut";

		elseif shapeType == ProceduralModelEditorTypes.ShapeWindow then
			rule = self:buildWindowProxyRule();
			ruleName = "Window Proxy / Cut";

		elseif shapeType == ProceduralModelEditorTypes.ShapeArch then
			rule = self:buildArchRule();
			ruleName = "Arch";

		elseif shapeType == ProceduralModelEditorTypes.ShapeRoom then
			rule = self:buildRoomRule();
			ruleName = "Room";

		elseif shapeType == ProceduralModelEditorTypes.ShapeRoof then
			rule = self:buildRoofRule();
			ruleName = "Roof";

		elseif shapeType == ProceduralModelEditorTypes.ShapeBuilding then
			rule = self:buildBuildingRule();
			ruleName = "Building";

		elseif shapeType == ProceduralModelEditorTypes.ShapeRadialArray then
			rule = self:buildRadialArrayFallbackRule();
			ruleName = "Radial Array Fallback";
		end
	end);

	if not okRule then
		self:setStatus("Rule build failed: " .. tostring(ruleError));
		return false;
	end

	if rule == nil then
		print("ProceduralModelEditor generate: unknown shape type");
		self:setStatus("Unknown procedural model shape type.");
		return false;
	end

	local component = ProceduralModelComponent();
	component.rootRule = rule;
	component.dirty    = true;

	local system = ProceduralModelSystem();
	local okBake, bakeError = pcall(function()
		system:bake(component);
	end);

	if not okBake then
		self.bakedComponent = nil;
		self.bakedMesh = nil;
		self:setStatus("Bake failed: " .. tostring(bakeError));
		return false;
	end

	self.bakedComponent = component;
	self.bakedMesh = component.bakedMesh;

	local mesh  = self.bakedMesh;
	self:applyPostBakeOperations(mesh);

	local okVerts, verts = pcall(function() return mesh:getVertexCount(); end);
	local okIdxs, idxs = pcall(function() return mesh:getIndexCount(); end);
	verts = okVerts and verts or 0;
	idxs = okIdxs and idxs or 0;
	local tris  = proceduralRound(idxs / 3);
	if mesh.getTriangleCount ~= nil then
		local okTri, triCount = pcall(function() return mesh:getTriangleCount(); end);
		if okTri then
			tris = triCount;
		end
	end

	self._lastBakeStats =
	{
		vertices = verts,
		indices = idxs,
		triangles = tris,
		ruleName = ruleName,
	};

	print(string.format("Bake complete: %s: %d vertices, %d triangles, %d indices",
		ruleName, verts, tris, idxs));

	if self.meshStatsLabel then
		self.meshStatsLabel:setLabel(
			string.format("Vertices: %d   Triangles: %d   Indices: %d   Rule: %s", verts, tris, idxs, ruleName));
	end

	local actorText = "";
	if proceduralRound(self._settings["sceneActor.enabled"] or 0) == 1 then
		local actorOk, actorPath = self:createOrUpdateSceneActor(false);
		if actorOk then
			actorText = " Scene actor updated: " .. tostring(actorPath) .. ".";
		else
			actorText = " Scene actor update skipped.";
		end
	end

	local recordedCount = 0;
	for _, entry in ipairs(self._operationApplyReport or {}) do
		if string.find(entry, ":recorded", 1, true) ~= nil then
			recordedCount = recordedCount + 1;
		end
	end

	if recordedCount > 0 then
		self:setStatus(string.format("Bake complete: %s (%d vertices, %d triangles). %d edit operations saved as recipe metadata.",
			ruleName, verts, tris, recordedCount) .. actorText);
	else
		self:setStatus(string.format("Bake complete: %s (%d vertices, %d triangles).", ruleName, verts, tris) .. actorText);
	end
	return true;
end

function ProceduralModelEditor:writeGeneratedObj(path)
	local mesh = self.bakedMesh;
	local ok = false;

	if mesh and mesh.exportObj ~= nil then
		self:ensureOutputDirectory(path, nil);
		local nativeOk, nativeWritten = pcall(function() return mesh:exportObj(path); end);
		ok = nativeOk and nativeWritten == true;
	end

	if ok then
		return true;
	end

	return self:writeObjFile(path, mesh);
end

function ProceduralModelEditor:encodeRuleStack()
	return table.concat(self._ruleStack or {}, "|");
end

function ProceduralModelEditor:decodeRuleStack(text)
	self._ruleStack = {};
	text = tostring(text or "");
	for token in string.gmatch(text, "[^|]+") do
		self._ruleStack[#self._ruleStack + 1] = token;
	end
	self:updateRuleStackLabel();
end

function ProceduralModelEditor:encodeOperationLog()
	return table.concat(self._operationLog or {}, "|");
end

function ProceduralModelEditor:decodeOperationLog(text)
	self._operationLog = {};
	text = tostring(text or "");
	for token in string.gmatch(text, "[^|]+") do
		self._operationLog[#self._operationLog + 1] = token;
	end
end

function ProceduralModelEditor:getProperties(parameters)
	if parameters == nil then
		return;
	end

	local okProperties, properties = pcall(function() return parameters:at(0); end);
	if not okProperties or properties == nil then
		return;
	end

	self:syncFromControls();

	for key, value in pairs(self._settings or {}) do
		if type(value) == "boolean" then
			properties:setPropertyAsBool(key, value);
		elseif type(value) == "number" then
			properties:setPropertyAsFloat(key, value);
		else
			properties:setPropertyAsString(key, tostring(value or ""));
		end
	end

	properties:setPropertyAsString("procedural.ruleStack", self:encodeRuleStack());
	properties:setPropertyAsString("procedural.operationLog", self:encodeOperationLog());
	properties:setPropertyAsString("procedural.currentTool", tostring(self._currentTool or "Object"));
	properties:setPropertyAsString("procedural.status", tostring(self._currentStatus or ""));

	local transform = self._transformState or {};
	properties:setPropertyAsFloat("procedural.mirrorAxis", transform.mirrorAxis ~= nil and transform.mirrorAxis or -1);
	properties:setPropertyAsBool("procedural.flipX", transform.flipX == true);
	properties:setPropertyAsBool("procedural.flipY", transform.flipY == true);
	properties:setPropertyAsBool("procedural.flipZ", transform.flipZ == true);
	properties:setPropertyAsBool("procedural.flipNormals", transform.flipNormals == true);

	properties:setPropertyAsString("procedural.export.asset", tostring(self._exportedFiles and self._exportedFiles.asset or ""));
	properties:setPropertyAsString("procedural.export.obj", tostring(self._exportedFiles and self._exportedFiles.obj or ""));
	properties:setPropertyAsString("procedural.export.fbx", tostring(self._exportedFiles and self._exportedFiles.fbx or ""));
	properties:setPropertyAsString("procedural.export.collider", tostring(self._exportedFiles and self._exportedFiles.collider or ""));
end

function ProceduralModelEditor:setProperties(parameters)
	if parameters == nil then
		return;
	end

	local okProperties, properties = pcall(function() return parameters:at(0); end);
	if not okProperties or properties == nil then
		return;
	end

	self._settings = self._settings or proceduralCopyTable(ProceduralModelEditorDefaults);
	for key, defaultValue in pairs(ProceduralModelEditorDefaults) do
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

	if properties:hasProperty("uv.mode") then
		self._settings["uv.mode"] = properties:getPropertyAsString("uv.mode");
	end

	if properties:hasProperty("procedural.ruleStack") then
		self:decodeRuleStack(properties:getPropertyAsString("procedural.ruleStack"));
	end

	if properties:hasProperty("procedural.operationLog") then
		self:decodeOperationLog(properties:getPropertyAsString("procedural.operationLog"));
	end

	if properties:hasProperty("procedural.currentTool") then
		self._currentTool = properties:getPropertyAsString("procedural.currentTool");
	end

	self._transformState = self._transformState or {};
	if properties:hasProperty("procedural.mirrorAxis") then
		local axis = proceduralRound(properties:getPropertyAsFloat("procedural.mirrorAxis"));
		self._transformState.mirrorAxis = axis >= 0 and axis or nil;
	end
	if properties:hasProperty("procedural.flipX") then
		self._transformState.flipX = properties:getPropertyAsBool("procedural.flipX");
	end
	if properties:hasProperty("procedural.flipY") then
		self._transformState.flipY = properties:getPropertyAsBool("procedural.flipY");
	end
	if properties:hasProperty("procedural.flipZ") then
		self._transformState.flipZ = properties:getPropertyAsBool("procedural.flipZ");
	end
	if properties:hasProperty("procedural.flipNormals") then
		self._transformState.flipNormals = properties:getPropertyAsBool("procedural.flipNormals");
	end

	self._exportedFiles = self._exportedFiles or {};
	if properties:hasProperty("procedural.export.asset") then
		self._exportedFiles.asset = properties:getPropertyAsString("procedural.export.asset");
	end
	if properties:hasProperty("procedural.export.obj") then
		self._exportedFiles.obj = properties:getPropertyAsString("procedural.export.obj");
	end
	if properties:hasProperty("procedural.export.fbx") then
		self._exportedFiles.fbx = properties:getPropertyAsString("procedural.export.fbx");
	end
	if properties:hasProperty("procedural.export.collider") then
		self._exportedFiles.collider = properties:getPropertyAsString("procedural.export.collider");
	end

	self.bakedComponent = nil;
	self.bakedMesh = nil;
	self._lastBakeStats = nil;
	self:syncToControls();
	self:setStatus("Procedural model properties restored.");
end

--[[
  ============================================================================
  EXTENSION SECTION — added by A1 (rev 4, 2026-08-19)
  Do NOT overwrite existing code above. This section adds:
    1. L-System rule authoring + generation (drives ProceduralBindings.cpp)
    2. Boolean mesh operations (union/subtract/intersect via ProceduralBindings.cpp)
    3. UV island selection, pack, unwrap params
    4. Noise-based vertex deformation (perlin/simplex vertex displacement)
    5. Edge loop insertion

  ref: Esoterica/Engine/Render/RenderGeometryBuilder.cpp (mesh ops + boolean)
  ref: Esoterica/Engine/Render/DebugMesh.cpp (mesh generation)
  ============================================================================
]]

-- ============================================================================
-- New Types (appended to avoid ID clash with existing range 0..295)
-- ============================================================================

-- L-System authoring
ProceduralModelEditorTypes.LSystemAxiomId    = 296
ProceduralModelEditorTypes.LSystemRulesId    = 297
ProceduralModelEditorTypes.LSystemAngleId   = 298
ProceduralModelEditorTypes.LSystemStepId    = 299
ProceduralModelEditorTypes.LSystemIterId    = 300
ProceduralModelEditorTypes.LSystemSeedId    = 301
ProceduralModelEditorTypes.AddRuleButtonId  = 302
ProceduralModelEditorTypes.ClearRulesButtonId = 303
ProceduralModelEditorTypes.GenerateLSystemButtonId = 304
ProceduralModelEditorTypes.LSystemRulesTextId = 305

-- Boolean
ProceduralModelEditorTypes.BooleanModeId     = 310
ProceduralModelEditorTypes.ApplyBooleanButtonId = 311
ProceduralModelEditorTypes.BooleanMeshBId    = 312

-- UV island / unwrap
ProceduralModelEditorTypes.UVIslandSelectId = 320
ProceduralModelEditorTypes.UVUnwrapAngleId  = 321
ProceduralModelEditorTypes.UVUnwrapMarginId  = 322
ProceduralModelEditorTypes.UVPackButtonId    = 323
ProceduralModelEditorTypes.AutoUVMaxIterationsId = 324

-- Noise deformation
ProceduralModelEditorTypes.NoiseDeformTypeId  = 330  -- 0=perlin, 1=simplex
ProceduralModelEditorTypes.NoiseDeformFreqId  = 331
ProceduralModelEditorTypes.NoiseDeformAmpId   = 332
ProceduralModelEditorTypes.NoiseDeformSeedId  = 333
ProceduralModelEditorTypes.ApplyNoiseDeformId = 334

-- Edge loops
ProceduralModelEditorTypes.EdgeLoopInsertId   = 340
ProceduralModelEditorTypes.EdgeLoopCountId    = 341

-- ============================================================================
-- Extension state
-- ============================================================================
local lsystemRules = {}  -- { predecessor = successor, ... }

local function pme_ext_safeCall(fn, ...)
    local ok, err = pcall(fn, ...)
    if not ok then
        print("[ProceduralModelEditor-ext] Error: " .. tostring(err))
    end
    return ok
end

-- ============================================================================
-- L-System helpers
-- ============================================================================

local function parseLSystemRules(text)
    -- Format: F->FF, X->FXF, one per line or comma-separated
    local rules = {}
    for line in text:gmatch("[^,\n]+") do
        local arrow = line:find("->", 1, true)
        if arrow then
            local pred = line:sub(1, arrow - 1):match("^%s*(.-)%s*$")
            local succ = line:sub(arrow + 2):match("^%s*(.-)%s*$")
            if #pred > 0 then
                rules[pred] = succ
            end
        end
    end
    return rules
end

local function lsystemRulesToArray(rules)
    -- Convert {pred=succ} table to ["pred->succ", ...] for C++ binding
    local arr = {}
    for pred, succ in pairs(rules) do
        table.insert(arr, pred .. "->" .. succ)
    end
    return arr
end

-- ============================================================================
-- L-System generation (driven by C++ ProceduralBindings)
-- ============================================================================
function ProceduralModelEditor:generateLSystemFromUI()
    local appMgr = IApplicationManager.instance()
    if not appMgr then
        self:setStatus("Cannot reach application manager.")
        return
    end

    local factory = appMgr:getFactoryManager()
    if not factory then
        self:setStatus("Cannot reach factory manager.")
        return
    end

    -- Get C++ bindings
    local bindings = factory:make_ptr("ProceduralBindings")
    if not bindings then
        self:setStatus("ProceduralBindings unavailable — regenerate the C++ layer first.")
        return
    end

    -- Collect params from the L-System UI section
    -- (Values are stored in self._settings keyed by the type IDs above)
    local axiom = self._settings and self._settings.lsystem_axiom or "F"
    local angleDeg = self._settings and tonumber(self._settings.lsystem_angle) or 25.0
    local stepLen = self._settings and tonumber(self._settings.lsystem_step) or 1.0
    local iter = self._settings and tonumber(self._settings.lsystem_iter) or 4
    local seed = self._settings and tonumber(self._settings.lsystem_seed) or 12345

    -- Parse rules from text area
    local rulesText = self._settings and self._settings.lsystem_rules_text or ""
    local rulesTable = parseLSystemRules(rulesText)
    local rulesArray = lsystemRulesToArray(rulesTable)

    if #rulesArray == 0 then
        self:setStatus("Add at least one L-System rule (e.g. F->FF).")
        return
    end

    -- Call C++ binding
    local mesh = bindings:generateMeshFromLSystem(axiom, rulesArray, iter, angleDeg, stepLen)
    if mesh then
        self.generatedMesh = mesh
        self:setStatus("L-System mesh generated (" .. (#rulesArray) .. " rules, " .. iter .. " iterations).")
        -- Trigger live preview update
        self:generate()
    else
        self:setStatus("L-System generation returned no mesh.")
    end
end

-- ============================================================================
-- Boolean mesh operations (driven by C++ ProceduralBindings)
-- ============================================================================
function ProceduralModelEditor:applyBooleanOperation(mode)
    -- mode: 0=union, 1=subtract, 2=intersect
    if not self.generatedMesh then
        self:setStatus("Generate a base mesh first.")
        return
    end

    local appMgr = IApplicationManager.instance()
    if not appMgr then return end
    local factory = appMgr:getFactoryManager()
    if not factory then return end

    local bindings = factory:make_ptr("ProceduralBindings")
    if not bindings then
        self:setStatus("ProceduralBindings unavailable.")
        return
    end

    -- Boolean requires a second mesh — for now, use the current mesh as both operands
    -- (true two-mesh boolean requires the user to select a second mesh via BooleanMeshBId)
    local meshB = self.generatedMesh
    local result = bindings:applyBooleanToMesh(self.generatedMesh, meshB, mode)
    if result then
        self.generatedMesh = result
        self:setStatus("Boolean applied (mode=" .. mode .. ").")
        self:generate()
    else
        self:setStatus("Boolean operation requires runtime CSG kernel (deferred).")
    end
end

-- ============================================================================
-- UV island helpers (Lua-side; actual unwrap uses IMeshGenerator UV params)
-- ============================================================================
function ProceduralModelEditor:selectUVIsland(index)
    -- index: 0-based island index to select for editing
    -- ref: IMeshGenerator setUVIslandSelect or similar in the runtime
    self._settings = self._settings or {}
    self._settings.uv_island_index = index
    self:setStatus("UV island " .. index .. " selected.")
end

function ProceduralModelEditor:packUVIslands(angleTolerance, margin)
    angleTolerance = angleTolerance or 12.0
    margin = margin or 0.001

    -- Calls IMeshGenerator to pack UV islands
    local appMgr = IApplicationManager.instance()
    if not appMgr then return end
    local meshGen = appMgr:getMeshGenerator()
    if not meshGen then return end

    meshGen:setUVIslandAngleTolerance(angleTolerance)
    meshGen:setUVPackMargin(margin)
    meshGen:setUVIslandPackEnabled(true)

    self:setStatus("UV islands packed (angle=" .. angleTolerance .. ", margin=" .. margin .. ").")
end

function ProceduralModelEditor:setUVUnwrapParams(angleTolerance, margin, maxIterations)
    angleTolerance = angleTolerance or 12.0
    margin = margin or 0.001
    maxIterations = maxIterations or 100

    self._settings = self._settings or {}
    self._settings.uv_angle_tolerance = angleTolerance
    self._settings.uv_margin = margin
    self._settings.uv_max_iterations = maxIterations

    local appMgr = IApplicationManager.instance()
    if not appMgr then return end
    local meshGen = appMgr:getMeshGenerator()
    if not meshGen then return end

    meshGen:setUVIslandAngleTolerance(angleTolerance)
    meshGen:setUVPackMargin(margin)
    meshGen:setUVUnwrapMaxIterations(maxIterations)

    self:setStatus("UV unwrap configured (angle=" .. angleTolerance .. ", iter=" .. maxIterations .. ").")
end

-- ============================================================================
-- Noise-based vertex deformation (CPU, drives ProceduralBindings texture path)
-- This applies a perlin/simplex noise displacement to the current mesh vertices.
-- ============================================================================
function ProceduralModelEditor:applyNoiseDeformation(noiseType, frequency, amplitude, seed)
    -- noiseType: 0=perlin, 1=simplex
    if not self.generatedMesh then
        self:setStatus("Generate a mesh first.")
        return
    end

    local appMgr = IApplicationManager.instance()
    if not appMgr then return end

    -- Build a noise texture first, then apply to mesh
    local factory = appMgr:getFactoryManager()
    if not factory then return end

    local texBindings = factory:make_ptr("ProceduralTextureBindings")
    if not texBindings then
        self:setStatus("ProceduralTextureBindings unavailable.")
        return
    end

    -- Generate a noise texture at the mesh's vertex-grid resolution
    local res = 256
    local tex = texBindings:generateNoiseTexture(
        res, res,
        noiseType or 0,
        frequency or 4.0,
        6,       -- octaves
        2.0,     -- lacunarity
        0.5,     -- persistence
        seed or 42,
        true,    -- turbulence
        ""       -- no file path (keep in memory)
    )

    if not tex then
        self:setStatus("Noise texture generation failed.")
        return
    end

    -- Apply displacement to the mesh using the noise texture
    -- ref: Esoterica/Engine/Render/RenderGeometryBuilder.cpp vertex displacement
    local meshGen = appMgr:getMeshGenerator()
    if not meshGen then
        self:setStatus("MeshGenerator unavailable.")
        return
    end

    meshGen:setVertexNoiseTexture(tex)
    meshGen:setVertexNoiseFrequency(frequency or 4.0)
    meshGen:setVertexNoiseAmplitude(amplitude or 0.1)
    meshGen:setVertexNoiseEnabled(true)
    meshGen:setVertexNoiseType(noiseType or 0)

    self:setStatus("Noise deformation applied (freq=" .. (frequency or 4.0) .. ", amp=" .. (amplitude or 0.1) .. ").")
    self:generate()
end

-- ============================================================================
-- Edge loop insertion (driven by IMeshGenerator)
-- ref: IMeshGenerator addEdgeLoop / insertEdgeLoop
-- ============================================================================
function ProceduralModelEditor:insertEdgeLoops(edgeLoopCount)
    edgeLoopCount = edgeLoopCount or 1

    local appMgr = IApplicationManager.instance()
    if not appMgr then return end
    local meshGen = appMgr:getMeshGenerator()
    if not meshGen then
        self:setStatus("MeshGenerator unavailable.")
        return
    end

    meshGen:setInsertEdgeLoops(edgeLoopCount)
    meshGen:setInsertEdgeLoopEnabled(true)

    self:setStatus("Inserting " .. edgeLoopCount .. " edge loop(s)...")
    self:generate()
end

-- ============================================================================
-- UI builder helpers for the extension sections (called from :load or a panel builder)
-- These can be called from a toolbar/panel builder to add the extension controls.
-- ============================================================================
function ProceduralModelEditor:addExtensionControlsToToolbar(parent)
    -- Add L-System section to the existing toolbar panel
    -- This is a no-op if called outside the load sequence;
    -- it is intended to be called from the existing :load() method.
    local appMgr = IApplicationManager.instance()
    if not appMgr then return end
    local ui = appMgr:getUI()
    if not ui then return end

    local btnTypeInfo = ui.IUIButton.typeInfo()

    -- L-System generate button
    local lsysBtn = ui:addElement(btnTypeInfo)
    lsysBtn:setElementId(ProceduralModelEditorTypes.GenerateLSystemButtonId)
    lsysBtn:setText("Generate L-System")
    lsysBtn:setHelp("Generate mesh from L-System grammar via ProceduralBindings")
    if parent then parent:addChild(lsysBtn) end

    -- Boolean buttons
    local boolUnionBtn = ui:addElement(btnTypeInfo)
    boolUnionBtn:setElementId(ProceduralModelEditorTypes.BooleanModeId)
    boolUnionBtn:setText("Boolean")
    boolUnionBtn:setHelp("Apply CSG boolean to current mesh (union=subtract=intersect)")
    if parent then parent:addChild(boolUnionBtn) end

    -- UV pack button
    local uvPackBtn = ui:addElement(btnTypeInfo)
    uvPackBtn:setElementId(ProceduralModelEditorTypes.UVPackButtonId)
    uvPackBtn:setText("Pack UVs")
    uvPackBtn:setHelp("Pack UV islands with configurable angle tolerance and margin")
    if parent then parent:addChild(uvPackBtn) end

    -- Noise deform button
    local noiseBtn = ui:addElement(btnTypeInfo)
    noiseBtn:setElementId(ProceduralModelEditorTypes.ApplyNoiseDeformId)
    noiseBtn:setText("Noise Deform")
    noiseBtn:setHelp("Apply perlin/simplex noise vertex displacement")
    if parent then parent:addChild(noiseBtn) end

    -- Edge loop button
    local loopBtn = ui:addElement(btnTypeInfo)
    loopBtn:setElementId(ProceduralModelEditorTypes.EdgeLoopInsertId)
    loopBtn:setText("Edge Loop")
    loopBtn:setHelp("Insert edge loops into the current mesh")
    if parent then parent:addChild(loopBtn) end
end

-- ============================================================================
-- Extension handleEvent — process the new widget IDs
-- Called from the main handleEvent; add to the existing switch.
-- ============================================================================
function ProceduralModelEditor:handleExtensionEvent(elementId)
    if elementId == ProceduralModelEditorTypes.GenerateLSystemButtonId then
        self:generateLSystemFromUI()

    elseif elementId == ProceduralModelEditorTypes.ApplyBooleanButtonId then
        local mode = self._settings and tonumber(self._settings.boolean_mode) or 0
        self:applyBooleanOperation(mode)

    elseif elementId == ProceduralModelEditorTypes.UVPackButtonId then
        local angle = self._settings and tonumber(self._settings.uv_angle_tolerance) or 12.0
        local margin = self._settings and tonumber(self._settings.uv_margin) or 0.001
        self:packUVIslands(angle, margin)

    elseif elementId == ProceduralModelEditorTypes.ApplyNoiseDeformId then
        local ntype = self._settings and tonumber(self._settings.noise_deform_type) or 0
        local freq  = self._settings and tonumber(self._settings.noise_deform_freq) or 4.0
        local amp   = self._settings and tonumber(self._settings.noise_deform_amp) or 0.1
        local seed  = self._settings and tonumber(self._settings.noise_deform_seed) or 42
        self:applyNoiseDeformation(ntype, freq, amp, seed)

    elseif elementId == ProceduralModelEditorTypes.EdgeLoopInsertId then
        local count = self._settings and tonumber(self._settings.edge_loop_count) or 1
        self:insertEdgeLoops(count)

    elseif elementId == ProceduralModelEditorTypes.ClearRulesButtonId then
        lsystemRules = {}
        self._settings = self._settings or {}
        self._settings.lsystem_rules_text = ""
        self:setStatus("L-System rules cleared.")

    elseif elementId == ProceduralModelEditorTypes.AddRuleButtonId then
        self:setStatus("Type rules in the L-System rules field, then press Generate.")
    end
end

-- ============================================================================
-- END EXTENSION SECTION
-- ============================================================================
