#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/WPAudioManager.hpp>
#include <WPAudio/WPAudioSound.hpp>
#include <WPAudio/WPAudioSoundListener.hpp>
#include <WPAudio/WPAudio.hpp>
#include <Workphone/WorkphoneInterface.hpp>
#include <new>

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
#        include <xaudio2.h>
#    elif WP_USE_WASAPI
#        include <audioclient.h>
#        include <mmdeviceapi.h>
#    endif

#    include <windows.h>

#    if defined _MSC_VER && WP_USE_XAUDIO2
#        pragma comment( lib, "xaudio2.lib" )
#    endif
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
#    include <AudioUnit/AudioUnit.h>
#elif defined WP_PLATFORM_ANDROID
#    include <SLES/OpenSLES.h>
#endif

namespace workphone
{
    struct WPAudioManager::PlatformAudioState
    {
#if defined WP_PLATFORM_WIN32
        bool ownsComInitialization = false;

#    if WP_USE_XAUDIO2
        IXAudio2 *xAudio2 = nullptr;
        IXAudio2MasteringVoice *masterVoice = nullptr;
#    elif WP_USE_WASAPI
        IAudioClient *audioClient = nullptr;
        IMMDevice *device = nullptr;
#    endif
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        AudioComponentInstance audioUnit = nullptr;
#elif defined WP_PLATFORM_ANDROID
        SLObjectItf engineObject = nullptr;
        SLEngineItf engine = nullptr;
        SLObjectItf outputMixObject = nullptr;
#endif
    };

    WP_CLASS_REGISTER_DERIVED( workphone, WPAudioManager, SoundManager );

    WPAudioManager::WPAudioManager() = default;

    WPAudioManager::~WPAudioManager()
    {
        removeAll();
        cleanupPlatformAudio();
    }

    void WPAudioManager::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );

        auto factoryManager = WPAudio::getFactoryManager();
        setFactoryManager( factoryManager );

        if( !initializePlatformAudio() )
        {
            setLoadingState( LoadingState::Error );
            return;
        }

        SoundManager::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void WPAudioManager::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );

        removeAll();
        SoundManager::unload( data );
        cleanupPlatformAudio();

        setLoadingState( LoadingState::Unloaded );
    }

    bool WPAudioManager::initializePlatformAudio()
    {
        cleanupPlatformAudio();

        auto state = new( std::nothrow ) PlatformAudioState();
        if( !state )
        {
            return false;
        }

        m_platformAudioState = state;

#if defined WP_PLATFORM_WIN32
        const auto comResult = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
        if( SUCCEEDED( comResult ) )
        {
            state->ownsComInitialization = true;
        }
        else if( comResult != RPC_E_CHANGED_MODE )
        {
            cleanupPlatformAudio();
            return false;
        }

#    if WP_USE_XAUDIO2
        auto result = XAudio2Create( &state->xAudio2 );
        if( FAILED( result ) )
        {
            cleanupPlatformAudio();
            return false;
        }

        result = state->xAudio2->CreateMasteringVoice( &state->masterVoice );
        if( FAILED( result ) )
        {
            cleanupPlatformAudio();
            return false;
        }
#    elif WP_USE_WASAPI
        IMMDeviceEnumerator *enumerator = nullptr;
        auto result =
            CoCreateInstance( __uuidof( MMDeviceEnumerator ), nullptr, CLSCTX_ALL,
                              __uuidof( IMMDeviceEnumerator ), reinterpret_cast<void **>( &enumerator ) );
        if( FAILED( result ) )
        {
            cleanupPlatformAudio();
            return false;
        }

        result = enumerator->GetDefaultAudioEndpoint( eRender, eConsole, &state->device );
        enumerator->Release();
        if( FAILED( result ) )
        {
            cleanupPlatformAudio();
            return false;
        }

        result = state->device->Activate( __uuidof( IAudioClient ), CLSCTX_ALL, nullptr,
                                          reinterpret_cast<void **>( &state->audioClient ) );
        if( FAILED( result ) )
        {
            cleanupPlatformAudio();
            return false;
        }

        WAVEFORMATEX *mixFormat = nullptr;
        result = state->audioClient->GetMixFormat( &mixFormat );
        if( FAILED( result ) )
        {
            cleanupPlatformAudio();
            return false;
        }

        result =
            state->audioClient->Initialize( AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, mixFormat, nullptr );
        CoTaskMemFree( mixFormat );
        if( FAILED( result ) )
        {
            cleanupPlatformAudio();
            return false;
        }
#    endif
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        AudioComponentDescription description = {};
        description.componentType = kAudioUnitType_Output;
#    if defined WP_PLATFORM_IOS
        description.componentSubType = kAudioUnitSubType_RemoteIO;
#    else
        description.componentSubType = kAudioUnitSubType_DefaultOutput;
#    endif
        description.componentManufacturer = kAudioUnitManufacturer_Apple;

        auto component = AudioComponentFindNext( nullptr, &description );
        if( !component || AudioComponentInstanceNew( component, &state->audioUnit ) != noErr ||
            !state->audioUnit || AudioUnitInitialize( state->audioUnit ) != noErr )
        {
            cleanupPlatformAudio();
            return false;
        }
#elif defined WP_PLATFORM_ANDROID
        auto result = slCreateEngine( &state->engineObject, 0, nullptr, 0, nullptr, nullptr );
        if( result != SL_RESULT_SUCCESS || !state->engineObject )
        {
            cleanupPlatformAudio();
            return false;
        }

        result = ( *state->engineObject )->Realize( state->engineObject, SL_BOOLEAN_FALSE );
        if( result != SL_RESULT_SUCCESS )
        {
            cleanupPlatformAudio();
            return false;
        }

        result =
            ( *state->engineObject )->GetInterface( state->engineObject, SL_IID_ENGINE, &state->engine );
        if( result != SL_RESULT_SUCCESS || !state->engine )
        {
            cleanupPlatformAudio();
            return false;
        }

        result =
            ( *state->engine )
                ->CreateOutputMix( state->engine, &state->outputMixObject, 0, nullptr, nullptr );
        if( result != SL_RESULT_SUCCESS || !state->outputMixObject )
        {
            cleanupPlatformAudio();
            return false;
        }

        result = ( *state->outputMixObject )->Realize( state->outputMixObject, SL_BOOLEAN_FALSE );
        if( result != SL_RESULT_SUCCESS )
        {
            cleanupPlatformAudio();
            return false;
        }
#else
        cleanupPlatformAudio();
        return false;
#endif

        return true;
    }

    void WPAudioManager::cleanupPlatformAudio()
    {
        auto state = m_platformAudioState;
        if( !state )
        {
            return;
        }

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        if( state->masterVoice )
        {
            state->masterVoice->DestroyVoice();
            state->masterVoice = nullptr;
        }

        if( state->xAudio2 )
        {
            state->xAudio2->Release();
            state->xAudio2 = nullptr;
        }
#    elif WP_USE_WASAPI
        if( state->audioClient )
        {
            state->audioClient->Stop();
            state->audioClient->Release();
            state->audioClient = nullptr;
        }

        if( state->device )
        {
            state->device->Release();
            state->device = nullptr;
        }
#    endif

        if( state->ownsComInitialization )
        {
            CoUninitialize();
            state->ownsComInitialization = false;
        }
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        if( state->audioUnit )
        {
            AudioOutputUnitStop( state->audioUnit );
            AudioUnitUninitialize( state->audioUnit );
            AudioComponentInstanceDispose( state->audioUnit );
            state->audioUnit = nullptr;
        }
#elif defined WP_PLATFORM_ANDROID
        if( state->outputMixObject )
        {
            ( *state->outputMixObject )->Destroy( state->outputMixObject );
            state->outputMixObject = nullptr;
        }

        if( state->engineObject )
        {
            ( *state->engineObject )->Destroy( state->engineObject );
            state->engineObject = nullptr;
            state->engine = nullptr;
        }
#endif

        delete state;
        m_platformAudioState = nullptr;
    }

    void WPAudioManager::update()
    {
        SoundManager::update();

        // Update all active sounds (e.g., check playback state, handle streaming, etc.)
        for( auto &sound : m_sounds )
        {
            if( sound && sound->isLoaded() )
            {
                // Individual sounds can handle their own update logic if needed
                sound->update();
            }
        }
    }

    void WPAudioManager::loadSoundMap( const Properties &soundMap )
    {
        // Store sound name to file path mappings
        // This allows sounds to be referenced by name rather than file path
        // TODO: Implement property iteration when Properties API is available
    }

    bool WPAudioManager::loadSoundEvents( const String &filePath )
    {
        // TODO: Load sound event definitions from a file
        // This could include trigger conditions, sound groups, etc.
        return false;
    }

    SmartPtr<ISound> WPAudioManager::addSound( const String &filePath, bool bLoop )
    {
        auto sound = make_ptr<WPAudioSound>();
        sound->setOwner( this );
        sound->setFilePath( filePath );
        sound->setLoop( bLoop );
        sound->load( nullptr );

        if( !sound->isLoaded() )
        {
            return nullptr;
        }

        sound->setVolume( m_mute ? 0.0f : m_volume );

        m_sounds.push_back( sound );
        return sound;
    }

    void WPAudioManager::removeSound( SmartPtr<ISound> sound )
    {
        if( !sound )
        {
            return;
        }

        // Ensure the sound is unloaded before removing
        if( sound->isLoaded() )
        {
            sound->unload( nullptr );
        }

        auto it = std::find( m_sounds.begin(), m_sounds.end(), sound );
        if( it != m_sounds.end() )
        {
            m_sounds.erase( it );
        }
    }

    void WPAudioManager::removeAll()
    {
        // Unload and remove all sounds
        for( auto &sound : m_sounds )
        {
            if( sound && sound->isLoaded() )
            {
                sound->unload( nullptr );
            }
        }

        m_sounds.clear();
        m_listeners.clear();
    }

    SmartPtr<ISoundListener3> WPAudioManager::addListener3( const String &name,
                                                            const Vector3F &position )
    {
        auto listener = make_ptr<WPAudioSoundListener>();
        listener->setName( name );
        listener->setPosition( position );
        m_listeners[name] = listener;
        return listener;
    }

    SmartPtr<ISoundListener3> WPAudioManager::findListener3( const String &name )
    {
        auto it = m_listeners.find( name );
        if( it != m_listeners.end() )
        {
            return it->second;
        }

        return nullptr;
    }

    bool WPAudioManager::isRealtime() const
    {
        return false;
    }

    bool WPAudioManager::isMute() const
    {
        return m_mute;
    }

    void WPAudioManager::setMute( bool mute )
    {
        if( m_mute == mute )
        {
            return;
        }

        m_mute = mute;

        // Propagate mute state to all sounds
        for( auto &sound : m_sounds )
        {
            if( sound )
            {
                sound->setVolume( mute ? 0.0f : m_volume );
            }
        }

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        if( m_platformAudioState && m_platformAudioState->masterVoice )
        {
            m_platformAudioState->masterVoice->SetVolume( mute ? 0.0f : m_volume );
        }
#endif
    }

    void WPAudioManager::setVolume( f32 fVolume )
    {
        m_volume = fVolume;

        // Don't propagate if muted
        if( m_mute )
        {
            return;
        }

        // Propagate volume to all sounds
        for( auto &sound : m_sounds )
        {
            if( sound )
            {
                sound->setVolume( fVolume );
            }
        }

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        if( m_platformAudioState && m_platformAudioState->masterVoice )
        {
            m_platformAudioState->masterVoice->SetVolume( fVolume );
        }
#endif
    }

    f32 WPAudioManager::getVolume() const
    {
        return m_volume;
    }

    void WPAudioManager::startRecording()
    {
        // TODO: Implement audio recording
        // This would involve creating a capture device and buffers
    }

    void WPAudioManager::stopRecording()
    {
        // TODO: Implement stopping audio recording
    }

    u32 WPAudioManager::getBufferSize() const
    {
        // TODO: Return recording buffer size
        return 0;
    }

    void WPAudioManager::copyContentsToMemory( void *buffer, u32 size )
    {
        // TODO: Copy recorded audio data to provided buffer
    }

    void WPAudioManager::_getObject( void **ppObject ) const
    {
        if( !ppObject )
        {
            return;
        }

        *ppObject = nullptr;
        if( !m_platformAudioState )
        {
            return;
        }

#if defined WP_PLATFORM_WIN32
#    if WP_USE_XAUDIO2
        *ppObject = m_platformAudioState->xAudio2;
#    elif WP_USE_WASAPI
        *ppObject = m_platformAudioState->audioClient;
#    endif
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        *ppObject = m_platformAudioState->audioUnit;
#elif defined WP_PLATFORM_ANDROID
        *ppObject =
            const_cast<void *>( reinterpret_cast<const void *>( m_platformAudioState->engine ) );
#endif
    }

}  // namespace workphone
