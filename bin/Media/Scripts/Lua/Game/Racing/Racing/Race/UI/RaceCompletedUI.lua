class 'RaceCompletedUI' (BaseComponent)

function RaceCompletedUI:__init()
	BaseComponent.__init(self)

	self.content = nil
	self.resultPrefab = nil
	self.nextUpdateTime = 0.0
	self.hostButtons = nil
	self.clientButtons = nil
end

function RaceCompletedUI:Update()
	self:UpdateResults()
end

function RaceCompletedUI:UpdateResults()
	if self.nextUpdateTime < Saracen.Time.time then
		self:PopulateGUI()
		self.nextUpdateTime = Saracen.Time.time + 0.1
	end

	if MultiplayerConnector.instance and MultiplayerConnector.instance.inRoom then
		if RaceManager.instance.isHost then
			self.hostButtons:SetActive(true)
			self.clientButtons:SetActive(false)
		else
			self.hostButtons:SetActive(false)
			self.clientButtons:SetActive(true)
		end
	else
		self.hostButtons:SetActive(true)
		self.clientButtons:SetActive(false)
	end
end

function RaceCompletedUI:PopulateGUI()
	if not self.content then
		return
	end

	if self.resultPrefab then
		local itemCount = 0
		self.content.gameObject:DestroyAllChildren()

		if not RankManager.instance then
			return
		end
		
		for i = 1, RankManager.instance.currentRacers do
			local ranker = RankManager.instance.racerRanks[i]
			if ranker then
				local statistics = ranker.racerStats
				if statistics then
					local success, go = pcall(GameObject.Instantiate, self.resultPrefab)
					if success and go then
						local item = go:GetComponent("RaceStandingsUI")
						if item then
							if statistics.finishRank ~= -1 then
								item.position:SetText(tostring(statistics.finishRank))
							else
								item.position:SetText(tostring(statistics.rank))
							end

							item.position:SetText(tostring(itemCount + 1))

							local racerObject = ranker.racer
							if racerObject then
								local modelController = racerObject:GetComponent("ModelController")
								local raceName = racerObject:GetComponent("RacerName")
								local model = racerObject:GetComponentInChildren("Model")
								
								local playerName = ApplicationManager.instance.playerName
								if MultiplayerConnector.instance and MultiplayerConnector.instance.inRoom then
									if modelController and modelController.playerName ~= "" then
										playerName = modelController.playerName
									end
								end

								if modelController and not modelController.isAiPlayer then
									item.racerName:SetText(playerName)
								elseif raceName and raceName.playerName ~= "" then
									item.racerName:SetText(raceName.playerName)
								elseif model and model.referenceModelSetupData.modelName ~= "" then
									item.racerName:SetText(model.referenceModelSetupData.modelName)
								else
									item.racerName:SetText(racerObject.name)
								end
							end

							item.totalTime:SetText(statistics.totalRaceTime)
							item.bestLapTime:SetText(statistics.bestLapTime == "" and "--:--:--" or statistics.bestLapTime)
						end

						local rectTransform = go:GetComponent("RectTransform")
						if rectTransform then
							local pos = rectTransform.anchoredPosition
							local size = rectTransform.sizeDelta
							
							pos.x = 0.0
							pos.y = itemCount * -size.y
							rectTransform.anchoredPosition = pos
						end

						go.transform:SetParent(self.content.transform, false)
						itemCount = itemCount + 1
					end
				end
			end
		end
	end
end