#define WORKPHONE_MATERIAL_SCENE_PROBE_IMPLEMENTATION
#include "../../Samples/cpp/VehicleAdvanced/MaterialSceneCpuAttributionProbe.hpp"
#include <iostream>
#include <stdexcept>
using namespace material_scene_cpu;
static void burn(unsigned count) { volatile uint64_t value=1;while(count--) value=value*1664525+1013904223; }
static void require(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
int main()
{
    try
    {
        calibrate();configure(1,123,true);
        const auto start=material_scene_cpu::clock();
        {
            MaterialScope section;burn(20000);section.stop();
            { BindingScope binding;burn(200000);successfulBind(); }
            burn(4000000); // Represents excluded native draw work.
            section.resume();
            { BindingScope unbinding(true);burn(200000); }
            burn(20000);
        }
        const auto total=material_scene_cpu::clock()-start;
        Metrics m;read(&m);
        require(!m.errors && !context.section && m.sampledSections==1 && m.bindCalls==1 && m.successfulBinds==1 && m.unbindCalls==1,
                "Real material sections own and retire nested binding probes");
        require(m.materialSegments==3 && m.stageSegments[Binding]==2 && m.stageCycles[Binding]>m.materialCycles &&
                m.stageCycles[Binding]+m.materialCycles<total/2,
                "Binding and preparation intervals are disjoint and native draw CPU is excluded");
        { StageScope capture(Capture);burn(20000); }
        read(&m);require(m.stageSegments[Capture]==1 && m.stageCycles[Capture]>0,"Complete record capture is counted");
        configure(1,123,false);{ MaterialScope section;BindingScope binding;successfulBind(); }
        read(&m);require(!m.bindCalls && !m.visitedSections,"Disabled scene probes record nothing");
        std::cout<<"PASS: scene preparation, binding/unbinding, native exclusion, capture and disabled mode\n";
        return 0;
    }
    catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
