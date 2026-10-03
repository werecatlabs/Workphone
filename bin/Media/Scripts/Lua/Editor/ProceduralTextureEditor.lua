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
    for id, node in pairs(self.nodes) do
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
        self.generatedTexture = tex
        self:setStatus("Noise texture generated: " .. res .. "x" .. res)
    else
        self:setStatus("Noise generation failed.")
    end

    return tex
end

function ProceduralTextureEditor:generateColorCorrectNode(node)
    if not node or not self.generatedTexture then return nil end
    local p = node.params

    local appMgr = IApplicationManager.instance()
    local factory = appMgr:getFactoryManager()
    if not factory then return nil end

    local bindings = factory:make_ptr("ProceduralTextureBindings")
    if not bindings then return nil end

    local tex = bindings:colourCorrectTexture(
        self.generatedTexture,
        p.ccBrightness or 0.0,
        p.ccContrast   or 1.0,
        p.ccSaturation or 1.0,
        p.ccBlackPoint or 0.0,
        p.ccWhitePoint or 1.0,
        ""
    )

    if tex then
        self.generatedTexture = tex
        self:setStatus("Colour-corrected.")
    end
    return tex
end

function ProceduralTextureEditor:generateNormalNode(node)
    if not node or not self.generatedTexture then return nil end
    local p = node.params

    local appMgr = IApplicationManager.instance()
    local factory = appMgr:getFactoryManager()
    if not factory then return nil end

    local bindings = factory:make_ptr("ProceduralTextureBindings")
    if not bindings then return nil end

    local tex = bindings:generateNormalMapFromHeight(
        self.generatedTexture,
        p.normalStrength or 1.0,
        p.normalInvertY  or false,
        ""
    )

    if tex then
        self.generatedTexture = tex
        self:setStatus("Normal map generated.")
    end
    return tex
end

function ProceduralTextureEditor:generate()
    -- Find first node and generate chain
    local firstNode = nil
    for id, node in pairs(self.nodes) do
        firstNode = node
        break
    end
    if not firstNode then
        self:setStatus("Add a node first.")
        return
    end

    self.generatedTexture = nil

    if firstNode.type == "noise" then
        self:generateNoiseNode(firstNode)
    elseif firstNode.type == "normal" then
        self:generateNormalNode(firstNode)
    elseif firstNode.type == "colorcorrect" then
        self:generateColorCorrectNode(firstNode)
    else
        self:setStatus("Node type '" .. firstNode.type .. "' generation not yet implemented.")
    end
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
