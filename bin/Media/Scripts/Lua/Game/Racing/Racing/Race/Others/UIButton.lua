if not RacingSupport then include("RacingSupport.lua") end
class 'UIButton' (RacingComponent)
function UIButton:__init(component) BaseComponent.__init(self, component); self.enabled, self.cooldown, self.nextActivation = true, 0.2, 0 end
function UIButton:bind(button, callback)
    assert(button and type(callback) == "function", "Button and callback required")
    self.button, self.callback = button, callback
end
function UIButton:activate()
    local now = RacingSupport.now()
    if not self.enabled or not self.callback or now < self.nextActivation then return false end
    self.nextActivation = now+self.cooldown; self.pending = true; return true
end
function UIButton:update()
    if self.pending then self.pending = false; if self.enabled then self.callback() end end
end
function UIButton:setEnabled(value) self.enabled = value == true; if self.button then self.button:setEnabled(self.enabled) end end
function UIButton:shutdown() self.pending, self.button, self.callback = false, nil, nil end
