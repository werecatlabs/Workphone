#define WORKPHONE_MATERIAL_SCENE_PROBE_IMPLEMENTATION
#define WORKPHONE_MATERIAL_SCENE_NATIVE_BRIDGE
#include "../../Samples/cpp/VehicleAdvanced/MaterialSceneCpuPhaseProbe.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace material_scene_cpu;
extern "C" int wp_scene_test_phase_draw(int,int);
extern "C" void wp_scene_test_phase_events(int);
static void require(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
static void exact(const Metrics &m,const NativePhaseMetrics &p)
{
    uint64_t cycles=0,segments=0,regions=0;
    for(unsigned i=0;i<NativePhaseCount;++i) { cycles+=p.cycles[i];segments+=p.segments[i];regions+=p.regions[i]; }
    require(!m.errors && !p.errors && p.thread==m.thread && p.stride==m.stride && p.draws==m.nativeDrawCalls && p.sampledDraws==m.nativeSampledDraws,
            "Phase coverage retains the same owner, sampling and draw identities");
    require(cycles==m.stageCycles[NativeBinding] && segments==m.stageSegments[NativeBinding] && regions==m.nativeBindingRegions,
            "Phase cycles, sampled segments and all-region counts exactly partition aggregate binding");
}
int main()
{
    try {
        const auto api=wp_material_scene_cpu_v4();const auto phases=wp_material_scene_native_phases_v1();
        require(api->version==4 && api->bytes==sizeof(Api) && phases->version==1 && phases->bytes==sizeof(NativePhaseApi),"Aggregate ABI remains v4, phase sidecar is versioned separately");
        api->calibrate();phases->enable(true);api->configure(1,123,true);
        const auto started=material_scene_cpu::clock();
        { MaterialScope section;section.stop();require(wp_scene_test_phase_draw(1,0)==1,"Plain C cold draw succeeds");section.resume(); }
        const auto total=material_scene_cpu::clock()-started;
        Metrics m;NativePhaseMetrics p;api->read(&m);phases->read(&p);exact(m,p);
        for(unsigned i=0;i<6;++i) require(p.regions[i]==1 && p.segments[i]==1,"Every cold phase counted once");
        require(p.cycles[TextureSampler]>p.cycles[FixedState] && p.cycles[TextureSampler]>p.cycles[Transfer],"C fixture's independent heavy resource work is attributed to its phase");
        require(m.stageCycles[NativeBinding]+m.stageCycles[NativeUpload]<total/2,"Native execution is outside phase and upload clocks");
        phases->enable(true);api->configure(1,123,true);require(wp_scene_test_phase_draw(0,0)==1,"Warm C draw succeeds");api->read(&m);phases->read(&p);exact(m,p);
        require(!p.regions[TerrainShadow] && !p.regions[MaterialShader] && p.regions[TextureSampler]==1,"Warm skips retain conditional fixed binding counts");
        phases->enable(true);api->configure(1,123,true);require(wp_scene_test_phase_draw(1,1)==0,"C upload failure returns");api->read(&m);phases->read(&p);exact(m,p);
        require(m.nativeFailedDraws==1 && m.nativeFailedUploads==1 && !p.regions[TextureSampler],"Failure closes all entered phases without inventing later work");
        Metrics aggregate;
        for(bool enabled:{false,true}) {
            phases->enable(enabled);api->configure(32,456,true);
            for(unsigned i=0;i<4096;++i) wp_scene_test_phase_events(i%3==0);
            api->read(&m);phases->read(&p);
            if(!enabled) { aggregate=m;for(auto n:p.regions) require(!n,"Aggregate-only mode does not collect phase regions"); }
            else { exact(m,p);require(m.nativeSampledDraws==aggregate.nativeSampledDraws && m.stageSegments[NativeBinding]==aggregate.stageSegments[NativeBinding],"Phase enabling preserves sampled draws and clock-pair counts"); }
        }
        phases->enable(true);api->configure(1,123,false);wp_scene_test_phase_events(1);api->read(&m);phases->read(&p);
        require(!m.nativeDrawCalls && !p.regions[0],"Inactive mode records neither aggregate nor phases");
        for(int bad:{0,7}) {
            context={};phases->enable(true);api->configure(1,123,true);
            nativePhaseEvent(WP_SCENE_NATIVE_DRAW_BEGIN,0);nativePhaseEvent(WP_SCENE_NATIVE_BIND_BEGIN,bad);
            nativePhaseEvent(WP_SCENE_NATIVE_BIND_END,1);nativePhaseEvent(WP_SCENE_NATIVE_DRAW_END,1);
            phases->read(&p);require(p.errors>0,"Invalid phase tags cannot silently report CPU");
        }
        context={};phases->enable(true);api->configure(1,123,true);
        nativePhaseEvent(WP_SCENE_NATIVE_DRAW_BEGIN,0);nativePhaseEvent(WP_SCENE_NATIVE_BIND_BEGIN,1);
        nativePhaseEvent(WP_SCENE_NATIVE_UPLOAD_BEGIN,0);nativePhaseEvent(WP_SCENE_NATIVE_UPLOAD_END,1);
        nativePhaseEvent(WP_SCENE_NATIVE_BIND_END,1);nativePhaseEvent(WP_SCENE_NATIVE_DRAW_END,1);phases->read(&p);
        require(p.errors>0,"Overlapping upload and binding must be rejected");
        context={};phases->enable(true);api->configure(1,123,true);nativePhaseEvent(WP_SCENE_NATIVE_DRAW_BEGIN,0);
        nativePhaseEvent(WP_SCENE_NATIVE_BIND_BEGIN,1);phases->read(&p);require(p.errors>0,"Read with an open phase is invalid");
        NativePhaseMetrics foreign;std::thread other([&] { phases->read(&foreign); });other.join();
        require(foreign.errors>0,"Cross-thread reads cannot claim owner attribution");
        std::cout<<"PASS: native phase partition, C bridge, sampling parity, disabled/failure/invalid boundaries\n";return 0;
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
