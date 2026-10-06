if not RacingSupport then include("RacingSupport.lua") end
class 'FramerateCounter' (RacingComponent)
function FramerateCounter:__init(component) BaseComponent.__init(self, component); self.frames, self.elapsed, self.fps = 0, 0, 0 end
function FramerateCounter:bind(label) self.label = label end
function FramerateCounter:update(dt)
    dt = RacingSupport.delta(self, dt)
    if dt <= 0 then return end
    self.frames, self.elapsed = self.frames+1, self.elapsed+dt
    if self.elapsed >= 0.5 then
        self.fps = self.frames/self.elapsed
        RacingSupport.label(self.label, string.format("%.0f FPS  %.1f ms",self.fps,1000/self.fps))
        self.frames, self.elapsed = 0, 0
    end
end
