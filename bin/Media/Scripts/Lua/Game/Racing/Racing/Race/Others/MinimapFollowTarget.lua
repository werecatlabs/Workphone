if not TargetFollower then include("TargetFollower.lua") end
class 'MinimapFollowTarget' (TargetFollower)
function MinimapFollowTarget:__init(component)
    TargetFollower.__init(self, component)
    self.offset, self.localOffset, self.smoothing = Vector3F(0,40,0), false, 0
    self.fixedRotation = Quaternion(math.sqrt(0.5), -math.sqrt(0.5), 0, 0)
end
