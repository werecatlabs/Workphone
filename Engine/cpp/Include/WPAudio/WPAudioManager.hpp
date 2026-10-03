#ifndef _SoundManager_H
#define _SoundManager_H

#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Sound/SoundManager.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <map>

namespace workphone
{
    /**
     * @class WPAudioManager
     * @brief Audio manager implementation for the Workphone audio backend.
     *
     * WPAudioManager is responsible for initializing and managing the platform-specific
     * audio system (XAudio2 on Windows, Audio Unit on macOS/iOS, and OpenSL ES on
     * Android). It handles:
     * - Creating and destroying sound instances
     * - Managing global audio state (master volume, mute)
     * - Managing 3D audio listeners
     * - Providing access to the underlying audio API for sound instances
     *
     * Individual sound loading, playback, and per-sound resources are encapsulated
     * within WPAudioSound instances to maintain proper separation of concerns.
     */
    class WPAudioManager : public SoundManager
    {
    public:
        /**
         * @brief Construct a new WPAudioManager instance.
         */
        WPAudioManager();

        /**
         * @brief Destroy the WPAudioManager and release all resources.
         */
        ~WPAudioManager() override;

        /**
         * @brief Load and initialize the audio manager.
         *
         * Initializes the platform-specific audio system.
         *
         * @param data Optional initialization data.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload and shut down the audio manager.
         *
         * Releases all sounds and platform-specific audio resources.
         *
         * @param data Optional unload data.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Update the audio manager and all active sounds.
         *
         * Called once per frame to update sound states and process audio.
         */
        void update() override;

        /**
         * @brief Load a mapping of sound names to file paths.
         *
         * @param soundMap Properties containing sound name/path mappings.
         */
        void loadSoundMap( const Properties &soundMap );

        /**
         * @brief Load sound event definitions from a file.
         *
         * @param filePath Path to the sound events file.
         * @return true if loaded successfully, false otherwise.
         */
        bool loadSoundEvents( const String &filePath );

        /**
         * @brief Create and load a sound from a file.
         *
         * @param filePath Path to the audio file (e.g., WAV file).
         * @param loop Whether the sound should loop.
         * @return Smart pointer to the created sound, or nullptr on failure.
         */
        SmartPtr<ISound> addSound( const String &filePath, bool loop = true );

        /**
         * @brief Remove and unload a sound.
         *
         * @param sound The sound to remove.
         */
        void removeSound( SmartPtr<ISound> sound );

        /**
         * @brief Add a 3D sound listener.
         *
         * @param name Unique name for the listener.
         * @param position Initial 3D position of the listener.
         * @return Smart pointer to the created listener.
         */
        SmartPtr<ISoundListener3> addListener3( const String &name, const Vector3F &position ) override;

        /**
         * @brief Remove all sounds and listeners.
         */
        void removeAll();

        /**
         * @brief Find a listener by name.
         *
         * @param name Name of the listener to find.
         * @return Smart pointer to the listener, or nullptr if not found.
         */
        SmartPtr<ISoundListener3> findListener3( const String &name ) override;

        /**
         * @brief Set the master volume for all sounds.
         *
         * @param fVolume Volume level (0.0 to 1.0).
         */
        void setVolume( f32 fVolume ) override;

        /**
         * @brief Get the current master volume.
         *
         * @return Volume level (0.0 to 1.0).
         */
        f32 getVolume() const override;

        /**
         * @brief Start recording audio input.
         */
        void startRecording() override;

        /**
         * @brief Stop recording audio input.
         */
        void stopRecording() override;

        /**
         * @brief Get the size of the recording buffer.
         *
         * @return Buffer size in bytes.
         */
        u32 getBufferSize() const override;

        /**
         * @brief Copy recorded audio data to memory.
         *
         * @param buffer Destination buffer.
         * @param size Size of the buffer in bytes.
         */
        void copyContentsToMemory( void *buffer, u32 size ) override;

        /**
         * @brief Get the underlying platform-specific audio object.
         *
         * @param ppObject Output pointer to receive the audio object.
         */
        void _getObject( void **ppObject ) const override;

        /**
         * @brief Check if the audio system is running in real-time mode.
         *
         * @return true if real-time, false otherwise.
         */
        bool isRealtime() const override;

        /**
         * @brief Check if audio is currently muted.
         *
         * @return true if muted, false otherwise.
         */
        bool isMute() const override;

        /**
         * @brief Set the mute state for all audio.
         *
         * @param mute true to mute, false to unmute.
         */
        void setMute( bool mute ) override;

        WP_CLASS_REGISTER_DECL;

    private:
        struct PlatformAudioState;

        /**
         * @brief Initialize the native audio backend for the target platform.
         *
         * @return true if successful, false otherwise.
         */
        bool initializePlatformAudio();

        /**
         * @brief Release all native audio backend resources.
         */
        void cleanupPlatformAudio();

        /// Opaque native state. Platform SDK types are intentionally kept out of this header.
        PlatformAudioState *m_platformAudioState = nullptr;

        /// Collection of all active sounds
        ConcurrentArray<SmartPtr<ISound>> m_sounds;

        /// Map of named 3D audio listeners
        using SoundListenerMap = std::map<String, SmartPtr<ISoundListener3>>;
        SoundListenerMap m_listeners;

        /// Map of sound names to file paths
        using SoundMap = std::map<String, String>;
        SoundMap m_soundMap;

        /// Master volume level (0.0 to 1.0)
        f32 m_volume = 1.0f;

        /// Global mute state
        bool m_mute = false;
    };
}  // namespace workphone

#endif
