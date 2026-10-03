GameTemplateGenerator = GameTemplateGenerator or {}

local function rt()
	if TemplateBuilderRuntime then
		return TemplateBuilderRuntime
	end

	local function vec3(x, y, z)
		if Vector3 then
			return Vector3(x or 0.0, y or 0.0, z or 0.0)
		end

		return { x = x or 0.0, y = y or 0.0, z = z or 0.0 }
	end

	TemplateBuilderRuntime =
	{
		vec3 = vec3,
		color = function(r, g, b, a)
			if Color then
				return Color(r or 1.0, g or 1.0, b or 1.0, a or 1.0)
			end

			return { r = r or 1.0, g = g or 1.0, b = b or 1.0, a = a or 1.0 }
		end,
		euler = function(rotation)
			if Quaternion and Quaternion.Euler then
				return Quaternion.Euler(rotation)
			end

			return rotation
		end,
		newGameObject = function(name)
			if GameObject and GameObject.New then
				return GameObject.New(name)
			end

			return { name = name, transform = {} }
		end,
		createPrimitive = function(primitiveType, name)
			local obj = nil
			if GameObject and GameObject.CreatePrimitive then
				obj = GameObject.CreatePrimitive(primitiveType or "Cube")
			else
				obj = { transform = {} }
			end

			obj.name = name
			obj.primitiveType = primitiveType or "Cube"
			obj.transform = obj.transform or {}
			return obj
		end,
		setParent = function(obj, parent)
			if obj and obj.transform and obj.transform.SetParent then
				obj.transform:SetParent(parent, false)
			elseif obj then
				obj.parent = parent

				if parent then
					parent.children = parent.children or {}
					table.insert(parent.children, obj)
				end
			end
		end,
		setTransform = function(obj, position, rotationEuler, scale)
			if not obj then
				return
			end

			obj.transform = obj.transform or {}
			obj.transform.localPosition = position
			obj.transform.position = position
			obj.transform.localRotation = Quaternion and Quaternion.Euler and Quaternion.Euler(rotationEuler) or rotationEuler
			obj.transform.rotation = obj.transform.localRotation
			obj.transform.localScale = scale
		end,
		addComponent = function(obj, componentName)
			if obj and obj.AddComponent then
				return obj:AddComponent(componentName)
			end

			obj.components = obj.components or {}
			obj.components[componentName] = obj.components[componentName] or { gameObject = obj, transform = obj.transform }
			return obj.components[componentName]
		end,
		getComponent = function(obj, componentName)
			if obj and obj.GetComponent then
				return obj:GetComponent(componentName)
			end

			return obj and obj.components and obj.components[componentName] or nil
		end,
		destroy = function(obj)
			if not obj then
				return
			end

			if Object and Object.DestroyImmediate then
				Object.DestroyImmediate(obj)
			elseif Object and Object.Destroy then
				Object.Destroy(obj)
			else
				obj.destroyed = true
			end
		end
	}

	return TemplateBuilderRuntime
end

local function spawn(values)
	if TemplateSpawnDefinition then
		return TemplateSpawnDefinition(values)
	end

	return values
end

local function makeDefinition(values)
	if GameTemplateDefinition then
		return GameTemplateDefinition(values)
	end

	return values
end

function GameTemplateGenerator.Generate(genre, clearScene)
	local definition = GameTemplateGenerator.CreateDefinition(genre)
	return DataDrivenGameTemplateGenerator.Generate(definition, clearScene)
end

function GameTemplateGenerator.CreateDefinition(genre)
	genre = genre or (GameTemplateGenre and GameTemplateGenre.Platformer or "Platformer")

	if genre == GameTemplateGenre.Racing then
		return GameTemplateGenerator.CreateRacingDefinition()
	elseif genre == GameTemplateGenre.Flight then
		return GameTemplateGenerator.CreateFlightDefinition()
	elseif genre == GameTemplateGenre.TopDownShooter then
		return GameTemplateGenerator.CreateTopDownShooterDefinition()
	elseif genre == GameTemplateGenre.RPG then
		return GameTemplateGenerator.CreateRPGDefinition()
	end

	return GameTemplateGenerator.CreatePlatformerDefinition()
end

function GameTemplateGenerator.CreatePlatformerDefinition()
	local runtime = rt()
	local definition = makeDefinition(
	{
		templateName = "Platformer Template",
		genre = GameTemplateGenre.Platformer,
		description = "A simple platformer template with platforms, collectibles, player, camera, and HUD.",
		playerFallbackPrimitive = "Capsule",
		playerSpawnPosition = runtime.vec3(-8.0, 2.0, 0.0),
		playerScale = runtime.vec3(1.0, 1.0, 1.0),
		cameraPosition = runtime.vec3(0.0, 5.0, -14.0),
		cameraRotation = runtime.vec3(18.0, 0.0, 0.0),
		hudTitle = "Platformer Template",
		objectiveText = "Objective: collect items and reach the goal."
	})

	definition:AddEnvironmentObject(spawn({ name = "Ground", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, -0.5, 0.0), scale = runtime.vec3(24.0, 1.0, 3.0), color = runtime.color(0.25, 0.25, 0.25, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "Platform 01", fallbackPrimitive = "Cube", position = runtime.vec3(-3.0, 2.0, 0.0), scale = runtime.vec3(4.0, 0.5, 3.0), color = runtime.color(0.3, 0.35, 0.4, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "Platform 02", fallbackPrimitive = "Cube", position = runtime.vec3(4.0, 4.0, 0.0), scale = runtime.vec3(4.0, 0.5, 3.0), color = runtime.color(0.3, 0.35, 0.4, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "Platform 03", fallbackPrimitive = "Cube", position = runtime.vec3(10.0, 6.0, 0.0), scale = runtime.vec3(4.0, 0.5, 3.0), color = runtime.color(0.3, 0.35, 0.4, 1.0) }))

	for i = 0, 5 do
		definition:AddGameplayObject(spawn(
		{
			name = "Collectible " .. i,
			fallbackPrimitive = "Sphere",
			position = runtime.vec3(-6.0 + i * 3.0, 2.0 + i * 0.5, 0.0),
			scale = runtime.vec3(0.5, 0.5, 0.5),
			isTrigger = true,
			color = runtime.color(1.0, 0.8, 0.1, 1.0)
		}))
	end

	definition:AddGameplayObject(spawn({ name = "Goal", fallbackPrimitive = "Cube", position = runtime.vec3(12.0, 7.0, 0.0), scale = runtime.vec3(1.0, 3.0, 1.0), isTrigger = true, color = runtime.color(0.1, 0.9, 0.3, 1.0) }))
	return definition
end

function GameTemplateGenerator.CreateTopDownShooterDefinition()
	local runtime = rt()
	local definition = makeDefinition(
	{
		templateName = "Top Down Shooter Template",
		genre = GameTemplateGenre.TopDownShooter,
		description = "A top-down arena template with player, walls, pickups, and enemy spawn points.",
		playerFallbackPrimitive = "Capsule",
		playerSpawnPosition = runtime.vec3(0.0, 1.0, 0.0),
		cameraPosition = runtime.vec3(0.0, 18.0, -2.0),
		cameraRotation = runtime.vec3(80.0, 0.0, 0.0),
		orthographicCamera = true,
		orthographicSize = 10.0,
		hudTitle = "Top Down Shooter Template",
		objectiveText = "Objective: survive enemy waves."
	})

	definition:AddEnvironmentObject(spawn({ name = "Arena Floor", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 0.0, 0.0), scale = runtime.vec3(24.0, 0.5, 24.0), color = runtime.color(0.18, 0.18, 0.2, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "North Wall", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 1.0, 12.0), scale = runtime.vec3(24.0, 2.0, 0.5), color = runtime.color(0.4, 0.42, 0.48, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "South Wall", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 1.0, -12.0), scale = runtime.vec3(24.0, 2.0, 0.5), color = runtime.color(0.4, 0.42, 0.48, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "East Wall", fallbackPrimitive = "Cube", position = runtime.vec3(12.0, 1.0, 0.0), scale = runtime.vec3(0.5, 2.0, 24.0), color = runtime.color(0.4, 0.42, 0.48, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "West Wall", fallbackPrimitive = "Cube", position = runtime.vec3(-12.0, 1.0, 0.0), scale = runtime.vec3(0.5, 2.0, 24.0), color = runtime.color(0.4, 0.42, 0.48, 1.0) }))
	definition:AddGameplayObject(spawn({ name = "Weapon Pickup", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 0.75, 5.0), scale = runtime.vec3(1.0, 1.0, 1.0), isTrigger = true, color = runtime.color(0.2, 0.7, 1.0, 1.0) }))

	for i = 0, 5 do
		local angle = (i / 6.0) * math.pi * 2.0
		definition:AddGameplayObject(spawn(
		{
			name = "Enemy Spawn " .. i,
			fallbackPrimitive = "Cube",
			position = runtime.vec3(math.cos(angle) * 9.0, 0.5, math.sin(angle) * 9.0),
			scale = runtime.vec3(1.0, 1.0, 1.0),
			isTrigger = true,
			color = runtime.color(0.9, 0.2, 0.2, 1.0)
		}))
	end

	return definition
end

function GameTemplateGenerator.CreateRacingDefinition()
	local runtime = rt()
	local definition = makeDefinition(
	{
		templateName = "Racing Template",
		genre = GameTemplateGenre.Racing,
		description = "A simple racing template with car player, track, barriers, checkpoints, and chase camera.",
		playerFallbackPrimitive = "Cube",
		playerSpawnPosition = runtime.vec3(0.0, 1.0, -32.0),
		playerScale = runtime.vec3(1.8, 0.6, 3.2),
		cameraPosition = runtime.vec3(0.0, 6.0, -12.0),
		cameraRotation = runtime.vec3(20.0, 0.0, 0.0),
		hudTitle = "Racing Template",
		objectiveText = "Objective: complete all checkpoints."
	})

	definition:AddEnvironmentObject(spawn({ name = "Track Base", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 0.0, 0.0), scale = runtime.vec3(40.0, 0.25, 90.0), color = runtime.color(0.12, 0.12, 0.12, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "Left Barrier", fallbackPrimitive = "Cube", position = runtime.vec3(-8.0, 1.0, 0.0), scale = runtime.vec3(0.5, 2.0, 80.0), color = runtime.color(0.7, 0.1, 0.1, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "Right Barrier", fallbackPrimitive = "Cube", position = runtime.vec3(8.0, 1.0, 0.0), scale = runtime.vec3(0.5, 2.0, 80.0), color = runtime.color(0.7, 0.1, 0.1, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "Start Line", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 0.1, -30.0), scale = runtime.vec3(10.0, 0.1, 1.0), color = runtime.color(1.0, 1.0, 1.0, 1.0) }))

	for i = 0, 4 do
		definition:AddGameplayObject(spawn({ name = "Checkpoint " .. i, fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 1.5, -25.0 + i * 14.0), scale = runtime.vec3(12.0, 3.0, 0.35), isTrigger = true, color = runtime.color(0.1, 0.5, 1.0, 0.5) }))
	end

	return definition
end

function GameTemplateGenerator.CreateFlightDefinition()
	local runtime = rt()
	local definition = makeDefinition(
	{
		templateName = "Flight Template",
		genre = GameTemplateGenre.Flight,
		description = "A flight template with runway, terrain placeholder, flight rings, and long-range camera.",
		playerFallbackPrimitive = "Cube",
		playerSpawnPosition = runtime.vec3(0.0, 10.0, 0.0),
		playerScale = runtime.vec3(3.5, 0.35, 2.5),
		cameraPosition = runtime.vec3(0.0, 14.0, -28.0),
		cameraRotation = runtime.vec3(20.0, 0.0, 0.0),
		farClipPlane = 5000.0,
		hudTitle = "Flight Template",
		objectiveText = "Objective: fly through the rings."
	})

	definition:AddEnvironmentObject(spawn({ name = "Runway", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 0.0, 0.0), scale = runtime.vec3(12.0, 0.25, 80.0), color = runtime.color(0.18, 0.18, 0.18, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "Terrain Placeholder", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, -1.0, 0.0), scale = runtime.vec3(200.0, 0.5, 200.0), color = runtime.color(0.25, 0.45, 0.25, 1.0) }))

	for i = 0, 5 do
		definition:AddGameplayObject(spawn({ name = "Flight Ring " .. i, fallbackPrimitive = "Torus", position = runtime.vec3(0.0, 8.0 + i * 2.0, 20.0 + i * 25.0), rotationEuler = runtime.vec3(90.0, 0.0, 0.0), scale = runtime.vec3(4.0, 4.0, 4.0), isTrigger = true, color = runtime.color(1.0, 0.75, 0.15, 1.0) }))
	end

	return definition
end

function GameTemplateGenerator.CreateRPGDefinition()
	local runtime = rt()
	local definition = makeDefinition(
	{
		templateName = "RPG Template",
		genre = GameTemplateGenre.RPG,
		description = "A small RPG village template with NPCs and a quest marker.",
		playerFallbackPrimitive = "Capsule",
		playerSpawnPosition = runtime.vec3(0.0, 1.0, 0.0),
		cameraPosition = runtime.vec3(0.0, 14.0, -10.0),
		cameraRotation = runtime.vec3(55.0, 0.0, 0.0),
		orthographicCamera = true,
		orthographicSize = 9.0,
		hudTitle = "RPG Template",
		objectiveText = "Objective: talk to NPCs and complete quests."
	})

	definition:AddEnvironmentObject(spawn({ name = "Village Ground", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 0.0, 0.0), scale = runtime.vec3(30.0, 0.5, 30.0), color = runtime.color(0.24, 0.42, 0.24, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "House 01", fallbackPrimitive = "Cube", position = runtime.vec3(-7.0, 1.5, 6.0), scale = runtime.vec3(4.0, 3.0, 4.0), color = runtime.color(0.5, 0.32, 0.22, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "House 02", fallbackPrimitive = "Cube", position = runtime.vec3(7.0, 1.5, 5.0), scale = runtime.vec3(4.0, 3.0, 4.0), color = runtime.color(0.5, 0.32, 0.22, 1.0) }))
	definition:AddEnvironmentObject(spawn({ name = "Shop", fallbackPrimitive = "Cube", position = runtime.vec3(0.0, 1.5, 10.0), scale = runtime.vec3(5.0, 3.0, 4.0), color = runtime.color(0.45, 0.35, 0.28, 1.0) }))
	definition:AddGameplayObject(spawn({ name = "Quest Giver", fallbackPrimitive = "Capsule", position = runtime.vec3(-3.0, 1.0, 4.0), color = runtime.color(0.2, 0.5, 1.0, 1.0) }))
	definition:AddGameplayObject(spawn({ name = "Merchant", fallbackPrimitive = "Capsule", position = runtime.vec3(4.0, 1.0, 8.0), color = runtime.color(0.9, 0.65, 0.25, 1.0) }))
	definition:AddGameplayObject(spawn({ name = "Quest Marker", fallbackPrimitive = "Cube", position = runtime.vec3(-3.0, 2.5, 4.0), scale = runtime.vec3(0.5, 0.5, 0.5), isTrigger = true, color = runtime.color(0.1, 0.9, 0.3, 1.0) }))

	return definition
end

function GameTemplateGenerator.GetObjectiveText(genre)
	if genre == GameTemplateGenre.Platformer then
		return "Objective: collect items and reach the goal."
	elseif genre == GameTemplateGenre.TopDownShooter then
		return "Objective: survive enemy waves."
	elseif genre == GameTemplateGenre.Racing then
		return "Objective: complete all checkpoints."
	elseif genre == GameTemplateGenre.Flight then
		return "Objective: fly through the rings."
	elseif genre == GameTemplateGenre.RPG then
		return "Objective: talk to NPCs and complete quests."
	end

	return "Objective: build your game."
end
