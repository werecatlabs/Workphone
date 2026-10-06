if not RacingSupport then include("RacingSupport.lua") end
-- Register loaded native ISound objects. Asset loading belongs to the resource
-- owner; this manager controls playback without allocating temporary actors.
class 'SoundManager' (RacingComponent)
function SoundManager:__init(component)
    BaseComponent.__init(self, component); self.sounds, self.musicVolume, self.effectVolume, self.muted = {}, 0.5, 1, false
end
function SoundManager:register(name, sound)
    assert(type(name) == "string" and name ~= "" and sound, "Sound name and ISound required")
    assert(not self.sounds[name], "Sound already registered"); self.sounds[name] = sound
end
function SoundManager:PlaySound(name)
    local sound = self.sounds[name]
    if not sound then return false, "Unknown sound: " .. tostring(name) end
    sound:setLoop(false); sound:setVolume(self.muted and 0 or self.effectVolume)
    sound:stop(); sound:play(); return true
end
function SoundManager:PlaySoundAtLocation(name, position)
    local sound = self.sounds[name]
    if not sound then return false, "Unknown sound: " .. tostring(name) end
    sound:setPosition(RacingSupport.vector(position)); return self:PlaySound(name)
end
function SoundManager:playMusic(name)
    local sound = self.sounds[name]; if not sound then return false, "Unknown music" end
    if self.music then self.music:stop() end
    self.music = sound; sound:setLoop(true); sound:setVolume(self.muted and 0 or self.musicVolume); sound:play(); return true
end
function SoundManager:setVolumes(music, effects)
    self.musicVolume, self.effectVolume = RacingSupport.number(music, 0, 1), RacingSupport.number(effects, 0, 1)
    for _, sound in pairs(self.sounds) do sound:setVolume(self.muted and 0 or (sound == self.music and music or effects)) end
end
function SoundManager:mute(value) self.muted = value == true; self:setVolumes(self.musicVolume, self.effectVolume) end
function SoundManager:shutdown()
    for _, sound in pairs(self.sounds) do sound:stop() end
    self.sounds, self.music = {}, nil
end
