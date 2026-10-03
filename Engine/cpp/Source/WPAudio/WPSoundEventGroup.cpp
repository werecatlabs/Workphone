#include <WPAudio/WPSoundEventGroup.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPSoundEventGroup, SoundEventGroup );

    WPSoundEventGroup::WPSoundEventGroup() : m_volume( 1.0f ), m_mute( false )
    {
    }

    WPSoundEventGroup::~WPSoundEventGroup()
    {
        removeAllEvents();
    }

    void WPSoundEventGroup::setVolume( f32 volume )
    {
        // Clamp volume to valid range
        m_volume = std::max( 0.0f, std::min( volume, 1.0f ) );

        // Update all events in the group if not muted
        if( !m_mute )
        {
            updateEvents();
        }
    }

    f32 WPSoundEventGroup::getVolume() const
    {
        return m_volume;
    }

    void WPSoundEventGroup::setMute( bool mute )
    {
        if( m_mute == mute )
        {
            return;
        }

        m_mute = mute;

        // Update all events in the group
        updateEvents();
    }

    bool WPSoundEventGroup::isMute() const
    {
        return m_mute;
    }

    void WPSoundEventGroup::addEvent( SmartPtr<ISoundEvent> event )
    {
        if( !event )
        {
            return;
        }

        // Check if the event is already in the group
        auto it = std::find( m_events.begin(), m_events.end(), event );
        if( it != m_events.end() )
        {
            return;  // Already added
        }

        m_events.push_back( event );

        // Apply current group settings to the new event
        if( m_mute )
        {
            event->setMute( true );
        }
        else
        {
            event->setVolume( event->getVolume() * m_volume );
        }
    }

    void WPSoundEventGroup::removeEvent( SmartPtr<ISoundEvent> event )
    {
        if( !event )
        {
            return;
        }

        auto it = std::find( m_events.begin(), m_events.end(), event );
        if( it != m_events.end() )
        {
            m_events.erase( it );
        }
    }

    void WPSoundEventGroup::removeAllEvents()
    {
        m_events.clear();
    }

    Array<SmartPtr<ISoundEvent>> WPSoundEventGroup::getEvents() const
    {
        return m_events;
    }

    u32 WPSoundEventGroup::getEventCount() const
    {
        return static_cast<u32>( m_events.size() );
    }

    void WPSoundEventGroup::startAll()
    {
        for( auto &event : m_events )
        {
            if( event )
            {
                event->start();
            }
        }
    }

    void WPSoundEventGroup::stopAll()
    {
        for( auto &event : m_events )
        {
            if( event )
            {
                event->stop();
            }
        }
    }

    void WPSoundEventGroup::updateEvents()
    {
        for( auto &event : m_events )
        {
            if( event )
            {
                if( m_mute )
                {
                    event->setMute( true );
                }
                else
                {
                    event->setMute( false );
                    // Note: In a real implementation, you might want to store the original
                    // event volume and multiply by the group volume. For now, we just
                    // apply the group volume directly.
                    event->setVolume( m_volume );
                }
            }
        }
    }

}  // namespace workphone
