#pragma once
// Keep the v4 aggregate ABI and clocks intact. Only this test overlay replaces
// its export to register the phase-forwarding C callback.
#ifndef WORKPHONE_MATERIAL_SCENE_NO_EXPORT
#define WORKPHONE_MATERIAL_PHASE_EXPORT
#define WORKPHONE_MATERIAL_SCENE_NO_EXPORT
#include "MaterialSceneCpuAttributionProbe.hpp"
#undef WORKPHONE_MATERIAL_SCENE_NO_EXPORT
#else
#include "MaterialSceneCpuAttributionProbe.hpp"
#endif
#include "MaterialSceneCpuNativePhases.hpp"
#ifdef WORKPHONE_MATERIAL_SCENE_PROBE_IMPLEMENTATION
namespace material_scene_cpu
{
    struct NativePhaseContext
    {
        NativePhaseMetrics metrics;
        uint64_t beforeCycles=0, beforeSegments=0;
        int active=-1;
        bool enabled=false;
    };
    inline thread_local NativePhaseContext nativePhases;
    inline void enableNativePhases(bool enabled)
    { nativePhases={}; nativePhases.enabled=enabled; }
    inline void configureWithNativePhases(uint32_t stride,uint32_t seed,bool enabled)
    {
        const auto phasesEnabled=nativePhases.enabled && enabled;
        configure(stride,seed,enabled);
        nativePhases={}; nativePhases.enabled=phasesEnabled;
    }
    inline void nativePhaseEvent(int event,int value)
    {
        if(!context.enabled || !nativePhases.enabled) { nativeEvent(event,value);return; }
        auto &p=nativePhases;auto &m=p.metrics;
        if(event==WP_SCENE_NATIVE_BIND_BEGIN)
        {
            // These operations precede the existing begin clock.
            if(value<1 || value>int(NativePhaseCount) || p.active!=-1 || !context.nativeDraw ||
               context.active[NativeBinding] || context.active[NativeUpload]) ++m.errors;
            else
            {
                p.active=value-1;++m.regions[p.active];
                p.beforeCycles=context.metrics.stageCycles[NativeBinding];
                p.beforeSegments=context.metrics.stageSegments[NativeBinding];
            }
            nativeEvent(event,value);return;
        }
        if(event==WP_SCENE_NATIVE_BIND_END)
        {
            nativeEvent(event,value); // Close the same aggregate clock first.
            if(p.active<0) { ++m.errors;return; }
            const auto cycles=context.metrics.stageCycles[NativeBinding];
            const auto segments=context.metrics.stageSegments[NativeBinding];
            if(cycles<p.beforeCycles || segments<p.beforeSegments || segments-p.beforeSegments>1) ++m.errors;
            else { m.cycles[p.active]+=cycles-p.beforeCycles;m.segments[p.active]+=segments-p.beforeSegments; }
            p.active=-1;return;
        }
        if((event==WP_SCENE_NATIVE_DRAW_BEGIN || event==WP_SCENE_NATIVE_DRAW_END ||
            event==WP_SCENE_NATIVE_UPLOAD_BEGIN) && p.active!=-1) ++m.errors;
        nativeEvent(event,value);
    }
    inline void readNativePhases(NativePhaseMetrics *out)
    {
        auto &p=nativePhases;
        if(p.active!=-1 || GetCurrentThreadId()!=context.metrics.thread) ++p.metrics.errors;
        p.metrics.thread=context.metrics.thread;p.metrics.stride=context.metrics.stride;
        p.metrics.draws=context.metrics.nativeDrawCalls;p.metrics.sampledDraws=context.metrics.nativeSampledDraws;
        if(out) *out=p.metrics;
    }
}
#ifdef WORKPHONE_MATERIAL_PHASE_EXPORT
extern "C" __declspec(dllexport) const material_scene_cpu::Api *wp_material_scene_cpu_v4()
{
    using namespace material_scene_cpu;
#ifdef WORKPHONE_MATERIAL_SCENE_NATIVE_BRIDGE
    wp_material_scene_native_set_callback(nativePhaseEvent);
#endif
    static const Api api{Version,sizeof(Api),configureWithNativePhases,begin,end,read,calibrate};return &api;
}
extern "C" __declspec(dllexport) const material_scene_cpu::NativePhaseApi *wp_material_scene_native_phases_v1()
{
    using namespace material_scene_cpu;
    static const NativePhaseApi api{NativePhaseVersion,sizeof(NativePhaseApi),enableNativePhases,readNativePhases};return &api;
}
#endif
#endif
