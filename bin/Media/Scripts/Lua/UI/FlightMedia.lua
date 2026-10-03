include("FlightDialog.lua")

class 'FileBrowserDialog' (FlightDialog)

function FileBrowserDialog:__init(component)
	FlightDialog.__init(self, component)
	self.mode = "open"
	self.directory = ""
	self.extension = "*"
	self.isFolderMode = false
	self.entries = {}
	self.selectedPath = nil
	self.history = {}
end

function FileBrowserDialog:setDirectory(path)
	path = tostring(path or "")
	if path == "" then return false end
	self.directory = path
	table.insert(self.history, path)
	return self:refresh()
end
function FileBrowserDialog:setExtension(value) self.extension = tostring(value or "*"); return self:refresh() end
function FileBrowserDialog:setMode(value) self.mode = string.lower(tostring(value or "open")); return true end
function FileBrowserDialog:setFolderMode(value) self.isFolderMode = value == true end
function FileBrowserDialog:refresh()
	local provider = self.fileProvider
	local called, entries
	if type(provider) == "function" then called, entries = pcall(provider, self.directory, self.extension, self.isFolderMode)
	else called, entries = self:call(provider, { "getEntries", "GetEntries", "getFiles", "GetFiles", "list", "List" }, self.directory, self.extension, self.isFolderMode) end
	if called then self.entries = self:list(entries) else self.entries = {} end
	self:emit("entriesChanged", self.entries); return true
end
function FileBrowserDialog:gotoPath(path) return self:setDirectory(path) end
function FileBrowserDialog:gotoParent()
	local parent = self.directory:match("^(.*)[/\\][^/\\]+[/\\]?$")
	return parent and self:setDirectory(parent) or false
end
function FileBrowserDialog:select(path) self.selectedPath = tostring(path or ""); self:emit("selected", self.selectedPath); return self.selectedPath ~= "" end
function FileBrowserDialog:onSubmit(path) return self:select(path or self.selectedPath) end
function FileBrowserDialog:clickSubmit()
	if not self.selectedPath then self:setStatus("Select a file or folder", true); return false end
	self:emit("submitted", self.selectedPath, self.mode); self:hide(true); return true
end
function FileBrowserDialog:clickCancel() self.selectedPath = nil; self:emit("cancelled"); return self:hide(true) end
function FileBrowserDialog:clickClose() return self:clickCancel() end
function FileBrowserDialog:start() FlightDialog.start(self); if self.directory ~= "" then self:refresh() end; return true end

FileBrowserDialog.Update = FlightDialog.update
FileBrowserDialog.ClickClose = FileBrowserDialog.clickClose
FileBrowserDialog.ClickCancel = FileBrowserDialog.clickCancel
FileBrowserDialog.OnSubmit = FileBrowserDialog.onSubmit
FileBrowserDialog.ClickSubmit = FileBrowserDialog.clickSubmit
FileBrowserDialog.SetDirectory = FileBrowserDialog.setDirectory
FileBrowserDialog.SetExtension = FileBrowserDialog.setExtension
FileBrowserDialog.Start = FileBrowserDialog.start
FileBrowserDialog.OnDestroy = FlightDialog.onDestroy
FileBrowserDialog.Goto = FileBrowserDialog.gotoPath

class 'FlightRecorder' (FlightDialog)

function FlightRecorder:__init(component)
	FlightDialog.__init(self, component)
	self.timer = nil
	self.recordingsPanel = nil
	self.savePanel = nil
	self.files = {}
	self.currentFile = nil
	self.isRecording = false
	self.isPlaying = false
	self.isPaused = false
	self.playbackTime = 0.0
	self.totalTime = 0.0
	self.recordStartedAt = nil
end

function FlightRecorder:getRecorder()
	local called, recorder = self:call(self:getApplicationManager(), { "getFlightRecorder", "GetFlightRecorder" })
	return called and recorder or nil
end
function FlightRecorder:onOpen() self:setFlightFilesInfo(); return true end
function FlightRecorder:start() FlightDialog.start(self); return self:onOpen() end
function FlightRecorder:onClickRecord()
	if self.isRecording then return false end
	self.isRecording, self.isPlaying, self.isPaused = true, false, false
	self.recordStartedAt = self:getTime()
	self:call(self:getRecorder(), { "startRecording", "StartRecording", "record", "Record" })
	self:setStatus("Recording flight", false); self:emit("recordingStarted"); return true
end
function FlightRecorder:onClickPlay()
	if not self.currentFile then self:setStatus("Load a recording first", true); return false end
	self.isPlaying, self.isPaused, self.isRecording = true, false, false
	self:call(self:getRecorder(), { "play", "Play" }, self.currentFile, self.playbackTime)
	self:emit("playbackStarted", self.currentFile); return true
end
function FlightRecorder:onClickPause()
	if not self.isPlaying then return false end
	self.isPaused = not self.isPaused
	self:call(self:getRecorder(), { self.isPaused and "pause" or "resume", self.isPaused and "Pause" or "Resume" })
	return true
end
function FlightRecorder:onClickStop()
	self:call(self:getRecorder(), { "stop", "Stop", "stopRecording", "StopRecording" })
	if self.isRecording and self.recordStartedAt then self.totalTime = self:getTime() - self.recordStartedAt end
	self.isRecording, self.isPlaying, self.isPaused = false, false, false
	self:emit("stopped", self.totalTime); return true
end
function FlightRecorder:onClickSave(path)
	if self.isRecording then self:onClickStop() end
	local called = self:call(self:getRecorder(), { "save", "Save", "saveRecording", "SaveRecording" }, path)
	if called then self:setFlightFilesInfo() end
	return called
end
function FlightRecorder:clickLoadRecording(path) self.currentFile = path or self.currentFile; return self.currentFile ~= nil end
function FlightRecorder:clickFileDialogSubmit(path) return self:clickLoadRecording(path) end
function FlightRecorder:clickRecordingsPanelClose() return self:setControlVisible(self.recordingsPanel, false) end
function FlightRecorder:clickClose() self:onClickStop(); return FlightDialog.clickClose(self) end
function FlightRecorder:onTimerValueChanged(value) return self:setPlaybackTime(value) end
function FlightRecorder:onShuttleForward() return self:setPlaybackTime(self.playbackTime + 5.0) end
function FlightRecorder:onShuttleBackward() return self:setPlaybackTime(self.playbackTime - 5.0) end
function FlightRecorder:setNormalisedPlaybackTime(value) return self:setPlaybackTime(self:limit(value, 0, 1, 0) * self.totalTime) end
function FlightRecorder:setPlaybackTime(value) self.playbackTime = self:limit(value, 0, math.max(0, self.totalTime), 0); self:call(self:getRecorder(), { "setTime", "SetTime", "seek", "Seek" }, self.playbackTime); return self.playbackTime end
function FlightRecorder:setTotalTime(value) self.totalTime = math.max(0, tonumber(value) or 0); return self.totalTime end
function FlightRecorder:setFlightFilesInfo()
	local called, files = self:call(self:getRecorder(), { "getRecordings", "GetRecordings", "getFiles", "GetFiles" })
	if called then self.files = self:list(files) end
	self:emit("filesChanged", self.files); return self.files
end
function FlightRecorder:onFlightEnded() self.isPlaying = false; self.playbackTime = self.totalTime; self:emit("playbackEnded"); return true end
function FlightRecorder:update()
	FlightDialog.update(self)
	if self.isRecording and self.recordStartedAt then self:setTotalTime(self:getTime() - self.recordStartedAt) end
	if self.isPlaying and not self.isPaused then self.playbackTime = math.min(self.totalTime, self.playbackTime + 0.016) end
	self:setControlValue(self.timer, self.totalTime > 0 and self.playbackTime / self.totalTime or 0)
end

FlightRecorder.OnOpen = FlightRecorder.onOpen
FlightRecorder.Start = FlightRecorder.start
FlightRecorder.OnDestroy = FlightDialog.onDestroy
FlightRecorder.ClickFileDialogSubmit = FlightRecorder.clickFileDialogSubmit
FlightRecorder.Update = FlightRecorder.update
FlightRecorder.ClickLoadRecording = FlightRecorder.clickLoadRecording
FlightRecorder.ClickRecordingsPanelClose = FlightRecorder.clickRecordingsPanelClose
FlightRecorder.ClickClose = FlightRecorder.clickClose
FlightRecorder.OnClickPlay = FlightRecorder.onClickPlay
FlightRecorder.OnClickPause = FlightRecorder.onClickPause
FlightRecorder.OnClickStop = FlightRecorder.onClickStop
FlightRecorder.OnTimerValueChanged = FlightRecorder.onTimerValueChanged
FlightRecorder.OnClickRecord = FlightRecorder.onClickRecord
FlightRecorder.OnClickSave = FlightRecorder.onClickSave
FlightRecorder.OnShuttleForward = FlightRecorder.onShuttleForward
FlightRecorder.OnShuttleBackward = FlightRecorder.onShuttleBackward
FlightRecorder.SetFlightFilesInfo = FlightRecorder.setFlightFilesInfo
FlightRecorder.SetNormalisedPlaybackTime = FlightRecorder.setNormalisedPlaybackTime
FlightRecorder.SetPlaybackTime = FlightRecorder.setPlaybackTime
FlightRecorder.SetTotalTime = FlightRecorder.setTotalTime
FlightRecorder.OnFlightEnded = FlightRecorder.onFlightEnded

class 'Mp3PlayerDialog' (FlightDialog)

function Mp3PlayerDialog:__init(component)
	FlightDialog.__init(self, component)
	self.files = {}
	self.currentFile = nil
	self.isPlaying = false
	self.isPaused = false
	self.playbackTime = 0.0
	self.totalTime = 0.0
	self.volume = 1.0
	self.optionsPanel = nil
end
function Mp3PlayerDialog:getPlayer() local called, player = self:call(self:getApplicationManager(), { "getMp3Player", "GetMp3Player", "getAudioPlayer", "GetAudioPlayer" }); return called and player or nil end
function Mp3PlayerDialog:loadMp3(path) self.currentFile = path; local called = self:call(self:getPlayer(), { "load", "Load", "loadMp3", "LoadMp3" }, path); if called then self:emit("loaded", path) end; return called end
function Mp3PlayerDialog:start() FlightDialog.start(self); self:setMp3FilesInfo(); return true end
function Mp3PlayerDialog:onClickPlay() if not self.currentFile then return false end; self.isPlaying, self.isPaused = true, false; return self:call(self:getPlayer(), { "play", "Play" }) end
function Mp3PlayerDialog:onClickPause() if not self.isPlaying then return false end; self.isPaused = not self.isPaused; return self:call(self:getPlayer(), { self.isPaused and "pause" or "resume", self.isPaused and "Pause" or "Resume" }) end
function Mp3PlayerDialog:onClickStop() self.isPlaying, self.isPaused, self.playbackTime = false, false, 0; return self:call(self:getPlayer(), { "stop", "Stop" }) end
function Mp3PlayerDialog:clickLoadOptionsPanel() return self:setControlVisible(self.optionsPanel, true) end
function Mp3PlayerDialog:clickOptionsPanelClose() return self:setControlVisible(self.optionsPanel, false) end
function Mp3PlayerDialog:onShuttleForward() return self:setPlaybackTime(self.playbackTime + 5) end
function Mp3PlayerDialog:onShuttleBackward() return self:setPlaybackTime(self.playbackTime - 5) end
function Mp3PlayerDialog:setNormalisedPlaybackTime(value) return self:setPlaybackTime(self:limit(value, 0, 1, 0) * self.totalTime) end
function Mp3PlayerDialog:setPlaybackTime(value) self.playbackTime = self:limit(value, 0, math.max(0, self.totalTime), 0); self:call(self:getPlayer(), { "seek", "Seek", "setTime", "SetTime" }, self.playbackTime); return self.playbackTime end
function Mp3PlayerDialog:setTotalTime(value) self.totalTime = math.max(0, tonumber(value) or 0); return self.totalTime end
function Mp3PlayerDialog:setMp3FilesInfo() local called, files = self:call(self:getPlayer(), { "getFiles", "GetFiles", "getPlaylist", "GetPlaylist" }); if called then self.files = self:list(files) end; self:emit("filesChanged", self.files); return self.files end
function Mp3PlayerDialog:onMp3Ended() self.isPlaying = false; self.playbackTime = 0; self:emit("ended", self.currentFile); return true end
function Mp3PlayerDialog:onVolumeSlider(value) self.volume = self:limit(value, 0, 1, 1); self:call(self:getPlayer(), { "setVolume", "SetVolume" }, self.volume); return self.volume end
function Mp3PlayerDialog:update() FlightDialog.update(self); if self.isPlaying and not self.isPaused then self.playbackTime = math.min(self.totalTime, self.playbackTime + 0.016) end end

Mp3PlayerDialog.LoadMp3 = Mp3PlayerDialog.loadMp3
Mp3PlayerDialog.Start = Mp3PlayerDialog.start
Mp3PlayerDialog.Update = Mp3PlayerDialog.update
Mp3PlayerDialog.OnClickPlay = Mp3PlayerDialog.onClickPlay
Mp3PlayerDialog.OnClickPause = Mp3PlayerDialog.onClickPause
Mp3PlayerDialog.OnClickStop = Mp3PlayerDialog.onClickStop
Mp3PlayerDialog.ClickLoadOptionsPanel = Mp3PlayerDialog.clickLoadOptionsPanel
Mp3PlayerDialog.ClickOptionsPanelClose = Mp3PlayerDialog.clickOptionsPanelClose
Mp3PlayerDialog.OnShuttleForward = Mp3PlayerDialog.onShuttleForward
Mp3PlayerDialog.OnShuttleBackward = Mp3PlayerDialog.onShuttleBackward
Mp3PlayerDialog.SetMp3FilesInfo = Mp3PlayerDialog.setMp3FilesInfo
Mp3PlayerDialog.SetNormalisedPlaybackTime = Mp3PlayerDialog.setNormalisedPlaybackTime
Mp3PlayerDialog.SetPlaybackTime = Mp3PlayerDialog.setPlaybackTime
Mp3PlayerDialog.SetTotalTime = Mp3PlayerDialog.setTotalTime
Mp3PlayerDialog.OnMp3Ended = Mp3PlayerDialog.onMp3Ended
Mp3PlayerDialog.OnVolumeSlider = Mp3PlayerDialog.onVolumeSlider

class 'MessageBoxConfirm' (FlightDialog)

function MessageBoxConfirm:__init(component) FlightDialog.__init(self, component); self.messageText = ""; self.payload = nil end
function MessageBoxConfirm:setMessageText(value, payload) self.messageText = tostring(value or ""); self.payload = payload; self:setControlText("message", self.messageText); return true end
function MessageBoxConfirm:onComfirmButton() self:emit("confirmed", self.payload); self:hide(true); return true end
function MessageBoxConfirm:onCancelButton() self:emit("cancelled", self.payload); self:hide(true); return true end
MessageBoxConfirm.OnComfirmButton = MessageBoxConfirm.onComfirmButton
MessageBoxConfirm.OnConfirmButton = MessageBoxConfirm.onComfirmButton
MessageBoxConfirm.OnCancelButton = MessageBoxConfirm.onCancelButton
MessageBoxConfirm.SetMessageText = MessageBoxConfirm.setMessageText
