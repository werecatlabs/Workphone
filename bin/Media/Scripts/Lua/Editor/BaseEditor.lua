class 'BaseEditor'

function BaseEditor:__init(window)
	print("BaseEditor __init called");
	self.window = window;
end

function BaseEditor:__finalize()
	print("BaseEditor __finalize called");
	self.window = nil;	
end

function BaseEditor:load()
	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();
	
	local parentWindow = self.window:getParentWindow();
	parentWindow:setSize(Vector2F(500.0, 300.0));
end

function BaseEditor:unload()
end

function BaseEditor:getProperties(parameters)
	local properties = parameters:at(0);
end

function BaseEditor:setProperties(parameters)
	local applicationManager = IApplicationManager.instance();
	local fileSystem = applicationManager:getFileSystem();

	local properties = parameters:at(0);
end

function BaseEditor:update()
end

function BaseEditor:show()
end

function BaseEditor:hide()
end