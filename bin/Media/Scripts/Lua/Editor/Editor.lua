class 'Editor'

function Editor:__init()
	self._editor = nil
	self._editorName = "Editor"
	self._editorVersion = "1.0.0"
	self._editorDescription = "A simple editor for managing game objects."
end

function Editor:__finalize()
	if self._editor then
		self._editor:__finalize()
		self._editor = nil
	end
end