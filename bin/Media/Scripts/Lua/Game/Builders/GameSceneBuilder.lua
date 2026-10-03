class 'GameSceneBuilder' (BaseComponent)

function GameSceneBuilder:__init(component)
	BaseComponent.__init(self, component);
	self.genre = GameTemplateGenre and GameTemplateGenre.Platformer or "Platformer";
	self.clearScene = false;
	self.generatedRoot = nil;
end

function GameSceneBuilder:__finalize()
	BaseComponent.__finalize(self);
end

function GameSceneBuilder:update()
end

function GameSceneBuilder:getProperties(parameters)
	local properties = parameters:at(0);
	properties:setPropertyAsString("genre", self.genre);
	properties:setPropertyAsBool("clearScene", self.clearScene);
	properties:setPropertyAsButton("Generate", false);
end

function GameSceneBuilder:setProperties(parameters)	
	local properties = parameters:at(0);
	if properties:hasProperty("genre") then
		self.genre = properties:getPropertyAsString("genre");
	end

	if properties:hasProperty("clearScene") then
		self.clearScene = properties:getPropertyAsBool("clearScene");
	end

	if properties:isButtonPressed("Generate") then
		self:generate();
	end
end

function GameSceneBuilder:generate()
	self.generatedRoot = GameTemplateGenerator.Generate(self.genre, self.clearScene);
	return self.generatedRoot;
end
