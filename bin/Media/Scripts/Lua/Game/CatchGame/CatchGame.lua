include("CatchGameCore.lua")

class 'CatchGame' (BaseComponent)

function CatchGame:__init(component)
	BaseComponent.__init(self, component)
	self.game = CatchGameCore.new()
	self.itemActors = {}
	self.started = false
	self.restartWasDown = false
end

function CatchGame:__finalize()
	self:shutdown()
	BaseComponent.__finalize(self)
end

function CatchGame:createActor(parent, name)
	local actor = self.gameManager:createActor()
	actor:setName(name)
	parent:addChild(actor)
	return actor
end

function CatchGame:addLayout(actor, x, y, width, height, zOrder)
	local layout = actor:addComponent("LayoutTransform")
	if layout then
		layout:setPosition(Vector2F(x, y))
		layout:setSize(Vector2F(width, height))
		layout:setZOrder(zOrder or 0)
	end
	return layout
end

function CatchGame:addMaterial(actor)
	local material = actor:addComponent("Material")
	if material then
		material:setMaterialPath("DefaultUI.mat")
	end
end

function CatchGame:createPanel(parent, name, x, y, width, height, colour, zOrder)
	local actor = self:createActor(parent, name)
	self:addLayout(actor, x, y, width, height, zOrder)
	local image = actor:addComponent("Image")
	if image then
		image:setColour(colour)
	end
	self:addMaterial(actor)
	return actor
end

function CatchGame:createText(parent, name, value, x, y, width, height, colour)
	local actor = self:createActor(parent, name)
	self:addLayout(actor, x, y, width, height, 5)
	local text = actor:addComponent("Text")
	if text then
		text:setText(value)
		text:setColour(colour)
		text:setHorizontalAlignment(1)
		text:setVerticalAlignment(1)
	end
	return actor, text
end

function CatchGame:start()
	if self.started then
		return true
	end

	local application = IApplicationManager.instance()
	self.gameManager = application and application:getGameManager()
	self.input = application and application:getInput()
	self.inputDevices = application and application:getInputDeviceManager()
	self.timer = application and application:getTimer()
	local owner = self:getActor()
	if not self.gameManager or not owner then
		print("CatchGame: game manager or owner actor is unavailable")
		return false
	end

	self.uiRoot = self:createActor(owner, "CatchGame.UI")
	local config = self.game.config
	local white = ColourF(0.94, 0.98, 1.0, 1.0)
	self:createPanel(self.uiRoot, "Background", 0.0, 0.0, config.width, config.height,
		ColourF(0.025, 0.055, 0.10, 0.98), 0)
	self:createText(self.uiRoot, "Title", "CATCH!", 0.0, -config.height * 0.5 + 35.0,
		300.0, 50.0, white)
	self.hudActor, self.hudText = self:createText(self.uiRoot, "HUD", "", 0.0,
		-config.height * 0.5 + 78.0, 520.0, 42.0, white)
	self.messageActor, self.messageText = self:createText(self.uiRoot, "Message",
		"Move with A/D or Left/Right", 0.0, -20.0, 620.0, 60.0,
		ColourF(0.72, 0.84, 0.95, 1.0))
	self.playerActor = self:createPanel(self.uiRoot, "Player", self.game.playerX,
		config.playerY, config.playerWidth, config.playerHeight,
		ColourF(0.15, 0.78, 0.95, 1.0), 3)
	self.playerLayout = self.playerActor:getComponent("LayoutTransform")
	self.started = true
	self:updateHud()
	return true
end

function CatchGame:shutdown()
	if self.gameManager and self.uiRoot then
		self.gameManager:destroyActor(self.uiRoot)
	end
	self.uiRoot = nil
	self.playerActor = nil
	self.playerLayout = nil
	self.hudText = nil
	self.messageText = nil
	self.itemActors = {}
	self.started = false
end

function CatchGame:isKeyDown(code)
	return self.inputDevices and code ~= nil and self.inputDevices:isKeyPressed(code)
end

function CatchGame:readInput()
	local axis = self.input and self.input:getAxisValue(0) or 0.0
	local left = self:isKeyDown(KeyCode and KeyCode.Left)
		or self:isKeyDown(KeyCode and KeyCode.A)
	local right = self:isKeyDown(KeyCode and KeyCode.Right)
		or self:isKeyDown(KeyCode and KeyCode.D)
	if left ~= right then
		axis = left and -1.0 or 1.0
	end
	return math.max(-1.0, math.min(1.0, tonumber(axis) or 0.0))
end

function CatchGame:isRestartDown()
	return self:isKeyDown(KeyCode and KeyCode.R)
		or self:isKeyDown(KeyCode and KeyCode.Return)
end

function CatchGame:createItemVisual(item)
	local actor = self:createPanel(self.uiRoot, "Item." .. tostring(item.id), item.x, item.y,
		self.game.config.itemSize, self.game.config.itemSize,
		ColourF(1.0, 0.72, 0.18, 1.0), 2)
	self.itemActors[item.id] = {
		actor = actor,
		layout = actor:getComponent("LayoutTransform"),
	}
end

function CatchGame:destroyItemVisual(itemId)
	local visual = self.itemActors[itemId]
	if visual then
		self.gameManager:destroyActor(visual.actor)
		self.itemActors[itemId] = nil
	end
end

function CatchGame:updateHud()
	if self.hudText then
		self.hudText:setText("Score: " .. tostring(self.game.score)
			.. "    Misses: " .. tostring(self.game.misses)
			.. "/" .. tostring(self.game.config.maxMisses))
	end
end

function CatchGame:reset()
	for itemId, _ in pairs(self.itemActors) do
		self:destroyItemVisual(itemId)
	end
	self.game:reset()
	if self.messageText then
		self.messageText:setText("Move with A/D or Left/Right")
	end
	self:updateHud()
end

function CatchGame:update()
	if not self.started and not self:start() then
		return
	end

	local restartDown = self:isRestartDown()
	if restartDown and not self.restartWasDown then
		self:reset()
	end
	self.restartWasDown = restartDown

	local deltaTime = self.timer and self.timer:getDeltaTime() or (1.0 / 60.0)
	local events = self.game:update(deltaTime, self:readInput())
	for _, event in ipairs(events) do
		if event.type == "spawned" then
			self:createItemVisual(event.item)
		elseif event.type == "caught" or event.type == "missed" then
			self:destroyItemVisual(event.item.id)
		elseif event.type == "game_over" and self.messageText then
			self.messageText:setText("Game over - press R or Enter to restart")
		end
	end

	if self.playerLayout then
		self.playerLayout:setPosition(Vector2F(self.game.playerX, self.game.config.playerY))
	end
	for _, item in ipairs(self.game.items) do
		local visual = self.itemActors[item.id]
		if visual and visual.layout then
			visual.layout:setPosition(Vector2F(item.x, item.y))
		end
	end
	if #events > 0 then
		self:updateHud()
	end
end

-- Convenience entry point for the Lua console or a bootstrap script:
--     include("CatchGame.lua")
--     launchCatchGame()
function launchCatchGame()
	local application = IApplicationManager.instance()
	local gameManager = application and application:getGameManager()
	if not gameManager then
		print("launchCatchGame: game manager is unavailable")
		return nil
	end

	local previous = gameManager:getActorByName("CatchGame")
	if previous then
		gameManager:destroyActor(previous)
	end

	local actor = gameManager:createActor()
	actor:setName("CatchGame")
	local scene = gameManager:getCurrentScene()
	if scene then
		scene:addActor(actor)
	end
	local component = actor:addComponentById(UserComponent.typeInfo())
	if not component then
		gameManager:destroyActor(actor)
		print("launchCatchGame: failed to create UserComponent")
		return nil
	end
	component:setClassName("CatchGame")
	return actor
end
