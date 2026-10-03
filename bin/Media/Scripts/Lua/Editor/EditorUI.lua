class 'EditorUI'

EditorUITypes = 
{
	RunUnitTests = 8,
	CreateCharacterTest = 9,
	CreateVehicleTest = 10,	
}

function EditorUI:__init()
	self.isLoaded = false;
end

function EditorUI:__finalize()
end

function EditorUI:load()
	if self.isLoaded then
		return;
	end

	local menuTypeInfo = IUIMenu.typeInfo();
	local menuItemTypeInfo = IUIMenuItem.typeInfo();

	local applicationManager = IApplicationManager.instance();
	local ui = applicationManager:getUI();

	local application = ui:getApplication();
	local menuBar = application:getMenubar();

	local utilMenu = menuBar:findMenuByLabel("Util");
	if utilMenu then
		local unitTestMenu = ui:addElement(menuTypeInfo);

		unitTestMenu:setLabel("Unit Test");
		utilMenu:addMenuItem(unitTestMenu);

		local unitTestItem = ui:addElement(menuItemTypeInfo);
		unitTestItem:setElementId( EditorUITypes.RunUnitTests );
		unitTestItem:setText( "Run Unit Tests" );
		unitTestItem:setHelp( "Runs all unit tests" );
		unitTestMenu:addMenuItem(unitTestItem);

		local characterMenu = ui:addElement(menuTypeInfo);
		characterMenu:setLabel("Character");
		utilMenu:addMenuItem(characterMenu);

		local characterTestItem = ui:addElement(menuItemTypeInfo);
		characterTestItem:setElementId( EditorUITypes.CreateCharacterTest );
		characterTestItem:setText( "Create Character Test" );
		characterTestItem:setHelp( "Creates a character test" );
		characterMenu:addMenuItem(characterTestItem);

		local vehicleMenu = ui:addElement(menuTypeInfo);
		vehicleMenu:setLabel("Vehicle");
		utilMenu:addMenuItem(vehicleMenu);

		local vehicleTestItem = ui:addElement(menuItemTypeInfo);
		vehicleTestItem:setElementId( EditorUITypes.CreateVehicleTest );
        	vehicleTestItem:setText( "Create Vehicle Test" );
		vehicleTestItem:setHelp( "Creates a vehicle test" );
		vehicleMenu:addMenuItem(vehicleTestItem);
	end

	self.isLoaded = utilMenu ~= nil;
end

function EditorUI:unload()
end

function EditorUI:handleEvent(parameters, results)
	if parameters == nil then
		print("EditorUI parameters nil");
		return;
	end

	local sender = nil;
	if parameters.at then
		local ok, value = pcall(function() return parameters:at(3); end);
		if ok then
			sender = value;
		end
	else
		sender = parameters.sender or parameters.element;
	end

	if sender == nil then
		return;
	end

	local ok, elementId = pcall(function() return sender:getElementId(); end);
	if not ok or elementId == nil then
		return;
	end
	
	if elementId == EditorUITypes.CreateVehicleTest then
		print("EditorUI CreateVehicleTest called");
	end
end

function EditorUI:createRacingTemplate()
	local definition = nil; --ScriptableObject.CreateInstance<GameTemplateDefinition>();

--[[
	definition.templateName = "Example Racing";
	definition.genre = GameTemplateGenre.Racing;
	definition.description = "A simple racing template with car player, track, barriers, checkpoints, and chase camera.";

	definition.playerFallbackPrimitive = PrimitiveType.Cube;
	definition.playerSpawnPosition = new Vector3(0.0f, 1.0f, -32.0f);
	definition.playerScale = new Vector3(1.8f, 0.6f, 3.2f);

	definition.cameraPosition = new Vector3(0.0f, 6.0f, -12.0f);
	definition.cameraRotation = new Vector3(20.0f, 0.0f, 0.0f);
	definition.addCameraFollow = true;

	definition.hudTitle = "Racing Template";
	definition.objectiveText = "Objective: complete all checkpoints.";

	definition.environmentObjects.Add(new TemplateSpawnDefinition
	{
		name = "Track Base",
		fallbackPrimitive = PrimitiveType.Cube,
		position = Vector3.zero,
		scale = new Vector3(40.0f, 0.25f, 90.0f),
		color = new Color(0.12f, 0.12f, 0.12f)
	});

	definition.environmentObjects.Add(new TemplateSpawnDefinition
	{
		name = "Left Barrier",
		fallbackPrimitive = PrimitiveType.Cube,
		position = new Vector3(-8.0f, 1.0f, 0.0f),
		scale = new Vector3(0.5f, 2.0f, 80.0f),
		color = new Color(0.7f, 0.1f, 0.1f)
	});

	definition.environmentObjects.Add(new TemplateSpawnDefinition
	{
		name = "Right Barrier",
		fallbackPrimitive = PrimitiveType.Cube,
		position = new Vector3(8.0f, 1.0f, 0.0f),
		scale = new Vector3(0.5f, 2.0f, 80.0f),
		color = new Color(0.7f, 0.1f, 0.1f)
	});

	definition.environmentObjects.Add(new TemplateSpawnDefinition
	{
		name = "Start Line",
		fallbackPrimitive = PrimitiveType.Cube,
		position = new Vector3(0.0f, 0.1f, -30.0f),
		scale = new Vector3(10.0f, 0.1f, 1.0f),
		color = Color.white
	});

	for (int i = 0; i < 5; i++)
	{
		definition.gameplayObjects.Add(new TemplateSpawnDefinition
		{
			name = "Checkpoint " + i,
			fallbackPrimitive = PrimitiveType.Cube,
			position = new Vector3(0.0f, 1.5f, -25.0f + i * 14.0f),
			scale = new Vector3(12.0f, 3.0f, 0.35f),
			isTrigger = true,
			color = new Color(0.1f, 0.5f, 1.0f, 0.5f)
		});
	}

	const string folder = "Assets/GameTemplates";

	if (!AssetDatabase.IsValidFolder(folder))
	{
		AssetDatabase.CreateFolder("Assets", "GameTemplates");
	}

	string path = folder + "/ExampleRacingTemplate.asset";

	AssetDatabase.CreateAsset(definition, path);
	AssetDatabase.SaveAssets();
	AssetDatabase.Refresh();

	Selection.activeObject = definition;

	Debug.Log("Created template asset: " + path);
	]]
end

function EditorUI:createPlatformerTemplate()
	local definition = nil; --ScriptableObject.CreateInstance<GameTemplateDefinition>();

--[[
	definition.templateName = "Example Platformer";
	definition.genre = GameTemplateGenre.Platformer;
	definition.description = "A simple platformer template with platforms, collectibles, player, camera, and HUD.";

	definition.playerFallbackPrimitive = PrimitiveType.Capsule;
	definition.playerSpawnPosition = new Vector3(-8.0f, 2.0f, 0.0f);
	definition.playerScale = Vector3.one;

	definition.cameraPosition = new Vector3(0.0f, 5.0f, -14.0f);
	definition.cameraRotation = new Vector3(18.0f, 0.0f, 0.0f);
	definition.orthographicCamera = false;
	definition.addCameraFollow = true;

	definition.hudTitle = "Platformer Template";
	definition.objectiveText = "Objective: collect items and reach the goal.";

	definition.environmentObjects.Add(new TemplateSpawnDefinition
	{
		name = "Ground",
		fallbackPrimitive = PrimitiveType.Cube,
		position = new Vector3(0.0f, -0.5f, 0.0f),
		scale = new Vector3(24.0f, 1.0f, 3.0f),
		color = new Color(0.25f, 0.25f, 0.25f)
	});

	definition.environmentObjects.Add(new TemplateSpawnDefinition
	{
		name = "Platform 01",
		fallbackPrimitive = PrimitiveType.Cube,
		position = new Vector3(-3.0f, 2.0f, 0.0f),
		scale = new Vector3(4.0f, 0.5f, 3.0f),
		color = new Color(0.3f, 0.35f, 0.4f)
	});

	definition.environmentObjects.Add(new TemplateSpawnDefinition
	{
		name = "Platform 02",
		fallbackPrimitive = PrimitiveType.Cube,
		position = new Vector3(4.0f, 4.0f, 0.0f),
		scale = new Vector3(4.0f, 0.5f, 3.0f),
		color = new Color(0.3f, 0.35f, 0.4f)
	});

	definition.environmentObjects.Add(new TemplateSpawnDefinition
	{
		name = "Platform 03",
		fallbackPrimitive = PrimitiveType.Cube,
		position = new Vector3(10.0f, 6.0f, 0.0f),
		scale = new Vector3(4.0f, 0.5f, 3.0f),
		color = new Color(0.3f, 0.35f, 0.4f)
	});

	for (int i = 0; i < 6; i++)
	{
		definition.gameplayObjects.Add(new TemplateSpawnDefinition
		{
			name = "Collectible " + i,
			fallbackPrimitive = PrimitiveType.Sphere,
			position = new Vector3(-6.0f + i * 3.0f, 2.0f + i * 0.5f, 0.0f),
			scale = Vector3.one * 0.5f,
			addBoxCollider = true,
			isTrigger = true,
			color = new Color(1.0f, 0.8f, 0.1f)
		});
	}

	definition.gameplayObjects.Add(new TemplateSpawnDefinition
	{
		name = "Goal",
		fallbackPrimitive = PrimitiveType.Cube,
		position = new Vector3(12.0f, 7.0f, 0.0f),
		scale = new Vector3(1.0f, 3.0f, 1.0f),
		isTrigger = true,
		color = new Color(0.1f, 0.9f, 0.3f)
	});

	const string folder = "Assets/GameTemplates";

	if (!AssetDatabase.IsValidFolder(folder))
	{
		AssetDatabase.CreateFolder("Assets", "GameTemplates");
	}

	string path = folder + "/ExamplePlatformerTemplate.asset";

	AssetDatabase.CreateAsset(definition, path);
	AssetDatabase.SaveAssets();
	AssetDatabase.Refresh();

	Selection.activeObject = definition;

	Debug.Log("Created template asset: " + path);
	]]
end
