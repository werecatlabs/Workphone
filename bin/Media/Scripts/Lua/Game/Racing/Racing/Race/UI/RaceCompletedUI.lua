if not RaceSession then include("RaceSession.lua") end
class 'RaceCompletedUI' (BaseComponent)
function RaceCompletedUI:__init(component) BaseComponent.__init(self, component) end
function RaceCompletedUI:summary(session, seed, record)
    local text = {string.format("Track %d   |   %d completed laps", seed, session.completedLaps),
        "Total " .. RaceSession.formatTime(session.elapsed) .. "   Best " .. RaceSession.formatTime(session.bestLapTime),
        "Track record " .. RaceSession.formatTime(record)}
    for i, time in ipairs(session.lapTimes) do
        if i <= 6 then text[#text + 1] = string.format("Lap %d   %s", i, RaceSession.formatTime(time)) end
    end
    return table.concat(text, "\n")
end
