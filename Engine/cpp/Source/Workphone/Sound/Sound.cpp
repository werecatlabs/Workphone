#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Sound/Sound.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, Sound, Resource<ISound> );

    // Define static const property key strings
    const String Sound::saveStr = "Save";
    const String Sound::importStr = "Import";
    const String Sound::playStr = "Play";
    const String Sound::stopStr = "Stop";

    Sound::Sound() = default;

    Sound::~Sound() = default;

    void Sound::play()
    {
        m_isPlaying = true;
    }

    void Sound::pause()
    {
        m_isPlaying = false;
    }

    void Sound::stop()
    {
        m_isPlaying = false;
    }

    bool Sound::isPlaying() const
    {
        return m_isPlaying;
    }

    bool Sound::isValid() const
    {
        return isLoaded() && !StringUtil::isNullOrEmpty( getFilePath() );
    }

    void Sound::setVolume( f32 volume )
    {
        // Clamp volume to valid range [0.0, 1.0]
        m_volume = std::clamp( volume, 0.0f, 1.0f );
    }

    f32 Sound::getVolume() const
    {
        return m_volume;
    }

    void Sound::setLoop( bool loop )
    {
        setFlag( SOUND_FLAG_LOOP, loop );
    }

    bool Sound::getLoop() const
    {
        return ( m_flags & SOUND_FLAG_LOOP ) != 0;
    }

    void Sound::getSpectrum( Array<f32> &spectrum, u32 numValues ) const
    {
        spectrum.clear();
        spectrum.resize( numValues, 0.0f );
    }

    void Sound::setPan( f32 pan )
    {
        // Clamp pan to valid range [-1.0, 1.0]
        m_pan = std::clamp( pan, -1.0f, 1.0f );
    }

    f32 Sound::getPan() const
    {
        return m_pan;
    }

    void Sound::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
    }

    Vector3<real_Num> Sound::getPosition() const
    {
        return m_position;
    }

    void Sound::setMinMaxDistance( f32 minDistance, f32 maxDistance )
    {
        m_minDistance = minDistance;
        m_maxDistance = maxDistance;
    }

    void Sound::getMinMaxDistance( f32 &minDistance, f32 &maxDistance )
    {
        minDistance = m_minDistance;
        maxDistance = m_maxDistance;
    }

    SmartPtr<ISoundManager> Sound::getOwner() const
    {
        return m_owner;
    }

    void Sound::setOwner( SmartPtr<ISoundManager> owner )
    {
        m_owner = owner;
    }

    u32 Sound::getFlags() const
    {
        return m_flags;
    }

    void Sound::setFlags( u32 flags )
    {
        m_flags = flags;
    }

    void Sound::setFlag( u32 flags, bool value )
    {
        if( value )
        {
            m_flags |= flags;
        }
        else
        {
            m_flags &= ~flags;
        }
    }

    bool Sound::getFlag( u32 flags ) const
    {
        return ( m_flags & flags ) != 0;
    }

    SmartPtr<Properties> Sound::getProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();

        properties->setButtonPressed( Sound::saveStr, false );
        properties->setButtonPressed( Sound::importStr, false );

        properties->setButtonPressed( Sound::playStr, false );
        properties->setButtonPressed( Sound::stopStr, false );

        return properties;
    }

    void Sound::setProperties( SmartPtr<Properties> properties )
    {
        if( properties->isButtonPressed( Sound::playStr ) )
        {
            play();
        }

        if( properties->isButtonPressed( Sound::stopStr ) )
        {
            stop();
        }
    }

    Parameter Sound::handleEvent( EventType eventType, hash_type eventValue,
                                  const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                  SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        return {};
    }

    bool Sound::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

}  // namespace workphone
