#pragma once
#include <Workphone/System/ResourcePayloadStream.hpp>
#include <cstring>

namespace workphone::resource
{
    struct ResourceChunkResult
    {
        u64 key=0,generation=0,offset=0;
        ResourceID resourceId;
        std::shared_ptr<const RuntimeResourceChunk> resource;
        String error;
    };
    struct ResourceChunkProgress
    {
        u64 requestedBytes=0,readyBytes=0,reservedBytes=0,ioBytes=0;
        u32 pendingChunks=0;
        bool assembling=false,complete=false;
    };
    namespace chunk_stream_detail
    {
        template<class K,class V>using Map=payload_stream_detail::Map<K,V>;
        using Key=std::pair<String,u64>;
        struct Priority {s32 request=0,resident=0;};
        struct Work
        {
            StreamBudget metadata,reservation;
            Key identity;
            u64 key=0,length=0;
            bool complete=false,cacheHit=false;
            std::shared_ptr<const RuntimeResourceRangeInfo> info;
            std::shared_ptr<const RuntimeResourceChunk> resource;
            String error;
            Map<u64,Priority> interests;
            s32 priority() const {s32 p=std::numeric_limits<s32>::min();for(const auto &i:interests)p=std::max(p,i.second.request);return p;}
        };
        struct Piece {std::shared_ptr<Work> work;u64 subscription=0;};
        struct Request
        {
            StreamBudget metadata;
            ResourceID id;
            u64 key=0,generation=0,offset=0,length=0,assemblyKey=0;
            s32 priority=0,residentPriority=0;
            bool assembling=false,complete=false;
            Array<Piece> pieces{Allocator<Piece>(sourceBudget)};
            std::shared_ptr<const RuntimeResourceChunk> resource;
            String error;
        };
    }

    class ResourceChunkCache final : public SourceBudgetObject
    {
        friend class ResourceChunkStream;
        template<class K,class V>using Map=chunk_stream_detail::Map<K,V>;
        using Key=chunk_stream_detail::Key;using Work=chunk_stream_detail::Work;using Request=chunk_stream_detail::Request;
        struct Resident {StreamBudget metadata;std::shared_ptr<const RuntimeResourceChunk> resource;s32 priority=0;u64 access=0;};
        struct Held {StreamBudget reservation,source;std::shared_ptr<const RuntimeResourceChunk> resource;};
        struct Assembly
        {
            StreamBudget admission;
            ResourceID id;
            u64 offset=0,length=0,payloadSize=0,ioBytes=0;
            Array<std::shared_ptr<const RuntimeResourceChunk>> chunks{Allocator<std::shared_ptr<const RuntimeResourceChunk>>(sourceBudget)};
        };
        ResourceChunkCache()=default;
        bool owner() const{return std::this_thread::get_id()==m_owner;}
        static void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}
        static void report(String &out,const char *message) noexcept
        {try{out=message;}catch(...){try{out="Chunk operation failed";}catch(...){out.clear();}}}
        std::shared_ptr<const RuntimeResourceRangeInfo> info(const ResourceID &id,String &error)
        {
            const auto old=m_info.find(id.str());if(old!=m_info.end())if(auto value=old->second.lock())return value;
            auto value=std::allocate_shared<RuntimeResourceRangeInfo>(SourceBudgetAllocator<RuntimeResourceRangeInfo>{});
            if(!m_mount->resourceInfo(id,*value,error))return {};
            m_info[id.str()]=value;return value;
        }
        void pump()
        {
            WorldStreamResult value;
            while(m_producer->take(value)) {
                const auto work=m_byKey.find(value.key);
                if(work!=m_byKey.end()) {
                    work->second->complete=true;work->second->error=std::move(value.error);
                    if(value.payload)work->second->resource=std::shared_ptr<const RuntimeResourceChunk>(value.payload,static_cast<const RuntimeResourceChunk*>(value.payload.get()));
                    work->second->reservation.reset();continue;
                }
                const auto assembly=m_assemblies.find(value.key);if(assembly==m_assemblies.end())continue;
                if(auto request=assembly->second.lock()) {
                    request->complete=true;request->assembling=false;request->error=std::move(value.error);
                    if(value.payload)request->resource=std::shared_ptr<const RuntimeResourceChunk>(value.payload,static_cast<const RuntimeResourceChunk*>(value.payload.get()));
                }
                m_assemblies.erase(assembly);
            }
            for(auto i=m_info.begin();i!=m_info.end();)if(i->second.expired())i=m_info.erase(i);else ++i;
        }
        void evict()
        {
            auto victim=m_resident.begin();for(auto i=m_resident.begin();i!=m_resident.end();++i)
                if(i->second.priority<victim->second.priority||(i->second.priority==victim->second.priority&&i->second.access<victim->second.access))victim=i;
            m_residentBytes-=victim->second.resource->bytes.size();m_resident.erase(victim);
        }
        void trim(){while(m_resident.size()>m_residentEntries||m_residentBytes>m_residentLimit)evict();}
        void room(u64 bytes)
        {
            const auto r=size_t(StreamMemoryClass::ResultCpu);
            while(!m_resident.empty()){const auto state=m_scheduler->stats();if(bytes<=state.limits[r]-state.used[r])break;evict();}
            const auto state=m_scheduler->stats();if(bytes>state.limits[r]-state.used[r])m_mount->trimCompletedPayloads();
        }
        void retain(const std::shared_ptr<Work> &work,s32 priority)
        {
            if(!m_residentEntries||!work->resource||work->resource->bytes.size()>m_residentLimit)return;
            try {
                auto old=m_resident.find(work->identity);
                if(old!=m_resident.end()){old->second.priority=std::max(old->second.priority,priority);old->second.access=++m_access;return;}
                String error;auto lease=m_scheduler->reserve(StreamMemoryClass::PendingCpu,512+8*work->identity.first.size(),error);if(!lease)return;
                m_resident.emplace(work->identity,Resident{std::move(lease),work->resource,priority,++m_access});m_residentBytes+=work->resource->bytes.size();trim();
            }catch(...){/* Optional residency cannot discard a delivered range. */}
        }
        u64 release(const std::shared_ptr<Work> &work,u64 subscription,bool cache,s32 priority)
        {
            if(!work->interests.erase(subscription))return 0;--m_logical;
            if(cache)retain(work,priority);
            if(!work->interests.empty()){m_producer->setPriority(work->key,work->priority());return 0;}
            m_producer->cancel(work->key);const auto key=m_producer->contains(work->key)?work->key:0;
            m_byKey.erase(work->key);m_work.erase(work->identity);return key;
        }
        static WorldStreamResult assemble(const std::shared_ptr<Assembly> &input,const StreamCancellation &cancel)
        {
            WorldStreamResult result;if(cancel.cancelled())return result;
            auto value=std::allocate_shared<RuntimeResourceChunk>(SourceBudgetAllocator<RuntimeResourceChunk>{});value->admission=input->admission;
            value->resourceId=input->id;value->offset=input->offset;value->payloadSize=input->payloadSize;value->bytes.resize(static_cast<size_t>(input->length));u64 copied=0;
            for(const auto &chunk:input->chunks) {
                require(!cancel.cancelled(),"Range assembly cancelled");
                const auto start=std::max(input->offset,chunk->offset),end=std::min(input->offset+input->length,chunk->offset+chunk->bytes.size());
                if(end>start){std::memcpy(value->bytes.data()+start-input->offset,chunk->bytes.data()+start-chunk->offset,static_cast<size_t>(end-start));copied+=end-start;}
                value->legacyWholePayloadFallback|=chunk->legacyWholePayloadFallback;
            }
            require(copied==input->length,"Verified chunks do not cover requested range");value->payloadBytesRead=input->ioBytes;
            value->hash=hashBytes(value->bytes.data(),value->bytes.size());result.payload=value;return result;
        }
        void schedule(const std::shared_ptr<Request> &request)
        {
            if(request->complete||request->assembling)return;
            for(const auto &piece:request->pieces) {
                if(!piece.work->complete)return;
                if(!piece.work->resource){report(request->error,piece.work->error.empty()?"Chunk read failed":piece.work->error.c_str());request->complete=true;return;}
            }
            try {
                String error;auto source=m_scheduler->reserve(StreamMemoryClass::SourceCpu,request->length+8192+128*request->pieces.size(),error);
                if(!source){report(request->error,error.c_str());request->complete=true;return;}
                auto input=std::allocate_shared<Assembly>(SourceBudgetAllocator<Assembly>{});input->admission=std::move(source);input->id=request->id;
                input->offset=request->offset;input->length=request->length;input->payloadSize=request->pieces[0].work->info->header.payloadSize;
                for(const auto &piece:request->pieces){input->chunks.push_back(piece.work->resource);if(!piece.work->cacheHit)input->ioBytes+=piece.work->resource->payloadBytesRead;}
                require(m_nextKey!=std::numeric_limits<u64>::max(),"Chunk job identity exhausted");const auto key=m_nextKey++;
                WorldStreamRequest job;job.key=key;job.priority=request->priority;job.workingBytes=request->length;job.pendingBytes=1024+128*input->chunks.size();
                job.execute=[input](const StreamCancellation &cancel){return assemble(input,cancel);};
                Map<u64,std::weak_ptr<Request>> staged;staged.emplace(key,request);
                if(!m_producer->enqueue({job},error)){report(request->error,error.c_str());request->complete=true;return;}
                m_assemblies.merge(staged);request->assemblyKey=key;request->assembling=true;
            }catch(const std::exception &failure){report(request->error,failure.what());request->complete=true;}
        }
        std::shared_ptr<ResourcePayloadMount> m_mount;
        std::shared_ptr<WorldStreamingScheduler> m_scheduler;
        std::shared_ptr<WorldStreamProducer> m_producer;
        std::thread::id m_owner;
        Map<Key,std::shared_ptr<Work>> m_work;
        Map<u64,std::shared_ptr<Work>> m_byKey;
        Map<u64,std::weak_ptr<Request>> m_assemblies;
        Map<String,std::weak_ptr<const RuntimeResourceRangeInfo>> m_info;
        Map<Key,Resident> m_resident;
        u64 m_nextKey=1,m_nextSubscription=1,m_access=0,m_residentBytes=0,m_residentLimit=0;
        size_t m_residentEntries=0;
        u32 m_maximum=0,m_logical=0;
    };

    /** Owner-thread verified byte-range transport. New chunk unions reserve their
     * entire ResultCpu footprint at admission, preventing partial-range deadlock.
     * Native users retain their last published fallback on admission/read failure.
     * Destruction is on owner or after an external owner-quiescence fence. */
    class ResourceChunkStream final : public SourceBudgetObject
    {
        using Cache=ResourceChunkCache;using Work=chunk_stream_detail::Work;using Request=chunk_stream_detail::Request;
        using Key=chunk_stream_detail::Key;template<class K,class V>using Map=chunk_stream_detail::Map<K,V>;
    public:
        static std::unique_ptr<ResourceChunkStream> create(std::shared_ptr<ResourcePayloadMount> mount,String &error)
        {
            error.clear();try {
                if(!mount||!mount->owner()){error="Chunk stream requires its owner-thread mount";return {};}
                auto cache=mount->m_chunks.lock();if(!cache) {
                    cache=std::shared_ptr<Cache>(new Cache);cache->m_mount=mount;cache->m_scheduler=mount->m_scheduler;cache->m_owner=std::this_thread::get_id();
                    cache->m_maximum=cache->m_scheduler->stats().maximumPending;
                    WorldStreamProducerLimits limits=mount->m_limits;limits.maximumPending=cache->m_maximum;
                    cache->m_producer=cache->m_scheduler->createProducer(limits,error);if(!cache->m_producer)return {};
                    mount->m_chunks=cache;
                }
                auto result=std::unique_ptr<ResourceChunkStream>(new ResourceChunkStream);result->m_cache=std::move(cache);return result;
            }catch(const std::exception &failure){Cache::report(error,failure.what());return {};}
        }
        ~ResourceChunkStream(){if(m_cache)retireAfterOwnerQuiesced();}
        ResourceChunkStream(const ResourceChunkStream &)=delete;
        ResourceChunkStream &operator=(const ResourceChunkStream &)=delete;
        bool resourceInfo(const ResourceID &id,RuntimeResourceRangeInfo &info,String &error) const{return m_cache->m_mount->resourceInfo(id,info,error);}
        StreamBudget reserveOwner(StreamMemoryClass category,u64 bytes,String &error) const{return m_cache->m_scheduler->reserve(category,bytes,error);}
        bool request(u64 key,const ResourceID &id,u64 offset,u64 length,s32 priority,s32 residentPriority,String &error)
        {
            error.clear();if(!m_cache->owner()){error="Range request requires its owner thread";return false;}
            try {
                Cache::require(id.isValid()&&id.str().size()<=1024&&length,"Invalid resource range demand");
                auto info=m_cache->info(id,error);if(!info)return false;
                Cache::require(offset<=info->header.payloadSize&&length<=info->header.payloadSize-offset,"Range exceeds pinned payload");
                const auto begin=offset/info->chunkBytes,end=(offset+length-1)/info->chunkBytes,count=end-begin+1;
                const auto old=m_requests.find(key);const auto oldCount=old==m_requests.end()?0:old->second->pieces.size();
                Cache::require(count<=m_cache->m_maximum&&count<=m_cache->m_maximum-(m_cache->m_logical-oldCount),"Range exceeds shared chunk interest capacity");
                Cache::require(m_nextGeneration!=std::numeric_limits<u64>::max(),"Range generation exhausted");m_cache->pump();
                String failure;auto metadata=reserveOwner(StreamMemoryClass::PendingCpu,2048+256*count+8*id.str().size(),failure);if(!metadata){Cache::report(error,failure.c_str());return false;}
                auto next=std::allocate_shared<Request>(SourceBudgetAllocator<Request>{});next->metadata=std::move(metadata);next->id=id;next->key=key;
                next->generation=m_nextGeneration;next->offset=offset;next->length=length;next->priority=priority;next->residentPriority=residentPriority;
                Map<Key,std::shared_ptr<Work>> staged;Map<u64,std::shared_ptr<Work>> keys;
                Map<u64,Map<u64,chunk_stream_detail::Priority>> interests;
                Map<u64,std::shared_ptr<Request>> replacement;replacement.emplace(key,next);
                Array<WorldStreamRequest> jobs{Allocator<WorldStreamRequest>(sourceBudget)};
                u64 needed=0;for(u64 index=begin;index<=end;++index) {
                    const Key identity{id.str(),index};if(!m_cache->m_work.count(identity)&&!m_cache->m_resident.count(identity))
                        needed+=std::min<u64>(info->chunkBytes,info->header.payloadSize-index*info->chunkBytes);
                }
                m_cache->room(needed);
                for(u64 index=begin;index<=end;++index) {
                    const Key identity{id.str(),index};std::shared_ptr<Work> work;const auto existing=m_cache->m_work.find(identity);
                    if(existing!=m_cache->m_work.end())work=existing->second;
                    else {
                        Cache::require(m_cache->m_nextKey!=std::numeric_limits<u64>::max(),"Chunk job identity exhausted");
                        auto budget=reserveOwner(StreamMemoryClass::PendingCpu,1024+8*id.str().size(),error);if(!budget)return false;
                        work=std::allocate_shared<Work>(SourceBudgetAllocator<Work>{});work->metadata=std::move(budget);work->identity=identity;work->info=info;
                        work->key=m_cache->m_nextKey++;work->length=std::min<u64>(info->chunkBytes,info->header.payloadSize-index*info->chunkBytes);
                        const auto resident=m_cache->m_resident.find(identity);
                        if(resident!=m_cache->m_resident.end()){work->complete=true;work->cacheHit=true;work->resource=resident->second.resource;resident->second.access=++m_cache->m_access;}
                        else {
                            m_cache->room(work->length);work->reservation=reserveOwner(StreamMemoryClass::ResultCpu,work->length,error);if(!work->reservation)return false;
                            WorldStreamRequest job;job.key=work->key;job.priority=priority;job.pendingBytes=512+8*id.str().size();
                            const auto readBytes=info->independentlyVerified?work->length:info->header.payloadSize;
                            const auto limit=m_cache->m_scheduler->stats().limits[size_t(StreamMemoryClass::WorkingCpu)];
                            Cache::require(readBytes<=limit&&info->metadataBytes<=limit-readBytes,"Chunk read exceeds working capacity");job.workingBytes=readBytes+info->metadataBytes;
                            auto read=m_cache->m_mount->m_read;auto reservation=work->reservation;auto scheduler=m_cache->m_scheduler;
                            // The working-capacity subtraction above checked
                            // metadata+readBytes; retained length never exceeds it.
                            const auto retainedBytes=info->metadataBytes+work->length;
                            const auto fallbackBytes=info->independentlyVerified?0:info->metadataBytes+info->header.payloadSize;
                            const bool capturedSource=scheduler!=WorldStreamingScheduler::shared();
                            job.execute=[read,id,index,reservation,scheduler,retainedBytes,fallbackBytes,capturedSource](const StreamCancellation &cancel){WorldStreamResult value;if(cancel.cancelled())return value;
                                StreamBudget source,fallback;
                                if(capturedSource){source=scheduler->reserve(StreamMemoryClass::SourceCpu,retainedBytes,value.error);if(!source)return value;
                                    if(fallbackBytes){fallback=scheduler->reserve(StreamMemoryClass::SourceCpu,fallbackBytes,value.error);if(!fallback)return value;}}
                                auto chunk=read->loadChunk(id,index,value.error,true);if(cancel.cancelled())return WorldStreamResult{};
                                if(chunk){auto held=std::allocate_shared<Cache::Held>(SourceBudgetAllocator<Cache::Held>{});held->reservation=reservation;held->source=std::move(source);held->resource=chunk;
                                    value.payload=std::shared_ptr<const RuntimeResourceChunk>(held,chunk.get());}return value;};
                            jobs.push_back(std::move(job));
                        }
                        staged.emplace(identity,work);keys.emplace(work->key,work);
                    }
                    Cache::require(m_cache->m_nextSubscription!=std::numeric_limits<u64>::max(),"Chunk subscription identity exhausted");
                    const auto subscription=m_cache->m_nextSubscription++;interests[work->key].emplace(subscription,chunk_stream_detail::Priority{priority,residentPriority});
                    next->pieces.push_back({work,subscription});
                }
                if(!m_cache->m_producer->enqueue(jobs,error))return false;
                m_cache->m_work.merge(staged);m_cache->m_byKey.merge(keys);
                for(auto &entry:interests){auto &work=*m_cache->m_byKey.at(entry.first);work.interests.merge(entry.second);m_cache->m_producer->setPriority(work.key,work.priority());}
                m_cache->m_logical+=u32(count);++m_nextGeneration;
                if(old!=m_requests.end()){retire(old->second,false);m_requests.erase(old);}m_requests.merge(replacement);return true;
            }catch(const std::exception &failure){Cache::report(error,failure.what());return false;}
        }
        void process(){if(!m_cache->owner())return;m_cache->pump();for(const auto &entry:m_requests)m_cache->schedule(entry.second);}
        bool take(ResourceChunkResult &output)
        {
            output={};if(!m_cache->owner())return false;process();auto found=m_requests.end();
            for(auto i=m_requests.begin();i!=m_requests.end();++i)if(i->second->complete&&
                (found==m_requests.end()||i->second->priority>found->second->priority))found=i;
            if(found==m_requests.end())return false;auto request=found->second;
            output.key=request->key;output.generation=request->generation;output.offset=request->offset;output.resourceId=request->id;
            output.resource=request->resource;Cache::report(output.error,request->error.c_str());retire(request,bool(request->resource));m_requests.erase(found);return true;
        }
        ResourceChunkProgress progress(u64 key)
        {
            ResourceChunkProgress out;if(!m_cache->owner())return out;process();const auto found=m_requests.find(key);if(found==m_requests.end())return out;
            const auto &r=*found->second;out.requestedBytes=r.length;out.complete=r.complete;out.assembling=r.assembling;
            for(const auto &piece:r.pieces){const auto &work=*piece.work;
                if(work.reservation||work.resource||m_cache->m_producer->contains(work.key))out.reservedBytes+=work.length;
                if(work.resource){const auto begin=std::max(r.offset,work.resource->offset),end=std::min(r.offset+r.length,work.resource->offset+work.resource->bytes.size());
                    if(end>begin)out.readyBytes+=end-begin;if(!work.cacheHit)out.ioBytes+=work.resource->payloadBytesRead;}else if(!work.complete)++out.pendingChunks;}
            return out;
        }
        bool cancel(u64 key){if(!m_cache->owner())return false;const auto found=m_requests.find(key);if(found==m_requests.end())return false;retire(found->second,false);m_requests.erase(found);return true;}
        void cancel(){if(!m_cache->owner())return;for(const auto &entry:m_requests)retire(entry.second,false);m_requests.clear();}
        void drain(){if(!m_cache->owner())return;retireAfterOwnerQuiesced();}
        /** Caller must externally stop/fence owner processing before off-owner
         * teardown. Workers touch immutable inputs, never these owner maps. */
        void retireAfterOwnerQuiesced(){for(const auto &entry:m_requests)retire(entry.second,false);m_requests.clear();for(size_t i=0;i<m_retiringCount;++i)m_cache->m_producer->drain(m_retiring[i]);m_retiringCount=0;}
        bool setPriority(u64 key,s32 priority)
        {if(!m_cache->owner())return false;const auto found=m_requests.find(key);if(found==m_requests.end())return false;found->second->priority=priority;
            for(const auto &piece:found->second->pieces){piece.work->interests.at(piece.subscription).request=priority;m_cache->m_producer->setPriority(piece.work->key,piece.work->priority());}
            if(found->second->assembling)m_cache->m_producer->setPriority(found->second->assemblyKey,priority);return true;}
        bool setResidencyPriority(u64 key,s32 priority)
        {if(!m_cache->owner())return false;const auto found=m_requests.find(key);if(found==m_requests.end())return false;found->second->residentPriority=priority;
            for(const auto &piece:found->second->pieces)piece.work->interests.at(piece.subscription).resident=priority;return true;}
        bool setResidencyPriority(const ResourceID &id,u64 offset,u64 length,s32 priority)
        {
            if(!m_cache->owner()||!length||offset>std::numeric_limits<u64>::max()-length)return false;
            const auto begin=offset/65536,end=(offset+length-1)/65536;bool changed=false;
            for(auto &entry:m_cache->m_resident)if(entry.first.first==id.str()&&entry.first.second>=begin&&entry.first.second<=end){entry.second.priority=priority;changed=true;}return changed;
        }
        void setResidencyLimits(size_t entries,u64 bytes){if(!m_cache->owner())return;m_cache->m_residentEntries=std::min<size_t>(entries,m_cache->m_maximum);m_cache->m_residentLimit=bytes;m_cache->trim();}
        u64 residentBytes() const{return m_cache->m_residentBytes;}
    private:
        ResourceChunkStream()=default;
        void retired(u64 key)
        {if(!key)return;size_t keep=0;for(size_t i=0;i<m_retiringCount;++i)if(m_cache->m_producer->contains(m_retiring[i]))m_retiring[keep++]=m_retiring[i];m_retiringCount=keep;
            if(m_retiringCount==m_retiring.size()){m_cache->m_producer->drain(key);return;}m_retiring[m_retiringCount++]=key;}
        void retire(const std::shared_ptr<Request> &request,bool retain)
        {
            if(request->assemblyKey){m_cache->m_producer->cancel(request->assemblyKey);m_cache->m_assemblies.erase(request->assemblyKey);
                if(m_cache->m_producer->contains(request->assemblyKey))retired(request->assemblyKey);}
            for(const auto &piece:request->pieces)retired(m_cache->release(piece.work,piece.subscription,retain,request->residentPriority));
        }
        std::shared_ptr<Cache> m_cache;
        Map<u64,std::shared_ptr<Request>> m_requests;
        std::array<u64,256> m_retiring{};size_t m_retiringCount=0;
        u64 m_nextGeneration=1;
    };
}
