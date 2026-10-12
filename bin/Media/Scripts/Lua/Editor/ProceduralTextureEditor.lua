--[[
  ProceduralTextureEditor.lua
  A node-graph-style procedural texture editor for Workphone Editor.
  
  Generates: noise textures (perlin/simplex/value/worley/fbm),
             normal maps from height, colour correction,
             and assigns results to material slots.
  
  Driven by: C++ ProceduralTextureBindings (IScriptReceiver).
  ref: Esoterica/Code/Engine/Render/ResourceLoader_RenderTexture.cpp
       (texture generation pipeline)
  
  This is a hot-reloadable Lua editor — edit this file and reload to see changes live.
]]

class 'ProceduralTextureEditor' (BaseEditor)

-- ============================================================================
-- Widget IDs (unique range to avoid clashes with other editors)
-- ============================================================================
ProceduralTextureEditorTypes = {
    -- Output
    OutputFormatId      = 1000,
    OutputResolutionId  = 1001,
    OutputSrgbId       = 1002,
    OutputMipmapsId    = 1003,
    OutputPathId       = 1004,
    SaveTextureButtonId = 1005,
    ApplyToMaterialId   = 1006,
    SaveRecipeId        = 1007,
    LoadRecipeId        = 1008,

    -- Node list
    AddNoiseNodeId     = 2000,
    AddNormalNodeId    = 2001,
    AddColorCorrectId  = 2002,
    AddSplatNodeId     = 2003,
    AddTriplanarNodeId = 2004,
    AddAtlasNodeId     = 2005,
    DeleteNodeId       = 2006,
    NodeListId         = 2007,

    -- Noise node params
    NoiseTypeId         = 2100,  -- 0=perlin, 1=simplex, 2=value, 3=worley, 4=fbm
    NoiseFrequencyId     = 2101,
    NoiseOctavesId       = 2102,
    NoiseLacunarityId    = 2103,
    NoisePersistenceId   = 2104,
    NoiseSeedId          = 2105,
    NoiseTurbulenceId    = 2106,

    -- Normal map params
    NormalStrengthId      = 2200,
    NormalInvertYId       = 2201,
    NormalSourceSlotId    = 2202,  -- drop a texture here

    -- Colour correct params
    CCBrightnessId       = 2300,
    CCContrastId         = 2301,
    CCSaturationId       = 2302,
    CCBlackPointId       = 2303,
    CCWhitePointId       = 2304,

    -- Splat params
    SplatLayersId        = 2310,
    SplatBlendModeId      = 2311,  -- 0=mix, 1=add, 2=multiply

    -- Triplanar params
    TriplanarScaleId      = 2400,
    TriplanarSharpnessId  = 2401,

    -- Atlas params
    AtlasTileCountXId     = 2410,
    AtlasTileCountYId     = 2411,

    -- Preview
    PreviewShapeId      = 3000,
    PreviewEnvId        = 3001,
    PreviewExposureId   = 3002,
    CheckerBgId         = 3003,
    UVGridId            = 3004,

    -- Material binding
    MaterialSlotId      = 4000,  -- dropdown: albedo/normal/metallic/etc.
    MaterialPathId      = 4001,
}

local ProceduralTextureEditorDefaults = {
    outputFormat   = "tga",
    outputResolution = 1024,
    outputSrgb     = false,
    outputMipmaps  = true,
    outputPath     = "None",
    materialPath   = "None",
    materialSlot   = "Albedo",

    noiseType      = 0,       -- perlin
    noiseFrequency = 4.0,
    noiseOctaves   = 6,
    noiseLacunarity = 2.0,
    noisePersistence = 0.5,
    noiseSeed     = 12345,
    noiseTurbulence = false,

    normalStrength = 1.0,
    normalInvertY  = false,

    ccBrightness   = 0.0,
    ccContrast     = 1.0,
    ccSaturation   = 1.0,
    ccBlackPoint   = 0.0,
    ccWhitePoint   = 1.0,

    splatLayers   = 1,
    splatBlendMode = 0,

    triplanarScale   = 1.0,
    triplanarSharpness = 1.0,

    atlasTileCountX = 4,
    atlasTileCountY = 4,

    checkerBg    = true,
    uvGrid       = false,
    previewExposure = 1.0,
}

local function copyDefaults()
    local t = {}
    for k, v in pairs(ProceduralTextureEditorDefaults) do
        t[k] = v
    end
    return t
end

-- ============================================================================
-- State
-- ============================================================================
local pte -- forward

local function safeCall(fn, ...)
    local ok, err = pcall(fn, ...)
    if not ok then
        print("[ProceduralTextureEditor] Error: " .. tostring(err))
    end
    return ok
end

local function safeGetNumber(ctrl, fallback)
    if ctrl == nil then return fallback end
    local v = ctrl:getValue()
    local n = tonumber(v)
    return n or fallback
end

local function safeGetBool(ctrl, fallback)
    if ctrl == nil then return fallback end
    return ctrl:isChecked() == true
end

local function safeGetText(ctrl, fallback)
    if ctrl == nil then return fallback or "" end
    return ctrl:getText() or ""
end

-- ============================================================================
-- Internal node-graph state (kept in Lua; driven by C++)
-- ============================================================================
local nodeGraph = {
    nodes    = {},   -- { id = { type="noise"|"normal"|"colorcorrect"|"splat"|"triplanar"|"atlas", params={...} } }
    connections = {},  -- { {fromNode, fromPin, toNode, toPin} }
    nextId   = 1,
}

-- ============================================================================
-- Class
-- ============================================================================
function ProceduralTextureEditor:__init(window)
    BaseEditor.__init(self, window)
    self.state       = copyDefaults()
    self.nodes       = {}
    self.connections = {}
    self.nextNodeId  = 1
    self.selectedNodeId = nil
    self.generatedTexture = nil
    self.material    = nil
end

function ProceduralTextureEditor:__finalize()
    self.generatedTexture = nil
    self.material = nil
    BaseEditor.__finalize(self)
end

-- ============================================================================
-- load — build the full editor UI
-- ============================================================================
function ProceduralTextureEditor:load()
    BaseEditor.load(self)

    local appMgr   = IApplicationManager.instance()
    local ui       = appMgr:getUI()
    local parentWin = self.window:getParentWindow()
    if not parentWin then return end

    -- Ensure minimum size
    parentWin:setSize(Vector2F(900.0, 700.0))

    -- -------------------------------------------------------
    -- 1. Toolbar row: add-node buttons + output controls
    -- -------------------------------------------------------
    local toolbarWin = ui:addElement(ui.IUIWindow.typeInfo())
    toolbarWin:setLabel("Texture Tools")
    toolbarWin:setSize(Vector2F(880.0, 40.0))
    parentWin:addChild(toolbarWin)

    local toolTypeInfo = ui.IUIButton.typeInfo()
    local function addButton(parent, label, id, tip)
        local btn = ui:addElement(toolTypeInfo)
        btn:setElementId(id)
        btn:setText(label)
        btn:setHelp(tip or "")
        btn:setSize(Vector2F(120.0, 24.0))
        parent:addChild(btn)
        return btn
    end

    addButton(toolbarWin, "Add Noise",    ProceduralTextureEditorTypes.AddNoiseNodeId,     "Add a noise generator node")
    addButton(toolbarWin, "Add Normal",   ProceduralTextureEditorTypes.AddNormalNodeId,    "Add a normal-map-from-height node")
    addButton(toolbarWin, "Add ColorCorr",ProceduralTextureEditorTypes.AddColorCorrectId, "Add a colour-correct node")
    addButton(toolbarWin, "Add Splat",    ProceduralTextureEditorTypes.AddSplatNodeId,    "Add a splatmap node")
    addButton(toolbarWin, "Add Triplanar",ProceduralTextureEditorTypes.AddTriplanarNodeId, "Add a triplanar projection node")
    addButton(toolbarWin, "Add Atlas",    ProceduralTextureEditorTypes.AddAtlasNodeId,     "Add an atlas node")

    -- Output format
    local dropdownTypeInfo = ui.IUILabelDropdownPair.typeInfo()
    local formatDropdown = ui:addElement(dropdownTypeInfo)
    formatDropdown:setLabel("Format:")
    formatDropdown:setElementId(ProceduralTextureEditorTypes.OutputFormatId)
    formatDropdown:setSize(Vector2F(300.0, 24.0))
    toolbarWin:addChild(formatDropdown)

    -- Output resolution
    local resDropdown = ui:addElement(dropdownTypeInfo)
    resDropdown:setLabel("Res:")
    resDropdown:setElementId(ProceduralTextureEditorTypes.OutputResolutionId)
    resDropdown:setSize(Vector2F(200.0, 24.0))
    toolbarWin:addChild(resDropdown)

    -- sRGB toggle
    local toggleTypeInfo = ui.IUILabelTogglePair.typeInfo()
    local srgbToggle = ui:addElement(toggleTypeInfo)
    srgbToggle:setLabel("sRGB:")
    srgbToggle:setElementId(ProceduralTextureEditorTypes.OutputSrgbId)
    srgbToggle:setChecked(self.state.outputSrgb)
    toolbarWin:addChild(srgbToggle)

    addButton(toolbarWin, "Save Recipe", ProceduralTextureEditorTypes.SaveRecipeId, "Save graph settings beside the output texture")
    addButton(toolbarWin, "Load Recipe", ProceduralTextureEditorTypes.LoadRecipeId, "Load graph settings beside the output texture")
    -- Save button
    addButton(toolbarWin, "Save Texture", ProceduralTextureEditorTypes.SaveTextureButtonId, "Save generated texture to disk")
    addButton(toolbarWin, "Apply Material",ProceduralTextureEditorTypes.ApplyToMaterialId, "Apply to material slot")

    -- -------------------------------------------------------
    -- 2. Node list panel (left)
    -- -------------------------------------------------------
    local nodePanel = ui:addElement(ui.IUIWindow.typeInfo())
    nodePanel:setLabel("Nodes")
    nodePanel:setSize(Vector2F(260.0, 500.0))
    parentWin:addChild(nodePanel)

    local treeTypeInfo = ui.IUITreeCtrl.typeInfo()
    self.nodeTree = ui:addElement(treeTypeInfo)
    self.nodeTree:setElementId(ProceduralTextureEditorTypes.NodeListId)
    self.nodeTree:setSize(Vector2F(250.0, 480.0))
    nodePanel:addChild(self.nodeTree)

    -- -------------------------------------------------------
    -- 3. Parameters panel (right)
    -- -------------------------------------------------------
    local paramPanel = ui:addElement(ui.IUIWindow.typeInfo())
    paramPanel:setLabel("Node Parameters")
    paramPanel:setSize(Vector2F(620.0, 500.0))
    parentWin:addChild(paramPanel)

    self.paramWindow = ui:addElement(ui.IUIWindow.typeInfo())
    self.paramWindow:setLabel("Select a node to edit parameters")
    self.paramWindow:setSize(Vector2F(600.0, 480.0))
    paramPanel:addChild(self.paramWindow)

    -- -------------------------------------------------------
    -- 4. Preview panel (bottom)
    -- -------------------------------------------------------
    local previewPanel = ui:addElement(ui.IUIWindow.typeInfo())
    previewPanel:setLabel("Preview")
    previewPanel:setSize(Vector2F(880.0, 120.0))
    parentWin:addChild(previewPanel)

    local labelTypeInfo = ui.IUILabelTextInputPair.typeInfo()
    self.previewExposureSlider = ui:addElement(ui.IUILabelSliderPair.typeInfo())
    self.previewExposureSlider:setLabel("Exposure:")
    self.previewExposureSlider:setElementId(ProceduralTextureEditorTypes.PreviewExposureId)
    self.previewExposureSlider:setMinValue(0.1)
    self.previewExposureSlider:setMaxValue(4.0)
    self.previewExposureSlider:setValue(self.state.previewExposure)
    self.previewExposureSlider:setSize(Vector2F(200.0, 24.0))
    previewPanel:addChild(self.previewExposureSlider)

    -- Checker background toggle
    local checkerToggle = ui:addElement(toggleTypeInfo)
    checkerToggle:setLabel("Checker BG:")
    checkerToggle:setElementId(ProceduralTextureEditorTypes.CheckerBgId)
    checkerToggle:setChecked(self.state.checkerBg)
    previewPanel:addChild(checkerToggle)

    -- UV grid toggle
    local uvToggle = ui:addElement(toggleTypeInfo)
    uvToggle:setLabel("UV Grid:")
    uvToggle:setElementId(ProceduralTextureEditorTypes.UVGridId)
    uvToggle:setChecked(self.state.uvGrid)
    previewPanel:addChild(uvToggle)

    -- Status label
    self.statusLabel = ui:addElement(ui.IUIText.typeInfo())
    self.statusLabel:setText("Procedural Texture Editor ready.")
    self.statusLabel:setElementId(8000)
    previewPanel:addChild(self.statusLabel)

    -- -------------------------------------------------------
    -- 5. Material binding row
    -- -------------------------------------------------------
    local matPanel = ui:addElement(ui.IUIWindow.typeInfo())
    matPanel:setLabel("Material Output")
    matPanel:setSize(Vector2F(880.0, 40.0))
    parentWin:addChild(matPanel)

    local matSlotDropdown = ui:addElement(dropdownTypeInfo)
    matSlotDropdown:setLabel("Slot:")
    matSlotDropdown:setElementId(ProceduralTextureEditorTypes.MaterialSlotId)
    matSlotDropdown:setSize(Vector2F(200.0, 24.0))
    matPanel:addChild(matSlotDropdown)

    local matPathInput = ui:addElement(labelTypeInfo)
    matPathInput:setLabel("Material:")
    matPathInput:setElementId(ProceduralTextureEditorTypes.MaterialPathId)
    matPathInput:setSize(Vector2F(500.0, 24.0))
    matPathInput:setValue(self.state.materialPath)
    matPanel:addChild(matPathInput)

    -- Wire up event handler
    self.window:addObjectListener(self)

    self:setStatus("Procedural Texture Editor loaded.")
end

-- ============================================================================
-- Event handling
-- ============================================================================
function ProceduralTextureEditor:handleEvent(parameters, results)
    local elementId = parameters:at(0):asInt()

    if elementId == ProceduralTextureEditorTypes.AddNoiseNodeId then
        self:addNode("noise")
    elseif elementId == ProceduralTextureEditorTypes.AddNormalNodeId then
        self:addNode("normal")
    elseif elementId == ProceduralTextureEditorTypes.AddColorCorrectId then
        self:addNode("colorcorrect")
    elseif elementId == ProceduralTextureEditorTypes.AddSplatNodeId then
        self:addNode("splat")
    elseif elementId == ProceduralTextureEditorTypes.AddTriplanarNodeId then
        self:addNode("triplanar")
    elseif elementId == ProceduralTextureEditorTypes.AddAtlasNodeId then
        self:addNode("atlas")
    elseif elementId == ProceduralTextureEditorTypes.SaveRecipeId or elementId == ProceduralTextureEditorTypes.LoadRecipeId then
        local path = safeGetText(self.outputPathCtrl, self.state.outputPath)
        if not path or path == "" or path == "None" then
            self:setStatus("Set an output path first.")
        else
            local ok
            if elementId == ProceduralTextureEditorTypes.SaveRecipeId then ok = self:saveRecipe(path .. ".recipe.json")
            else ok = self:loadRecipe(path .. ".recipe.json") end
            self:setStatus(ok and "Recipe operation completed." or "Recipe operation failed; current graph retained.")
        end
    elseif elementId == ProceduralTextureEditorTypes.SaveTextureButtonId then
        self:saveTexture()
    elseif elementId == ProceduralTextureEditorTypes.ApplyToMaterialId then
        self:applyToMaterial()
    elseif elementId == ProceduralTextureEditorTypes.DeleteNodeId then
        self:deleteSelectedNode()
    elseif elementId == ProceduralTextureEditorTypes.NodeListId then
        -- selection changed — update param panel
        self:onNodeSelected(self:getSelectedNodeIdFromTree())
    end

    return results
end

-- ============================================================================
-- Node management
-- ============================================================================
function ProceduralTextureEditor:addNode(nodeType)
    local id = self.nextNodeId
    self.nextNodeId = self.nextNodeId + 1

    local node = {
        id    = id,
        type  = nodeType,
        label = nodeType .. "_" .. id,
        params = copyDefaults(),
    }

    self.nodes[id] = node

    -- Add to tree
    if self.nodeTree then
        local item = ui.IUITreeCtrlItem()
        item:setText(node.label)
        item:setData(id)
        self.nodeTree:addItem(item)
    end

    self:setStatus("Added node: " .. node.label)
    return id
end

function ProceduralTextureEditor:deleteSelectedNode()
    local selId = self.selectedNodeId
    if not selId or not self.nodes[selId] then
        self:setStatus("No node selected to delete.")
        return
    end

    self.nodes[selId] = nil
    self.selectedNodeId = nil
    self:rebuildNodeTree()
    self:clearParamPanel()
    self:setStatus("Node deleted.")
end

function ProceduralTextureEditor:getSelectedNodeIdFromTree()
    if not self.nodeTree then return nil end
    local sel = self.nodeTree:getSelectedItem()
    if not sel then return nil end
    return sel:getData()
end

function ProceduralTextureEditor:onNodeSelected(nodeId)
    self.selectedNodeId = nodeId
    if nodeId then
        self:buildParamPanelForNode(self.nodes[nodeId])
    else
        self:clearParamPanel()
    end
end

function ProceduralTextureEditor:rebuildNodeTree()
    if not self.nodeTree then return end
    self.nodeTree:clear()
    for _, id in ipairs(self:orderedNodeIds()) do
        local node = self.nodes[id]
        local item = ui.IUITreeCtrlItem()
        item:setText(node.label)
        item:setData(id)
        self.nodeTree:addItem(item)
    end
end

-- ============================================================================
-- Parameter panel
-- ============================================================================
function ProceduralTextureEditor:clearParamPanel()
    if not self.paramWindow then return end
    -- Remove all children (best-effort)
    self.paramWindow:clear()
end

function ProceduralTextureEditor:buildParamPanelForNode(node)
    if not node then return end
    self:clearParamPanel()

    local appMgr = IApplicationManager.instance()
    local ui     = appMgr:getUI()

    local label
    local x = 10.0
    local y = 10.0
    local rowH = 28.0
    local colW = 180.0

    local function nextRow()
        y = y + rowH
        if y > 450.0 then
            x = x + colW + 10.0
            y = 10.0
        end
    end

    local function addLabel(text)
        local lbl = ui:addElement(ui.IUIText.typeInfo())
        lbl:setText(text)
        lbl:setPosition(Vector2F(x, y))
        lbl:setSize(Vector2F(colW, 20.0))
        self.paramWindow:addChild(lbl)
    end

    local function addSlider(id, labelText, minV, maxV, key)
        addLabel(labelText)
        local slider = ui:addElement(ui.IUILabelSliderPair.typeInfo())
        slider:setLabel("")
        slider:setElementId(id)
        slider:setMinValue(minV)
        slider:setMaxValue(maxV)
        slider:setValue(node.params[key])
        slider:setPosition(Vector2F(x + 80.0, y))
        slider:setSize(Vector2F(150.0, 24.0))
        self.paramWindow:addChild(slider)
        nextRow()
    end

    local function addIntSlider(id, labelText, minV, maxV, key)
        addLabel(labelText)
        local slider = ui:addElement(ui.IUILabelSliderPair.typeInfo())
        slider:setLabel("")
        slider:setElementId(id)
        slider:setMinValue(minV)
        slider:setMaxValue(maxV)
        slider:setValue(node.params[key])
        slider:setPosition(Vector2F(x + 80.0, y))
        slider:setSize(Vector2F(150.0, 24.0))
        self.paramWindow:addChild(slider)
        nextRow()
    end

    local function addToggle(id, labelText, key)
        addLabel(labelText)
        local tgl = ui:addElement(ui.IUILabelTogglePair.typeInfo())
        tgl:setLabel("")
        tgl:setElementId(id)
        tgl:setChecked(node.params[key])
        tgl:setPosition(Vector2F(x + 80.0, y))
        tgl:setSize(Vector2F(80.0, 24.0))
        self.paramWindow:addChild(tgl)
        nextRow()
    end

    if node.type == "noise" then
        addLabel("Noise type: 0=perlin, 1=simplex, 2=value, 3=worley, 4=fbm")
        nextRow()
        addIntSlider(ProceduralTextureEditorTypes.NoiseTypeId,        "Type",       0, 4, "noiseType")
        addSlider (ProceduralTextureEditorTypes.NoiseFrequencyId,     "Frequency",  0.1, 32.0, "noiseFrequency")
        addIntSlider(ProceduralTextureEditorTypes.NoiseOctavesId,       "Octaves",   1, 12,  "noiseOctaves")
        addSlider (ProceduralTextureEditorTypes.NoiseLacunarityId,    "Lacunarity", 1.1, 4.0, "noiseLacunarity")
        addSlider (ProceduralTextureEditorTypes.NoisePersistenceId,    "Persistence", 0.1, 1.0, "noisePersistence")
        addIntSlider(ProceduralTextureEditorTypes.NoiseSeedId,         "Seed",       0, 99999, "noiseSeed")
        addToggle (ProceduralTextureEditorTypes.NoiseTurbulenceId,     "Turbulence",  "noiseTurbulence")

    elseif node.type == "normal" then
        addSlider (ProceduralTextureEditorTypes.NormalStrengthId,     "Strength",   0.1, 10.0, "normalStrength")
        addToggle (ProceduralTextureEditorTypes.NormalInvertYId,       "Invert Y",   "normalInvertY")

    elseif node.type == "colorcorrect" then
        addSlider (ProceduralTextureEditorTypes.CCBrightnessId,        "Brightness", -1.0, 1.0, "ccBrightness")
        addSlider (ProceduralTextureEditorTypes.CCContrastId,          "Contrast",   0.1, 3.0,  "ccContrast")
        addSlider (ProceduralTextureEditorTypes.CCSaturationId,        "Saturation", 0.0, 3.0,  "ccSaturation")
        addSlider (ProceduralTextureEditorTypes.CCBlackPointId,        "Black Pt",   0.0, 1.0,  "ccBlackPoint")
        addSlider (ProceduralTextureEditorTypes.CCWhitePointId,        "White Pt",   0.0, 1.0,  "ccWhitePoint")

    elseif node.type == "splat" then
        addIntSlider(ProceduralTextureEditorTypes.SplatLayersId,       "Layers",     1, 8, "splatLayers")

    elseif node.type == "triplanar" then
        addSlider (ProceduralTextureEditorTypes.TriplanarScaleId,      "Scale",      0.1, 10.0, "triplanarScale")
        addSlider (ProceduralTextureEditorTypes.TriplanarSharpnessId,  "Sharpness",  1.0, 16.0, "triplanarSharpness")

    elseif node.type == "atlas" then
        addIntSlider(ProceduralTextureEditorTypes.AtlasTileCountXId,  "Tiles X",   1, 16, "atlasTileCountX")
        addIntSlider(ProceduralTextureEditorTypes.AtlasTileCountYId,   "Tiles Y",    1, 16, "atlasTileCountY")
    end

    -- Generate button
    local genBtn = ui:addElement(ui.IUIButton.typeInfo())
    genBtn:setElementId(9000)  -- local generate trigger
    genBtn:setText("Generate")
    genBtn:setPosition(Vector2F(x, y + 10.0))
    genBtn:setSize(Vector2F(200.0, 28.0))
    self.paramWindow:addChild(genBtn)
end

-- ============================================================================
-- Texture generation (calls C++ via ScriptInvoker -> ProceduralTextureBindings)
-- ============================================================================
function ProceduralTextureEditor:getBindings()
    -- Get the bindings object from the factory
    local appMgr = IApplicationManager.instance()
    local factory = appMgr:getFactoryManager()
    if not factory then return nil end
    -- The bindings are created as a shared object registered with the factory.
    -- We look them up by type.
    return factory:make_ptr(procedural_bindings.ProcduralTextureBindings)
end

function ProceduralTextureEditor:generateNoiseNode(node)
    if not node then return nil end
    local p = node.params

    local appMgr = IApplicationManager.instance()
    local factory = appMgr:getFactoryManager()
    if not factory then
        self:setStatus("No factory available for generation.")
        return nil
    end

    -- Create bindings (this pattern mirrors SetupMaterialJob.cpp)
    local bindings = factory:make_ptr("ProceduralTextureBindings")
    if not bindings then
        self:setStatus("Could not create ProceduralTextureBindings.")
        return nil
    end

    local res = self.state.outputResolution
    local tex = bindings:generateNoiseTexture(
        res, res,
        p.noiseType or 0,
        p.noiseFrequency or 4.0,
        p.noiseOctaves  or 6,
        p.noiseLacunarity  or 2.0,
        p.noisePersistence or 0.5,
        p.noiseSeed     or 12345,
        p.noiseTurbulence or false,
        ""
    )

    if tex then
        self:setStatus("Noise texture generated: " .. res .. "x" .. res)
    else
        self:setStatus("Noise generation failed.")
    end

    return tex
end

function ProceduralTextureEditor:generateColorCorrectNode(node, source)
    if not node or not source then return nil end
    local p = node.params

    local appMgr = IApplicationManager.instance()
    local factory = appMgr:getFactoryManager()
    if not factory then return nil end

    local bindings = factory:make_ptr("ProceduralTextureBindings")
    if not bindings then return nil end

    local tex = bindings:colourCorrectTexture(
        source,
        p.ccBrightness or 0.0,
        p.ccContrast   or 1.0,
        p.ccSaturation or 1.0,
        p.ccBlackPoint or 0.0,
        p.ccWhitePoint or 1.0,
        ""
    )

    if tex then
        self:setStatus("Colour-corrected.")
    end
    return tex
end

function ProceduralTextureEditor:generateNormalNode(node, source)
    if not node or not source then return nil end
    local p = node.params

    local appMgr = IApplicationManager.instance()
    local factory = appMgr:getFactoryManager()
    if not factory then return nil end

    local bindings = factory:make_ptr("ProceduralTextureBindings")
    if not bindings then return nil end

    local tex = bindings:generateNormalMapFromHeight(
        source,
        p.normalStrength or 1.0,
        p.normalInvertY  or false,
        ""
    )

    if tex then
        self:setStatus("Normal map generated.")
    end
    return tex
end

function ProceduralTextureEditor:orderedNodeIds()
    local ids = {}
    for id in pairs(self.nodes) do ids[#ids + 1] = id end
    table.sort(ids)
    return ids
end

function ProceduralTextureEditor:generate()
    local ids = self:orderedNodeIds()
    if #ids == 0 then self:setStatus("Add a noise node first."); return false end
    -- This editor currently represents a linear chain in creation order.
    -- Reject unsupported nodes before invoking any generation or changing output.
    for index, id in ipairs(ids) do
        local kind = self.nodes[id].type
        if (index == 1 and kind ~= "noise") or
           (index > 1 and kind ~= "normal" and kind ~= "colorcorrect") then
            self:setStatus("Use one noise source followed by normal/colour correction nodes.")
            return false
        end
    end
    local ok, candidate = pcall(function()
        local texture = self:generateNoiseNode(self.nodes[ids[1]])
        if not texture then error("Noise source failed") end
        for index = 2, #ids do
            local node = self.nodes[ids[index]]
            if node.type == "normal" then texture = self:generateNormalNode(node, texture)
            else texture = self:generateColorCorrectNode(node, texture) end
            if not texture then error("Node " .. tostring(node.id) .. " failed") end
        end
        return texture
    end)
    if not ok then
        self:setStatus("Generation failed; previous output retained: " .. tostring(candidate))
        return false
    end
    self.generatedTexture = candidate
    self:setStatus("Generated " .. #ids .. " nodes in creation order.")
    return true
end

-- Versioned plain-data recipes; no runtime texture pointers or executable Lua.
function ProceduralTextureEditor:exportRecipe()
    local recipe = {version = 1, state = {}, nodes = {}}
    for key in pairs(ProceduralTextureEditorDefaults) do recipe.state[key] = self.state[key] end
    for _, id in ipairs(self:orderedNodeIds()) do
        local node, params = self.nodes[id], {}
        for key in pairs(ProceduralTextureEditorDefaults) do params[key] = node.params[key] end
        recipe.nodes[#recipe.nodes + 1] = {id = id, type = node.type, params = params}
    end
    return recipe
end

function ProceduralTextureEditor:importRecipe(recipe)
    if type(recipe) ~= "table" or recipe.version ~= 1 or type(recipe.nodes) ~= "table" or
       type(recipe.state) ~= "table" or #recipe.nodes > 256 then return false end
    local entries = 0
    for index in pairs(recipe.nodes) do
        if type(index) ~= "number" or index < 1 or index % 1 ~= 0 or index > #recipe.nodes then return false end
        entries = entries + 1
    end
    if entries ~= #recipe.nodes then return false end
    local function validatedSettings(settings)
        if type(settings) ~= "table" then return nil end
        local result = copyDefaults()
        for key, default in pairs(ProceduralTextureEditorDefaults) do
            local value = settings[key]
            if value ~= nil then
                if type(value) ~= type(default) then return nil end
                if type(value) == "number" and (value ~= value or math.abs(value) == math.huge) then return nil end
                result[key] = value
            end
        end
        local r = result.outputResolution
        if r < 1 or r > 8192 or r % 1 ~= 0 then return nil end
        if result.noiseOctaves < 1 or result.noiseOctaves > 32 or result.noiseOctaves % 1 ~= 0 then return nil end
        return result
    end
    local state, nodes, maximum = validatedSettings(recipe.state), {}, 0
    if not state then return false end
    for _, node in ipairs(recipe.nodes) do
        if type(node) ~= "table" or type(node.id) ~= "number" or node.id < 1 or
           node.id > 1000000 or node.id % 1 ~= 0 or nodes[node.id] then return false end
        if node.type ~= "noise" and node.type ~= "normal" and node.type ~= "colorcorrect" then return false end
        local params = validatedSettings(node.params)
        if not params then return false end
        nodes[node.id] = {id = node.id, type = node.type, label = node.type .. "_" .. node.id, params = params}
        maximum = math.max(maximum, node.id)
    end
    self.state, self.nodes, self.nextNodeId = state, nodes, maximum + 1
    self.selectedNodeId = nil
    self:rebuildNodeTree()
    self:clearParamPanel()
    self:setStatus("Recipe loaded; generate to replace the current output.")
    return true
end

function ProceduralTextureEditor:saveRecipe(path)
    if type(path) ~= "string" or path == "" then return false end
    local ok, json = pcall(require, "cjson")
    if not ok then self:setStatus("JSON codec unavailable."); return false end
    local encoded, contents = pcall(json.encode, self:exportRecipe())
    if not encoded then return false end
    local file = io.open(path, "wb")
    if not file then return false end
    local written = file:write(contents)
    local closed = file:close()
    return written ~= nil and closed ~= nil
end

function ProceduralTextureEditor:loadRecipe(path)
    if type(path) ~= "string" or path == "" then return false end
    local file = io.open(path, "rb")
    if not file then return false end
    local contents = file:read(4 * 1024 * 1024 + 1)
    file:close()
    if not contents or #contents > 4 * 1024 * 1024 then return false end
    local ok, json = pcall(require, "cjson")
    if not ok then return false end
    local decoded, recipe = pcall(json.decode, contents)
    return decoded and self:importRecipe(recipe)
end

-- ============================================================================
-- Persistence
-- ============================================================================
function ProceduralTextureEditor:saveTexture()
    if not self.generatedTexture then
        self:setStatus("Generate a texture first.")
        return
    end

    local path = safeGetText(self.outputPathCtrl, self.state.outputPath)
    if not path or path == "None" or path == "" then
        self:setStatus("Set an output path first.")
        return
    end

    local appMgr = IApplicationManager.instance()
    local factory = appMgr:getFactoryManager()
    if not factory then return end

    local bindings = factory:make_ptr("ProceduralTextureBindings")
    if not bindings then return end

    local ok = bindings:saveTextureToFile(self.generatedTexture, path, self.state.outputFormat)
    if ok then
        self:setStatus("Texture saved: " .. path)
    else
        self:setStatus("Save failed.")
    end
end

function ProceduralTextureEditor:applyToMaterial()
    if not self.generatedTexture then
        self:setStatus("Generate a texture first.")
        return
    end

    local appMgr = IApplicationManager.instance()
    local factory = appMgr:getFactoryManager()
    if not factory then return end

    local matPath = safeGetText(self.materialPathCtrl, self.state.materialPath)
    if not matPath or matPath == "None" or matPath == "" then
        self:setStatus("Set a material path first.")
        return
    end

    local bindings = factory:make_ptr("ProceduralTextureBindings")
    if not bindings then return end

    local mat = bindings:createOrGetMaterial(matPath)
    if not mat then
        self:setStatus("Could not find or create material: " .. matPath)
        return
    end

    local slotName = self.state.materialSlot or "Albedo"
    bindings:assignTextureToMaterialSlot(mat, slotName, self.generatedTexture)
    self:setStatus("Applied to " .. matPath .. "." .. slotName)
end

-- ============================================================================
-- BaseEditor stubs
-- ============================================================================
function ProceduralTextureEditor:unload() end
function ProceduralTextureEditor:show()   end
function ProceduralTextureEditor:hide()   end

function ProceduralTextureEditor:setStatus(msg)
    if self.statusLabel then
        self.statusLabel:setText(msg)
    end
    print("[ProceduralTextureEditor] " .. msg)
end

function ProceduralTextureEditor:getProperties(parameters)
    -- Not used in this editor
end

function ProceduralTextureEditor:setProperties(parameters)
    -- Not used in this editor
end
