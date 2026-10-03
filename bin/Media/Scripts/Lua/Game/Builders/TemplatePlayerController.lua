class 'TemplatePlayerController' (BaseComponent)

local function axis(name)
	if Input and Input.GetAxis then
		return Input.GetAxis(name)
	end

	return 0.0
end

local function button(name)
	if Input and Input.GetButton then
		return Input.GetButton(name)
	end

	return false
end

local function key(code)
	if Input and Input.GetKey then
		return Input.GetKey(code)
	end

	return false
end

local function vec3(x, y, z)
	if TemplateBuilderRuntime and TemplateBuilderRuntime.vec3 then
		return TemplateBuilderRuntime.vec3(x, y, z)
	end

	if Vector3 then
		return Vector3(x or 0.0, y or 0.0, z or 0.0)
	end

	return { x = x or 0.0, y = y or 0.0, z = z or 0.0 }
end

local function magnitudeSquared(v)
	return (v.x or 0.0) * (v.x or 0.0) + (v.y or 0.0) * (v.y or 0.0) + (v.z or 0.0) * (v.z or 0.0)
end

local function normalized(v)
	local length = math.sqrt(magnitudeSquared(v))

	if length <= 0.0001 then
		return vec3(0.0, 0.0, 0.0)
	end

	return vec3((v.x or 0.0) / length, (v.y or 0.0) / length, (v.z or 0.0) / length)
end

local function scale(v, amount)
	return vec3((v.x or 0.0) * amount, (v.y or 0.0) * amount, (v.z or 0.0) * amount)
end

function TemplatePlayerController:__init(component)
	BaseComponent.__init(self, component)
	self.genre = GameTemplateGenre and GameTemplateGenre.Custom or "Custom"
	self.moveSpeed = 8.0
	self.turnSpeed = 120.0
	self.jumpForce = 7.0
	self.flightLift = 12.0
	self.flightPitchSpeed = 60.0
	self.flightRollSpeed = 90.0
	self.body = nil
	self.isGrounded = false
end

function TemplatePlayerController:__finalize()
	BaseComponent.__finalize(self)
end

function TemplatePlayerController:SetGenre(value)
	self.genre = value
end

function TemplatePlayerController:Awake()
	self.body = self:GetComponent("Rigidbody")

	if self.body then
		self.body.mass = 1.0
		self.body.interpolation = "Interpolate"
	end
end

function TemplatePlayerController:update()
	self:FixedUpdate()
end

function TemplatePlayerController:FixedUpdate()
	if not self.body then
		self.body = self:GetComponent("Rigidbody")
	end

	if self.genre == GameTemplateGenre.Platformer then
		self:UpdatePlatformer()
	elseif self.genre == GameTemplateGenre.TopDownShooter then
		self:UpdateTopDownShooter()
	elseif self.genre == GameTemplateGenre.Racing then
		self:UpdateRacing()
	elseif self.genre == GameTemplateGenre.Flight then
		self:UpdateFlight()
	elseif self.genre == GameTemplateGenre.RPG then
		self:UpdateRPG()
	end
end

function TemplatePlayerController:GetComponent(componentName)
	if BaseComponent.GetComponent then
		return BaseComponent.GetComponent(self, componentName)
	end

	local owner = self.entity or self.gameObject or self.component
	if owner and owner.GetComponent then
		return owner:GetComponent(componentName)
	end

	return owner and owner.components and owner.components[componentName] or nil
end

function TemplatePlayerController:SetVelocity(velocity)
	if not self.body then
		return
	end

	self.body.linearVelocity = velocity
	self.body.velocity = velocity
end

function TemplatePlayerController:AddForce(force, mode)
	if self.body and self.body.AddForce then
		self.body:AddForce(force, mode or "Acceleration")
	elseif self.body then
		self.body.lastForce = force
	end
end

function TemplatePlayerController:MoveRotation(rotation)
	if self.body and self.body.MoveRotation then
		self.body:MoveRotation(rotation)
	elseif self.body then
		self.body.rotation = rotation
	end
end

function TemplatePlayerController:UpdatePlatformer()
	local x = axis("Horizontal")
	local current = self.body and (self.body.linearVelocity or self.body.velocity) or vec3(0.0, 0.0, 0.0)
	self:SetVelocity(vec3(x * self.moveSpeed, current.y or 0.0, current.z or 0.0))

	if button("Jump") and self.isGrounded then
		self:AddForce(vec3(0.0, self.jumpForce, 0.0), "VelocityChange")
		self.isGrounded = false
	end
end

function TemplatePlayerController:UpdateTopDownShooter()
	local movement = normalized(vec3(axis("Horizontal"), 0.0, axis("Vertical")))
	self:SetVelocity(scale(movement, self.moveSpeed))
	self:FaceDirection(movement)
end

function TemplatePlayerController:UpdateRacing()
	local throttle = axis("Vertical")
	local steering = axis("Horizontal")
	local transform = self.transform or (self.entity and self.entity.transform) or {}
	local forward = transform.forward or vec3(0.0, 0.0, 1.0)
	self:AddForce(scale(forward, throttle * self.moveSpeed), "Acceleration")
	self.lastSteering = steering
end

function TemplatePlayerController:UpdateFlight()
	local throttle = math.max(0.0, math.min(1.0, axis("Vertical")))
	local yaw = axis("Horizontal")
	local pitch = 0.0
	local roll = 0.0

	if key("W") then pitch = -1.0 end
	if key("S") then pitch = 1.0 end
	if key("A") then roll = 1.0 end
	if key("D") then roll = -1.0 end

	local transform = self.transform or (self.entity and self.entity.transform) or {}
	local forward = transform.forward or vec3(0.0, 0.0, 1.0)
	self:AddForce(scale(forward, self.moveSpeed * throttle), "Acceleration")
	self:AddForce(vec3(0.0, self.flightLift * throttle, 0.0), "Acceleration")
	self.lastFlightInput = { pitch = pitch, yaw = yaw, roll = roll }
end

function TemplatePlayerController:UpdateRPG()
	local movement = normalized(vec3(axis("Horizontal"), 0.0, axis("Vertical")))
	self:SetVelocity(scale(movement, self.moveSpeed))
	self:FaceDirection(movement)
end

function TemplatePlayerController:FaceDirection(direction)
	if magnitudeSquared(direction) <= 0.001 then
		return
	end

	local transform = self.transform or (self.entity and self.entity.transform)

	if not transform then
		return
	end

	if Quaternion and Quaternion.LookRotation then
		transform.rotation = Quaternion.LookRotation(direction)
	else
		transform.forward = direction
	end
end

function TemplatePlayerController:OnCollisionStay(collision)
	if not collision or not collision.contacts then
		return
	end

	for _, contact in ipairs(collision.contacts) do
		local normal = contact.normal or {}
		if (normal.y or 0.0) > 0.5 then
			self.isGrounded = true
			return
		end
	end
end

