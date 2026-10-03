class 'AboutDialog'

function AboutDialog:__init(window)
	print("AboutDialog __init called");
	self.window = window;
	self.editorWindow = nil;

	self.properties = nil;
end

function AboutDialog:__finalize()
	print("AboutDialog __finalize called");
	self.window = nil;
	self.properties = nil;
end

function AboutDialog:load()
	local windowTypeInfo = IUIWindow.typeInfo();
	local buttonTypeInfo = IUIButton.typeInfo();
	local textTypeInfo = IUIText.typeInfo();
	local textEntryTypeInfo = IUITextEntry.typeInfo();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	local parent = self.window:getParent();
	local parentWindow = self.window:getParentWindow();

	local editorWindow = ui:addElement(windowTypeInfo);
	editorWindow:setLabel("About");
	editorWindow:setSize(Vector2F(600.0, 400.0));
	self.editorWindow = editorWindow;
	parentWindow:addChild(self.editorWindow);

	local header = ui:addElement(textTypeInfo);
	header:setText("About");
	parentWindow:addChild(header);

	-- Description / credits
	local desc = ui:addElement(textTypeInfo);
	local descriptionText = "A small editor built on the Workphone framework.\n\n" ..
		"Credits: Editor team, contributors and libraries.\n\n" ..
		"For help, open the Help menu or visit the project website.";
	desc:setText(descriptionText);
	parentWindow:addChild(desc);

	-- Close button (best-effort; behaviour implemented in Lua if event dispatch is available)
	local closeBtn = ui:addElement(buttonTypeInfo);
	closeBtn:setLabel("Close");
	parentWindow:addChild(closeBtn);
end

function AboutDialog:unload()
	-- Nothing special to cleanup in the Lua side; C++ UI elements are managed by the host.
	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	local parentWindow = self.window:getParentWindow();
	self.editorWindow = nil;

	if parentWindow then
		parentWindow:destroyAllChildren();
	end
end

function AboutDialog:show()
	print("AboutDialog show called");
	
	local parentWindow = self.window:getParentWindow();
	local debugWindow = self.window:getDebugWindow();
	
	parentWindow:setVisible(true, false);
	debugWindow:setVisible(false, false);
	
	if self.editorWindow then
		self.editorWindow:setVisible(true, false);
	end
	
	--parentWindow:setVisible(false, false); -- todo temp code to hide new ui
end

function AboutDialog:hide()
	print("AboutDialog hide called");
	local debugWindow = self.window:getDebugWindow();
	if debugWindow then
		debugWindow:setVisible(false, false);
	end 

	--self:unload();
	
	if self.editorWindow then
		self.editorWindow:setVisible(false, false);
	end
	
	local parentWindow = self.window:getParentWindow();
	if parentWindow then
		parentWindow:setVisible(false, false);
	end
end