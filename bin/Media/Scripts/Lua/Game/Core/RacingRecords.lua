-- Plain data rather than executable Lua: safe to load from a player-owned file.
if not DataLoader then include("DataLoader.lua") end
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
    local text = file:read(1048577) or ""; file:close()
    if #text > 1048576 then self.loadError="Lap records exceed 1 MiB"; return false,self.loadError end
    local entries, settings, best = 0, {seed=7,quality=1,laps=3}, {}
    for line in text:gmatch("[^\r\n]+") do
        local key, value = line:match("^([%w:_]+)=([%d%.]+)$")
        if key == "seed" then settings.seed = math.floor(number(value, 0, 4294967295) or 7)
        elseif key == "quality" then settings.quality = math.floor(number(value, 0, 3) or 1)
        elseif key == "laps" then settings.laps = math.floor(number(value, 1, 10) or 3)
        elseif key and key:match("^best:%d+$") then
            entries=entries+1
            if entries>4096 then self.loadError="Too many lap records"; return false,self.loadError end
            if number(tonumber(key:sub(6)),0,4294967295) then best[key] = number(value, 0.001, 86400) end
        end
    end
    self.loadError=nil
    self.settings, self.best = settings, best
    return true
end

function RacingRecords:getBest(seed)
    if not number(seed,0,4294967295) then return 0 end
    return self.best["best:" .. tostring(math.floor(seed))] or 0
end

function RacingRecords:record(seed, time)
    if not number(seed,0,4294967295) or not number(time, 0.001, 86400) then return false end
    local key = "best:" .. tostring(math.floor(seed))
    if self.best[key] and self.best[key] <= time then return false end
    self.best[key] = time
    return true
end

function RacingRecords:save()
    if self.loadError then return false,self.loadError end
    local settings = self.settings
    if not number(settings.seed,0,4294967295) or not number(settings.quality,0,3) or not number(settings.laps,1,10) then
        return false,"Invalid racing preferences"
    end
    local lines = {string.format("seed=%d\nquality=%d\nlaps=%d\n", math.floor(settings.seed), math.floor(settings.quality), math.floor(settings.laps))}
    local keys = {}
    for key in pairs(self.best) do keys[#keys + 1] = key end
    table.sort(keys)
    for _, key in ipairs(keys) do
        if not key:match("^best:%d+$") or not number(self.best[key],0.001,86400) then return false,"Invalid lap record" end
        lines[#lines+1] = string.format("%s=%.6f\n", key, self.best[key])
    end
    return DataLoader.writeAtomic(self.path, table.concat(lines))
end
