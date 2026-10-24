#pragma once

#include <Workphone/Mesh/ProgressiveMeshOptions.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Graphics/GraphicsMesh.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Directors/MeshResourceDirector.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <array>
#include <memory>
#include <stdexcept>

namespace progressive_mesh_budget_contracts
{
    using namespace workphone;

    inline void require( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }
    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 value )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ),
                     "progressive mesh capacity applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };
    template<class F> inline void denied( F operation, const char *message )
    {
        bool refused = false;
        try { operation(); } catch( const std::bad_alloc & ) { refused = true; }
        require( refused, message );
    }
    template<class T> inline u64 arrayBytes( size_t capacity )
    { return capacity ? sourceBudgetStorageBytes( capacity * sizeof(T), alignof(T) ) : 0; }
    template<class T> inline u64 bodyBytes()
    { return sourceBudgetStorageBytes( sizeof(T), alignof(std::max_align_t) ); }

    inline ProgressiveMeshLodLevel level( f32 distance, f32 remaining, const char *mesh = "" )
    {
        ProgressiveMeshLodLevel value;
        value.distance = distance;
        value.remainingGeometry = remaining;
        value.manualMesh = mesh;
        return value;
    }
    inline ProgressiveMeshOptions borrowed( size_t capacity )
    {
        ProgressiveMeshOptions value;
        value.generationMode = MeshLodGenerationMode::Custom;
        value.preset = MeshLodPreset::Custom;
        Array<ProgressiveMeshLodLevel> raw;
        raw.reserve( capacity );
        raw.push_back( level(10,0.8f,"city_lod_a.mesh") );
        raw.push_back( level(30,0.4f,"city_lod_b.mesh") );
        raw.push_back( level(90,0.1f,"city_lod_c.mesh") );
        value.levels = std::move( raw );
        return value;
    }

    inline void defaultsBodiesAndPresets()
    {
        Capacity budget;
        const auto baseline = budget.used();
        ProgressiveMeshOptions defaults;
        require( !defaults.isEnabled() && defaults.preset == MeshLodPreset::Balanced &&
                 defaults.transitionMode == MeshLodTransitionMode::Distance &&
                 defaults.simplificationMetric == MeshSimplificationMetric::QuadricError &&
                 defaults.minimumSourceTriangleCount == 500 && defaults.minimumLodTriangleCount == 64 &&
                 defaults.levels.empty() && defaults.levels.get_allocator().isSourceBudgeted() &&
                 budget.used() == baseline, "default inline options preserve settings without a heap lease" );
        budget.set( baseline + bodyBytes<ProgressiveMeshOptions>() - 1 );
        denied( [] { auto value = std::make_unique<ProgressiveMeshOptions>(); },
                "dynamic progressive options admit their actual body" );
        require( budget.used() == baseline, "denied options body leaves no charge" );
        budget.set( baseline + bodyBytes<ProgressiveMeshOptions>() );
        {
            auto value = std::make_unique<ProgressiveMeshOptions>();
            require( budget.used() == baseline + bodyBytes<ProgressiveMeshOptions>(),
                     "empty dynamic options count exactly one body" );
            budget.set( budget.used() );
        }
        require( budget.used() == baseline, "full-cap options destruction retires its body" );
        alignas(ProgressiveMeshOptions) unsigned char storage[sizeof(ProgressiveMeshOptions)];
        budget.set( baseline );
        auto placed = new( storage ) ProgressiveMeshOptions();
        placed->~ProgressiveMeshOptions();
        require( budget.used() == baseline, "caller placement storage has no duplicate body charge" );

        const std::array<MeshLodPreset,3> presets{{MeshLodPreset::Performance, MeshLodPreset::Balanced,
                                                 MeshLodPreset::Quality}};
        for( const auto preset : presets )
        {
            const size_t count = preset == MeshLodPreset::Performance ? 3 : 4;
            const auto bytes = arrayBytes<ProgressiveMeshLodLevel>(count);
            budget.set( baseline + bytes - 1 );
            denied( [&] { auto levels = ProgressiveMeshOptions::getPresetLevels(preset); },
                    "preset allocation requires every physical level slot" );
            require( budget.used() == baseline, "denied preset leaves no temporary inventory" );
            budget.set( baseline + bytes );
            {
                auto levels = ProgressiveMeshOptions::getPresetLevels(preset);
                require( levels.size() == count && levels.capacity() == count &&
                         levels.get_allocator().isSourceBudgeted() && budget.used() == baseline + bytes &&
                         levels.front().distance < levels.back().distance &&
                         levels.front().remainingGeometry > levels.back().remainingGeometry,
                         "preset snapshots preserve ordered reduction values in exact Source storage" );
                if( preset == MeshLodPreset::Performance )
                    require( levels.front().distance == 12 && levels.back().remainingGeometry == 0.15f,
                             "performance preset values remain compatible" );
                if( preset == MeshLodPreset::Balanced )
                    require( levels.front().distance == 15 && levels.back().remainingGeometry == 0.12f,
                             "balanced preset values remain compatible" );
                if( preset == MeshLodPreset::Quality )
                    require( levels.front().distance == 25 && levels.back().remainingGeometry == 0.30f,
                             "quality preset values remain compatible" );
                levels.clear();
                require( budget.used() == baseline + bytes, "preset clear retains its actual spare capacity" );
                budget.set( budget.used() );
            }
            require( budget.used() == baseline, "full-cap preset retirement frees all physical slots" );
        }
        budget.set( baseline );
        auto empty = ProgressiveMeshOptions::getPresetLevels(MeshLodPreset::Custom);
        require( empty.empty() && empty.get_allocator().isSourceBudgeted() && budget.used() == baseline,
                 "custom presets return an empty Source-policy snapshot without admission" );
    }

    inline void copiesAndResolvedSnapshots()
    {
        Capacity budget;
        const auto baseline = budget.used();
        auto retained = makeSourceBudgetSnapshot<ProgressiveMeshLodLevel>();
        {
            auto input = borrowed(13);
            input.levels[1].enabled = false;
            input.minimumSourceTriangleCount = 1900;
            input.preserveUvSeams = false;
            require( !input.levels.get_allocator().isSourceBudgeted() && budget.used() == baseline,
                     "fixture explicitly imports a raw caller inventory with spare capacity" );
            ProgressiveMeshOptions destination;
            destination.levels.reserve(5);
            destination.levels.push_back(level(20,0.6f));
            const auto old = destination;
            const auto before = budget.used();
            const auto oldData = destination.levels.data();
            const auto bytes = arrayBytes<ProgressiveMeshLodLevel>(13);
            budget.set( before + bytes - 1 );
            denied( [&] { destination = input; }, "whole-options copy admits raw spare capacity before any publication" );
            require( destination == old && destination.levels.data() == oldData &&
                     destination.levels.capacity() == 5 && budget.used() == before,
                     "denied options copy preserves scalar settings, level contents, pointer and spare slots" );
            budget.set( before + bytes );
            destination = input;
            require( destination == input && destination.levels.capacity() == 13 &&
                     destination.levels.get_allocator().isSourceBudgeted() &&
                     budget.used() == before - arrayBytes<ProgressiveMeshLodLevel>(5) + bytes,
                     "exact options copy normalizes raw levels while retaining requested spare capacity" );
            budget.restore();
            require( destination.getHash() == input.getHash() && destination.validate(),
                     "canonical storage preserves hash and enabled-level validation" );
            const auto owned = budget.used();
            budget.set( owned + bytes - 1 );
            denied( [&] { retained = input.getResolvedLevels(); },
                    "raw resolved-level snapshots require Source admission" );
            require( retained.empty() && budget.used() == owned,
                     "denied resolution preserves caller output and all owner inventories" );
            budget.set( owned + bytes );
            retained = input.getResolvedLevels();
            require( retained.size() == 2 && retained.capacity() == 13 &&
                     retained.get_allocator().isSourceBudgeted() && retained[0].distance == 10 &&
                     retained[1].distance == 90 && input.levels.size() == 3,
                     "resolved snapshots filter disabled levels stably without losing Source spare-capacity ownership" );
            budget.set( budget.used() );
        }
        require( budget.used() == baseline + arrayBytes<ProgressiveMeshLodLevel>(13) &&
                 retained[1].manualMesh == "city_lod_c.mesh",
                 "detached resolved levels survive options retirement with spare capacity charged" );
        const auto owned = budget.used();
        const auto pointer = retained.data();
        budget.set( owned + arrayBytes<ProgressiveMeshLodLevel>(29) - 1 );
        denied( [&] { retained.reserve(29); }, "caller snapshot growth admits old/new physical overlap" );
        require( retained.data() == pointer && retained.size() == 2 && retained.capacity() == 13 &&
                 budget.used() == owned, "denied caller growth preserves contents and capacity" );
        budget.set( owned + arrayBytes<ProgressiveMeshLodLevel>(29) );
        retained.reserve(29);
        require( budget.used() == baseline + arrayBytes<ProgressiveMeshLodLevel>(29),
                 "successful caller growth physically retires the old buffer" );
        budget.set( budget.used() );
        retained.clear();
        require( budget.used() == baseline + arrayBytes<ProgressiveMeshLodLevel>(29),
                 "clearing retained resolved levels keeps the actual spare storage charged" );
        retained = makeSourceBudgetSnapshot<ProgressiveMeshLodLevel>();
        require( budget.used() == baseline, "full-cap resolved snapshot retirement releases the complete capacity" );
    }

    inline void propertyTransactions()
    {
        Capacity budget;
        auto imported = borrowed(7);
        imported.generationMode = MeshLodGenerationMode::Manual;
        imported.transitionMode = MeshLodTransitionMode::ScreenArea;
        imported.simplificationMetric = MeshSimplificationMetric::Curvature;
        imported.minimumSourceTriangleCount = 1200;
        imported.minimumLodTriangleCount = 96;
        imported.preserveBorders = false;
        imported.preserveUvSeams = false;
        imported.preserveHardEdges = false;
        imported.preserveMaterialBoundaries = false;
        imported.preserveSkinning = false;
        imported.borderImportance = 2;
        imported.uvImportance = 3;
        imported.normalImportance = 4;
        imported.skinningImportance = 5;
        imported.hardEdgeAngleDegrees = 45;
        imported.useVertexNormals = false;
        imported.compressIndexBuffers = false;
        imported.optimiseHiddenInterior = true;
        imported.outsideImportance = 6;
        imported.outsideWalkAngleDegrees = 75;
        imported.generateShadowLods = false;
        imported.shadowLodOffset = 3;
        imported.recalculateBounds = false;
        imported.halfPosition = true;
        imported.halfTexCoords = true;
        imported.qTangents = false;
        imported.halfPoseData = false;
        imported.levels[0].screenArea = 0.5f;
        imported.levels[1].screenArea = 0.2f;
        imported.levels[2].screenArea = 0.05f;
        imported.levels[1].reductionMode = MeshLodReductionMode::FixedVertexReduction;
        imported.levels[1].verticesToRemove = 250;
        imported.levels[2].reductionMode = MeshLodReductionMode::ErrorThreshold;
        imported.levels[2].maximumError = 0.125f;
        auto properties = imported.toProperties("progressive");
        auto unrelated = make_ptr<Properties>();
        unrelated->setName("unrelated");
        properties->addChild(unrelated);
        ProgressiveMeshOptions expected;
        expected.fromProperties(properties);
        require( expected == imported && expected.validate() && expected.getHash() == imported.getHash(),
                 "property import preserves every option, manual path and reduction mode" );
        const auto childBytes = arrayBytes<SmartPtr<Properties>>(4);
        const auto levelBytes = arrayBytes<ProgressiveMeshLodLevel>(3);
        const std::array<u64,5> gates{{0, childBytes - 1, childBytes,
                                      childBytes + levelBytes - 1, childBytes + levelBytes}};
        size_t failures = 0, successes = 0;
        for( const auto gate : gates )
        {
            budget.restore();
            ProgressiveMeshOptions target;
            target.levels.reserve(11);
            target.levels.push_back(level(5,0.7f,"old.mesh"));
            target.minimumSourceTriangleCount = 777;
            target.halfPoseData = true;
            const auto old = target;
            const auto before = budget.used();
            const auto pointer = target.levels.data();
            budget.set( before + gate );
            const auto epoch = sourceBudgetDenialCount();
            try
            {
                target.fromProperties(properties);
                ++successes;
                require( target == expected && target.levels.get_allocator().isSourceBudgeted() &&
                         target.levels.capacity() == 3 && budget.used() == before -
                            arrayBytes<ProgressiveMeshLodLevel>(11) + levelBytes,
                         "exact property admission publishes the complete staged options and releases old spare slots" );
            }
            catch( const std::bad_alloc & )
            {
                ++failures;
                require( sourceBudgetDenialCount() != epoch && target == old &&
                         target.levels.data() == pointer && target.levels.capacity() == 11 &&
                         budget.used() == before,
                         "child-snapshot or level-storage denial preserves every old setting and physical inventory" );
            }
            budget.restore();
            require( target.getHash() == (target == expected ? expected.getHash() : old.getHash()),
                     "rollback or successful import preserves the corresponding complete hash" );
        }
        require( failures == 4 && successes == 1,
                 "property gates distinguish child snapshot and level storage admission without estimated headroom" );
        budget.restore();
        const auto before = budget.used();
        auto output = properties;
        budget.set( before );
        denied( [&] { output = imported.toProperties("replacement"); },
                "denied property export preserves the caller's previous document owner" );
        require( output == properties && budget.used() == before,
                 "denied property export retires all partial document allocations" );
        budget.restore();
        auto noLevels = make_ptr<Properties>();
        noLevels->setProperty("minimumSourceTriangleCount",u32(0));
        noLevels->setProperty("hardEdgeAngleDegrees",f32(900));
        expected.fromProperties(noLevels);
        require( expected.levels.empty() && expected.levels.get_allocator().isSourceBudgeted() &&
                 expected.generationMode == MeshLodGenerationMode::Manual &&
                 expected.minimumSourceTriangleCount == 1 && expected.hardEdgeAngleDegrees == 180,
                 "sparse property imports preserve prior scalars, clamp values, and replace levels with an empty Source inventory" );
        const auto sparse = expected;
        budget.set( budget.used() );
        expected.fromProperties(nullptr);
        require( expected == sparse, "null import is an allocation-free no-op" );
    }

    class GraphicsProbe : public render::GraphicsMesh
    {
    public:
        SmartPtr<render::IGraphicsObject> clone( const String & ) const override { return nullptr; }
    };

    inline void engineBoundaries()
    {
        Capacity budget;
        const auto baseline = budget.used();
        auto retained = makeSourceBudgetSnapshot<ProgressiveMeshLodLevel>();
        {
            auto input = borrowed(9);
            scene::MeshResourceDirector director;
            scene::Mesh component;
            GraphicsProbe graphics;
            const auto before = budget.used();
            budget.set( before );
            denied( [&] { director.setProgressiveMeshOptions(input); },
                    "import director setter normalizes borrowed Source policy before publication" );
            denied( [&] { component.setProgressiveMeshOptions(input); },
                    "scene mesh setter stages its caller's complete level inventory" );
            denied( [&] { graphics.setProgressiveMeshOptions(input); },
                    "graphics mesh atomic store cannot retain a borrowed raw array" );
            require( !director.getProgressiveMeshOptions().isEnabled() &&
                     !component.getProgressiveMeshOptions().isEnabled() &&
                     !graphics.getProgressiveMeshOptions().isEnabled() && budget.used() == before,
                     "denied engine imports preserve all previous options" );
            const auto bytes = arrayBytes<ProgressiveMeshLodLevel>(9);
            budget.set( before + bytes );
            director.setProgressiveMeshOptions(input);
            require( director.getProgressiveMeshOptions() == input &&
                     director.getProgressiveMeshOptions().levels.get_allocator().isSourceBudgeted() &&
                     director.getProgressiveMeshOptions().levels.capacity() == 9 &&
                     budget.used() == before + bytes,
                     "director admits the exact borrowed spare inventory" );
            budget.set( budget.used() + bytes );
            component.setProgressiveMeshOptions(input);
            require( component.getProgressiveMeshOptions() == input &&
                     component.getProgressiveMeshOptions().levels.get_allocator().isSourceBudgeted(),
                     "scene mesh retains a canonical Source-owned options value" );
            const auto beforeGraphics = budget.used();
            budget.set( beforeGraphics + 2 * bytes - 1 );
            denied( [&] { graphics.setProgressiveMeshOptions(input); },
                    "graphics atomic publication admits its by-value candidate and stored copy overlap" );
            require( !graphics.getProgressiveMeshOptions().isEnabled() && budget.used() == beforeGraphics,
                     "denial inside atomic stored-copy construction preserves the graphics value" );
            budget.set( beforeGraphics + 2 * bytes );
            graphics.setProgressiveMeshOptions(input);
            require( budget.used() == beforeGraphics + bytes,
                     "graphics publication retires its by-value candidate after committing Source storage" );
            budget.set( budget.used() + bytes );
            auto copied = graphics.getProgressiveMeshOptions();
            require( copied == input && copied.levels.get_allocator().isSourceBudgeted() &&
                     copied.levels.capacity() == 9,
                     "graphics by-value reads return canonical Source snapshots with spare capacity" );
            retained = std::move(copied.levels);
            budget.set( budget.used() );
            director.setProgressiveMeshOptions(ProgressiveMeshOptions{});
            component.setProgressiveMeshOptions(ProgressiveMeshOptions{});
            graphics.setProgressiveMeshOptions(ProgressiveMeshOptions{});
            require( budget.used() == before + bytes && retained[0].distance == 10,
                     "all engine boundaries can retire their options at a full cap while caller snapshots remain valid" );
            budget.set( budget.used() );
        }
        require( budget.used() == baseline + arrayBytes<ProgressiveMeshLodLevel>(9),
                 "only the escaped graphics snapshot remains charged after owner teardown" );
        budget.set( budget.used() );
        retained = makeSourceBudgetSnapshot<ProgressiveMeshLodLevel>();
        require( budget.used() == baseline, "last caller snapshot frees the actual physical spare capacity" );
    }

    inline void run()
    {
        defaultsBodiesAndPresets();
        copiesAndResolvedSnapshots();
        propertyTransactions();
        engineBoundaries();
    }
}
