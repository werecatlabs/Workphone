#ifndef SoundEvent_h__
#define SoundEvent_h__

#include <Workphone/Interface/Sound/ISoundEvent.hpp>

namespace workphone
{

    /**
     * @class SoundEvent
     * @brief Represents a sound event that can be played, stopped, and manipulated in 3D space.
     *
     * This class implements the ISoundEvent interface and provides control over the
     * playback state, volume, muting, 3D attributes, and custom sound event parameters.
     */
    class WPCore_API SoundEvent : public ISoundEvent
    {
    public:
        /**
         * @brief Constructs a new SoundEvent instance.
         */
        SoundEvent();

        /**
         * @brief Destroys the SoundEvent instance.
         */
        ~SoundEvent() override;

        /**
         * @brief Starts the playback of the sound event.
         */
        void start() override;

        /**
         * @brief Stops the playback of the sound event.
         */
        void stop() override;

        /**
         * @brief Checks if the sound event is currently playing.
         * @return True if playing, otherwise false.
         */
        bool isPlaying() const override;

        /**
         * @brief Sets the mute state of the sound event.
         * @param state True to mute, false to unmute.
         */
        void setMute( bool state ) override;

        /**
         * @brief Checks if the sound event is currently muted.
         * @return True if muted, otherwise false.
         */
        bool isMuted() const;

        /**
         * @brief Retrieves the current volume level.
         * @return The volume level (typically 0.0 to 1.0).
         */
        f32 getVolume() const override;

        /**
         * @brief Sets the volume level of the sound event.
         * @param volume The desired volume level.
         */
        void setVolume( f32 volume ) override;

        /**
         * @brief Retrieves a specific parameter associated with the sound event.
         * @param name The name of the parameter to retrieve.
         * @return A smart pointer to the sound event parameter, or nullptr if not found.
         */
        SmartPtr<ISoundEventParam> getParameter( const String &name ) override;

        /**
         * @brief Sets the 3D spatial attributes of the sound event.
         * @param pos The 3D position.
         * @param vel The 3D velocity.
         * @param ori The 3D orientation.
         */
        void set3DAttributes( Vector3<real_Num> pos, Vector3<real_Num> vel,
                              Quaternion<real_Num> ori ) override;

        /**
         * @brief Retrieves the 3D position of the sound event.
         * @return The current position vector.
         */
        Vector3<real_Num> getPosition() const;

        /**
         * @brief Retrieves the 3D velocity of the sound event.
         * @return The current velocity vector.
         */
        Vector3<real_Num> getVelocity() const;

        /**
         * @brief Retrieves the 3D orientation of the sound event.
         * @return The current orientation quaternion.
         */
        Quaternion<real_Num> getOrientation() const;

    private:
        bool m_isPlaying = false;
        bool m_isMuted = false;
        f32 m_volume = 1.0f;

        Vector3<real_Num> m_position = Vector3<real_Num>::zero();
        Vector3<real_Num> m_velocity = Vector3<real_Num>::zero();
        Quaternion<real_Num> m_orientation = Quaternion<real_Num>::identity();

        std::unordered_map<String, SmartPtr<ISoundEventParam>> m_parameters;
    };

}  // namespace workphone

#endif  // SoundEvent_h__
