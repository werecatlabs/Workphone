#pragma once
#include <Workphone/System/ResourceClosureStream.hpp>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace resource_closure_stream_contracts
{
    using namespace workphone;
    using namespace workphone::resource;
    inline void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}
    inline ResourcePayloadResult wait(ResourceClosureStream &stream)
    {
        ResourcePayloadResult result;const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(!stream.take(result)&&std::chrono::steady_clock::now()<until)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(result.generation!=0,"Closure completes before its bounded deadline");return result;
    }
    inline void run(const RuntimeResourceConfig &config,const ResourceID &root)
    {
        String error;ResourceClosureLimits limits;limits.maximumResourcePayloadBytes=1024;limits.maximumClosurePayloadBytes=2048;
        limits.maximumResources=2;limits.maximumDepth=2;
        {
            auto scheduler=std::make_shared<WorldStreamingScheduler>();
            auto mount=ResourcePayloadMount::create(config,scheduler,{},error);require(mount!=nullptr,error.c_str());
            auto probe=ResourcePayloadStream::create(mount,error);require(probe!=nullptr,error.c_str());
            auto first=ResourceClosureStream::create(mount,error),second=ResourceClosureStream::create(mount,error);
            require(first&&second,"Two owner closure requesters share one captured mount");
            const auto working=size_t(StreamMemoryClass::WorkingCpu),source=size_t(StreamMemoryClass::SourceCpu);
            auto pressure=scheduler->reserve(StreamMemoryClass::WorkingCpu,scheduler->stats().limits[working],error);require(pressure!=nullptr,error.c_str());
            require(first->request(101,root,limits,0,error),error.c_str());require(second->request(102,root,limits,10,error),error.c_str());
            require(first->request(103,root,limits,20,error),error.c_str());
            require(probe->stats().pending==1&&probe->stats().cancelled==0,"Replacing one closure requester retains the shared root read");
            const auto generation=first->generation();auto invalid=limits;invalid.maximumDepth=0;
            require(!first->request(104,root,invalid,0,error)&&first->generation()==generation,"Invalid closure replacement preserves its accepted request");
            require(first->setPriority(100)&&second->setPriority(-100),"Closure priorities reach shared pending payload work");
            pressure.reset();ResourcePayloadResult left,right;
            const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while((!left.resource||!right.resource)&&std::chrono::steady_clock::now()<until) {
                if(!left.resource)first->take(left);if(!right.resource)second->take(right);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            require(left.key==103&&left.resource&&right.key==102&&right.resource,"Both closure requesters receive their current complete result");
            require(left.resource->dependencies.size()==1&&right.resource->dependencies.size()==1&&
                left.resource->dependencies[0]->header.resourceId==ResourceID("data://child.txtres"),
                "Complete graph preserves normal RuntimeResource dependency layout for existing installers");
            require(left.resource->header.payloadHash==right.resource->header.payloadHash&&
                left.resource->dependencies[0]->header.payloadHash==right.resource->dependencies[0]->header.payloadHash,
                "Independent compatible-set graphs contain the same pinned root/child generations");
            require(probe->stats().completed==2,"Two closure consumers share exactly one physical read per unique root/child");
            auto escaped=left.resource->dependencies[0];left={};right={};first->drain();second->drain();
            first.reset();second.reset();probe.reset();mount.reset();
            require(scheduler->stats().used[source]>0,"Escaped cloned dependency retains its own payload/header SourceCpu lease");
            escaped.reset();require(!scheduler->stats().used[source]&&!scheduler->stats().used[working]&&
                !scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)]&&!scheduler->stats().used[size_t(StreamMemoryClass::PendingCpu)],
                "Final closure owners retire all isolated mount, shallow, clone and queue reservations");
        }
        {
            auto scheduler=std::make_shared<WorldStreamingScheduler>();
            auto mount=ResourcePayloadMount::create(config,scheduler,{},error);require(mount!=nullptr,error.c_str());
            auto probe=ResourcePayloadStream::create(mount,error);auto stream=ResourceClosureStream::create(mount,error);
            require(probe&&stream,"Closure limit fixtures initialize");u64 rootBytes=0,metadata=0;
            require(probe->memoryRequirements(root,rootBytes,metadata,error)&&rootBytes>0,"Pinned root has bounded admission metadata");
            for(unsigned test=0;test<3;++test) {
                auto limited=limits;if(test==0)limited.maximumResources=1;if(test==1)limited.maximumDepth=1;
                if(test==2)limited.maximumClosurePayloadBytes=rootBytes;
                const auto before=probe->stats().completed;
                require(stream->request(200+test,root,limited,0,error),error.c_str());auto rejected=wait(*stream);
                require(!rejected.resource&&!rejected.error.empty(),"Count/depth/aggregate limit failures produce explicit closure errors");
                stream->drain();require(probe->stats().completed==before+1,"Invalid discovered closure rejects before reading child payloads");
            }
            auto captured=config;captured.maxDependencyDepth=1;
            auto restrictedMount=ResourcePayloadMount::create(captured,scheduler,{},error);
            require(restrictedMount!=nullptr,error.c_str());auto restricted=ResourceClosureStream::create(restrictedMount,error);
            require(restricted!=nullptr,error.c_str());require(restricted->request(250,root,limits,0,error),error.c_str());
            auto rejected=wait(*restricted);require(!rejected.resource&&!rejected.error.empty(),
                "Closure transport preserves the captured mount's stricter closure limits");restricted->drain();
        }
        {
            // Leave enough working capacity for either shallow read, but not the
            // complete CPU copy. Cancellation must retire a queued assembly job.
            auto scheduler=std::make_shared<WorldStreamingScheduler>();
            auto mount=ResourcePayloadMount::create(config,scheduler,{},error);require(mount!=nullptr,error.c_str());
            auto probe=ResourcePayloadStream::create(mount,error);auto stream=ResourceClosureStream::create(mount,error);
            require(probe&&stream,"Closure cancellation fixture initializes");u64 rootBytes=0,rootMetadata=0,childBytes=0,childMetadata=0;
            require(probe->memoryRequirements(root,rootBytes,rootMetadata,error)&&
                probe->memoryRequirements(ResourceID("data://child.txtres"),childBytes,childMetadata,error),"Closure nodes have captured admission plans");
            const auto working=size_t(StreamMemoryClass::WorkingCpu);
            const auto available=std::max(rootBytes+rootMetadata,childBytes+childMetadata);
            auto pressure=scheduler->reserve(StreamMemoryClass::WorkingCpu,scheduler->stats().limits[working]-available,error);
            require(pressure!=nullptr,error.c_str());require(stream->request(301,root,limits,0,error),error.c_str());
            ResourcePayloadResult stale;const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while((probe->stats().completed<2||scheduler->stats().pending!=1||scheduler->stats().active)&&std::chrono::steady_clock::now()<until) {
                require(!stream->take(stale),"Whole closure assembly remains queued under working pressure");
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            require(probe->stats().completed==2&&scheduler->stats().pending==1,"All shallow reads finish while the asynchronous graph copy waits");
            const auto oldGeneration=stream->generation();stream->cancel();require(!stream->take(stale)&&stream->generation()!=oldGeneration,
                "Cancelled assembly cannot publish a stale closure");
            require(stream->request(302,root,limits,0,error),error.c_str());pressure.reset();auto current=wait(*stream);
            require(current.key==302&&current.resource&&current.generation!=oldGeneration,"A new request survives prior assembly cancellation");
            stream->drain();
        }
    }
}
