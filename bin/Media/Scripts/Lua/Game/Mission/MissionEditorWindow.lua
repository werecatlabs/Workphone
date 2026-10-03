include("MissionAuthoring.lua")

class 'MissionEditorWindow' (MissionAuthoring)

function MissionEditorWindow:__init()
	MissionAuthoring.__init(self)
	self.isOpen = false
end

function MissionEditorWindow:open()
	self.isOpen = true
	return self
end

function MissionEditorWindow:close()
	self.isOpen = false
	return true
end

MissionEditorWindow.Open = function()
	local window = MissionEditorWindow()
	return window:open()
end
MissionEditorWindow.Close = MissionEditorWindow.close
