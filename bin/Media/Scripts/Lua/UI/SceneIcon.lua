include("BaseComponent.lua")

class 'SceneIcon' (BaseComponent)

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

function SceneIcon:__init(component)
	BaseComponent.__init(self, component)
	self.component = component
	self.sceneId = 0
	self.sceneName = ""
	self.thumb = nil
	self.sceneNameText = nil
	self.scenerySetup = nil
	self.highlightObject = nil
	self.highlightImage = nil
	self.highlightColor = makeColour(0.20, 0.65, 1.0, 1.0)
	self.normalColor = makeColour(1.0, 1.0, 1.0, 1.0)
	self.sprite = nil
	self.applicationController = nil
	self.onScenerySelected = nil
	self.isHighlighted = false
	self.highlightStateKnown = false
	self.lastError = ""
end

function SceneIcon:__finalize()
	self.onScenerySelected = nil
	BaseComponent.__finalize(self)
end

function SceneIcon:setThumb(value) self.thumb = value end
function SceneIcon:getThumb() return self.thumb end
function SceneIcon:setSceneNameText(value) self.sceneNameText = value end
function SceneIcon:getSceneNameText() return self.sceneNameText end
function SceneIcon:setHighlightObject(value) self.highlightObject = value; self.highlightStateKnown = false end
function SceneIcon:setHighlightImage(value) self.highlightImage = value; self.highlightStateKnown = false end
function SceneIcon:setHighlightColor(value) self.highlightColor = value; self.highlightStateKnown = false end
function SceneIcon:setNormalColor(value) self.normalColor = value; self.highlightStateKnown = false end
function SceneIcon:setApplicationController(value) self.applicationController = value end
function SceneIcon:setScenerySelectedCallback(value) self.onScenerySelected = value end
function SceneIcon:getSceneId() return self.sceneId end
function SceneIcon:setSceneId(value) self.sceneId = math.floor(tonumber(value) or 0) end
function SceneIcon:getSceneName() return self.sceneName end
function SceneIcon:setSceneName(value)
	self.sceneName = tostring(value or "")
	self:setLabelText(self.sceneName)
end
function SceneIcon:getScenerySetup() return self.scenerySetup end
function SceneIcon:getIsHighlighted() return self.isHighlighted end

function SceneIcon:getApplicationController()
	if self.applicationController then return self.applicationController end

	local ok, controller = pcall(function()
		if ApplicationManager then
			local instance = ApplicationManager.instance
			if type(instance) == "function" then
				local called, value = pcall(instance)
				if called then return value end
				called, value = pcall(instance, ApplicationManager)
				if called then return value end
			else
				return instance
			end
		end
		return nil
	end)
	if ok and controller then return controller end

	ok, controller = pcall(function() return IApplicationManager.instance() end)
	return ok and controller or nil
end

function SceneIcon:setLabelText(value)
	if not self.sceneNameText then return false end
	local called = tryCall(self.sceneNameText, { "setText", "SetText", "setValue", "SetValue" }, tostring(value or ""))
	if called then return true end
	local readable, current = pcall(function() return self.sceneNameText.text end)
	if readable and current ~= nil then
		return pcall(function() self.sceneNameText.text = tostring(value or "") end)
	end
	return false
end

function SceneIcon:setSceneSetup(setup)
	self.scenerySetup = setup
	self.highlightStateKnown = false
	self.lastError = ""
	if not setup then
		self.sceneId = 0
		self.sceneName = ""
		self:setLabelText("")
		self:updateHighlight(true)
		return true
	end

	self.sceneId = math.floor(tonumber(readMember(setup,
		{ "getSceneId", "GetSceneId" }, { "sceneId", "id" }, self.sceneId)) or self.sceneId)
	local label = tostring(readMember(setup,
		{ "getLabel", "GetLabel" }, { "label" }, self.sceneName) or "")
	local internalName = readMember(setup,
		{ "getSceneName", "GetSceneName" }, { "sceneName" }, label)
	self.sceneName = tostring(internalName or label)
	self:setLabelText(label ~= "" and label or self.sceneName)
	self:updateHighlight(true)
	return true
end

function SceneIcon:setSprite(sprite)
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
	if not called then
		local ok = pcall(function() self.thumb.sprite = sprite end)
		called = ok
	end
	if not called then self.lastError = "Thumbnail sprite or texture could not be assigned" end
	return called
end

function SceneIcon:getSelectedScenery()
	local controller = self:getApplicationController()
	local called, selected = tryCall(controller,
		{ "getSelectedScenery", "GetSelectedScenery", "getSelectedScene", "GetSelectedScene" })
	if called then return selected end
	local ok, direct = pcall(function() return controller and controller.selectedScenery end)
	return ok and direct or nil
end

function SceneIcon:setSelectedScenery(setup)
	local controller = self:getApplicationController()
	if not controller then return false end
	local called, result = tryCall(controller,
		{ "setSelectedScenery", "SetSelectedScenery", "setSelectedScene", "SetSelectedScene" }, setup)
	if called then return result ~= false end
	local ok = pcall(function() controller.selectedScenery = setup end)
	return ok
end

function SceneIcon:getSetupId(setup)
	if not setup then return nil end
	local value = readMember(setup,
		{ "getSceneId", "GetSceneId" }, { "sceneId", "id" }, nil)
	return tonumber(value)
end

function SceneIcon:isSetupSelected(selected)
	if not self.scenerySetup or not selected then return false end
	local sameObject = false
	pcall(function() sameObject = selected == self.scenerySetup end)
	if sameObject then return true end
	local selectedId = self:getSetupId(selected)
	local ownId = self:getSetupId(self.scenerySetup) or self.sceneId
	return selectedId ~= nil and ownId ~= nil and selectedId == ownId
end

function SceneIcon:setObjectEnabled(object, enabled)
	if not object then return false end
	local called = tryCall(object,
		{ "setActive", "SetActive", "setEnabled", "SetEnabled", "setVisible", "SetVisible" }, enabled == true)
	return called
end

function SceneIcon:isObjectEnabled(object)
	if not object then return false end
	local called, value = tryCall(object,
		{ "isActiveInHierarchy", "IsActiveInHierarchy", "isEnabled", "IsEnabled", "isVisible", "IsVisible" })
	if called then return value == true end
	local ok, direct = pcall(function() return object.activeInHierarchy end)
	return ok and direct == true
end

function SceneIcon:setHighlightColour(colour)
	if not self.highlightImage then return false end
	local called = tryCall(self.highlightImage,
		{ "setColour", "SetColour", "setColor", "SetColor" }, colour)
	if called then return true end
	return pcall(function() self.highlightImage.color = colour end)
end

function SceneIcon:centreHighlightObject()
	if not self.highlightObject or not self:isObjectEnabled(self.highlightObject) then return false end
	local called, transform = tryCall(self.highlightObject,
		{ "getComponent", "GetComponent" }, "LayoutTransform")
	if not called or not transform then return false end
	local vectorOk, zero = pcall(function() return Vector2F(0.0, 0.0) end)
	if vectorOk then
		local positioned = tryCall(transform, { "setPosition", "SetPosition", "setLocalPosition", "SetLocalPosition" }, zero)
		if positioned then return true end
	end
	local positioned = tryCall(transform,
		{ "setPosition", "SetPosition", "setLocalPosition", "SetLocalPosition" }, 0.0, 0.0)
	return positioned
end

function SceneIcon:applyHighlight(selected)
	selected = selected == true
	if self.highlightImage then
		self:setHighlightColour(selected and self.highlightColor or self.normalColor)
	elseif self.highlightObject then
		self:setObjectEnabled(self.highlightObject, selected)
	end
	self:centreHighlightObject()
	self.isHighlighted = selected
	self.highlightStateKnown = true
	return true
end

function SceneIcon:updateHighlight(force)
	local selected = self:isSetupSelected(self:getSelectedScenery())
	if force == true or not self.highlightStateKnown or selected ~= self.isHighlighted then
		self:applyHighlight(selected)
	elseif self.highlightObject and self:isObjectEnabled(self.highlightObject) then
		-- Keep the overlay centred if a layout pass moved it.
		self:centreHighlightObject()
	end
	return selected
end

function SceneIcon:clickScenery()
	if not self.scenerySetup then
		self.lastError = "A scenery setup must be assigned before selection"
		return false
	end
	if not self:setSelectedScenery(self.scenerySetup) then
		self.lastError = "Selected scenery could not be applied"
		return false
	end
	self:updateHighlight(true)
	if self.onScenerySelected then
		local ok, errorMessage = pcall(self.onScenerySelected, self, self.scenerySetup, self.sceneId)
		if not ok then
			self.lastError = "Scenery selection callback failed: " .. tostring(errorMessage)
			return false
		end
	end
	self.lastError = ""
	return true
end

function SceneIcon:start()
	self:updateHighlight(true)
end

function SceneIcon:update()
	self:updateHighlight(false)
end

function SceneIcon:handleEvent(parameters, results)
	if not parameters then return false end
	local ok, eventHash = pcall(function() return parameters:at(1) end)
	if not ok then return false end
	local clickHash, activateHash = nil, nil
	pcall(function() clickHash = IEvent.CLICK_HASH; activateHash = IEvent.ACTIVATE_HASH end)
	if eventHash == clickHash or eventHash == activateHash then return self:clickScenery() end
	return false
end

function SceneIcon:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsInt("sceneId", self.sceneId)
	properties:setPropertyAsString("sceneName", self.sceneName)
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
	properties:setButtonPressed("Select Scenery", false)
	properties:setButtonPressed("Refresh Highlight", false)
end

function SceneIcon:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:hasProperty("sceneId") then self:setSceneId(properties:getPropertyAsInt("sceneId")) end
	if properties:hasProperty("sceneName") then self:setSceneName(properties:getPropertyAsString("sceneName")) end
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
	if properties:isButtonPressed("Select Scenery") then self:clickScenery() end
	if properties:isButtonPressed("Refresh Highlight") then self:updateHighlight(true) end
end

-- Compatibility aliases for the original C# API and Unity event wiring.
SceneIcon.Start = SceneIcon.start
SceneIcon.Update = SceneIcon.update
SceneIcon.SetSceneSetup = SceneIcon.setSceneSetup
SceneIcon.SetSprite = SceneIcon.setSprite
SceneIcon.ClickScenery = SceneIcon.clickScenery
SceneIcon.HandleEvent = SceneIcon.handleEvent
