include("FlightDialog.lua")

class 'RaceSettings' (FlightDialog)

function RaceSettings:__init(component)
	FlightDialog.__init(self, component)
	self.laps = 3
	self.opponents = 3
	self.raceType = 1
	self.aiDifficulty = 1
	self.maximumLaps = 999
	self.maximumOpponents = 64
end

function RaceSettings:addLaps(value) self.laps = math.floor(self:limit(self.laps + (tonumber(value) or 1), 1, self.maximumLaps, 3)); self:setControlText("laps", self.laps); return self.laps end
function RaceSettings:addOpponent(value) self.opponents = math.floor(self:limit(self.opponents + (tonumber(value) or 1), 0, self.maximumOpponents, 3)); self:setControlText("opponents", self.opponents); return self.opponents end
function RaceSettings:addRaceType(value) self.raceType = math.max(0, self.raceType + math.floor(tonumber(value) or 1)); self:setControlText("raceType", self.raceType); return self.raceType end
function RaceSettings:addAiDifficulty(value) self.aiDifficulty = math.floor(self:limit(self.aiDifficulty + (tonumber(value) or 1), 0, 4, 1)); self:setControlText("difficulty", self.aiDifficulty); return self.aiDifficulty end
function RaceSettings:apply()
	local settings = { laps = self.laps, opponents = self.opponents, raceType = self.raceType, aiDifficulty = self.aiDifficulty }
	self:call(self:getApplicationManager(), { "setRaceSettings", "SetRaceSettings" }, settings)
	self:emit("applied", settings); return settings
end
function RaceSettings:clickBack() self:apply(); return self:clickClose() end

RaceSettings.Start = FlightDialog.start
RaceSettings.Update = FlightDialog.update
RaceSettings.AddLaps = RaceSettings.addLaps
RaceSettings.AddOpponent = RaceSettings.addOpponent
RaceSettings.AddRaceType = RaceSettings.addRaceType
RaceSettings.AddAiDifficulty = RaceSettings.addAiDifficulty
RaceSettings.ClickBack = RaceSettings.clickBack

class 'OnlineMenu' (FlightDialog)

function OnlineMenu:__init(component)
	FlightDialog.__init(self, component)
	self.regions = { "Auto", "Europe", "North America", "Asia", "Oceania" }
	self.regionIndex = 1
	self.sessionRegionIndex = 1
	self.raceSessionRegionIndex = 1
	self.playerName = "Pilot"
	self.rooms = {}
	self.selectedSession = nil
	self.connected = false
	self.activePanel = "browser"
	self.roomRefreshInterval = 2.0
	self.nextRoomRefresh = 0.0
end

function OnlineMenu:start()
	FlightDialog.start(self)
	self:populateAllRegionDropDowns()
	self:handleConnect()
	return true
end
function OnlineMenu:handleConnect()
	local manager = self:getNetworkManager()
	local called, state = self:call(manager, { "isConnected", "IsConnected", "getConnected", "GetConnected" })
	self.connected = called and state == true
	self:setStatus(self.connected and "Online" or "Offline", not self.connected)
	return self.connected
end
function OnlineMenu:populateGUI() self:updateRoomList(); return true end
function OnlineMenu:updateRoomList()
	local called, rooms = self:call(self:getNetworkManager(), { "getRoomList", "GetRoomList", "getSessions", "GetSessions" }, self.regions[self.regionIndex])
	if called then self.rooms = self:list(rooms) end
	self:emit("roomsChanged", self.rooms)
	return self.rooms
end
function OnlineMenu:update()
	FlightDialog.update(self)
	local now = self:getTime()
	if now >= self.nextRoomRefresh then self.nextRoomRefresh = now + self.roomRefreshInterval; self:handleConnect(); if self.connected then self:updateRoomList() end end
end
function OnlineMenu:showPanel(name) self.activePanel = tostring(name or "browser"); self:emit("panelChanged", self.activePanel); return true end
function OnlineMenu:clickGotoMainMenu() self:call(self:getUIManager(), { "setHomeState", "SetHomeState" }, 0); return true end
function OnlineMenu:clickCreateFreePanel() return self:showPanel("create_free") end
function OnlineMenu:clickCreateRacePanel() return self:showPanel("create_race") end
function OnlineMenu:clickJoinPanel(session) self.selectedSession = session or self.selectedSession; return self:showPanel("join") end
function OnlineMenu:createSession(race)
	local config = { playerName = self.playerName, region = self.regions[race and self.raceSessionRegionIndex or self.sessionRegionIndex], race = race == true }
	local called, result = self:call(self:getNetworkManager(), { "createSession", "CreateSession", "createRoom", "CreateRoom" }, config)
	if not called or result == false then self:setStatus("Could not create session", true); return false end
	self:emit("sessionCreated", config); return true
end
function OnlineMenu:clickCreateSession_() return self:createSession(false) end
function OnlineMenu:clickCreateSession() return self:createSession(false) end
function OnlineMenu:clickCreateRace() return self:createSession(true) end
function OnlineMenu:clickJoinSession(session)
	self.selectedSession = session or self.selectedSession
	if not self.selectedSession then self:setStatus("Select a session first", true); return false end
	local called, result = self:call(self:getNetworkManager(), { "joinSession", "JoinSession", "joinRoom", "JoinRoom" }, self.selectedSession, self.playerName)
	if not called or result == false then self:setStatus("Could not join session", true); return false end
	self:emit("sessionJoined", self.selectedSession); return true
end
function OnlineMenu:clickClose() return self:clickCloseDialog() end
function OnlineMenu:clickCloseDialog() return FlightDialog.clickClose(self) end
function OnlineMenu:populateAllRegionDropDowns() self:emit("regionsChanged", self.regions); return self.regions end
function OnlineMenu:onRegionDropDown(value) self.regionIndex = math.floor(self:limit((tonumber(value) or 0) + 1, 1, #self.regions, 1)); return self:updateRoomList() end
function OnlineMenu:onSessionRegionDropDown(value) self.sessionRegionIndex = math.floor(self:limit((tonumber(value) or 0) + 1, 1, #self.regions, 1)); return true end
function OnlineMenu:onRaceSessionRegionDropDown(value) self.raceSessionRegionIndex = math.floor(self:limit((tonumber(value) or 0) + 1, 1, #self.regions, 1)); return true end
function OnlineMenu:onPlayerNameTextChanged(value) self.playerName = tostring(value or "Pilot"); return true end
function OnlineMenu:onJoinPlayerNameTextChanged(value) return self:onPlayerNameTextChanged(value) end
function OnlineMenu:onRacePlayerNameTextChanged(value) return self:onPlayerNameTextChanged(value) end

OnlineMenu.HandleConnect = OnlineMenu.handleConnect
OnlineMenu.Start = OnlineMenu.start
OnlineMenu.Update = OnlineMenu.update
OnlineMenu.PopulateGUI = OnlineMenu.populateGUI
OnlineMenu.UpdateRoomList = OnlineMenu.updateRoomList
OnlineMenu.ClickGotoMainMenu = OnlineMenu.clickGotoMainMenu
OnlineMenu.ClickCreateFreePanel = OnlineMenu.clickCreateFreePanel
OnlineMenu.ClickCreateRacePanel = OnlineMenu.clickCreateRacePanel
OnlineMenu.ClickJoinPanel = OnlineMenu.clickJoinPanel
OnlineMenu.ClickCreateSession_ = OnlineMenu.clickCreateSession_
OnlineMenu.ClickCreateSession = OnlineMenu.clickCreateSession
OnlineMenu.ClickCreateRace = OnlineMenu.clickCreateRace
OnlineMenu.ClickJoinSession = OnlineMenu.clickJoinSession
OnlineMenu.ClickClose = OnlineMenu.clickClose
OnlineMenu.PopulateAllRegionDropDowns = OnlineMenu.populateAllRegionDropDowns
OnlineMenu.OnRegionDropDown = OnlineMenu.onRegionDropDown
OnlineMenu.OnSessionRegionDropDown = OnlineMenu.onSessionRegionDropDown
OnlineMenu.OnRaceSessionRegionDropDown = OnlineMenu.onRaceSessionRegionDropDown
OnlineMenu.OnPlayerNameTextChanged = OnlineMenu.onPlayerNameTextChanged
OnlineMenu.OnJoinPlayerNameTextChanged = OnlineMenu.onJoinPlayerNameTextChanged
OnlineMenu.OnRacePlayerNameTextChanged = OnlineMenu.onRacePlayerNameTextChanged

class 'ChatDialog' (FlightDialog)

function ChatDialog:__init(component)
	FlightDialog.__init(self, component)
	self.messages = {}
	self.maximumMessages = 200
	self.connected = false
	self.unreadCount = 0
	self.flightModePosition = nil
	self.workbenchPosition = nil
end

function ChatDialog:start() FlightDialog.start(self); self:updateConnect(); return true end
function ChatDialog:updateConnect()
	local called, value = self:call(self:getNetworkManager(), { "isConnected", "IsConnected" })
	self.connected = called and value == true
	self:setControlEnabled("input", self.connected)
	return self.connected
end
function ChatDialog:addMessage(sender, text, timestamp)
	local message = { sender = tostring(sender or "Pilot"), text = tostring(text or ""), time = timestamp or self:getTime() }
	table.insert(self.messages, message)
	while #self.messages > self.maximumMessages do table.remove(self.messages, 1) end
	if not self:isVisible() then self.unreadCount = self.unreadCount + 1 end
	self:emit("message", message); return message
end
function ChatDialog:sendMessage(text)
	text = tostring(text or "")
	if text == "" or not self.connected then return false end
	local called, result = self:call(self:getNetworkManager(), { "sendChatMessage", "SendChatMessage", "sendMessage", "SendMessage" }, text)
	if called and result ~= false then self:addMessage("Me", text); return true end
	return false
end
function ChatDialog:updateVisibility() if self:isVisible() then self.unreadCount = 0 end; return self:isVisible() end
function ChatDialog:onLevelWasLoaded() self:updateConnect(); return self:updateVisibility() end
function ChatDialog:update() FlightDialog.update(self); if self:shouldUpdate() then self:updateConnect(); self:updateVisibility() end end

ChatDialog.Start = ChatDialog.start
ChatDialog.Update = ChatDialog.update
ChatDialog.UpdateVisibility = ChatDialog.updateVisibility
ChatDialog.UpdateConnect = ChatDialog.updateConnect
ChatDialog.OnLevelWasLoaded = ChatDialog.onLevelWasLoaded
ChatDialog.SendMessage = ChatDialog.sendMessage
ChatDialog.AddMessage = ChatDialog.addMessage
