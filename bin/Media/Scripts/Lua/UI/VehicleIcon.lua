include("BaseComponent.lua")

class 'VehicleIcon' (BaseComponent)

local function tryCall(object, names, ...)
	if not object then return false, nil end
	for _, name in ipairs(names) do
		local found, fn = pcall(function() return object[name] end)
		if found and fn then
			local called, result = pcall(fn, object, ...)
			if called then return true, result end
		end
	end
	return false, nil
end

local function readMember(object, getterNames, fieldNames, defaultValue)
	local called, value = tryCall(object, getterNames)
	if called and value ~= nil then return value end
	if object then
		for _, name in ipairs(fieldNames) do
			local ok, direct = pcall(function() return object[name] end)
			if ok and direct ~= nil then return direct end
		end
	end
	return defaultValue
end

local function writeMember(object, setterNames, fieldNames, value)
	if not object then return false end
	local called, result = tryCall(object, setterNames, value)
	if called then return result ~= false end
	for _, name in ipairs(fieldNames) do
		local ok = pcall(function() object[name] = value end)
		if ok then return true end
	end
	return false
end

local function singleton(className, interfaceName)
	local classObject = rawget(_G, className)
	if classObject then
		local ok, instance = pcall(function() return classObject.instance end)
		if ok then
			if type(instance) == "function" then
				local called, value = pcall(instance, classObject)
				if not called then called, value = pcall(instance) end
				if called and value then return value end
			elseif instance then
				return instance
			end
		end
		-- Some Lua ports expose the service itself instead of a singleton property.
		return classObject
	end
	local interface = interfaceName and rawget(_G, interfaceName) or nil
	if interface then
		local called, value = tryCall(interface, { "instance", "Instance", "getInstance", "GetInstance" })
		if called then return value end
	end
	return nil
end

local function makeColour(red, green, blue, alpha)
	local ok, colour = pcall(function() return ColourF(red, green, blue, alpha) end)
	if ok then return colour end
	return { r = red, g = green, b = blue, a = alpha }
end

local function colourComponent(colour, name, defaultValue)
	if not colour then return defaultValue end
	local ok, value = pcall(function() return colour[name] end)
	return ok and tonumber(value) or defaultValue
end

local function integer(value, defaultValue)
	local number = tonumber(value)
	if not number then return defaultValue or 0 end
	return math.floor(number)
end

function VehicleIcon:__init(component)
	BaseComponent.__init(self, component)
	self.component = component
	self.sceneId = 0
	self.sceneName = ""
	self.thumb = nil
	self.sceneNameText = nil
	self.modelSetup = nil
	self.highlightObject = nil
	self.highlightImage = nil
	self.highlightColor = makeColour(0.20, 0.65, 1.0, 1.0)
	self.normalColor = makeColour(1.0, 1.0, 1.0, 1.0)
	self.configModelId = 0
	self.isModelHanger = false
	self.sprite = nil
	self.applicationController = nil
	self.loadingController = nil
	self.uiController = nil
	self.thumbnailProvider = nil
	self.onSelected = nil
	self.isHighlighted = false
	self.highlightStateKnown = false
	self.lastError = ""
end

function VehicleIcon:__finalize()
	self.onSelected = nil
	self.thumbnailProvider = nil
	BaseComponent.__finalize(self)
end

function VehicleIcon:setThumb(value) self.thumb = value end
function VehicleIcon:getThumb() return self.thumb end
function VehicleIcon:setSceneNameText(value) self.sceneNameText = value end
function VehicleIcon:getSceneNameText() return self.sceneNameText end
function VehicleIcon:setHighlightObject(value) self.highlightObject = value; self.highlightStateKnown = false end
function VehicleIcon:setHighlightImage(value) self.highlightImage = value; self.highlightStateKnown = false end
function VehicleIcon:setHighlightColor(value) self.highlightColor = value; self.highlightStateKnown = false end
function VehicleIcon:setNormalColor(value) self.normalColor = value; self.highlightStateKnown = false end
function VehicleIcon:setApplicationController(value) self.applicationController = value end
function VehicleIcon:setLoadingController(value) self.loadingController = value end
function VehicleIcon:setUIController(value) self.uiController = value end
function VehicleIcon:setThumbnailProvider(value) self.thumbnailProvider = value end
function VehicleIcon:setSelectedCallback(value) self.onSelected = value end
function VehicleIcon:getSceneId() return self.sceneId end
function VehicleIcon:setSceneId(value) self.sceneId = integer(value, 0) end
function VehicleIcon:getSceneName() return self.sceneName end
function VehicleIcon:setSceneName(value)
	self.sceneName = tostring(value or "")
	self:setLabelText(self.sceneName)
end
function VehicleIcon:getModelSetup() return self.modelSetup end
function VehicleIcon:getConfigModelId() return self.configModelId end
function VehicleIcon:setConfigModelId(value)
	self.configModelId = integer(value, 0)
	self.highlightStateKnown = false
end
function VehicleIcon:getIsModelHanger() return self.isModelHanger end
function VehicleIcon:setIsModelHanger(value)
	self.isModelHanger = value == true
	self.highlightStateKnown = false
end
function VehicleIcon:getIsHighlighted() return self.isHighlighted end

function VehicleIcon:getApplicationController()
	return self.applicationController or singleton("ApplicationManager", "IApplicationManager")
end

function VehicleIcon:getLoadingController()
	return self.loadingController or singleton("LoadingManager", "ILoadingManager")
end

function VehicleIcon:getUIController()
	return self.uiController or singleton("UIManager", "IUIManager")
end

function VehicleIcon:setLabelText(value)
	if not self.sceneNameText then return false end
	local text = tostring(value or "")
	local called = tryCall(self.sceneNameText, { "setText", "SetText", "setValue", "SetValue" }, text)
	if called then return true end
	return pcall(function() self.sceneNameText.text = text end)
end

function VehicleIcon:setSprite(sprite)
	self.sprite = sprite
	if not self.thumb then
		self.lastError = "Thumbnail image is unavailable"
		return false
	end
	local called = false
	if type(sprite) == "string" then
		called = tryCall(self.thumb,
			{ "setTextureName", "SetTextureName", "setSpriteName", "SetSpriteName" }, sprite)
	end
	if not called then
		called = tryCall(self.thumb,
			{ "setSprite", "SetSprite", "setImage", "SetImage", "setTexture", "SetTexture" }, sprite)
	end
	if not called then called = pcall(function() self.thumb.sprite = sprite end) end
	if not called then self.lastError = "Thumbnail sprite or texture could not be assigned" end
	return called
end

function VehicleIcon:getThumbnailBundle()
	local loading = self:getLoadingController()
	return readMember(loading,
		{ "getThumbsModelsAssetBundle", "GetThumbsModelsAssetBundle", "getModelThumbnailBundle", "GetModelThumbnailBundle" },
		{ "thumbsModelsAssetBundle", "modelThumbnailBundle" }, nil)
end

function VehicleIcon:loadThumbnail(setup)
	if not setup then return false end
	local imageName = tostring(readMember(setup,
		{ "getImageName", "GetImageName" }, { "imageName", "m_ImageName" }, "") or "")
	local resource = readMember(setup,
		{ "getThumbResource", "GetThumbResource" }, { "thumbResource", "m_ThumbResource" }, nil)
	local candidate = nil

	if self.thumbnailProvider then
		local called, value
		if type(self.thumbnailProvider) == "function" then
			called, value = pcall(self.thumbnailProvider, imageName, resource, setup)
		else
			called, value = tryCall(self.thumbnailProvider,
				{ "loadModelThumbnail", "LoadModelThumbnail", "loadThumbnail", "LoadThumbnail", "load", "Load" },
				imageName, resource, setup)
		end
		if called then candidate = value end
	end

	local bundle = self:getThumbnailBundle()
	if not candidate and imageName ~= "" then
		local called, value = tryCall(bundle,
			{ "loadAsset", "LoadAsset", "getAsset", "GetAsset", "loadSprite", "LoadSprite" }, imageName)
		if called then candidate = value end
	end

	if not candidate and resource ~= nil and tostring(resource) ~= "" then
		local called, value = tryCall(bundle,
			{ "loadAsset", "LoadAsset", "getAsset", "GetAsset", "loadSprite", "LoadSprite" }, resource)
		if called then candidate = value end
	end
	-- Resource names are still useful to engines whose image control resolves assets itself.
	if not candidate and resource ~= nil and tostring(resource) ~= "" then candidate = resource end
	if not candidate and imageName ~= "" then candidate = imageName end
	if not candidate then
		self.lastError = "Model thumbnail metadata is unavailable"
		return false
	end
	return self:setSprite(candidate)
end

function VehicleIcon:setModelSetup(setup)
	self.modelSetup = setup
	self.highlightStateKnown = false
	self.lastError = ""
	if not setup then
		self.sceneId = 0
		self.sceneName = ""
		self.sprite = nil
		self:setLabelText("")
		if self.thumb then self:setSprite(nil) end
		self:updateHighlight(true)
		return true
	end

	self.sceneId = integer(readMember(setup,
		{ "getReferenceId", "GetReferenceId", "getSceneId", "GetSceneId" },
		{ "referenceId", "m_ReferenceId", "sceneId", "id" }, self.sceneId), self.sceneId)
	self.sceneName = tostring(readMember(setup,
		{ "getModelName", "GetModelName", "getName", "GetName" },
		{ "modelName", "m_ModelName", "name" }, "") or "")
	self:setLabelText(self.sceneName)
	-- A missing thumbnail is non-fatal; the icon remains selectable and reports the issue.
	self:loadThumbnail(setup)
	self:updateHighlight(true)
	return true
end

function VehicleIcon:setModelName(modelName)
	self.sceneName = tostring(modelName or "")
	return self:setLabelText(self.sceneName)
end

function VehicleIcon:getSelectedModelData()
	local controller = self:getApplicationController()
	return readMember(controller,
		{ "getSelectedModelData", "GetSelectedModelData" }, { "selectedModelData" }, nil)
end

function VehicleIcon:getSelectedModelId()
	local controller = self:getApplicationController()
	local value = readMember(controller,
		{ "getSelectedModelId", "GetSelectedModelId" }, { "selectedModelId" }, nil)
	return value ~= nil and tonumber(value) or nil
end

function VehicleIcon:getModelReferenceId(setup)
	if not setup then return nil end
	local value = readMember(setup,
		{ "getReferenceId", "GetReferenceId", "getSceneId", "GetSceneId" },
		{ "referenceId", "m_ReferenceId", "sceneId", "id" }, nil)
	return value ~= nil and tonumber(value) or nil
end

function VehicleIcon:isSelected()
	if self.isModelHanger then
		local selected = self:getSelectedModelData()
		if not selected or not self.modelSetup then return false end
		local sameObject = false
		pcall(function() sameObject = selected == self.modelSetup end)
		if sameObject then return true end
		-- Wrapped native objects are not always reference-equal in Lua.
		local selectedId = self:getModelReferenceId(selected)
		local ownId = self:getModelReferenceId(self.modelSetup) or self.sceneId
		return selectedId ~= nil and ownId ~= nil and selectedId == ownId
	end
	local selectedId = self:getSelectedModelId()
	return selectedId ~= nil and selectedId == self.configModelId
end

function VehicleIcon:setObjectEnabled(object, enabled)
	if not object then return false end
	local called = tryCall(object,
		{ "setActive", "SetActive", "setEnabled", "SetEnabled", "setVisible", "SetVisible" }, enabled == true)
	return called
end

function VehicleIcon:isObjectEnabled(object)
	if not object then return false end
	local called, value = tryCall(object,
		{ "isActiveInHierarchy", "IsActiveInHierarchy", "isEnabled", "IsEnabled", "isVisible", "IsVisible" })
	if called then return value == true end
	local direct = readMember(object, {}, { "activeInHierarchy", "active", "enabled", "visible" }, false)
	return direct == true
end

function VehicleIcon:setHighlightColour(colour)
	if not self.highlightImage then return false end
	local called = tryCall(self.highlightImage, { "setColour", "SetColour", "setColor", "SetColor" }, colour)
	if called then return true end
	return pcall(function() self.highlightImage.color = colour end)
end

function VehicleIcon:centreHighlightObject()
	if not self.highlightObject or not self:isObjectEnabled(self.highlightObject) then return false end
	local called, transform = tryCall(self.highlightObject, { "getComponent", "GetComponent" }, "LayoutTransform")
	if not called or not transform then return false end
	local ok, zero = pcall(function() return Vector2F(0.0, 0.0) end)
	if ok then
		local positioned = tryCall(transform,
			{ "setPosition", "SetPosition", "setLocalPosition", "SetLocalPosition" }, zero)
		if positioned then return true end
	end
	return tryCall(transform,
		{ "setPosition", "SetPosition", "setLocalPosition", "SetLocalPosition" }, 0.0, 0.0)
end

function VehicleIcon:applyHighlight(selected)
	selected = selected == true
	if self.highlightImage then
		self:setHighlightColour(selected and self.highlightColor or self.normalColor)
	elseif self.highlightObject then
		self:setObjectEnabled(self.highlightObject, selected)
	end
	self.isHighlighted = selected
	self.highlightStateKnown = true
	self:centreHighlightObject()
	return true
end

function VehicleIcon:select() return self:applyHighlight(true) end
function VehicleIcon:deselect() return self:applyHighlight(false) end

function VehicleIcon:updateHighlight(force)
	local selected = self:isSelected()
	if force == true or not self.highlightStateKnown or selected ~= self.isHighlighted then
		self:applyHighlight(selected)
	elseif self.highlightObject and self:isObjectEnabled(self.highlightObject) then
		self:centreHighlightObject()
	end
	return selected
end

function VehicleIcon:applySelection()
	local controller = self:getApplicationController()
	if not controller then return false end
	local dataSet = writeMember(controller,
		{ "setSelectedModelData", "SetSelectedModelData" }, { "selectedModelData" }, self.modelSetup)
	local idSet = writeMember(controller,
		{ "setSelectedModelId", "SetSelectedModelId" }, { "selectedModelId" }, self.configModelId)
	return dataSet and idSet
end

function VehicleIcon:notifyUIManager()
	local ui = self:getUIController()
	if not ui then return true end
	local called, result = tryCall(ui, { "clickModelIcon", "ClickModelIcon" }, self)
	return not called or result ~= false
end

function VehicleIcon:clickIcon()
	if not self.modelSetup then
		self.lastError = "A model setup must be assigned before selection"
		return false
	end
	if not self:applySelection() then
		self.lastError = "Selected model data could not be applied"
		return false
	end
	if not self:notifyUIManager() then
		self.lastError = "The UI manager rejected the selected model"
		return false
	end
	self:updateHighlight(true)
	if self.onSelected then
		local ok, errorMessage = pcall(self.onSelected, self, self.configModelId, self.modelSetup)
		if not ok then
			self.lastError = "Model selection callback failed: " .. tostring(errorMessage)
			return false
		end
	end
	self.lastError = ""
	return true
end

function VehicleIcon:start()
	self:updateHighlight(true)
end

function VehicleIcon:update()
	self:updateHighlight(false)
end

function VehicleIcon:handleEvent(parameters, results)
	if not parameters then return false end
	local ok, eventHash = pcall(function() return parameters:at(1) end)
	if not ok then return false end
	local clickHash, activateHash = nil, nil
	pcall(function() clickHash = IEvent.CLICK_HASH; activateHash = IEvent.ACTIVATE_HASH end)
	if eventHash == clickHash or eventHash == activateHash then return self:clickIcon() end
	return false
end

function VehicleIcon:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsInt("sceneId", self.sceneId)
	properties:setPropertyAsString("sceneName", self.sceneName)
	properties:setPropertyAsInt("configModelId", self.configModelId)
	properties:setPropertyAsBool("isModelHanger", self.isModelHanger)
	properties:setPropertyAsBool("isHighlighted", self.isHighlighted)
	properties:setPropertyAsFloat("highlightRed", colourComponent(self.highlightColor, "r", 0.20))
	properties:setPropertyAsFloat("highlightGreen", colourComponent(self.highlightColor, "g", 0.65))
	properties:setPropertyAsFloat("highlightBlue", colourComponent(self.highlightColor, "b", 1.0))
	properties:setPropertyAsFloat("highlightAlpha", colourComponent(self.highlightColor, "a", 1.0))
	properties:setPropertyAsFloat("normalRed", colourComponent(self.normalColor, "r", 1.0))
	properties:setPropertyAsFloat("normalGreen", colourComponent(self.normalColor, "g", 1.0))
	properties:setPropertyAsFloat("normalBlue", colourComponent(self.normalColor, "b", 1.0))
	properties:setPropertyAsFloat("normalAlpha", colourComponent(self.normalColor, "a", 1.0))
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("Select Model", false)
	properties:setButtonPressed("Refresh Highlight", false)
end

function VehicleIcon:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("sceneId") then self:setSceneId(properties:getPropertyAsInt("sceneId")) end
	if properties:hasProperty("sceneName") then self:setSceneName(properties:getPropertyAsString("sceneName")) end
	if properties:hasProperty("configModelId") then self:setConfigModelId(properties:getPropertyAsInt("configModelId")) end
	if properties:hasProperty("isModelHanger") then self:setIsModelHanger(properties:getPropertyAsBool("isModelHanger")) end
	local highlightChanged = properties:hasProperty("highlightRed") or properties:hasProperty("highlightGreen") or
		properties:hasProperty("highlightBlue") or properties:hasProperty("highlightAlpha")
	if highlightChanged then
		self.highlightColor = makeColour(
			properties:getPropertyAsFloat("highlightRed"), properties:getPropertyAsFloat("highlightGreen"),
			properties:getPropertyAsFloat("highlightBlue"), properties:getPropertyAsFloat("highlightAlpha"))
	end
	local normalChanged = properties:hasProperty("normalRed") or properties:hasProperty("normalGreen") or
		properties:hasProperty("normalBlue") or properties:hasProperty("normalAlpha")
	if normalChanged then
		self.normalColor = makeColour(
			properties:getPropertyAsFloat("normalRed"), properties:getPropertyAsFloat("normalGreen"),
			properties:getPropertyAsFloat("normalBlue"), properties:getPropertyAsFloat("normalAlpha"))
	end
	if highlightChanged or normalChanged then self:updateHighlight(true) end
	if properties:isButtonPressed("Select Model") then self:clickIcon() end
	if properties:isButtonPressed("Refresh Highlight") then self:updateHighlight(true) end
end

-- Compatibility aliases for ModelIcon.cs and Unity event wiring.
VehicleIcon.Start = VehicleIcon.start
VehicleIcon.Update = VehicleIcon.update
VehicleIcon.Select = VehicleIcon.select
VehicleIcon.Deselect = VehicleIcon.deselect
VehicleIcon.SetModelSetup = VehicleIcon.setModelSetup
VehicleIcon.SetModelName = VehicleIcon.setModelName
VehicleIcon.SetSprite = VehicleIcon.setSprite
VehicleIcon.ClickIcon = VehicleIcon.clickIcon
VehicleIcon.HandleEvent = VehicleIcon.handleEvent
