#pragma once
#include <Workphone/System/ResourcePayloadStream.hpp>

namespace workphone::resource
{
    enum class ResourceLeafStatus {Pending,Ready,Error};

    /** One closure node's owner-thread asynchronous metadata/bootstrap loader.
     * Implementations use the captured mount's verified range transport and own
     * their retained Source/native leases. Ready must carry a nonnull streamData
     * descriptor and preserve every pinned header identity/fingerprint/dependency.
     * No GPU/audio creation belongs in this transport interface. */
    class ResourceLeafProvider
    {
    public:
        virtual ~ResourceLeafProvider()=default;
        virtual ResourceLeafStatus poll(std::shared_ptr<const RuntimeResource> &result,String &error)=0;
        virtual u64 maximumResidentBytes() const=0;
        virtual void setPriority(s32) {}
        virtual void cancel()=0;
        virtual void drain()=0;
    };

    class ResourceLeafProviderFactory
    {
    public:
        virtual ~ResourceLeafProviderFactory()=default;
        virtual bool handles(const ResourceID &) const=0;
        /** Stateful recipe selection returns a fresh request scope. nullptr with
         * no error reuses a stateless factory; a nonempty error rejects the request. */
        virtual std::shared_ptr<ResourceLeafProviderFactory> cloneForRequest(const ResourceID &,String &) {return {};}
        virtual std::shared_ptr<ResourceLeafProvider> create(std::shared_ptr<ResourcePayloadMount>,
            const ResourceID &,const RuntimeResourceRangeInfo &,s32 priority,String &error)=0;
        /** Called on validated full resources before discovering their children,
         * allowing a scene compiler's opt-in mesh IDs to select sparse loading. */
        virtual bool observeResourceReady(const std::shared_ptr<const RuntimeResource> &,String &) {return true;}
    };
}
