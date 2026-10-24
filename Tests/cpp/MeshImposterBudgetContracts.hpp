#pragma once

#include <Workphone/Mesh/MeshImposterGenerator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>

namespace mesh_imposter_budget_contracts
{
    using namespace workphone;
    inline void require( bool condition, const char *message )
    { if( !condition ) throw std::runtime_error( message ); }
    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set( u64 value )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ), "imposter limit applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };
    template<class F> inline void denied( F operation, const char *message )
    {
        bool rejected = false;
        try { operation(); } catch( const std::bad_alloc & ) { rejected = true; }
        require( rejected, message );
    }
    template<class T> inline u64 bytes( size_t capacity )
    { return capacity ? sourceBudgetStorageBytes( capacity * sizeof(T), alignof(T) ) : 0; }

    inline void rasterAdmission()
    {
        Capacity budget;
        const Array<MeshImposterTriangle> triangles = {
            { { Vector3F(-.5f,0,-.1f), Vector3F(.5f,0,-.1f), Vector3F(0,1,-.1f) }, ColourF(0,0,1,1) },
            { { Vector3F(-.5f,0,.1f), Vector3F(.5f,0,.1f), Vector3F(0,1,.1f) }, ColourF(1,0,0,1) }
        };
        const auto baseline = budget.used();
        constexpr u32 width = 17, height = 19;
        const auto image = bytes<u8>( size_t(width) * height * 3 * 4 );
        const auto depth = bytes<float>( size_t(width) * height );
        const auto bake = [&] { return generateMeshImposters( triangles, {-.6f,0,-.2f}, {.6f,1.1f,.2f}, width, height ); };
        for( const auto headroom : { image - 1, image + depth - 1 } )
        {
            budget.set( baseline + headroom );
            denied( [&] { auto candidate = bake(); }, "atlas and overlapping depth each require complete admission" );
            require( budget.used() == baseline, "denied raster stages fully retire their storage" );
        }
        budget.set( baseline + image + depth );
        auto atlas = bake();
        require( budget.used() == baseline + image && atlas.width == width * 3 && atlas.height == height &&
                 atlas.rgba.capacity() == atlas.rgba.size() && atlas.rgba.get_allocator().isSourceBudgeted(),
                 "non-power-of-two atlas uses exact retained pixel storage after depth retirement" );
        const auto resident = budget.used();
        const auto pointer = atlas.rgba.data();
        budget.set( resident + image + depth - 1 );
        denied( [&] { atlas = bake(); }, "denied rebake preserves the existing caller generation" );
        require( atlas.rgba.data() == pointer && atlas.width == width * 3 && budget.used() == resident,
                 "rebake failure preserves pixels, dimensions and exact residency" );
        budget.set( resident + image + depth );
        auto again = bake();
        require( again.rgba == atlas.rgba && budget.used() == resident + image,
                 "retry reproduces deterministic colour/depth/alpha with independent ownership" );
        budget.set( budget.used() );
        const auto denials = sourceBudgetDenialCount();
        atlas = MeshImposterAtlas{};
        again = MeshImposterAtlas{};
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "full-cap atlas retirement admits no replacement storage" );
        budget.set( baseline );
        auto invalid = triangles;
        invalid[0].positions[0].x = std::numeric_limits<float>::quiet_NaN();
        bool rejected = false;
        try { auto candidate = generateMeshImposters( invalid, {-.6f,0,-.2f}, {.6f,1.1f,.2f}, width, height ); }
        catch( const std::invalid_argument & ) { rejected = true; }
        require( rejected && budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "invalid source is rejected before output or depth admission" );
    }

    inline void importedCapacityAndBodies()
    {
        Capacity budget;
        const auto baseline = budget.used();
        const auto body = sourceBudgetStorageBytes( sizeof(MeshImposterAtlas), alignof(std::max_align_t) );
        budget.set( baseline + body - 1 );
        denied( [] { auto atlas = std::make_unique<MeshImposterAtlas>(); }, "heap atlas body requires admission" );
        require( budget.used() == baseline, "denied atlas body has no remaining lease" );
        budget.restore();
        MeshImposterAtlas external;
        external.width = external.height = external.tileWidth = 1;
        Array<u8> raw;
        raw.reserve( 37 ); raw.resize( 4, 91 );
        external.rgba = raw;
        const auto imported = bytes<u8>( raw.capacity() );
        budget.set( baseline + body + imported - 1 );
        denied( [&] { auto copy = std::make_unique<MeshImposterAtlas>( external ); },
                "body-success/pixel-denial copy unwinds both owned stages" );
        require( budget.used() == baseline, "partial atlas copy retires its wrapper body" );
        budget.set( baseline + body + imported );
        auto owner = std::make_unique<MeshImposterAtlas>( external );
        require( budget.used() == baseline + body + imported && owner->rgba.capacity() == 37 &&
                 owner->rgba.get_allocator().isSourceBudgeted(), "caller raw spare capacity canonicalizes into Source storage" );
        external.rgba[0] = 7;
        require( owner->rgba[0] == 91, "imported pixels are independent from external mutation" );
        budget.set( budget.used() + imported );
        std::optional<Array<u8>> escaped( owner->rgba );
        const auto oldPointer = owner->rgba.data();
        const auto full = budget.used();
        budget.set( full );
        denied( [&] { *owner = external; }, "copy assignment stages imported capacity before publication" );
        require( owner->rgba.data() == oldPointer && owner->rgba[0] == 91 && budget.used() == full,
                 "denied assignment preserves complete atlas metadata and storage" );
        const auto denials = sourceBudgetDenialCount();
        owner.reset();
        require( budget.used() == baseline + imported && escaped->capacity() == 37 && escaped->front() == 91,
                 "escaped pixel snapshots retain spare capacity beyond wrapper destruction" );
        escaped.reset();
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "last caller release retires exact physical atlas capacity without admission" );
    }
    inline void run() { rasterAdmission(); importedCapacityAndBodies(); }
}
