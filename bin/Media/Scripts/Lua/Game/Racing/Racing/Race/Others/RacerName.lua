if not RacingSupport then include("RacingSupport.lua") end
class 'RacerName' (RacingComponent)
function RacerName:__init(component) BaseComponent.__init(self, component) end
function RacerName:bind(label, view) self.label, self.view = assert(label), assert(view) end
function RacerName:update()
    if not self.label or not self.view then return end
    local name = tostring(self.view.name or ("Racer " .. self.view.id)):gsub("[%c]", " "):sub(1,32)
    RacingSupport.label(self.label, tostring(self.view.m_Rank) .. "  " .. name)
end
function RacerName:shutdown() self.label,self.view=nil,nil end
