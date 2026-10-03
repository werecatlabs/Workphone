#ifndef __WPSoundEvent_h__
#define __WPSoundEvent_h__

#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Sound/SoundEvent.hpp>
#include <Workphone/Core/Array.hpp>
#include <map>

namespace workphone
{
    /**
     * @class WPSoundEvent
     * @brief Audio backend implementation of sound events.
     *
     * WPSoundEvent represents a high-level sound abstraction that encapsulates one or more
     * sound instances along with runtime parameters. Similar to FMOD Studio events, this
     * allows for complex sound behaviors controlled through named parameters (e.g., RPM,
     * speed, intensity).
     *
     * Events differ from basic sounds in that they:
     * - Can control multiple sound layers simultaneously
     * - Support runtime parameters that modify playback (pitch, volume, filters)
     * - Encapsulate 3D positioning with velocity and orientation
     * - Can be grouped for bulk volume/mute control
     *
     * This implementation provides a flexible foundation for implementing game audio events
     * without requiring external audio middleware like FMOD or Wwise.
     */
    class WPSoundEvent : public SoundEvent
    {
    public:
        /**
         * @brief Construct a new WPSoundEvent instance.
         */
        WPSoundEvent();

        /**
         * @brief Destroy the WPSoundEvent instance.
         */
        ~WPSoundEvent() override;

        /**
         * @brief Start playing the sound event.
         *
         * This triggers playback of all associated sounds and applies current parameter values.
         */
        void start() override;

        /**
         * @brief Stop playing the sound event.
         *
         * This stops all associated sounds and resets the playback state.
         */
        void stop() override;

        /**
         * @brief Check if the sound event is currently playing.
         * @return true if any associated sound is playing, false otherwise.
         */
        bool isPlaying() const override;

        /**
         * @brief Set the mute state of the event.
         * @param state true to mute, false to unmute.
         */
        void setMute( bool state ) override;

        /**
         * @brief Get the current mute state.
         * @return true if muted, false otherwise.
         */
        bool isMute() const;

        /**
         * @brief Get the current volume of the event.
         * @return Volume level (0.0 to 1.0).
         */
        f32 getVolume() const override;

        /**
         * @brief Set the volume of the event.
         * @param volume Volume level (0.0 to 1.0).
         */
        void setVolume( f32 volume ) override;

        /**
         * @brief Get a parameter by name, creating it if it doesn't exist.
         * @param name The parameter name (e.g., "RPM", "Speed", "Intensity").
         * @return Smart pointer to the parameter.
         */
        SmartPtr<ISoundEventParam> getParameter( const String &name ) override;

        /**
         * @brief Set the 3D attributes of the event.
         * @param pos Position in 3D space.
         * @param vel Velocity vector for doppler effect.
         * @param ori Orientation quaternion.
         */
        void set3DAttributes( Vector3<real_Num> pos, Vector3<real_Num> vel,
                              Quaternion<real_Num> ori ) override;

        /**
         * @brief Get the 3D position of the event.
         * @return Position vector.
         */
        Vector3<real_Num> getPosition() const;

        /**
         * @brief Get the 3D velocity of the event.
         * @return Velocity vector.
         */
        Vector3<real_Num> getVelocity() const;

        /**
         * @brief Get the 3D orientation of the event.
         * @return Orientation quaternion.
         */
        Quaternion<real_Num> getOrientation() const;

        /**
         * @brief Add a sound instance to this event.
         * @param sound The sound to add.
         */
        void addSound( SmartPtr<ISound> sound );

        /**
         * @brief Remove a sound instance from this event.
         * @param sound The sound to remove.
         */
        void removeSound( SmartPtr<ISound> sound );

        /**
         * @brief Remove all sound instances from this event.
         */
        void removeAllSounds();

        /**
         * @brief Get all sounds associated with this event.
         * @return Array of sound instances.
         */
        Array<SmartPtr<ISound>> getSounds() const;

        /**
         * @brief Set the event group this event belongs to.
         * @param group The event group.
         */
        void setEventGroup( SmartPtr<ISoundEventGroup> group );

        /**
         * @brief Get the event group this event belongs to.
         * @return The event group, or nullptr if not in a group.
         */
        SmartPtr<ISoundEventGroup> getEventGroup() const;

        WP_CLASS_REGISTER_DECL;

    private:
        /**
         * @brief Update all sounds with current event state (volume, mute, position).
         */
        void updateSounds();

        /// Volume level (0.0 to 1.0)
        f32 m_volume = 1.0f;

        /// Mute state
        bool m_mute = false;

        /// Playing state
        bool m_isPlaying = false;

        /// 3D position
        Vector3<real_Num> m_position;

        /// 3D velocity (for doppler effect)
        Vector3<real_Num> m_velocity;

        /// 3D orientation
        Quaternion<real_Num> m_orientation;

        /// Collection of sound instances controlled by this event
        Array<SmartPtr<ISound>> m_sounds;

        /// Map of named parameters (e.g., "RPM" -> parameter object)
        using ParameterMap = std::map<String, SmartPtr<ISoundEventParam>>;
        ParameterMap m_parameters;

        /// Optional event group this event belongs to
        SmartPtr<ISoundEventGroup> m_eventGroup;
    };

}  // namespace workphone

#endif  // __WPSoundEvent_h__
