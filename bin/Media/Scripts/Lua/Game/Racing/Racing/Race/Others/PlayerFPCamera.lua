if not TargetFollower then include("TargetFollower.lua") end
class 'PlayerFPCamera' (TargetFollower)
function PlayerFPCamera:__init(component) TargetFollower.__init(self, component); self.offset, self.smoothing = Vector3F(0,1,0), 0 end
