-- Deterministic gameplay model for the Catch Game sample.
-- This file intentionally has no engine dependencies so it can be unit tested
-- and reused by server-side simulations or alternative renderers.

CatchGameCore = CatchGameCore or {}

local defaults = {
	width = 900.0,
	height = 600.0,
	playerWidth = 120.0,
	playerHeight = 28.0,
	playerY = 245.0,
	playerSpeed = 520.0,
	itemSize = 36.0,
	itemFallSpeed = 220.0,
	speedIncreasePerCatch = 4.0,
	spawnInterval = 0.75,
	maxMisses = 3,
	maxFrameDelta = 0.25,
	maxSpawnsPerUpdate = 4,
}

local function finite(value)
	return type(value) == "number" and value == value
		and value ~= math.huge and value ~= -math.huge
end

local function numberOr(value, fallback, minimum)
	if not finite(value) then
		return fallback
	end
	if minimum ~= nil and value < minimum then
		return minimum
	end
	return value
end

local function clamp(value, minimum, maximum)
	return math.max(minimum, math.min(maximum, value))
end

local function copyConfig(overrides)
	local result = {}
	for key, value in pairs(defaults) do
		result[key] = value
	end
	for key, value in pairs(overrides or {}) do
		result[key] = value
	end

	result.width = numberOr(result.width, defaults.width, 1.0)
	result.height = numberOr(result.height, defaults.height, 1.0)
	result.playerWidth = numberOr(result.playerWidth, defaults.playerWidth, 1.0)
	result.playerHeight = numberOr(result.playerHeight, defaults.playerHeight, 1.0)
	result.playerSpeed = numberOr(result.playerSpeed, defaults.playerSpeed, 0.0)
	result.itemSize = numberOr(result.itemSize, defaults.itemSize, 1.0)
	result.itemFallSpeed = numberOr(result.itemFallSpeed, defaults.itemFallSpeed, 0.0)
	result.speedIncreasePerCatch =
		numberOr(result.speedIncreasePerCatch, defaults.speedIncreasePerCatch, 0.0)
	result.spawnInterval = numberOr(result.spawnInterval, defaults.spawnInterval, 0.01)
	result.maxMisses = math.max(1, math.floor(numberOr(result.maxMisses, defaults.maxMisses, 1)))
	result.maxFrameDelta = numberOr(result.maxFrameDelta, defaults.maxFrameDelta, 0.001)
	result.maxSpawnsPerUpdate =
		math.max(1, math.floor(numberOr(result.maxSpawnsPerUpdate,
			defaults.maxSpawnsPerUpdate, 1)))
	result.playerY = numberOr(result.playerY, result.height * 0.4)
	return result
end

function CatchGameCore.new(config, random)
	local game = {
		config = copyConfig(config),
		random = type(random) == "function" and random or math.random,
	}
	return setmetatable(game, { __index = CatchGameCore }):reset()
end

function CatchGameCore:reset()
	self.playerX = 0.0
	self.score = 0
	self.misses = 0
	self.running = true
	self.spawnElapsed = 0.0
	self.nextItemId = 1
	self.items = {}
	return self
end

function CatchGameCore:getPlayerLimit()
	return math.max(0.0, (self.config.width - self.config.playerWidth) * 0.5)
end

function CatchGameCore:spawnItem(x, y)
	local halfWidth = math.max(0.0, (self.config.width - self.config.itemSize) * 0.5)
	local randomValue = tonumber(self.random()) or 0.5
	randomValue = clamp(randomValue, 0.0, 1.0)
	local item = {
		id = self.nextItemId,
		x = clamp(tonumber(x) or ((randomValue * 2.0 - 1.0) * halfWidth),
			-halfWidth, halfWidth),
		y = tonumber(y) or (-self.config.height * 0.5 - self.config.itemSize),
	}
	self.nextItemId = self.nextItemId + 1
	table.insert(self.items, item)
	return item
end

function CatchGameCore:update(deltaTime, inputAxis)
	local events = {}
	if not self.running then
		return events
	end

	local dt = finite(deltaTime) and math.max(0.0, deltaTime) or 0.0
	dt = math.min(dt, self.config.maxFrameDelta)
	local axis = finite(inputAxis) and clamp(inputAxis, -1.0, 1.0) or 0.0
	self.playerX = clamp(self.playerX + axis * self.config.playerSpeed * dt,
		-self:getPlayerLimit(), self:getPlayerLimit())

	self.spawnElapsed = self.spawnElapsed + dt
	local spawned = 0
	while self.spawnElapsed >= self.config.spawnInterval
		and spawned < self.config.maxSpawnsPerUpdate do
		self.spawnElapsed = self.spawnElapsed - self.config.spawnInterval
		local item = self:spawnItem()
		table.insert(events, { type = "spawned", item = item })
		spawned = spawned + 1
	end
	if spawned == self.config.maxSpawnsPerUpdate then
		self.spawnElapsed = math.min(self.spawnElapsed, self.config.spawnInterval)
	end

	local playerTop = self.config.playerY - self.config.playerHeight * 0.5
	local playerBottom = self.config.playerY + self.config.playerHeight * 0.5
	local playerHalfWidth = self.config.playerWidth * 0.5
	local itemHalfSize = self.config.itemSize * 0.5
	local bottom = self.config.height * 0.5 + itemHalfSize

	for index = #self.items, 1, -1 do
		local item = self.items[index]
		local previousY = item.y
		local fallSpeed = self.config.itemFallSpeed
			+ self.score * self.config.speedIncreasePerCatch
		item.y = item.y + fallSpeed * dt

		local horizontalHit = math.abs(item.x - self.playerX)
			<= playerHalfWidth + itemHalfSize
		local crossedPlayer = previousY - itemHalfSize <= playerBottom
			and item.y + itemHalfSize >= playerTop

		if horizontalHit and crossedPlayer then
			table.remove(self.items, index)
			self.score = self.score + 1
			table.insert(events, { type = "caught", item = item, score = self.score })
		elseif item.y - itemHalfSize > bottom then
			table.remove(self.items, index)
			self.misses = self.misses + 1
			table.insert(events, { type = "missed", item = item, misses = self.misses })
			if self.misses >= self.config.maxMisses then
				self.running = false
				table.insert(events, {
					type = "game_over",
					score = self.score,
					misses = self.misses,
				})
				break
			end
		end
	end

	return events
end

function CatchGameCore:snapshot()
	local result = {
		playerX = self.playerX,
		score = self.score,
		misses = self.misses,
		running = self.running,
		items = {},
	}
	for index, item in ipairs(self.items) do
		result.items[index] = { id = item.id, x = item.x, y = item.y }
	end
	return result
end
