#ifndef __WP_SoundListener3_h__
#define __WP_SoundListener3_h__

#include <Workphone/Interface/Sound/ISoundListener3.hpp>

namespace workphone
{

    /** Sound listener.
     */
    class WPCore_API SoundListener3 : public ISoundListener3
    {
    public:
        /** Constructor. */
        SoundListener3();

        /** Destructor. */
        ~SoundListener3() override;

        /** @copydoc ISoundListener3::setPosition */
        void setPosition( const Vector3<real_Num> &position ) override;

        /** @copydoc ISoundListener3::getPosition */
        Vector3<real_Num> getPosition() const override;

        /** @copydoc ISoundListener3::setForwardVector */
        void setForwardVector( const Vector3<real_Num> &vector ) override;

        /** @copydoc ISoundListener3::getForwardVector */
        Vector3<real_Num> getForwardVector() const override;

        /** @copydoc ISoundListener3::setVelocity */
        void setVelocity( const Vector3<real_Num> &velocity ) override;

        /** @copydoc ISoundListener3::getVelocity */
        Vector3<real_Num> getVelocity() const override;

        WP_CLASS_REGISTER_DECL;

    private:
        /** Position. */
        Vector3<real_Num> m_position;

        /** Velocity. */
        Vector3<real_Num> m_velocity;

        /** Forward vector. */
        Vector3<real_Num> m_vector;
    };
}  // namespace workphone

#endif  // SoundListener3_h__
