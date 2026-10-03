include("BaseComponent.lua")

class 'ApplicationSceneBuilder' (BaseComponent)

function ApplicationSceneBuilder:__init(component)
	BaseComponent.__init(self, component);
	self.applicationName = "Generated Application";
	self.defaultGenre = GameTemplateGenre and GameTemplateGenre.Platformer or "Platformer";
	self.clearScene = false;
	self.generatedRoot = nil;
end

function ApplicationSceneBuilder:__finalize()
	BaseComponent.__finalize(self);
end

function ApplicationSceneBuilder:update()
end

function ApplicationSceneBuilder:getProperties(parameters)
	local properties = parameters:at(0);
	properties:setPropertyAsString("applicationName", self.applicationName);
	properties:setPropertyAsString("defaultGenre", self.defaultGenre);
	properties:setPropertyAsBool("clearScene", self.clearScene);
	properties:setPropertyAsButton("Generate", false);
end

function ApplicationSceneBuilder:setProperties(parameters)	
	local properties = parameters:at(0);
	if properties:hasProperty("applicationName") then
		self.applicationName = properties:getPropertyAsString("applicationName");
	end

	if properties:hasProperty("defaultGenre") then
		self.defaultGenre = properties:getPropertyAsString("defaultGenre");
	end

	if properties:hasProperty("clearScene") then
		self.clearScene = properties:getPropertyAsBool("clearScene");
	end

	if properties:isButtonPressed("Generate") then
		self:generate();
	end
end

function ApplicationSceneBuilder:generate()
	local definition = GameTemplateGenerator.CreateDefinition(self.defaultGenre);
	definition.templateName = self.applicationName;
	self.generatedRoot = DataDrivenGameTemplateGenerator.Generate(definition, self.clearScene);
	return self.generatedRoot;
end
