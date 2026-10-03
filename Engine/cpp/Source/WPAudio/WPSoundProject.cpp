#include <WPAudio/WPSoundProject.hpp>
#include <WPAudio/WPSoundEvent.hpp>
#include <Workphone/Workphone.hpp>

#if defined WP_PLATFORM_WIN32
#    include <mmsystem.h>

#    if WP_USE_XAUDIO2
#        include <xaudio2.h>
#    elif WP_USE_WASAPI
#        include <Audioclient.h>
#        include <mmdeviceapi.h>
#    endif

#    include <windows.h>

#    pragma comment( lib, "xaudio2.lib" )
#elif defined WP_PLATFORM_APPLE
#    define WP_USE_WASAPI 0
#    define WP_USE_XAUDIO2 0

#    include <CoreAudio/CoreAudio.h>
#    include <AudioToolbox/AudioQueue.h>
#    include <AudioToolbox/AudioFile.h>
#    include <CoreFoundation/CoreFoundation.h>
#endif

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
#        include <xaudio2fx.h>
#        include <xapofx.h>
#    endif
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPSoundProject, SoundProject );

    WPSoundProject::WPSoundProject() :
        m_masterVolume( 1.0f ),
        m_enableReverb( false )
#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        ,
        m_reverbEffect( nullptr )
#    endif
#endif
    {
#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        // Initialize default reverb parameters
        memset( &m_reverbParams, 0, sizeof( m_reverbParams ) );
        m_reverbParams.WetDryMix = 50.0f;
        m_reverbParams.ReflectionsDelay = 5;
        m_reverbParams.ReverbDelay = 5;
        m_reverbParams.RearDelay = 5;
        m_reverbParams.PositionLeft = XAUDIO2FX_REVERB_DEFAULT_POSITION;
        m_reverbParams.PositionRight = XAUDIO2FX_REVERB_DEFAULT_POSITION;
        m_reverbParams.PositionMatrixLeft = XAUDIO2FX_REVERB_DEFAULT_POSITION_MATRIX;
        m_reverbParams.PositionMatrixRight = XAUDIO2FX_REVERB_DEFAULT_POSITION_MATRIX;
        m_reverbParams.EarlyDiffusion = 8;
        m_reverbParams.LateDiffusion = 8;
        m_reverbParams.LowEQGain = 8;
        m_reverbParams.LowEQCutoff = 4;
        m_reverbParams.HighEQGain = 8;
        m_reverbParams.HighEQCutoff = 6;
        m_reverbParams.RoomFilterFreq = 5000.0f;
        m_reverbParams.RoomFilterMain = -10.0f;
        m_reverbParams.RoomFilterHF = -1.0f;
        m_reverbParams.ReflectionsGain = -26.0f;
        m_reverbParams.ReverbGain = 10.0f;
        m_reverbParams.DecayTime = 1.0f;
        m_reverbParams.Density = 100.0f;
        m_reverbParams.RoomSize = XAUDIO2FX_REVERB_DEFAULT_ROOM_SIZE;
#    endif
#endif
    }

    WPSoundProject::~WPSoundProject()
    {
        removeAllSoundEvents();

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        if( m_reverbEffect )
        {
            m_reverbEffect->Release();
            m_reverbEffect = nullptr;
        }
#    endif
#endif
    }

    SmartPtr<ISoundEvent> WPSoundProject::addSoundEvent( const String &soundEventName )
    {
        // Check if event already exists
        auto it = m_soundEvents.find( soundEventName );
        if( it != m_soundEvents.end() )
        {
            // Event already exists, return existing
            return it->second;
        }

        // Create new sound event
        auto soundEvent = make_ptr<WPSoundEvent>();
        if( soundEvent )
        {
            // Set initial volume based on master volume
            soundEvent->setVolume( m_masterVolume );

            // Add to collection
            m_soundEvents[soundEventName] = soundEvent;
        }

        return soundEvent;
    }

    void WPSoundProject::removeSoundEvent( SmartPtr<ISoundEvent> soundEvent )
    {
        if( !soundEvent )
        {
            return;
        }

        // Find and remove the event
        for( auto it = m_soundEvents.begin(); it != m_soundEvents.end(); ++it )
        {
            if( it->second == soundEvent )
            {
                // Stop the event if playing
                if( soundEvent->isPlaying() )
                {
                    soundEvent->stop();
                }

                m_soundEvents.erase( it );
                break;
            }
        }
    }

    f32 WPSoundProject::getMasterVolume() const
    {
        return m_masterVolume;
    }

    void WPSoundProject::setMasterVolume( f32 masterVolume )
    {
        m_masterVolume = masterVolume;

        // Propagate master volume to all sound events
        for( auto &pair : m_soundEvents )
        {
            if( pair.second )
            {
                pair.second->setVolume( masterVolume );
            }
        }
    }

    void WPSoundProject::setupReverb()
    {
#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        if( m_reverbEffect )
        {
            // Reverb already set up
            return;
        }

        // Create XAudio2 reverb effect
        HRESULT hr = XAudio2CreateReverb( &m_reverbEffect );
        if( FAILED( hr ) )
        {
            m_reverbEffect = nullptr;
            return;
        }

        // Reverb effect created successfully
        // The effect can be applied to individual sound events or the master voice
#    endif
#endif
    }

    bool WPSoundProject::getEnableReverb() const
    {
        return m_enableReverb;
    }

    void WPSoundProject::setEnableReverb( bool enableReverb )
    {
        if( m_enableReverb == enableReverb )
        {
            return;
        }

        m_enableReverb = enableReverb;

        if( enableReverb )
        {
            // Set up reverb if not already done
            setupReverb();
        }

        // TODO: Apply or remove reverb effect from sound events or master voice
        // This would typically involve modifying the effect chain on the XAudio2 voice
    }

    SmartPtr<ISoundEvent> WPSoundProject::findSoundEvent( const String &name ) const
    {
        auto it = m_soundEvents.find( name );
        if( it != m_soundEvents.end() )
        {
            return it->second;
        }
        return nullptr;
    }

    Array<SmartPtr<ISoundEvent>> WPSoundProject::getSoundEvents() const
    {
        Array<SmartPtr<ISoundEvent>> events;
        events.reserve( m_soundEvents.size() );

        for( const auto &pair : m_soundEvents )
        {
            events.push_back( pair.second );
        }

        return events;
    }

    void WPSoundProject::removeAllSoundEvents()
    {
        // Stop all playing events
        for( auto &pair : m_soundEvents )
        {
            if( pair.second && pair.second->isPlaying() )
            {
                pair.second->stop();
            }
        }

        // Clear the collection
        m_soundEvents.clear();
    }

}  // namespace workphone
