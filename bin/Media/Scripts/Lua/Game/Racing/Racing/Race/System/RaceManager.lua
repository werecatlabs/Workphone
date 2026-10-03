class 'RaceManager' (BaseComponent)

function RaceManager:__init()
	-- enum
	self.RaceType = { Circuit = 0, TimeTrial = 1, LapKnockout = 2, SpeedTrap = 3, Checkpoints = 4, Elimination = 5, Drift = 6 }
	self.RaceState = { Initialising = 0, StartingGrid = 1, Racing = 2, Paused = 3, Complete = 4, KnockedOut = 5, Replay = 6, None = 7 }
	self.PlayerSpawnPosition = { Randomized = 0, Selected = 1 }
	self.TimerType = { CountUp = 0, CountDown = 1 }
	self.AISpawnType = { Randomized = 0, Order = 1 }

	self._raceType = self.RaceType.Circuit
	self.m_RaceState = self.RaceState.Initialising

	self._playerSpawnPosition = self.PlayerSpawnPosition.Randomized
	self._aiSpawnType = self.AISpawnType.Randomized
	self.timerType = self.TimerType.CountUp

	-- int
	self.totalLaps = 3
	self.totalRacers = 1 --The total number of racers (player included)
	self.m_PlayerStartRank = 4 --The rank you will start the race as

	self.countdownFrom = 3
	self.currentCountdownTime = 0
	self.numOpponents = 0

	-- float
	self.raceDistance = 0.0 --Your race track's distance.
	self.countdownDelay = 3.0
	self.countdownTimer = 1.0
	self.initialCheckpointTime = 10.0 --start time (Checkpoint race);
	self.eliminationTime = 20.0 --start time (Checkpoint race);
	self.eliminationCounter = 0.0 --timer for elimination
	self.ghostAlpha = 0.3

	-- bool
	self.startCountdown = false
	self.continueAfterFinish = true --Should the racers keep driving after finish.
	self.showRacerNames = true --Should names appear above player cars
	self.showRacerPointers = true --Should minimap pointers appear above all racers
	self.showRaceInfoMessages = true --Show final lap indication , new best lap, speed trap & racer knockout information texts
	self.forceWrongwayRespawn = false --should the player get respawned if going the wrong way
	self.m_RaceStarted = false --has the race began
	self.raceCompleted = false --has the player car finished the race
	self.racePaused = false --is the game paused
	self.raceKO = false --**LapKnockout Mode Only** has the player car been knockedOut
	self.loadRacePreferences = false --Load menu prefrences?
	self.allowDuplicateRacers = false --allow duplicate AI
	self.enableGhostVehicle = true
	self.useGhostMaterial = false
	self.enableReplay = true
	self.viewingReplay = false
	self.countDownStarted = false
	self.initAtStart = false
	self.m_IsUserStarting = false
	self.m_IsUserEnding = false
	self.m_IsHost = false

	-- other
	self.m_NextPlayerDisableTime = 0.0
	self.m_FinishIndex = 0

	self.raceStartTime = -1.0
	self.raceEndTime = -1.0

	self.isSessionEnding = false
	self.m_IsRestarting = false

	-- lists (tables in lua)
	self.opponentControllerPrefabs = {}
	self.spawnpoints = {}
	self.raceRewards = {}
	self.opponentNamesList = {}
	self.eliminationList = {}
	self.aiModelControllers = {}
	self.m_RaceViews = {}
	self.m_ActiveRaceViews = {}

	-- unity/engine specific objects (will need adaptation)
	self.pathContainer = nil
	self.spawnpointContainer = nil
	self.checkpointContainer = nil
	self.startPoint = nil

	self.playerCar = nil
	self.playerPointer = nil
	self.opponentPointer = nil
	self.racerName = nil
	self.activeGhostCar = nil

	self.opponentNames = nil -- was TextAsset
	self.nameReader = nil -- was StringReader
	self.playerName = "You"
	self.ghostShader = nil
	self.ghostMaterial = nil
	
	self.m_RaceManagerView = nil
	self.m_RaceControllerPrefabPath = ""
end

function RaceManager:OnUpdate(dt)
	if self.m_RaceState == self.RaceState.StartingGrid then
		-- countdown logic
		if self.countDownStarted then
			self.countdownTimer = self.countdownTimer - dt
			if self.countdownTimer < 0 then
				self.currentCountdownTime = self.currentCountdownTime - 1
				self.countdownTimer = 1.0
				if self.currentCountdownTime <= 0 then
					self:StartRace()
				end
			end
		end
	elseif self.m_RaceState == self.RaceState.Racing then
		-- Race logic for different modes
		if self._raceType == self.RaceType.Elimination then
			self.eliminationCounter = self.eliminationCounter - dt
			if self.eliminationCounter <= 0 then
				if #self.m_ActiveRaceViews > 1 then
					local lastRacer = self:LastPlace()
					if lastRacer then
						self:KnockoutRacer(lastRacer)
						self:CalculateEliminationTime()
					end
				else
					-- All opponents are eliminated, can trigger race end
					if not self.raceCompleted then
						self:EndRace(1)
					end
				end
			end
		end

		-- Lap Knockout Logic
		if self._raceType == self.RaceType.LapKnockout then
			-- If only one racer remains, they are the winner.
			if #self.m_ActiveRaceViews <= 1 and not self.raceCompleted then
				self:EndRace(1)
			end
			
			-- Check for racers to knock out at the end of each lap.
			-- A racer object/table is expected to have `lap`, `lastCompletedLap`, and `isLast` fields.
			for _, racer in ipairs(self.m_ActiveRaceViews) do
				if not racer.raceCompleted and racer.lap > racer.lastCompletedLap then
					racer.lastCompletedLap = racer.lap -- Mark lap as processed.

					if racer.isLast then
						local isEliminated = false
						for _, eliminatedRacer in ipairs(self.eliminationList) do
							if racer == eliminatedRacer then
								isEliminated = true
								break
							end
						end

						if not isEliminated then
							table.insert(self.eliminationList, racer)
							self:KnockoutRacer(racer)
						end
					end
				end
			end
		end

		if self.raceCompleted and not self:AllRacersFinished() and self.continueAfterFinish then
			-- Racers continue driving after finishing
		elseif self.raceCompleted and self:AllRacersFinished() and not self.continueAfterFinish then
			-- End scenario when all racers stop at finish
			print("All racers have finished the race.")
			-- TODO: Disable all cars, show final results
			self:SetRaceState(self.RaceState.Complete)
		end
	elseif self.m_RaceState == self.RaceState.Complete then
		-- logic for when race is complete
	elseif self.m_RaceState == self.RaceState.KnockedOut then
		-- logic for when player is knocked out
	elseif self.m_RaceState == self.RaceState.Replay then
		-- logic for replay
	end
end

function RaceManager:SetRaceState(newState)
	if self.m_RaceState ~= newState then
		self:OnLeaveRaceState()
		self.m_RaceState = newState
		self:OnEnterRaceState()
	end
end

function RaceManager:OnEnterRaceState()
	-- logic to execute when entering a new state
end

function RaceManager:OnLeaveRaceState()
	-- logic to execute when leaving a state
end

function RaceManager:InitializeRace()
	if self.m_RaceState == self.RaceState.Initialising then
		return
	end

	self:SetRaceState(self.RaceState.Initialising)
	self.m_FinishIndex = 0

	self:LoadRacerNames()
	self:ConfigureNodes()
	self:SetupSpawnPoints()
	self:SpawnRacers()

	-- After initialization, move to starting grid
	self:SetRaceState(self.RaceState.StartingGrid)
	self:StartCountdown()
end

function RaceManager:StartCountdown()
	self.countDownStarted = true
	self.currentCountdownTime = self.countdownFrom
	self.countdownTimer = 1.0
end

function RaceManager:LoadRacerNames()
	-- TODO: Implement logic to load racer names
	self.opponentNamesList = { "CPU 1", "CPU 2", "CPU 3" }
end

function RaceManager:ConfigureNodes()
	-- TODO: Implement logic to configure race path nodes
end

function RaceManager:SetupSpawnPoints()
	-- TODO: Implement logic to set up spawn points
end

function RaceManager:SpawnRacers()
	-- TODO: Implement logic to spawn player and AI racers
end

function RaceManager:StartRace()
	self:SetRaceState(self.RaceState.Racing)
	-- In a real scenario, you'd get the time from the engine
	self.raceStartTime = 0 -- Placeholder for Time.time

	if self._raceType == self.RaceType.Elimination then
		self:CalculateEliminationTime()
	end

	-- TODO: Add logic to start timers for all racers
	print("Race Started!")
end

function RaceManager:EndRace(rank)
	self.raceCompleted = true
	self:SetRaceState(self.RaceState.Complete)
	print("Race Finished! Rank: " .. tostring(rank))

	-- TODO: Add logic for what happens when the race ends
end

function RaceManager:CalculateEliminationTime()
	if self.totalRacers > 1 then
		self.eliminationCounter = self.eliminationTime / (self.totalRacers - 1)
	else
		self.eliminationCounter = self.eliminationTime
	end
end

function RaceManager:KnockoutRacer(racer)
	-- A racer object/table is expected to have fields like:
	-- isPlayer (boolean), gameObject (engine object), name (string)
	
	print("Knocking out racer: " .. racer.name)

	if racer.isPlayer then
		self.raceKO = true
		self:SetRaceState(self.RaceState.KnockedOut)
		print("Player has been knocked out!")
	else
		-- TODO: Implement logic to disable an AI racer
		-- For example: racer.gameObject:Disable()
		
		-- Remove from active racers
		for i, activeRacer in ipairs(self.m_ActiveRaceViews) do
			if activeRacer == racer then
				table.remove(self.m_ActiveRaceViews, i)
				break
			end
		end
	end
	
	-- TODO: Show message that a racer has been knocked out
end

function RaceManager:LastPlace()
	local lastRacer = nil
	local worstRank = -1

	-- A racer object/table is expected to have a 'rank' field
	for i, racer in ipairs(self.m_ActiveRaceViews) do
		if racer.rank > worstRank then
			worstRank = racer.rank
			lastRacer = racer
		end
	end
	return lastRacer
end

function RaceManager:AllRacersFinished()
	-- A racer object/table is expected to have a 'raceCompleted' boolean field
	for i, racer in ipairs(self.m_ActiveRaceViews) do
		if not racer.raceCompleted then
			return false
		end
	end
	return true
end

function RaceManager:FormatTime(time)
	local minutes = math.floor(time / 60)
	local seconds = math.floor(time % 60)
	local milliseconds = math.floor((time * 100) % 100)
	return string.format("%02d:%02d:%02d", minutes, seconds, milliseconds)
end

function RaceManager:RespawnRacer(racer, node, ignoreCollisionTime)
	-- racer and node are expected to be engine-specific objects
	-- racer should have position and rotation properties
	-- node should have position and rotation properties

	print("Respawning racer: " .. racer.name)

	self:DisableRacerComponents(racer)
	self:ChangeLayer(racer, "IgnoreCollision")

	-- Move the racer to the respawn node's position and rotation
	racer.position = node.position
	racer.rotation = node.rotation
	
	-- TODO: Implement a delayed call here using your engine's timer/coroutine system.
	-- After 'ignoreCollisionTime' seconds, you should re-enable the racer.
	-- For example:
	-- Timer.Create(ignoreCollisionTime, function()
	--    self:ChangeLayer(racer, "Player")
	--    -- Re-enable any other components if needed
	-- end)
	
	print("Racer respawned. Collision will be ignored for " .. tostring(ignoreCollisionTime) .. " seconds.")
end

function RaceManager:DisableRacerComponents(racer)
	-- TODO: Implement logic to disable physics components like rigidbodies and colliders.
	-- This prevents strange physics behavior during the respawn.
	-- For example: racer.rigidbody:SetEnabled(false)
	print("Disabling components for racer: " .. racer.name)
end

function RaceManager:ChangeLayer(racer, layerName)
	-- TODO: Implement logic to change the physics layer of the racer's game object.
	-- This is used to prevent collisions while respawning.
	-- For example: racer.gameObject:SetLayer(layerName)
	print("Changing layer for " .. racer.name .. " to " .. layerName)
end