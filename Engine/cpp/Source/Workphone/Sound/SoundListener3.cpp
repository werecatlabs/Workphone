#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Sound/SoundListener3.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, SoundListener3, ISoundListener3 );

    SoundListener3::SoundListener3() = default;

    SoundListener3::~SoundListener3() = default;

    void SoundListener3::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
    }

    Vector3<real_Num> SoundListener3::getPosition() const
    {
        return m_position;
    }

    void SoundListener3::setForwardVector( const Vector3<real_Num> &vector )
    {
        m_vector = vector;
    }

    Vector3<real_Num> SoundListener3::getForwardVector() const
    {
        return m_vector;
    }

    void SoundListener3::setVelocity( const Vector3<real_Num> &velocity )
    {
        m_velocity = velocity;
    }

    Vector3<real_Num> SoundListener3::getVelocity() const
    {
        return m_velocity;
    }

}  // namespace workphone
