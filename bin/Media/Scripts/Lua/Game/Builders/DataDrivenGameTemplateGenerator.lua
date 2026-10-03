DataDrivenGameTemplateGenerator = DataDrivenGameTemplateGenerator or {}

local function generatorRuntime()
	if TemplateBuilderRuntime then
		return TemplateBuilderRuntime
	end

	return
	{
		vec3 = function(x, y, z)
			if Vector3 then
				return Vector3(x or 0.0, y or 0.0, z or 0.0)
			end

			return { x = x or 0.0, y = y or 0.0, z = z or 0.0 }
		end,
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
end

function DataDrivenGameTemplateGenerator.Generate(definition, clearPreviousGeneratedContent)
	if not definition then
		print("Cannot generate template. Definition is nil.")
		return nil
	end

	if clearPreviousGeneratedContent then
		DataDrivenGameTemplateGenerator.ClearPreviousGeneratedContent()
	end

	local runtime = generatorRuntime()
	local root = runtime.newGameObject("Generated Template - " .. tostring(definition.templateName or "Untitled"))
	runtime.addComponent(root, "TemplateGeneratedMarker")
	root.templateDefinition = definition

	local managers = DataDrivenGameTemplateGenerator.CreateChild(root.transform, "Managers")
	local world = DataDrivenGameTemplateGenerator.CreateChild(root.transform, "World")
	local gameplay = DataDrivenGameTemplateGenerator.CreateChild(root.transform, "Gameplay")
	local ui = DataDrivenGameTemplateGenerator.CreateChild(root.transform, "UI")

	local spawnPoint = DataDrivenGameTemplateGenerator.CreateSpawnPoint(gameplay.transform, definition)
	local player = definition:CreatePlayer(gameplay.transform)

	DataDrivenGameTemplateGenerator.CreateManagers(managers.transform, definition)
	DataDrivenGameTemplateGenerator.CreateGeneratedGameManager(managers.transform, definition, player, spawnPoint.transform)

	if definition.createLighting then
		DataDrivenGameTemplateGenerator.CreateLighting(root.transform, definition)
	end

	if definition.createCamera then
		DataDrivenGameTemplateGenerator.CreateCamera(root.transform, definition, player.transform)
	end

	root.generatedEnvironmentObjects = DataDrivenGameTemplateGenerator.CreateSpawnList(world.transform, definition.environmentObjects)
	root.generatedGameplayObjects = DataDrivenGameTemplateGenerator.CreateSpawnList(gameplay.transform, definition.gameplayObjects)

	if definition.createHUD then
		DataDrivenGameTemplateGenerator.CreateHUD(ui.transform, definition)
	end

	DataDrivenGameTemplateGenerator.lastGeneratedRoot = root
	return root
end

function DataDrivenGameTemplateGenerator.ClearPreviousGeneratedContent()
	local runtime = generatorRuntime()
	local destroy = runtime.destroy or function(obj)
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

	if Object and Object.FindObjectsByType then
		local markers = Object.FindObjectsByType("TemplateGeneratedMarker")

		if markers then
			for i = #markers, 1, -1 do
				local marker = markers[i]
				local obj = marker and marker.gameObject
				if obj then
					destroy(obj)
				end
			end
		end
	elseif DataDrivenGameTemplateGenerator.lastGeneratedRoot then
		destroy(DataDrivenGameTemplateGenerator.lastGeneratedRoot)
	end

	DataDrivenGameTemplateGenerator.lastGeneratedRoot = nil
end

function DataDrivenGameTemplateGenerator.CreateSpawnPoint(parent, definition)
	local runtime = generatorRuntime()
	local spawn = DataDrivenGameTemplateGenerator.CreateChild(parent, "Player Spawn Point")

	if runtime.setTransform then
		runtime.setTransform(spawn, definition.playerSpawnPosition, definition.playerSpawnRotation, runtime.vec3(1.0, 1.0, 1.0))
	else
		spawn.transform.localPosition = definition.playerSpawnPosition
		spawn.transform.position = definition.playerSpawnPosition
		spawn.transform.localRotation = runtime.euler(definition.playerSpawnRotation)
		spawn.transform.rotation = spawn.transform.localRotation
	end

	return spawn
end

function DataDrivenGameTemplateGenerator.CreateManagers(parent, definition)
	local runtime = generatorRuntime()

	for _, prefab in ipairs(definition.managerPrefabs or {}) do
		if prefab then
			local instance = nil

			if GameObject and GameObject.Instantiate then
				instance = GameObject.Instantiate(prefab, parent)
			elseif Object and Object.Instantiate then
				instance = Object.Instantiate(prefab, parent)
			end

			if not instance then
				instance = runtime.newGameObject(prefab.name or "Manager")
				instance.prefab = prefab
			end

			instance.name = prefab.name or instance.name
			runtime.setParent(instance, parent)
		end
	end
end

function DataDrivenGameTemplateGenerator.CreateGeneratedGameManager(parent, definition, player, spawnPoint)
	local runtime = generatorRuntime()
	local gameManagerObj = DataDrivenGameTemplateGenerator.CreateChild(parent, "Generated Game Manager")
	local manager = runtime.addComponent(gameManagerObj, "GeneratedGameManager")

	if manager and manager.Initialize then
		manager:Initialize(definition.genre, player, spawnPoint)
	elseif manager then
		manager.genre = definition.genre
		manager.player = player
		manager.playerSpawnPoint = spawnPoint
	end

	return manager
end

function DataDrivenGameTemplateGenerator.CreateLighting(parent, definition)
	local runtime = generatorRuntime()
	local lightObj = runtime.newGameObject("Directional Light")
	runtime.setParent(lightObj, parent)

	if runtime.setTransform then
		runtime.setTransform(lightObj, runtime.vec3(0.0, 0.0, 0.0), runtime.vec3(50.0, -30.0, 0.0), runtime.vec3(1.0, 1.0, 1.0))
	else
		lightObj.transform = lightObj.transform or {}
		lightObj.transform.rotation = runtime.euler(runtime.vec3(50.0, -30.0, 0.0))
	end

	local light = runtime.addComponent(lightObj, "Light")
	if light then
		light.type = "Directional"
		light.intensity = 1.2
	end

	if RenderSettings then
		RenderSettings.ambientLight = definition.ambientLight
	end

	return lightObj
end

function DataDrivenGameTemplateGenerator.CreateCamera(parent, definition, target)
	local runtime = generatorRuntime()
	local cameraObj = runtime.newGameObject("Main Camera")
	runtime.setParent(cameraObj, parent)
	cameraObj.tag = "MainCamera"

	if runtime.setTransform then
		runtime.setTransform(cameraObj, definition.cameraPosition, definition.cameraRotation, runtime.vec3(1.0, 1.0, 1.0))
	else
		cameraObj.transform = cameraObj.transform or {}
		cameraObj.transform.localPosition = definition.cameraPosition
		cameraObj.transform.position = definition.cameraPosition
		cameraObj.transform.localRotation = runtime.euler(definition.cameraRotation)
		cameraObj.transform.rotation = cameraObj.transform.localRotation
	end

	local camera = runtime.addComponent(cameraObj, "Camera")
	if camera then
		camera.clearFlags = "Skybox"
		camera.orthographic = definition.orthographicCamera
		camera.orthographicSize = definition.orthographicSize
		camera.farClipPlane = definition.farClipPlane
	end

	if definition.addCameraFollow then
		local follow = runtime.addComponent(cameraObj, "CameraFollow")

		if follow and follow.SetTarget then
			follow:SetTarget(target)
		elseif follow then
			follow.target = target
		end

		if follow and follow.SetGenre then
			follow:SetGenre(definition.genre)
		elseif follow then
			follow.genre = definition.genre
		end
	end

	return camera
end

function DataDrivenGameTemplateGenerator.CreateSpawnList(parent, list)
	local created = {}

	for _, spawn in ipairs(list or {}) do
		if spawn and spawn.CreateInstance then
			table.insert(created, spawn:CreateInstance(parent))
		elseif spawn and TemplateSpawnDefinition then
			local definition = TemplateSpawnDefinition(spawn)
			table.insert(created, definition:CreateInstance(parent))
		end
	end

	return created
end

function DataDrivenGameTemplateGenerator.CreateHUD(parent, definition)
	local runtime = generatorRuntime()
	local canvasObj = DataDrivenGameTemplateGenerator.CreateChild(parent, "Game HUD Canvas")
	local canvas = runtime.addComponent(canvasObj, "Canvas")

	if canvas then
		canvas.renderMode = "ScreenSpaceOverlay"
	end

	local scaler = runtime.addComponent(canvasObj, "CanvasScaler")
	if scaler then
		scaler.uiScaleMode = "ScaleWithScreenSize"
		scaler.referenceResolution = { x = 1920, y = 1080 }
		scaler.matchWidthOrHeight = 0.5
	end

	runtime.addComponent(canvasObj, "GraphicRaycaster")

	DataDrivenGameTemplateGenerator.CreateUIText(
		canvasObj.transform,
		"HUD Title",
		definition.hudTitle,
		32,
		{ x = 20, y = -20 },
		"UpperLeft"
	)

	DataDrivenGameTemplateGenerator.CreateUIText(
		canvasObj.transform,
		"Objective Text",
		definition.objectiveText,
		24,
		{ x = 20, y = -70 },
		"UpperLeft"
	)

	return canvasObj
end

function DataDrivenGameTemplateGenerator.CreateUIText(parent, name, text, size, position, alignment)
	local runtime = generatorRuntime()
	local obj = DataDrivenGameTemplateGenerator.CreateChild(parent, name)
	local rect = runtime.addComponent(obj, "RectTransform")

	if rect then
		rect.anchorMin = { x = 0.0, y = 1.0 }
		rect.anchorMax = { x = 0.0, y = 1.0 }
		rect.pivot = { x = 0.0, y = 1.0 }
		rect.sizeDelta = { x = 900, y = 60 }
		rect.anchoredPosition = position
	end

	local label = runtime.addComponent(obj, "Text")
	if label then
		label.text = text or ""
		label.fontSize = size or 24
		label.color = runtime.color(1.0, 1.0, 1.0, 1.0)
		label.alignment = alignment or "UpperLeft"
	end

	return label
end

function DataDrivenGameTemplateGenerator.CreateChild(parent, name)
	local runtime = generatorRuntime()
	local obj = runtime.newGameObject(name)
	runtime.setParent(obj, parent)
	obj.transform = obj.transform or {}
	return obj
end
