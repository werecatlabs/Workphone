#ifndef __WPSoundEventGroup_h__
#define __WPSoundEventGroup_h__

#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Sound/SoundEventGroup.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    /**
     * @class WPSoundEventGroup
     * @brief Audio backend implementation of sound event groups.
     *
     * WPSoundEventGroup represents a logical grouping of sound events (e.g., "Music", "SFX", "UI").
     * It acts as a mixer bus or submix, allowing centralized volume and mute control for all events
     * in the group. This is useful for implementing audio categories where multiple sounds should
     * share common volume settings (e.g., muting all music, adjusting all SFX volume).
     *
     * Individual sound event volumes are modulated by the group volume, allowing fine-grained
     * control while respecting group-level settings.
     */
    class WPSoundEventGroup : public SoundEventGroup
    {
    public:
        /**
         * @brief Construct a new WPSoundEventGroup instance.
         */
        WPSoundEventGroup();

        /**
         * @brief Destroy the WPSoundEventGroup instance.
         */
        ~WPSoundEventGroup() override;

        /**
         * @brief Set the volume for all events in this group.
         * @param volume The group volume (0.0 to 1.0). Individual event volumes are multiplied by this value.
         */
        void setVolume( f32 volume ) override;

        /**
         * @brief Get the current group volume.
         * @return The group volume (0.0 to 1.0).
         */
        f32 getVolume() const;

        /**
         * @brief Set the mute state for all events in this group.
         * @param mute true to mute all events in the group, false to unmute.
         */
        void setMute( bool mute ) override;

        /**
         * @brief Get the current mute state of the group.
         * @return true if the group is muted, false otherwise.
         */
        bool isMute() const;

        /**
         * @brief Add a sound event to this group.
         * @param event The sound event to add.
         */
        void addEvent( SmartPtr<ISoundEvent> event );

        /**
         * @brief Remove a sound event from this group.
         * @param event The sound event to remove.
         */
        void removeEvent( SmartPtr<ISoundEvent> event );

        /**
         * @brief Remove all sound events from this group.
         */
        void removeAllEvents();

        /**
         * @brief Get all sound events in this group.
         * @return Array of sound events.
         */
        Array<SmartPtr<ISoundEvent>> getEvents() const;

        /**
         * @brief Get the number of events in this group.
         * @return The number of events.
         */
        u32 getEventCount() const;

        /**
         * @brief Start all events in this group.
         */
        void startAll();

        /**
         * @brief Stop all events in this group.
         */
        void stopAll();

        WP_CLASS_REGISTER_DECL;

    private:
        /**
         * @brief Update all events in the group with current volume/mute settings.
         */
        void updateEvents();

        /// The group volume multiplier (0.0 to 1.0)
        f32 m_volume = 1.0f;

        /// The group mute state
        bool m_mute = false;

        /// Collection of sound events in this group
        Array<SmartPtr<ISoundEvent>> m_events;
    };

}  // namespace workphone

#endif  // __WPSoundEventGroup_h__
