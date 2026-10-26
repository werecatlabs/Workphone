#define WORKPHONE_MATERIAL_CPU_PROBE_IMPLEMENTATION
#include "MaterialCpuAttributionProbe.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace material_cpu_attribution;
static void require(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
static void burn(unsigned count) { volatile uint64_t value=1; for(unsigned i=0;i<count;++i) value=value*1664525+1013904223; }
int main()
{
    try
    {
        calibrate(); configure(1,123,false);
        { MaterialScope scope; burn(1000); }
        Metrics disabled{}; read(&disabled);
        require(!disabled.visitedSections && !disabled.materialCycles,"Disabled probes record no work");
        configure(1,123,true);
        const auto totalStart=material_cpu_attribution::clock();
        { MaterialScope scope; burn(100000); scope.stop(); burn(4000000); Sleep(20); scope.resume(); burn(100000); }
        const auto total=material_cpu_attribution::clock()-totalStart;
        Metrics enabled{}; read(&enabled);
        require(!enabled.errors && enabled.visitedSections==1 && enabled.sampledSections==1 &&
                enabled.materialSegments==2 && enabled.materialCycles>0 && enabled.materialCycles<total/4,
                "Paused native work is excluded while both material intervals are included");
        require(enabled.overheadMax>=enabled.overheadMedian && enabled.overheadMedian>0,"Counter overhead is calibrated");
        uint64_t selected=0;
        for(unsigned repeat=0;repeat<2;++repeat)
        {
            configure(32,456,true);
            for(unsigned i=0;i<4096;++i) { MaterialScope scope; }
            Metrics result{};read(&result);
            require(result.visitedSections==4096 && result.sampledSections>50 && result.sampledSections<250,
                    "Deterministic sampling covers the material population");
            if(repeat) require(result.sampledSections==selected,"Identical seeds select identical sections");
            selected=result.sampledSections;
        }
        configure(1,789,true);begin(Publication);burn(10000);end(Publication);
        Metrics parent{};read(&parent);
        require(parent.stageSegments[Publication]==1 && parent.stageCycles[Publication]>0,"Owner stages count CPU cycles");
        std::thread other([] { configure(1,987,true); MaterialScope scope; burn(10000); });other.join();
        Metrics after{};read(&after);
        require(after.stageCycles[Publication]==parent.stageCycles[Publication] && !after.visitedSections,
                "Probe state is confined to its Windows owner thread");
        std::cout<<"PASS: owner-thread cycles, paused native work, deterministic sampling, disabled mode and calibration\n";
        return 0;
    }
    catch(const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
