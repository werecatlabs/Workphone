class 'InputManager' (BaseEditor)

InputManagerTypes =
{
	None = 0,

	-- Action mapping buttons
	AddAction    = 1,
	RemoveAction = 2,

	-- Axis mapping buttons
	AddAxis    = 3,
	RemoveAxis = 4,

	-- Device buttons
	RefreshDevices = 5,
	CreateMouse = 8,
	CreateKeyboard = 9,
	CreateJoysticks = 10,
	CursorVisible = 11,

	-- Settings buttons
	SaveSettings  = 6,
	ResetSettings = 7,
}

-- Helper: create and register a separator text label
local function addSeparatorLabel(ui, parent, labelText)
	local textTypeInfo = IUIText.typeInfo();
	local sep = ui:addElement(textTypeInfo);
	sep:setText("-- " .. labelText .. " --");
	sep:setSameLine(false);
	parent:addChild(sep);
	return sep;
end

-- Helper: build a labeled slider and attach it to parent
local function addSlider(ui, parent, label, minVal, maxVal, defaultVal)
	local sliderPairTypeInfo = IUILabelSliderPair.typeInfo();
	local slider = ui:addElement(sliderPairTypeInfo);
	slider:setLabel(label);
	slider:setMinValue(minVal);
	slider:setMaxValue(maxVal);
	slider:setValue(defaultVal);
	parent:addChild(slider);
	return slider;
end

-- Helper: build a labeled button and attach it to parent
local function addButton(ui, parent, label, elementId, sameLine)
	local buttonTypeInfo = IUIButton.typeInfo();
	local btn = ui:addElement(buttonTypeInfo);
	btn:setLabel(label);
	if elementId then btn:setElementId(elementId); end
	btn:setSameLine(sameLine == true);
	parent:addChild(btn);
	return btn;
end

-- Helper: build a labeled toggle and attach it to parent
local function addToggle(ui, parent, label, elementId, sameLine)
	local toggleTypeInfo = IUILabelTogglePair.typeInfo();
	local toggle = ui:addElement(toggleTypeInfo);
	toggle:setLabel(label);
	if elementId then toggle:setElementId(elementId); end
	toggle:setSameLine(sameLine == true);
	parent:addChild(toggle);
	return toggle;
end

-- Helper: create a text row and attach it to parent
local function addText(ui, parent, text)
	local textTypeInfo = IUIText.typeInfo();
	local label = ui:addElement(textTypeInfo);
	label:setText(text);
	label:setSameLine(false);
	parent:addChild(label);
	return label;
end

function InputManager:__init(window)
	print("InputManager constructor called");

	self.window = window;

	-- top-level container
	self.editorWindow = nil;
	self.tabBar       = nil;

	-- Actions tab
	self.addActionButton      = nil;
	self.removeActionButton   = nil;
	self.actionListWindow     = nil;
	self.actionAdvancedHeader = nil;
	self.actionDeadZoneSlider = nil;

	-- Axes tab
	self.addAxisButton         = nil;
	self.removeAxisButton      = nil;
	self.axisListWindow        = nil;
	self.axisAdvancedHeader    = nil;
	self.axisSensitivitySlider = nil;
	self.axisDeadZoneSlider    = nil;
	self.axisGravitySlider     = nil;
	self.invertAxisDropdown    = nil;

	-- Devices tab
	self.deviceListWindow         = nil;
	self.refreshDevicesButton     = nil;
	self.deviceAdvancedHeader     = nil;
	self.createMouseToggle        = nil;
	self.createKeyboardToggle     = nil;
	self.createJoysticksToggle    = nil;
	self.cursorVisibleToggle      = nil;
	self.mouseSpeedSlider         = nil;
	self.controllerDeadZoneSlider = nil;
	self.deviceSignature          = nil;
	self.joystickDisplays         = nil;

	-- Settings tab
	self.inputSchemeDropdown    = nil;
	self.settingsAdvancedHeader = nil;
	self.repeatDelaySlider      = nil;
	self.repeatRateSlider       = nil;
	self.saveSettingsButton     = nil;
	self.resetSettingsButton    = nil;
end

function InputManager:__finalize()
	print("InputManager __finalize called");
	self:_clearRefs();
	self.window = nil;
end

-- Centralised nil-out so both __finalize and unload stay DRY
function InputManager:_clearRefs()
	self.editorWindow = nil;
	self.tabBar       = nil;

	self.addActionButton      = nil;
	self.removeActionButton   = nil;
	self.actionListWindow     = nil;
	self.actionAdvancedHeader = nil;
	self.actionDeadZoneSlider = nil;

	self.addAxisButton         = nil;
	self.removeAxisButton      = nil;
	self.axisListWindow        = nil;
	self.axisAdvancedHeader    = nil;
	self.axisSensitivitySlider = nil;
	self.axisDeadZoneSlider    = nil;
	self.axisGravitySlider     = nil;
	self.invertAxisDropdown    = nil;

	self.deviceListWindow         = nil;
	self.refreshDevicesButton     = nil;
	self.deviceAdvancedHeader     = nil;
	self.createMouseToggle        = nil;
	self.createKeyboardToggle     = nil;
	self.createJoysticksToggle    = nil;
	self.cursorVisibleToggle      = nil;
	self.mouseSpeedSlider         = nil;
	self.controllerDeadZoneSlider = nil;
	self.deviceSignature          = nil;
	self.joystickDisplays         = nil;

	self.inputSchemeDropdown    = nil;
	self.settingsAdvancedHeader = nil;
	self.repeatDelaySlider      = nil;
	self.repeatRateSlider       = nil;
	self.saveSettingsButton     = nil;
	self.resetSettingsButton    = nil;
end

function InputManager:load()
	print("InputManager load start");

	local windowTypeInfo           = IUIWindow.typeInfo();
	local dropdownTypeInfo         = IUIDropdown.typeInfo();
	local tabBarTypeInfo           = IUITabBar.typeInfo();
	local collapsingHeaderTypeInfo = IUICollapsingHeader.typeInfo();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	local parentWindow = self.window:getParentWindow();

	-- ── Top-level container ───────────────────────────────────────────────
	self.editorWindow = ui:addElement(windowTypeInfo);
	self.editorWindow:setLabel("Input Manager");
	self.editorWindow:setSize(Vector2F(620.0, 480.0));
	parentWindow:addChild(self.editorWindow);

	-- ── Tab bar ──────────────────────────────────────────────────────────
	self.tabBar = ui:addElement(tabBarTypeInfo);
	local actionsTabItem  = self.tabBar:addTabItem();  actionsTabItem:setLabel("Actions");
	local axesTabItem     = self.tabBar:addTabItem();  axesTabItem:setLabel("Axes");
	local devicesTabItem  = self.tabBar:addTabItem();  devicesTabItem:setLabel("Devices");
	local settingsTabItem = self.tabBar:addTabItem();  settingsTabItem:setLabel("Settings");
	self.editorWindow:addChild(self.tabBar);

	-- ════════════════════════════════════════════════════════════════════
	-- ACTIONS TAB
	-- ════════════════════════════════════════════════════════════════════
	addSeparatorLabel(ui, actionsTabItem, "Action Mappings");

	self.addActionButton    = addButton(ui, actionsTabItem, "Add Action",
		InputManagerTypes.AddAction, false);
	self.removeActionButton = addButton(ui, actionsTabItem, "Remove Action",
		InputManagerTypes.RemoveAction, true);

	self.actionListWindow = ui:addElement(windowTypeInfo);
	self.actionListWindow:setSize(Vector2F(560.0, 220.0));
	actionsTabItem:addChild(self.actionListWindow);

	self.actionAdvancedHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.actionAdvancedHeader:setLabel("Advanced Settings");
	actionsTabItem:addChild(self.actionAdvancedHeader);

	self.actionDeadZoneSlider = addSlider(ui, self.actionAdvancedHeader,
		"Dead Zone", 0.0, 1.0, 0.1);

	-- ════════════════════════════════════════════════════════════════════
	-- AXES TAB
	-- ════════════════════════════════════════════════════════════════════
	addSeparatorLabel(ui, axesTabItem, "Axis Mappings");

	self.addAxisButton    = addButton(ui, axesTabItem, "Add Axis",
		InputManagerTypes.AddAxis, false);
	self.removeAxisButton = addButton(ui, axesTabItem, "Remove Axis",
		InputManagerTypes.RemoveAxis, true);

	self.axisListWindow = ui:addElement(windowTypeInfo);
	self.axisListWindow:setSize(Vector2F(560.0, 180.0));
	axesTabItem:addChild(self.axisListWindow);

	self.axisAdvancedHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.axisAdvancedHeader:setLabel("Advanced Settings");
	axesTabItem:addChild(self.axisAdvancedHeader);

	self.axisSensitivitySlider = addSlider(ui, self.axisAdvancedHeader,
		"Sensitivity", 0.0, 10.0, 1.0);
	self.axisDeadZoneSlider    = addSlider(ui, self.axisAdvancedHeader,
		"Dead Zone", 0.0, 1.0, 0.1);
	self.axisGravitySlider     = addSlider(ui, self.axisAdvancedHeader,
		"Gravity", 0.0, 10.0, 3.0);

	local invertOptions = Parameters();
	invertOptions:push_back("Normal");
	invertOptions:push_back("Inverted");
	self.invertAxisDropdown = ui:addElement(dropdownTypeInfo);
	self.invertAxisDropdown:setLabel("Invert");
	self.invertAxisDropdown:setOptions(invertOptions:getAsStringArray());
	self.invertAxisDropdown:setSelectedOption(0);
	self.axisAdvancedHeader:addChild(self.invertAxisDropdown);

	-- ════════════════════════════════════════════════════════════════════
	-- DEVICES TAB
	-- ════════════════════════════════════════════════════════════════════
	addSeparatorLabel(ui, devicesTabItem, "Connected Devices");

	self.deviceListWindow = ui:addElement(windowTypeInfo);
	self.deviceListWindow:setSize(Vector2F(560.0, 220.0));
	devicesTabItem:addChild(self.deviceListWindow);

	self.refreshDevicesButton = addButton(ui, devicesTabItem, "Refresh Devices",
		InputManagerTypes.RefreshDevices, false);

	addSeparatorLabel(ui, devicesTabItem, "Device Controls");

	self.createMouseToggle = addToggle(ui, devicesTabItem, "Create Mouse",
		InputManagerTypes.CreateMouse, false);
	self.createKeyboardToggle = addToggle(ui, devicesTabItem, "Create Keyboard",
		InputManagerTypes.CreateKeyboard, true);
	self.createJoysticksToggle = addToggle(ui, devicesTabItem, "Create Joysticks",
		InputManagerTypes.CreateJoysticks, false);
	self.cursorVisibleToggle = addToggle(ui, devicesTabItem, "Cursor Visible",
		InputManagerTypes.CursorVisible, true);

	self.deviceAdvancedHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.deviceAdvancedHeader:setLabel("Advanced Settings");
	devicesTabItem:addChild(self.deviceAdvancedHeader);

	self.mouseSpeedSlider         = addSlider(ui, self.deviceAdvancedHeader,
		"Joystick Sensitivity", 0.1, 10.0, 1.0);
	self.controllerDeadZoneSlider = addSlider(ui, self.deviceAdvancedHeader,
		"Controller Dead Zone", 0.0, 1.0, 0.15);

	-- ════════════════════════════════════════════════════════════════════
	-- SETTINGS TAB
	-- ════════════════════════════════════════════════════════════════════
	addSeparatorLabel(ui, settingsTabItem, "Input Scheme");

	local schemeOptions = Parameters();
	schemeOptions:push_back("Default");
	schemeOptions:push_back("FPS");
	schemeOptions:push_back("RTS");
	schemeOptions:push_back("Custom");
	self.inputSchemeDropdown = ui:addElement(dropdownTypeInfo);
	self.inputSchemeDropdown:setLabel("Scheme");
	self.inputSchemeDropdown:setOptions(schemeOptions:getAsStringArray());
	self.inputSchemeDropdown:setSelectedOption(0);
	settingsTabItem:addChild(self.inputSchemeDropdown);

	self.settingsAdvancedHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.settingsAdvancedHeader:setLabel("Advanced Settings");
	settingsTabItem:addChild(self.settingsAdvancedHeader);

	self.repeatDelaySlider = addSlider(ui, self.settingsAdvancedHeader,
		"Key Repeat Delay (ms)", 100, 1000, 500);
	self.repeatRateSlider  = addSlider(ui, self.settingsAdvancedHeader,
		"Key Repeat Rate (ms)", 10, 200, 50);

	addSeparatorLabel(ui, settingsTabItem, "Save / Reset");

	self.saveSettingsButton  = addButton(ui, settingsTabItem, "Save Settings",
		InputManagerTypes.SaveSettings, false);
	self.resetSettingsButton = addButton(ui, settingsTabItem, "Reset to Defaults",
		InputManagerTypes.ResetSettings, true);

	-- ── Register event handlers ───────────────────────────────────────
	self.window:setHandleEvents(self.addActionButton,      true);
	self.window:setHandleEvents(self.removeActionButton,   true);
	self.window:setHandleEvents(self.addAxisButton,        true);
	self.window:setHandleEvents(self.removeAxisButton,     true);
	self.window:setHandleEvents(self.refreshDevicesButton, true);
	self.window:setHandleEvents(self.createMouseToggle,    true);
	self.window:setHandleEvents(self.createKeyboardToggle, true);
	self.window:setHandleEvents(self.createJoysticksToggle,true);
	self.window:setHandleEvents(self.cursorVisibleToggle,  true);
	self.window:setHandleEvents(self.mouseSpeedSlider,     true);
	self.window:setHandleEvents(self.controllerDeadZoneSlider, true);
	self.window:setHandleEvents(self.saveSettingsButton,   true);
	self.window:setHandleEvents(self.resetSettingsButton,  true);

	self:refreshDevices();

	print("InputManager load end");
end

function InputManager:unload()
	print("InputManager unload called");

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
		self.editorWindow:destroyAllChildren();

		local editorParent = self.editorWindow:getParent();
		if editorParent then
			editorParent:removeChild(self.editorWindow);
		end

		ui:removeElement(self.editorWindow);
	end

	self:_clearRefs();
end

function InputManager:show()
	print("InputManager show called");

	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(true, false);
	end

	self:refreshDevices();
end

function InputManager:hide()
	print("InputManager hide called");

	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(false, false);
	end
end

function InputManager:update()
	if self.deviceListWindow == nil then
		return;
	end

	local inputDeviceManager = self:getInputDeviceManager();
	if inputDeviceManager == nil then
		if self.deviceSignature ~= "unavailable" then
			self:refreshDevices();
		end
		return;
	end

	local connectedJoysticks, signature = self:getConnectedJoysticks(inputDeviceManager);
	if signature ~= self.deviceSignature then
		self:refreshDevices();
		return;
	end

	if self.joystickDisplays == nil then
		return;
	end

	for i = 1, #self.joystickDisplays do
		self:updateJoystickDisplay(self.joystickDisplays[i]);
	end
end

function InputManager:getInputDeviceManager()
	local applicationManager = IApplicationManager.instance();
	if applicationManager == nil then
		return nil;
	end

	return applicationManager:getInputDeviceManager();
end

function InputManager:syncDeviceControls()
	local inputDeviceManager = self:getInputDeviceManager();
	if inputDeviceManager == nil then
		return;
	end

	if self.createMouseToggle then
		self.createMouseToggle:setValue(inputDeviceManager:getCreateMouse());
	end

	if self.createKeyboardToggle then
		self.createKeyboardToggle:setValue(inputDeviceManager:getCreateKeyboard());
	end

	if self.createJoysticksToggle then
		self.createJoysticksToggle:setValue(inputDeviceManager:getCreateJoysticks());
	end

	if self.cursorVisibleToggle then
		self.cursorVisibleToggle:setValue(inputDeviceManager:isCursorVisible());
	end
end

function InputManager:applyJoystickSettings()
	local inputDeviceManager = self:getInputDeviceManager();
	if inputDeviceManager == nil then
		return;
	end

	local joysticks = inputDeviceManager:getJoysticks();
	if joysticks == nil then
		return;
	end

	local deadZone = 0.15;
	if self.controllerDeadZoneSlider then
		deadZone = self.controllerDeadZoneSlider:getValue();
	end

	local sensitivity = 1.0;
	if self.mouseSpeedSlider then
		sensitivity = self.mouseSpeedSlider:getValue();
	end

	local count = joysticks:size();
	for i = 0, count - 1 do
		local joystick = joysticks:at(i);
		if joystick then
			joystick:setDeadZone(deadZone);
			joystick:setSensitivity(sensitivity);
		end
	end
end

function InputManager:getConnectedJoysticks(inputDeviceManager)
	local connectedJoysticks = {};
	local signatureParts = {};

	if inputDeviceManager == nil or not inputDeviceManager:getCreateJoysticks() then
		return connectedJoysticks, "disabled";
	end

	local joysticks = inputDeviceManager:getJoysticks();
	if joysticks == nil then
		return connectedJoysticks, "none";
	end

	for i = 0, joysticks:size() - 1 do
		local joystick = joysticks:at(i);
		if joystick then
			local numAxes = joystick:getNumAxes();
			local numButtons = joystick:getNumButtons();
			if numAxes > 0 or numButtons > 0 then
				connectedJoysticks[#connectedJoysticks + 1] =
				{
					index = i,
					joystick = joystick,
				};
				signatureParts[#signatureParts + 1] =
					string.format("%d:%d:%d", i, numAxes, numButtons);
			end
		end
	end

	return connectedJoysticks, table.concat(signatureParts, ",");
end

function InputManager:updateJoystickDisplay(display)
	if display == nil or display.joystick == nil then
		return;
	end

	local joystick = display.joystick;
	for axis = 0, joystick:getNumAxes() - 1 do
		local axisText = display.axisTexts[axis + 1];
		if axisText then
			axisText:setText(
				string.format("%s: %.2f", joystick:getAxisName(axis), joystick:getAxis(axis)));
		end
	end

	local pressedButtons = "";
	for button = 0, joystick:getNumButtons() - 1 do
		if joystick:isButtonDown(button) then
			if pressedButtons ~= "" then pressedButtons = pressedButtons .. ", "; end
			pressedButtons = pressedButtons .. joystick:getButtonName(button);
		end
	end

	if pressedButtons == "" then
		pressedButtons = "none";
	end

	if display.buttonsText then
		display.buttonsText:setText("Pressed Buttons: " .. pressedButtons);
	end
end

function InputManager:refreshDevices()
	if self.deviceListWindow == nil then
		return;
	end

	local applicationManager = IApplicationManager.instance();
	if applicationManager == nil then
		return;
	end

	local ui = applicationManager:getUI();
	self.deviceListWindow:destroyAllChildren();
	self.joystickDisplays = {};

	local inputDeviceManager = applicationManager:getInputDeviceManager();
	if inputDeviceManager == nil then
		self.deviceSignature = "unavailable";
		addText(ui, self.deviceListWindow, "Input device manager is not available.");
		return;
	end

	self:syncDeviceControls();
	self:applyJoystickSettings();

	local mouseState = "disabled";
	if inputDeviceManager:getCreateMouse() then mouseState = "enabled"; end
	addText(ui, self.deviceListWindow, "Mouse: " .. mouseState);

	local keyboardState = "disabled";
	if inputDeviceManager:getCreateKeyboard() then keyboardState = "enabled"; end
	addText(ui, self.deviceListWindow, "Keyboard: " .. keyboardState);

	local connectedJoysticks, signature = self:getConnectedJoysticks(inputDeviceManager);
	self.deviceSignature = signature;
	if #connectedJoysticks == 0 then
		addText(ui, self.deviceListWindow, "Joysticks: none connected");
		return;
	end

	addText(ui, self.deviceListWindow, "Joysticks: " .. #connectedJoysticks .. " connected");

	for i = 1, #connectedJoysticks do
		local joystickInfo = connectedJoysticks[i];
		local joystick = joystickInfo.joystick;
		if joystick then
			local name = joystick:getName();
			if name == nil or name == "" then
				name = "Joystick " .. joystickInfo.index;
			end

			addSeparatorLabel(ui, self.deviceListWindow, name);
			addText(ui, self.deviceListWindow,
				string.format("Axes: %d   Buttons: %d   Dead Zone: %.2f   Sensitivity: %.2f",
					joystick:getNumAxes(), joystick:getNumButtons(),
					joystick:getDeadZone(), joystick:getSensitivity()));

			local display =
			{
				joystick = joystick,
				axisTexts = {},
				buttonsText = nil,
			};

			for axis = 0, joystick:getNumAxes() - 1 do
				display.axisTexts[axis + 1] = addText(ui, self.deviceListWindow, "");
			end

			display.buttonsText = addText(ui, self.deviceListWindow, "");
			self.joystickDisplays[#self.joystickDisplays + 1] = display;
			self:updateJoystickDisplay(display);
		end
	end
end

function InputManager:handleEvent(parameters, results)
	print("InputManager handleEvent called");

	if parameters == nil then
		print("InputManager parameters nil");
		return;
	end

	local eventHash = parameters:at(1);
	local args      = parameters:at(2);
	local sender    = parameters:at(3);
	if sender == nil then
		return;
	end

	local elementId = sender:getElementId();

	if eventHash == IEvent.handleSelection then
		if elementId == InputManagerTypes.AddAction then
			print("InputManager handleEvent AddAction");
		elseif elementId == InputManagerTypes.RemoveAction then
			print("InputManager handleEvent RemoveAction");
		elseif elementId == InputManagerTypes.AddAxis then
			print("InputManager handleEvent AddAxis");
		elseif elementId == InputManagerTypes.RemoveAxis then
			print("InputManager handleEvent RemoveAxis");
		elseif elementId == InputManagerTypes.RefreshDevices then
			print("InputManager handleEvent RefreshDevices");
			self:refreshDevices();
		elseif elementId == InputManagerTypes.SaveSettings then
			print("InputManager handleEvent SaveSettings");
		elseif elementId == InputManagerTypes.ResetSettings then
			print("InputManager handleEvent ResetSettings");
		end
	elseif eventHash == IEvent.handleValueChanged then
		local inputDeviceManager = self:getInputDeviceManager();
		if inputDeviceManager == nil then
			return;
		end

		if elementId == InputManagerTypes.CreateMouse then
			inputDeviceManager:setCreateMouse(sender:getValue());
			self:refreshDevices();
		elseif elementId == InputManagerTypes.CreateKeyboard then
			inputDeviceManager:setCreateKeyboard(sender:getValue());
			self:refreshDevices();
		elseif elementId == InputManagerTypes.CreateJoysticks then
			inputDeviceManager:setCreateJoysticks(sender:getValue());
			self:refreshDevices();
		elseif elementId == InputManagerTypes.CursorVisible then
			inputDeviceManager:setCursorVisible(sender:getValue());
			self:refreshDevices();
		elseif sender == self.mouseSpeedSlider or sender == self.controllerDeadZoneSlider then
			self:applyJoystickSettings();
			self:refreshDevices();
		end
	end
end
