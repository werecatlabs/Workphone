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
            return m_producer->replace(requests,error);
        }

        bool resolveUUID(const String &uuid,ResourceID &id,String &error) const
        { return m_mount->resolveRuntimeUUID(uuid,id,error); }

        bool take(ResourcePayloadResult &output)
        {
            output={}; WorldStreamResult result;
            if(!m_producer->take(result)) return false;
            output.key=result.key; output.generation=result.generation; output.error=std::move(result.error);
            if(result.payload) output.resource=std::shared_ptr<const RuntimeResource>(result.payload,
                static_cast<const RuntimeResource*>(result.payload.get()));
            return true;
        }
        void cancel() { m_producer->cancel(); }
        void drain() { m_producer->drain(); }
        WorldStreamProducerStats stats() const { return m_producer->stats(); }

    private:
        ResourcePayloadStream()=default;
        // Producer retires before the private mount; callbacks also pin the mount.
        std::shared_ptr<ResourceSystem> m_mount;
        std::shared_ptr<WorldStreamProducer> m_producer;
        u32 m_maximumPending=0;
    };
}
