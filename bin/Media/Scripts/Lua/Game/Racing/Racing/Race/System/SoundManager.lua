class 'SoundManager' (BaseComponent)

function SoundManager:__init()
	BaseComponent.__init(self)

	if SoundManager.instance then
		Log.Warning("SoundManager instance already exists. Overwriting.")
	end
	SoundManager.instance = self

	-- Example list of game sounds. You should populate this with your actual sound data.
	-- The 'sound' value would typically be a path to your audio file.
	self.gameSounds = {
		-- { soundName = "ButtonClick", sound = "path/to/your/button_click.wav" },
		-- { soundName = "PlayerDeath", sound = "path/to/your/player_death.ogg" },
	}

	-- The background music to be played.
	self.backgroundMusic = nil -- e.g., "path/to/your/music.mp3"
	-- The volume of the background music.
	self.backgroundMusicVolume = 0.5

	-- In many component-based systems, we need to ensure the entity has an AudioSource.
	-- This line is a placeholder; you might need to adapt it to your engine's API.
	if self.entity and not self.entity:GetComponent("AudioSource") then
		self.entity:AddComponent("AudioSource")
	end
end

-- This function is typically called once the component is ready and active.
function SoundManager:OnStart()
	-- The C# version creates a new GameObject for background music.
	-- We'll replicate that by creating a new entity.
	if self.backgroundMusic then
		-- NOTE: The following lines assume your engine has a 'World' or similar global
		-- object to create entities, and that entities have methods to add components
		-- and play audio. You will likely need to change this to your engine's API.
		local bgmEntity = World:CreateEntity("Background Music")
		if bgmEntity then
			local audioSource = bgmEntity:AddComponent("AudioSource")
			if audioSource then
				audioSource.clip = self.backgroundMusic
				audioSource.volume = self.backgroundMusicVolume
				audioSource.loop = true
				audioSource.spatialBlend = 0 -- 2D sound for background music.
				audioSource:Play()
			end
		end
	end
end

-- Plays a sound from the 'gameSounds' list.
-- @param name: The name of the sound to play.
-- @param sound2D: Boolean, true for 2D sound (no spatial blend), false for 3D.
function SoundManager:PlaySound(name, sound2D)
	local audioSource = self.entity:GetComponent("AudioSource")
	if not audioSource then
		Log.Error("This entity does not have an AudioSource component.")
		return
	end

	audioSource.spatialBlend = sound2D and 0 or 1

	for _, soundData in ipairs(self.gameSounds) do
		if soundData.soundName == name then
			-- 'PlayOneShot' is a common function name from Unity. Your engine
			-- might have a different function, like 'Play' or 'Trigger'.
			audioSource:PlayOneShot(soundData.sound)
			return
		end
	end
end

-- Plays a sound from the 'gameSounds' list at a specific 3D position.
-- @param name: The name of the sound to play.
-- @param position: A Vector3-like table (e.g., {x=0, y=0, z=0}) for the position.
function SoundManager:PlaySoundAtLocation(name, position)
	for _, soundData in ipairs(self.gameSounds) do
		if soundData.soundName == name then
			-- Here we assume a default volume of 1.0 for the sound effect.
			self:PlayClip(soundData.sound, position, 1.0)
			return
		end
	end
end

-- Creates a temporary entity to play an audio clip at a certain position.
-- @param clip: The audio clip (e.g., path to file) to play.
-- @param position: A Vector3-like table for the position.
-- @param volume: The volume of the clip.
function SoundManager:PlayClip(clip, position, volume)
	-- NOTE: The following code for creating a temporary entity is a placeholder.
	-- You will need to adapt it to your engine's specific API for creating
	-- entities, setting their position, handling audio playback, and
	-- destroying them after a delay.
	local tempEntity = World:CreateEntity("OneShotAudio")
	if tempEntity then
		tempEntity:SetPosition(position)
		local audioSource = tempEntity:AddComponent("AudioSource")

		if audioSource then
			audioSource.spatialBlend = 1.0 -- 3D sound.
			audioSource.clip = clip
			audioSource.volume = volume
			audioSource:Play()

			-- We need to destroy the temporary entity after the clip finishes.
			-- This requires getting the clip's length and a way to schedule
			-- a delayed action, which is highly engine-specific.
			-- local clipLength = audioSource:GetClipLength() -- This is a guess.
			-- Timer:CallLater(clipLength, function() tempEntity:Destroy() end)
		end
	end
end