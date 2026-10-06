if not RacingSupport then include("RacingSupport.lua") end
class 'CameraSwitcher' (RacingComponent)
function CameraSwitcher:__init(component) BaseComponent.__init(self, component); self.cameras, self.index = {}, 0 end
function CameraSwitcher:bind(cameras)
    assert(#cameras > 0, "At least one camera required"); self.cameras = cameras; self:select(1)
end
function CameraSwitcher:select(index)
    assert(index == math.floor(index) and self.cameras[index], "Unknown camera")
    for i, camera in ipairs(self.cameras) do camera:setActive(i == index) end
    self.index = index
end
function CameraSwitcher:next() self:select(self.index % #self.cameras+1) end
function CameraSwitcher:shutdown() self.cameras, self.index = {}, 0 end
