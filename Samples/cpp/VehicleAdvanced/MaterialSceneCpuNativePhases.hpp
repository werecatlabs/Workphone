#pragma once
#include <cstdint>
namespace material_scene_cpu
{
    constexpr uint32_t NativePhaseVersion=1, NativePhaseCount=6;
    enum NativePhase : uint32_t { Eligibility, FixedState, Transfer, TerrainShadow, MaterialShader, TextureSampler };
    inline constexpr const char *NativePhaseNames[NativePhaseCount]={
        "eligibility","fixed_state","transfer_metadata","terrain_shadow_bindings","material_shader_bindings","texture_sampler_bindings"};
    struct NativePhaseMetrics
    {
        uint64_t cycles[NativePhaseCount]{}, segments[NativePhaseCount]{}, regions[NativePhaseCount]{};
        uint64_t errors=0, draws=0, sampledDraws=0;
        uint32_t thread=0, stride=0;
    };
    struct NativePhaseApi
    {
        uint32_t version, bytes;
        void (*enable)(bool);
        void (*read)(NativePhaseMetrics *);
    };
    using GetNativePhaseApi=const NativePhaseApi *(*)();
}
