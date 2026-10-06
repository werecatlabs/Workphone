if not TargetFollower then include("TargetFollower.lua") end
class 'ReplayCamera' (TargetFollower)
function ReplayCamera:__init(component) TargetFollower.__init(self, component); self.offset = Vector3F(0,3,9) end
