class 'Splash' (BaseComponent)

function Splash:__init(object)
	BaseComponent.__init(self, object);	
	self.sceneToLoad = "Application";
	self.loadDelay = 3.0;
end

function Splash:__finalize()
	BaseComponent.__finalize(self);
end

function Splash:update()
	local applicationManager = IApplicationManager.instance();
    local timer = applicationManager:getTimer();
    local sceneManager = applicationManager:getGameManager();

    if timer:getTimeSinceLevelLoad() > self.loadDelay then
		sceneManager:loadScene(self.sceneToLoad, true);
	end
end

function Splash:getProperties(parameters)
	local properties = parameters:at(0);
	properties:setPropertyAsString("sceneToLoad", self.sceneToLoad);
	properties:setPropertyAsFloat("loadDelay", self.loadDelay);
end

function Splash:setProperties(parameters)
	local applicationManager = IApplicationManager.instance();
	local fileSystem = applicationManager:getFileSystem();
	local factoryManager = applicationManager:getFactoryManager();

	local properties = parameters:at(0);

	self.sceneToLoad = properties:getPropertyAsString("sceneToLoad");
	self.loadDelay = properties:getPropertyAsFloat("loadDelay");
end