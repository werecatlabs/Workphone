#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/SoundResourceDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, SoundResourceDirector, Director );

    const String SoundResourceDirector::playStr = "Play";
    const String SoundResourceDirector::stopStr = "Stop";
    const String SoundResourceDirector::saveStr = "Save";
    const String SoundResourceDirector::importStr = "Import";

    SoundResourceDirector::SoundResourceDirector() = default;

    SoundResourceDirector::~SoundResourceDirector() = default;

    void SoundResourceDirector::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        ResourceDirector::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void SoundResourceDirector::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        auto applicationManager = core::IApplicationManager::instance();
        auto soundManager = applicationManager->getSoundManager();
        if( soundManager )
        {
            if( auto sound = getSound() )
            {
                soundManager->destroyResource( sound );
            }
        }

        ResourceDirector::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<Properties> SoundResourceDirector::getProperties() const
    {
        auto properties = ResourceDirector::getProperties();

        properties->setButtonPressed( playStr, false );
        properties->setButtonPressed( stopStr, false );

        properties->setButtonPressed( saveStr, false );
        properties->setButtonPressed( importStr, false );

        return properties;
    }

    void SoundResourceDirector::setProperties( SmartPtr<Properties> properties )
    {
        ResourceDirector::setProperties( properties );

        if( properties->isButtonPressed( playStr ) )
        {
            play();
        }

        if( properties->isButtonPressed( stopStr ) )
        {
            stop();
        }
    }

    void SoundResourceDirector::play()
    {
        auto sound = getSound();
        if( !sound )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto soundManager = applicationManager->getSoundManager();
            if( soundManager )
            {
                auto resourcePath = getResourcePath();
                sound = soundManager->loadResourceByType<ISound>( resourcePath );

                setSound( sound );
            }
            else
            {
                WP_LOG_ERROR( "SoundManager is not found" );
            }
        }

        if( sound )
        {
            sound->play();
        }
    }

    void SoundResourceDirector::stop()
    {
        if( auto sound = getSound() )
        {
            sound->stop();
        }
    }

    void SoundResourceDirector::setSound( SmartPtr<ISound> sound )
    {
        m_sound = sound;
    }

    SmartPtr<ISound> SoundResourceDirector::getSound() const
    {
        return m_sound;
    }
}  // namespace workphone::scene
