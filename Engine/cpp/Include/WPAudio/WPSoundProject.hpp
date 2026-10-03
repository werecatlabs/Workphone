#ifndef __WPSoundProject_h__
#define __WPSoundProject_h__

#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Sound/SoundProject.hpp>
#include <Workphone/Core/Array.hpp>
#include <map>

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
#        include <xaudio2fx.h>
#    endif
#endif

namespace workphone
{
    /**
     * @class WPSoundProject
     * @brief Audio project implementation for managing sound events, master volume, and reverb.
     *
     * WPSoundProject encapsulates project-level audio settings and sound event collections.
     * It manages:
     * - Collection of sound events organized by name
     * - Master volume control for the entire project
     * - Reverb effects and settings
     * - Sound event lifecycle (creation, removal)
     *
     * This class maintains proper encapsulation by managing its own sound events rather than
     * delegating to the manager, which should only handle low-level audio system resources.
     */
    class WPSoundProject : public SoundProject
    {
    public:
        /**
         * @brief Construct a new WPSoundProject instance.
         */
        WPSoundProject();

        /**
         * @brief Destroy the WPSoundProject and release all resources.
         */
        ~WPSoundProject() override;

        /**
         * @brief Add a sound event to the project.
         *
         * Creates a new sound event and adds it to the project's collection.
         *
         * @param soundEventName Name identifier for the sound event.
         * @return Smart pointer to the created sound event, or nullptr on failure.
         */
        SmartPtr<ISoundEvent> addSoundEvent( const String &soundEventName ) override;

        /**
         * @brief Remove a sound event from the project.
         *
         * @param soundEvent The sound event to remove.
         */
        void removeSoundEvent( SmartPtr<ISoundEvent> soundEvent ) override;

        /**
         * @brief Get the project's master volume.
         *
         * @return Master volume level (0.0 to 1.0).
         */
        f32 getMasterVolume() const override;

        /**
         * @brief Set the project's master volume.
         *
         * This affects all sound events within the project.
         *
         * @param masterVolume Volume level (0.0 to 1.0).
         */
        void setMasterVolume( f32 masterVolume ) override;

        /**
         * @brief Set up reverb effect for the project.
         *
         * Initializes platform-specific reverb (e.g., XAudio2 reverb on Windows).
         */
        void setupReverb() override;

        /**
         * @brief Check if reverb is enabled.
         *
         * @return true if reverb is enabled, false otherwise.
         */
        bool getEnableReverb() const override;

        /**
         * @brief Enable or disable reverb for the project.
         *
         * @param enableReverb true to enable, false to disable.
         */
        void setEnableReverb( bool enableReverb ) override;

        /**
         * @brief Find a sound event by name.
         *
         * @param name Name of the sound event to find.
         * @return Smart pointer to the sound event, or nullptr if not found.
         */
        SmartPtr<ISoundEvent> findSoundEvent( const String &name ) const;

        /**
         * @brief Get all sound events in the project.
         *
         * @return Array of all sound events.
         */
        Array<SmartPtr<ISoundEvent>> getSoundEvents() const;

        /**
         * @brief Remove all sound events from the project.
         */
        void removeAllSoundEvents();

        WP_CLASS_REGISTER_DECL;

    private:
        /// Map of named sound events
        using SoundEventMap = std::map<String, SmartPtr<ISoundEvent>>;
        SoundEventMap m_soundEvents;

        /// Master volume for the project (0.0 to 1.0)
        f32 m_masterVolume;

        /// Whether reverb is enabled
        bool m_enableReverb;

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        /// XAudio2 reverb effect (if enabled)
        IUnknown *m_reverbEffect;

        /// Reverb effect parameters
        XAUDIO2FX_REVERB_PARAMETERS m_reverbParams;
#    endif
#endif
    };

}  // namespace workphone

#endif  // __WPSoundProject_h__
