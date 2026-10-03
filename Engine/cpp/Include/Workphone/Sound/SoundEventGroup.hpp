#ifndef SoundEventGroup_h__
#define SoundEventGroup_h__

#include <Workphone/Interface/Sound/ISoundEventGroup.hpp>

namespace workphone
{

    /**
     * @class SoundEventGroup
     * @brief A grouping of sound events that can be controlled collectively.
     *
     * This class implements the ISoundEventGroup interface, allowing multiple sound events
     * to share common properties like volume and mute state.
     */
    class WPCore_API SoundEventGroup : public ISoundEventGroup
    {
    public:
        /**
         * @brief Constructs a new SoundEventGroup instance.
         */
        SoundEventGroup();

        /**
         * @brief Destroys the SoundEventGroup instance.
         */
        ~SoundEventGroup() override;

        /**
         * @brief Sets the volume level for all sound events in this group.
         * @param volume The volume level (typically between 0.0 and 1.0).
         */
        void setVolume( f32 volume ) override;

        /**
         * @brief Sets the mute state for all sound events in this group.
         * @param mute True to mute the group, false to unmute.
         */
        void setMute( bool mute ) override;

        /**
         * @brief Retrieves the current volume level of the group.
         * @return The current volume level.
         */
        f32 getVolume() const;

        /**
         * @brief Checks if the sound event group is currently muted.
         * @return True if muted, otherwise false.
         */
        bool isMuted() const;

    private:
        f32 m_volume = 1.0f;     ///< Volume level [0.0, 1.0], default is full volume.
        bool m_isMuted = false;  ///< Mute state, default is unmuted.
    };

}  // namespace workphone

#endif  // SoundEventGroup_h__
