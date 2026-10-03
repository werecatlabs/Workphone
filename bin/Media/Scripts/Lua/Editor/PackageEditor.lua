class 'PackageEditor' (BaseEditor)

PackageEditorTypes = {
	PackageTexturesSwitch = 1,
	BuildTypeDropdown = 2,
	OutputPathEntry = 3,
	WindowsBuildSwitch = 4,
	MacOSSwitch = 5,
	IOSSwitch = 6,
	AndroidSwitch = 7,
	CompressionSlider = 8,
	CustomVersionEntry = 9,
	PackageButton = 10,
}

function PackageEditor:__init(window)
	print("PackageEditor constructor called");

	self.window = window;
	self.editorWindow = nil;
	self.tabBar = nil;
	self.generalTab = nil;
	self.platformsTab = nil;
	self.advancedTab = nil;
	self.generalHeader = nil;
	self.windowsHeader = nil;
	self.macosHeader = nil;
	self.iosHeader = nil;
	self.androidHeader = nil;
	self.advancedHeader = nil;
	self.packageButton = nil;
	self.packageTexturesSwitch = nil;
	self.buildTypeDropdown = nil;
	self.outputPathEntry = nil;
	self.testText = nil;
end

function PackageEditor:__finalize()
	print("PackageEditor __finalize called");

	self.window = nil;
	self.editorWindow = nil;
	self.tabBar = nil;
	self.generalTab = nil;
	self.platformsTab = nil;
	self.advancedTab = nil;
	self.generalHeader = nil;
	self.windowsHeader = nil;
	self.macosHeader = nil;
	self.iosHeader = nil;
	self.androidHeader = nil;
	self.advancedHeader = nil;
	self.packageTexturesSwitch = nil;
	self.buildTypeDropdown = nil;
	self.outputPathEntry = nil;
	self.packageButton = nil;
	self.compressionSlider = nil;
	self.customVersionEntry = nil;
	self.advancedPropertyGrid = nil;
	self.testText = nil;
end

function PackageEditor:load()
	print("PackageEditor load start");

	local windowTypeInfo = IUIWindow.typeInfo();
	local dropdownTypeInfo = IUIDropdown.typeInfo();
	local buttonTypeInfo = IUIButton.typeInfo();
	local imageTypeInfo = IUIImage.typeInfo();
	local textTypeInfo = IUIText.typeInfo();
	local checkboxTypeInfo = IUICheckbox.typeInfo();
	local tabBarTypeInfo = IUITabBar.typeInfo();
	local toggleSwitchTypeInfo = IUILabelTogglePair.typeInfo();
	local collapsingHeaderTypeInfo = IUICollapsingHeader.typeInfo();
	local sliderPairTypeInfo = IUILabelSliderPair.typeInfo();
	local textEntryTypeInfo = IUITextEntry.typeInfo();
	local propertyGridTypeInfo = IUIPropertyGrid.typeInfo();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	local packageManager = applicationManager:getPackageManager();

	local parent = self.window:getParent();
	local parentWindow = self.window:getParentWindow();

	self.editorWindow = ui:addElement(windowTypeInfo);
	self.editorWindow:setLabel("Editor window");
	self.editorWindow:setSize(Vector2F(600.0, 400.0));
	parentWindow:addChild(self.editorWindow);

	-- Tab bar setup
	self.tabBar = ui:addElement(tabBarTypeInfo);
	self.generalTab = self.tabBar:addTabItem();
	self.generalTab:setLabel("General");
	self.platformsTab = self.tabBar:addTabItem();
	self.platformsTab:setLabel("Platforms");
	self.advancedTab = self.tabBar:addTabItem();
	self.advancedTab:setLabel("Advanced");
	self.editorWindow:addChild(self.tabBar);

	-- General Tab
	self.generalHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.generalHeader:setLabel("General Settings");
	self.generalTab:addChild(self.generalHeader);

	self.packageTexturesSwitch = ui:addElement(toggleSwitchTypeInfo);
	self.packageTexturesSwitch:setLabel("Package Textures");
	self.packageTexturesSwitch:setElementId(PackageEditorTypes.PackageTexturesSwitch);
	self.generalHeader:addChild(self.packageTexturesSwitch);
	self.window:setHandleEvents(self.packageTexturesSwitch, true);

	self.buildTypeDropdown = ui:addElement(dropdownTypeInfo);
	self.buildTypeDropdown:setLabel("Build Type");
	self.buildTypeDropdown:addOption("Debug");
	self.buildTypeDropdown:addOption("Release");
	self.buildTypeDropdown:setSelectedOption(1); 
	self.buildTypeDropdown:setSameLine(false);
	self.buildTypeDropdown:setElementId(PackageEditorTypes.BuildTypeDropdown);
	self.generalHeader:addChild(self.buildTypeDropdown);
	self.window:setHandleEvents(self.buildTypeDropdown, true);

	self.outputPathEntry = ui:addElement(textEntryTypeInfo);
	self.outputPathEntry:setLabel("Output Path");
	self.outputPathEntry:setElementId(PackageEditorTypes.OutputPathEntry);
	self.outputPathEntry:setSameLine(false);
	self.generalHeader:addChild(self.outputPathEntry);
	self.window:setHandleEvents(self.outputPathEntry, true);

	-- Platforms Tab
	self.windowsHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.windowsHeader:setLabel("Windows");
	self.platformsTab:addChild(self.windowsHeader);

	self.windowsBuildSwitch = ui:addElement(toggleSwitchTypeInfo);
	self.windowsBuildSwitch:setLabel("Enable Windows Build");
	self.windowsBuildSwitch:setElementId(PackageEditorTypes.WindowsBuildSwitch);
	self.windowsHeader:addChild(self.windowsBuildSwitch);
	self.window:setHandleEvents(self.windowsBuildSwitch, true);

	self.macosHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.macosHeader:setLabel("MacOS");
	self.platformsTab:addChild(self.macosHeader);

	self.macosSwitch = ui:addElement(toggleSwitchTypeInfo);
	self.macosSwitch:setLabel("Enable macOS Build");
	self.macosSwitch:setElementId(PackageEditorTypes.MacOSSwitch);
	self.macosHeader:addChild(self.macosSwitch);
	self.window:setHandleEvents(self.macosSwitch, true);

	self.iosHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.iosHeader:setLabel("iOS");
	self.platformsTab:addChild(self.iosHeader);

	self.iosSwitch = ui:addElement(toggleSwitchTypeInfo);
	self.iosSwitch:setLabel("Enable iOS Build");
	self.iosSwitch:setElementId(PackageEditorTypes.IOSSwitch);
	self.iosHeader:addChild(self.iosSwitch);
	self.window:setHandleEvents(self.iosSwitch, true);

	self.androidHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.androidHeader:setLabel("Android");
	self.platformsTab:addChild(self.androidHeader);

	self.androidSwitch = ui:addElement(toggleSwitchTypeInfo);
	self.androidSwitch:setLabel("Enable Android Build");
	self.androidSwitch:setElementId(PackageEditorTypes.AndroidSwitch);
	self.androidHeader:addChild(self.androidSwitch);
	self.window:setHandleEvents(self.androidSwitch, true);

	-- Advanced Tab
	self.advancedHeader = ui:addElement(collapsingHeaderTypeInfo);
	self.advancedHeader:setLabel("Advanced Settings");
	self.advancedTab:addChild(self.advancedHeader);

	self.compressionSlider = ui:addElement(sliderPairTypeInfo);
	self.compressionSlider:setLabel("Compression Level");
	self.compressionSlider:setMinValue(0);
	self.compressionSlider:setMaxValue(9);
	self.compressionSlider:setValue(5);
	self.compressionSlider:setElementId(PackageEditorTypes.CompressionSlider);
	self.advancedHeader:addChild(self.compressionSlider);
	self.window:setHandleEvents(self.compressionSlider, true);

	self.customVersionEntry = ui:addElement(textEntryTypeInfo);
	self.customVersionEntry:setLabel("Custom Version");
	self.customVersionEntry:setElementId(PackageEditorTypes.CustomVersionEntry);
	self.advancedHeader:addChild(self.customVersionEntry);
	self.window:setHandleEvents(self.customVersionEntry, true);

	self.advancedPropertyGrid = ui:addElement(propertyGridTypeInfo);
	self.advancedHeader:addChild(self.advancedPropertyGrid);

	--self.advancedProperties = factoryManager:createById(Properties.typeInfo());
	--self.advancedProperties:setPropertyAsString("exampleKey", "exampleValue");
	--self.advancedPropertyGrid:setProperties(self.advancedProperties);

	self.packageButton = ui:addElement(buttonTypeInfo);
	self.packageButton:setLabel("Package");
	self.packageButton:setElementId(PackageEditorTypes.PackageButton);
	self.editorWindow:addChild(self.packageButton);
	self.window:setHandleEvents(self.packageButton, true);

	--local value = packageManager:getPackageTextures();
	--self.packageTexturesSwitch:setValue(value);

	print("PackageEditor load end");
end

function PackageEditor:unload()
	print("PackageEditor unload called");

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	
	if (self.editorWindow) then
		print("PackageEditor unload self.editorWindow");
	
		self.editorWindow:setVisible(false, false);
		self.editorWindow:destroyAllChildren();
		
		local editorParent = self.editorWindow:getParent();
		if (editorParent) then
			editorParent:removeChild(self.editorWindow);			
		end	
		
		ui:removeElement(self.editorWindow);
		
		self.editorWindow = nil;
	end	
	
	self.editorWindow = nil;
	self.generalHeader = nil;
	self.windowsHeader = nil;
	self.macosHeader = nil;
	self.iosHeader = nil;
	self.androidHeader = nil;
	self.packageTexturesSwitch = nil;
	self.packageButton = nil;
	self.testText = nil;

	print("PackageEditor unload end");
end

function PackageEditor:show()
	print("PackageEditor show called");

	local parentWindow = self.window:getParentWindow();
	local debugWindow = self.window:getDebugWindow();
	
	--self:unload();
	--self:load();
	
	if parentWindow then
		--parentWindow:setVisible(true, false);
	end
	
	if debugWindow then
		--debugWindow:setVisible(false, false);
		--debugWindow:setVisible(true, false); -- for testing
	end
	
	if self.editorWindow then
		self.editorWindow:setVisible(true, false);
		print("PackageEditor show setVisible");
	end
	
	print("PackageEditor show end");
end

function PackageEditor:hide()
	print("PackageEditor hide called");

	local debugWindow = self.window:getDebugWindow();
		
	if debugWindow then
		debugWindow:setVisible(false, false);
	end
	
	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
	end
	
	--local parentWindow = self.window:getParentWindow();
	--parentWindow:setVisible(false, false);
end

function PackageEditor:handleEvent(parameters, results)
	print("PackageEditor handleEvent called");
	
	local applicationManager = IApplicationManager.instance();
	local resourceDatabase = applicationManager:getResourceDatabase();
	local packageManager = applicationManager:getPackageManager();
	
	if (parameters == nil) then 
		print("PackageEditor parameters nil");
	end 
	
	local eventHash = parameters:at(1);
	local sender = parameters:at(3);
	local args = parameters:at(2);

	local elementId = sender:getElementId();
	
	if (eventHash == IEvent.handleSelection) then
		print("PackageEditor handleSelection called");
		if (elementId == PackageEditorTypes.PackageButton) then
			print("PackageEditor: Package button pressed");
			packageManager:createPackage();
		end
	end
	
	if (eventHash == IEvent.handleValueChanged) then
		print("PackageEditor handleValueChanged called");
		
		if (elementId == PackageEditorTypes.PackageTexturesSwitch) then
			local value = self.packageTexturesSwitch:getValue();
			packageManager:setPackageTextures(value);
		elseif (elementId == PackageEditorTypes.BuildTypeDropdown) then
			local option = self.buildTypeDropdown:getSelectedOption();
			print("Build type changed to option: "..tostring(option));
			packageManager:setBuildType(option); -- Implement this in packageManager if needed
		elseif (elementId == PackageEditorTypes.OutputPathEntry) then
			local path = self.outputPathEntry:getText();
			print("Output path changed: "..tostring(path));
			packageManager:setOutputPath(path); -- Implement this in packageManager if needed
		elseif (elementId == PackageEditorTypes.WindowsBuildSwitch) then
			local value = self.windowsBuildSwitch:getValue();
			print("Windows build enabled: "..tostring(value));
			packageManager:setWindowsBuildEnabled(value); -- Implement this in packageManager if needed
		elseif (elementId == PackageEditorTypes.MacOSSwitch) then
			local value = self.macosSwitch:getValue();
			print("macOS build enabled: "..tostring(value));
			packageManager:setMacOSBuildEnabled(value); -- Implement this in packageManager if needed
		elseif (elementId == PackageEditorTypes.IOSSwitch) then
			local value = self.iosSwitch:getValue();
			print("iOS build enabled: "..tostring(value));
			packageManager:setIOSBuildEnabled(value); -- Implement this in packageManager if needed
		elseif (elementId == PackageEditorTypes.AndroidSwitch) then
			local value = self.androidSwitch:getValue();
			print("Android build enabled: "..tostring(value));
			packageManager:setAndroidBuildEnabled(value); -- Implement this in packageManager if needed
		elseif (elementId == PackageEditorTypes.CompressionSlider) then
			local value = self.compressionSlider:getValue();
			print("Compression level changed: "..tostring(value));
			packageManager:setCompressionLevel(value); -- Implement this in packageManager if needed
		elseif (elementId == PackageEditorTypes.CustomVersionEntry) then
			local version = self.customVersionEntry:getText();
			print("Custom version changed: "..tostring(version));
			packageManager:setCustomVersion(version); -- Implement this in packageManager if needed
		end
	end
end

