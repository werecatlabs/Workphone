#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/WPAudioManager.hpp>
#include <WPAudio/WPAudioSound.hpp>
#include <WPAudio/WPAudioSoundListener.hpp>
#include <WPAudio/WPAudio.hpp>
#include <Workphone/WorkphoneInterface.hpp>
#include <new>
#include <cmath>
#include <algorithm>
#include <cwctype>

#if defined WP_PLATFORM_WIN32
#    include <mmdeviceapi.h>
#    include <functiondiscoverykeys_devpkey.h>
#    include <propvarutil.h>
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
        SoundManager::load( data );

        if( !initializePlatformAudio() )
        {
            setLoadingState( LoadingState::Error );
            return;
        }

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
        if( m_audioBackend != 0 && m_audioBackend != 3 )
        {
            WP_LOG_ERROR( "WPAudio: the selected external audio backend plugin is not loaded." );
            return false;
        }

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
            WP_LOG_ERROR( "WPAudio: COM initialization failed: " + StringUtil::toString( comResult ) );
            cleanupPlatformAudio();
            return false;
        }

#    if WP_USE_XAUDIO2
        auto result = XAudio2Create( &state->xAudio2 );
        if( FAILED( result ) )
        {
            WP_LOG_ERROR( "WPAudio: XAudio2 initialization failed: " + StringUtil::toString( result ) );
            cleanupPlatformAudio();
            return false;
        }

        LPWSTR selectedDevice = nullptr;
        if( m_outputDevice != 0 )
        {
            IMMDeviceEnumerator *enumerator = nullptr;
            IMMDeviceCollection *devices = nullptr;
            if( SUCCEEDED( CoCreateInstance( __uuidof( MMDeviceEnumerator ), nullptr, CLSCTX_ALL,
                                            __uuidof( IMMDeviceEnumerator ),
                                            reinterpret_cast<void **>( &enumerator ) ) ) )
            {
                if( SUCCEEDED( enumerator->EnumAudioEndpoints( eRender, DEVICE_STATE_ACTIVE, &devices ) ) )
                {
                    UINT count = 0;
                    devices->GetCount( &count );
                    for( UINT i = 0; i < count && !selectedDevice; ++i )
                    {
                        IMMDevice *device = nullptr;
                        IPropertyStore *store = nullptr;
                        if( SUCCEEDED( devices->Item( i, &device ) ) )
                        {
                            if( SUCCEEDED( device->OpenPropertyStore( STGM_READ, &store ) ) )
                            {
                                PROPVARIANT name;
                                PropVariantInit( &name );
                                if( SUCCEEDED( store->GetValue( PKEY_Device_FriendlyName, &name ) ) &&
                                    name.vt == VT_LPWSTR && name.pwszVal )
                                {
                                    std::wstring label( name.pwszVal );
                                    std::transform( label.begin(), label.end(), label.begin(),
                                        []( wchar_t c ) { return static_cast<wchar_t>( std::towlower( c ) ); } );
                                    const bool match = m_outputDevice == 1 ?
                                        label.find( L"head" ) != std::wstring::npos : m_outputDevice == 2 ?
                                        label.find( L"speaker" ) != std::wstring::npos : m_outputDevice == 3 ?
                                        label.find( L"controller" ) != std::wstring::npos :
                                        label.find( L"mobile" ) != std::wstring::npos ||
                                            label.find( L"phone" ) != std::wstring::npos;
                                    if( match ) device->GetId( &selectedDevice );
                                }
                                PropVariantClear( &name );
                                store->Release();
                            }
                            device->Release();
                        }
                    }
                    devices->Release();
                }
                enumerator->Release();
            }
            if( !selectedDevice )
            {
                WP_LOG_ERROR( "WPAudio: requested output device is not connected." );
                cleanupPlatformAudio();
                return false;
            }
        }
        result = state->xAudio2->CreateMasteringVoice( &state->masterVoice, 2,
            m_playbackSampleRate, 0, selectedDevice );
        if( selectedDevice ) CoTaskMemFree( selectedDevice );
        if( FAILED( result ) )
        {
            WP_LOG_ERROR( "WPAudio: cannot open the output device: " + StringUtil::toString( result ) );
            cleanupPlatformAudio();
            return false;
        }
#    elif WP_USE_WASAPI
        IMMDeviceEnumerator *enumerator = nullptr;
        auto result = CoCreateInstance( __uuidof( MMDeviceEnumerator ), nullptr, CLSCTX_ALL,
                                        __uuidof( IMMDeviceEnumerator ),
                                        reinterpret_cast<void **>( &enumerator ) );
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

        result = state->audioClient->Initialize( AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, mixFormat, nullptr );
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

        result = ( *state->engine )
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

        m_activeSampleRate = m_playbackSampleRate;
        m_activeOutputDevice = m_outputDevice;
        m_activeBackend = m_audioBackend;
        setVolume( m_volume );
        return true;
    }

    SmartPtr<Properties> WPAudioManager::getProperties() const
    {
        auto properties = SoundManager::getProperties();
        properties->setProperty( "active.sound.sampleRate", m_activeSampleRate );
        properties->setProperty( "active.sound.outputDevice", m_activeOutputDevice );
        properties->setProperty( "active.sound.backend", m_activeBackend );
        return properties;
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
        for( auto &sound : m_sounds.snapshot() )
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

        SoundManager::destroyResource( sound );
        sound->setOwner( nullptr );
    }

    void WPAudioManager::removeAll()
    {
        SoundManager::destroyAll();
    }

    SmartPtr<ISoundListener3> WPAudioManager::addListener3( const String &name,
                                                            const Vector3F &position )
    {
        return SoundManager::addListener3( name, position );
    }

    SmartPtr<ISoundListener3> WPAudioManager::findListener3( const String &name )
    {
        return SoundManager::findListener3( name );
    }

    bool WPAudioManager::isRealtime() const
    {
        return isLoaded() && m_platformAudioState != nullptr;
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

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        if( m_platformAudioState && m_platformAudioState->masterVoice )
        {
            m_platformAudioState->masterVoice->SetVolume( mute ? 0.0f : m_volume );
        }
#else
        for( auto &sound : m_sounds.snapshot() )
        {
            if( sound )
            {
                sound->setVolume( sound->getVolume() );
            }
        }
#endif
    }

    void WPAudioManager::setVolume( f32 fVolume )
    {
        m_volume = std::isfinite( fVolume ) ? std::clamp( fVolume, 0.0f, 1.0f ) : 0.0f;
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        if( m_platformAudioState && m_platformAudioState->masterVoice )
        {
            m_platformAudioState->masterVoice->SetVolume( m_mute ? 0.0f : m_volume );
        }
#else
        for( auto &sound : m_sounds.snapshot() )
        {
            if( sound )
            {
                sound->setVolume( sound->getVolume() );
            }
        }
#endif
    }

    f32 WPAudioManager::getVolume() const
    {
        return m_volume;
    }

    void WPAudioManager::startRecording()
    {
        WP_LOG_WARNING( "WPAudio: recording is unsupported by this backend." );
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
        WP_LOG_WARNING( "WPAudio: no capture buffer is available." );
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
        *ppObject = const_cast<void *>( reinterpret_cast<const void *>( m_platformAudioState->engine ) );
#endif
    }

}  // namespace workphone
