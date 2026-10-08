if not RacingSupport then include("RacingSupport.lua") end
-- Camera motion and mouse-wheel zoom remain in VehicleCameraController.
class 'PlayerCamera' (RacingComponent)
function PlayerCamera:__init(component) BaseComponent.__init(self, component) end
function PlayerCamera:bind(controller) self.controller = assert(controller,"Native camera controller required"); self:setView(false) end
function PlayerCamera:setView(close)
    assert(self.controller,"Player camera is unbound")
    self.close = close == true
    self.controller:setDistance(self.close and 3.5 or 8)
    self.controller:setHeight(self.close and 1.5 or 3)
end
function PlayerCamera:toggle() self:setView(not self.close) end
-- Editor Play selects game cameras before any procedural race has been generated.
-- Keep a renderable viewport for menus, then select the race camera when ready.
function PlayerCamera:selectViewport(editorPreview)
    local application = assert(IApplicationManager.instance())
    if application:isEditor() then
        application:setEditorCamera(editorPreview == true)
        application:getCameraManager():reset()
    end
end
function PlayerCamera:shutdown() self.controller=nil end
