#pragma once
#include <cstdint>
#include <windows.h>

// Test-only ABI. The implementation is compiled into one renderer translation
// unit by the mechanical source overlay; shipping sources contain no probes.
namespace material_scene_cpu
{
    constexpr uint32_t Version = 1;
    enum Stage : uint32_t { Publication, Capture, Retirement, Binding, StageCount };
    struct Metrics
    {
        uint64_t stageCycles[StageCount]{};
        uint64_t stageSegments[StageCount]{};
        uint64_t materialCycles = 0, materialSegments = 0;
        uint64_t visitedSections = 0, sampledSections = 0;
        uint64_t texturePaths = 0, errors = 0;
        uint64_t bindCalls = 0, unbindCalls = 0, successfulBinds = 0;
        uint64_t overheadMedian = 0, overheadMax = 0;
        uint32_t stride = 0, thread = 0;
    };
    struct Api
    {
        uint32_t version, bytes;
        void (*configure)(uint32_t, uint32_t, bool);
        void (*begin)(uint32_t);
        void (*end)(uint32_t);
        void (*read)(Metrics *);
        void (*calibrate)();
    };
    using GetApi = const Api *(*)();
}

#ifdef WORKPHONE_MATERIAL_SCENE_PROBE_IMPLEMENTATION
#include <algorithm>
#include <array>
namespace material_scene_cpu
{
    class MaterialScope;
    struct Context
    {
        Metrics metrics;
        MaterialScope *section = nullptr;
        uint64_t started[StageCount]{};
        bool active[StageCount]{};
        bool enabled = false;
        uint32_t random = 1;
        uint64_t overheadMedian = 0, overheadMax = 0;
    };
    inline thread_local Context context;
    inline uint64_t clock()
    {
        using Query = BOOL (WINAPI *)(HANDLE, PULONG64);
        static const auto query = reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "QueryThreadCycleTime"));
        ULONG64 value = 0;
        if(!query || !query(GetCurrentThread(), &value)) ++context.metrics.errors;
        return value;
    }
    inline void calibrate()
    {
        for(unsigned i=0;i<64;++i) (void)clock();
        std::array<uint64_t,1024> samples{};
        for(auto &value:samples)
        {
            const auto start=clock(), end=clock();
            if(end<start) ++context.metrics.errors;
            value=end>=start?end-start:0;
        }
        std::sort(samples.begin(),samples.end());
        context.overheadMedian=samples[samples.size()/2]; context.overheadMax=samples.back();
    }
    inline void configure(uint32_t stride,uint32_t seed,bool enabled)
    {
        const auto errors=context.metrics.errors;
        context.metrics={};
        context.metrics.errors=errors;
        context.metrics.stride=stride; context.metrics.thread=GetCurrentThreadId();
        context.metrics.overheadMedian=context.overheadMedian; context.metrics.overheadMax=context.overheadMax;
        context.enabled=enabled; context.random=seed?seed:1;
        for(auto &active:context.active) active=false;
        if(!stride) ++context.metrics.errors;
    }
    inline void begin(uint32_t stage)
    {
        if(!context.enabled) return;
        if(stage>=StageCount || context.active[stage]) { ++context.metrics.errors; return; }
        context.active[stage]=true; context.started[stage]=clock();
    }
    inline void end(uint32_t stage)
    {
        if(!context.enabled) return;
        if(stage>=StageCount || !context.active[stage]) { ++context.metrics.errors; return; }
        const auto now=clock();
        if(now<context.started[stage]) ++context.metrics.errors;
        else context.metrics.stageCycles[stage]+=now-context.started[stage];
        ++context.metrics.stageSegments[stage]; context.active[stage]=false;
    }
    inline void read(Metrics *out)
    {
        for(auto active:context.active) if(active) ++context.metrics.errors;
        if(out) *out=context.metrics;
    }
    inline void texturePath() { if(context.enabled) ++context.metrics.texturePaths; }
    class MaterialScope
    {
    public:
        MaterialScope()
        {
            if(!context.enabled) return;
            previous=context.section; context.section=this;
            ++context.metrics.visitedSections;
            auto &random=context.random; random^=random<<13; random^=random>>17; random^=random<<5;
            sampled=context.metrics.stride && random%context.metrics.stride==0;
            if(sampled) { ++context.metrics.sampledSections; resume(); }
        }
        ~MaterialScope() { stop(); if(context.enabled) context.section=previous; }
        bool isSampled() const { return sampled; }
        bool pause() { const auto was=active; stop(); return was; }
        void stop()
        {
            if(!active) return;
            const auto now=clock();
            if(now<started) ++context.metrics.errors;
            else context.metrics.materialCycles+=now-started;
            ++context.metrics.materialSegments; active=false;
        }
        void resume() { if(sampled && !active) { started=clock(); active=true; } }
    private:
        MaterialScope *previous=nullptr;
        uint64_t started=0;
        bool sampled=false,active=false;
    };
    class StageScope
    {
    public:
        explicit StageScope(Stage value) : stage(value) { begin(stage); }
        ~StageScope() { end(stage); }
    private: Stage stage;
    };
    class BindingScope
    {
    public:
        explicit BindingScope(bool unbind=false)
        {
            if(!context.enabled) return;
            if(unbind) ++context.metrics.unbindCalls; else ++context.metrics.bindCalls;
            section=context.section;
            if(section && section->isSampled()) { resume=section->pause(); started=clock(); sampled=true; }
        }
        ~BindingScope()
        {
            if(!sampled) return;
            const auto now=clock();
            if(now<started) ++context.metrics.errors;
            else context.metrics.stageCycles[Binding]+=now-started;
            ++context.metrics.stageSegments[Binding];
            if(resume) section->resume();
        }
    private: MaterialScope *section=nullptr; uint64_t started=0; bool sampled=false,resume=false;
    };
    inline void successfulBind() { if(context.enabled) ++context.metrics.successfulBinds; }
}
#ifndef WORKPHONE_MATERIAL_SCENE_NO_EXPORT
extern "C" __declspec(dllexport) const material_scene_cpu::Api *wp_material_scene_cpu_v1()
{
    using namespace material_scene_cpu;
    static const Api api{Version,sizeof(Api),configure,begin,end,read,calibrate};
    return &api;
}
#endif
#endif
