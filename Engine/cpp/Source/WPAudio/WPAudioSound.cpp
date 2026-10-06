#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/WPAudioSound.hpp>
#include <Workphone/Workphone.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <new>
#include <vector>

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
#    include <xaudio2.h>
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
#    include <AudioToolbox/AudioQueue.h>
#elif defined WP_PLATFORM_ANDROID
#    include <SLES/OpenSLES.h>
#    include <SLES/OpenSLES_Android.h>
#endif

namespace workphone
{
    namespace
    {
        constexpr size_Num WavChunkHeaderSize = 8;
        constexpr u16 WavFormatPcm = 1;

        struct WavData
        {
            std::vector<u8> format;
            std::vector<u8> samples;
            u16 formatTag = 0;
            u16 channels = 0;
            u32 sampleRate = 0;
            u16 blockAlign = 0;
            u16 bitsPerSample = 0;
        };

        bool readExact( IStream *stream, void *buffer, size_Num bytes )
        {
            return stream && stream->read( buffer, bytes ) == bytes;
        }

        bool readFourCC( IStream *stream, char ( &fourCC )[4] )
        {
            return readExact( stream, fourCC, sizeof( fourCC ) );
        }

        bool readUInt32( IStream *stream, u32 &value )
        {
            return readExact( stream, &value, sizeof( value ) );
        }

        bool fourCCEquals( const char ( &fourCC )[4], const char *value )
        {
            return std::memcmp( fourCC, value, 4 ) == 0;
        }

        bool skipBytes( IStream *stream, size_Num bytesToSkip )
        {
            if( !stream )
            {
                return false;
            }

            const auto position = stream->tell();
            return bytesToSkip <= stream->size() - std::min( position, stream->size() ) &&
                   stream->seek( position + bytesToSkip );
        }

        template <class T>
        T readWavField( const std::vector<u8> &format, size_Num offset )
        {
            T value = {};
            if( offset + sizeof( value ) <= format.size() )
            {
                std::memcpy( &value, format.data() + offset, sizeof( value ) );
            }
            return value;
        }

        bool readWav( IStream *stream, WavData &wav )
        {
            if( !stream || !stream->isOpen() )
            {
                return false;
            }

            char riff[4] = {};
            u32 riffSize = 0;
            char wave[4] = {};
            if( !readFourCC( stream, riff ) || !readUInt32( stream, riffSize ) ||
                !readFourCC( stream, wave ) || !fourCCEquals( riff, "RIFF" ) ||
                !fourCCEquals( wave, "WAVE" ) )
            {
                return false;
            }
            (void)riffSize;

            while( stream->tell() + WavChunkHeaderSize <= stream->size() &&
                   ( wav.format.empty() || wav.samples.empty() ) )
            {
                char chunkId[4] = {};
                u32 chunkSize = 0;
                if( !readFourCC( stream, chunkId ) || !readUInt32( stream, chunkSize ) ||
                    chunkSize > stream->size() - stream->tell() )
                {
                    return false;
                }

                if( fourCCEquals( chunkId, "fmt " ) )
                {
                    if( chunkSize < 16 )
                    {
                        return false;
                    }
                    wav.format.resize( chunkSize );
                    if( !readExact( stream, wav.format.data(), chunkSize ) )
                    {
                        return false;
                    }
                }
                else if( fourCCEquals( chunkId, "data" ) )
                {
                    if( chunkSize == 0 )
                    {
                        return false;
                    }
                    wav.samples.resize( chunkSize );
                    if( !readExact( stream, wav.samples.data(), chunkSize ) )
                    {
                        return false;
                    }
                }
                else if( !skipBytes( stream, chunkSize ) )
                {
                    return false;
                }

                if( ( chunkSize & 1U ) != 0 && !skipBytes( stream, 1 ) )
                {
                    return false;
                }
            }

            if( wav.format.empty() || wav.samples.empty() )
            {
                return false;
            }

            wav.formatTag = readWavField<u16>( wav.format, 0 );
            wav.channels = readWavField<u16>( wav.format, 2 );
            wav.sampleRate = readWavField<u32>( wav.format, 4 );
            wav.blockAlign = readWavField<u16>( wav.format, 12 );
            wav.bitsPerSample = readWavField<u16>( wav.format, 14 );
            return wav.channels > 0 && wav.sampleRate > 0 && wav.blockAlign > 0 && wav.bitsPerSample > 0;
        }

        SmartPtr<IStream> openAudioStream( const String &filename )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager ? applicationManager->getFileSystem() : nullptr;
            if( !fileSystem )
            {
                return nullptr;
            }

            auto stream = fileSystem->open( filename, true, true, false, false, false );
            if( !stream )
            {
                stream = fileSystem->open( filename, true, true, false, true, true );
            }
            return stream;
        }
    }  // namespace

    struct WPAudioSound::PlatformSoundState
    {
        WavData wav;
        std::atomic_bool playing{ false };
        std::atomic_bool loop{ false };

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        struct VoiceCallback final : IXAudio2VoiceCallback
        {
            explicit VoiceCallback( PlatformSoundState *owner ) : owner( owner )
            {
            }

            void STDMETHODCALLTYPE OnVoiceProcessingPassStart( UINT32 ) override
            {
            }
            void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override
            {
            }
            void STDMETHODCALLTYPE OnStreamEnd() override
            {
                owner->playing = false;
            }
            void STDMETHODCALLTYPE OnBufferStart( void * ) override
            {
            }
            void STDMETHODCALLTYPE OnBufferEnd( void * ) override
            {
                if( !owner->loop )
                {
                    owner->playing = false;
                }
            }
            void STDMETHODCALLTYPE OnLoopEnd( void * ) override
            {
            }
            void STDMETHODCALLTYPE OnVoiceError( void *, HRESULT ) override
            {
                owner->playing = false;
            }

            PlatformSoundState *owner;
        } callback{ this };

        IXAudio2SourceVoice *sourceVoice = nullptr;
        XAUDIO2_BUFFER buffer = {};
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        static void outputCallback( void *context, AudioQueueRef queue, AudioQueueBufferRef buffer )
        {
            auto state = static_cast<PlatformSoundState *>( context );
            if( state->loop && state->playing )
            {
                AudioQueueEnqueueBuffer( queue, buffer, 0, nullptr );
            }
            else
            {
                state->playing = false;
            }
        }

        AudioQueueRef queue = nullptr;
        AudioQueueBufferRef buffer = nullptr;
#elif defined WP_PLATFORM_ANDROID
        static void bufferCallback( SLAndroidSimpleBufferQueueItf queue, void *context )
        {
            auto state = static_cast<PlatformSoundState *>( context );
            if( state->loop && state->playing )
            {
                ( *queue )->Enqueue( queue, state->wav.samples.data(),
                                     static_cast<SLuint32>( state->wav.samples.size() ) );
            }
            else
            {
                state->playing = false;
            }
        }

        SLObjectItf playerObject = nullptr;
        SLObjectItf outputMixObject = nullptr;
        SLPlayItf player = nullptr;
        SLAndroidSimpleBufferQueueItf bufferQueue = nullptr;
        SLVolumeItf volume = nullptr;
#endif
    };

    WP_CLASS_REGISTER_DERIVED( workphone, WPAudioSound, ISound );

    WPAudioSound::WPAudioSound() = default;

    WPAudioSound::~WPAudioSound()
    {
        unloadPlatformSound();
    }

    void WPAudioSound::load( SmartPtr<ISharedObject> data )
    {
        (void)data;
        if( isLoaded() )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );
        auto filePath = getFilePath();
        const auto originalFilePath = filePath;
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            setLoadingState( LoadingState::Unloaded );
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        if( applicationManager )
        {
            if( auto fileSystem = applicationManager->getFileSystem() )
            {
                const auto workingDirectory = Path::getWorkingDirectory();
                auto exists = [&]( const String &candidate ) {
                    const auto clean = StringUtil::cleanupPath( candidate );
                    return fileSystem->isExistingFile( clean ) ||
                           std::filesystem::exists( clean.c_str() );
                };
                auto resolve = [&]( const String &candidate ) {
                    auto clean = StringUtil::cleanupPath( candidate );
                    if( exists( clean ) )
                    {
                        return clean;
                    }
                    auto absolute =
                        StringUtil::cleanupPath( Path::getAbsolutePath( workingDirectory, clean ) );
                    return exists( absolute ) ? absolute : clean;
                };

                filePath = resolve( filePath );
                if( !exists( filePath ) )
                {
                    filePath = resolve( applicationManager->getMediaPath() + "/" + filePath );
                }
                if( !exists( filePath ) )
                {
                    filePath = resolve( String( "../../../../../Media/" ) + originalFilePath );
                }
            }
        }

        setLoadingState( loadPlatformSound( filePath ) ? LoadingState::Loaded : LoadingState::Unloaded );
    }

    void WPAudioSound::unload( SmartPtr<ISharedObject> data )
    {
        (void)data;
        setLoadingState( LoadingState::Unloading );
        unloadPlatformSound();
        Sound::stop();
        setLoadingState( LoadingState::Unloaded );
    }

    bool WPAudioSound::loadPlatformSound( const String &filename )
    {
        unloadPlatformSound();

        auto state = new( std::nothrow ) PlatformSoundState();
        if( !state )
        {
            return false;
        }
        state->loop = getLoop();

        auto stream = openAudioStream( filename );
        if( !readWav( stream.get(), state->wav ) )
        {
            delete state;
            return false;
        }

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        void *audioObject = nullptr;
        if( auto owner = getOwner() )
        {
            owner->_getObject( &audioObject );
        }
        auto xAudio2 = static_cast<IXAudio2 *>( audioObject );
        if( !xAudio2 ||
            FAILED( xAudio2->CreateSourceVoice(
                &state->sourceVoice, reinterpret_cast<const WAVEFORMATEX *>( state->wav.format.data() ),
                0, XAUDIO2_DEFAULT_FREQ_RATIO, &state->callback ) ) )
        {
            delete state;
            return false;
        }

        state->buffer.AudioBytes = static_cast<UINT32>( state->wav.samples.size() );
        state->buffer.pAudioData = state->wav.samples.data();
        state->buffer.Flags = XAUDIO2_END_OF_STREAM;
        state->buffer.LoopCount = state->loop ? XAUDIO2_LOOP_INFINITE : 0;
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        if( state->wav.formatTag != WavFormatPcm )
        {
            delete state;
            return false;
        }

        AudioStreamBasicDescription format = {};
        format.mSampleRate = state->wav.sampleRate;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsSignedInteger;
        if( state->wav.bitsPerSample == 8 )
        {
            format.mFormatFlags = kLinearPCMFormatFlagIsPacked;
        }
        format.mBytesPerPacket = state->wav.blockAlign;
        format.mFramesPerPacket = 1;
        format.mBytesPerFrame = state->wav.blockAlign;
        format.mChannelsPerFrame = state->wav.channels;
        format.mBitsPerChannel = state->wav.bitsPerSample;

        if( AudioQueueNewOutput( &format, &PlatformSoundState::outputCallback, state, nullptr, nullptr,
                                 0, &state->queue ) != noErr ||
            !state->queue ||
            AudioQueueAllocateBuffer( state->queue, static_cast<UInt32>( state->wav.samples.size() ),
                                      &state->buffer ) != noErr )
        {
            if( state->queue )
            {
                AudioQueueDispose( state->queue, true );
            }
            delete state;
            return false;
        }
        std::memcpy( state->buffer->mAudioData, state->wav.samples.data(), state->wav.samples.size() );
        state->buffer->mAudioDataByteSize = static_cast<UInt32>( state->wav.samples.size() );
#elif defined WP_PLATFORM_ANDROID
        if( state->wav.formatTag != WavFormatPcm || state->wav.channels > 2 ||
            ( state->wav.bitsPerSample != 8 && state->wav.bitsPerSample != 16 ) )
        {
            delete state;
            return false;
        }

        void *audioObject = nullptr;
        if( auto owner = getOwner() )
        {
            owner->_getObject( &audioObject );
        }
        auto engine = reinterpret_cast<SLEngineItf>( audioObject );
        if( !engine )
        {
            delete state;
            return false;
        }

        SLDataLocator_AndroidSimpleBufferQueue inputLocator = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                                1 };
        const SLuint32 speakerMask = state->wav.channels == 1
                                         ? SL_SPEAKER_FRONT_CENTER
                                         : SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,        state->wav.channels,      state->wav.sampleRate * 1000,
            state->wav.bitsPerSample, state->wav.bitsPerSample, speakerMask,
            SL_BYTEORDER_LITTLEENDIAN
        };
        SLDataSource source = { &inputLocator, &pcm };
        SLDataLocator_OutputMix outputLocator = { SL_DATALOCATOR_OUTPUTMIX, nullptr };
        SLDataSink sink = { &outputLocator, nullptr };
        // The manager exposes the engine for compatibility; ask it for its output mix through
        // the engine by creating a private mix owned by this player.
        SLObjectItf outputMixObject = nullptr;
        auto result = ( *engine )->CreateOutputMix( engine, &outputMixObject, 0, nullptr, nullptr );
        if( result != SL_RESULT_SUCCESS || !outputMixObject ||
            ( *outputMixObject )->Realize( outputMixObject, SL_BOOLEAN_FALSE ) != SL_RESULT_SUCCESS )
        {
            if( outputMixObject )
            {
                ( *outputMixObject )->Destroy( outputMixObject );
            }
            delete state;
            return false;
        }
        outputLocator.outputMix = outputMixObject;

        const SLInterfaceID ids[] = { SL_IID_BUFFERQUEUE, SL_IID_VOLUME };
        const SLboolean required[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
        result = ( *engine )->CreateAudioPlayer( engine, &state->playerObject, &source, &sink, 2, ids,
                                                 required );
        if( result != SL_RESULT_SUCCESS || !state->playerObject ||
            ( *state->playerObject )->Realize( state->playerObject, SL_BOOLEAN_FALSE ) !=
                SL_RESULT_SUCCESS ||
            ( *state->playerObject )->GetInterface( state->playerObject, SL_IID_PLAY, &state->player ) !=
                SL_RESULT_SUCCESS ||
            ( *state->playerObject )
                    ->GetInterface( state->playerObject, SL_IID_BUFFERQUEUE, &state->bufferQueue ) !=
                SL_RESULT_SUCCESS ||
            ( *state->playerObject )
                    ->GetInterface( state->playerObject, SL_IID_VOLUME, &state->volume ) !=
                SL_RESULT_SUCCESS )
        {
            if( state->playerObject )
            {
                ( *state->playerObject )->Destroy( state->playerObject );
            }
            ( *outputMixObject )->Destroy( outputMixObject );
            delete state;
            return false;
        }
        // Retain the private output mix alongside the player by storing it as the object context.
        // OpenSL keeps the realized mix alive until the player is destroyed; destroy it in unload.
        if( ( *state->bufferQueue )
                ->RegisterCallback( state->bufferQueue, &PlatformSoundState::bufferCallback, state ) !=
            SL_RESULT_SUCCESS )
        {
            ( *state->playerObject )->Destroy( state->playerObject );
            ( *outputMixObject )->Destroy( outputMixObject );
            delete state;
            return false;
        }
        // Store the mix in the player's object slot is not supported, so keep it explicitly.
        state->outputMixObject = outputMixObject;
#else
        delete state;
        return false;
#endif

        m_platformSoundState = state;
        setVolume( getVolume() );
        return true;
    }

    void WPAudioSound::unloadPlatformSound()
    {
        auto state = m_platformSoundState;
        if( !state )
        {
            return;
        }
        state->playing = false;

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        if( state->sourceVoice )
        {
            state->sourceVoice->Stop( 0 );
            state->sourceVoice->DestroyVoice();
        }
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        if( state->queue )
        {
            AudioQueueStop( state->queue, true );
            AudioQueueDispose( state->queue, true );
        }
#elif defined WP_PLATFORM_ANDROID
        if( state->playerObject )
        {
            ( *state->playerObject )->Destroy( state->playerObject );
        }
        if( state->outputMixObject )
        {
            ( *state->outputMixObject )->Destroy( state->outputMixObject );
        }
#endif

        delete state;
        m_platformSoundState = nullptr;
    }

    void WPAudioSound::play()
    {
        auto state = m_platformSoundState;
        if( !state )
        {
            return;
        }

        // Set this before starting the native backend because very short buffers can invoke
        // their completion callback before the start call returns.
        state->playing = true;

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        XAUDIO2_VOICE_STATE voiceState = {};
        state->sourceVoice->GetState( &voiceState );
        if( voiceState.BuffersQueued == 0 &&
            FAILED( state->sourceVoice->SubmitSourceBuffer( &state->buffer ) ) )
        {
            state->playing = false;
            return;
        }
        if( FAILED( state->sourceVoice->Start( 0 ) ) )
        {
            state->playing = false;
            return;
        }
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        AudioQueueReset( state->queue );
        if( AudioQueueEnqueueBuffer( state->queue, state->buffer, 0, nullptr ) != noErr ||
            AudioQueueStart( state->queue, nullptr ) != noErr )
        {
            state->playing = false;
            return;
        }
#elif defined WP_PLATFORM_ANDROID
        ( *state->bufferQueue )->Clear( state->bufferQueue );
        if( ( *state->bufferQueue )
                    ->Enqueue( state->bufferQueue, state->wav.samples.data(),
                               static_cast<SLuint32>( state->wav.samples.size() ) ) !=
                SL_RESULT_SUCCESS ||
            ( *state->player )->SetPlayState( state->player, SL_PLAYSTATE_PLAYING ) !=
                SL_RESULT_SUCCESS )
        {
            state->playing = false;
            return;
        }
#else
        state->playing = false;
        return;
#endif
        Sound::play();
    }

    void WPAudioSound::pause()
    {
        auto state = m_platformSoundState;
        if( !state )
        {
            return;
        }
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        state->sourceVoice->Stop( 0 );
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        AudioQueuePause( state->queue );
#elif defined WP_PLATFORM_ANDROID
        ( *state->player )->SetPlayState( state->player, SL_PLAYSTATE_PAUSED );
#endif
        state->playing = false;
        Sound::pause();
    }

    void WPAudioSound::stop()
    {
        auto state = m_platformSoundState;
        if( state )
        {
            state->playing = false;
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
            state->sourceVoice->Stop( 0 );
            state->sourceVoice->FlushSourceBuffers();
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
            AudioQueueStop( state->queue, true );
#elif defined WP_PLATFORM_ANDROID
            ( *state->player )->SetPlayState( state->player, SL_PLAYSTATE_STOPPED );
            ( *state->bufferQueue )->Clear( state->bufferQueue );
#endif
        }
        Sound::stop();
    }

    bool WPAudioSound::isPlaying() const
    {
        return m_platformSoundState && m_platformSoundState->playing;
    }

    void WPAudioSound::setVolume( f32 volume )
    {
        Sound::setVolume( volume );
        auto state = m_platformSoundState;
        if( !state )
        {
            return;
        }
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        state->sourceVoice->SetVolume( getVolume() );
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        AudioQueueSetParameter( state->queue, kAudioQueueParam_Volume, getVolume() );
#elif defined WP_PLATFORM_ANDROID
        const auto gain = getVolume() <= 0.0f
                              ? SL_MILLIBEL_MIN
                              : static_cast<SLmillibel>( 2000.0f * std::log10( getVolume() ) );
        ( *state->volume )->SetVolumeLevel( state->volume, gain );
#endif
    }

    void WPAudioSound::setLoop( bool loop )
    {
        Sound::setLoop( loop );
        auto state = m_platformSoundState;
        if( state )
        {
            state->loop = loop;
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
            state->buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
#endif
        }
    }

    Parameter WPAudioSound::handleEvent( EventType eventType, hash_type eventValue,
                                         const Array<Parameter> &arguments,
                                         SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                         SmartPtr<IEvent> event )
    {
        return Sound::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

    bool WPAudioSound::handleStateChanged( SmartPtr<IState> &state )
    {
        return Sound::handleStateChanged( state );
    }

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
    IXAudio2SourceVoice *WPAudioSound::getSourceVoice() const
    {
        return m_platformSoundState ? m_platformSoundState->sourceVoice : nullptr;
    }

    void WPAudioSound::setSourceVoice( IXAudio2SourceVoice *sourceVoice )
    {
        if( !m_platformSoundState )
        {
            m_platformSoundState = new( std::nothrow ) PlatformSoundState();
        }
        if( m_platformSoundState )
        {
            if( m_platformSoundState->sourceVoice && m_platformSoundState->sourceVoice != sourceVoice )
            {
                m_platformSoundState->sourceVoice->DestroyVoice();
            }
            m_platformSoundState->sourceVoice = sourceVoice;
        }
    }
#endif
}  // namespace workphone
