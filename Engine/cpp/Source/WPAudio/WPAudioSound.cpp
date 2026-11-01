#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/WPAudioSound.hpp>
#include <WPAudio/SharedClipCache.hpp>
#include <WPAudio/AudioResourceCompiler.hpp>
#include <WPAudio/AudioResourceStream.hpp>
#include <Workphone/Sound/SoundManager.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <new>
#include <vector>
#include <workphone_audio_core.h>

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
        constexpr u16 WavFormatPcm = 1;
        void audioFailure(String &error,const char *message) noexcept
        {try{error=message;}catch(...){try{error="Audio operation failed";}catch(...){error.clear();}}}

        struct WavData
        {
            StreamBudget admission;
            std::vector<u8> format;
            std::vector<u8> samples;
            u16 formatTag = 0, channels = 0;
            u32 sampleRate = 0;
            u16 blockAlign = 0, bitsPerSample = 0;
        };

        audio::SharedClipCache<WavData> clipCache;

        template<class Bytes>
        bool readWavBytes( const Bytes &bytes, WavData &wav )
        {
            wp_audio_wav_info info{};
            info.size = sizeof( info );
            if( wp_audio_wav_inspect( bytes.data(), bytes.size(), &info ) != WP_AUDIO_OK )
            {
                return false;
            }
            String budgetError;
            wav.admission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,
                u64(info.data_bytes)+4096,budgetError);
            if(!wav.admission)return false;
            // Native WAVEFORMATEX is 18 bytes even for a 16-byte PCM fmt chunk.
            wav.format.assign( 18, 0 );
            auto field16 = [&]( size_t offset, u16 value ) {
                wav.format[offset] = static_cast<u8>( value );
                wav.format[offset + 1] = static_cast<u8>( value >> 8 );
            };
            auto field32 = [&]( size_t offset, u32 value ) {
                for( size_t i = 0; i < 4; ++i )
                {
                    wav.format[offset + i] = static_cast<u8>( value >> ( i * 8 ) );
                }
            };
            field16( 0, info.format_tag );
            field16( 2, info.channels );
            field32( 4, info.sample_rate );
            field32( 8, info.sample_rate * info.block_align );
            field16( 12, info.block_align );
            field16( 14, info.bits_per_sample );
            wav.samples.assign( bytes.begin() + info.data_offset,
                                bytes.begin() + info.data_offset + info.data_bytes );
            wav.formatTag = info.format_tag;
            wav.channels = info.channels;
            wav.sampleRate = info.sample_rate;
            wav.blockAlign = info.block_align;
            wav.bitsPerSample = info.bits_per_sample;
            return true;
        }

        bool readWav(IStream *stream,WavData &wav)
        {
            if(!stream || !stream->isOpen() || stream->size()>64u*1024u*1024u || stream->size()<12 || !stream->seek(0)) return false;
            std::vector<u8> bytes(stream->size());
            return stream->read(bytes.data(),bytes.size())==bytes.size() && readWavBytes(bytes,wav);
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
        StreamBudget admission;
        std::shared_ptr<const WavData> wav;
        std::unique_ptr<audio::AudioResourceStream> range;
        std::atomic_bool playing{ false };
        std::atomic_bool loop{ false };
        bool paused = false;
        bool preserveStart=false;
        u64 startFrame=0;

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        bool streaming = false;
        std::atomic_size_t streamCursor{ 0 };
        size_t streamBlockBytes = 0;
        void applyMix(f32 gain,f32 pan)
        {
            sourceVoice->SetVolume(gain);const auto channels=range?range->blockAlign()/2:wav?wav->channels:0;
            if(channels&&channels<=2){const float left=std::sqrt(std::clamp(1.0f-pan,0.0f,1.0f)),right=std::sqrt(std::clamp(1.0f+pan,0.0f,1.0f));
                const float mono[]={left,right},stereo[]={left,0,0,right};sourceVoice->SetOutputMatrix(nullptr,channels,2,channels==1?mono:stereo);}
        }
        bool submitStreamBlock()
        {
            if(range){
                const auto block=range->ring().acquire(static_cast<u32>(streamBlockBytes));if(!block.bytes||!block.size)return false;
                XAUDIO2_BUFFER next{};next.AudioBytes=block.size;next.pAudioData=block.bytes;
                if(block.slot<audio::AudioPcmRing::slotCount)next.pContext=reinterpret_cast<void*>(block.slot+1);
                // The range ring owns completion. A submitted native EOS flag
                // would become stale if looping changes before this block ends.
                const bool submitted=SUCCEEDED(sourceVoice->SubmitSourceBuffer(&next));range->ring().submitted(block,submitted);return submitted;
            }
            size_t begin = streamCursor.load();
            size_t end;
            for( ;; )
            {
                const auto offset = begin >= wav->samples.size() ? size_t( 0 ) : begin;
                if( begin >= wav->samples.size() && !loop ) return false;
                end = offset + std::min( streamBlockBytes, wav->samples.size() - offset );
                if( streamCursor.compare_exchange_weak( begin, end ) ) { begin = offset; break; }
            }
            XAUDIO2_BUFFER next = {};
            next.AudioBytes = static_cast<UINT32>( end - begin );
            next.pAudioData = wav->samples.data() + begin;
            if( end == wav->samples.size() && !loop ) next.Flags = XAUDIO2_END_OF_STREAM;
            return SUCCEEDED( sourceVoice->SubmitSourceBuffer( &next ) );
        }
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

            void STDMETHODCALLTYPE OnBufferEnd( void *context ) override
            {
                if(owner->range&&context){const auto slot=reinterpret_cast<uintptr_t>(context)-1;
                    if(slot<audio::AudioPcmRing::slotCount){const bool final=owner->range->ring().slot(slot).dataEnd&&!owner->loop;
                        owner->range->ring().complete(slot,final);if(final)owner->playing=false;}}
                if( owner->streaming && owner->playing ) owner->submitStreamBlock();
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
                ( *queue )->Enqueue( queue, state->wav->samples.data(),
                                     static_cast<SLuint32>( state->wav->samples.size() ) );
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
        if( !m_cookedResource && StringUtil::isNullOrEmpty( filePath ) )
        {
            setLoadingState( LoadingState::Error );
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        if( applicationManager && !m_cookedResource )
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

        setLoadingState( loadPlatformSound( filePath ) ? LoadingState::Loaded : LoadingState::Error );
    }

    void WPAudioSound::reload( SmartPtr<ISharedObject> data )
    {
        const auto hadGoodData = m_platformSoundState != nullptr;
        setLoadingState( LoadingState::Unloaded );
        load( data );
        m_reloadFailed = !isLoaded();
        if( m_reloadFailed && hadGoodData )
        {
            setLoadingState( LoadingState::Loaded );
            WP_LOG_WARNING( "WPAudio: reload failed; previous clip retained." );
        }
    }

    bool WPAudioSound::hasReloadError() const
    {
        return m_reloadFailed;
    }

    std::shared_ptr<const resource::RuntimeResource> WPAudioSound::captureRuntimeResourceSnapshot(
        std::shared_ptr<const resource::RuntimeResource> resource,std::shared_ptr<resource::ResourcePayloadMount> mount,String &error)
    {return audio::AudioResourceStream::capture(std::move(resource),std::move(mount),error);}

    bool WPAudioSound::loadResource(std::shared_ptr<const resource::RuntimeResource> resource,
        std::shared_ptr<resource::ResourcePayloadMount> mount)
    {
        String error;
        if(isLoaded()||!resource)return false;
        auto snapshot=captureRuntimeResourceSnapshot(std::move(resource),mount,error);if(!snapshot)return false;
        m_cookedResource=std::move(snapshot);m_runtimeMount=std::move(mount);
        load(nullptr);
        if(!isLoaded()){m_cookedResource.reset();m_runtimeMount.reset();}
        return isLoaded();
    }

    void WPAudioSound::unload( SmartPtr<ISharedObject> data )
    {
        (void)data;
        try{setLoadingState( LoadingState::Unloading );}catch(const std::bad_alloc&){}
        unloadPlatformSound();
        Sound::stop();
        m_cookedResource.reset();
        m_runtimeMount.reset();
        try{setLoadingState( LoadingState::Unloaded );}catch(const std::bad_alloc&){}
    }

    void WPAudioSound::update()
    {if(auto state=m_platformSoundState)if(state->range)state->range->process(state->loop);}

    bool WPAudioSound::seekFrames(u64 frame,String &error) try
    {
        error.clear();auto state=m_platformSoundState;if(!state){error="Cannot seek an unloaded sound";return false;}
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        if(state->range&&!state->range->owner()){error="PCM seek requires its audio control owner";return false;}
        const auto alignment=state->range?state->range->blockAlign():state->wav?state->wav->blockAlign:0;
        const auto count=state->range?state->range->frames():alignment?state->wav->samples.size()/alignment:0;
        if(!alignment||frame>count){error="PCM seek frame is outside the clip";return false;}
        auto scratch=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::WorkingCpu,65536,error);if(!scratch)return false;
        auto gain=getVolume(),pan=getPan();if(auto manager=workphone::dynamic_pointer_cast<SoundManager>(getOwner()))manager->getMixParameters(this,gain,pan);
        void *object=nullptr;if(auto manager=getOwner())manager->_getObject(&object);auto engine=static_cast<IXAudio2*>(object);
        IXAudio2SourceVoice *candidate=nullptr;
        const auto format=state->range?state->range->format():state->wav->format.data();
        if(!engine||FAILED(engine->CreateSourceVoice(&candidate,reinterpret_cast<const WAVEFORMATEX*>(format),0,XAUDIO2_DEFAULT_FREQ_RATIO,&state->callback))){error="Native audio seek voice staging failed";return false;}
        const bool resume=state->playing.load();state->playing=false;
        if(state->sourceVoice){state->sourceVoice->Stop(0);state->sourceVoice->DestroyVoice();}
        state->sourceVoice=candidate;state->startFrame=frame;state->preserveStart=true;state->paused=false;
        if(state->range){if(!state->range->reset(frame,error))return false;state->range->process(state->loop);}
        else {state->streamCursor=frame*alignment;state->buffer.PlayBegin=static_cast<UINT32>(frame);}
        state->applyMix(gain,pan);if(resume&&frame<count)play();return true;
#else
        (void)frame;error="Frame seeking is unavailable on this native audio backend";return false;
#endif
    }
    catch(const std::exception &failure){audioFailure(error,failure.what());return false;}
    u64 WPAudioSound::getPlaybackFrame() const
    {
        const auto state=m_platformSoundState;if(!state)return 0;
        if(state->range)return state->range->ring().stats().position/state->range->blockAlign();
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        if(state->sourceVoice&&state->wav&&state->wav->blockAlign){XAUDIO2_VOICE_STATE value{};state->sourceVoice->GetState(&value);
            const auto frames=state->wav->samples.size()/state->wav->blockAlign;const auto at=state->startFrame+value.SamplesPlayed;
            return state->loop&&frames?at%frames:std::min<u64>(at,frames);}
#endif
        return 0;
    }
    WPAudioSound::StreamingStats WPAudioSound::getStreamingStats() const
    {
        StreamingStats out;const auto state=m_platformSoundState;if(!state||!state->range)return out;
        const auto stats=state->range->ring().stats();out.streamed=true;out.finished=stats.finished;
        out.residentPcmBytes=state->range->residentBytes();out.submittedBytes=stats.submitted;out.completedBytes=stats.completed;
        out.underruns=stats.underruns;out.silenceBytes=stats.silenceBytes;out.readFailures=state->range->readFailures();out.admissionFailures=state->range->admissionFailures();return out;
    }

    bool WPAudioSound::loadPlatformSound( const String &filename )
    {
        String admissionError;
        u64 voiceBytes=65536;
#if defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        if(m_cookedResource)voiceBytes+=m_cookedResource->payload.size();
#endif
        auto voiceAdmission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,voiceBytes,admissionError);
        if(!voiceAdmission)return false;
        auto state = new( std::nothrow ) PlatformSoundState();
        if( !state )
        {
            return false;
        }
        state->admission=std::move(voiceAdmission);
        state->loop = getLoop();

        try
        {
            if(m_cookedResource&&m_cookedResource->streamData){
                state->range=audio::AudioResourceStream::create(m_cookedResource,m_runtimeMount,admissionError);
                if(!state->range){delete state;return false;}
            }else {
            StreamBudget working;
            if(m_cookedResource){String error;working=WorldStreamingScheduler::shared()->reserve(
                StreamMemoryClass::WorkingCpu,u64(m_cookedResource->payload.size())+65536,error);
                if(!working){delete state;return false;}}
            auto stream = m_cookedResource ? SmartPtr<IStream>() : openAudioStream( filename );
            auto candidate = std::make_shared<WavData>();
            if( !(m_cookedResource ? readWavBytes(m_cookedResource->payload,*candidate) : readWav( stream.get(), *candidate )) )
            {
                delete state;
                return false;
            }
            const auto bytes = candidate->samples.size() + candidate->format.size();
            const auto result = clipCache.intern(
                candidate, bytes,
                []( const WavData &a, const WavData &b ) {
                    return a.format == b.format && a.samples == b.samples;
                },
                state->wav );
            if( result != audio::SharedClipCache<WavData>::Result::Ready )
            {
                WP_LOG_WARNING( "WPAudio: resident clip budget exceeded." );
                delete state;
                return false;
            }
            }
        }
        catch( const std::bad_alloc & )
        {
            delete state;
            return false;
        }
        auto candidateGain=getVolume(),candidatePan=getPan();
        try{if(auto manager=workphone::dynamic_pointer_cast<SoundManager>(getOwner()))manager->getMixParameters(this,candidateGain,candidatePan);
#if !defined WP_PLATFORM_WIN32
            if(auto owner=getOwner())candidateGain*=owner->isMute()?0.0f:owner->getVolume();
#endif
        }catch(const std::exception&){delete state;return false;}

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        void *audioObject = nullptr;
        if( auto owner = getOwner() )
        {
            owner->_getObject( &audioObject );
        }
        auto xAudio2 = static_cast<IXAudio2 *>( audioObject );
        if( !xAudio2 ||
            FAILED( xAudio2->CreateSourceVoice(
                &state->sourceVoice, reinterpret_cast<const WAVEFORMATEX *>( state->range?state->range->format():state->wav->format.data() ),
                0, XAUDIO2_DEFAULT_FREQ_RATIO, &state->callback ) ) )
        {
            delete state;
            return false;
        }

        state->buffer.AudioBytes = state->wav?static_cast<UINT32>( state->wav->samples.size() ):0;
        state->buffer.pAudioData = state->wav?state->wav->samples.data():nullptr;
        state->buffer.Flags = XAUDIO2_END_OF_STREAM;
        state->buffer.LoopCount = state->loop ? XAUDIO2_LOOP_INFINITE : 0;
        if( auto manager = workphone::dynamic_pointer_cast<SoundManager>( getOwner() ) )
        {
            state->streaming = bool(state->range)||manager->isStreamingEnabled();
            state->streamBlockBytes = static_cast<size_t>( manager->getPlaybackBufferFrames() ) *
                                      (state->range?state->range->blockAlign():state->wav->channels*state->wav->bitsPerSample/8);
            state->streamBlockBytes = std::max<size_t>( state->streamBlockBytes,
                                                       state->range?state->range->blockAlign():state->wav->channels*state->wav->bitsPerSample/8 );
        }
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        if( state->wav->formatTag != WavFormatPcm )
        {
            delete state;
            return false;
        }

        AudioStreamBasicDescription format = {};
        format.mSampleRate = state->wav->sampleRate;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsSignedInteger;
        if( state->wav->bitsPerSample == 8 )
        {
            format.mFormatFlags = kLinearPCMFormatFlagIsPacked;
        }
        format.mBytesPerPacket = state->wav->blockAlign;
        format.mFramesPerPacket = 1;
        format.mBytesPerFrame = state->wav->blockAlign;
        format.mChannelsPerFrame = state->wav->channels;
        format.mBitsPerChannel = state->wav->bitsPerSample;

        if( AudioQueueNewOutput( &format, &PlatformSoundState::outputCallback, state, nullptr, nullptr,
                                 0, &state->queue ) != noErr ||
            !state->queue ||
            AudioQueueAllocateBuffer( state->queue, static_cast<UInt32>( state->wav->samples.size() ),
                                      &state->buffer ) != noErr )
        {
            if( state->queue )
            {
                AudioQueueDispose( state->queue, true );
            }
            delete state;
            return false;
        }
        std::memcpy( state->buffer->mAudioData, state->wav->samples.data(), state->wav->samples.size() );
        state->buffer->mAudioDataByteSize = static_cast<UInt32>( state->wav->samples.size() );
#elif defined WP_PLATFORM_ANDROID
        if( state->wav->formatTag != WavFormatPcm || state->wav->channels > 2 ||
            ( state->wav->bitsPerSample != 8 && state->wav->bitsPerSample != 16 ) )
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
        const SLuint32 speakerMask = state->wav->channels == 1
                                         ? SL_SPEAKER_FRONT_CENTER
                                         : SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,         state->wav->channels,      state->wav->sampleRate * 1000,
            state->wav->bitsPerSample, state->wav->bitsPerSample, speakerMask,
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

        // Publish only after parsing, budget admission and native voice creation
        // succeed. Existing clones keep the previous immutable generation pinned.
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        state->applyMix(candidateGain,candidatePan);
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        AudioQueueSetParameter(state->queue,kAudioQueueParam_Volume,candidateGain);
#elif defined WP_PLATFORM_ANDROID
        const auto gain=candidateGain<=0.0f?SL_MILLIBEL_MIN:static_cast<SLmillibel>(2000.0f*std::log10(candidateGain));
        (*state->volume)->SetVolumeLevel(state->volume,gain);
#endif
        unloadPlatformSound();
        m_platformSoundState = state;
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
        if( auto manager = workphone::dynamic_pointer_cast<SoundManager>( getOwner() ) )
            if( !manager->admitVoice( this ) ) return;
        if(state->range){
            if(!state->range->owner())return;
            if(state->range->ring().stats().finished){String error;if(!seekFrames(0,error))return;state=m_platformSoundState;}
            state->range->process(state->loop);
        }

        // Set this before starting the native backend because very short buffers can invoke
        // their completion callback before the start call returns.
        state->playing = true;

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        XAUDIO2_VOICE_STATE voiceState = {};
        state->sourceVoice->GetState( &voiceState );
        if( voiceState.BuffersQueued == 0 && state->streaming && !state->range&&!state->preserveStart ) state->streamCursor = 0;
        if( voiceState.BuffersQueued == 0 && ( state->streaming ? !state->submitStreamBlock() :
            FAILED( state->sourceVoice->SubmitSourceBuffer( &state->buffer ) ) ) )
        {
            state->playing = false;
            return;
        }
        state->preserveStart=false;
        if( FAILED( state->sourceVoice->Start( 0 ) ) )
        {
            state->playing = false;
            return;
        }
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        if( !state->paused )
        {
            AudioQueueReset( state->queue );
        }
        if( ( !state->paused &&
              AudioQueueEnqueueBuffer( state->queue, state->buffer, 0, nullptr ) != noErr ) ||
            AudioQueueStart( state->queue, nullptr ) != noErr )
        {
            state->playing = false;
            return;
        }
#elif defined WP_PLATFORM_ANDROID
        if( !state->paused )
        {
            ( *state->bufferQueue )->Clear( state->bufferQueue );
        }
        if( ( !state->paused &&
              ( *state->bufferQueue )
                      ->Enqueue( state->bufferQueue, state->wav->samples.data(),
                                 static_cast<SLuint32>( state->wav->samples.size() ) ) !=
                  SL_RESULT_SUCCESS ) ||
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
        state->paused = false;
        Sound::play();
    }

    void WPAudioSound::pause()
    {
        auto state = m_platformSoundState;
        if( !state || !state->playing )
        {
            return;
        }
        state->paused = true;
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
        if(state&&state->range){state->playing=false;String error;(void)seekFrames(0,error);Sound::stop();return;}
        if( state )
        {
            state->playing = false;
            state->paused = false;
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
        Sound::setVolume( std::isfinite( volume ) ? volume : 0.0f );
        auto state = m_platformSoundState;
        if( !state )
        {
            return;
        }
        auto effectiveGain = getVolume();
        auto effectivePan = getPan();
        if( auto manager = workphone::dynamic_pointer_cast<SoundManager>( getOwner() ) )
            manager->getMixParameters( this, effectiveGain, effectivePan );
#if !defined WP_PLATFORM_WIN32
        if( auto owner = getOwner() )
        {
            effectiveGain *= owner->isMute() ? 0.0f : owner->getVolume();
        }
#endif
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        state->applyMix(effectiveGain,effectivePan);
#elif defined WP_PLATFORM_APPLE || defined WP_PLATFORM_IOS
        AudioQueueSetParameter( state->queue, kAudioQueueParam_Volume, effectiveGain );
#elif defined WP_PLATFORM_ANDROID
        const auto gain = effectiveGain <= 0.0f
                              ? SL_MILLIBEL_MIN
                              : static_cast<SLmillibel>( 2000.0f * std::log10( effectiveGain ) );
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
            state->buffer.LoopCount = !state->streaming && loop ? XAUDIO2_LOOP_INFINITE : 0;
            // ExitLoop affects the submitted buffer; changing the descriptor alone does not.
            // Enabling looping after submission takes effect on the next play/restart.
            if( !loop && state->sourceVoice )
            {
                state->sourceVoice->ExitLoop();
            }
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
