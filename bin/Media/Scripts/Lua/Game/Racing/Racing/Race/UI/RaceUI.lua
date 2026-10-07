if not RacingSupport then include("RacingSupport.lua") end
if not RaceSession then include("RaceSession.lua") end
if not UICanvas then include("UICanvas.lua") end
class 'RaceUI' (RacingComponent)
function RaceUI:__init(component) BaseComponent.__init(self, component); self.lastText = {} end
function RaceUI:generate(manager, owner)
    self:destroy()
    self.manager = manager
    local ok, failure = pcall(function()
    local function actor(parent, name, x, y, width, height, z)
        local item = manager:createActor()
        assert(item, "Cannot create HUD actor: " .. name)
        if not self.root then self.root = item end -- Track ownership before a binding can fail.
        item:setName(name); parent:addChild(item)
        local layout = UICanvas.addComponent(item, "LayoutTransform")
        UICanvas.center(layout)
        layout:setPosition(Vector2F(x, y)); layout:setSize(Vector2F(width, height)); layout:setZOrder(z, false)
        return item
    end
    self.root = actor(owner, "Racing.HUD", 0, 0, 1920, 1080, 10)
    UICanvas.attach(self.root, false)
    local panel = actor(self.root, "Dashboard", 0, 405, 1500, 170, 10)
    UICanvas.addComponent(panel, "Image"):setColour(ColourF(0.012, 0.022, 0.04, 0.90))
    UICanvas.addComponent(panel, "Material"):setMaterialPath("DefaultUI.mat")
    local function text(parent, name, y, height, size)
        local item = actor(parent, name, 0, y, 1460, height, 12)
        local label = UICanvas.addComponent(item, "Text")
        local properties = label:getProperties()
        properties:setPropertyAsInt("size", size)
        label:setProperties(properties)
        label:setHorizontalAlignment(2); label:setVerticalAlignment(2)
        label:setColour(ColourF(0.94, 0.97, 1, 1))
        return label
    end
    self.dashboard = text(panel, "Race telemetry", 0, 146, 30)
    self.countdown = text(self.root, "Countdown", -100, 140, 72)
    self.help = text(self.root, "Controls", 510, 42, 22)
    self.help:setText("W/S or arrows: throttle/brake   A/D: steer   R: restart   C: camera   Esc: pause")
    end)
    if not ok then self:destroy(); error("RaceUI: " .. tostring(failure)) end
    return true
end
function RaceUI:setText(field, value)
    if self.lastText[field] ~= value then self[field]:setText(value); self.lastText[field] = value end
end
function RaceUI:resetSession()
    self:setText("dashboard", "Preparing vehicle and circuit...")
    self:setText("countdown", "GET READY")
end
function RaceUI:updateSession(session, statistics, progress, speed, gear, record, seed)
    local laps = session.mode == "timeTrial" and tostring(statistics.m_Lap)
        or string.format("%d / %d", math.min(statistics.m_Lap, session.lapLimit), session.lapLimit)
    self:setText("dashboard", string.format(
        "%s   |   %.0f km/h   |   Gear %s   |   Lap %s\nCurrent %s   Last %s   Best %s\nTotal %s   Record %s   Track %d%s",
        session.mode == "timeTrial" and "TIME TRIAL" or "CIRCUIT CHALLENGE", speed * 3.6, gear, laps,
        statistics.currentLapTime, statistics.prevLapTime, statistics.bestLapTime, statistics.totalRaceTime,
        RaceSession.formatTime(record), seed, progress.offTrack and "   OFF TRACK" or progress.wrongWay and "   WRONG WAY" or ""))
    self:setText("countdown", session.state == "countdown" and tostring(math.ceil(session.countdown))
        or session.state == "racing" and session.elapsed < 1 and "GO!" or "")
end
function RaceUI:show(value) if self.root then self.root:setEnabled(value, false) end end
function RaceUI:destroy()
    if self.root then self.manager:destroyActor(self.root, true) end
    self.root, self.lastText = nil, {}
    self.dashboard, self.countdown, self.help = nil, nil, nil
end
RaceUI.shutdown = RaceUI.destroy
