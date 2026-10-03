class 'GeneratedGameManager' (BaseComponent)

function GeneratedGameManager:__init(component)
	BaseComponent.__init(self, component)
	self.genre = GameTemplateGenre and GameTemplateGenre.Custom or "Custom"
	self.player = nil
	self.playerSpawnPoint = nil
	self.hasPositionedPlayer = false
end

function GeneratedGameManager:__finalize()
	BaseComponent.__finalize(self)
end

function GeneratedGameManager:Initialize(templateGenre, templatePlayer, spawnPoint)
	self.genre = templateGenre or self.genre
	self.player = templatePlayer
	self.playerSpawnPoint = spawnPoint
	self:PositionPlayerAtSpawn()
end

function GeneratedGameManager:GetGenre()
	return self.genre
end

function GeneratedGameManager:GetPlayer()
	return self.player
end

function GeneratedGameManager:update()
	if not self.hasPositionedPlayer then
		self:PositionPlayerAtSpawn()
	end
end

function GeneratedGameManager:PositionPlayerAtSpawn()
	if not self.player or not self.playerSpawnPoint then
		return
	end

	local playerTransform = self.player.transform or self.player
	local spawnTransform = self.playerSpawnPoint.transform or self.playerSpawnPoint

	if not playerTransform or not spawnTransform then
		return
	end

	if spawnTransform.position then
		playerTransform.position = spawnTransform.position
	end

	if spawnTransform.rotation then
		playerTransform.rotation = spawnTransform.rotation
	end

	self.hasPositionedPlayer = true
end

