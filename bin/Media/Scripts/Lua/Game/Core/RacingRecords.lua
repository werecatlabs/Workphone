-- Plain data rather than executable Lua: safe to load from a player-owned file.
RacingRecords = RacingRecords or {}
RacingRecords.__index = RacingRecords

local function number(value, low, high)
    value = tonumber(value)
    if value and value == value and value >= low and value <= high then return value end
end

function RacingRecords.new(path)
    return setmetatable({path = path, best = {}, settings = {seed = 7, quality = 1, laps = 3}}, RacingRecords)
end

function RacingRecords:load()
    local file = io.open(self.path, "r")
    if not file then return false end
    for line in file:lines() do
        local key, value = line:match("^([%w:_]+)=([%d%.]+)$")
        if key == "seed" then self.settings.seed = math.floor(number(value, 0, 4294967295) or 7)
        elseif key == "quality" then self.settings.quality = math.floor(number(value, 0, 3) or 1)
        elseif key == "laps" then self.settings.laps = math.floor(number(value, 1, 10) or 3)
        elseif key and key:match("^best:%d+$") then
            self.best[key] = number(value, 0.001, 86400)
        end
    end
    file:close()
    return true
end

function RacingRecords:getBest(seed)
    return self.best["best:" .. tostring(math.floor(seed))] or 0
end

function RacingRecords:record(seed, time)
    if not number(time, 0.001, 86400) then return false end
    local key = "best:" .. tostring(math.floor(seed))
    if self.best[key] and self.best[key] <= time then return false end
    self.best[key] = time
    return true
end

function RacingRecords:save()
    local file, errorMessage = io.open(self.path, "w")
    if not file then return false, errorMessage end
    local settings = self.settings
    local ok, failure = file:write(string.format("seed=%d\nquality=%d\nlaps=%d\n", settings.seed, settings.quality, settings.laps))
    local keys = {}
    for key in pairs(self.best) do keys[#keys + 1] = key end
    table.sort(keys)
    for _, key in ipairs(keys) do
        if ok then ok, failure = file:write(string.format("%s=%.6f\n", key, self.best[key])) end
    end
    local closed, closeError = file:close()
    return ok ~= nil and closed ~= nil, failure or closeError
end
