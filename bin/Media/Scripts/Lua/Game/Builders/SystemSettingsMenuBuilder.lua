class 'SystemSettingsMenu' (BaseComponent)

function SystemSettingsMenu:__init(component)
	BaseComponent.__init(self, component)
	self.title = "System Settings"
	self.masterVolume = 1.0
	self.musicVolume = 1.0
	self.sfxVolume = 1.0
	self.qualityIndex = 0
	self.resolutionIndex = 0
	self.fullscreen = true
	self.generatedRoot = nil
	self.controls = {}
end

function SystemSettingsMenu:__finalize()
	BaseComponent.__finalize(self)
end

function SystemSettingsMenu:update()
end

function SystemSettingsMenu:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("title", self.title)
	properties:setPropertyAsFloat("masterVolume", self.masterVolume)
	properties:setPropertyAsFloat("musicVolume", self.musicVolume)
	properties:setPropertyAsFloat("sfxVolume", self.sfxVolume)
	properties:setPropertyAsBool("fullscreen", self.fullscreen)
	properties:setPropertyAsButton("Generate", false)
	properties:setPropertyAsButton("Apply", false)
	properties:setPropertyAsButton("Reset", false)
end

function SystemSettingsMenu:setProperties(parameters)
	local properties = parameters:at(0)

	if properties:hasProperty("title") then
		self.title = properties:getPropertyAsString("title")
	end

	if properties:hasProperty("masterVolume") then
		self.masterVolume = properties:getPropertyAsFloat("masterVolume")
	end

	if properties:hasProperty("musicVolume") then
		self.musicVolume = properties:getPropertyAsFloat("musicVolume")
	end

	if properties:hasProperty("sfxVolume") then
		self.sfxVolume = properties:getPropertyAsFloat("sfxVolume")
	end

	if properties:hasProperty("fullscreen") then
		self.fullscreen = properties:getPropertyAsBool("fullscreen")
	end

	if properties:isButtonPressed("Generate") then
		self:generate()
	end

	if properties:isButtonPressed("Apply") then
		self:ApplySettings()
	end

	if properties:isButtonPressed("Reset") then
		self:ResetSettings()
	end
end

function SystemSettingsMenu:generate()
	local root = DataDrivenGameTemplateGenerator.CreateChild(nil, self.title)
	local canvas = self:CreateCanvas(root.transform)
	self:CreateSettingsPanel(canvas.transform)
	self.generatedRoot = root
	self:LoadSettingsIntoUI()
	return root
end

function SystemSettingsMenu:CreateCanvas(parent)
	local runtime = TemplateBuilderRuntime
	local canvas = DataDrivenGameTemplateGenerator.CreateChild(parent, "System Settings Canvas")

	if runtime then
		local canvasComponent = runtime.addComponent(canvas, "Canvas")
		if canvasComponent then
			canvasComponent.renderMode = "ScreenSpaceOverlay"
		end

		local scaler = runtime.addComponent(canvas, "CanvasScaler")
		if scaler then
			scaler.uiScaleMode = "ScaleWithScreenSize"
			scaler.referenceResolution = { x = 1920, y = 1080 }
			scaler.matchWidthOrHeight = 0.5
		end

		runtime.addComponent(canvas, "GraphicRaycaster")
	end

	return canvas
end

function SystemSettingsMenu:CreateSettingsPanel(parent)
	local panel = DataDrivenGameTemplateGenerator.CreateChild(parent, "Settings Panel")
	self.controls.title = DataDrivenGameTemplateGenerator.CreateUIText(panel.transform, "Title", self.title, 40, { x = 0, y = -60 }, "UpperCenter")
	self.controls.resolution = self:CreateDropdown(panel.transform, "Resolution", { "Current", "1280 x 720", "1920 x 1080", "2560 x 1440" }, self.resolutionIndex, { x = 0, y = -150 })
	self.controls.quality = self:CreateDropdown(panel.transform, "Quality", { "Low", "Medium", "High", "Ultra" }, self.qualityIndex, { x = 0, y = -220 })
	self.controls.fullscreen = self:CreateToggle(panel.transform, "Fullscreen", self.fullscreen, { x = 0, y = -290 })
	self.controls.masterVolume = self:CreateSlider(panel.transform, "Master Volume", self.masterVolume, { x = 0, y = -360 })
	self.controls.musicVolume = self:CreateSlider(panel.transform, "Music Volume", self.musicVolume, { x = 0, y = -430 })
	self.controls.sfxVolume = self:CreateSlider(panel.transform, "SFX Volume", self.sfxVolume, { x = 0, y = -500 })
	self:CreateBottomButtons(panel.transform)
	return panel
end

function SystemSettingsMenu:CreateDropdown(parent, name, options, selectedIndex, position)
	local control = DataDrivenGameTemplateGenerator.CreateChild(parent, name .. " Dropdown")
	control.options = options
	control.value = selectedIndex or 0
	DataDrivenGameTemplateGenerator.CreateUIText(control.transform, name .. " Label", name, 24, { x = (position.x or 0) - 260, y = position.y or 0 }, "MiddleLeft")
	DataDrivenGameTemplateGenerator.CreateUIText(control.transform, name .. " Value", options[(control.value or 0) + 1] or options[1], 24, { x = (position.x or 0) + 80, y = position.y or 0 }, "MiddleLeft")
	return control
end

function SystemSettingsMenu:CreateToggle(parent, name, value, position)
	local control = DataDrivenGameTemplateGenerator.CreateChild(parent, name .. " Toggle")
	control.value = value == true
	DataDrivenGameTemplateGenerator.CreateUIText(control.transform, name .. " Label", name, 24, { x = (position.x or 0) - 260, y = position.y or 0 }, "MiddleLeft")
	DataDrivenGameTemplateGenerator.CreateUIText(control.transform, name .. " Value", control.value and "On" or "Off", 24, { x = (position.x or 0) + 80, y = position.y or 0 }, "MiddleLeft")
	return control
end

function SystemSettingsMenu:CreateSlider(parent, name, value, position)
	local control = DataDrivenGameTemplateGenerator.CreateChild(parent, name .. " Slider")
	control.minValue = 0.0
	control.maxValue = 1.0
	control.value = self:Clamp01(value)
	DataDrivenGameTemplateGenerator.CreateUIText(control.transform, name .. " Label", name, 24, { x = (position.x or 0) - 260, y = position.y or 0 }, "MiddleLeft")
	DataDrivenGameTemplateGenerator.CreateUIText(control.transform, name .. " Value", tostring(math.floor(control.value * 100.0)) .. "%", 24, { x = (position.x or 0) + 80, y = position.y or 0 }, "MiddleLeft")
	return control
end

function SystemSettingsMenu:CreateBottomButtons(parent)
	DataDrivenGameTemplateGenerator.CreateUIText(parent, "Apply Button", "Apply", 26, { x = -160, y = -600 }, "MiddleCenter")
	DataDrivenGameTemplateGenerator.CreateUIText(parent, "Reset Button", "Reset", 26, { x = 0, y = -600 }, "MiddleCenter")
	DataDrivenGameTemplateGenerator.CreateUIText(parent, "Back Button", "Back", 26, { x = 160, y = -600 }, "MiddleCenter")
end

function SystemSettingsMenu:PopulateDropdowns()
	if self.controls.resolution then
		self:PopulateResolutionDropdown()
	end

	if self.controls.quality then
		self:PopulateQualityDropdown()
	end
end

function SystemSettingsMenu:PopulateResolutionDropdown()
	if not self.controls.resolution then
		return
	end

	self.controls.resolution.options = self.controls.resolution.options or { "Current" }
end

function SystemSettingsMenu:PopulateQualityDropdown()
	if not self.controls.quality then
		return
	end

	self.controls.quality.options = self.controls.quality.options or { "Low", "Medium", "High", "Ultra" }
end

function SystemSettingsMenu:LoadSettingsIntoUI()
	if self.controls.masterVolume then self.controls.masterVolume.value = self.masterVolume end
	if self.controls.musicVolume then self.controls.musicVolume.value = self.musicVolume end
	if self.controls.sfxVolume then self.controls.sfxVolume.value = self.sfxVolume end
	if self.controls.fullscreen then self.controls.fullscreen.value = self.fullscreen end
	if self.controls.quality then self.controls.quality.value = self.qualityIndex end
	if self.controls.resolution then self.controls.resolution.value = self.resolutionIndex end
end

function SystemSettingsMenu:ApplySettings()
	self:ApplyResolution()
	self:ApplyQuality()
	self:ApplyAudioOnly()
	self:SaveSettings()
end

function SystemSettingsMenu:ApplyResolution()
	local resolutionControl = self.controls.resolution
	local fullscreenControl = self.controls.fullscreen

	self.resolutionIndex = resolutionControl and resolutionControl.value or self.resolutionIndex
	self.fullscreen = fullscreenControl and fullscreenControl.value or self.fullscreen

	if Screen then
		Screen.fullScreen = self.fullscreen
	end
end

function SystemSettingsMenu:ApplyQuality()
	local qualityControl = self.controls.quality
	self.qualityIndex = qualityControl and qualityControl.value or self.qualityIndex

	if QualitySettings and QualitySettings.SetQualityLevel then
		QualitySettings.SetQualityLevel(self.qualityIndex)
	end
end

function SystemSettingsMenu:ApplyAudioOnly()
	self.masterVolume = self.controls.masterVolume and self:Clamp01(self.controls.masterVolume.value) or self.masterVolume
	self.musicVolume = self.controls.musicVolume and self:Clamp01(self.controls.musicVolume.value) or self.musicVolume
	self.sfxVolume = self.controls.sfxVolume and self:Clamp01(self.controls.sfxVolume.value) or self.sfxVolume

	self:SetMixerVolume("MasterVolume", self.masterVolume)
	self:SetMixerVolume("MusicVolume", self.musicVolume)
	self:SetMixerVolume("SFXVolume", self.sfxVolume)
end

function SystemSettingsMenu:SetMixerVolume(parameterName, linearValue)
	linearValue = self:Clamp01(linearValue)
	self.lastMixerValues = self.lastMixerValues or {}
	self.lastMixerValues[parameterName] = linearValue
end

function SystemSettingsMenu:SaveSettings()
	self.savedSettings =
	{
		masterVolume = self.masterVolume,
		musicVolume = self.musicVolume,
		sfxVolume = self.sfxVolume,
		qualityIndex = self.qualityIndex,
		resolutionIndex = self.resolutionIndex,
		fullscreen = self.fullscreen
	}
end

function SystemSettingsMenu:ResetSettings()
	self.masterVolume = 1.0
	self.musicVolume = 1.0
	self.sfxVolume = 1.0
	self.qualityIndex = 0
	self.resolutionIndex = 0
	self.fullscreen = true
	self:LoadSettingsIntoUI()
	self:ApplySettings()
end

function SystemSettingsMenu:HideSettings()
	if self.generatedRoot then
		self.generatedRoot.active = false
	end
end

function SystemSettingsMenu:ShowSettings()
	if self.generatedRoot then
		self.generatedRoot.active = true
	end
end

function SystemSettingsMenu:Clamp01(value)
	value = tonumber(value) or 0.0

	if value < 0.0 then
		return 0.0
	end

	if value > 1.0 then
		return 1.0
	end

	return value
end

