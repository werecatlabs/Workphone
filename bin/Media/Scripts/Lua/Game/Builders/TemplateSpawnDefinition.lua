local function templateVec3(x, y, z)
	if Vector3 then
		return Vector3(x or 0.0, y or 0.0, z or 0.0)
	end

	return { x = x or 0.0, y = y or 0.0, z = z or 0.0 }
end

local function templateColor(r, g, b, a)
	if Color then
		return Color(r or 1.0, g or 1.0, b or 1.0, a or 1.0)
	end

	return { r = r or 1.0, g = g or 1.0, b = b or 1.0, a = a or 1.0 }
end

local function templateEuler(rotation)
	rotation = rotation or templateVec3(0.0, 0.0, 0.0)

	if Quaternion and Quaternion.Euler then
		return Quaternion.Euler(rotation)
	end

	return rotation
end

local function templateNewGameObject(name)
	if GameObject then
		if GameObject.New then
			return GameObject.New(name)
		end

		if GameObject.Create then
			return GameObject.Create(name)
		end

		local ok, obj = pcall(GameObject, name)
		if ok then
			return obj
		end
	end

	return { name = name, transform = {} }
end

local function templateCreatePrimitive(primitiveType, name)
	local obj = nil

	if GameObject and GameObject.CreatePrimitive then
		obj = GameObject.CreatePrimitive(primitiveType or "Cube")
	else
		obj = templateNewGameObject(name)
	end

	if obj then
		obj.name = name
		obj.primitiveType = primitiveType or "Cube"
		obj.transform = obj.transform or {}
	end

	return obj
end

local function templateSetParent(obj, parent)
	if not obj then
		return
	end

	obj.transform = obj.transform or {}

	if parent and obj.transform.SetParent then
		obj.transform:SetParent(parent, false)
	else
		obj.parent = parent

		if parent then
			parent.children = parent.children or {}
			table.insert(parent.children, obj)
		end
	end
end

local function templateSetTransform(obj, position, rotationEuler, scale)
	if not obj then
		return
	end

	obj.transform = obj.transform or {}
	obj.transform.localPosition = position or templateVec3(0.0, 0.0, 0.0)
	obj.transform.position = obj.transform.localPosition
	obj.transform.localRotation = templateEuler(rotationEuler)
	obj.transform.rotation = obj.transform.localRotation
	obj.transform.localScale = scale or templateVec3(1.0, 1.0, 1.0)
end

local function templateAddComponent(obj, componentName)
	if not obj then
		return nil
	end

	if obj.AddComponent then
		return obj:AddComponent(componentName)
	end

	obj.components = obj.components or {}
	obj.components[componentName] = obj.components[componentName] or { gameObject = obj, transform = obj.transform }
	return obj.components[componentName]
end

local function templateGetComponent(obj, componentName)
	if not obj then
		return nil
	end

	if obj.GetComponent then
		return obj:GetComponent(componentName)
	end

	return obj.components and obj.components[componentName] or nil
end

local function templateGetComponentInChildren(obj, componentName)
	if not obj then
		return nil
	end

	if obj.GetComponentInChildren then
		return obj:GetComponentInChildren(componentName)
	end

	return templateGetComponent(obj, componentName)
end

local function templateDestroy(obj)
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

TemplateBuilderRuntime = TemplateBuilderRuntime or {}
TemplateBuilderRuntime.vec3 = TemplateBuilderRuntime.vec3 or templateVec3
TemplateBuilderRuntime.color = TemplateBuilderRuntime.color or templateColor
TemplateBuilderRuntime.euler = TemplateBuilderRuntime.euler or templateEuler
TemplateBuilderRuntime.newGameObject = TemplateBuilderRuntime.newGameObject or templateNewGameObject
TemplateBuilderRuntime.createPrimitive = TemplateBuilderRuntime.createPrimitive or templateCreatePrimitive
TemplateBuilderRuntime.setParent = TemplateBuilderRuntime.setParent or templateSetParent
TemplateBuilderRuntime.setTransform = TemplateBuilderRuntime.setTransform or templateSetTransform
TemplateBuilderRuntime.addComponent = TemplateBuilderRuntime.addComponent or templateAddComponent
TemplateBuilderRuntime.getComponent = TemplateBuilderRuntime.getComponent or templateGetComponent
TemplateBuilderRuntime.getComponentInChildren = TemplateBuilderRuntime.getComponentInChildren or templateGetComponentInChildren
TemplateBuilderRuntime.destroy = TemplateBuilderRuntime.destroy or templateDestroy

class 'TemplateSpawnDefinition'

function TemplateSpawnDefinition:__init(values)
	values = values or {}

	self.name = values.name or "Template Object"
	self.prefab = values.prefab
	self.fallbackPrimitive = values.fallbackPrimitive or "Cube"
	self.useFallbackPrimitive = values.useFallbackPrimitive ~= false
	self.position = values.position or templateVec3(0.0, 0.0, 0.0)
	self.rotationEuler = values.rotationEuler or templateVec3(0.0, 0.0, 0.0)
	self.scale = values.scale or templateVec3(1.0, 1.0, 1.0)
	self.addBoxCollider = values.addBoxCollider ~= false
	self.addRigidbody = values.addRigidbody == true
	self.isTrigger = values.isTrigger == true
	self.color = values.color or templateColor(0.5, 0.5, 0.5, 1.0)
	self.applyColor = values.applyColor ~= false
end

function TemplateSpawnDefinition:CreateInstance(parent)
	local runtime = TemplateBuilderRuntime
	local instance = nil

	if self.prefab then
		if GameObject and GameObject.Instantiate then
			instance = GameObject.Instantiate(self.prefab, parent)
		elseif Object and Object.Instantiate then
			instance = Object.Instantiate(self.prefab, parent)
		end

		if not instance then
			instance = runtime.newGameObject(self.name)
			instance.prefab = self.prefab
		end

		instance.name = self.name
		runtime.setParent(instance, parent)
	else
		if self.useFallbackPrimitive then
			instance = runtime.createPrimitive(self.fallbackPrimitive, self.name)
		else
			instance = runtime.newGameObject(self.name)
		end

		runtime.setParent(instance, parent)
	end

	if runtime.setTransform then
		runtime.setTransform(instance, self.position, self.rotationEuler, self.scale)
	else
		instance.transform = instance.transform or {}
		instance.transform.localPosition = self.position
		instance.transform.position = self.position
		instance.transform.localRotation = runtime.euler(self.rotationEuler)
		instance.transform.rotation = instance.transform.localRotation
		instance.transform.localScale = self.scale
	end

	self:ConfigureCollider(instance)
	self:ConfigureRigidbody(instance)
	self:ConfigureMaterial(instance)

	return instance
end

function TemplateSpawnDefinition:ConfigureCollider(instance)
	local runtime = TemplateBuilderRuntime
	local collider = runtime.getComponent(instance, "Collider") or runtime.getComponent(instance, "BoxCollider")

	if not self.addBoxCollider then
		runtime.destroy(collider)
		return
	end

	if not collider then
		collider = runtime.addComponent(instance, "BoxCollider")
	end

	if collider then
		collider.isTrigger = self.isTrigger
	end
end

function TemplateSpawnDefinition:ConfigureRigidbody(instance)
	local runtime = TemplateBuilderRuntime
	local rigidbody = runtime.getComponent(instance, "Rigidbody")

	if not self.addRigidbody then
		runtime.destroy(rigidbody)
		return
	end

	if not rigidbody then
		rigidbody = runtime.addComponent(instance, "Rigidbody")
	end
end

function TemplateSpawnDefinition:ConfigureMaterial(instance)
	if not self.applyColor then
		return
	end

	local runtime = TemplateBuilderRuntime
	local renderer = runtime.getComponentInChildren(instance, "Renderer")

	if not renderer then
		renderer = runtime.addComponent(instance, "Renderer")
	end

	if not renderer then
		return
	end

	local material = { color = self.color }
	if Material and Shader and Shader.Find then
		material = Material(Shader.Find("Standard"))
		material.color = self.color
	end

	renderer.sharedMaterial = material
	renderer.material = material
end
