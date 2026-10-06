if not RacingRecords then include("RacingRecords.lua") end
class 'PlayerData' (BaseComponent)
function PlayerData:__init(component) BaseComponent.__init(self, component) end
function PlayerData:open(path)
    self.records = RacingRecords.new(path)
    self.records:load()
    return self.records
end
function PlayerData:save() return self.records:save() end
