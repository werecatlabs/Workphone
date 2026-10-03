class 'BaseComponent' 

function BaseComponent:__init(component)
	self.component = component;
end

function BaseComponent:__finalize()
end

function BaseComponent:update()
end

function BaseComponent:getActor()
	return self.component:getActor();
end

function BaseComponent:getProperties(parameters)
	local properties = parameters:at(0);
end

function BaseComponent:setProperties(parameters)
	local applicationManager = IApplicationManager.instance();
	local fileSystem = applicationManager:getFileSystem();

	local properties = parameters:at(0);
end