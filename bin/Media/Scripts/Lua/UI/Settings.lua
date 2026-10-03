include("UIDialog.lua")

class 'Settings' (UIDialog)

function Settings:__init(component)
	UIDialog.__init(self, component)
	self.component = component
	self.homeController = nil
	self.generatedRoot = nil
	self.graphicsPanel = nil
	self.audioPanel = nil
	self.graphicsButton = nil
	self.audioButton = nil
	self.exitButton = nil
end

function Settings:__finalize()
	UIDialog.__finalize(self)
end

function Settings:update()
end

function Settings:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setButtonPressed("Generate", false)
end

function Settings:setProperties(parameters)
	local properties = parameters:at(0)
	if properties:isButtonPressed("Generate") then
		self:generate()
	end
end

function Settings:setHomeController(value)
	self.homeController = value
end

function Settings:generate()
	local applicationManager = IApplicationManager.instance()
	local gameManager = applicationManager and applicationManager:getGameManager()
	local root = self.component and self.component:getActor() or self:getActor()
	if not gameManager or not root then
		print("Settings:generate failed: actor or game manager is unavailable")
		return nil
	end

	-- This is an editor action, so generating twice must not duplicate controls.
	root:destroyChildren()

	local white = ColourF(0.94, 0.97, 1.0, 1.0)
	local muted = ColourF(0.65, 0.72, 0.80, 1.0)
	local panelColour = ColourF(0.035, 0.055, 0.085, 0.98)
	local controlColour = ColourF(0.08, 0.13, 0.20, 1.0)

	local function createActor(parent, name)
		local actor = gameManager:createActor()
		actor:setName(name)
		parent:addChild(actor)
		return actor
	end

	local function addTransform(actor, position, size, zOrder)
		local transform = actor:addComponent("LayoutTransform")
		if transform then
			transform:setPosition(position)
			transform:setSize(size)
			transform:setZOrder(zOrder or 0)
		end
		return transform
	end

	local function addMaterial(actor)
		local material = actor:addComponent("Material")
		if material then material:setMaterialPath("DefaultUI.mat") end
		return material
	end

	local function createPanel(parent, name, position, size, colour, enabled)
		local actor = createActor(parent, name)
		addTransform(actor, position, size, 0)
		local image = actor:addComponent("Image")
		if image then image:setColour(colour) end
		addMaterial(actor)
		actor:setEnabled(enabled ~= false)
		return actor, image
	end

	local function createText(parent, name, value, position, size, colour)
		local actor = createActor(parent, name)
		addTransform(actor, position, size, 2)
		local text = actor:addComponent("Text")
		if text then
			text:setText(value)
			text:setColour(colour or white)
			text:setHorizontalAlignment(1)
			text:setVerticalAlignment(1)
		end
		return actor, text
	end

	local function createButton(parent, name, label, position, size)
		local actor = createActor(parent, name)
		addTransform(actor, position, size or Vector2F(220.0, 64.0), 1)
		local image = actor:addComponent("Image")
		if image then image:setColour(controlColour) end
		local button = actor:addComponent("Button")
		if button then
			button:setTextStr(label)
			button:setTextSize(24)
			button:setNormalColour(ColourF(0.10, 0.18, 0.28, 1.0))
			button:setHighlightedColour(ColourF(0.18, 0.42, 0.68, 1.0))
			button:setPressedColour(ColourF(0.07, 0.28, 0.48, 1.0))
			button:setDisabledColour(ColourF(0.12, 0.12, 0.12, 0.6))
		end
		addMaterial(actor)
		createText(actor, "Text", label, Vector2F(0.0, 0.0),
			Vector2F(200.0, 52.0), white)
		return actor, button
	end

	local function createSettingRow(parent, name, label, value, y)
		local row = createActor(parent, name)
		addTransform(row, Vector2F(0.0, y), Vector2F(760.0, 58.0), 1)
		createText(row, "Label", label, Vector2F(-210.0, 0.0),
			Vector2F(300.0, 48.0), muted)
		local valueActor, valueButton = createButton(row, "Value", value,
			Vector2F(220.0, 0.0), Vector2F(280.0, 52.0))
		return row, valueActor, valueButton
	end

	-- Dialog shell and tab bar.
	local dialog = createPanel(root, "SettingsDialog", Vector2F(0.0, 0.0),
		Vector2F(1080.0, 820.0), panelColour, true)
	createText(dialog, "Title", "Settings", Vector2F(0.0, -340.0),
		Vector2F(600.0, 90.0), white)

	self.graphicsButtonActor, self.graphicsButton = createButton(
		dialog, "GraphicsButton", "Graphics", Vector2F(-135.0, -260.0), Vector2F(250.0, 64.0))
	self.audioButtonActor, self.audioButton = createButton(
		dialog, "AudioButton", "Audio", Vector2F(135.0, -260.0), Vector2F(250.0, 64.0))
	self.exitButtonActor, self.exitButton = createButton(
		dialog, "ExitButton", "Back", Vector2F(0.0, 340.0), Vector2F(250.0, 64.0))

	self.graphicsPanel = createPanel(dialog, "GraphicsPanel", Vector2F(0.0, 35.0),
		Vector2F(900.0, 500.0), ColourF(0.025, 0.04, 0.065, 0.96), true)
	createSettingRow(self.graphicsPanel, "Resolution", "Resolution", "1920 x 1080", -150.0)
	createSettingRow(self.graphicsPanel, "Quality", "Quality", "High", -70.0)
	createSettingRow(self.graphicsPanel, "Fullscreen", "Fullscreen", "On", 10.0)
	createSettingRow(self.graphicsPanel, "VSync", "Vertical Sync", "On", 90.0)
	createSettingRow(self.graphicsPanel, "AntiAliasing", "Anti-aliasing", "4x", 170.0)

	self.audioPanel = createPanel(dialog, "AudioPanel", Vector2F(0.0, 35.0),
		Vector2F(900.0, 500.0), ColourF(0.025, 0.04, 0.065, 0.96), false)
	createSettingRow(self.audioPanel, "MasterVolume", "Master Volume", "100%", -110.0)
	createSettingRow(self.audioPanel, "MusicVolume", "Music Volume", "100%", -20.0)
	createSettingRow(self.audioPanel, "SfxVolume", "Effects Volume", "100%", 70.0)
	createSettingRow(self.audioPanel, "Mute", "Mute All", "Off", 160.0)

	self.generatedRoot = dialog
	return dialog
end

function Settings:showGraphics()
	if self.graphicsPanel then self.graphicsPanel:setEnabled(true) end
	if self.audioPanel then self.audioPanel:setEnabled(false) end
end

function Settings:showAudio()
	if self.graphicsPanel then self.graphicsPanel:setEnabled(false) end
	if self.audioPanel then self.audioPanel:setEnabled(true) end
end

function Settings:exitSettings()
	if self.homeController then
		local callback = self.homeController.setHomeState or self.homeController.SetHomeState
		if callback then callback(self.homeController, 0) end
	elseif self.generatedRoot then
		self.generatedRoot:setEnabled(false)
	end
end

function Settings:handleEvent(parameters, results)
	if not parameters then return end
	local eventHash = parameters:at(1)
	if eventHash ~= IEvent.CLICK_HASH and eventHash ~= IEvent.ACTIVATE_HASH then return end

	local sender = parameters:at(3)
	if not sender then return end
	local actor = sender
	if sender.getActor then actor = sender:getActor() end
	if not actor or not actor.getName then return end

	local name = actor:getName()
	if name == "GraphicsButton" then
		self:showGraphics()
	elseif name == "AudioButton" then
		self:showAudio()
	elseif name == "ExitButton" then
		self:exitSettings()
	end
end
