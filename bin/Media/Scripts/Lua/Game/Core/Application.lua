class 'Application' (BaseComponent)

function Application:__init(component)
	BaseComponent.__init(self, component);
	self.player = nil;
end

function Application:__finalize()
end

function Application:update()
end

function Application:getProperties(parameters)
	local properties = parameters:at(0);

	properties:setButtonPressed("Generate", false);
end

function Application:setProperties(parameters)
	local applicationManager = IApplicationManager.instance();
	local fileSystem = applicationManager:getFileSystem();
	local factoryManager = applicationManager:getFactoryManager();

	local properties = parameters:at(0);

	if properties:isButtonPressed("Generate") then
		--self:generate();
		self:generateNew();
	end
end

function Application:generate()
	local applicationManager = IApplicationManager.instance();
	local gameManager = applicationManager:getGameManager();

	local actor = self:getActor();
	if actor then
		actor:destroyChildren();

		local components = actor:getComponents();
		for i = 0, components:size() - 1 do
			local component = components:at(i);
			component:generate();
		end
	end
	
	local userComponentTypeId = UserComponent.typeInfo();
	
	local uiActor = gameManager:createActor();
	uiActor:setName("UiManager");
	
	local uiManager = uiActor:addComponent("UserComponent");
	uiManager:setClassName("UiManager");
end

function Application:generateNew()
	local applicationManager = IApplicationManager.instance();
	local gameManager = applicationManager:getGameManager();

	local actor = self:getActor();
	actor:destroyChildren();
	
	local userComponentTypeId = UserComponent.typeInfo();
	
	local uiActor = gameManager:createActor();
	uiActor:setName("UIManager");
	actor:addChild( uiActor );
	
	local uiManager = uiActor:addComponentById( userComponentTypeId );
	uiManager:setClassName("UIManager");
	uiManager:generate();
end

