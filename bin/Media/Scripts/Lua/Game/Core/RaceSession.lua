-- Engine-independent race timing and checkpoint rules, shared by samples and games.
RaceSession = RaceSession or {}
RaceSession.__index = RaceSession

function RaceSession.new()
    return setmetatable({state = "idle"}, RaceSession)
end

function RaceSession.resetProgress(progress)
    progress.lastTrackIndex, progress.nextCheckpoint, progress.lap = 0, 1, 1
    progress.lapStart = nil
    progress.lastLapTime, progress.bestLapTime = 0, 0
end

-- A lap needs each quarter in order and a forward crossing of the start line.
-- Reverse travel and discontinuous jumps cannot award a checkpoint or a lap.
function RaceSession.advanceLap(progress, index, count, onRoad, now)
    if count <= 0 then return false end
    progress.lapStart = progress.lapStart or now
    local previous = progress.lastTrackIndex
    local forward = (index - previous + count) % count
    local continuous = forward > 0 and forward <= count / 8
    local completed = false
    if continuous and onRoad then
        if previous > count * 3 / 4 and index < count / 4 and progress.nextCheckpoint == 4 then
            progress.lastLapTime = now - progress.lapStart
            if progress.bestLapTime == 0 or progress.lastLapTime < progress.bestLapTime then
                progress.bestLapTime = progress.lastLapTime
            end
            progress.lap, progress.lapStart, progress.nextCheckpoint = progress.lap + 1, now, 1
            completed = true
        elseif math.floor(index * 4 / count) == progress.nextCheckpoint then
            progress.nextCheckpoint = progress.nextCheckpoint + 1
        end
    elseif forward > count / 8 then
        progress.nextCheckpoint = 1
    end
    progress.lastTrackIndex = index
    return completed
end

function RaceSession:begin(mode, laps)
    self.mode, self.lapLimit = mode or "race", math.max(1, math.floor(laps or 3))
    self.state, self.elapsed, self.countdown = "countdown", 0, 3
    self.completedLaps, self.lapTimes = 0, {}
    RaceSession.resetProgress(self)
end

function RaceSession:pause()
    if self.state ~= "racing" and self.state ~= "countdown" then return false end
    self.resumeState, self.state = self.state, "paused"
    return true
end

function RaceSession:resume()
    if self.state ~= "paused" then return false end
    self.state, self.resumeState = self.resumeState, nil
    return true
end

function RaceSession:finish()
    if self.state == "idle" or self.state == "finished" then return false end
    self.state = "finished"
    return true
end

function RaceSession:update(dt, index, count, onRoad)
    dt = math.max(0, dt or 0)
    if self.state == "countdown" then
        self.countdown = math.max(0, self.countdown - dt)
        if self.countdown == 0 then
            self.state, self.lapStart, self.lastTrackIndex = "racing", 0, index or 0
            return "go"
        end
    elseif self.state == "racing" then
        self.elapsed = self.elapsed + dt
        if RaceSession.advanceLap(self, index, count, onRoad, self.elapsed) then
            self.completedLaps = self.completedLaps + 1
            self.lapTimes[self.completedLaps] = self.lastLapTime
            if self.mode == "race" and self.completedLaps >= self.lapLimit then
                self:finish()
                return "finish"
            end
            return "lap"
        end
    end
end

function RaceSession.formatTime(seconds)
    if not seconds or seconds <= 0 then return "--:--.---" end
    local ms = math.floor(seconds * 1000 + 0.5)
    return string.format("%02d:%02d.%03d", math.floor(ms / 60000), math.floor(ms / 1000) % 60, ms % 1000)
end
