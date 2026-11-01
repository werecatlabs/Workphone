#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/AudioEmitter.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <stdexcept>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, AudioEmitter, Component );

    const String AudioEmitter::soundStr = String( "Sound" );
    const String AudioEmitter::playStr = String( "Play" );
    const String AudioEmitter::stopStr = String( "Stop" );
    const String AudioEmitter::pauseStr = String( "Pause" );
    const String AudioEmitter::unpauseStr = String( "Unpause" );

    AudioEmitter::AudioEmitter() = default;

    void AudioEmitter::configureRuntimeAssets(std::shared_ptr<const resource::RuntimeAssetContext> context)
    {
        if(!context||!context->resolve||isLoaded()||getLoadingState()==LoadingState::Loading||
            getLoadingState()==LoadingState::Unloading||(m_runtimeAssets&&m_runtimeAssets!=context))
            throw std::logic_error("AudioEmitter runtime assets require an unloaded component and captured resolver");
        m_runtimeAssets=std::move(context);m_runtimeAssetsMode=true;m_runtimeOwner=std::this_thread::get_id();
    }
    void AudioEmitter::requireRuntimeOwner() const {if(m_runtimeAssetsMode&&m_runtimeOwner!=std::this_thread::get_id())throw std::logic_error("Runtime AudioEmitter requires its audio control owner");}
    String AudioEmitter::getRuntimeDiagnostic() const {return m_runtimeDiagnostic;}

    AudioEmitter::~AudioEmitter()
    {
        releaseInstance();
    }

    void AudioEmitter::releaseInstance()
    {
        if( auto instance = m_instance )
        {
            m_instance = nullptr;
            instance->stop();
            if( auto owner = instance->getOwner() )
            {
                owner->destroyResource( instance );
            }
            else
            {
                instance->unload( nullptr );
            }
        }
    }

    void AudioEmitter::load( SmartPtr<ISharedObject> data )
    {
        requireRuntimeOwner();
        if(m_runtimeAssetsMode&&!m_runtimeAssets)throw std::logic_error("Runtime AudioEmitter requires reconfiguration before load");
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
        requireRuntimeOwner();
        try{setLoadingState( LoadingState::Unloading );}catch(const std::bad_alloc&){}
        releaseInstance();
        if(m_runtimeAssetsMode)m_sound=nullptr;
        m_runtimeResource.reset();m_runtimeAssets.reset();

        if( data )
        {
            Component::unload( data );
        }
        else
        {
            WP_LOG_WARNING( String( "AudioEmitter::unload() called with null data." ) );
            Component::unload( nullptr );
        }

        try{setLoadingState( LoadingState::Unloaded );}catch(const std::bad_alloc&){}
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
        if(m_runtimeAssetsMode)properties->setProperty(soundStr,m_runtimeSoundUuid,Properties::resourceStr);
        else properties->setPropertyAsType( soundStr, sound );
        if(m_runtimeAssetsMode)properties->setProperty("runtimeAudioDiagnostic",m_runtimeDiagnostic,true);

        properties->setButtonPressed( playStr, false );
        properties->setButtonPressed( stopStr, false );
        properties->setButtonPressed( pauseStr, false );
        properties->setButtonPressed( unpauseStr, false );

        return properties;
    }

    void AudioEmitter::setProperties( SmartPtr<Properties> properties )
    {
        requireRuntimeOwner();
        if( !properties )
        {
            return;
        }

        if(m_runtimeAssetsMode){
            if(!m_runtimeAssets)throw std::logic_error("Runtime AudioEmitter has no captured asset context");
            String uuid=m_runtimeSoundUuid,error;properties->getPropertyValue(soundStr,uuid);
            auto resource=m_runtimeAssets->resolve(uuid,error);
            if(!resource)throw std::runtime_error(error.empty()?"Runtime AudioEmitter requires a captured Sound UUID":error.c_str());
#if WP_TYPED_ANIMATION_RESOURCE
            if(resource->header.resourceType!=resource::ResourceTypeID("wav"))throw std::runtime_error("AudioEmitter Sound UUID requires typed wav");
#else
            throw std::runtime_error("Cooked audio resources are unavailable in this build");
#endif
            if(resource!=m_runtimeResource){
                auto app=core::IApplicationManager::instancePtr();auto manager=app?app->getSoundManager():nullptr;
                auto candidate=manager?manager->createRuntimeSound(resource,error,m_runtimeAssets->mount):SmartPtr<ISound>();
                if(!candidate){m_runtimeDiagnostic=error.empty()?"Runtime audio output manager/device is unavailable":error;throw std::runtime_error(m_runtimeDiagnostic.c_str());}
                struct RetireCandidate {SmartPtr<ISoundManager> manager;SmartPtr<ISound> sound;bool committed=false;
                    ~RetireCandidate(){if(!committed&&sound)try{manager->destroyResource(sound);}catch(...){try{sound->unload(nullptr);sound->setOwner(nullptr);}catch(...){}}}
                } cleanup{manager,candidate};
                releaseInstance();m_sound=candidate;m_instance=candidate;
                m_runtimeResource=std::move(resource);m_runtimeSoundUuid=std::move(uuid);m_runtimeDiagnostic.clear();
                cleanup.committed=true;
            }
            Component::setProperties(properties);
        }else {
            Component::setProperties(properties);

        auto sound = SmartPtr<ISound>();
        if( properties->getPropertyAsType( soundStr, sound ) )
        {
            setSound( sound );
        }
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
        if(m_runtimeAssetsMode)throw std::logic_error("Runtime AudioEmitter Sound is controlled by its captured UUID");
        if( m_sound != sound )
        {
            releaseInstance();
        }
        m_sound = sound;
    }

    void AudioEmitter::play()
    {
        requireRuntimeOwner();
        if(m_runtimeAssetsMode&&!m_instance)throw std::logic_error("Runtime AudioEmitter has no installed native voice");
        if( !m_instance && m_sound )
        {
            if( auto owner = m_sound->getOwner() )
            {
                m_instance =
                    workphone::dynamic_pointer_cast<ISound>( owner->cloneResource( m_sound, String() ) );
            }
            if( !m_instance )
            {
                WP_LOG_WARNING( "AudioEmitter: could not create an independent playback instance." );
            }
        }
        if( m_instance )
        {
            m_instance->play();
        }
    }

    void AudioEmitter::stop()
    {
        requireRuntimeOwner();
        if( m_instance )
        {
            m_instance->stop();
        }
    }

    void AudioEmitter::pause()
    {
        requireRuntimeOwner();
        if( m_instance )
        {
            m_instance->pause();
        }
    }

    void AudioEmitter::unpause()
    {
        requireRuntimeOwner();
        if( m_instance )
        {
            m_instance->play();
        }
    }
    bool AudioEmitter::seekFrames(u64 frame,String &error)
    {requireRuntimeOwner();if(!m_instance){error="AudioEmitter has no independent playback instance";return false;}return m_instance->seekFrames(frame,error);}
}  // namespace workphone::scene
