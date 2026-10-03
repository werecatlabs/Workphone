#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Sound/SoundProject.hpp>
#include <Workphone/Sound/SoundEvent.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Sound/ISoundEvent.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SoundProject, ISoundProject );

    SoundProject::SoundProject() = default;

    SoundProject::~SoundProject()
    {
        // Clean up all sound events
        m_soundEvents.clear();
    }

    SmartPtr<ISoundEvent> SoundProject::addSoundEvent( const String &soundEventName )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        // Check if event with this name already exists
        auto it = m_soundEventMap.find( soundEventName );
        if( it != m_soundEventMap.end() )
        {
            return it->second;
        }

        // Create a new sound event using the factory
        if( factoryManager )
        {
            auto soundEvent = factoryManager->make_object<ISoundEvent>();
            if( soundEvent )
            {
                soundEvent->setName( soundEventName );
                m_soundEvents.push_back( soundEvent );
                m_soundEventMap[soundEventName] = soundEvent;
                return soundEvent;
            }
        }

        return nullptr;
    }

    void SoundProject::removeSoundEvent( SmartPtr<ISoundEvent> soundEvent )
    {
        if( !soundEvent )
        {
            return;
        }

        // Remove from map
        auto name = soundEvent->getName();
        m_soundEventMap.erase( name );

        // Remove from vector
        m_soundEvents.erase( std::remove( m_soundEvents.begin(), m_soundEvents.end(), soundEvent ),
                             m_soundEvents.end() );
    }

    f32 SoundProject::getMasterVolume() const
    {
        return m_masterVolume;
    }

    void SoundProject::setMasterVolume( f32 masterVolume )
    {
        // Clamp volume to valid range [0.0, 1.0]
        m_masterVolume = std::clamp( masterVolume, 0.0f, 1.0f );

        // Apply master volume to all sound events
        for( auto &soundEvent : m_soundEvents )
        {
            if( soundEvent )
            {
                soundEvent->setVolume( soundEvent->getVolume() * m_masterVolume );
            }
        }
    }

    void SoundProject::setupReverb()
    {
        // Initialize reverb settings with default values
        m_reverbEnabled = true;
    }

    bool SoundProject::getEnableReverb() const
    {
        return m_reverbEnabled;
    }

    void SoundProject::setEnableReverb( bool enableReverb )
    {
        m_reverbEnabled = enableReverb;
    }

    const std::vector<SmartPtr<ISoundEvent>> &SoundProject::getSoundEvents() const
    {
        return m_soundEvents;
    }

    SmartPtr<ISoundEvent> SoundProject::findSoundEvent( const String &name ) const
    {
        auto it = m_soundEventMap.find( name );
        return ( it != m_soundEventMap.end() ) ? it->second : nullptr;
    }

}  // namespace workphone
