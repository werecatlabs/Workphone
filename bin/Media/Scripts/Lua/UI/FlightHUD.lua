include("FlightDialog.lua")

class 'Hud' (FlightDialog)

function Hud:__init(component)
	FlightDialog.__init(self, component)
	self.controlsFpsText = nil
	self.telemetry = { altitude = 0, airspeed = 0, heading = 0, battery = 1, signal = 1, fps = 0 }
	self.updateInterval = 0.10
end

function Hud:setControlsFpsText(value) self.controlsFpsText = value end
function Hud:setTelemetryControl(name, value) self:bindControl(name, value) end

function Hud:start()
	FlightDialog.start(self)
	self:setStatus("Flight HUD ready", false)
	return true
end

function Hud:readTelemetry()
	local app = self:getApplicationManager()
	local called, data = self:call(app, { "getFlightTelemetry", "GetFlightTelemetry", "getTelemetry", "GetTelemetry" })
	if called and data then
		for key in pairs(self.telemetry) do
			local value = self:read(data, { "get" .. key, "Get" .. key }, { key }, nil)
			if value ~= nil then self.telemetry[key] = value end
		end
	end
	return self.telemetry
end

function Hud:updateInfo()
	local data = self:readTelemetry()
	self:setControlText("Altitude", string.format("%.0f m", tonumber(data.altitude) or 0))
	self:setControlText("Airspeed", string.format("%.0f km/h", tonumber(data.airspeed) or 0))
	self:setControlText("Heading", string.format("%03d°", math.floor(tonumber(data.heading) or 0) % 360))
	self:setControlText("Battery", string.format("%.0f%%", self:limit(data.battery, 0, 1, 0) * 100))
	self:setControlText("Signal", string.format("%.0f%%", self:limit(data.signal, 0, 1, 0) * 100))
	self:setControlText(self.controlsFpsText, string.format("FPS %.0f", tonumber(data.fps) or 0))
	self:emit("telemetry", data)
	return data
end

function Hud:update()
	FlightDialog.update(self)
	if self:shouldUpdate() then self:updateInfo() end
end

function Hud:setTelemetry(values)
	if type(values) ~= "table" then return false end
	for key in pairs(self.telemetry) do if values[key] ~= nil then self.telemetry[key] = values[key] end end
	self:updateInfo()
	return true
end

Hud.Start = Hud.start
Hud.Update = Hud.update
Hud.UpdateInfo = Hud.updateInfo

class 'TrainingHud' (FlightDialog)

function TrainingHud:__init(component)
	FlightDialog.__init(self, component)
	self.progressPieChart = nil
	self.progress = 0.0
	self.objective = ""
	self.completed = false
end

function TrainingHud:setProgressPieChart(value) self.progressPieChart = value end
function TrainingHud:setObjective(value) self.objective = tostring(value or ""); self:setControlText("objective", self.objective) end
function TrainingHud:setProgress(value)
	self.progress = self:limit(value, 0, 1, 0)
	self.completed = self.progress >= 1.0
	self:setControlValue(self.progressPieChart, self.progress)
	self:setControlText("progress", string.format("%.0f%%", self.progress * 100))
	if self.completed then self:setStatus("Objective complete", false); self:emit("completed", self.objective) end
	return self.progress
end

function TrainingHud:resetProgress() self.completed = false; return self:setProgress(0) end

TrainingHud.SetObjective = TrainingHud.setObjective
TrainingHud.SetProgress = TrainingHud.setProgress
TrainingHud.ResetProgress = TrainingHud.resetProgress

class 'DemoHud' (FlightDialog)

function DemoHud:__init(component)
	FlightDialog.__init(self, component)
	self.timeText = nil
	self.nextTimeUpdate = 0.0
	self.startedAt = 0.0
end

function DemoHud:setTimeText(value) self.timeText = value end
function DemoHud:start() FlightDialog.start(self); self.startedAt = self:getTime(); return true end
function DemoHud:update()
	FlightDialog.update(self)
	local now = self:getTime()
	if now >= self.nextTimeUpdate then
		self.nextTimeUpdate = now + 1.0
		local elapsed = math.max(0, math.floor(now - self.startedAt))
		self:setControlText(self.timeText, string.format("%02d:%02d", math.floor(elapsed / 60), elapsed % 60))
	end
end

DemoHud.Start = DemoHud.start
DemoHud.Update = DemoHud.update

class 'TrainingMenu' (FlightDialog)

function TrainingMenu:__init(component)
	FlightDialog.__init(self, component)
	self.lessons = {}
	self.selectedLesson = 1
	self.activeLesson = nil
end

function TrainingMenu:setLessons(value) self.lessons = self:list(value); self.selectedLesson = math.min(math.max(1, self.selectedLesson), math.max(1, #self.lessons)) end
function TrainingMenu:selectLesson(index) self.selectedLesson = math.min(math.max(1, math.floor(tonumber(index) or 1)), math.max(1, #self.lessons)); return self.lessons[self.selectedLesson] end
function TrainingMenu:startLesson(index)
	local lesson = self:selectLesson(index or self.selectedLesson)
	if not lesson then self:setStatus("No training lesson selected", true); return false end
	self.activeLesson = lesson
	local called = self:call(self:getApplicationManager(), { "startTraining", "StartTraining", "startLesson", "StartLesson" }, lesson)
	self:emit("lessonStarted", lesson)
	return called
end
function TrainingMenu:onClickClose() return self:clickClose() end
function TrainingMenu:update() FlightDialog.update(self) end

TrainingMenu.Start = FlightDialog.start
TrainingMenu.Update = TrainingMenu.update
TrainingMenu.OnClickClose = TrainingMenu.onClickClose
TrainingMenu.StartLesson = TrainingMenu.startLesson

class 'StatsMenu' (FlightDialog)

function StatsMenu:__init(component)
	FlightDialog.__init(self, component)
	self.statistics = {}
	self.updateInterval = 0.5
end

function StatsMenu:setStatistics(value) self.statistics = type(value) == "table" and value or {}; self:refresh(); return true end
function StatsMenu:refresh()
	local called, stats = self:call(self:getApplicationManager(), { "getFlightStatistics", "GetFlightStatistics", "getStatistics", "GetStatistics" })
	if called and type(stats) == "table" then self.statistics = stats end
	for name, value in pairs(self.statistics) do self:setControlText(name, value) end
	self:emit("refreshed", self.statistics)
	return self.statistics
end
function StatsMenu:update() FlightDialog.update(self); if self:shouldUpdate() then self:refresh() end end
function StatsMenu:onEnable() FlightDialog.onEnable(self); self:refresh(); return true end
function StatsMenu:onClickClose() return self:clickClose() end

StatsMenu.Start = FlightDialog.start
StatsMenu.Update = StatsMenu.update
StatsMenu.OnEnable = StatsMenu.onEnable
StatsMenu.OnClickClose = StatsMenu.onClickClose

class 'CGModeInfo' (FlightDialog)

function CGModeInfo:__init(component)
	FlightDialog.__init(self, component)
	self.texts = {}
	self.cgData = {}
end

function CGModeInfo:setTexts(value) self.texts = self:list(value) end
function CGModeInfo:update()
	FlightDialog.update(self)
	if not self:shouldUpdate() then return end
	local called, data = self:call(self:getApplicationManager(), { "getCentreOfGravityInfo", "GetCentreOfGravityInfo", "getCGInfo", "GetCGInfo" })
	if called and data then self.cgData = data end
	local labels = { "Mass", "CG X", "CG Y", "CG Z" }
	for index, control in ipairs(self.texts) do
		local value = self.cgData[index] or self.cgData[labels[index]] or "--"
		self:setControlText(control, (labels[index] or "CG") .. ": " .. tostring(value))
	end
end

CGModeInfo.Start = FlightDialog.start
CGModeInfo.Update = CGModeInfo.update

class 'Tooltip' (FlightDialog)

function Tooltip:__init(component)
	FlightDialog.__init(self, component)
	self.tooltipText = ""
	self.target = nil
	self.delay = 0.35
	self.showAt = nil
end

function Tooltip:updateToolTip(value) self.tooltipText = tostring(value or ""); self:setControlText("text", self.tooltipText); return true end
function Tooltip:onPointerEnter(target) self.target = target; self.showAt = self:getTime() + self.delay; return true end
function Tooltip:onPointerExit() self.target = nil; self.showAt = nil; self:hide(true); return true end
function Tooltip:onSelect(target) return self:onPointerEnter(target) end
function Tooltip:onDeselect() return self:onPointerExit() end
function Tooltip:update()
	FlightDialog.update(self)
	if self.target and self.showAt and self:getTime() >= self.showAt then self.showAt = nil; self:show(true) end
end

Tooltip.OnPointerEnter = Tooltip.onPointerEnter
Tooltip.OnPointerExit = Tooltip.onPointerExit
Tooltip.OnSelect = Tooltip.onSelect
Tooltip.OnDeselect = Tooltip.onDeselect
Tooltip.OnDisable = Tooltip.onPointerExit
Tooltip.OnBecameInvisible = Tooltip.onPointerExit
Tooltip.OnEnable = FlightDialog.onEnable
Tooltip.OnBecameVisible = FlightDialog.onEnable
Tooltip.UpdateToolTip = Tooltip.updateToolTip

class 'InformationText' (FlightDialog)

function InformationText:__init(component)
	FlightDialog.__init(self, component)
	self.text = ""
	self.isError = false
	self.expiresAt = nil
end

function InformationText:setText(value) self.text = tostring(value or ""); self:setControlText("text", self.text); return true end
function InformationText:setError(value) self.isError = value == true; return true end
function InformationText:showMessage(value, duration, isError)
	self:setText(value); self:setError(isError); self.expiresAt = duration and duration > 0 and self:getTime() + duration or nil; self:show(true); return true
end
function InformationText:update()
	FlightDialog.update(self)
	if self.expiresAt and self:getTime() >= self.expiresAt then self.expiresAt = nil; self:hide(true) end
end

InformationText.SetText = InformationText.setText
InformationText.SetError = InformationText.setError
InformationText.ShowMessage = InformationText.showMessage
