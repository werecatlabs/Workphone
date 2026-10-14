include("RacingGameFull.lua")

class 'RacingGameApplication' (RacingGameFull)

function RacingGameApplication:__init(component)
    RacingGameFull.__init(self, component)
end
