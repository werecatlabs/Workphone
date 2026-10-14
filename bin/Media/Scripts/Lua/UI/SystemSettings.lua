include("UIDialog.lua")
include("UICanvas.lua") 

class 'SystemSettings' (UIDialog)

function SystemSettings:__init(component)
	UIDialog.__init(self, component);
end

function SystemSettings:__finalize(component)
	UIDialog.__finalize(self);
end

function SystemSettings:generate()
	self:generate();
end