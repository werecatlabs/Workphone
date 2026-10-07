-- Compatibility entry point: reuse the full game instead of redefining the vehicle sample.
if not RacingGameFull then include("RacingGameFull.lua") end
class 'RacingGameApplication' (RacingGameFull)
function RacingGameApplication:__init(component)
    RacingGameFull.__init(self, component)
end
