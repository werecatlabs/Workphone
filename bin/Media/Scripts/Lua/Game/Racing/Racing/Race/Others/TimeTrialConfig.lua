class 'TimeTrialConfig' (BaseComponent)
function TimeTrialConfig:__init(component) BaseComponent.__init(self, component); self.mode = "timeTrial" end
function TimeTrialConfig:apply(manager) manager:InitializeRace(self.mode, 1) end
