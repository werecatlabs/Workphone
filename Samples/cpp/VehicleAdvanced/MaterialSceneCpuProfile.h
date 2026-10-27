#pragma once
#if defined(_WIN32)
#include "MaterialSceneCpuAttributionProbe.hpp"
#include <cstdlib>
#include <ostream>
#include <stdexcept>

namespace workphone::advanced
{
    // Optional test-overlay client. Normal runs never look up the probe bridge.
    class MaterialSceneCpuProfile
    {
    public:
        void begin()
        {
            if(!std::getenv("WP_MATERIAL_SCENE_CPU")) return;
            using namespace material_scene_cpu;
            auto module=GetModuleHandleW(L"WPGraphics.dll");
            auto get=module?reinterpret_cast<GetApi>(GetProcAddress(module,"wp_material_scene_cpu_v4")):nullptr;
            api=get?get():nullptr;
            if(!api || api->version!=Version || api->bytes!=sizeof(Api))
                throw std::runtime_error("Material scene attribution requires its matching test renderer overlay");
            api->calibrate(); api->configure(32,0x9e3779b9u,true);
        }
        void end()
        {
            if(!api) return;
            api->read(&metrics);
            api->configure(32,1,false);
            if(metrics.errors || !metrics.visitedSections || !metrics.sampledSections ||
               metrics.thread!=GetCurrentThreadId())
                throw std::runtime_error("Material scene attribution has incomplete owner-thread coverage");
        }
        void beginStage(material_scene_cpu::Stage stage) { if(api) api->begin(stage); }
        void endStage(material_scene_cpu::Stage stage) { if(api) api->end(stage); }
        void write(std::ostream &out) const
        {
            out << ",\n\"materialCpu\":";
            if(!api) { out << "null"; return; }
            const auto &m=metrics;
            out << "{\"protocol\":\"material_scene_cpu_v4\",\"clock\":\"QueryThreadCycleTime\""
                << ",\"stride\":" << m.stride << ",\"thread\":" << m.thread
                << ",\"preparationSampleCycles\":" << m.materialCycles
                << ",\"preparationSampleSegments\":" << m.materialSegments
                << ",\"bindingSampleCycles\":" << m.stageCycles[material_scene_cpu::Binding]
                << ",\"bindingSampleSegments\":" << m.stageSegments[material_scene_cpu::Binding]
                << ",\"publicationCommitCycles\":" << m.stageCycles[material_scene_cpu::Publication]
                << ",\"publicationCommitSegments\":" << m.stageSegments[material_scene_cpu::Publication]
                << ",\"captureSealCycles\":" << m.stageCycles[material_scene_cpu::Capture]
                << ",\"captureSealSegments\":" << m.stageSegments[material_scene_cpu::Capture]
                << ",\"recordClearCycles\":" << m.stageCycles[material_scene_cpu::Retirement]
                << ",\"recordClearSegments\":" << m.stageSegments[material_scene_cpu::Retirement]
                << ",\"visitedSections\":" << m.visitedSections << ",\"sampledSections\":" << m.sampledSections
                << ",\"texturePaths\":" << m.texturePaths << ",\"bindCalls\":" << m.bindCalls
                << ",\"successfulBinds\":" << m.successfulBinds << ",\"unbindCalls\":" << m.unbindCalls
                << ",\"nativeUploadSampleCycles\":" << m.stageCycles[material_scene_cpu::NativeUpload]
                << ",\"nativeUploadSampleSegments\":" << m.stageSegments[material_scene_cpu::NativeUpload]
                << ",\"nativeBindingSampleCycles\":" << m.stageCycles[material_scene_cpu::NativeBinding]
                << ",\"nativeBindingSampleSegments\":" << m.stageSegments[material_scene_cpu::NativeBinding]
                << ",\"nativeDrawCalls\":" << m.nativeDrawCalls << ",\"nativeUnscopedDrawCalls\":" << m.nativeUnscopedDrawCalls
                << ",\"nativeSampledDraws\":" << m.nativeSampledDraws << ",\"nativeFailedDraws\":" << m.nativeFailedDraws
                << ",\"nativeCombinedDraws\":" << m.nativeCombinedDraws << ",\"nativeSplitDraws\":" << m.nativeSplitDraws
                << ",\"nativeUploadCalls\":" << m.nativeUploadCalls << ",\"nativeAuxUploadCalls\":" << m.nativeAuxUploadCalls
                << ",\"nativeFailedUploads\":" << m.nativeFailedUploads << ",\"nativeBindingRegions\":" << m.nativeBindingRegions
                << ",\"authoringCycles\":" << m.stageCycles[material_scene_cpu::Authoring]
                << ",\"authoringSegments\":" << m.stageSegments[material_scene_cpu::Authoring]
                << ",\"editPublicationCycles\":" << m.stageCycles[material_scene_cpu::EditPublication]
                << ",\"editPublicationSegments\":" << m.stageSegments[material_scene_cpu::EditPublication]
                << ",\"editVerificationCycles\":" << m.stageCycles[material_scene_cpu::EditVerification]
                << ",\"editVerificationSegments\":" << m.stageSegments[material_scene_cpu::EditVerification]
                << ",\"clockOverheadMedian\":" << m.overheadMedian << ",\"clockOverheadMax\":" << m.overheadMax
                << ",\"clockErrors\":" << m.errors << "}";
        }
    private:
        const material_scene_cpu::Api *api=nullptr;
        material_scene_cpu::Metrics metrics;
    };
}

#else
#include <cstdlib>
#include <ostream>
#include <stdexcept>
namespace workphone::advanced {
class MaterialSceneCpuProfile {
public:
 void begin() { if(std::getenv("WP_MATERIAL_SCENE_CPU")) throw std::runtime_error("Material scene CPU profiling requires Windows"); }
 void end() {}
 void write(std::ostream &out) const { out << ",\n\"materialCpu\":null"; }
}; }
#endif
