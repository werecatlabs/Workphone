#pragma once
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <iosfwd>

namespace workphone::resource
{
    struct MorphVertexDelta {u32 vertex=0;Vector3F position,normal;};
    struct MorphTarget
    {
        String id;
        Array<MorphVertexDelta> deltas=makeSourceBudgetSnapshot<MorphVertexDelta>();
    };
    struct MorphWeightKey
    {
        f32 time=0;
        Array<f32> weights=makeSourceBudgetSnapshot<f32>();
    };
    struct WPCore_API MorphClipInfo
    {
        ResourceID mesh;
        String clip,target;
        bool packagedBuild=false;
        f32 duration=0;
        u32 vertices=0,format=0;
        u64 topologyHash=0,bindHash=0;
    };
    struct WPCore_API MorphClipData
    {
        StreamBudget admission;
        MorphClipInfo info;
        Array<MorphTarget> targets=makeSourceBudgetSnapshot<MorphTarget>();
        Array<MorphWeightKey> keys=makeSourceBudgetSnapshot<MorphWeightKey>();
    };
    /** Typed sparse morph records. The graphics compiler/consumer supplies its
     * ABI version and validates the recorded native mesh signatures. */
    class WPCore_API MorphResourceCodec final
    {
    public:
        enum : size_t { maximumSourceBytes=4u<<20, maximumPayloadBytes=2u<<20 };
        static bool parseSource(const String &,MorphClipData &,String &);
        static bool write(const MorphClipData &,std::ostream &,String &);
        static bool decode(const RuntimeResource &,u64 expectedCompilerVersion,MorphClipData &,String &);
        /** Bounded clip metadata only; native publication still validates ABI. */
        static bool describe(const RuntimeResource &,MorphClipInfo &,String &);
        static bool weights(const MorphClipData &,f32 time,Array<f32> &,String &);
    };
}
