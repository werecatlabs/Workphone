--[[
  SceneBuilderEditor.lua
  A procedural scene / world / prefab builder for Workphone Editor.
  
  Surfaces: scene hierarchy, placement toolbar, procedural world generators,
            road/city/terrain generation, prefab authoring, layer/streaming.
  
  Driven by: C++ SceneBuilderBindings (IScriptReceiver).
  ref: Esoterica/Code/EngineTools/MapEditor/* (scene/map editor reference)
  
  This is a hot-reloadable Lua editor — edit this file and reload to see changes live.
]]

class 'SceneBuilderEditor' (BaseEditor)

-- ============================================================================
-- Widget IDs
-- ============================================================================
SceneBuilderEditorTypes = {
    -- Scene hierarchy
    SceneTreeId        = 5000,
    CreateActorId      = 5001,
    DeleteActorId      = 5002,
    DuplicateActorId   = 5003,
    RenameActorId      = 5004,

    -- Placement toolbar
    PlacementModeId    = 5100,  -- 0=paint, 1=single, 2=scatter
    PlacementTypeId    = 5101,  -- 0=prefab, 1=procedural, 2=terrain, 3=road
    PrefabPathId       = 5102,
    ProceduralPresetId  = 5103,
    BrushSizeId        = 5110,
    BrushStrengthId     = 5111,
    PlacementRotationXId = 5112,
    PlacementRotationYId = 5113,
    PlacementRotationZId = 5114,
    PlacementScaleMinId = 5115,
    PlacementScaleMaxId = 5116,

    -- Procedural world
    WorldSeedId        = 5200,
    WorldSizeId        = 5201,
    SeaLevelId         = 5202,
    HeightScaleId      = 5203,
    BiomeParamsId      = 5204,
    GenerateWorldBtnId = 5205,

    -- Terrain tools
    TerrainBrushId     = 5300,  -- 0=raise, 1=lower, 2=flatten, 3=smooth, 4=texture
    TerrainLayersId    = 5301,

    -- Road network
    RoadTypeId         = 5400,
    RoadSeedId         = 5401,
    BlockSizeId        = 5402,
    RoadWidthId        = 5403,
    BuildRoadsBtnId    = 5404,

    -- City generator
    CityRadiusId       = 5500,
    CityDensityId      = 5501,
    BuildingRuleId     = 5502,
    ParkPlacementId    = 5503,
    GenerateCityBtnId  = 5504,

    -- Layers
    LayersWindowId     = 5600,
    LayerToggleId      = 5601,
    LayerLockId        = 5602,
    LayerColorId       = 5603,

    -- Prefab
    CreatePrefabId     = 5700,
    InstantiatePrefabId = 5701,
    UnpackPrefabId     = 5702,

    -- LOD / quality
    LodLevelId         = 5800,
    QualityPresetId    = 5801,

    -- Save / load
    SaveSceneId        = 5900,
    LoadSceneId        = 5901,
    ExportPrefabId     = 5902,

    -- Object type filter in hierarchy
    FilterAllId        = 5950,
    FilterMeshId       = 5951,
    FilterLightId      = 5952,
    FilterCameraId     = 5953,
    FilterTerrainId    = 5954,
    FilterRoadId       = 5955,
    FilterCityId        = 5956,
}

local SceneBuilderEditorDefaults = {
    placementMode    = 0,   -- 0=paint
    placementType    = 0,   -- 0=prefab
    prefabPath      = "None",
    proceduralPreset = "DefaultBuilding",
    brushSize       = 2.0,
    brushStrength   = 0.5,
    rotationX       = 0.0,
    rotationY       = 0.0,
    rotationZ       = 0.0,
    scaleMin        = 1.0,
    scaleMax        = 1.0,

    worldSeed        = 12345,
    worldSize        = 8192,
    seaLevel         = 10.0,
    heightScale      = 200.0,
    biomeParams      = "",

    terrainBrush     = 0,  -- 0=raise
    terrainLayers    = 1,

    roadType         = 0,
    roadSeed         = 42,
    blockSize        = 100,
    roadWidth        = 8.0,

    cityRadius       = 2000,
    cityDensity      = 5,
    buildingRule     = "Standard",
    parkPlacement    = true,

    lodLevel         = 0,
    qualityPreset    = 0,  -- 0=high, 1=medium, 2=low
}

local function copyDefaults()
    local t = {}
    for k, v in pairs(SceneBuilderEditorDefaults) do
        t[k] = v
    end
    return t
end

-- ============================================================================
-- Helpers
-- ============================================================================
local function safeCall(fn, ...)
    local ok, err = pcall(fn, ...)
    if not ok then
        print("[SceneBuilderEditor] Error: " .. tostring(err))
    end
    return ok
end

local function safeGetNumber(ctrl, fallback)
    if not ctrl then return fallback end
    local v = ctrl:getValue()
    local n = tonumber(v)
    return n or fallback
end

local function safeGetText(ctrl, fallback)
    if not ctrl then return fallback or "" end
    return ctrl:getText() or ""
end

-- ============================================================================
-- Class
-- ============================================================================
function SceneBuilderEditor:__init(window)
    BaseEditor.__init(self, window)
    self.state = copyDefaults()
    self.scene = nil
    self.selectedActorIds = {}
    self.placementActor = nil
    self.currentLayer = 0
end

function SceneBuilderEditor:__finalize()
    self.scene = nil
    self.selectedActorIds = nil
    BaseEditor.__finalize(self)
end

-- ============================================================================
-- load — build the full editor UI
-- ============================================================================
function SceneBuilderEditor:load()
    BaseEditor.load(self)

    local appMgr   = IApplicationManager.instance()
    local ui       = appMgr:getUI()
    local parentWin = self.window:getParentWindow()
    if not parentWin then return end

    parentWin:setSize(Vector2F(1100.0, 750.0))

    -- -------------------------------------------------------
    -- 1. Scene hierarchy (left)
    -- -------------------------------------------------------
    local hierarchyWin = ui:addElement(ui.IUIWindow.typeInfo())
    hierarchyWin:setLabel("Scene Hierarchy")
    hierarchyWin:setSize(Vector2F(280.0, 600.0))
    hierarchyWin:setPosition(Vector2F(0, 0))
    parentWin:addChild(hierarchyWin)

    -- Toolbar row
    local btnTypeInfo = ui.IUIButton.typeInfo()
    local function addHbtn(parent, label, id, tip)
        local b = ui:addElement(btnTypeInfo)
        b:setElementId(id)
        b:setText(label)
        b:setHelp(tip or "")
        b:setSize(Vector2F(50.0, 20.0))
        parent:addChild(b)
        return b
    end

    local hToolbar = ui:addElement(ui.IUIWindow.typeInfo())
    hToolbar:setLabel("Actions")
    hToolbar:setSize(Vector2F(270.0, 26.0))
    hierarchyWin:addChild(hToolbar)

    addHbtn(hToolbar, "+", SceneBuilderEditorTypes.CreateActorId,    "Create new actor")
    addHbtn(hToolbar, "-", SceneBuilderEditorTypes.DeleteActorId,   "Delete selected")
    addHbtn(hToolbar, "D", SceneBuilderEditorTypes.DuplicateActorId, "Duplicate selected")
    addHbtn(hToolbar, "R", SceneBuilderEditorTypes.RenameActorId,   "Rename selected")

    -- Filter buttons row
    local hFilter = ui:addElement(ui.IUIWindow.typeInfo())
    hFilter:setLabel("Filter")
    hFilter:setSize(Vector2F(270.0, 26.0))
    hierarchyWin:addChild(hFilter)

    local filterTypeInfo = ui.IUILabelTogglePair.typeInfo()
    local function addToggle(parent, label, id)
        local t = ui:addElement(filterTypeInfo)
        t:setLabel(label)
        t:setElementId(id)
        t:setChecked(true)
        t:setSize(Vector2F(60.0, 20.0))
        parent:addChild(t)
        return t
    end

    addToggle(hFilter, "Mesh", SceneBuilderEditorTypes.FilterMeshId)
    addToggle(hFilter, "Light", SceneBuilderEditorTypes.FilterLightId)
    addToggle(hFilter, "Cam", SceneBuilderEditorTypes.FilterCameraId)
    addToggle(hFilter, "Terr", SceneBuilderEditorTypes.FilterTerrainId)

    -- Tree control for hierarchy
    local treeTypeInfo = ui.IUITreeCtrl.typeInfo()
    self.sceneTree = ui:addElement(treeTypeInfo)
    self.sceneTree:setElementId(SceneBuilderEditorTypes.SceneTreeId)
    self.sceneTree:setSize(Vector2F(270.0, 540.0))
    hierarchyWin:addChild(self.sceneTree)

    -- -------------------------------------------------------
    -- 2. Placement toolbar (top-right)
    -- -------------------------------------------------------
    local placeWin = ui:addElement(ui.IUIWindow.typeInfo())
    placeWin:setLabel("Placement")
    placeWin:setSize(Vector2F(800.0, 80.0))
    placeWin:setPosition(Vector2F(290.0, 0))
    parentWin:addChild(placeWin)

    local dropdownTypeInfo = ui.IUILabelDropdownPair.typeInfo()
    local labelInputTypeInfo = ui.IUILabelTextInputPair.typeInfo()
    local sliderTypeInfo = ui.IUILabelSliderPair.typeInfo()
    local toggleTypeInfo = ui.IUILabelTogglePair.typeInfo()

    -- Placement mode
    local modeDropdown = ui:addElement(dropdownTypeInfo)
    modeDropdown:setLabel("Mode:")
    modeDropdown:setElementId(SceneBuilderEditorTypes.PlacementModeId)
    modeDropdown:setSize(Vector2F(200.0, 24.0))
    placeWin:addChild(modeDropdown)

    -- Placement type
    local typeDropdown = ui:addElement(dropdownTypeInfo)
    typeDropdown:setLabel("Type:")
    typeDropdown:setElementId(SceneBuilderEditorTypes.PlacementTypeId)
    typeDropdown:setSize(Vector2F(200.0, 24.0))
    placeWin:addChild(typeDropdown)

    -- Prefab path
    local prefabInput = ui:addElement(labelInputTypeInfo)
    prefabInput:setLabel("Prefab:")
    prefabInput:setElementId(SceneBuilderEditorTypes.PrefabPathId)
    prefabInput:setValue(self.state.prefabPath)
    prefabInput:setSize(Vector2F(300.0, 24.0))
    placeWin:addChild(prefabInput)

    -- Brush size
    local brushSlider = ui:addElement(sliderTypeInfo)
    brushSlider:setLabel("Brush:")
    brushSlider:setElementId(SceneBuilderEditorTypes.BrushSizeId)
    brushSlider:setMinValue(0.5)
    brushSlider:setMaxValue(20.0)
    brushSlider:setValue(self.state.brushSize)
    brushSlider:setSize(Vector2F(160.0, 24.0))
    placeWin:addChild(brushSlider)

    -- -------------------------------------------------------
    -- 3. Procedural world panel
    -- -------------------------------------------------------
    local worldWin = ui:addElement(ui.IUIWindow.typeInfo())
    worldWin:setLabel("Procedural World")
    worldWin:setSize(Vector2F(260.0, 300.0))
    worldWin:setPosition(Vector2F(290.0, 90))
    parentWin:addChild(worldWin)

    local function addField(parent, label, id, defaultVal, row)
        local inp = ui:addElement(labelInputTypeInfo)
        inp:setLabel(label)
        inp:setElementId(id)
        inp:setValue(tostring(defaultVal))
        inp:setSize(Vector2F(240.0, 22.0))
        parent:addChild(inp)
        return inp
    end

    addField(worldWin, "Seed:",       SceneBuilderEditorTypes.WorldSeedId,       self.state.worldSeed)
    addField(worldWin, "Size:",      SceneBuilderEditorTypes.WorldSizeId,       self.state.worldSize)
    addField(worldWin, "Sea Level:",  SceneBuilderEditorTypes.SeaLevelId,        self.state.seaLevel)
    addField(worldWin, "Height:",    SceneBuilderEditorTypes.HeightScaleId,     self.state.heightScale)
    addField(worldWin, "Biome:",      SceneBuilderEditorTypes.BiomeParamsId,     self.state.biomeParams)

    local genWorldBtn = ui:addElement(btnTypeInfo)
    genWorldBtn:setElementId(SceneBuilderEditorTypes.GenerateWorldBtnId)
    genWorldBtn:setText("Generate World")
    genWorldBtn:setSize(Vector2F(240.0, 26.0))
    worldWin:addChild(genWorldBtn)

    -- -------------------------------------------------------
    -- 4. Terrain tools panel
    -- -------------------------------------------------------
    local terrainWin = ui:addElement(ui.IUIWindow.typeInfo())
    terrainWin:setLabel("Terrain Tools")
    terrainWin:setSize(Vector2F(260.0, 160.0))
    terrainWin:setPosition(Vector2F(290.0, 400))
    parentWin:addChild(terrainWin)

    local terrainBrushDropdown = ui:addElement(dropdownTypeInfo)
    terrainBrushDropdown:setLabel("Brush:")
    terrainBrushDropdown:setElementId(SceneBuilderEditorTypes.TerrainBrushId)
    terrainBrushDropdown:setSize(Vector2F(240.0, 24.0))
    terrainWin:addChild(terrainBrushDropdown)

    local terrainStrengthSlider = ui:addElement(sliderTypeInfo)
    terrainStrengthSlider:setLabel("Strength:")
    terrainStrengthSlider:setElementId(SceneBuilderEditorTypes.BrushStrengthId)
    terrainStrengthSlider:setMinValue(0.01)
    terrainStrengthSlider:setMaxValue(1.0)
    terrainStrengthSlider:setValue(self.state.brushStrength)
    terrainStrengthSlider:setSize(Vector2F(240.0, 24.0))
    terrainWin:addChild(terrainStrengthSlider)

    -- -------------------------------------------------------
    -- 5. Road network panel
    -- -------------------------------------------------------
    local roadWin = ui:addElement(ui.IUIWindow.typeInfo())
    roadWin:setLabel("Road Network")
    roadWin:setSize(Vector2F(260.0, 160.0))
    roadWin:setPosition(Vector2F(560.0, 90))
    parentWin:addChild(roadWin)

    addField(roadWin, "Road Type:",    SceneBuilderEditorTypes.RoadTypeId,    self.state.roadType)
    addField(roadWin, "Seed:",        SceneBuilderEditorTypes.RoadSeedId,    self.state.roadSeed)
    addField(roadWin, "Block Size:",  SceneBuilderEditorTypes.BlockSizeId,   self.state.blockSize)
    addField(roadWin, "Road Width:",   SceneBuilderEditorTypes.RoadWidthId,   self.state.roadWidth)

    local buildRoadsBtn = ui:addElement(btnTypeInfo)
    buildRoadsBtn:setElementId(SceneBuilderEditorTypes.BuildRoadsBtnId)
    buildRoadsBtn:setText("Build Roads")
    buildRoadsBtn:setSize(Vector2F(240.0, 26.0))
    roadWin:addChild(buildRoadsBtn)

    -- -------------------------------------------------------
    -- 6. City generator panel
    -- -------------------------------------------------------
    local cityWin = ui:addElement(ui.IUIWindow.typeInfo())
    cityWin:setLabel("City Generator")
    cityWin:setSize(Vector2F(260.0, 200.0))
    cityWin:setPosition(Vector2F(560.0, 260))
    parentWin:addChild(cityWin)

    addField(cityWin, "Radius:",      SceneBuilderEditorTypes.CityRadiusId,   self.state.cityRadius)
    addField(cityWin, "Density:",    SceneBuilderEditorTypes.CityDensityId,  self.state.cityDensity)
    addField(cityWin, "Bldg Rule:",  SceneBuilderEditorTypes.BuildingRuleId,  self.state.buildingRule)

    local parkToggle = ui:addElement(toggleTypeInfo)
    parkToggle:setLabel("Parks:")
    parkToggle:setElementId(SceneBuilderEditorTypes.ParkPlacementId)
    parkToggle:setChecked(self.state.parkPlacement)
    cityWin:addChild(parkToggle)

    local genCityBtn = ui:addElement(btnTypeInfo)
    genCityBtn:setElementId(SceneBuilderEditorTypes.GenerateCityBtnId)
    genCityBtn:setText("Generate City")
    genCityBtn:setSize(Vector2F(240.0, 26.0))
    cityWin:addChild(genCityBtn)

    -- -------------------------------------------------------
    -- 7. Prefab panel
    -- -------------------------------------------------------
    local prefabWin = ui:addElement(ui.IUIWindow.typeInfo())
    prefabWin:setLabel("Prefabs")
    prefabWin:setSize(Vector2F(260.0, 80.0))
    prefabWin:setPosition(Vector2F(560.0, 470))
    parentWin:addChild(prefabWin)

    local createPrefabBtn = ui:addElement(btnTypeInfo)
    createPrefabBtn:setElementId(SceneBuilderEditorTypes.CreatePrefabId)
    createPrefabBtn:setText("Create Prefab")
    createPrefabBtn:setSize(Vector2F(120.0, 24.0))
    prefabWin:addChild(createPrefabBtn)

    local instPrefabBtn = ui:addElement(btnTypeInfo)
    instPrefabBtn:setElementId(SceneBuilderEditorTypes.InstantiatePrefabId)
    instPrefabBtn:setText("Instantiate")
    instPrefabBtn:setSize(Vector2F(120.0, 24.0))
    prefabWin:addChild(instPrefabBtn)

    -- -------------------------------------------------------
    -- 8. Save / load row (bottom)
    -- -------------------------------------------------------
    local ioWin = ui:addElement(ui.IUIWindow.typeInfo())
    ioWin:setLabel("Scene I/O")
    ioWin:setSize(Vector2F(800.0, 36.0))
    ioWin:setPosition(Vector2F(290.0, 700))
    parentWin:addChild(ioWin)

    local saveBtn = ui:addElement(btnTypeInfo)
    saveBtn:setElementId(SceneBuilderEditorTypes.SaveSceneId)
    saveBtn:setText("Save Scene")
    saveBtn:setSize(Vector2F(150.0, 24.0))
    ioWin:addChild(saveBtn)

    local loadBtn = ui:addElement(btnTypeInfo)
    loadBtn:setElementId(SceneBuilderEditorTypes.LoadSceneId)
    loadBtn:setText("Load Scene")
    loadBtn:setSize(Vector2F(150.0, 24.0))
    ioWin:addChild(loadBtn)

    local exportBtn = ui:addElement(btnTypeInfo)
    exportBtn:setElementId(SceneBuilderEditorTypes.ExportPrefabId)
    exportBtn:setText("Export Prefab")
    exportBtn:setSize(Vector2F(150.0, 24.0))
    ioWin:addChild(exportBtn)

    -- Status
    self.statusLabel = ui:addElement(ui.IUIText.typeInfo())
    self.statusLabel:setText("Scene Builder ready.")
    self.statusLabel:setElementId(9999)
    ioWin:addChild(self.statusLabel)

    -- Wire event listener
    self.window:addObjectListener(self)

    self:setStatus("Scene Builder loaded.")
end

-- ============================================================================
-- Event handling
-- ============================================================================
function SceneBuilderEditor:handleEvent(parameters, results)
    local elementId = parameters:at(0):asInt()

    if elementId == SceneBuilderEditorTypes.CreateActorId then
        self:createActor()
    elseif elementId == SceneBuilderEditorTypes.DeleteActorId then
        self:deleteSelectedActors()
    elseif elementId == SceneBuilderEditorTypes.DuplicateActorId then
        self:duplicateSelectedActors()
    elseif elementId == SceneBuilderEditorTypes.GenerateWorldBtnId then
        self:generateWorld()
    elseif elementId == SceneBuilderEditorTypes.BuildRoadsBtnId then
        self:buildRoads()
    elseif elementId == SceneBuilderEditorTypes.GenerateCityBtnId then
        self:generateCity()
    elseif elementId == SceneBuilderEditorTypes.CreatePrefabId then
        self:createPrefab()
    elseif elementId == SceneBuilderEditorTypes.InstantiatePrefabId then
        self:instantiatePrefab()
    elseif elementId == SceneBuilderEditorTypes.SaveSceneId then
        self:saveScene()
    elseif elementId == SceneBuilderEditorTypes.LoadSceneId then
        self:loadScene()
    elseif elementId == SceneBuilderEditorTypes.ExportPrefabId then
        self:exportPrefab()
    end

    return results
end

-- ============================================================================
-- Scene helpers
-- ============================================================================
function SceneBuilderEditor:getBindings()
    local appMgr = IApplicationManager.instance()
    if not appMgr then return nil end
    local factory = appMgr:getFactoryManager()
    if not factory then return nil end
    return factory:make_ptr("SceneBuilderBindings")
end

function SceneBuilderEditor:getScene()
    if self.scene then return self.scene end
    local bindings = self:getBindings()
    if not bindings then return nil end
    self.scene = bindings:getCurrentScene()
    if not self.scene then
        self.scene = bindings:createScene("UntitledScene")
    end
    return self.scene
end

function SceneBuilderEditor:refreshHierarchy()
    -- Rebuild the scene tree from the current scene
    if not self.sceneTree then return end
    self.sceneTree:clear()

    local scene = self:getScene()
    if not scene then return end

    -- Get actors from scene — this mirrors the pattern in SceneWindow.cpp
    local actors = scene:getActors()
    if not actors then return end

    for i = 0, actors:size() - 1 do
        local actor = actors:at(i)
        if actor then
            local item = ui.IUITreeCtrlItem()
            item:setText(actor:getName() or ("Actor_" .. i))
            item:setData(i)
            self.sceneTree:addItem(item)
        end
    end

    self:setStatus("Scene hierarchy refreshed (" .. actors:size() .. " actors).")
end

-- ============================================================================
-- Scene operations
-- ============================================================================
function SceneBuilderEditor:createActor()
    local bindings = self:getBindings()
    if not bindings then
        self:setStatus("SceneBuilderBindings unavailable.")
        return
    end

    local scene = self:getScene()
    if not scene then
        self:setStatus("No scene.")
        return
    end

    local actorName = "Actor_" .. (math.random(1000, 9999))
    local actor = bindings:addActor(scene, actorName, "")
    if actor then
        self:refreshHierarchy()
        self:setStatus("Created: " .. actorName)
    else
        self:setStatus("Failed to create actor.")
    end
end

function SceneBuilderEditor:deleteSelectedActors()
    local bindings = self:getBindings()
    if not bindings then return end
    local scene = self:getScene()
    if not scene then return end

    -- Delete all selected actors from the tree
    for _, actorId in ipairs(self.selectedActorIds) do
        local actors = scene:getActors()
        if actors and actorId < actors:size() then
            local actor = actors:at(actorId)
            if actor then
                bindings:removeActor(scene, actor)
            end
        end
    end

    self.selectedActorIds = {}
    self:refreshHierarchy()
    self:setStatus("Selected actors deleted.")
end

function SceneBuilderEditor:duplicateSelectedActors()
    local bindings = self:getBindings()
    if not bindings then return end

    for _, actorId in ipairs(self.selectedActorIds) do
        local scene = self:getScene()
        if scene then
            local actors = scene:getActors()
            if actors and actorId < actors:size() then
                local actor = actors:at(actorId)
                if actor then
                    bindings:duplicateActor(actor)
                end
            end
        end
    end

    self.selectedActorIds = {}
    self:refreshHierarchy()
    self:setStatus("Actors duplicated.")
end

-- ============================================================================
-- Procedural world
-- ============================================================================
function SceneBuilderEditor:generateWorld()
    local bindings = self:getBindings()
    if not bindings then
        self:setStatus("SceneBuilderBindings unavailable.")
        return
    end

    local seed  = safeGetText(self:getFieldCtrl(SceneBuilderEditorTypes.WorldSeedId),   tostring(self.state.worldSeed))
    local size  = safeGetNumber(self:getFieldCtrl(SceneBuilderEditorTypes.WorldSizeId),  self.state.worldSize)
    local sea   = safeGetNumber(self:getFieldCtrl(SceneBuilderEditorTypes.SeaLevelId),  self.state.seaLevel)
    local hScale= safeGetNumber(self:getFieldCtrl(SceneBuilderEditorTypes.HeightScaleId), self.state.heightScale)
    local biome = safeGetText(self:getFieldCtrl(SceneBuilderEditorTypes.BiomeParamsId), self.state.biomeParams)

    bindings:generateWorld(seed, size, sea, hScale, biome)
    self:setStatus("World generation called (seed=" .. seed .. ").")
end

function SceneBuilderEditor:buildRoads()
    local bindings = self:getBindings()
    if not bindings then
        self:setStatus("SceneBuilderBindings unavailable.")
        return
    end

    local seed      = safeGetNumber(self:getFieldCtrl(SceneBuilderEditorTypes.RoadSeedId),   self.state.roadSeed)
    local blockSize = safeGetNumber(self:getFieldCtrl(SceneBuilderEditorTypes.BlockSizeId),  self.state.blockSize)
    local roadWidth = safeGetNumber(self:getFieldCtrl(SceneBuilderEditorTypes.RoadWidthId),   self.state.roadWidth)

    bindings:generateCity(seed, blockSize, roadWidth, blockSize, roadWidth)
    self:setStatus("Road network generation called.")
end

function SceneBuilderEditor:generateCity()
    local bindings = self:getBindings()
    if not bindings then
        self:setStatus("SceneBuilderBindings unavailable.")
        return
    end

    local radius  = safeGetNumber(self:getFieldCtrl(SceneBuilderEditorTypes.CityRadiusId),  self.state.cityRadius)
    local density = safeGetNumber(self:getFieldCtrl(SceneBuilderEditorTypes.CityDensityId), self.state.cityDensity)

    bindings:generateCity(radius, radius, density, 100, 8)
    self:setStatus("City generation called (radius=" .. radius .. ").")
end

-- ============================================================================
-- Prefab
-- ============================================================================
function SceneBuilderEditor:createPrefab()
    local bindings = self:getBindings()
    if not bindings then return end
    local scene = self:getScene()
    if not scene then return end

    local actors = scene:getActors()
    if not actors or actors:size() == 0 then
        self:setStatus("Scene is empty.")
        return
    end

    -- Create prefab from the first selected actor, or first actor
    local actorId = self.selectedActorIds[1] or 0
    if actorId < actors:size() then
        local actor = actors:at(actorId)
        if actor then
            local prefab = bindings:createPrefab(actor, "NewPrefab")
            if prefab then
                self:setStatus("Prefab created from: " .. actor:getName())
            end
        end
    end
end

function SceneBuilderEditor:instantiatePrefab()
    local bindings = self:getBindings()
    if not bindings then return end
    local scene = self:getScene()
    if not scene then return end

    -- Would normally browse for a prefab — here we just try to create one
    self:setStatus("Instantiate prefab: browse for a prefab file first.")
end

-- ============================================================================
-- Scene I/O
-- ============================================================================
function SceneBuilderEditor:saveScene()
    local bindings = self:getBindings()
    if not bindings then return end
    local scene = self:getScene()
    if not scene then return end

    local path = "Scenes/EditorScene.scene"
    bindings:saveScene(scene, path)
    self:setStatus("Scene save called: " .. path)
end

function SceneBuilderEditor:loadScene()
    local bindings = self:getBindings()
    if not bindings then return end

    local path = "Scenes/EditorScene.scene"
    local loadedScene = bindings:loadScene(path)
    if loadedScene then
        self.scene = loadedScene
        self:refreshHierarchy()
        self:setStatus("Scene loaded: " .. path)
    else
        self:setStatus("Scene load not yet implemented in bindings.")
    end
end

function SceneBuilderEditor:exportPrefab()
    self:setStatus("Export prefab: right-click a prefab in the hierarchy.")
end

-- ============================================================================
-- Utility
-- ============================================================================
function SceneBuilderEditor:getFieldCtrl(elementId)
    -- Placeholder: in a full implementation, cache controls by elementId at registration time
    return nil
end

-- ============================================================================
-- BaseEditor stubs
-- ============================================================================
function SceneBuilderEditor:unload() end
function SceneBuilderEditor:show()   end
function SceneBuilderEditor:hide()   end

function SceneBuilderEditor:setStatus(msg)
    if self.statusLabel then
        self.statusLabel:setText(msg)
    end
    print("[SceneBuilderEditor] " .. msg)
end

function SceneBuilderEditor:getProperties(parameters) end
function SceneBuilderEditor:setProperties(parameters) end
