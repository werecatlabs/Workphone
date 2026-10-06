-- Camera motion and mouse-wheel zoom remain in VehicleCameraController.
class 'PlayerCamera' (BaseComponent)
function PlayerCamera:__init(component) BaseComponent.__init(self, component) end
function PlayerCamera:bind(controller) self.controller = controller; self:setView(false) end
function PlayerCamera:setView(close)
    self.close = close == true
    self.controller:setDistance(self.close and 3.5 or 8)
    self.controller:setHeight(self.close and 1.5 or 3)
end
function PlayerCamera:toggle() self:setView(not self.close) end
