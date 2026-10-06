if not RacingSupport then include("RacingSupport.lua") end
class 'MobileControlManager' (RacingComponent)
function MobileControlManager:__init(component) BaseComponent.__init(self,component); self.touches,self.enabled={},false end
function MobileControlManager:bind(car) self.car=assert(car); self:enable(false) end
function MobileControlManager:enable(value)
    self.enabled=value==true; self.touches={}
    if self.car then if self.enabled then self.car:setControls(0,0,0) else self.car:usePlayerControls() end end
end
function MobileControlManager:setTouch(id,control,value)
    assert(control=="throttle" or control=="brake" or control=="steering","Unknown touch control")
    assert(type(id)=="number" or type(id)=="string","Touch ID required")
    value=RacingSupport.number(value,control=="steering" and -1 or 0,1,"touch value")
    local count=0; for _ in pairs(self.touches) do count=count+1 end
    assert(self.touches[id] or count<16,"Too many touches")
    if self.enabled then self.touches[id]={control=control,value=value}; self:apply() end
end
function MobileControlManager:release(id) self.touches[id]=nil; if self.enabled then self:apply() end end
function MobileControlManager:apply()
    if not self.car or not self.enabled then return end
    local throttle,brake,steering=0,0,0
    for _,touch in pairs(self.touches) do
        if touch.control=="throttle" then throttle=math.max(throttle,touch.value)
        elseif touch.control=="brake" then brake=math.max(brake,touch.value)
        else steering=steering+touch.value end
    end
    self.car:setControls(throttle,brake,RacingSupport.clamp(steering,-1,1))
end
function MobileControlManager:cancel() self.touches={}; if self.enabled and self.car then self.car:setControls(0,1,0) end end
function MobileControlManager:shutdown() self:cancel(); self.enabled,self.car=false,nil end
