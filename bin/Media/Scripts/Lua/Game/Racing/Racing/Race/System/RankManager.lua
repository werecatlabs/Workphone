class 'RankManager' (BaseComponent)

function RankManager:new(go)
	super(go)
end

function RankManager:OnCreate()
	RankManager.instance = self

	self.racerRanks = {} -- List<Ranker>
	self.racerStats = {} -- List<ProgressTracker>
	self.totalRacers = 0
	self.currentRacers = 0
	self.m_NextUpdateTime = 0.0
end

function RankManager:OnDestroy()
	RankManager.instance = nil
end

local function table_contains(tbl, val)
	for _, value in ipairs(tbl) do
		if value == val then
			return true
		end
	end
	return false
end

function RankManager:RefreshRacerCount()
	self.racerStats = {}

	local racerList = {}

	if MultiplayerConnector and MultiplayerConnector.inRoom then
		local modelControllers = Scene:FindObjectsOfType("ModelController")
		for _, modelController in ipairs(modelControllers) do
			local stats = modelController:GetComponent("Statistics")
			if stats then
				table.insert(racerList, stats)
			end
		end
	else
		if RaceManager.instance and RaceManager.instance.aiModelControllers then
			local opponentControllers = RaceManager.instance.aiModelControllers
			for _, ctrl in ipairs(opponentControllers) do
				if ctrl then
					local stats = ctrl:GetComponent("Statistics")
					if stats then
						table.insert(racerList, stats)
					end
				end
			end
		end

		if ApplicationManager.instance and ApplicationManager.instance.modelController then
			local modelController = ApplicationManager.instance.modelController
			if modelController then
				local stats = modelController:GetComponent("Statistics")
				if stats then
					table.insert(racerList, stats)
				end
			end
		end
	end

	self.totalRacers = #racerList

	for i = 1, #racerList do
		if not racerList[i].knockedOut then
			local progressTracker = racerList[i].gameObject:GetComponent("ProgressTracker")
			if progressTracker and not table_contains(self.racerStats, progressTracker) then
				table.insert(self.racerStats, progressTracker)
			end
		end
	end

	self.currentRacers = #self.racerStats
end

local function compareRankers(a, b)
	if a.racerStats and b.racerStats then
		if (a.racerStats.finishedRace and b.racerStats.finishedRace) or
		   (a.racerStats.finishedRace and not b.racerStats.finishedRace) or
		   (not a.racerStats.finishedRace and b.racerStats.finishedRace) then
			return a.racerStats.totalTimeCounter < b.racerStats.totalTimeCounter
		end
	end

	local v0 = 100.0 - a.raceCompletion
	local v1 = 100.0 - b.raceCompletion

	return v0 < v1
end

function RankManager:Update(dt)
	if self.m_NextUpdateTime < Time.real.timeSinceStartup then
		self.racerRanks = {}

		for i = 1, self.currentRacers do
			local progressTracker = self.racerStats[i]
			if progressTracker then
				local stats = progressTracker.gameObject:GetComponent("Statistics")
				if stats then
					local ranker = {
						racer = progressTracker.gameObject,
						racerStats = stats,
						raceCompletion = progressTracker.raceCompletion - (stats.rank / 10000.0)
					}
					table.insert(self.racerRanks, ranker)
				end
			end
		end

		table.sort(self.racerRanks, compareRankers)

		self:SetCarRank()

		self.m_NextUpdateTime = Time.real.timeSinceStartup + 0.1
	end
end

function RankManager:SetCarRank()
	if MultiplayerConnector and MultiplayerConnector.inRoom then
		if not MultiplayerConnector.isHost then
			return
		end
	end

	local raceManager = RaceManager.instance
	if raceManager and raceManager.raceStarted and not raceManager.raceCompleted then
		for r = 1, self.currentRacers do
			local ranker = self.racerRanks[r]
			if ranker and ranker.racer then
				local statistics = ranker.racer:GetComponent("Statistics")
				if statistics and not statistics.finishedRace then
					statistics:SetRank(r)
				end
			end
		end
	end
end