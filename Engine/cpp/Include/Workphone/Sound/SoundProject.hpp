#ifndef SoundProject_h__
#define SoundProject_h__

#include <Workphone/Interface/Sound/ISoundProject.hpp>
#include <unordered_map>

namespace workphone
{
    /**
     * @class SoundProject
     * @brief Manages the overall sound project, including sound events, master volume, and global
     * effects.
     *
     * This class implements the ISoundProject interface and acts as a container for all sound events
     * within a project, providing global controls for audio output and reverb settings.
     */
    class WPCore_API SoundProject : public ISoundProject
    {
    public:
        /**
         * @brief Constructs a new SoundProject instance.
         */
        SoundProject();

        /**
         * @brief Destroys the SoundProject instance.
         */
        ~SoundProject() override;

        /**
         * @brief Adds a new sound event to the project.
         * @param soundEventName The unique name of the sound event to create.
         * @return A smart pointer to the created ISoundEvent.
         */
        SmartPtr<ISoundEvent> addSoundEvent( const String &soundEventName ) override;

        /**
         * @brief Removes an existing sound event from the project.
         * @param soundEvent Smart pointer to the sound event to be removed.
         */
        void removeSoundEvent( SmartPtr<ISoundEvent> soundEvent ) override;

        /**
         * @brief Retrieves the current master volume level.
         * @return The master volume (typically between 0.0 and 1.0).
         */
        f32 getMasterVolume() const override;

        /**
         * @brief Sets the master volume level for the project.
         * @param masterVolume The desired master volume.
         */
        void setMasterVolume( f32 masterVolume ) override;

        /**
         * @brief Configures the global reverb settings for the project.
         */
        void setupReverb() override;

        /**
         * @brief Checks if global reverb is currently enabled.
         * @return True if reverb is enabled, otherwise false.
         */
        bool getEnableReverb() const override;

        /**
         * @brief Enables or disables global reverb.
         * @param enableReverb True to enable reverb, false to disable.
         */
        void setEnableReverb( bool enableReverb ) override;

        /**
         * @brief Retrieves all sound events managed by this project.
         * @return A constant reference to a vector of smart pointers to sound events.
         */
        const std::vector<SmartPtr<ISoundEvent>> &getSoundEvents() const;

        /**
         * @brief Finds a specific sound event within the project by its name.
         * @param name The name of the sound event to search for.
         * @return A smart pointer to the found ISoundEvent, or nullptr if not found.
         */
        SmartPtr<ISoundEvent> findSoundEvent( const String &name ) const;

        WP_CLASS_REGISTER_DECL;

    private:
        std::vector<SmartPtr<ISoundEvent>> m_soundEvents;
        std::unordered_map<String, SmartPtr<ISoundEvent>> m_soundEventMap;
        f32 m_masterVolume = 1.0f;
        bool m_reverbEnabled = false;
    };
}  // namespace workphone

#endif  // SoundProject_h__
