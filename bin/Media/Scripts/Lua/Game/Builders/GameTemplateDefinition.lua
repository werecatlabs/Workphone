local function definitionRuntime()
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

class 'GameTemplateDefinition'

function GameTemplateDefinition:__init(values)
	values = values or {}
	local runtime = definitionRuntime()

	self.templateName = values.templateName or "New Game Template"
	self.genre = values.genre or (GameTemplateGenre and GameTemplateGenre.Custom or "Custom")
	self.description = values.description or "A generated game template."
	self.createLighting = values.createLighting ~= false
	self.createHUD = values.createHUD ~= false
	self.ambientLight = values.ambientLight or runtime.color(0.35, 0.35, 0.38, 1.0)
	self.playerPrefab = values.playerPrefab
	self.playerFallbackPrimitive = values.playerFallbackPrimitive or "Capsule"
	self.playerSpawnPosition = values.playerSpawnPosition or runtime.vec3(0.0, 1.0, 0.0)
	self.playerSpawnRotation = values.playerSpawnRotation or runtime.vec3(0.0, 0.0, 0.0)
	self.playerScale = values.playerScale or runtime.vec3(1.0, 1.0, 1.0)
	self.addTemplatePlayerController = values.addTemplatePlayerController ~= false
	self.addPlayerRigidbody = values.addPlayerRigidbody ~= false
	self.createCamera = values.createCamera ~= false
	self.cameraPosition = values.cameraPosition or runtime.vec3(0.0, 8.0, -10.0)
	self.cameraRotation = values.cameraRotation or runtime.vec3(45.0, 0.0, 0.0)
	self.addCameraFollow = values.addCameraFollow ~= false
	self.orthographicCamera = values.orthographicCamera == true
	self.orthographicSize = values.orthographicSize or 10.0
	self.farClipPlane = values.farClipPlane or 1000.0
	self.environmentObjects = values.environmentObjects or {}
	self.gameplayObjects = values.gameplayObjects or {}
	self.managerPrefabs = values.managerPrefabs or {}
	self.hudTitle = values.hudTitle or "Generated Template"
	self.objectiveText = values.objectiveText or "Objective: build your game."
end

function GameTemplateDefinition:AddEnvironmentObject(spawnDefinition)
	table.insert(self.environmentObjects, spawnDefinition)
	return spawnDefinition
end

function GameTemplateDefinition:AddGameplayObject(spawnDefinition)
	table.insert(self.gameplayObjects, spawnDefinition)
	return spawnDefinition
end

function GameTemplateDefinition:CreatePlayer(parent)
	local runtime = definitionRuntime()
	local player = nil

	if self.playerPrefab then
		if GameObject and GameObject.Instantiate then
			player = GameObject.Instantiate(self.playerPrefab, parent)
		elseif Object and Object.Instantiate then
			player = Object.Instantiate(self.playerPrefab, parent)
		end

		if not player then
			player = runtime.newGameObject("Player")
			player.prefab = self.playerPrefab
		end

		player.name = "Player"
		runtime.setParent(player, parent)
	else
		player = runtime.createPrimitive(self.playerFallbackPrimitive, "Player")
		runtime.setParent(player, parent)
	end

	if runtime.setTransform then
		runtime.setTransform(player, self.playerSpawnPosition, self.playerSpawnRotation, self.playerScale)
	else
		player.transform = player.transform or {}
		player.transform.localPosition = self.playerSpawnPosition
		player.transform.position = self.playerSpawnPosition
		player.transform.localRotation = runtime.euler(self.playerSpawnRotation)
		player.transform.rotation = player.transform.localRotation
		player.transform.localScale = self.playerScale
	end

	if self.addPlayerRigidbody and not runtime.getComponent(player, "Rigidbody") then
		local body = runtime.addComponent(player, "Rigidbody")

		if body then
			if self.genre == GameTemplateGenre.TopDownShooter or
				self.genre == GameTemplateGenre.RPG or
				self.genre == GameTemplateGenre.Racing then
				body.constraints = "FreezeRotationX|FreezeRotationZ"
			end

			if self.genre == GameTemplateGenre.Flight then
				body.useGravity = false
			end
		end
	end

	if self.addTemplatePlayerController and not runtime.getComponent(player, "TemplatePlayerController") then
		local controller = runtime.addComponent(player, "TemplatePlayerController")

		if controller and controller.SetGenre then
			controller:SetGenre(self.genre)
		elseif controller then
			controller.genre = self.genre
		end
	end

	return player
end
