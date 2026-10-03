class 'ProceduralScene' (BaseComponent)

function ProceduralScene:__init()
	BaseComponent.__init(self, object);	
	print("ProceduralScene constructor called");
	self.player = nil
	self.proceduralCities = {}
	self.proceduralRenderer = nil
	self.sidewalkRenderer = nil
	self.cityGenerator = nil
	self.roadPrefab = nil
	self.roadNodePrefab = nil
	self.sidewalkPrefab = nil
	self.sidewalkNodePrefab = nil
	self.roadConnectionPrefab = nil
	self.blockPrefab = nil
	self.blockNodePrefab = nil
	self.lotNodePrefab = nil
	self.lotPrefab = nil
	self.pathPrefab = nil
	self.cityCellPrefab = nil
	self.riverPrefab = nil
	self.riverNodePrefab = nil
	self.isRoadsDirty = false
	self.generateRoads = true
	self.generateBlocks = true
	self.generateLots = true
	self.generateBuildings = true
	self.generateDynamicMeshes = true
	self.createTraffic = false
	self.createRivers = false
	self.currentRoad = nil
	self.previousRoadNode = nil
end

function ProceduralScene:__finalize()
end

function ProceduralScene:loadData(filePath)
	print("ProceduralScene:test called");
	
	local applicationManager = IApplicationManager.instance();
	local sceneManager = applicationManager:getSceneManager();
	local scene = sceneManager:getCurrentScene();	
end

function ProceduralScene:getProperties(parameters)
	local properties = parameters:at(0);
	properties:setPropertyAsButton("Import", false);
end

function ProceduralScene:setProperties(parameters)
	local applicationManager = IApplicationManager.instance();
	local fileSystem = applicationManager:getFileSystem();
	local factoryManager = applicationManager:getFactoryManager();

	local properties = parameters:at(0);
	if properties:isButtonPressed("Import") then
		local fileDialog = fileSystem:openFileDialog();
		if fileDialog then 
			local result = fileDialog:openDialog();
			if result == INativeFileDialog.Dialog_Okay then 
				local filePath = fileDialog:getFilePath();

				local cityGeneratorDefault = factoryManager:make_object("CityGenerator");
				local proceduralGenerator = factoryManager:make_object("ProceduralGenerator");

				if cityGeneratorDefault then
					cityGeneratorDefault:setFilePath( filePath );
					cityGeneratorDefault:load( nil );
				end
			end
		end
	end
end

-- Clear all procedural cities
function ProceduralScene:Clear()
	for _, city in ipairs(self.proceduralCities) do
		if city.Clear then city:Clear() end
	end
end

-- Update method (stub)
function ProceduralScene:Update()
	if self.isRoadsDirty then
		if self.cityGenerator and self.cityGenerator.CreateFromData then
			self.cityGenerator:CreateFromData()
		end
		
		self.isRoadsDirty = false
	end
end

-- Remove overlapping lots in all cities
function ProceduralScene:RemoveOverlappingLots()
	for _, city in ipairs(self.proceduralCities) do
		if city.RemoveOverlappingLots then city:RemoveOverlappingLots() end
	end
end

-- Create cells in all cities
function ProceduralScene:CreateCells()
	for _, city in ipairs(self.proceduralCities) do
		city.m_Scene = self
		if city.CreateCells then city:CreateCells() end
	end
end

-- Destroy cells in all cities
function ProceduralScene:DestroyCells()
	for _, city in ipairs(self.proceduralCities) do
		city.m_Scene = self
		if city.DestroyCells then city:DestroyCells() end
	end
end

-- Show block debug in all cities
function ProceduralScene:ShowBlockDebug()
	for _, city in ipairs(self.proceduralCities) do
		if city.GetBlocks then
			local blocks = city:GetBlocks()
			for _, block in ipairs(blocks) do
				block.ShowPolygons = true
			end
		end
	end
end

-- Hide block debug in all cities
function ProceduralScene:HideBlockDebug()
	for _, city in ipairs(self.proceduralCities) do
		if city.GetBlocks then
			local blocks = city:GetBlocks()
			for _, block in ipairs(blocks) do
				block.ShowPolygons = false
			end
		end
	end
end