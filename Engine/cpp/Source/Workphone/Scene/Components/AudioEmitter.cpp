#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/AudioEmitter.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, AudioEmitter, Component );

    const String AudioEmitter::soundStr = String( "Sound" );
    const String AudioEmitter::playStr = String( "Play" );
    const String AudioEmitter::stopStr = String( "Stop" );
    const String AudioEmitter::pauseStr = String( "Pause" );
    const String AudioEmitter::unpauseStr = String( "Unpause" );

    AudioEmitter::AudioEmitter() = default;

    AudioEmitter::~AudioEmitter() = default;

    void AudioEmitter::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        static const auto componentName = String( "AudioEmitter" );
        setName( componentName );

        if( data )
        {
            Component::load( data );
        }
        else
        {
            WP_LOG_WARNING( String( "AudioEmitter::load() called with null data." ) );
            Component::load( nullptr );
        }

        setLoadingState( LoadingState::Loaded );
    }

    void AudioEmitter::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        if( data )
        {
            Component::unload( data );
        }
        else
        {
            WP_LOG_WARNING( String( "AudioEmitter::unload() called with null data." ) );
            Component::unload( nullptr );
        }

        setLoadingState( LoadingState::Unloaded );
    }

    auto AudioEmitter::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();
        if( !properties )
        {
            WP_LOG_ERROR( String(
                "AudioEmitter::getProperties() failed: Component::getProperties() returned nullptr." ) );
            return nullptr;
        }

        auto sound = getSound();
        properties->setPropertyAsType( soundStr, sound );

        properties->setButtonPressed( playStr, false );
        properties->setButtonPressed( stopStr, false );
        properties->setButtonPressed( pauseStr, false );
        properties->setButtonPressed( unpauseStr, false );

        return properties;
    }

    void AudioEmitter::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );
        if( !properties )
        {
            return;
        }

        auto sound = SmartPtr<ISound>();
        if( properties->getPropertyAsType( soundStr, sound ) )
        {
            setSound( sound );
        }

        if( properties->isButtonPressed( playStr ) )
        {
            play();
        }

        if( properties->isButtonPressed( stopStr ) )
        {
            stop();
        }

        if( properties->isButtonPressed( pauseStr ) )
        {
            pause();
        }

        if( properties->isButtonPressed( unpauseStr ) )
        {
            unpause();
        }
    }

    auto AudioEmitter::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto children = Array<SmartPtr<ISharedObject>>();

        auto sound = getSound();
        if( sound )
        {
            children.emplace_back( sound );
        }

        return children;
    }

    auto AudioEmitter::getSound() const -> SmartPtr<ISound>
    {
        return m_sound;
    }

    void AudioEmitter::setSound( SmartPtr<ISound> sound )
    {
        m_sound = sound;
    }

    void AudioEmitter::play()
    {
        if( auto sound = getSound() )
        {
            sound->play();
        }
        else
        {
            WP_LOG_WARNING( String( "AudioEmitter::play() called but no sound is assigned." ) );
        }
    }

    void AudioEmitter::stop()
    {
        if( auto sound = getSound() )
        {
            sound->stop();
        }
        else
        {
            WP_LOG_WARNING( String( "AudioEmitter::stop() called but no sound is assigned." ) );
        }
    }

    void AudioEmitter::pause()
    {
        if( auto sound = getSound() )
        {
            sound->pause();
        }
        else
        {
            WP_LOG_WARNING( String( "AudioEmitter::pause() called but no sound is assigned." ) );
        }
    }

    void AudioEmitter::unpause()
    {
        if( auto sound = getSound() )
        {
            sound->play();
        }
        else
        {
            WP_LOG_WARNING( String( "AudioEmitter::unpause() called but no sound is assigned." ) );
        }
    }
}  // namespace workphone::scene
