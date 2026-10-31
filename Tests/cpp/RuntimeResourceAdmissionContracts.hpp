#pragma once
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <filesystem>
#include <stdexcept>

namespace runtime_resource_admission_contracts
{
    using namespace workphone;
    using namespace workphone::resource;
    inline void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}

    inline std::shared_ptr<const RuntimeResource> find(std::shared_ptr<const RuntimeResource> node,const ResourceID &id)
    {
        if(node->header.resourceId==id)return node;
        for(const auto &child:node->dependencies)if(auto result=find(child,id))return result;
        return {};
    }

    inline void run(const RuntimeResourceConfig &config,const ResourceID &rootId,const ResourceID &childId,const String &uuid)
    {
        auto scheduler=WorldStreamingScheduler::shared();String error;
        const auto original=scheduler->stats();const auto source=size_t(StreamMemoryClass::SourceCpu),working=size_t(StreamMemoryClass::WorkingCpu);
        struct Restore {
            std::shared_ptr<WorldStreamingScheduler> scheduler;WorldStreamStats original;
            ~Restore(){String ignored;scheduler->setBudget(StreamMemoryClass::SourceCpu,original.limits[size_t(StreamMemoryClass::SourceCpu)],ignored);
                scheduler->setBudget(StreamMemoryClass::WorkingCpu,original.limits[size_t(StreamMemoryClass::WorkingCpu)],ignored);}
        } restore{scheduler,original};

        // A syntactically valid manifest must be refused before its container,
        // parser copies or captured maps can allocate without an admission.
        {
            ResourceSystem denied(nullptr,nullptr);const auto state=scheduler->stats();
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu,state.used[source]+2048,error),"Cannot constrain manifest source admission");
            const bool mounted=denied.initializeRuntime(config,error);
            require(!mounted&&!denied.isInitialized(),"Manifest mount bypassed SourceCpu admission");
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu,original.limits[source],error),"Cannot restore manifest source budget");
            require(scheduler->setBudget(StreamMemoryClass::WorkingCpu,0,error),"Cannot constrain manifest working admission");
            require(!denied.initializeRuntime(config,error)&&!denied.isInitialized(),"Manifest parser bypassed WorkingCpu admission");
            require(scheduler->setBudget(StreamMemoryClass::WorkingCpu,original.limits[working],error),"Cannot restore manifest working budget");
        }

        const auto bytes=std::filesystem::file_size(std::filesystem::u8path(config.manifestPath.c_str()));
        const u64 manifestBytes=65536+12*bytes;
        auto admission=scheduler->reserve(StreamMemoryClass::SourceCpu,manifestBytes,error);
        auto scratch=scheduler->reserve(StreamMemoryClass::WorkingCpu,manifestBytes,error);
        require(admission&&scratch,"Cannot prepare adopted manifest admissions");
        ResourceSystem runtime(nullptr,nullptr);
        require(!runtime.initializeRuntime(config,error,scratch,scratch),"Manifest accepted a WorkingCpu lease as retained SourceCpu");
        require(!runtime.initializeRuntime(config,error,admission,admission),"Manifest accepted a SourceCpu lease as parser WorkingCpu");
        auto small=scheduler->reserve(StreamMemoryClass::SourceCpu,manifestBytes-1,error);
        require(small&&!runtime.initializeRuntime(config,error,small,scratch),"Manifest accepted an undersized adopted admission");small.reset();
        std::weak_ptr<StreamBudgetLease> manifestOwner=admission;
        require(runtime.initializeRuntime(config,error,admission,scratch),"Valid adopted manifest admissions were refused");
        admission.reset();scratch.reset();require(!manifestOwner.expired(),"Mount did not retain its adopted manifest lease");

        const auto mounted=scheduler->stats();
        require(scheduler->setBudget(StreamMemoryClass::SourceCpu,mounted.used[source]+2048,error),"Cannot constrain synchronous resource source admission");
        require(!runtime.load(rootId,error),"Synchronous resource payload bypassed SourceCpu admission");
        require(scheduler->setBudget(StreamMemoryClass::SourceCpu,original.limits[source],error),"Cannot restore synchronous source budget");
        require(scheduler->setBudget(StreamMemoryClass::WorkingCpu,0,error),"Cannot constrain synchronous reader scratch");
        require(!runtime.load(rootId,error),"Synchronous resource reader bypassed WorkingCpu admission");
        require(scheduler->setBudget(StreamMemoryClass::WorkingCpu,original.limits[working],error),"Cannot restore synchronous working budget");
        auto root=runtime.loadUUID(uuid,error);require(root!=nullptr,"Admission failure poisoned the runtime generation cache");
        require(runtime.load(rootId,error)==root,"Cached loads lost their admitted resource identity");
        auto escaped=find(root,childId);require(escaped!=nullptr,"Admission fixture has no child to escape");
        const auto expected=escaped->payload;root.reset();runtime.clearRuntimeCache();runtime.shutdown();
        require(manifestOwner.expired(),"Shutdown retained the captured manifest admission");manifestOwner.reset();
        require(escaped->payload==expected,"Shutdown invalidated a separately retained child payload");
        const auto retained=scheduler->stats().used[source];escaped.reset();
        const auto retired=scheduler->stats().used[source];
        require(retained>=retired+8192,"Escaped child failed to retain and release its own SourceCpu admission");
    }
}
