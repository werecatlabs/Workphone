#define WORKPHONE_MATERIAL_SCENE_PROBE_IMPLEMENTATION
#include "../../Samples/cpp/VehicleAdvanced/MaterialSceneCpuAttributionProbe.hpp"
#include <iostream>
#include <stdexcept>
using namespace material_scene_cpu;
extern "C" int wp_material_scene_test_native_draw(int split,int fail);
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
        { StageScope authoring(Authoring);burn(20000); }
        { StageScope publication(EditPublication);burn(20000); }
        { StageScope observer(EditVerification);burn(40000); }
        read(&m);
        require(m.stageSegments[Authoring]==1 && m.stageCycles[Authoring]>0 &&
                m.stageSegments[EditPublication]==1 && m.stageCycles[EditPublication]>0 &&
                m.stageSegments[EditVerification]==1 && m.stageCycles[EditVerification]>0 &&
                m.stageSegments[Capture]==1 && m.stageSegments[Binding]==2,
                "Full edit authoring, publication and observer intervals retain independent attribution");
        configure(1,123,false);{ MaterialScope section;BindingScope binding;successfulBind(); }
        read(&m);require(!m.bindCalls && !m.visitedSections,"Disabled scene probes record nothing");
        wp_material_scene_native_set_callback(nativeEvent);
        configure(1,123,true);
        const auto nativeStart=material_scene_cpu::clock();
        { MaterialScope section;section.stop();require(wp_material_scene_test_native_draw(0,0)==1,"C fixture draws");section.resume(); }
        const auto nativeTotal=material_scene_cpu::clock()-nativeStart;
        read(&m);
        require(!m.errors && m.nativeDrawCalls==1 && m.nativeCombinedDraws==1 && !m.nativeSplitDraws &&
                !m.nativeUnscopedDrawCalls && m.nativeSampledDraws==1 && m.nativeUploadCalls==2 && m.nativeAuxUploadCalls==1 &&
                m.nativeBindingRegions==2 && m.stageSegments[NativeUpload]==2 && m.stageSegments[NativeBinding]==2,
                "Plain C events reach owner TLS and preserve native path/count identities");
        require(m.stageCycles[NativeUpload]>m.stageCycles[NativeBinding] &&
                m.stageCycles[NativeUpload]+m.stageCycles[NativeBinding]+m.materialCycles<nativeTotal/2,
                "C upload and binding are disjoint and exclude simulated IA/MVP/draw work");
        configure(1,123,true);require(wp_material_scene_test_native_draw(1,1)==0,"C upload failure returns");read(&m);
        require(!m.errors && m.nativeSplitDraws==1 && m.nativeUnscopedDrawCalls==1 && m.nativeFailedUploads==1 &&
                m.nativeFailedDraws==1 && !context.nativeDraw && m.stageSegments[NativeUpload]==1,
                "Unscoped native calls are sampled and early failures close every interval");
        configure(32,123,true);for(unsigned i=0;i<4096;++i) {
            nativeEvent(WP_SCENE_NATIVE_DRAW_BEGIN,0);nativeEvent(WP_SCENE_NATIVE_DRAW_END,1);
        }
        read(&m);require(!m.errors && m.nativeUnscopedDrawCalls==4096 && m.nativeSampledDraws>50 && m.nativeSampledDraws<250,
                "Native calls outside C++ material sections receive independent sampling coverage");
        configure(1,123,true);nativeEvent(WP_SCENE_NATIVE_BIND_END,1);read(&m);
        require(m.errors>0,"Unbalanced C events cannot silently produce attribution");
        std::cout<<"PASS: scene preparation, binding/unbinding, native exclusion, capture and disabled mode\n";
        return 0;
    }
    catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
