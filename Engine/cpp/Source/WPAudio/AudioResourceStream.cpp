#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/AudioResourceStream.hpp>
#include <WPAudio/AudioResourceCompiler.hpp>
#include <Workphone/System/ResourceChunkStream.hpp>
#include <Workphone/System/ResourceLeafProvider.hpp>
#include <workphone_audio_core.h>
#include <limits>
#include <thread>
#include <cstring>

namespace workphone::audio
{
    namespace
    {
        constexpr u64 minimumStreamBytes=512ull<<10,bootstrapBytes=2*AudioPcmRing::blockBytes;
        void report(String &error,const char *message) noexcept
        {try{error=message;}catch(...){try{error="Audio stream failed";}catch(...){error.clear();}}}
        struct Snapshot final : resource::RuntimeResourceStreamData
        {
            StreamBudget admission;
            resource::CompiledResourceHeader header;
            std::shared_ptr<resource::ResourcePayloadMount> mount;
            Array<u8> bootstrap;
            std::array<u8,18> format{};
            u64 dataOffset=0,dataBytes=0;
            u32 alignment=0;
            bool cold=false;
            u64 initialReadBytes=0;
        };
        bool same(const resource::CompiledResourceHeader &a,const resource::CompiledResourceHeader &b)
        {return a.resourceId==b.resourceId&&a.resourceType==b.resourceType&&a.compilerVersion==b.compilerVersion&&
            a.sourceHash==b.sourceHash&&a.payloadHash==b.payloadHash&&a.payloadSize==b.payloadSize;}
        void field16(std::array<u8,18> &out,size_t at,u16 value){out[at]=u8(value);out[at+1]=u8(value>>8);}
        void field32(std::array<u8,18> &out,size_t at,u32 value){for(size_t i=0;i<4;++i)out[at+i]=u8(value>>(8*i));}
        std::shared_ptr<const Snapshot> snapshot(const resource::RuntimeResource &resource)
        {
            auto value=std::dynamic_pointer_cast<const Snapshot>(resource.streamData);
            if(!value||!resource.payload.empty()||!resource.dependencies.empty()||!same(value->header,resource.header)||
                resource.header.resourceId.isSubResource()||!resource.header.installDependencies.empty()||!resource.header.installArtifacts.empty()||
                !value->mount||!value->alignment||!value->dataBytes||value->dataBytes%value->alignment||
                value->dataOffset>resource.header.payloadSize||value->dataBytes>resource.header.payloadSize-value->dataOffset||
                value->bootstrap.empty()||value->bootstrap.size()>bootstrapBytes||value->bootstrap.size()>value->dataBytes)return {};
            return value;
        }
        std::shared_ptr<const resource::RuntimeResource> captured(
            const resource::CompiledResourceHeader &header,std::shared_ptr<resource::ResourcePayloadMount> mount,
            const std::array<u8,18> &format,u32 alignment,u64 dataOffset,u64 dataBytes,
            const u8 *bootstrap,size_t count,u64 initialReadBytes,String &error)
        {
            auto lease=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,count+16384,error);if(!lease)return {};
            auto value=std::make_shared<Snapshot>();value->admission=std::move(lease);value->header=header;value->mount=std::move(mount);
            value->format=format;value->alignment=alignment;value->dataOffset=dataOffset;value->dataBytes=dataBytes;
            value->cold=true;value->initialReadBytes=initialReadBytes;
            value->bootstrap.resize(count);std::memcpy(value->bootstrap.data(),bootstrap,count);
            auto result=std::make_shared<resource::RuntimeResource>();result->header=header;result->streamData=std::move(value);return result;
        }
        class ColdProvider final : public resource::ResourceLeafProvider
        {
        public:
            ColdProvider(StreamBudget admission,std::shared_ptr<resource::ResourcePayloadMount> mount,
                resource::CompiledResourceHeader header,std::unique_ptr<resource::ResourceChunkStream> chunks,s32 priority):
                m_admission(std::move(admission)),m_mount(std::move(mount)),m_header(std::move(header)),m_chunks(std::move(chunks)),m_priority(priority){}
            u64 maximumResidentBytes() const override{return bootstrapBytes+16384;}
            void setPriority(s32 value) override{m_priority=value;if(m_waiting)m_chunks->setPriority(m_waiting,value);}
            void cancel() noexcept override{m_cancelled=true;try{m_chunks->cancel();}catch(...){}}
            void drain() noexcept override{try{m_chunks->retireAfterOwnerQuiesced();}catch(...){}}
            resource::ResourceLeafStatus poll(std::shared_ptr<const resource::RuntimeResource> &result,String &error) override
            {
                result.reset();error.clear();try {
                    if(m_cancelled)throw std::runtime_error("Cold PCM loading was cancelled");
                    if(m_result){result=m_result;return resource::ResourceLeafStatus::Ready;}
                    if(m_waiting){resource::ResourceChunkResult read;
                        if(!m_chunks->take(read))return resource::ResourceLeafStatus::Pending;
                        if(read.key!=m_waiting||read.resourceId!=m_header.resourceId||!read.resource||!read.error.empty()||
                            read.offset!=m_rangeOffset||read.resource->offset!=m_rangeOffset||read.resource->bytes.size()!=m_rangeLength||
                            read.resource->resourceId!=m_header.resourceId||read.resource->payloadSize!=m_header.payloadSize||read.resource->legacyWholePayloadFallback)
                            throw std::runtime_error(read.error.empty()?"Cold PCM verified metadata read failed":read.error.c_str());
                        m_readBytes+=read.resource->payloadBytesRead;m_window=std::move(read.resource);m_waiting=0;
                    }
                    if(!m_started){if(!available(0,12))return request(0,error);
                        const auto bytes=at(0);if(std::memcmp(bytes,"RIFF",4)||std::memcmp(bytes+8,"WAVE",4)||u64(integer(bytes+4))+8!=m_header.payloadSize)
                            throw std::runtime_error("Cold PCM RIFF size or identity is invalid");
                        m_started=true;m_at=12;
                    }
                    while(m_at<m_header.payloadSize){
                        if(m_header.payloadSize-m_at<8)throw std::runtime_error("Cold PCM trailing chunk header is truncated");
                        if(!available(m_at,8))return request(m_at,error);
                        const auto bytes=at(m_at);const auto size=integer(bytes+4);const u64 payload=m_at+8;
                        if(u64(size)+(size&1)>m_header.payloadSize-payload)throw std::runtime_error("Cold PCM chunk exceeds its sealed payload");
                        if(!std::memcmp(bytes,"fmt ",4)){
                            if(m_formatSeen||size<16||size==17||size>18)throw std::runtime_error("Cold PCM format chunk is duplicate or unsupported");
                            if(!available(payload,size))return request(m_at,error);
                            const auto value=at(payload);const auto tag=shortInteger(value),channels=shortInteger(value+2),bits=shortInteger(value+14);
                            const auto rate=integer(value+4);const auto alignment=shortInteger(value+12);
                            if(tag!=1||(channels!=1&&channels!=2)||bits!=16||rate<8000||rate>192000||alignment!=channels*2||
                                integer(value+8)!=rate*alignment||(size==18&&shortInteger(value+16)!=0))throw std::runtime_error("Cold PCM requires mono/stereo PCM16 at 8-192 kHz");
                            field16(m_format,0,tag);field16(m_format,2,channels);field32(m_format,4,rate);field32(m_format,8,rate*alignment);field16(m_format,12,alignment);field16(m_format,14,bits);
                            m_alignment=alignment;m_formatSeen=true;
                        }else if(!std::memcmp(bytes,"data",4)){
                            if(m_dataSeen||!size)throw std::runtime_error("Cold PCM data chunk is duplicate or empty");
                            m_dataSeen=true;m_dataOffset=payload;m_dataBytes=size;
                        }
                        if(++m_chunkCount>1024)throw std::runtime_error("Cold PCM RIFF metadata exceeds 1024 chunks");
                        m_at=payload+size+(size&1);
                    }
                    if(!m_formatSeen||!m_dataSeen||m_dataBytes%m_alignment)throw std::runtime_error("Cold PCM is missing aligned format/data chunks");
                    const auto count=static_cast<size_t>(std::min<u64>(bootstrapBytes,m_dataBytes));
                    if(!available(m_dataOffset,count))return request(m_dataOffset,error,count);
                    m_result=captured(m_header,m_mount,m_format,m_alignment,m_dataOffset,m_dataBytes,at(m_dataOffset),count,m_readBytes,error);
                    if(!m_result)throw std::runtime_error(error.empty()?"Cold PCM snapshot residency was refused":error.c_str());
                    m_window.reset();result=m_result;return resource::ResourceLeafStatus::Ready;
                }catch(const std::exception &failure){report(error,failure.what());cancel();return resource::ResourceLeafStatus::Error;}
            }
        private:
            static u32 integer(const u8 *value){return u32(value[0])|(u32(value[1])<<8)|(u32(value[2])<<16)|(u32(value[3])<<24);}
            static u16 shortInteger(const u8 *value){return u16(value[0])|(u16(value[1])<<8);}
            bool available(u64 offset,u64 count) const{return m_window&&offset>=m_window->offset&&offset-m_window->offset<=m_window->bytes.size()&&count<=m_window->bytes.size()-(offset-m_window->offset);}
            const u8 *at(u64 offset) const{return m_window->bytes.data()+offset-m_window->offset;}
            resource::ResourceLeafStatus request(u64 offset,String &error,u64 count=65536)
            {
                if(m_nextKey==std::numeric_limits<u64>::max())throw std::runtime_error("Cold PCM request identity exhausted");
                const auto length=std::min<u64>(count,m_header.payloadSize-offset);const auto key=m_nextKey++;
                if(!m_chunks->request(key,m_header.resourceId,offset,length,m_priority,-100,error))throw std::runtime_error(error.empty()?"Cold PCM range admission was refused":error.c_str());
                m_waiting=key;m_rangeOffset=offset;m_rangeLength=length;return resource::ResourceLeafStatus::Pending;
            }
            StreamBudget m_admission;
            std::shared_ptr<resource::ResourcePayloadMount> m_mount;
            resource::CompiledResourceHeader m_header;
            std::unique_ptr<resource::ResourceChunkStream> m_chunks;
            std::shared_ptr<const resource::RuntimeResourceChunk> m_window;
            std::shared_ptr<const resource::RuntimeResource> m_result;
            std::array<u8,18> m_format{};
            u64 m_at=0,m_dataOffset=0,m_dataBytes=0,m_nextKey=1,m_waiting=0,m_readBytes=0,m_rangeOffset=0,m_rangeLength=0;
            u32 m_alignment=0,m_chunkCount=0;
            s32 m_priority=0;
            bool m_started=false,m_formatSeen=false,m_dataSeen=false,m_cancelled=false;
        };
        class ColdFactory final : public resource::ResourceLeafProviderFactory
        {
        public:
            explicit ColdFactory(std::shared_ptr<resource::ResourcePayloadMount> mount):m_mount(std::move(mount)){}
            bool handles(const resource::ResourceID &id) const override
            {
                if(id.type()!=resource::ResourceTypeID("wav"))return false;
                resource::RuntimeResourceRangeInfo info;String error;
                // A failed eligible lookup must fail in create(), never select
                // whole-payload fallback because its admission was refused.
                if(!m_mount->resourceInfo(id,info,error))return true;
                return info.independentlyVerified&&info.header.payloadSize>=minimumStreamBytes&&id.str().size()<=1024;
            }
            std::shared_ptr<resource::ResourceLeafProvider> create(std::shared_ptr<resource::ResourcePayloadMount> mount,
                const resource::ResourceID &id,const resource::RuntimeResourceRangeInfo &info,s32 priority,String &error) override
            {
                error.clear();try {
                    if(!mount)mount=m_mount;
                    if(!info.independentlyVerified||!info.header.isValid()||info.header.resourceId!=id||id.isSubResource()||
                        info.header.resourceType!=resource::ResourceTypeID("wav")||info.header.compilerVersion!=AudioResourceCompiler().versionFor(resource::ResourceTypeID("wav"))||
                        !info.header.installDependencies.empty()||!info.header.installArtifacts.empty()||info.header.payloadSize>64ull*1024*1024||info.header.payloadSize<12)
                        throw std::runtime_error("Unsupported cold PCM header or independently verified directory");
                    auto view=mount->createReadOnlyOwnerView(error);if(!view)return {};
                    resource::RuntimeResourceRangeInfo current;if(!view->resourceInfo(id,current,error))return {};
                    if(!same(current.header,info.header)||!current.independentlyVerified)throw std::runtime_error("Cold PCM mount generation changed");
                    auto chunks=resource::ResourceChunkStream::create(view,error);if(!chunks)return {};
                    auto lease=chunks->reserveOwner(StreamMemoryClass::SourceCpu,65536,error);if(!lease)return {};
                    return std::allocate_shared<ColdProvider>(SourceBudgetAllocator<ColdProvider>{},std::move(lease),std::move(view),info.header,std::move(chunks),priority);
                }catch(const std::exception &failure){report(error,failure.what());return {};}
            }
        private:
            std::shared_ptr<resource::ResourcePayloadMount> m_mount;
        };
    }
    struct AudioResourceStream::Impl
    {
        StreamBudget admission;
        std::shared_ptr<const Snapshot> source;
        std::shared_ptr<resource::ResourcePayloadMount> mount;
        std::unique_ptr<resource::ResourceChunkStream> chunks;
        AudioPcmRing ring;
        std::array<u64,AudioPcmRing::slotCount> pending{};
        u64 nextKey=1,nextSequence=0,nextOffset=0,readFailed=0,admissionFailed=0;
        std::thread::id owner=std::this_thread::get_id();
        void fill(bool loop)
        {
            if(ring.stats().finished)return;
            for(size_t pass=0;pass<AudioPcmRing::slotCount;++pass){
                if(nextOffset==source->dataBytes){if(!loop)return;nextOffset=0;}
                if(nextSequence==std::numeric_limits<u64>::max())return;
                const auto count=static_cast<u32>(std::min<u64>(AudioPcmRing::blockBytes,source->dataBytes-nextOffset));
                const auto slot=ring.book(nextSequence,nextOffset,count,nextOffset+count==source->dataBytes);
                if(slot==AudioPcmRing::slotCount)return;
                if(nextOffset<=source->bootstrap.size()&&count<=source->bootstrap.size()-nextOffset){ring.fill(slot,source->bootstrap.data()+nextOffset,count);}
                else {
                    String error;if(nextKey==std::numeric_limits<u64>::max()){ring.cancelFill(slot);return;}
                    const auto key=nextKey++;
                    if(!chunks->request(key,source->header.resourceId,source->dataOffset+nextOffset,count,100,-100,error)){
                        ++admissionFailed;ring.cancelFill(slot);return;}
                    pending[slot]=key;
                }
                ++nextSequence;nextOffset+=count;
            }
        }
    };
    AudioResourceStream::AudioResourceStream()=default;
    AudioResourceStream::~AudioResourceStream()=default;
    bool AudioResourceStream::isSnapshot(const resource::RuntimeResource &value){return bool(snapshot(value));}
    AudioResourceStream::CaptureStats AudioResourceStream::captureStats(const resource::RuntimeResource &resource)
    {CaptureStats out;if(auto value=snapshot(resource)){out.cold=value->cold;out.initialVerifiedReadBytes=value->initialReadBytes;out.bootstrapBytes=value->bootstrap.size();}return out;}
    std::shared_ptr<resource::ResourceLeafProviderFactory> AudioResourceStream::leafProviderFactory(
        std::shared_ptr<resource::ResourcePayloadMount> mount,String &error)
    {
        error.clear();if(!mount){error="Cold PCM factory requires a captured mount";return {};}
#if !defined(WP_PLATFORM_WIN32) || !WP_USE_XAUDIO2
        return {};
#else
        try{return std::allocate_shared<ColdFactory>(SourceBudgetAllocator<ColdFactory>{},std::move(mount));}
        catch(const std::exception &failure){report(error,failure.what());return {};}
#endif
    }
    std::shared_ptr<const resource::RuntimeResource> AudioResourceStream::capture(
        std::shared_ptr<const resource::RuntimeResource> resource,std::shared_ptr<resource::ResourcePayloadMount> mount,String &error)
    {
        error.clear();try {
            if(!resource){error="Missing audio input";return {};}
            if(resource->streamData){if(!snapshot(*resource)){error="Invalid native PCM stream snapshot";return {};}return resource;}
            if(!AudioResourceCompiler::validate(*resource,error))return {};
            if(!mount||resource->payload.size()<minimumStreamBytes||resource->header.resourceId.str().size()>1024)return resource;
#if !defined(WP_PLATFORM_WIN32) || !WP_USE_XAUDIO2
            return resource;
#else
            resource::RuntimeResourceRangeInfo info;
            if(!mount->resourceInfo(resource->header.resourceId,info,error))return {};
            if(!same(info.header,resource->header)){error="PCM stream mount does not match its captured generation";return {};}
            if(!info.independentlyVerified)return resource;
            wp_audio_wav_info wav{};wav.size=sizeof(wav);
            if(wp_audio_wav_inspect(resource->payload.data(),resource->payload.size(),&wav)!=WP_AUDIO_OK){error="Invalid PCM stream RIFF metadata";return {};}
            const auto boot=static_cast<size_t>(std::min<u64>(bootstrapBytes,wav.data_bytes));
            auto admission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,boot+16384,error);if(!admission)return {};
            auto value=std::make_shared<Snapshot>();value->admission=std::move(admission);value->header=resource->header;
            value->initialReadBytes=resource->header.payloadSize;
            value->mount=std::move(mount);value->alignment=wav.block_align;value->dataOffset=wav.data_offset;value->dataBytes=wav.data_bytes;
            value->bootstrap.resize(boot);std::memcpy(value->bootstrap.data(),resource->payload.data()+wav.data_offset,boot);
            field16(value->format,0,wav.format_tag);field16(value->format,2,wav.channels);field32(value->format,4,wav.sample_rate);
            field32(value->format,8,wav.sample_rate*wav.block_align);field16(value->format,12,wav.block_align);field16(value->format,14,wav.bits_per_sample);
            auto result=std::make_shared<resource::RuntimeResource>();result->header=resource->header;result->streamData=std::move(value);return result;
#endif
        }catch(const std::exception &failure){report(error,failure.what());return {};}
    }
    std::unique_ptr<AudioResourceStream> AudioResourceStream::create(
        std::shared_ptr<const resource::RuntimeResource> resource,std::shared_ptr<resource::ResourcePayloadMount> mount,String &error)
    {
        error.clear();try {
            const auto source=resource?snapshot(*resource):nullptr;if(!source){error="PCM streaming requires a sealed sparse audio snapshot";return {};}
            if(!mount)mount=source->mount;
            auto view=mount->createReadOnlyOwnerView(error);if(!view)return {};
            resource::RuntimeResourceRangeInfo info;if(!view->resourceInfo(source->header.resourceId,info,error))return {};
            if(!info.independentlyVerified||!same(info.header,source->header)){error="PCM stream generation or independent chunk pins changed";return {};}
            auto lease=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,sizeof(Impl)+65536,error);if(!lease)return {};
            auto result=std::unique_ptr<AudioResourceStream>(new AudioResourceStream);result->m_impl=std::make_unique<Impl>();result->m_impl->admission=std::move(lease);
            result->m_impl->source=source;result->m_impl->mount=std::move(view);result->m_impl->chunks=resource::ResourceChunkStream::create(result->m_impl->mount,error);if(!result->m_impl->chunks)return {};
            result->m_impl->ring.reset();result->m_impl->fill(false);return result;
        }catch(const std::exception &failure){report(error,failure.what());return {};}
    }
    void AudioResourceStream::process(bool loop)
    {
        if(!owner())return;auto &state=*m_impl;
        try {
            resource::ResourceChunkResult result;
            while(state.chunks->take(result))for(size_t i=0;i<AudioPcmRing::slotCount;++i)if(state.pending[i]==result.key){
                state.pending[i]=0;const auto &slot=state.ring.slot(i);
                if(!result.resource||result.resourceId!=state.source->header.resourceId||result.offset!=state.source->dataOffset+slot.offset||
                    result.resource->bytes.size()!=slot.size||!state.ring.fill(i,result.resource->bytes.data(),result.resource->bytes.size()))++state.readFailed;
                break;
            }
            for(size_t i=0;i<AudioPcmRing::slotCount;++i)if(!state.pending[i]&&state.ring.slot(i).state.load()==AudioPcmRing::Filling){
                if(state.nextKey==std::numeric_limits<u64>::max())return;const auto key=state.nextKey++;const auto &slot=state.ring.slot(i);String error;
                if(state.chunks->request(key,state.source->header.resourceId,state.source->dataOffset+slot.offset,slot.size,200,-100,error))state.pending[i]=key;else ++state.admissionFailed;
            }
            state.fill(loop);
        }catch(const std::exception&){++state.readFailed;}
    }
    bool AudioResourceStream::reset(u64 frame,String &error)
    {
        error.clear();if(!owner()||frame>frames()){error="PCM seek requires its control owner and an in-range frame";return false;}
        auto &state=*m_impl;state.chunks->cancel();state.pending={};state.nextOffset=frame*blockAlign();state.nextSequence=0;state.ring.reset(state.nextOffset);
        if(frame==frames())state.ring.finish();else try{state.fill(false);}catch(const std::exception&){++state.admissionFailed;}return true;
    }
    AudioPcmRing &AudioResourceStream::ring(){return m_impl->ring;}
    const AudioPcmRing &AudioResourceStream::ring() const{return m_impl->ring;}
    const u8 *AudioResourceStream::format() const{return m_impl->source->format.data();}
    u32 AudioResourceStream::blockAlign() const{return m_impl->source->alignment;}
    u64 AudioResourceStream::frames() const{return m_impl->source->dataBytes/blockAlign();}
    u64 AudioResourceStream::readFailures() const{return m_impl->readFailed;}
    u64 AudioResourceStream::admissionFailures() const{return m_impl->admissionFailed;}
    u64 AudioResourceStream::residentBytes() const{return sizeof(Impl)+m_impl->source->bootstrap.size();}
    bool AudioResourceStream::owner() const{return m_impl&&m_impl->owner==std::this_thread::get_id();}
}
