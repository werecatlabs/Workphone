class 'GameSceneBuilder' (BaseComponent)

function GameSceneBuilder:getProperties(parameters)
	local properties = parameters:at(0);
	properties:setPropertyAsButton("Import", false);
end

function GameSceneBuilder:setProperties(parameters)
	local applicationManager = IApplicationManager.instance();
	local fileSystem = applicationManager:getFileSystem();
	local factoryManager = applicationManager:getFactoryManager();

	local properties = parameters:at(0);
end
