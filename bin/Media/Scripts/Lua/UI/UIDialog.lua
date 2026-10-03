include("BaseComponent.lua")

class 'UIDialog' (BaseComponent)

UIDialog.REFERENCE_WIDTH = 1920
UIDialog.REFERENCE_HEIGHT = 1080

local function clamp(value, minimum, maximum, defaultValue)
	value = tonumber(value)
	if not value or value ~= value then value = defaultValue or minimum end
	if value < minimum then return minimum end
	if value > maximum then return maximum end
	return value
end

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

function UIDialog:__init(component)
	BaseComponent.__init(self, component)
	self.component = component

	self.dialogName = ""
	self.dialogReference = ""
	self.visible = false
	self.zOrder = 0
	self.showTime = 0.0
	self.hideTime = 0.0
	self.dialogFadeTime = 0.15
	self.isMouseOver = false
	self.isHiddenDialog = false
	self.overrideShow = false
	self.overrideHide = false
	self.dialogEnabled = true

	self.maxFadeAlpha = 1.0
	self.fadeSpeed = 0.5
	self.fadeOnShow = false
	self.fadeOnHide = false
	self.isShowing = false
	self.isHiding = false
	self.isExcludeMouseOver = false
	self.currentAlpha = 1.0
	self.transitionStartTime = nil
	self.transitionStartAlpha = nil
	self.transitionTargetAlpha = nil
	self.transitionDuration = 0.0

	self.fadeTarget = nil
	self.mouseHitTest = nil
	self.onShown = nil
	self.onHidden = nil
	self.onMouseEntered = nil
	self.onMouseExited = nil
	self.onVisibilityChanged = nil
	self.initialised = false
	self.lastError = ""
end

function UIDialog:__finalize()
	self.isShowing = false
	self.isHiding = false
	self.transitionStartTime = nil
	BaseComponent.__finalize(self)
end

function UIDialog:setFadeTarget(value) self.fadeTarget = value end
function UIDialog:setMouseHitTest(value) self.mouseHitTest = value end
function UIDialog:setShownCallback(value) self.onShown = value end
function UIDialog:setHiddenCallback(value) self.onHidden = value end
function UIDialog:setMouseEnteredCallback(value) self.onMouseEntered = value end
function UIDialog:setMouseExitedCallback(value) self.onMouseExited = value end
function UIDialog:setVisibilityChangedCallback(value) self.onVisibilityChanged = value end
function UIDialog:setDialogName(value) self.dialogName = tostring(value or "") end
function UIDialog:setDialogReference(value) self.dialogReference = tostring(value or "") end
function UIDialog:getDialogName() return self.dialogName end
function UIDialog:getDialogReference() return self.dialogReference end
function UIDialog:getZOrder() return self.zOrder end
function UIDialog:getFadeSpeed() return self.fadeSpeed end
function UIDialog:getDialogFadeTime() return self.dialogFadeTime end
function UIDialog:getFadeOnShow() return self.fadeOnShow end
function UIDialog:getFadeOnHide() return self.fadeOnHide end
function UIDialog:getMaxFadeAlpha() return self.maxFadeAlpha end
function UIDialog:getHiddenDialog() return self.isHiddenDialog end
function UIDialog:getExcludeMouseOver() return self.isExcludeMouseOver end
function UIDialog:getOverrideShow() return self.overrideShow end
function UIDialog:getOverrideHide() return self.overrideHide end
function UIDialog:getDialogEnabled() return self.dialogEnabled end
function UIDialog:getIsShowing() return self.isShowing end
function UIDialog:getIsHiding() return self.isHiding end
function UIDialog:getIsMouseOver() return self.isMouseOver end
function UIDialog:getVisible() return self.visible end
function UIDialog:getShowTime() return self.showTime end
function UIDialog:getHideTime() return self.hideTime end
function UIDialog:setShowTime(value) self.showTime = tonumber(value) or self.showTime end
function UIDialog:setHideTime(value) self.hideTime = tonumber(value) or self.hideTime end
function UIDialog:setIsShowing(value) self.isShowing = value == true end
function UIDialog:setIsHiding(value) self.isHiding = value == true end
function UIDialog:setIsMouseOver(value)
	if value == true then return self:onMouseOver() end
	return self:onMouseNotOver()
end
function UIDialog:setVisible(value)
	if value == true then return self:show(true) end
	return self:hide(true)
end
function UIDialog:setZOrder(value)
	self.zOrder = math.floor(tonumber(value) or 0)
	self:applyZOrder()
end
function UIDialog:setFadeSpeed(value) self.fadeSpeed = clamp(value, 0.0, 1000.0, 0.5) end
function UIDialog:setDialogFadeTime(value) self.dialogFadeTime = clamp(value, 0.0, 60.0, 0.15) end
function UIDialog:setFadeOnShow(value) self.fadeOnShow = value == true end
function UIDialog:setFadeOnHide(value) self.fadeOnHide = value == true end
function UIDialog:setMaxFadeAlpha(value)
	self.maxFadeAlpha = clamp(value, 0.0, 1.0, 1.0)
	if self.visible and not self.isShowing and not self.isHiding then self:setAlpha(self.maxFadeAlpha) end
end
function UIDialog:setHiddenDialog(value) self.isHiddenDialog = value == true end
function UIDialog:setExcludeMouseOver(value)
	self.isExcludeMouseOver = value == true
	if self.isExcludeMouseOver then self:onMouseNotOver() end
end
function UIDialog:setOverrideShow(value) self.overrideShow = value == true end
function UIDialog:setOverrideHide(value) self.overrideHide = value == true end
function UIDialog:setDialogEnabled(value)
	self.dialogEnabled = value == true
	if not self.dialogEnabled and self.visible then self:hide(true) end
end

function UIDialog:getActorSafe()
	if not self.component then return nil end
	local ok, actor = tryCall(self.component, { "getActor", "GetActor" })
	return ok and actor or nil
end

function UIDialog:getTime()
	local ok, manager = pcall(function() return IApplicationManager.instance() end)
	if not ok or not manager then return nil end
	local hasTimer, timer = tryCall(manager, { "getTimer", "GetTimer" })
	if not hasTimer or not timer then return nil end
	local called, value = tryCall(timer,
		{ "getTimeSinceLevelLoad", "getTimeSinceSceneLoad", "getTime", "GetTime" })
	return called and tonumber(value) or nil
end

function UIDialog:getTimeSinceShow()
	local now = self:getTime()
	return now and math.max(0.0, now - self.showTime) or 0.0
end

function UIDialog:getTimeSinceHide()
	local now = self:getTime()
	return now and math.max(0.0, now - self.hideTime) or 0.0
end

function UIDialog:setActorVisible(value)
	local actor = self:getActorSafe()
	if not actor then return false end
	local enabled = tryCall(actor, { "setEnabled", "SetEnabled" }, value == true)
	tryCall(actor, { "setVisible", "SetVisible" }, value == true)
	return enabled
end

function UIDialog:getActorVisible()
	local actor = self:getActorSafe()
	if not actor then return self.visible end
	local called, value = tryCall(actor, { "isEnabled", "IsEnabled" })
	if called then return value == true end
	called, value = tryCall(actor, { "isVisible", "IsVisible" })
	return called and value == true or self.visible
end

function UIDialog:applyZOrder()
	local actor = self:getActorSafe()
	if not actor then return false end
	local called, transform = tryCall(actor, { "getComponent", "GetComponent" }, "LayoutTransform")
	if called and transform then
		local applied = tryCall(transform, { "setZOrder", "SetZOrder" }, self.zOrder)
		return applied
	end
	return false
end

function UIDialog:getFadeTarget()
	if self.fadeTarget then return self.fadeTarget end
	local actor = self:getActorSafe()
	if not actor then return nil end
	for _, componentName in ipairs({ "CanvasGroup", "Image", "Text" }) do
		local found, component = tryCall(actor, { "getComponent", "GetComponent" }, componentName)
		if found and component then return component end
	end
	return nil
end

function UIDialog:setAlpha(value)
	value = clamp(value, 0.0, 1.0, self.currentAlpha)
	local target = self:getFadeTarget()
	if not target then
		self.currentAlpha = value
		return false
	end
	local called = tryCall(target, { "setAlpha", "SetAlpha", "setOpacity", "SetOpacity" }, value)
	if not called then
		local hasColour, colour = tryCall(target, { "getColour", "GetColour", "getColor", "GetColor" })
		if hasColour and colour then
			local changed = pcall(function() colour.a = value end)
			if changed then called = tryCall(target, { "setColour", "SetColour", "setColor", "SetColor" }, colour) end
		end
	end
	if called then self.currentAlpha = value end
	return called
end

function UIDialog:getFadeDuration()
	if self.dialogFadeTime > 0.0 then return self.dialogFadeTime end
	if self.fadeSpeed > 0.0 then return 1.0 / self.fadeSpeed end
	return 0.0
end

function UIDialog:notifyVisibilityChanged(value)
	if self.onVisibilityChanged then
		local ok, errorMessage = pcall(self.onVisibilityChanged, self, value == true)
		if not ok then self.lastError = "Visibility callback failed: " .. tostring(errorMessage) end
	end
end

function UIDialog:completeShow()
	self.isShowing = false
	self.isHiding = false
	self.transitionStartTime = nil
	self.visible = true
	if not self.overrideShow then self:setActorVisible(true) end
	self:setAlpha(self.maxFadeAlpha)
	if self.onShown then
		local ok, errorMessage = pcall(self.onShown, self)
		if not ok then self.lastError = "Shown callback failed: " .. tostring(errorMessage) end
	end
end

function UIDialog:completeHide()
	self.isShowing = false
	self.isHiding = false
	self.transitionStartTime = nil
	self.visible = false
	if not self.overrideHide then self:setActorVisible(false) end
	if self.onHidden then
		local ok, errorMessage = pcall(self.onHidden, self)
		if not ok then self.lastError = "Hidden callback failed: " .. tostring(errorMessage) end
	end
end

function UIDialog:beginFade(targetAlpha, showing)
	local now = self:getTime()
	local duration = self:getFadeDuration()
	if not now or duration <= 0.0 or not self:getFadeTarget() then return false end
	self.transitionStartTime = now
	self.transitionStartAlpha = self.currentAlpha
	self.transitionTargetAlpha = targetAlpha
	self.transitionDuration = duration
	self.isShowing = showing == true
	self.isHiding = not self.isShowing
	return true
end

function UIDialog:show(immediate)
	if not self.dialogEnabled then return false end
	local wasVisible = self.visible
	self.showTime = self:getTime() or self.showTime
	self.visible = true
	self.isHiding = false
	if not self.overrideShow then self:setActorVisible(true) end

	local animate = immediate ~= true and self.fadeOnShow
	if animate then
		self:setAlpha(0.0)
		if not self:beginFade(self.maxFadeAlpha, true) then self:completeShow() end
	else
		self:completeShow()
	end
	if not wasVisible then self:notifyVisibilityChanged(true) end
	return true
end

function UIDialog:hide(immediate)
	local wasVisible = self.visible
	self.hideTime = self:getTime() or self.hideTime
	self.visible = false
	self.isShowing = false

	local animate = immediate ~= true and self.fadeOnHide and not self.overrideHide
	if animate then
		if not self:beginFade(0.0, false) then self:completeHide() end
	else
		self:completeHide()
	end
	if wasVisible then self:notifyVisibilityChanged(false) end
	return true
end

function UIDialog:isVisible()
	return self.visible and (self.overrideShow or self:getActorVisible())
end

function UIDialog:updateFade()
	if not self.isShowing and not self.isHiding then return end
	local now = self:getTime()
	if not now or not self.transitionStartTime or self.transitionDuration <= 0.0 then
		if self.isShowing then self:completeShow() else self:completeHide() end
		return
	end
	local progress = clamp((now - self.transitionStartTime) / self.transitionDuration, 0.0, 1.0, 1.0)
	local alpha = self.transitionStartAlpha +
		(self.transitionTargetAlpha - self.transitionStartAlpha) * progress
	self:setAlpha(alpha)
	if progress >= 1.0 then
		if self.isShowing then self:completeShow() else self:completeHide() end
	end
end

function UIDialog:initialise()
	self.initialised = true
	self.isShowing = false
	self.isHiding = false
	self:applyZOrder()
	self:updateVisiblity()
	return true
end

function UIDialog:start()
	return self:initialise()
end

function UIDialog:update()
	self:updateFade()
	self:updateMouseOver()
end

function UIDialog:reset()
	self.isShowing = false
	self.isHiding = false
	self.transitionStartTime = nil
	self:onMouseNotOver()
	self:updateVisiblity()
	return true
end

function UIDialog:updateVisiblity()
	if self.isShowing or self.isHiding then return false end
	if self.visible and self.dialogEnabled then
		if not self.overrideShow then self:setActorVisible(true) end
		self:setAlpha(self.maxFadeAlpha)
	else
		if not self.overrideHide then self:setActorVisible(false) end
	end
	return true
end

function UIDialog:updateVisibility()
	return self:updateVisiblity()
end

function UIDialog:onMouseOver()
	if self.isExcludeMouseOver or self.isMouseOver then return false end
	self.isMouseOver = true
	if self.onMouseEntered then
		local ok, errorMessage = pcall(self.onMouseEntered, self)
		if not ok then self.lastError = "Mouse-enter callback failed: " .. tostring(errorMessage) end
	end
	return true
end

function UIDialog:onMouseNotOver()
	if not self.isMouseOver then return false end
	self.isMouseOver = false
	if self.onMouseExited then
		local ok, errorMessage = pcall(self.onMouseExited, self)
		if not ok then self.lastError = "Mouse-exit callback failed: " .. tostring(errorMessage) end
	end
	return true
end

function UIDialog:updateMouseOver()
	if self.isExcludeMouseOver or not self:isVisible() then
		self:onMouseNotOver()
		return false
	end
	local hit = nil
	if type(self.mouseHitTest) == "function" then
		local ok, value = pcall(self.mouseHitTest, self)
		if ok then hit = value == true end
	elseif self.mouseHitTest then
		local called, value = tryCall(self.mouseHitTest,
			{ "containsPointer", "ContainsPointer", "isMouseOver", "IsMouseOver", "hitTest", "HitTest" })
		if called then hit = value == true end
	end
	if hit == nil then return self.isMouseOver end
	if hit then self:onMouseOver() else self:onMouseNotOver() end
	return hit
end

function UIDialog:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("dialogName", self.dialogName)
	properties:setPropertyAsString("dialogReference", self.dialogReference)
	properties:setPropertyAsBool("visible", self.visible)
	properties:setPropertyAsInt("zOrder", self.zOrder)
	properties:setPropertyAsFloat("dialogFadeTime", self.dialogFadeTime)
	properties:setPropertyAsBool("isHiddenDialog", self.isHiddenDialog)
	properties:setPropertyAsBool("overrideShow", self.overrideShow)
	properties:setPropertyAsBool("overrideHide", self.overrideHide)
	properties:setPropertyAsBool("dialogEnabled", self.dialogEnabled)
	properties:setPropertyAsFloat("maxFadeAlpha", self.maxFadeAlpha)
	properties:setPropertyAsFloat("fadeSpeed", self.fadeSpeed)
	properties:setPropertyAsBool("fadeOnShow", self.fadeOnShow)
	properties:setPropertyAsBool("fadeOnHide", self.fadeOnHide)
	properties:setPropertyAsBool("isExcludeMouseOver", self.isExcludeMouseOver)
	properties:setPropertyAsString("lastError", self.lastError)
	properties:setButtonPressed("Show", false)
	properties:setButtonPressed("Hide", false)
	properties:setButtonPressed("Reset", false)
	properties:setButtonPressed("Generate", false)
end

function UIDialog:setProperties(parameters)
	local properties = parameters:at(0)
	if not properties then return end
	if properties:hasProperty("dialogName") then self:setDialogName(properties:getPropertyAsString("dialogName")) end
	if properties:hasProperty("dialogReference") then self:setDialogReference(properties:getPropertyAsString("dialogReference")) end
	if properties:hasProperty("zOrder") then self:setZOrder(properties:getPropertyAsInt("zOrder")) end
	if properties:hasProperty("dialogFadeTime") then self:setDialogFadeTime(properties:getPropertyAsFloat("dialogFadeTime")) end
	if properties:hasProperty("isHiddenDialog") then self:setHiddenDialog(properties:getPropertyAsBool("isHiddenDialog")) end
	if properties:hasProperty("overrideShow") then self:setOverrideShow(properties:getPropertyAsBool("overrideShow")) end
	if properties:hasProperty("overrideHide") then self:setOverrideHide(properties:getPropertyAsBool("overrideHide")) end
	if properties:hasProperty("dialogEnabled") then self:setDialogEnabled(properties:getPropertyAsBool("dialogEnabled")) end
	if properties:hasProperty("maxFadeAlpha") then self:setMaxFadeAlpha(properties:getPropertyAsFloat("maxFadeAlpha")) end
	if properties:hasProperty("fadeSpeed") then self:setFadeSpeed(properties:getPropertyAsFloat("fadeSpeed")) end
	if properties:hasProperty("fadeOnShow") then self:setFadeOnShow(properties:getPropertyAsBool("fadeOnShow")) end
	if properties:hasProperty("fadeOnHide") then self:setFadeOnHide(properties:getPropertyAsBool("fadeOnHide")) end
	if properties:hasProperty("isExcludeMouseOver") then self:setExcludeMouseOver(properties:getPropertyAsBool("isExcludeMouseOver")) end
	if properties:hasProperty("visible") then
		local requested = properties:getPropertyAsBool("visible")
		if requested then self:show(true) else self:hide(true) end
	end
	if properties:isButtonPressed("Show") then self:show() end
	if properties:isButtonPressed("Hide") then self:hide() end
	if properties:isButtonPressed("Reset") then self:reset() end
	if properties:isButtonPressed("Generate") then self:generate() end
end

function UIDialog:generate()
	return nil
end

-- Compatibility aliases for the original C# API and common Lua callers.
UIDialog.Start = UIDialog.start
UIDialog.Initialise = UIDialog.initialise
UIDialog.Update = UIDialog.update
UIDialog.Show = UIDialog.show
UIDialog.Hide = UIDialog.hide
UIDialog.IsVisible = UIDialog.isVisible
UIDialog.Reset = UIDialog.reset
UIDialog.UpdateVisiblity = UIDialog.updateVisiblity
UIDialog.UpdateVisibility = UIDialog.updateVisibility
UIDialog.OnMouseOver = UIDialog.onMouseOver
UIDialog.OnMouseNotOver = UIDialog.onMouseNotOver
UIDialog.UpdateMouseOver = UIDialog.updateMouseOver
UIDialog.setZorder = UIDialog.setZOrder
UIDialog.getZorder = UIDialog.getZOrder
UIDialog.setIsHiddenDialog = UIDialog.setHiddenDialog
UIDialog.getIsHiddenDialog = UIDialog.getHiddenDialog
UIDialog.setIsExcludeMouseOver = UIDialog.setExcludeMouseOver
UIDialog.getIsExcludeMouseOver = UIDialog.getExcludeMouseOver
