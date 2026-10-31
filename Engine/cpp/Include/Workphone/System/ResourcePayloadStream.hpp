#pragma once
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>

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
            return result;
        }

        bool replace(const Array<ResourcePayloadDemand> &demand,String &error)
        {
            if(demand.size()>m_maximumPending)
            { error="Resource stream demand exceeds configured pending capacity"; return false; }
            Array<WorldStreamRequest> requests;
            for(const auto &item:demand)
            {
                if(!item.resourceId.isValid() || item.resourceId.str().size()>1024 || !item.maximumPayloadBytes)
                { error="Invalid resource payload demand"; return false; }
                WorldStreamRequest request;
                request.key=item.key; request.priority=item.priority;
                request.pendingBytes=512+8*item.resourceId.str().size();
                request.workingBytes=item.maximumPayloadBytes;
                request.maximumResultBytes=item.maximumPayloadBytes;
                auto mount=m_mount;
                request.execute=[mount,item](const StreamCancellation &cancel) {
                    WorldStreamResult result;
                    if(cancel.cancelled()) return result;
                    auto resource=mount->loadPayload(item.resourceId,result.error,item.maximumPayloadBytes);
                    if(cancel.cancelled()) return WorldStreamResult{};
                    if(resource) { result.bytes=resource->payload.size(); result.payload=std::move(resource); }
                    return result;
                };
                requests.push_back(std::move(request));
            }
            if(!m_producer->replace(requests,error)) return false;
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
            output={}; WorldStreamResult result;
            if(!m_producer->take(result)) return false;
            output.key=result.key; output.generation=result.generation; output.error=std::move(result.error);
            if(result.payload) output.resource=std::shared_ptr<const RuntimeResource>(result.payload,
                static_cast<const RuntimeResource*>(result.payload.get()));
            if(output.resource && m_residentEntries && output.resource->payload.size()<=m_residentLimit) {
                const auto id=output.resource->header.resourceId.str();
                auto old=m_resident.find(id);
                if(old!=m_resident.end()) { m_residentBytes-=old->second.resource->payload.size(); m_resident.erase(old); }
                m_resident[id]={output.resource,++m_access};
                m_residentBytes+=output.resource->payload.size(); trimResident();
                const auto pending=m_producer->stats();
                if(pending.pending || pending.active) ensureReadHeadroom();
            }
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
        void cancel() { m_producer->cancel(); }
        void drain() { m_producer->drain(); }
        WorldStreamProducerStats stats() const { return m_producer->stats(); }

    private:
        ResourcePayloadStream()=default;
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
    };
}
