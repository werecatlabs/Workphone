include("UIDialog.lua")
include("SceneSelect.lua")
include("VehicleSelect.lua")
include("FlightSetup.lua")

-- Compatibility classes retain the Unity names while reusing the converted
-- production controllers used by the Lua UI.
class 'UnityDialog' (UIDialog)
function UnityDialog:__init(component)
	UIDialog.__init(self, component)
	self.settingsValues = {}
end

function UnityDialog:onPointerEnter()
	return self:onMouseOver()
end

function UnityDialog:onPointerExit()
	return self:onMouseNotOver()
end

function UnityDialog:handleSettingsChanged(name, value)
	if name == nil or tostring(name) == "" then return false end
	self.settingsValues[tostring(name)] = value
	return true
end

function UnityDialog:clickGotoMainMenu()
	self:hide()
	local managerOk, applicationManager = pcall(function() return IApplicationManager.instance() end)
	if not managerOk or not applicationManager then return false end
	local names = { "clickGotoMainMenu", "ClickGotoMainMenu", "gotoMainMenu", "GotoMainMenu" }
	for _, name in ipairs(names) do
		local found, callback = pcall(function() return applicationManager[name] end)
		if found and callback then
			local called = pcall(callback, applicationManager)
			if called then return true end
		end
	end
	return false
end

function UnityDialog:fadeIn(speed)
	if speed ~= nil then self:setFadeSpeed(speed) end
	self.fadeOnShow = true
	return self:show(false)
end

function UnityDialog:fadeOut(speed)
	if speed ~= nil then self:setFadeSpeed(speed) end
	self.fadeOnHide = true
	return self:hide(false)
end

function UnityDialog:fadeInCoroutine(speed) return self:fadeIn(speed) end
function UnityDialog:fadeOutCoroutine(speed) return self:fadeOut(speed) end
UnityDialog.Initialise = UIDialog.initialise
UnityDialog.Show = UIDialog.show
UnityDialog.Hide = UIDialog.hide
UnityDialog.Reset = UIDialog.reset
UnityDialog.CloseDialog = UIDialog.hide
UnityDialog.OnPointerEnter = UnityDialog.onPointerEnter
UnityDialog.OnPointerExit = UnityDialog.onPointerExit
UnityDialog.HandleSettingsChanged = UnityDialog.handleSettingsChanged
UnityDialog.ClickGotoMainMenu = UnityDialog.clickGotoMainMenu
UnityDialog.FadeIn = UnityDialog.fadeIn
UnityDialog.FadeOut = UnityDialog.fadeOut
UnityDialog.FadeInCoroutine = UnityDialog.fadeInCoroutine
UnityDialog.FadeOutCoroutine = UnityDialog.fadeOutCoroutine

class 'ScenerySelector' (SceneSelect)
function ScenerySelector:__init(component) SceneSelect.__init(self, component) end

class 'ModelHangerDialog' (VehicleSelect)
function ModelHangerDialog:__init(component) VehicleSelect.__init(self, component) end
