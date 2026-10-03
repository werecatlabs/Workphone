class 'MenuBuilder' (BaseComponent)

function MenuBuilder:__init(component)
	BaseComponent.__init(self, component)
	self.menuTitle = "Main Menu"
	self.generatedRoot = nil
end

function MenuBuilder:__finalize()
	BaseComponent.__finalize(self)
end

function MenuBuilder:update()
end

function MenuBuilder:getProperties(parameters)
	local properties = parameters:at(0)
	properties:setPropertyAsString("menuTitle", self.menuTitle)
	properties:setPropertyAsButton("Generate", false)
end

function MenuBuilder:setProperties(parameters)
	local properties = parameters:at(0)

	if properties:hasProperty("menuTitle") then
		self.menuTitle = properties:getPropertyAsString("menuTitle")
	end

	if properties:isButtonPressed("Generate") then
		self:generate()
	end
end

function MenuBuilder:generate()
	local runtime = TemplateBuilderRuntime
	local root = DataDrivenGameTemplateGenerator.CreateChild(nil, self.menuTitle)
	local canvas = DataDrivenGameTemplateGenerator.CreateChild(root.transform, "Menu Canvas")

	if runtime then
		local canvasComponent = runtime.addComponent(canvas, "Canvas")
		if canvasComponent then
			canvasComponent.renderMode = "ScreenSpaceOverlay"
		end

		runtime.addComponent(canvas, "CanvasScaler")
		runtime.addComponent(canvas, "GraphicRaycaster")
	end

	DataDrivenGameTemplateGenerator.CreateUIText(canvas.transform, "Title", self.menuTitle, 48, { x = 0, y = -80 }, "UpperCenter")
	DataDrivenGameTemplateGenerator.CreateUIText(canvas.transform, "Start Button", "Start Game", 28, { x = 0, y = -180 }, "MiddleCenter")
	DataDrivenGameTemplateGenerator.CreateUIText(canvas.transform, "Settings Button", "Settings", 28, { x = 0, y = -240 }, "MiddleCenter")
	DataDrivenGameTemplateGenerator.CreateUIText(canvas.transform, "Quit Button", "Quit", 28, { x = 0, y = -300 }, "MiddleCenter")

	self.generatedRoot = root
	return root
end

