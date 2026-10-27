#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Sound/SoundManager.hpp>
#include <Workphone/Sound/Sound.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/Interface/Sound/ISoundListener3.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/State/States/SoundManagerStateData.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <cmath>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SoundManager, ISoundManager );

    SoundManager::SoundManager() = default;

    SoundManager::~SoundManager()
    {
        destroyAll();
    }

    SmartPtr<Properties> SoundManager::getProperties() const
    {
        auto result = workphone::make_ptr<Properties>();
        result->setProperty( "sound.masterVolume", s32( getVolume() * 100.0f + 0.5f ) );
        result->setProperty( "sound.musicVolume", s32( m_musicGain * 100.0f + 0.5f ) );
        result->setProperty( "sound.sfxVolume", s32( m_sfxGain * 100.0f + 0.5f ) );
        result->setProperty( "sound.voiceVolume", s32( m_voiceGain * 100.0f + 0.5f ) );
        result->setProperty( "sound.sampleRate", m_playbackSampleRate );
        result->setProperty( "sound.bufferSize", m_playbackBufferFrames );
        result->setProperty( "sound.voiceLimit", m_voiceLimit );
        result->setProperty( "sound.backend", m_audioBackend );
        result->setProperty( "sound.outputDevice", m_outputDevice );
        result->setProperty( "sound.spatialization", m_spatializationEnabled );
        result->setProperty( "sound.streaming", m_streamingEnabled );
        result->setProperty( "sound.occlusion", m_occlusionEnabled );
        return result;
    }

    void SoundManager::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties ) return;
        s32 volume = 0;
        if( properties->getPropertyValue( "sound.masterVolume", volume ) )
            setVolume( std::clamp( volume, 0, 100 ) / 100.0f );
        if( properties->getPropertyValue( "sound.musicVolume", volume ) )
            m_musicGain = std::clamp( volume, 0, 100 ) / 100.0f;
        if( properties->getPropertyValue( "sound.sfxVolume", volume ) )
            m_sfxGain = std::clamp( volume, 0, 100 ) / 100.0f;
        if( properties->getPropertyValue( "sound.voiceVolume", volume ) )
            m_voiceGain = std::clamp( volume, 0, 100 ) / 100.0f;
        properties->getPropertyValue( "sound.sampleRate", m_playbackSampleRate );
        properties->getPropertyValue( "sound.bufferSize", m_playbackBufferFrames );
        properties->getPropertyValue( "sound.voiceLimit", m_voiceLimit );
        properties->getPropertyValue( "sound.backend", m_audioBackend );
        properties->getPropertyValue( "sound.outputDevice", m_outputDevice );
        properties->getPropertyValue( "sound.spatialization", m_spatializationEnabled );
        properties->getPropertyValue( "sound.streaming", m_streamingEnabled );
        properties->getPropertyValue( "sound.occlusion", m_occlusionEnabled );
        m_voiceLimit = std::clamp( m_voiceLimit, 1u, 512u );
        m_playbackSampleRate = std::clamp( m_playbackSampleRate, 22050u, 96000u );
        m_playbackBufferFrames = std::clamp( m_playbackBufferFrames, 128u, 4096u );
        for( auto &sound : m_sounds.snapshot() )
            if( sound && sound->isLoaded() ) sound->setVolume( sound->getVolume() );
    }

    bool SoundManager::admitVoice( const ISound *sound ) const
    {
        u32 active = 0;
        for( const auto &candidate : m_sounds.snapshot() )
            if( candidate && candidate.get() != sound && candidate->isPlaying() ) ++active;
        return active < m_voiceLimit;
    }

    void SoundManager::getMixParameters( ISound *sound, f32 &gain, f32 &pan ) const
    {
        if( !sound ) { gain = pan = 0; return; }
        gain = sound->getVolume();
        auto concrete = dynamic_cast<Sound *>( sound );
        pan = concrete ? concrete->getPan() : 0.0f;
        if( concrete )
            gain *= concrete->getCategory() == "music" ? m_musicGain :
                    concrete->getCategory() == "voice" ? m_voiceGain : m_sfxGain;
        if( sound->getFlag( ISound::SOUND_FLAG_MUTE ) ) gain = 0.0f;
        const auto listeners = m_listeners.snapshot();
        if( !m_spatializationEnabled || !sound->getFlag( ISound::SOUND_FLAG_3D ) || listeners.empty() )
            return;
        const auto listener = listeners.front();
        const auto offset = sound->getPosition() - listener->getPosition();
        const auto distance = static_cast<f32>( offset.length() );
        f32 minimum = 1.0f, maximum = 10000.0f;
        sound->getMinMaxDistance( minimum, maximum );
        maximum = std::max( maximum, minimum + 0.001f );
        gain *= std::clamp( 1.0f - ( distance - minimum ) / ( maximum - minimum ), 0.0f, 1.0f );
        if( distance > 0.001f )
        {
            const auto right = listener->getForwardVector().crossProduct( Vector3<real_Num>::unitY() ).normaliseCopy();
            pan = std::clamp( static_cast<f32>( offset.dotProduct( right ) / distance ), -1.0f, 1.0f );
        }
        if( m_occlusionEnabled && distance > 0.1f )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto physics = app ? app->getPhysicsManager() : nullptr;
            auto scene = physics ? physics->getPhysicsScene() : nullptr;
            if( scene )
            {
                ScopedLock lock( scene.get() );
                Vector3<real_Num> point, normal;
                SmartPtr<ISharedObject> object;
                if( scene->intersects( listener->getPosition(), sound->getPosition(), point, normal, object ) &&
                    ( point - listener->getPosition() ).length() < distance - 0.05f ) gain *= 0.25f;
            }
        }
    }

    void SoundManager::load( SmartPtr<ISharedObject> data )
    {
        if( auto app = core::IApplicationManager::instancePtr() ) setProperties( app->getPlayerSettings() );
        m_sounds.reserve( 100 );
    }

    void SoundManager::unload( SmartPtr<ISharedObject> data )
    {
        destroyAll();
    }

    void SoundManager::update()
    {
        if( !m_loadQueue.empty() )
        {
            SmartPtr<ISharedObject> sound;
            while( m_loadQueue.try_pop( sound ) )
            {
                auto instance = workphone::dynamic_pointer_cast<ISound>( sound );
                if( !instance || instance->getOwner().get() == this )
                {
                    sound->load( nullptr );
                }
            }
        }

        if( !m_unloadQueue.empty() )
        {
            SmartPtr<ISharedObject> sound;
            while( m_unloadQueue.try_pop( sound ) )
            {
                sound->unload( nullptr );
            }
        }
        for( auto sound : m_sounds.snapshot() )
            if( sound && sound->isPlaying() ) sound->setVolume( sound->getVolume() );
    }

    void SoundManager::destroyResource( SmartPtr<IResource> resource )
    {
        if( auto sound = workphone::dynamic_pointer_cast<ISound>( resource ) )
        {
            const auto registered = m_sounds.snapshot();
            if( std::find( registered.begin(), registered.end(), sound ) == registered.end() )
            {
                return;
            }
            sound->unload( nullptr );
            sound->setOwner( nullptr );
            m_sounds.erase( std::remove( m_sounds.begin(), m_sounds.end(), sound ), m_sounds.end() );
        }
    }

    void SoundManager::destroyAll()
    {
        SmartPtr<ISharedObject> pending;
        while( m_loadQueue.try_pop( pending ) )
        {
            pending->unload( nullptr );
        }
        while( m_unloadQueue.try_pop( pending ) )
        {
            pending->unload( nullptr );
        }
        auto sounds = m_sounds.snapshot();
        for( auto &sound : sounds )
        {
            if( sound )
            {
                sound->unload( nullptr );
                sound->setOwner( nullptr );
            }
        }

        m_sounds.clear();
        m_listeners.clear();
    }

    SmartPtr<ISoundListener3> SoundManager::addListener3( const String &name,
                                                          const Vector3<real_Num> &position
                                                          /*= Vector3<real_Num>::zero() */ )
    {
        auto factoryManager = getFactoryManager();
        if( factoryManager )
        {
            auto listener = factoryManager->make_object<ISoundListener3>();
            if( listener )
            {
                listener->setName( name );
                listener->setPosition( position );
                m_listeners.push_back( listener );
                return listener;
            }
        }

        return nullptr;
    }

    SmartPtr<ISoundListener3> SoundManager::findListener3( const String &name )
    {
        for( auto listener : m_listeners )
        {
            if( listener->getName() == name )
            {
                return listener;
            }
        }

        return nullptr;
    }

    void SoundManager::setVolume( f32 volume )
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->invalidateStateData<SoundManagerStateData>() )
            {
                state->volume = volume;
            }
        }
    }

    f32 SoundManager::getVolume() const
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->getStateByType<SoundManagerStateData>() )
            {
                return state->volume;
            }
        }

        return 0.0f;
    }

    void SoundManager::startRecording()
    {
    }

    void SoundManager::stopRecording()
    {
    }

    u32 SoundManager::getBufferSize() const
    {
        return 0;
    }

    void SoundManager::copyContentsToMemory( void *buffer, u32 size )
    {
    }

    bool SoundManager::isRealtime() const
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->getStateByType<SoundManagerStateData>() )
            {
                return BitUtil::getFlagValue( state->flags, ISoundManager::SOUND_FLAG_REALTIME );
            }
        }

        return false;
    }

    bool SoundManager::isMute() const
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->getStateByType<SoundManagerStateData>() )
            {
                return BitUtil::getFlagValue( state->flags, ISoundManager::SOUND_FLAG_MUTE );
            }
        }

        return false;
    }

    void SoundManager::setMute( bool mute )
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->getStateByType<SoundManagerStateData>() )
            {
                state->flags =
                    BitUtil::setFlagValue( state->flags, ISoundManager::SOUND_FLAG_MUTE, mute );
            }
        }
    }

    void SoundManager::loadObject( SmartPtr<ISharedObject> object, bool forceQueue /*= false */ )
    {
        if( forceQueue )
        {
            m_loadQueue.push( object );
        }
        else
        {
            object->load( nullptr );
        }
    }

    void SoundManager::unloadObject( SmartPtr<ISharedObject> object, bool forceQueue /*= false */ )
    {
        if( forceQueue )
        {
            m_unloadQueue.push( object );
        }
        else
        {
            object->unload( nullptr );
        }
    }

    SmartPtr<IResource> SoundManager::create( const String &name )
    {
        auto factoryManager = getFactoryManager();
        if( factoryManager )
        {
            auto sound = factoryManager->make_object<ISound>();
            if( sound )
            {
                auto uuid = StringUtil::generateUUID();

                auto handle = sound->getHandle();
                handle->setUUID( uuid );

                sound->setOwner( this );
                sound->setFilePath( name );
                loadObject( sound );
                if( !sound->isLoaded() )
                {
                    sound->unload( nullptr );
                    sound->setOwner( nullptr );
                    return nullptr;
                }
                m_sounds.push_back( sound );
                return sound;
            }
        }

        return nullptr;
    }

    SmartPtr<IResource> SoundManager::create( const String &uuid, const String &name )
    {
        auto factoryManager = getFactoryManager();
        if( factoryManager )
        {
            auto sound = factoryManager->make_object<ISound>();
            if( sound )
            {
                auto handle = sound->getHandle();
                handle->setUUID( uuid );

                sound->setOwner( this );
                sound->setFilePath( name );
                sound->load( nullptr );
                if( !sound->isLoaded() )
                {
                    sound->unload( nullptr );
                    sound->setOwner( nullptr );
                    return nullptr;
                }
                m_sounds.push_back( sound );
                return sound;
            }
        }

        return nullptr;
    }

    Pair<SmartPtr<IResource>, bool> SoundManager::createOrRetrieve( const String &uuid,
                                                                    const String &path,
                                                                    const String &type )
    {
        auto sound = getById( uuid );
        if( sound )
        {
            return { sound, false };
        }

        sound = create( uuid, path );
        if( sound )
        {
            return { sound, true };
        }

        return { nullptr, false };
    }

    Pair<SmartPtr<IResource>, bool> SoundManager::createOrRetrieve( const String &path )
    {
        auto sound = getByName( path );
        if( sound )
        {
            return { sound, false };
        }

        sound = create( path );
        if( sound )
        {
            return { sound, true };
        }

        return { nullptr, false };
    }

    void SoundManager::saveToFile( const String &filePath, SmartPtr<IResource> resource )
    {
        auto sound = workphone::dynamic_pointer_cast<ISound>( resource );
        if( sound )
        {
            sound->saveToFile( filePath );
        }
    }

    SmartPtr<IResource> SoundManager::loadFromFile( const String &filePath )
    {
        auto sound = createOrRetrieve( filePath );
        if( sound.first )
        {
            return sound.first;
        }

        return nullptr;
    }

    SmartPtr<IResource> SoundManager::loadResource( const String &name )
    {
        auto sound = createOrRetrieve( name );
        if( sound.first )
        {
            return sound.first;
        }

        return nullptr;
    }

    SmartPtr<IResource> SoundManager::getByName( const String &name )
    {
        auto sounds = m_sounds.snapshot();
        for( auto &sound : sounds )
        {
            if( sound )
            {
                if( sound->getFilePath() == name )
                {
                    return sound;
                }
            }
        }

        return nullptr;
    }

    SmartPtr<IResource> SoundManager::getById( const String &uuid )
    {
        auto sounds = m_sounds.snapshot();
        for( auto &sound : m_sounds )
        {
            if( sound )
            {
                auto handle = sound->getHandle();
                if( handle->getUUIDAsString() == uuid )
                {
                    return sound;
                }
            }
        }

        return nullptr;
    }

    SmartPtr<IFactoryManager> SoundManager::getFactoryManager() const
    {
        return m_factoryManager;
    }

    void SoundManager::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

    SmartPtr<IResource> SoundManager::cloneResource( SmartPtr<IResource> resource,
                                                     const String &clonedResourceName )
    {
        auto source = workphone::dynamic_pointer_cast<ISound>( resource );
        if( !source )
        {
            return nullptr;
        }
        // A clone is an independent transport, never createOrRetrieve(path).
        auto copy = workphone::dynamic_pointer_cast<ISound>( create( source->getFilePath() ) );
        if( copy )
        {
            copy->setVolume( source->getVolume() );
            copy->setLoop( source->getLoop() );
            if( auto concrete = workphone::dynamic_pointer_cast<Sound>( source ) )
            {
                copy->setPan( concrete->getPan() );
            }
            f32 minimum = 0.0f, maximum = 0.0f;
            source->getMinMaxDistance( minimum, maximum );
            copy->setMinMaxDistance( minimum, maximum );
            copy->setPosition( source->getPosition() );
            copy->setFlags( source->getFlags() );
        }
        return copy;
    }

    SmartPtr<IResource> SoundManager::cloneResource( const String &name,
                                                     const String &clonedResourceName )
    {
        return cloneResource( getByName( name ), clonedResourceName );
    }

    void SoundManager::lock()
    {
        m_mutex.lock();
    }

    bool SoundManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void SoundManager::unlock()
    {
        m_mutex.unlock();
    }

    void SoundManager::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    SmartPtr<IStateContext> SoundManager::getStateContext() const
    {
        return m_stateContext;
    }

    void SoundManager::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    SmartPtr<IStateListener> SoundManager::getStateListener() const
    {
        return m_stateListener;
    }

    void SoundManager::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        m_stateListener = stateListener;
    }

    u32 SoundManager::getFlags() const
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->getStateByType<SoundManagerStateData>() )
            {
                return state->flags;
            }
        }

        return 0;
    }

    void SoundManager::setFlags( u32 flags )
    {
        if( auto context = getStateContext() )
        {
            if( auto state = context->invalidateStateData<SoundManagerStateData>() )
            {
                state->flags = flags;
            }
        }
    }

    void SoundManager::setFlag( u32 flags, bool value )
    {
        auto currentFlags = getFlags();
        if( value )
        {
            currentFlags |= flags;
        }
        else
        {
            currentFlags &= ~flags;
        }

        setFlags( currentFlags );
    }

    bool SoundManager::getFlag( u32 flags ) const
    {
        auto currentFlags = getFlags();
        if( ( currentFlags & flags ) != 0 )
        {
            return true;
        }

        return false;
    }

    bool SoundManager::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool SoundManager::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    SoundManager::StateListener::StateListener() = default;

    SoundManager::StateListener::~StateListener() = default;

    void SoundManager::StateListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    bool SoundManager::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateMessage( message );
        }

        return false;
    }

    bool SoundManager::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateChanged( state );
        }

        return false;
    }

    SmartPtr<SoundManager> SoundManager::StateListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void SoundManager::StateListener::setOwner( SmartPtr<SoundManager> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone
