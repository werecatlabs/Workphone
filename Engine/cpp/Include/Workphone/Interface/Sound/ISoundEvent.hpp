#ifndef ISoundEvent_h__
#define ISoundEvent_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Sound/ISoundEventParam.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{

    /**
     * @brief An interface for a sound event class.
     */
    class WPCore_API ISoundEvent : public ISharedObject
    {
    public:
        /**
         * @brief Destructor for ISoundEvent.
         */
        ~ISoundEvent() override;

        /**
         * @brief Starts playing the sound event.
         */
        virtual void start() = 0;

        /**
         * @brief Stops playing the sound event.
         */
        virtual void stop() = 0;

        /**
         * @brief Returns whether the sound event is currently playing.
         * @return True if the sound event is playing, false otherwise.
         */
        virtual bool isPlaying() const = 0;

        /**
         * @brief Sets whether the sound event is muted.
         * @param state The new muted state of the sound event.
         */
        virtual void setMute( bool state ) = 0;

        /**
         * @brief Gets the volume of the sound event.
         * @return The volume of the sound event.
         */
        virtual f32 getVolume() const = 0;

        /**
         * @brief Sets the volume of the sound event.
         * @param volume The new volume of the sound event.
         */
        virtual void setVolume( f32 volume ) = 0;

        /**
         * @brief Gets a sound event parameter by name.
         * @param name The name of the sound event parameter.
         * @return A shared pointer to the sound event parameter.
         */
        virtual SmartPtr<ISoundEventParam> getParameter( const String &name ) = 0;

        /**
         * @brief Sets the 3D attributes of the sound event.
         * @param pos The position of the sound event.
         * @param vel The velocity of the sound event.
         * @param ori The orientation of the sound event.
         */
        virtual void set3DAttributes( Vector3<real_Num> pos, Vector3<real_Num> vel,
                                      Quaternion<real_Num> ori ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // SoundEvent_h__
