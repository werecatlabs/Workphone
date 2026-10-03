#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Sound/SoundEvent.hpp>
#include <Workphone/Sound/SoundEventParam.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>

namespace workphone
{

    SoundEvent::SoundEvent() = default;

    SoundEvent::~SoundEvent() = default;

    void SoundEvent::start()
    {
        m_isPlaying = true;
    }

    void SoundEvent::stop()
    {
        m_isPlaying = false;
    }

    bool SoundEvent::isPlaying() const
    {
        return m_isPlaying;
    }

    void SoundEvent::setMute( bool state )
    {
        m_isMuted = state;
    }

    bool SoundEvent::isMuted() const
    {
        return m_isMuted;
    }

    f32 SoundEvent::getVolume() const
    {
        return m_volume;
    }

    void SoundEvent::setVolume( f32 volume )
    {
        // Clamp volume to valid range [0.0, 1.0]
        m_volume = std::clamp( volume, 0.0f, 1.0f );
    }

    SmartPtr<ISoundEventParam> SoundEvent::getParameter( const String &name )
    {
        auto it = m_parameters.find( name );
        if( it != m_parameters.end() )
        {
            return it->second;
        }

        // Create a new parameter if it doesn't exist
        auto param = workphone::make_ptr<SoundEventParam>();
        m_parameters[name] = param;
        return param;
    }

    void SoundEvent::set3DAttributes( Vector3<real_Num> pos, Vector3<real_Num> vel,
                                      Quaternion<real_Num> ori )
    {
        m_position = pos;
        m_velocity = vel;
        m_orientation = ori;
    }

    Vector3<real_Num> SoundEvent::getPosition() const
    {
        return m_position;
    }

    Vector3<real_Num> SoundEvent::getVelocity() const
    {
        return m_velocity;
    }

    Quaternion<real_Num> SoundEvent::getOrientation() const
    {
        return m_orientation;
    }

}  // namespace workphone
