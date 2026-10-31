#pragma once
#include <Workphone/System/ResourcePayloadStream.hpp>
#include <Workphone/System/ResourceLeafProvider.hpp>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace workphone::resource
{
    struct ResourceClosureLimits
    {
        u64 maximumResourcePayloadBytes=8ull<<20;
        u64 maximumClosurePayloadBytes=64ull<<20;
        u32 maximumResources=256,maximumDepth=64;
    };

    /** Asynchronous immutable compatible-set transport for existing typed
     * installers. Discovers validated shallow dependencies without source access,
     * shares physical reads through a captured mount, and builds a separate
     * complete RuntimeResource DAG on a scheduler worker. Every copied node owns
     * its SourceCpu payload/header lease, including escaped dependency handles.
     * Request/take/cancel/priority/publication belong to the creating owner thread.
     * Consumers must apply their project/request generation fence before install.
     * This transports whole resources; chunk/LOD/mip/audio seeking is separate. */
    class ResourceClosureStream final : public SourceBudgetObject
    {
    public:
        static std::unique_ptr<ResourceClosureStream> create(std::shared_ptr<ResourcePayloadMount> mount,String &error,
                                                            std::shared_ptr<ResourceLeafProviderFactory> factory={})
        {
            try {
                auto payload=ResourcePayloadStream::create(std::move(mount),error);if(!payload)return {};
                auto result=std::unique_ptr<ResourceClosureStream>(new ResourceClosureStream);
                result->m_scheduler=payload->scheduler();result->m_payload=std::move(payload);result->m_owner=std::this_thread::get_id();result->m_factory=std::move(factory);
                WorldStreamProducerLimits limits;limits.maximumPending=1;limits.maximumActive=1;limits.maximumReady=1;limits.maximumReadyBytes=0;
                result->m_decode=result->m_scheduler->createProducer(limits,error);if(!result->m_decode)return {};return result;
            }catch(const std::exception &failure){report(error,failure.what());return {};}
        }
        ~ResourceClosureStream(){if(m_payload&&m_decode)drain();}
        ResourceClosureStream(const ResourceClosureStream &)=delete;
        ResourceClosureStream &operator=(const ResourceClosureStream &)=delete;
        /** Rejected validation/admission preserves the prior request. Once
         * accepted, later I/O/graph/CPU-budget errors are delivered by take(). */
        bool request(u64 key,const ResourceID &root,const ResourceClosureLimits &limits,s32 priority,String &error)
        {
            error.clear();if(!owner()){error="Closure request requires its owner thread";return false;}
            try {
                require(root.isValid()&&root.str().size()<=1024,"Invalid closure root identity");
                require(limits.maximumResourcePayloadBytes&&limits.maximumClosurePayloadBytes&&limits.maximumResources&&
                    limits.maximumResources<=m_payload->maximumPending()&&limits.maximumDepth&&limits.maximumDepth<=256,"Invalid closure limits");
                require(m_generation!=std::numeric_limits<u64>::max(),"Closure generation exhausted");
                auto factory=m_factory;if(factory){auto scoped=factory->cloneForRequest(root,error);if(!scoped&&!error.empty())return false;if(scoped)factory=std::move(scoped);}
                ResourceID stagedRoot=root;auto plan=prepare(root,limits,priority,factory);auto demand=demands(plan.nodes,limits,priority);
                if(!m_payload->replace(demand,error))return false;
                m_decode->cancel();m_nodes.swap(plan.nodes);m_factory=std::move(factory);m_root=std::move(stagedRoot);m_limits=limits;m_priority=priority;m_key=key;++m_generation;
                retireProviders(plan.nodes,&m_nodes);
                m_hasRequest=true;m_ready=false;m_taken=false;m_decoding=false;m_result.reset();m_error.clear();
                if(plan.complete)startDecode(plan.workingBytes);return true;
            }catch(const std::exception &failure){report(error,failure.what());return false;}
        }
        bool requestUUID(u64 key,const String &uuid,const ResourceClosureLimits &limits,s32 priority,String &error)
        {ResourceID id;if(!m_payload->resolveUUID(uuid,id,error))return false;return request(key,id,limits,priority,error);}
        bool setPriority(s32 priority)
        {
            if(!owner()||!m_hasRequest||m_taken)return false;m_priority=priority;
            for(const auto &entry:m_nodes)if(entry.second.provider)entry.second.provider->setPriority(priority);else if(!entry.second.resource)m_payload->setPriority(entry.second.key,priority);
            if(m_decoding)m_decode->setPriority(1,priority);return true;
        }
        bool take(ResourcePayloadResult &output)
        {
            output={};if(!owner()||!m_hasRequest||m_taken)return false;
            try {if(!m_ready)advance();}catch(const std::exception &failure){fail(failure.what());}
            if(!m_ready)return false;
            output.key=m_key;output.generation=m_generation;output.resource=m_result;report(output.error,m_error.c_str());
            m_result.reset();retireProviders(m_nodes);m_nodes.clear();m_taken=true;return true;
        }
        void cancel()
        {
            if(!owner())return;if(m_payload)m_payload->cancel();if(m_decode)m_decode->cancel();
            retireProviders(m_nodes);m_nodes.clear();m_result.reset();m_error.clear();m_hasRequest=false;m_ready=false;m_taken=false;m_decoding=false;
            if(m_generation!=std::numeric_limits<u64>::max())++m_generation;
        }
        void drain(){if(!owner())return;cancel();m_payload->drain();m_decode->drain();}
        u64 generation() const{return m_generation;}
    private:
        template<class K,class V>using Map=payload_stream_detail::Map<K,V>;
        struct Node
        {
            StreamBudget metadata;
            ResourceID id;
            std::shared_ptr<const RuntimeResource> resource;
            std::shared_ptr<ResourceLeafProvider> provider;
            std::shared_ptr<const RuntimeResourceRangeInfo> pin;
            std::weak_ptr<ResourceLeafProviderFactory> observedFactory;
            u64 key=0,payloadBytes=0,metadataBytes=0;
            u32 height=0;
            bool visiting=false;
        };
        struct Plan {Map<String,Node> nodes;u64 payloadBytes=0,workingBytes=0;bool complete=true;};
        struct Inputs
        {
            Map<String,Node> nodes;
            ResourceID root;
            std::shared_ptr<WorldStreamingScheduler> scheduler;
        };
        struct OwnedNode
        {
            StreamBudget storage;
            RuntimeResource resource;
        };
        ResourceClosureStream()=default;
        bool owner() const{return m_owner==std::this_thread::get_id();}
        static void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}
        static void report(String &output,const char *message) noexcept
        {try{output=message;}catch(...){try{output="Closure failed";}catch(...){output.clear();}}}
        static void retireProviders(Map<String,Node> &nodes,const Map<String,Node> *keep=nullptr)
        {
            for(const auto &entry:nodes)if(entry.second.provider){const auto retained=keep?keep->find(entry.first):nodes.end();
                if(keep&&retained!=keep->end()&&retained->second.provider==entry.second.provider)continue;entry.second.provider->cancel();}
            for(const auto &entry:nodes)if(entry.second.provider){const auto retained=keep?keep->find(entry.first):nodes.end();
                if(keep&&retained!=keep->end()&&retained->second.provider==entry.second.provider)continue;entry.second.provider->drain();}
        }
        void validateResidentEstimates() const
        {
            const auto &captured=m_payload->runtimeLimits();const auto per=std::min(m_limits.maximumResourcePayloadBytes,captured.maxPayloadBytes);
            const auto closure=std::min(m_limits.maximumClosurePayloadBytes,captured.maxClosurePayloadBytes);
            const auto working=m_scheduler->stats().limits[size_t(StreamMemoryClass::WorkingCpu)];u64 bytes=0,scratch=0;
            for(const auto &entry:m_nodes){const auto size=entry.second.provider?entry.second.provider->maximumResidentBytes():entry.second.payloadBytes;
                require(size<=per&&size<=closure-bytes,"Dynamic sparse leaf estimate exceeds closure payload limits");
                require(size<=working-scratch&&entry.second.metadataBytes<=working-scratch-size,"Dynamic sparse leaf estimate exceeds working capacity");
                bytes+=size;scratch+=size+entry.second.metadataBytes;}
        }
        Plan prepare(const ResourceID &root,const ResourceClosureLimits &limits,s32 priority,const std::shared_ptr<ResourceLeafProviderFactory> &factory)
        {
            Plan plan;const auto workingLimit=m_scheduler->stats().limits[size_t(StreamMemoryClass::WorkingCpu)];
            const auto &captured=m_payload->runtimeLimits();
            const auto maximumDepth=std::min(limits.maximumDepth,captured.maxDependencyDepth);
            const auto maximumResources=std::min(limits.maximumResources,captured.maxClosureResources);
            const auto maximumPayload=std::min(limits.maximumResourcePayloadBytes,captured.maxPayloadBytes);
            const auto maximumClosure=std::min(limits.maximumClosurePayloadBytes,captured.maxClosurePayloadBytes);
            std::function<u32(const ResourceID &,u32)> visit;
            visit=[&](const ResourceID &id,u32 depth)->u32 {
                require(depth<=maximumDepth&&id.str().size()<=1024,"Closure dependency depth/identity exceeds its limit");
                const auto present=plan.nodes.find(id.str());
                if(present!=plan.nodes.end()) {
                    require(!present->second.visiting,"Runtime install dependency cycle");
                    require(present->second.height<=maximumDepth-depth+1,"Closure shared subtree exceeds its depth limit");
                    return present->second.height;
                }
                require(plan.nodes.size()<maximumResources,"Closure unique resource count exceeds its limit");
                String error;u64 bytes=0,metadata=0;std::shared_ptr<const RuntimeResourceRangeInfo> pin;
                std::shared_ptr<ResourceLeafProvider> provider;const auto previous=m_nodes.find(id.str());
                if(factory&&factory->handles(id)) {
                    if(previous!=m_nodes.end()&&previous->second.provider){provider=previous->second.provider;pin=previous->second.pin;}
                    else {
                        auto info=std::allocate_shared<RuntimeResourceRangeInfo>(SourceBudgetAllocator<RuntimeResourceRangeInfo>{});
                        const bool found=m_payload->mount()->resourceInfo(id,*info,error);require(found,error.empty()?"Missing sparse resource pin":error.c_str());
                        // Earlier manifests cannot verify partial bytes; retain
                        // the original fully verified whole-resource path.
                        if(info->independentlyVerified){pin=info;provider=factory->create(m_payload->mount(),id,*info,priority,error);
                            require(provider!=nullptr,error.empty()?"Sparse leaf provider creation failed":error.c_str());}
                    }
                }
                if(provider){bytes=provider->maximumResidentBytes();metadata=pin->metadataBytes;}
                else {const bool planned=m_payload->memoryRequirements(id,bytes,metadata,error);require(planned,error.empty()?"Missing closure dependency":error.c_str());}
                require(bytes<=maximumPayload&&bytes<=maximumClosure-plan.payloadBytes,
                    "Closure payload exceeds its per-resource or aggregate limit");
                require(bytes<=workingLimit-plan.workingBytes&&metadata<=workingLimit-plan.workingBytes-bytes,
                    "Closure CPU assembly exceeds its working budget");
                auto budget=m_payload->reserve(StreamMemoryClass::PendingCpu,1024+8*id.str().size(),error);
                require(budget!=nullptr,error.empty()?"Closure metadata budget exhausted":error.c_str());
                Node node;node.metadata=std::move(budget);node.id=id;node.payloadBytes=bytes;node.metadataBytes=metadata;node.visiting=true;node.provider=provider;node.pin=pin;
                if(previous!=m_nodes.end()){node.key=previous->second.key;node.observedFactory=previous->second.observedFactory;if(bool(previous->second.provider)==bool(provider))node.resource=previous->second.resource;}
                else {require(m_nextKey!=std::numeric_limits<u64>::max(),"Closure node identity exhausted");node.key=m_nextKey++;}
                plan.payloadBytes+=bytes;plan.workingBytes+=bytes+metadata;
                auto &stored=plan.nodes.emplace(id.str(),std::move(node)).first->second;
                u32 height=1;
                if(stored.resource&&!provider&&factory&&stored.observedFactory.lock()!=factory) {
                    const bool observed=factory->observeResourceReady(stored.resource,error);require(observed,error.empty()?"Resource observation failed":error.c_str());stored.observedFactory=factory;
                }
                if(stored.resource)require(stored.resource->header.resourceId==id&&(provider?stored.resource->payload.size()<=bytes:stored.resource->payload.size()==bytes),"Closure payload identity changed");
                else plan.complete=false;
                const auto *header=provider?&pin->header:stored.resource?&stored.resource->header:nullptr;
                if(header)for(const auto &child:header->installDependencies)height=std::max(height,1+visit(child,depth+1));
                stored.height=height;stored.visiting=false;return height;
            };
            visit(root,1);return plan;
        }
        static Array<ResourcePayloadDemand> demands(const Map<String,Node> &nodes,const ResourceClosureLimits &limits,s32 priority)
        {
            Array<ResourcePayloadDemand> result{Allocator<ResourcePayloadDemand>(sourceBudget)};
            for(const auto &entry:nodes)if(!entry.second.resource&&!entry.second.provider)result.push_back({entry.second.key,entry.second.id,limits.maximumResourcePayloadBytes,priority});
            return result;
        }
        static WorldStreamResult assemble(const std::shared_ptr<const Inputs> &input,const StreamCancellation &cancel)
        {
            WorldStreamResult result;Map<String,std::shared_ptr<const RuntimeResource>> completed;
            std::function<std::shared_ptr<const RuntimeResource>(const ResourceID &)> clone;
            clone=[&](const ResourceID &id)->std::shared_ptr<const RuntimeResource> {
                require(!cancel.cancelled(),"Closure assembly cancelled");
                const auto old=completed.find(id.str());if(old!=completed.end())return old->second;
                const auto &node=input->nodes.at(id.str());require(node.resource!=nullptr,"Closure payload is not ready");
                String error;require(node.payloadBytes<=std::numeric_limits<u64>::max()-node.metadataBytes,"Closure retained storage overflow");
                auto storage=input->scheduler->reserve(StreamMemoryClass::SourceCpu,node.payloadBytes+node.metadataBytes,error);
                require(storage!=nullptr,error.empty()?"Closure retained storage budget exhausted":error.c_str());
                auto owner=std::allocate_shared<OwnedNode>(SourceBudgetAllocator<OwnedNode>{});owner->storage=std::move(storage);
                owner->resource.header=node.resource->header;owner->resource.streamData=node.resource->streamData;owner->resource.payload.resize(node.resource->payload.size());
                for(size_t at=0;at<node.resource->payload.size();) {
                    require(!cancel.cancelled(),"Closure assembly cancelled");const auto count=std::min<size_t>(65536,node.resource->payload.size()-at);
                    std::memcpy(owner->resource.payload.data()+at,node.resource->payload.data()+at,count);at+=count;
                }
                owner->resource.dependencies.reserve(node.resource->header.installDependencies.size());
                for(const auto &child:node.resource->header.installDependencies)owner->resource.dependencies.push_back(clone(child));
                auto resource=std::shared_ptr<const RuntimeResource>(owner,&owner->resource);completed.emplace(id.str(),resource);return resource;
            };
            result.payload=clone(input->root);return result;
        }
        void startDecode(u64 workingBytes)
        {
            try {
                auto input=std::allocate_shared<Inputs>(SourceBudgetAllocator<Inputs>{});input->nodes=m_nodes;input->root=m_root;input->scheduler=m_scheduler;
                // Mutable owner providers are never captured by CPU workers.
                // Only their immutable ready snapshots participate in assembly.
                for(auto &entry:input->nodes){entry.second.provider.reset();entry.second.pin.reset();entry.second.observedFactory.reset();}
                WorldStreamRequest request;request.key=1;request.priority=m_priority;request.workingBytes=workingBytes;
                request.pendingBytes=1024+128*input->nodes.size();request.execute=[input](const StreamCancellation &cancel){return assemble(input,cancel);};
                String error;if(!m_decode->replace({request},error)){fail(error.c_str());return;}
                m_decodeGeneration=m_decode->stats().generation;m_decoding=true;
            }catch(const std::exception &failure){fail(failure.what());}
        }
        void advance()
        {
            if(m_decoding) {
                WorldStreamResult result;if(!m_decode->take(result))return;
                if(result.generation!=m_decodeGeneration)return;
                if(!result.payload){fail(result.error.empty()?"Closure assembly returned no resource":result.error.c_str());return;}
                m_result=std::shared_ptr<const RuntimeResource>(result.payload,static_cast<const RuntimeResource*>(result.payload.get()));
                m_ready=true;m_decoding=false;return;
            }
            bool changed=false;ResourcePayloadResult value;
            for(auto &entry:m_nodes)if(entry.second.provider&&!entry.second.resource) {
                validateResidentEstimates();
                // Reused providers inherit an accepted replacement's priority;
                // rejected preparation never mutates the previous request.
                entry.second.provider->setPriority(m_priority);
                String error;std::shared_ptr<const RuntimeResource> ready;const auto status=entry.second.provider->poll(ready,error);
                validateResidentEstimates();
                if(status==ResourceLeafStatus::Error){fail(error.empty()?"Sparse leaf loading failed":error.c_str());return;}
                if(status==ResourceLeafStatus::Ready) {
                    const auto &pin=entry.second.pin->header;
                    require(ready&&ready->streamData&&ready->dependencies.empty()&&ready->header.isValid()&&ready->header.resourceId==pin.resourceId&&
                        ready->header.resourceType==pin.resourceType&&ready->header.compilerVersion==pin.compilerVersion&&ready->header.sourceHash==pin.sourceHash&&
                        ready->header.payloadHash==pin.payloadHash&&ready->header.payloadSize==pin.payloadSize&&ready->header.installDependencies==pin.installDependencies&&
                        ready->payload.size()<=entry.second.provider->maximumResidentBytes(),"Sparse leaf result differs from its captured pin or resident allowance");
                    entry.second.resource=std::move(ready);changed=true;
                }
            }
            while(m_payload->take(value)) {
                auto found=std::find_if(m_nodes.begin(),m_nodes.end(),[&](const auto &entry){return entry.second.key==value.key;});
                if(found==m_nodes.end())continue;
                if(!value.resource){fail(value.error.empty()?"Closure payload loading failed":value.error.c_str());return;}
                if(m_factory){String error;if(!m_factory->observeResourceReady(value.resource,error)){fail(error.empty()?"Resource observation failed":error.c_str());return;}found->second.observedFactory=m_factory;}
                found->second.resource=std::move(value.resource);changed=true;
            }
            if(!changed)return;
            auto plan=prepare(m_root,m_limits,m_priority,m_factory);auto demand=demands(plan.nodes,m_limits,m_priority);String error;
            if(!m_payload->replace(demand,error)){fail(error.c_str());return;}
            m_nodes.swap(plan.nodes);retireProviders(plan.nodes,&m_nodes);if(plan.complete)startDecode(plan.workingBytes);
        }
        void fail(const char *message)
        {
            report(m_error,message);m_payload->cancel();m_decode->cancel();retireProviders(m_nodes);m_nodes.clear();m_result.reset();
            m_ready=true;m_decoding=false;
        }
        std::shared_ptr<WorldStreamingScheduler> m_scheduler;
        std::shared_ptr<ResourceLeafProviderFactory> m_factory;
        std::unique_ptr<ResourcePayloadStream> m_payload;
        std::shared_ptr<WorldStreamProducer> m_decode;
        std::thread::id m_owner;
        Map<String,Node> m_nodes;
        ResourceID m_root;
        ResourceClosureLimits m_limits;
        u64 m_key=0,m_generation=0,m_nextKey=1,m_decodeGeneration=0;
        s32 m_priority=0;
        bool m_hasRequest=false,m_ready=false,m_taken=false,m_decoding=false;
        std::shared_ptr<const RuntimeResource> m_result;
        String m_error;
    };
}
