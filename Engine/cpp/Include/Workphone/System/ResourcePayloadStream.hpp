#pragma once
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <set>
#include <limits>

namespace workphone::resource
{
    struct ResourcePayloadDemand
    {
        u64 key=0;
        ResourceID resourceId;
        u64 maximumPayloadBytes=1024ull<<10;
        s32 priority=0;
    };
    struct ResourcePayloadResult
    {
        u64 key=0, generation=0;
        std::shared_ptr<const RuntimeResource> resource;
        String error;
    };

    /** Owner-thread demand and consumption, shared scheduler worker I/O. Owns a
     * private immutable read-only mount so queued reads cannot retarget a reopened
     * project. Payloads pin ResultCpu reservations until the last consumer drops
     * them. This does not construct GPU/audio/scene objects or load child payloads.
     */
    class ResourcePayloadStream final
    {
    public:
        ResourcePayloadStream(const ResourcePayloadStream &)=delete;
        ResourcePayloadStream &operator=(const ResourcePayloadStream &)=delete;
        static std::unique_ptr<ResourcePayloadStream> create(const RuntimeResourceConfig &config,
            const std::shared_ptr<WorldStreamingScheduler> &scheduler,
            const WorldStreamProducerLimits &limits, String &error)
        {
            error.clear();
            if(!scheduler) { error="Resource stream requires a scheduler"; return {}; }
            auto result=std::unique_ptr<ResourcePayloadStream>(new ResourcePayloadStream);
            result->m_scheduler=scheduler;
            result->m_mount=std::make_shared<ResourceSystem>(nullptr,nullptr);
            if(!result->m_mount->initializeRuntime(config,error)) return {};
            result->m_producer=scheduler->createProducer(limits,error);
            if(!result->m_producer) return {};
            const auto global=scheduler->stats().maximumPending;
            result->m_maximumPending=limits.maximumPending<global?limits.maximumPending:global;
            result->m_maximumResultBytes=limits.maximumReadyBytes;
            return result;
        }

        bool replace(const Array<ResourcePayloadDemand> &demand,String &error)
        {
            if(demand.size()>m_maximumPending)
            { error="Resource stream demand exceeds configured pending capacity"; return false; }
            // Validate logical requests before coalescing. Duplicate delivery keys
            // remain invalid even if they would refer to the same physical read.
            std::set<u64> keys;
            u64 pendingBytes=0;
            const auto budgets=m_scheduler->stats().limits;
            for(const auto &item:demand)
            {
                if(!item.resourceId.isValid() || item.resourceId.str().size()>1024 || !item.maximumPayloadBytes ||
                    item.maximumPayloadBytes>m_maximumResultBytes ||
                    item.maximumPayloadBytes>budgets[size_t(StreamMemoryClass::ResultCpu)] ||
                    item.maximumPayloadBytes>budgets[size_t(StreamMemoryClass::WorkingCpu)])
                { error="Invalid resource payload demand"; return false; }
                pendingBytes+=512+8*item.resourceId.str().size();
            }
            StreamBudget deliveryBudget;
            if(pendingBytes) {
                deliveryBudget=m_scheduler->reserve(StreamMemoryClass::PendingCpu,pendingBytes,error);
                if(!deliveryBudget) return false;
            }
            for(const auto &item:demand)
                if(!keys.insert(item.key).second) { error="Duplicate resource payload delivery key"; return false; }
            Array<WorldStreamRequest> requests;
            std::map<std::pair<String,u64>,size_t> groups;
            std::map<u64,Array<u64>> deliveries;
            std::map<u64,std::shared_ptr<const RuntimeResource>> cachedDeliveries;
            Array<String> hits;
            for(const auto &item:demand)
            {
                const auto identity=std::make_pair(item.resourceId.str(),item.maximumPayloadBytes);
                const auto group=groups.find(identity);
                if(group!=groups.end()) {
                    auto &request=requests[group->second];
                    request.priority=std::max(request.priority,item.priority);
                    deliveries[request.key].push_back(item.key);
                    continue;
                }
                groups.emplace(identity,requests.size());
                WorldStreamRequest request;
                request.key=item.key; request.priority=item.priority;
                request.pendingBytes=256+8*item.resourceId.str().size();
                request.workingBytes=item.maximumPayloadBytes;
                request.maximumResultBytes=item.maximumPayloadBytes;
                const auto cached=m_resident.find(item.resourceId.str());
                if(cached!=m_resident.end()) {
                    // The original result already owns the physical payload lease.
                    // The completion wrapper must not reserve its payload a second
                    // time, which could deadlock a cache filling ResultCpu capacity.
                    auto resource=cached->second.resource;
                    cachedDeliveries.emplace(item.key,resource);
                    request.workingBytes=0; request.maximumResultBytes=0;
                    // Resolve cache-owned completions before a cold read waiting
                    // for their physical leases. Otherwise a high-priority cold
                    // front job could permanently block this producer's hits.
                    request.priority=std::numeric_limits<s32>::max();
                    request.execute=[resource,item](const StreamCancellation &cancel) {
                        WorldStreamResult result;
                        if(cancel.cancelled()) return result;
                        if(resource->payload.size()>item.maximumPayloadBytes)
                            result.error="Resident resource exceeds requested payload limit";
                        else result.payload=resource;
                        return result;
                    };
                    hits.push_back(item.resourceId.str());
                }
                else {
                    auto mount=m_mount;
                    request.execute=[mount,item](const StreamCancellation &cancel) {
                        WorldStreamResult result;
                        if(cancel.cancelled()) return result;
                        auto resource=mount->loadPayload(item.resourceId,result.error,item.maximumPayloadBytes);
                        if(cancel.cancelled()) return WorldStreamResult{};
                        if(resource) { result.bytes=resource->payload.size(); result.payload=std::move(resource); }
                        return result;
                    };
                }
                deliveries[item.key].push_back(item.key);
                requests.push_back(std::move(request));
            }
            // A cold caller may also request INT_MAX priority. The scheduler's
            // equal-priority order follows submission order, so put cache jobs
            // first before submitting the batch to preserve forward progress.
            std::stable_partition(requests.begin(),requests.end(),[&](const auto &request) {
                return cachedDeliveries.count(request.key)!=0;
            });
            if(!m_producer->replace(requests,error)) return false;
            m_deliveries.swap(deliveries); m_deliveryBudget=std::move(deliveryBudget);
            m_cachedDeliveries.swap(cachedDeliveries);
            m_remaining=demand.size(); m_fanout={}; m_fanoutCursor=0;
            for(const auto &id:hits) m_resident.at(id).access=++m_access;
            u64 required=0;
            for(const auto &request:requests) required=std::max(required,request.maximumResultBytes);
            m_headroom=required; ensureReadHeadroom();
            return true;
        }
    private:
        void ensureReadHeadroom()
        {
            // Cache ownership must not prevent a newly admitted read from ever
            // obtaining a result reservation. External consumers remain pinned.
            while(!m_resident.empty()) {
                const auto state=m_scheduler->stats(); const auto index=size_t(StreamMemoryClass::ResultCpu);
                if(m_headroom<=state.limits[index]-state.used[index]) break;
                evictOldest();
            }
        }

    public:
        bool resolveUUID(const String &uuid,ResourceID &id,String &error) const
        { return m_mount->resolveRuntimeUUID(uuid,id,error); }

        bool take(ResourcePayloadResult &output)
        {
            output={};
            if(m_fanoutCursor) {
                const auto &keys=m_deliveries.at(m_fanout.key);
                output=m_fanout; output.key=keys[m_fanoutCursor++];
                if(m_fanoutCursor==keys.size()) { m_fanout={}; m_fanoutCursor=0; }
                consumed(); return true;
            }
            WorldStreamResult result;
            if(!m_producer->take(result)) return false;
            output.key=result.key; output.generation=result.generation; output.error=std::move(result.error);
            if(result.payload) output.resource=std::shared_ptr<const RuntimeResource>(result.payload,
                static_cast<const RuntimeResource*>(result.payload.get()));
            const auto cached=m_cachedDeliveries.find(result.key);
            if(cached!=m_cachedDeliveries.end()) {
                if(output.resource) output.resource=cached->second; // Preserve the original physical lease.
                m_cachedDeliveries.erase(cached);
            }
            const auto &keys=m_deliveries.at(result.key);
            if(output.resource && m_residentEntries && output.resource->payload.size()<=m_residentLimit) {
                const auto id=output.resource->header.resourceId.str();
                auto old=m_resident.find(id);
                if(old!=m_resident.end() && old->second.resource==output.resource) old->second.access=++m_access;
                else {
                    if(old!=m_resident.end()) { m_residentBytes-=old->second.resource->payload.size(); m_resident.erase(old); }
                    m_resident[id]={output.resource,++m_access};
                    m_residentBytes+=output.resource->payload.size(); trimResident();
                }
                const auto pending=m_producer->stats();
                // The current completion may still be counted active while its
                // worker releases callback captures. It needs no future read
                // headroom when this is the last physical delivery group.
                if(m_remaining>keys.size() && (pending.pending || pending.active)) ensureReadHeadroom();
            }
            if(keys.size()>1) { m_fanout=output; m_fanoutCursor=1; }
            consumed();
            return true;
        }
        /** Owner-thread payload cache. Eviction drops cache ownership; consumers
         * keep scheduler result leases until their final reference retires.
         * Header/object storage is not included in this payload-byte quota. */
        void setResidencyLimits(size_t entries,u64 payloadBytes)
        { m_residentEntries=entries; m_residentLimit=payloadBytes; trimResident(); }
        std::shared_ptr<const RuntimeResource> findResident(const ResourceID &id)
        {
            auto found=m_resident.find(id.str());
            if(found==m_resident.end()) return {};
            found->second.access=++m_access; return found->second.resource;
        }
        u64 residentPayloadBytes() const { return m_residentBytes; }
        void cancel() { m_producer->cancel(); clearDeliveries(); }
        void drain() { m_producer->drain(); clearDeliveries(); }
        // Queue/completion statistics count physical jobs. Coalesced logical
        // deliveries share one job and the original result reservation.
        WorldStreamProducerStats stats() const { return m_producer->stats(); }

    private:
        ResourcePayloadStream()=default;
        void clearDeliveries()
        {
            m_fanout={}; m_fanoutCursor=0; m_remaining=0;
            m_deliveries.clear(); m_cachedDeliveries.clear(); m_deliveryBudget.reset();
        }
        void consumed() { if(--m_remaining==0) clearDeliveries(); }
        std::map<u64,Array<u64>> m_deliveries;
        std::map<u64,std::shared_ptr<const RuntimeResource>> m_cachedDeliveries;
        StreamBudget m_deliveryBudget;
        ResourcePayloadResult m_fanout;
        size_t m_fanoutCursor=0,m_remaining=0;
        struct Resident { std::shared_ptr<const RuntimeResource> resource; u64 access; };
        std::map<String,Resident> m_resident;
        size_t m_residentEntries=0;
        u64 m_residentLimit=0,m_residentBytes=0,m_access=0,m_headroom=0;
        void trimResident()
        {
            while(m_resident.size()>m_residentEntries || m_residentBytes>m_residentLimit) evictOldest();
        }
        void evictOldest()
        {
            auto oldest=m_resident.begin();
            for(auto i=m_resident.begin();i!=m_resident.end();++i)
                if(i->second.access<oldest->second.access) oldest=i;
            m_residentBytes-=oldest->second.resource->payload.size(); m_resident.erase(oldest);
        }
        std::shared_ptr<WorldStreamingScheduler> m_scheduler;
        // Producer retires before the private mount; callbacks also pin the mount.
        std::shared_ptr<ResourceSystem> m_mount;
        std::shared_ptr<WorldStreamProducer> m_producer;
        u32 m_maximumPending=0;
        u64 m_maximumResultBytes=0;
    };
}
