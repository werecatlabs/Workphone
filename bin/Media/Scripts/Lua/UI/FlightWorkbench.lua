include("FlightDialog.lua")

class 'ModelEditor' (FlightDialog)

function ModelEditor:__init(component)
	FlightDialog.__init(self, component)
	self.propertiesText = nil
	self.angularLimitsText = nil
	self.selectedObjectText = nil
	self.selectedObject = nil
end
function ModelEditor:setSelectedObject(value) self.selectedObject = value; return self:textUpdate() end
function ModelEditor:textUpdate()
	local object = self.selectedObject
	local name = self:read(object, { "getName", "GetName" }, { "name" }, "Nothing selected")
	local properties = self:read(object, { "getPropertiesText", "GetPropertiesText" }, { "propertiesText" }, "")
	local limits = self:read(object, { "getAngularLimitsText", "GetAngularLimitsText" }, { "angularLimitsText" }, "")
	self:setControlText(self.selectedObjectText, name)
	self:setControlText(self.propertiesText, properties)
	self:setControlText(self.angularLimitsText, limits)
	return true
end
function ModelEditor:onClickReload() local called = self:call(self:getModelManager(), { "reloadCurrentModel", "ReloadCurrentModel", "reloadModel", "ReloadModel" }); if called then self:textUpdate() end; return called end
function ModelEditor:onClickUpdate() local called = self:call(self:getModelManager(), { "updateCurrentModel", "UpdateCurrentModel", "applyModelChanges", "ApplyModelChanges" }, self.selectedObject); if called then self:textUpdate() end; return called end
ModelEditor.TextUpdate = ModelEditor.textUpdate
ModelEditor.OnClickReload = ModelEditor.onClickReload
ModelEditor.OnClickUpdate = ModelEditor.onClickUpdate

class 'ComponentSelector' (FlightDialog)

function ComponentSelector:__init(component)
	FlightDialog.__init(self, component)
	self.modelType = "unknown"
	self.componentItems = {}
	self.selectedComponent = nil
	self.showVirtualTx = false
	self.showSkeleton = false
	self.uploadPanel = nil
end
function ComponentSelector:refresh()
	local called, items = self:call(self:getModelManager(), { "getAvailableComponents", "GetAvailableComponents", "getModelComponents", "GetModelComponents" }, self.modelType)
	if called then self.componentItems = self:list(items) end
	self:emit("componentsChanged", self.componentItems); return self.componentItems
end
function ComponentSelector:setupFromConfiguredModelData(data) self.modelType = tostring(self:read(data, { "getModelType", "GetModelType" }, { "modelType" }, self.modelType)); return self:refresh() end
function ComponentSelector:setupFromConfiguredModelDataString(data) local called, decoded = self:call(self:getApplicationManager(), { "decodeJson", "DecodeJson" }, data); return called and self:setupFromConfiguredModelData(decoded) or false end
function ComponentSelector:setupTestComponents(value) self.componentItems = self:list(value); self:emit("componentsChanged", self.componentItems); return true end
function ComponentSelector:selectComponent(item) self.selectedComponent = item; self:call(self:getUIManager(), { "clickComponentItem", "ClickComponentItem" }, item); self:emit("selected", item); return item ~= nil end
function ComponentSelector:toggleShowVirtualTx(value) self.showVirtualTx = value == nil and not self.showVirtualTx or value == true; self:call(self:getUIStateManager(), { "setVisualTxVisible", "SetVisualTxVisible" }, self.showVirtualTx); return self.showVirtualTx end
function ComponentSelector:toggleSkeletonView(value) self.showSkeleton = value == nil and not self.showSkeleton or value == true; self:call(self:getModelManager(), { "setSkeletonVisible", "SetSkeletonVisible" }, self.showSkeleton); return self.showSkeleton end
function ComponentSelector:clickFly() return self:call(self:getLoadingManager(), { "goToScene", "GoToScene", "loadCurrentScene", "LoadCurrentScene" }) end
function ComponentSelector:clickReset() self.selectedComponent = nil; return self:refresh() end
function ComponentSelector:onModelLoaded() return self:refresh() end
function ComponentSelector:onModelComponentChanged() return self:refresh() end
function ComponentSelector:onShowPanel() self:show(true); return self:refresh() end
function ComponentSelector:onHidePanel() return self:hide(true) end
function ComponentSelector:onScriptsReloaded() return self:refresh() end
function ComponentSelector:onClickUpload() self:setControlVisible(self.uploadPanel, true); return true end
function ComponentSelector:onClickCloseUploadPanel() self:setControlVisible(self.uploadPanel, false); return true end

ComponentSelector.OnDestroy = FlightDialog.onDestroy
ComponentSelector.Update = FlightDialog.update
ComponentSelector.SetupFromConfiguredModelDataString = ComponentSelector.setupFromConfiguredModelDataString
ComponentSelector.SetupFromConfiguredModelData = ComponentSelector.setupFromConfiguredModelData
ComponentSelector.Refresh = ComponentSelector.refresh
ComponentSelector.SetupTestComponents = ComponentSelector.setupTestComponents
ComponentSelector.ToggleShowVirtualTx = ComponentSelector.toggleShowVirtualTx
ComponentSelector.ToggleSkeletonView = ComponentSelector.toggleSkeletonView
ComponentSelector.ClickFly = ComponentSelector.clickFly
ComponentSelector.ClickReset = ComponentSelector.clickReset
ComponentSelector.OnModelLoaded = ComponentSelector.onModelLoaded
ComponentSelector.OnModelComponentChanged = ComponentSelector.onModelComponentChanged
ComponentSelector.OnShowPanel = ComponentSelector.onShowPanel
ComponentSelector.OnHidePanel = ComponentSelector.onHidePanel
ComponentSelector.OnScriptsReloaded = ComponentSelector.onScriptsReloaded
ComponentSelector.OnClickUpload = ComponentSelector.onClickUpload
ComponentSelector.OnClickCloseUploadPanel = ComponentSelector.onClickCloseUploadPanel

class 'ComponentConfig' (FlightDialog)

function ComponentConfig:__init(component)
	FlightDialog.__init(self, component)
	self.componentType = 0
	self.selectedComponent = nil
	self.originalValues = {}
	self.pendingValues = {}
	self.activePanel = "default"
end
function ComponentConfig:setSelectedComponent(value)
	self.selectedComponent = value
	local values = self:read(value, { "getConfiguration", "GetConfiguration" }, { "configuration", "values" }, {})
	self.originalValues, self.pendingValues = {}, {}
	if type(values) == "table" then for key, item in pairs(values) do self.originalValues[key] = item; self.pendingValues[key] = item end end
	return self:updateActivePanel()
end
function ComponentConfig:updateActivePanel() self.activePanel = tostring(self.componentType or "default"); self:emit("panelChanged", self.activePanel); return self.activePanel end
function ComponentConfig:setPendingValue(name, value) self.pendingValues[name] = value; self.dirty = true; return value end
function ComponentConfig:clickApply()
	if not self.selectedComponent then return false end
	local applied = self:call(self:getModelManager(), { "configureComponent", "ConfigureComponent", "applyComponentConfiguration", "ApplyComponentConfiguration" }, self.selectedComponent, self.pendingValues)
	if applied then for key, value in pairs(self.pendingValues) do self.originalValues[key] = value end; self.dirty = false; self:emit("applied", self.selectedComponent) end
	return applied
end
function ComponentConfig:clickCancel() self.pendingValues = {}; for key, value in pairs(self.originalValues) do self.pendingValues[key] = value end; self.dirty = false; return self:clickConfigClose() end
function ComponentConfig:clickConfigClose() return FlightDialog.clickClose(self) end
function ComponentConfig:clickCanopyChange() return self:call(self:getUIManager(), { "clickChangeComponent", "ClickChangeComponent" }, "canopy") end
function ComponentConfig:clickCanopyToggleVisibility() return self:call(self:getModelManager(), { "toggleComponentVisibility", "ToggleComponentVisibility" }, "canopy") end
function ComponentConfig:clickBodyshellChange() return self:call(self:getUIManager(), { "clickChangeComponent", "ClickChangeComponent" }, "bodyshell") end
function ComponentConfig:clickBodyshellToggleVisibility() return self:call(self:getModelManager(), { "toggleComponentVisibility", "ToggleComponentVisibility" }, "bodyshell") end
function ComponentConfig:onQueryResult(result) return self:setSelectedComponent(result) end

ComponentConfig.Start = FlightDialog.start
ComponentConfig.UpdateActivePanel = ComponentConfig.updateActivePanel
ComponentConfig.Update = FlightDialog.update
ComponentConfig.ClickConfigClose = ComponentConfig.clickConfigClose
ComponentConfig.ClickApply = ComponentConfig.clickApply
ComponentConfig.ClickCancel = ComponentConfig.clickCancel
ComponentConfig.ClickCanopyChange = ComponentConfig.clickCanopyChange
ComponentConfig.ClickCanopyToggleVisibility = ComponentConfig.clickCanopyToggleVisibility
ComponentConfig.ClickBodyshellChange = ComponentConfig.clickBodyshellChange
ComponentConfig.ClickBodyshellToggleVisibility = ComponentConfig.clickBodyshellToggleVisibility
ComponentConfig.OnQueryResult = ComponentConfig.onQueryResult

class 'ComponentChange' (FlightDialog)

function ComponentChange:__init(component)
	FlightDialog.__init(self, component)
	self.componentType = 0
	self.originalComponentId = nil
	self.availableComponents = {}
	self.selectedComponent = nil
end
function ComponentChange:updateConfiguredModels()
	local called, items = self:call(self:getModelManager(), { "getCompatibleComponents", "GetCompatibleComponents" }, self.componentType)
	if called then self.availableComponents = self:list(items) end
	self:emit("componentsChanged", self.availableComponents); return self.availableComponents
end
function ComponentChange:clickComponent(item) self.selectedComponent = item; self:emit("selected", item); return item ~= nil end
function ComponentChange:clickApply()
	if not self.selectedComponent then return false end
	local changed = self:call(self:getModelManager(), { "changeComponent", "ChangeComponent", "replaceComponent", "ReplaceComponent" }, self.originalComponentId, self.selectedComponent)
	if changed then self:emit("applied", self.selectedComponent); self:hide(true) end
	return changed
end
function ComponentChange:clickCancel() self.selectedComponent = nil; return self:hide(true) end
function ComponentChange:setupFromConfiguredModelData(data) self.originalComponentId = self:read(data, {}, { "componentId", "id" }, self.originalComponentId); return self:updateConfiguredModels() end
function ComponentChange:setupFromConfiguredModelDataString(data) local called, decoded = self:call(self:getApplicationManager(), { "decodeJson", "DecodeJson" }, data); return called and self:setupFromConfiguredModelData(decoded) or false end
function ComponentChange:onModelLoaded() return self:updateConfiguredModels() end
function ComponentChange:onModelComponentChanged() return self:updateConfiguredModels() end
function ComponentChange:onQueryResult(result) return self:setupFromConfiguredModelData(result) end

ComponentChange.Start = FlightDialog.start
ComponentChange.Update = FlightDialog.update
ComponentChange.ClickCancel = ComponentChange.clickCancel
ComponentChange.updateConfiguredModels = ComponentChange.updateConfiguredModels
ComponentChange.ClickApply = ComponentChange.clickApply
ComponentChange.ClickComponent = ComponentChange.clickComponent
ComponentChange.SetupFromConfiguredModelDataString = ComponentChange.setupFromConfiguredModelDataString
ComponentChange.SetupFromConfiguredModelData = ComponentChange.setupFromConfiguredModelData
ComponentChange.OnModelLoaded = ComponentChange.onModelLoaded
ComponentChange.OnModelComponentChanged = ComponentChange.onModelComponentChanged
ComponentChange.OnQueryResult = ComponentChange.onQueryResult

class 'AirFoilPanel' (FlightDialog)

function AirFoilPanel:__init(component)
	FlightDialog.__init(self, component)
	self.curve = {}
	self.currentFilePath = ""
	self.defaultCurve = { { -20, -0.6 }, { 0, 0 }, { 10, 1.0 }, { 18, 0.7 }, { 30, 0.2 } }
end
function AirFoilPanel:start() FlightDialog.start(self); if #self.curve == 0 then self:clickReset() end; return true end
function AirFoilPanel:clickReset() self.curve = {}; for _, point in ipairs(self.defaultCurve) do table.insert(self.curve, { point[1], point[2] }) end; self.dirty = true; self:emit("curveChanged", self.curve); return true end
function AirFoilPanel:fileOpen(path)
	local provider = self.fileProvider
	local called, data = self:call(provider, { "readAirfoil", "ReadAirfoil", "read", "Read" }, path)
	if not called then return false end
	self.currentFilePath = tostring(path or ""); self.curve = type(data) == "table" and data or self.curve; self.dirty = false; self:emit("curveChanged", self.curve); return true
end
function AirFoilPanel:clickOpen(path) return path and self:fileOpen(path) or self:emit("openRequested") end
function AirFoilPanel:clickSave(path)
	path = tostring(path or self.currentFilePath)
	if path == "" then self:emit("savePathRequested"); return false end
	local saved = self:call(self.fileProvider, { "writeAirfoil", "WriteAirfoil", "write", "Write" }, path, self.curve)
	if saved then self.currentFilePath, self.dirty = path, false end
	return saved
end
function AirFoilPanel:clickFileDialogSubmit(path) return self:fileOpen(path) end

AirFoilPanel.Start = AirFoilPanel.start
AirFoilPanel.OnDestroy = FlightDialog.onDestroy
AirFoilPanel.ClickOpen = AirFoilPanel.clickOpen
AirFoilPanel.ClickSave = AirFoilPanel.clickSave
AirFoilPanel.ClickReset = AirFoilPanel.clickReset
AirFoilPanel.ClickFileDialogSubmit = AirFoilPanel.clickFileDialogSubmit
AirFoilPanel.FileOpen = AirFoilPanel.fileOpen

class 'ModelSettings' (FlightDialog)

function ModelSettings:__init(component)
	FlightDialog.__init(self, component)
	self.settings = { physicsQuality = 1, crashReset = true, assists = true, damage = true }
end
function ModelSettings:load()
	local called, values = self:call(self:getModelManager(), { "getModelSettings", "GetModelSettings" })
	if called and type(values) == "table" then self.settings = values end
	return self.settings
end
function ModelSettings:apply() local applied = self:call(self:getModelManager(), { "setModelSettings", "SetModelSettings", "applyModelSettings", "ApplyModelSettings" }, self.settings); if applied then self.dirty = false; self:emit("applied", self.settings) end; return applied end
function ModelSettings:setSetting(name, value) self.settings[name] = value; self.dirty = true; return value end
ModelSettings.Load = ModelSettings.load
ModelSettings.Apply = ModelSettings.apply
ModelSettings.SetSetting = ModelSettings.setSetting

class 'SceneEditorManagerGUI' (FlightDialog)

function SceneEditorManagerGUI:__init(component)
	FlightDialog.__init(self, component)
	self.sceneRoot = nil
	self.panels = {}
	self.activePanel = nil
	self.objects = {}
	self.selectedObjectIndex = 0
	self.editing = false
end
function SceneEditorManagerGUI:getEditor() local called, editor = self:call(self:getApplicationManager(), { "getSceneEditor", "GetSceneEditor" }); return called and editor or nil end
function SceneEditorManagerGUI:start() FlightDialog.start(self); self:reset(); return true end
function SceneEditorManagerGUI:reset() self.activePanel, self.selectedObjectIndex, self.editing = nil, 0, false; self:hidePanels(); return true end
function SceneEditorManagerGUI:createSceneRoot() local called, root = self:call(self:getEditor(), { "createSceneRoot", "CreateSceneRoot" }); if called then self.sceneRoot = root end; return self.sceneRoot end
function SceneEditorManagerGUI:destroySceneRoot() local called = self:call(self:getEditor(), { "destroySceneRoot", "DestroySceneRoot" }, self.sceneRoot); self.sceneRoot = nil; return called end
function SceneEditorManagerGUI:showPanel(panel) self.activePanel = panel; for _, item in ipairs(self.panels) do self:setControlVisible(item, item == panel) end; return true end
function SceneEditorManagerGUI:hidePanel(panel) self:setControlVisible(panel, false); if self.activePanel == panel then self.activePanel = nil end; return true end
function SceneEditorManagerGUI:hidePanels() for _, panel in ipairs(self.panels) do self:setControlVisible(panel, false) end; self.activePanel = nil; return true end
function SceneEditorManagerGUI:showSelector() return self:showPanel(self.controls.selectorPanel) end
function SceneEditorManagerGUI:onClick(index) self.selectedObjectIndex = math.max(0, math.floor(tonumber(index) or 0)); self:emit("objectSelected", self.selectedObjectIndex); return true end
function SceneEditorManagerGUI:onClickPlay() self.editing = false; return self:call(self:getEditor(), { "play", "Play", "setEditing", "SetEditing" }, false) end
function SceneEditorManagerGUI:onClickEdit() self.editing = true; return self:call(self:getEditor(), { "edit", "Edit", "setEditing", "SetEditing" }, true) end
function SceneEditorManagerGUI:onClickEditPanel(panel) return self:showPanel(panel) end
function SceneEditorManagerGUI:setupEditorButtons() self:emit("buttonsChanged", self.editing); return true end
function SceneEditorManagerGUI:onCloseEditorPanel(panel) return self:hidePanel(panel or self.activePanel) end
function SceneEditorManagerGUI:onCLickSelectorButton() return self:showSelector() end
function SceneEditorManagerGUI:onCloseSelector() return self:hidePanel(self.controls.selectorPanel) end
function SceneEditorManagerGUI:loadScene(path) return self:call(self:getEditor(), { "loadScene", "LoadScene" }, path) end
function SceneEditorManagerGUI:saveScene(path) return self:call(self:getEditor(), { "saveScene", "SaveScene" }, path) end
function SceneEditorManagerGUI:generateTerrain() return self:call(self:getEditor(), { "generateTerrain", "GenerateTerrain" }) end
function SceneEditorManagerGUI:destroyTerrain() return self:call(self:getEditor(), { "destroyTerrain", "DestroyTerrain" }) end
function SceneEditorManagerGUI:onClickUploadButton() return self:emit("uploadRequested") end

SceneEditorManagerGUI.Start = SceneEditorManagerGUI.start
SceneEditorManagerGUI.Update = FlightDialog.update
SceneEditorManagerGUI.OnClickPlay = SceneEditorManagerGUI.onClickPlay
SceneEditorManagerGUI.OnClickEdit = SceneEditorManagerGUI.onClickEdit
SceneEditorManagerGUI.OnClickEditPanel = SceneEditorManagerGUI.onClickEditPanel
SceneEditorManagerGUI.Reset = SceneEditorManagerGUI.reset
SceneEditorManagerGUI.SetupEditorButtons = SceneEditorManagerGUI.setupEditorButtons
SceneEditorManagerGUI.CreateSceneRoot = SceneEditorManagerGUI.createSceneRoot
SceneEditorManagerGUI.DestroySceneRoot = SceneEditorManagerGUI.destroySceneRoot
SceneEditorManagerGUI.ShowPanel = SceneEditorManagerGUI.showPanel
SceneEditorManagerGUI.HidePanel = SceneEditorManagerGUI.hidePanel
SceneEditorManagerGUI.HidePanels = SceneEditorManagerGUI.hidePanels
SceneEditorManagerGUI.ShowSelector = SceneEditorManagerGUI.showSelector
SceneEditorManagerGUI.OnClick = SceneEditorManagerGUI.onClick
SceneEditorManagerGUI.OnCloseEditorPanel = SceneEditorManagerGUI.onCloseEditorPanel
SceneEditorManagerGUI.OnCLickSelectorButton = SceneEditorManagerGUI.onCLickSelectorButton
SceneEditorManagerGUI.OnCloseSelector = SceneEditorManagerGUI.onCloseSelector
SceneEditorManagerGUI.LoadScene = SceneEditorManagerGUI.loadScene
SceneEditorManagerGUI.SaveScene = SceneEditorManagerGUI.saveScene
SceneEditorManagerGUI.GenerateTerrain = SceneEditorManagerGUI.generateTerrain
SceneEditorManagerGUI.DestroyTerrain = SceneEditorManagerGUI.destroyTerrain
SceneEditorManagerGUI.OnClickUploadButton = SceneEditorManagerGUI.onClickUploadButton

class 'DevPanel' (FlightDialog)
function DevPanel:__init(component) FlightDialog.__init(self, component); self.commands = {}; self.history = {} end
function DevPanel:registerCommand(name, callback) self.commands[name] = callback end
function DevPanel:execute(name, ...) local fn = self.commands[name]; if not fn then return self:call(self:getApplicationManager(), { "executeDebugCommand", "ExecuteDebugCommand" }, name, ...) end; local ok, result = pcall(fn, self, ...); table.insert(self.history, { name = name, ok = ok, result = result }); return ok, result end
DevPanel.RegisterCommand = DevPanel.registerCommand
DevPanel.Execute = DevPanel.execute

class 'ColourPicker' (FlightDialog)
function ColourPicker:__init(component) FlightDialog.__init(self, component); self.colour = { r = 1, g = 1, b = 1, a = 1 } end
function ColourPicker:setColour(red, green, blue, alpha) self.colour = { r = self:limit(red,0,1,1), g = self:limit(green,0,1,1), b = self:limit(blue,0,1,1), a = self:limit(alpha,0,1,1) }; self:emit("changed", self.colour); return self.colour end
function ColourPicker:getColour() return self.colour end
ColourPicker.SetColour = ColourPicker.setColour
ColourPicker.GetColour = ColourPicker.getColour

class 'FixedWingDialog' (FlightDialog)
function FixedWingDialog:__init(component) FlightDialog.__init(self, component); self.settings = { aileronRate = 1, elevatorRate = 1, rudderRate = 1, flap = 0, expo = 0 } end
function FixedWingDialog:setSetting(name, value) self.settings[name] = tonumber(value) or value; self.dirty = true; return value end
function FixedWingDialog:apply() local applied = self:call(self:getModelManager(), { "setFixedWingSettings", "SetFixedWingSettings" }, self.settings); if applied then self.dirty = false; self:emit("applied", self.settings) end; return applied end
FixedWingDialog.SetSetting = FixedWingDialog.setSetting
FixedWingDialog.Apply = FixedWingDialog.apply
