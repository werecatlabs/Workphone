#pragma once
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <filesystem>
#include <limits>
#include <map>
#include <mutex>
#include <thread>

namespace workphone::resource
{
    struct ResourcePayloadDemand {u64 key=0;ResourceID resourceId;u64 maximumPayloadBytes=1024ull<<10;s32 priority=0;};
    struct ResourcePayloadResult {u64 key=0,generation=0;std::shared_ptr<const RuntimeResource> resource;String error;};
    namespace payload_stream_detail
    {
        template<class Key,class Value>using Map=std::map<Key,Value,std::less<Key>,SourceBudgetAllocator<std::pair<const Key,Value>>>;
        using Identity=std::pair<String,u64>;
        struct Work final : SourceBudgetObject
        {
            StreamBudget metadata;
            Identity identity;
            u64 key=0,resultBytes=0;
            bool complete=false,cached=false;
            std::shared_ptr<const RuntimeResource> resource,cachedSource;
            String error;
            Map<u64,s32> interests;
            s32 priority() const
            {
                if(cached)return std::numeric_limits<s32>::max();
                s32 result=std::numeric_limits<s32>::min();
                for(const auto &entry:interests)result=std::max(result,entry.second);
                return result;
            }
        };
        // A custom mount ledger participates alongside the shared allocator's
        // physical accounting. The alias retains both admissions with metadata.
        struct SourceAdmissions
        {
            StreamBudget captured,physical;
        };
    }
    class ResourcePayloadStream;
    class ResourceChunkStream;
    class ResourceChunkCache;
    /** One immutable manifest generation and persistent physical queue. Streams
     * sharing this object share pending/executing/completed reads. All requesters
     * belong to its creating thread. Cache quotas belong to this shared mount. */
    class ResourcePayloadMount final : public SourceBudgetObject
    {
    public:
        static std::shared_ptr<ResourcePayloadMount> create(const RuntimeResourceConfig &config,
            std::shared_ptr<WorldStreamingScheduler> scheduler,const WorldStreamProducerLimits &limits,String &error)
        {
            error.clear();try {
                if(!scheduler){error="Resource mount requires a scheduler";return {};}
                std::error_code ec;const auto bytes=std::filesystem::file_size(std::filesystem::u8path(config.manifestPath.c_str()),ec);
                if(ec||bytes>(24ull<<20)){error="Cannot bound runtime manifest storage";return {};}
                const u64 metadata=65536+12*bytes;
                auto working=scheduler->reserve(StreamMemoryClass::WorkingCpu,metadata,error);if(!working)return {};
                auto retained=scheduler->reserve(StreamMemoryClass::SourceCpu,metadata,error);if(!retained)return {};
                auto result=std::shared_ptr<ResourcePayloadMount>(new ResourcePayloadMount);
                result->m_metadata=std::move(retained);result->m_scheduler=std::move(scheduler);result->m_owner=std::this_thread::get_id();
                result->m_read=std::make_shared<ResourceSystem>(nullptr,nullptr);
                if(!result->m_read->initializeRuntime(config,error,result->m_metadata,working))return {};
                result->m_producer=result->m_scheduler->createProducer(limits,error);if(!result->m_producer)return {};
                result->m_limits=limits;result->m_maximumLogical=result->m_scheduler->stats().maximumPending;
                result->m_runtimeLimits=config;
                result->m_maximumWork=std::min(limits.maximumPending,result->m_maximumLogical);return result;
            }catch(const std::exception &failure){error=failure.what();return {};}
        }
        ResourcePayloadMount(const ResourcePayloadMount &)=delete;
        ResourcePayloadMount &operator=(const ResourcePayloadMount &)=delete;
        const RuntimeResourceConfig &runtimeLimits() const{return m_runtimeLimits;}
        std::function<StreamBudget(StreamMemoryClass,u64,String &)> budgetAllocator() const{return m_producer->budgetAllocator();}
        static std::shared_ptr<ResourcePayloadMount> createComposition(const Array<RuntimeResourceMountConfig> &packages,
            RuntimeMountCollisionPolicy policy,std::shared_ptr<WorldStreamingScheduler> scheduler,
            const WorldStreamProducerLimits &limits,String &error)
        {
            error.clear();try {
                if(!scheduler){error="Composed resource mount requires a scheduler";return {};}
                u64 source=0,scratch=0;if(!ResourceSystem::compositionMemoryRequirements(packages,source,scratch,error))return {};
                auto retained=scheduler->reserve(StreamMemoryClass::SourceCpu,source,error);if(!retained)return {};
                auto working=scheduler->reserve(StreamMemoryClass::WorkingCpu,scratch,error);if(!working)return {};
                auto result=std::shared_ptr<ResourcePayloadMount>(new ResourcePayloadMount);result->m_metadata=std::move(retained);
                result->m_scheduler=std::move(scheduler);result->m_owner=std::this_thread::get_id();result->m_read=std::make_shared<ResourceSystem>(nullptr,nullptr);
                if(!result->m_read->initializeRuntimeComposition(packages,policy,error,result->m_metadata,working))return {};
                result->m_producer=result->m_scheduler->createProducer(limits,error);if(!result->m_producer)return {};
                result->m_limits=limits;result->m_maximumLogical=result->m_scheduler->stats().maximumPending;
                result->m_maximumWork=std::min(limits.maximumPending,result->m_maximumLogical);result->m_runtimeLimits=packages.front().config;
                for(const auto &package:packages){const auto &config=package.config;
                    result->m_runtimeLimits.maxPayloadBytes=std::min(result->m_runtimeLimits.maxPayloadBytes,config.maxPayloadBytes);
                    result->m_runtimeLimits.maxClosurePayloadBytes=std::min(result->m_runtimeLimits.maxClosurePayloadBytes,config.maxClosurePayloadBytes);
                    result->m_runtimeLimits.maxClosureResources=std::min(result->m_runtimeLimits.maxClosureResources,config.maxClosureResources);
                    result->m_runtimeLimits.maxDependencyDepth=std::min(result->m_runtimeLimits.maxDependencyDepth,config.maxDependencyDepth);}
                return result;
            }catch(const std::exception &failure){try{error=failure.what();}catch(...){error.clear();}return {};}
        }
        bool provenance(const ResourceID &id,RuntimeResourceProvenance &value,String &error) const
        {return m_read->runtimeProvenance(id,value,error,m_scheduler);}
        /** Immutable generation read for existing CPU stream workers. Does not
         * access/pump owner queues; callers must provide an explicit payload cap. */
        std::shared_ptr<const RuntimeResource> loadPinnedPayload(const ResourceID &id,String &error,u64 maximumPayloadBytes) const
        {
            try {
                error.clear();if(!maximumPayloadBytes){error="Pinned payload read requires an explicit nonzero byte cap";return {};}
                u64 payload=0,metadata=0;if(!m_read->payloadMemoryRequirements(id,payload,metadata,error))return {};
                if(payload>maximumPayloadBytes||metadata>std::numeric_limits<u64>::max()-payload){error="Pinned payload exceeds its explicit read allowance";return {};}
                auto retained=m_scheduler->reserve(StreamMemoryClass::SourceCpu,payload+metadata,error);if(!retained)return {};
                auto scratch=m_scheduler->reserve(StreamMemoryClass::WorkingCpu,payload+metadata,error);if(!scratch)return {};
                struct Owned {StreamBudget admission;std::shared_ptr<const RuntimeResource> resource;};
                auto owner=std::allocate_shared<Owned>(SourceBudgetAllocator<Owned>{});owner->admission=std::move(retained);
                owner->resource=m_read->loadPayload(id,error,maximumPayloadBytes);if(!owner->resource)return {};
                return std::shared_ptr<const RuntimeResource>(owner,owner->resource.get());
            }catch(const std::exception &failure){try{error=failure.what();}catch(...){error.clear();}return {};}
        }
        bool resourceInfo(const ResourceID &id,RuntimeResourceRangeInfo &info,String &error) const
        {
            try {
                if(m_scheduler==WorldStreamingScheduler::shared())return m_read->resourceInfo(id,info,error);
                u64 payload=0,metadata=0;if(!m_read->payloadMemoryRequirements(id,payload,metadata,error))return false;
                auto source=m_scheduler->reserve(StreamMemoryClass::SourceCpu,metadata,error);if(!source)return false;
                auto admission=std::allocate_shared<payload_stream_detail::SourceAdmissions>(SourceBudgetAllocator<payload_stream_detail::SourceAdmissions>{});
                admission->captured=std::move(source);RuntimeResourceRangeInfo result;
                if(!m_read->resourceInfo(id,result,error))return false;
                admission->physical=std::move(result.admission);
                result.admission=StreamBudget(admission,admission->captured.get());std::swap(info,result);return true;
            }catch(const std::exception &failure){try{error=failure.what();}catch(...){error.clear();}return false;}
        }
        StreamBudget reserveOwner(StreamMemoryClass category,u64 bytes,String &error) const
        {return m_scheduler->reserve(category,bytes,error);}
        bool trimCompletedPayloads()
        {if(!owner())return false;m_resident.clear();m_residentBytes=0;return true;}
        /** Captures the same immutable generation on the calling owner thread;
         * never reparses a mutable manifest path. Live views on one owner share. */
        std::shared_ptr<ResourcePayloadMount> createReadOnlyOwnerView(String &error)
        {
            error.clear();try {
                std::lock_guard<std::mutex> lock(m_viewsMutex);
                for(auto i=m_views.begin();i!=m_views.end();)if(i->second.expired())i=m_views.erase(i);else ++i;
                const auto owner=std::this_thread::get_id();const auto found=m_views.find(owner);
                if(found!=m_views.end())if(auto view=found->second.lock())return view;
                auto view=std::shared_ptr<ResourcePayloadMount>(new ResourcePayloadMount);
                view->m_metadata=m_metadata;view->m_scheduler=m_scheduler;view->m_read=m_read;view->m_owner=owner;
                view->m_limits=m_limits;view->m_runtimeLimits=m_runtimeLimits;view->m_maximumLogical=m_maximumLogical;view->m_maximumWork=m_maximumWork;
                view->m_producer=m_scheduler->createProducer(m_limits,error);if(!view->m_producer)return {};
                m_views[owner]=view;return view;
            }catch(const std::exception &failure){error=failure.what();return {};}
        }
    private:
        friend class ResourcePayloadStream;
        friend class ResourceChunkStream;
        friend class ResourceChunkCache;
        using Work=payload_stream_detail::Work;using Identity=payload_stream_detail::Identity;
        template<class K,class V>using Map=payload_stream_detail::Map<K,V>;
        struct Resident {StreamBudget metadata;std::shared_ptr<const RuntimeResource> resource;u64 access=0;};
        struct Retained {StreamBudget metadata;std::shared_ptr<const RuntimeResource> resource;};
        ResourcePayloadMount()=default;
        bool owner() const{return std::this_thread::get_id()==m_owner;}
        void pump()
        {
            WorldStreamResult result;
            while(m_producer->take(result)) {
                const auto found=m_byKey.find(result.key);if(found==m_byKey.end())continue;
                auto &work=*found->second;work.complete=true;work.error=std::move(result.error);
                if(result.payload)work.resource=work.cached?work.cachedSource:
                    std::shared_ptr<const RuntimeResource>(result.payload,static_cast<const RuntimeResource*>(result.payload.get()));
                work.cachedSource.reset();
            }
        }
        u64 release(const std::shared_ptr<Work> &work,u64 subscription)
        {
            if(!work->interests.erase(subscription))return 0;--m_logical;
            if(!work->interests.empty()){m_producer->setPriority(work->key,work->priority());return 0;}
            m_producer->cancel(work->key);
            const auto retiring=m_producer->contains(work->key)?work->key:0;
            m_byKey.erase(work->key);m_work.erase(work->identity);return retiring;
        }
        void evictOldest()
        {
            auto oldest=m_resident.begin();for(auto i=m_resident.begin();i!=m_resident.end();++i)
                if(i->second.access<oldest->second.access)oldest=i;
            m_residentBytes-=oldest->second.resource->payload.size();m_resident.erase(oldest);
        }
        void trim(){while(m_resident.size()>m_residentEntries||m_residentBytes>m_residentLimit)evictOldest();}
        void headroom()
        {
            u64 required=0;for(const auto &entry:m_work)
                if(!entry.second->complete&&!entry.second->cached)required=std::max(required,entry.second->resultBytes);
            while(!m_resident.empty()) {
                const auto state=m_scheduler->stats();const auto i=size_t(StreamMemoryClass::ResultCpu);
                if(required<=state.limits[i]-state.used[i])break;evictOldest();
            }
        }
        void retain(const std::shared_ptr<Work> &work)
        {
            if(!work->resource||!m_residentEntries||work->resource->payload.size()>m_residentLimit)return;
            try {
                const auto &id=work->identity.first;auto old=m_resident.find(id);
                if(old!=m_resident.end()&&old->second.resource==work->resource){old->second.access=++m_access;return;}
                String error;auto budget=m_scheduler->reserve(StreamMemoryClass::PendingCpu,512+8*id.size(),error);if(!budget)return;
                if(old!=m_resident.end()){m_residentBytes-=old->second.resource->payload.size();m_resident.erase(old);}
                m_resident.emplace(id,Resident{std::move(budget),work->resource,++m_access});
                m_residentBytes+=work->resource->payload.size();trim();headroom();
            }catch(const std::exception &){/* Cache admission cannot lose a delivered result. */}
        }
        StreamBudget m_metadata;
        std::shared_ptr<WorldStreamingScheduler> m_scheduler;
        std::shared_ptr<ResourceSystem> m_read;
        std::shared_ptr<WorldStreamProducer> m_producer;
        std::weak_ptr<ResourceChunkCache> m_chunks;
        std::mutex m_viewsMutex;
        Map<std::thread::id,std::weak_ptr<ResourcePayloadMount>> m_views;
        WorldStreamProducerLimits m_limits;
        RuntimeResourceConfig m_runtimeLimits;
        std::thread::id m_owner;
        Map<Identity,std::shared_ptr<Work>> m_work;
        Map<u64,std::shared_ptr<Work>> m_byKey;
        Map<String,Resident> m_resident;
        size_t m_residentEntries=0;
        u64 m_nextWork=1,m_nextSubscription=1,m_residentLimit=0,m_residentBytes=0,m_access=0;
        u32 m_maximumWork=0,m_maximumLogical=0,m_logical=0;
    };

    /** Requester-local logical demand. Replacement retains matching work, and
     * cancellation only releases this requester's interests. Returned aliases
     * retain the original ResultCpu payload and SourceCpu header reservations. */
    class ResourcePayloadStream final : public SourceBudgetObject
    {
    public:
        ResourcePayloadStream(const ResourcePayloadStream &)=delete;
        ResourcePayloadStream &operator=(const ResourcePayloadStream &)=delete;
        ~ResourcePayloadStream(){if(m_mount)drain();}
        static std::unique_ptr<ResourcePayloadStream> create(const RuntimeResourceConfig &config,
            const std::shared_ptr<WorldStreamingScheduler> &scheduler,const WorldStreamProducerLimits &limits,String &error)
        {return create(ResourcePayloadMount::create(config,scheduler,limits,error),error);}
        static std::unique_ptr<ResourcePayloadStream> create(std::shared_ptr<ResourcePayloadMount> mount,String &error)
        {
            if(!mount||!mount->owner()){if(error.empty())error="Resource stream requires its owner-thread mount";return {};}
            auto result=std::unique_ptr<ResourcePayloadStream>(new ResourcePayloadStream);result->m_mount=std::move(mount);error.clear();return result;
        }
        std::shared_ptr<ResourcePayloadMount> mount() const{return m_mount;}
        std::shared_ptr<WorldStreamingScheduler> scheduler() const{return m_mount->m_scheduler;}
        u32 maximumPending() const{return m_mount->m_maximumWork;}
        const RuntimeResourceConfig &runtimeLimits() const{return m_mount->m_runtimeLimits;}
        bool memoryRequirements(const ResourceID &id,u64 &payload,u64 &metadata,String &error) const
        {return m_mount->m_read->payloadMemoryRequirements(id,payload,metadata,error);}
        StreamBudget reserve(StreamMemoryClass category,u64 bytes,String &error) const
        {return m_mount->m_scheduler->reserve(category,bytes,error);}
        bool replace(const Array<ResourcePayloadDemand> &demand,String &error)
        {
            error.clear();auto &mount=*m_mount;
            if(!mount.owner()){error="Resource demand requires the mount owner thread";return false;}
            try {
                if(demand.size()>mount.m_maximumWork||m_generation==std::numeric_limits<u64>::max()||
                    demand.size()>mount.m_maximumLogical-(mount.m_logical-m_delivery.size()))
                {error="Resource demand exceeds mount logical capacity";return false;}
                const auto budgets=mount.m_scheduler->stats().limits;
                for(const auto &item:demand)if(!item.resourceId.isValid()||item.resourceId.str().size()>1024||!item.maximumPayloadBytes||
                    item.maximumPayloadBytes>mount.m_limits.maximumReadyBytes||item.maximumPayloadBytes>budgets[size_t(StreamMemoryClass::ResultCpu)])
                {error="Invalid resource payload demand";return false;}
                mount.pump();
                Map<Identity,u32> wanted;
                Map<u64,u32> previous;
                for(const auto &item:demand)++wanted[{item.resourceId.str(),item.maximumPayloadBytes}];
                for(const auto &entry:m_delivery)++previous[entry.second.work->key];
                Array<u64> retiring{Allocator<u64>(sourceBudget)};
                for(const auto &entry:previous) {
                    const auto &work=mount.m_byKey.at(entry.first);
                    if(entry.second==work->interests.size()&&!wanted.count(work->identity))retiring.push_back(work->key);
                }
                Map<u64,Delivery> candidate;
                Map<Identity,std::shared_ptr<Work>> newWork;
                Map<u64,std::shared_ptr<Work>> newKeys;
                Map<u64,Map<u64,s32>> interests;
                Array<WorldStreamRequest> requests{Allocator<WorldStreamRequest>(sourceBudget)};
                for(const auto &item:demand) {
                    if(candidate.count(item.key)){error="Duplicate resource payload delivery key";return false;}
                    auto lease=mount.m_scheduler->reserve(StreamMemoryClass::PendingCpu,512+8*item.resourceId.str().size(),error);if(!lease)return false;
                    Identity identity{item.resourceId.str(),item.maximumPayloadBytes};std::shared_ptr<Work> work;
                    const auto found=mount.m_work.find(identity),prepared=newWork.find(identity);
                    if(found!=mount.m_work.end())work=found->second;
                    else if(prepared!=newWork.end())work=prepared->second;
                    else {
                        if(mount.m_nextWork==std::numeric_limits<u64>::max()||mount.m_work.size()-retiring.size()+newWork.size()>=mount.m_maximumWork)
                        {error="Resource physical work capacity exhausted";return false;}
                        auto metadata=mount.m_scheduler->reserve(StreamMemoryClass::PendingCpu,1024+8*identity.first.size(),error);if(!metadata)return false;
                        work=std::allocate_shared<Work>(SourceBudgetAllocator<Work>{});work->metadata=std::move(metadata);work->identity=identity;work->key=mount.m_nextWork++;
                        WorldStreamRequest request;request.key=work->key;request.priority=item.priority;request.pendingBytes=512+8*identity.first.size();
                        const auto cached=mount.m_resident.find(identity.first);
                        if(cached!=mount.m_resident.end()) {
                            work->cached=true;work->cachedSource=cached->second.resource;request.priority=std::numeric_limits<s32>::max();
                            auto resource=work->cachedSource;request.execute=[resource,item](const StreamCancellation &cancel){WorldStreamResult result;
                                if(!cancel.cancelled()){if(resource->payload.size()>item.maximumPayloadBytes)result.error="Resident resource exceeds requested payload limit";else result.payload=resource;}return result;};
                        }else {
                            u64 payload=0,metadataBytes=0;String planningError;
                            mount.m_read->payloadMemoryRequirements(item.resourceId,payload,metadataBytes,planningError);
                            work->resultBytes=std::min(payload,item.maximumPayloadBytes);request.maximumResultBytes=work->resultBytes;
                            const auto available=budgets[size_t(StreamMemoryClass::WorkingCpu)];
                            if(work->resultBytes>available||metadataBytes>available-work->resultBytes){error="Resource payload/header working budget is insufficient";return false;}
                            request.workingBytes=work->resultBytes+metadataBytes;
                            auto read=mount.m_read;auto scheduler=mount.m_scheduler;
                            request.execute=[read,scheduler,item,metadataBytes](const StreamCancellation &cancel){WorldStreamResult result;if(cancel.cancelled())return result;
                                auto metadata=scheduler->reserve(StreamMemoryClass::SourceCpu,metadataBytes,result.error);if(!metadata)return result;
                                auto resource=read->loadPayload(item.resourceId,result.error,item.maximumPayloadBytes);if(cancel.cancelled())return WorldStreamResult{};
                                if(resource){auto owner=std::allocate_shared<ResourcePayloadMount::Retained>(SourceBudgetAllocator<ResourcePayloadMount::Retained>{});
                                    owner->metadata=std::move(metadata);owner->resource=resource;result.bytes=resource->payload.size();result.payload=std::shared_ptr<const RuntimeResource>(owner,resource.get());}return result;};
                        }
                        newWork.emplace(identity,work);newKeys.emplace(work->key,work);requests.push_back(std::move(request));
                    }
                    if(mount.m_nextSubscription==std::numeric_limits<u64>::max()){error="Resource subscription identity exhausted";return false;}
                    const auto subscription=mount.m_nextSubscription++;
                    interests[work->key].emplace(subscription,item.priority);
                    candidate.emplace(item.key,Delivery{std::move(lease),work,subscription,u64(candidate.size()),item.priority});
                }
                for(auto &request:requests)for(const auto &entry:interests.at(request.key))request.priority=std::max(request.priority,entry.second);
                std::stable_partition(requests.begin(),requests.end(),[&](const auto &request){return newKeys.at(request.key)->cached;});
                if(!mount.m_producer->reconcile(requests,retiring,error))return false;
                mount.m_work.merge(newWork);mount.m_byKey.merge(newKeys);
                for(auto &entry:interests){auto &work=*mount.m_byKey.at(entry.first);work.interests.merge(entry.second);mount.m_producer->setPriority(work.key,work.priority());}
                mount.m_logical+=u32(candidate.size());m_delivery.swap(candidate);++m_generation;
                for(const auto &entry:candidate)release(entry.second);
                mount.headroom();return true;
            }catch(const std::exception &failure){error=failure.what();return false;}
        }
        bool cancel(u64 key)
        {
            if(!m_mount->owner())return false;const auto found=m_delivery.find(key);if(found==m_delivery.end())return false;
            release(found->second);m_delivery.erase(found);return true;
        }
        bool setPriority(u64 key,s32 priority)
        {
            if(!m_mount->owner())return false;const auto found=m_delivery.find(key);if(found==m_delivery.end())return false;
            found->second.priority=priority;auto &work=*found->second.work;work.interests.at(found->second.subscription)=priority;
            m_mount->m_producer->setPriority(work.key,work.priority());return true;
        }
        bool resolveUUID(const String &uuid,ResourceID &id,String &error) const{return m_mount->m_read->resolveRuntimeUUID(uuid,id,error);}
        bool take(ResourcePayloadResult &output)
        {
            output={};if(!m_mount->owner())return false;m_mount->pump();auto best=m_delivery.end();
            // Both physical jobs may finish before the owner polls. Preserve
            // cache-first delivery even when a cold caller also uses INT_MAX.
            for(auto i=m_delivery.begin();i!=m_delivery.end();++i)if(i->second.work->complete)
                if(best==m_delivery.end()||
                    (i->second.work->cached&&!best->second.work->cached)||
                    (i->second.work->cached==best->second.work->cached&&
                        (i->second.work->priority()>best->second.work->priority()||
                            (i->second.work->priority()==best->second.work->priority()&&i->second.order<best->second.order))))best=i;
            if(best==m_delivery.end())return false;
            output.key=best->first;output.generation=m_generation;output.resource=best->second.work->resource;output.error=best->second.work->error;
            m_mount->retain(best->second.work);release(best->second);m_delivery.erase(best);return true;
        }
        /** Mount-wide cache. Header/source leases are additional to payload quota;
         * consumer references keep reservations after cache eviction. */
        void setResidencyLimits(size_t entries,u64 payloadBytes)
        {m_mount->m_residentEntries=std::min<size_t>(entries,m_mount->m_maximumWork);m_mount->m_residentLimit=payloadBytes;m_mount->trim();}
        std::shared_ptr<const RuntimeResource> findResident(const ResourceID &id)
        {auto found=m_mount->m_resident.find(id.str());if(found==m_mount->m_resident.end())return {};found->second.access=++m_mount->m_access;return found->second.resource;}
        u64 residentPayloadBytes() const{return m_mount->m_residentBytes;}
        void cancel()
        {if(!m_mount||!m_mount->owner())return;for(const auto &entry:m_delivery)release(entry.second);m_delivery.clear();if(m_generation!=std::numeric_limits<u64>::max())++m_generation;}
        void drain(){if(!m_mount->owner())return;cancel();for(size_t i=0;i<m_retiringCount;++i)m_mount->m_producer->drain(m_retiring[i]);m_retiringCount=0;}
        // Physical counters are shared by the mount; generation is requester-local.
        WorldStreamProducerStats stats() const{auto value=m_mount->m_producer->stats();value.generation=m_generation;return value;}
    private:
        using Work=payload_stream_detail::Work;using Identity=payload_stream_detail::Identity;
        template<class K,class V>using Map=payload_stream_detail::Map<K,V>;
        struct Delivery {StreamBudget metadata;std::shared_ptr<Work> work;u64 subscription=0,order=0;s32 priority=0;};
        void release(const Delivery &delivery)
        {
            const auto key=m_mount->release(delivery.work,delivery.subscription);if(!key)return;
            size_t keep=0;for(size_t i=0;i<m_retiringCount;++i)
                if(m_mount->m_producer->contains(m_retiring[i]))m_retiring[keep++]=m_retiring[i];
            m_retiringCount=keep;
            // At most sixteen callbacks execute globally. Keep a defensive
            // scoped wait even if future scheduler limits exceed this fixed list.
            if(m_retiringCount==m_retiring.size()){m_mount->m_producer->drain(key);return;}
            m_retiring[m_retiringCount++]=key;
        }
        ResourcePayloadStream()=default;
        std::shared_ptr<ResourcePayloadMount> m_mount;
        Map<u64,Delivery> m_delivery;
        std::array<u64,256> m_retiring{};
        size_t m_retiringCount=0;
        u64 m_generation=0;
    };
}
