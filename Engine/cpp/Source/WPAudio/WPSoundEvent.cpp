#include <WPAudio/WPSoundEvent.hpp>
#include <WPAudio/WPSoundEventParam.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPSoundEvent, SoundEvent );

    WPSoundEvent::WPSoundEvent() :
        m_volume( 1.0f ),
        m_mute( false ),
        m_isPlaying( false ),
        m_position( 0.0f, 0.0f, 0.0f ),
        m_velocity( 0.0f, 0.0f, 0.0f ),
        m_orientation( 1.0f, 0.0f, 0.0f, 0.0f )
    {
    }

    WPSoundEvent::~WPSoundEvent()
    {
        stop();
        removeAllSounds();
    }

    void WPSoundEvent::start()
    {
        if( m_isPlaying )
        {
            return;
        }

        m_isPlaying = true;

        // Start all associated sounds
        for( auto &sound : m_sounds )
        {
            if( sound && sound->isLoaded() )
            {
                sound->play();
            }
        }

        // Apply current event state to all sounds
        updateSounds();
    }

    void WPSoundEvent::stop()
    {
        if( !m_isPlaying )
        {
            return;
        }

        m_isPlaying = false;

        // Stop all associated sounds
        for( auto &sound : m_sounds )
        {
            if( sound )
            {
                sound->stop();
            }
        }
    }

    bool WPSoundEvent::isPlaying() const
    {
        // Check if any sound is playing
        for( const auto &sound : m_sounds )
        {
            if( sound && sound->isPlaying() )
            {
                return true;
            }
        }

        return false;
    }

    void WPSoundEvent::setMute( bool state )
    {
        if( m_mute == state )
        {
            return;
        }

        m_mute = state;
        updateSounds();
    }

    bool WPSoundEvent::isMute() const
    {
        return m_mute;
    }

    f32 WPSoundEvent::getVolume() const
    {
        return m_volume;
    }

    void WPSoundEvent::setVolume( f32 volume )
    {
        // Clamp volume to valid range
        m_volume = std::max( 0.0f, std::min( volume, 1.0f ) );

        if( !m_mute )
        {
            updateSounds();
        }
    }

    SmartPtr<ISoundEventParam> WPSoundEvent::getParameter( const String &name )
    {
        // Check if parameter already exists
        auto it = m_parameters.find( name );
        if( it != m_parameters.end() )
        {
            return it->second;
        }

        // Create new parameter
        auto param = make_ptr<WPSoundEventParam>();
        param->setName( name );
        m_parameters[name] = param;

        return param;
    }

    void WPSoundEvent::set3DAttributes( Vector3<real_Num> pos, Vector3<real_Num> vel,
                                        Quaternion<real_Num> ori )
    {
        m_position = pos;
        m_velocity = vel;
        m_orientation = ori;

        // Update 3D positions of all associated sounds
        for( auto &sound : m_sounds )
        {
            if( sound )
            {
                sound->setPosition( pos );
                // Note: Velocity and orientation would require extended sound interface
                // or could be used for advanced effects like doppler shift
            }
        }
    }

    Vector3<real_Num> WPSoundEvent::getPosition() const
    {
        return m_position;
    }

    Vector3<real_Num> WPSoundEvent::getVelocity() const
    {
        return m_velocity;
    }

    Quaternion<real_Num> WPSoundEvent::getOrientation() const
    {
        return m_orientation;
    }

    void WPSoundEvent::addSound( SmartPtr<ISound> sound )
    {
        if( !sound )
        {
            return;
        }

        // Check if sound is already added
        auto it = std::find( m_sounds.begin(), m_sounds.end(), sound );
        if( it != m_sounds.end() )
        {
            return;  // Already added
        }

        m_sounds.push_back( sound );

        // Apply current event state to the new sound
        if( sound->isLoaded() )
        {
            sound->setVolume( m_mute ? 0.0f : m_volume );
            sound->setPosition( m_position );

            if( m_isPlaying )
            {
                sound->play();
            }
        }
    }

    void WPSoundEvent::removeSound( SmartPtr<ISound> sound )
    {
        if( !sound )
        {
            return;
        }

        auto it = std::find( m_sounds.begin(), m_sounds.end(), sound );
        if( it != m_sounds.end() )
        {
            // Stop the sound before removing
            if( ( *it )->isPlaying() )
            {
                ( *it )->stop();
            }

            m_sounds.erase( it );
        }
    }

    void WPSoundEvent::removeAllSounds()
    {
        // Stop all sounds before clearing
        for( auto &sound : m_sounds )
        {
            if( sound && sound->isPlaying() )
            {
                sound->stop();
            }
        }

        m_sounds.clear();
    }

    Array<SmartPtr<ISound>> WPSoundEvent::getSounds() const
    {
        return m_sounds;
    }

    void WPSoundEvent::setEventGroup( SmartPtr<ISoundEventGroup> group )
    {
        m_eventGroup = group;
    }

    SmartPtr<ISoundEventGroup> WPSoundEvent::getEventGroup() const
    {
        return m_eventGroup;
    }

    void WPSoundEvent::updateSounds()
    {
        for( auto &sound : m_sounds )
        {
            if( sound )
            {
                // Apply mute state
                if( m_mute )
                {
                    sound->setVolume( 0.0f );
                }
                else
                {
                    // Apply event volume
                    sound->setVolume( m_volume );
                }

                // Apply 3D position
                sound->setPosition( m_position );
            }
        }
    }

}  // namespace workphone
