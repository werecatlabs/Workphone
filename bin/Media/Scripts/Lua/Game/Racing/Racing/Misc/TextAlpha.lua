if not RacingSupport then include("RacingSupport.lua") end
class 'TextAlpha' (RacingComponent)
function TextAlpha:__init(component) BaseComponent.__init(self, component); self.alpha, self.target, self.speed = 1, 1, 2 end
function TextAlpha:bind(label, red, green, blue)
    self.label = assert(label)
    self.red,self.green,self.blue=RacingSupport.number(red or 1,0,1),RacingSupport.number(green or 1,0,1),RacingSupport.number(blue or 1,0,1)
end
function TextAlpha:fade(alpha, duration)
    self.target = RacingSupport.number(alpha,0,1,"alpha")
    RacingSupport.number(duration,0,3600,"fade duration")
    self.speed = duration > 0 and math.abs(self.target-self.alpha)/duration or math.huge
    if duration == 0 then self.alpha=self.target end
end
function TextAlpha:update(dt)
    dt = RacingSupport.delta(self,dt)
    local step = self.speed*dt
    if self.speed == math.huge then self.alpha=self.target
    elseif self.alpha < self.target then self.alpha=math.min(self.target,self.alpha+step)
    else self.alpha=math.max(self.target,self.alpha-step) end
    if self.label and self.lastAlpha ~= self.alpha then
        self.label:setColour(ColourF(self.red,self.green,self.blue,self.alpha)); self.lastAlpha=self.alpha
    end
end
