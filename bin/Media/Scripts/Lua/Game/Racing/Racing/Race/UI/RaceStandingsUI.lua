class 'RaceStandingsUI' (BaseComponent)

function RaceStandingsUI:__init()
	BaseComponent.__init(self)
	
	self.position = nil
    self.racerName = nil
    self.vehicleName = nil
    self.bestLapTime = nil
    self.totalTime = nil
end

function RaceStandingsUI:initialise()
	-- Corresponds to Start() in C#
end

function RaceStandingsUI:update()
	-- Corresponds to Update() in C#
end