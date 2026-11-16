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
    inline void spriteSheetAdmission()
    {
        Capacity budget;
        const Array<MeshImposterTriangle> triangles = {
            { { Vector3F( -.5f, 0, -.5f ), Vector3F( .5f, 0, -.5f ), Vector3F( 0, 1, 0 ) }, ColourF::White },
            { { Vector3F( .5f, 0, -.5f ), Vector3F( 0, 0, .5f ), Vector3F( 0, 1, 0 ) }, ColourF::White },
            { { Vector3F( 0, 0, .5f ), Vector3F( -.5f, 0, -.5f ), Vector3F( 0, 1, 0 ) }, ColourF::White }
        };
        MeshImposterSpriteSheetOptions options;
        options.tileWidth = 17; options.tileHeight = 19;
        options.azimuthViews = 8; options.elevationRings = 1; options.columns = 3;
        options.gutter = 2; options.supersampling = 1;
        constexpr size_t frames = 9, pixels = 17 * 19;
        const auto image = bytes<u8>( pixels * frames * 4 );
        const auto views = bytes<MeshImposterView>( frames );
        const auto depth = bytes<float>( pixels );
        const auto scratch = bytes<u8>( pixels * 4 );
        const auto retained = image + views, peak = retained + depth + scratch;
        const auto baseline = budget.used();
        const auto bake = [&] {
            return bakeMeshImposterSpriteSheet( triangles, { -.6f, -.1f, -.6f }, { .6f, 1.1f, .6f }, options );
        };
        for( const auto headroom : { retained - 1, retained + depth - 1, peak - 1 } )
        {
            budget.set( baseline + headroom );
            denied( [&] { auto candidate = bake(); },
                    "directional atlas, view metadata and overlapping raster scratch require complete admission" );
            require( budget.used() == baseline, "denied directional bake retires every admitted stage" );
        }
        budget.set( baseline + peak );
        auto sheet = bake();
        require( budget.used() == baseline + retained && sheet.width == 51 && sheet.height == 57 &&
                 sheet.views.size() == frames && sheet.views.capacity() == frames &&
                 sheet.rgba.size() == pixels * frames * 4 && sheet.rgba.capacity() == sheet.rgba.size() &&
                 sheet.rgba.get_allocator().isSourceBudgeted() && sheet.views.get_allocator().isSourceBudgeted(),
                 "directional output retains exact admitted pixel and view storage after scratch retirement" );
        const auto originalPixels = sheet.rgba.data();
        const auto originalViews = sheet.views.data();
        const auto resident = budget.used();
        budget.set( resident + peak - 1 );
        denied( [&] { sheet = bake(); }, "full-cap directional rebake preserves the caller's prior generation" );
        require( budget.used() == resident && sheet.rgba.data() == originalPixels &&
                 sheet.views.data() == originalViews && sheet.width == 51 && sheet.views.size() == frames,
                 "failed rebake cannot publish pixels separately from matching directional metadata" );
        budget.set( resident + peak );
        auto again = bake();
        require( again.rgba == sheet.rgba && budget.used() == resident + retained,
                 "directional retry reproduces pixels with independent retained allocations" );
        budget.set( budget.used() );
        const auto denials = sourceBudgetDenialCount();
        sheet = MeshImposterSpriteSheet{};
        again = MeshImposterSpriteSheet{};
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "last directional output release retires both arrays without allocation" );

        budget.set( baseline );
        auto invalid = triangles;
        invalid[0].colour.r = std::numeric_limits<float>::quiet_NaN();
        bool rejected = false;
        try { auto candidate = bakeMeshImposterSpriteSheet( invalid, { -.6f, -.1f, -.6f }, { .6f, 1.1f, .6f }, options ); }
        catch( const std::invalid_argument & ) { rejected = true; }
        require( rejected && budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "non-finite directional source colour fails before any source admission" );
        budget.set( baseline + peak );
        options.maximumRasterSamples = 1;
        rejected = false;
        try { auto candidate = bake(); }
        catch( const std::runtime_error & ) { rejected = true; }
        require( rejected && budget.used() == baseline,
                 "bounded directional raster failure retires pixels, metadata and scratch" );
    }

    inline void spriteSheetCopyAdmission()
    {
        Capacity budget;
        const auto baseline = budget.used();
        const auto body = sourceBudgetStorageBytes( sizeof( MeshImposterSpriteSheet ), alignof( std::max_align_t ) );
        MeshImposterSpriteSheet external;
        external.width = 11; external.height = 13; external.columns = 3; external.rows = 2;
        external.tileWidth = 7; external.tileHeight = 9; external.gutter = 2;
        Array<u8> rawPixels; rawPixels.reserve( 37 ); rawPixels.resize( 4, 91 );
        Array<MeshImposterView> rawViews; rawViews.reserve( 7 ); rawViews.resize( 1 );
        rawViews[0].towardCamera = { 0, 0, 1 };
        external.rgba = rawPixels; external.views = rawViews;
        const auto pixels = bytes<u8>( rawPixels.capacity() );
        const auto views = bytes<MeshImposterView>( rawViews.capacity() );
        budget.set( baseline + body + pixels + views - 1 );
        denied( [&] { auto candidate = std::make_unique<MeshImposterSpriteSheet>( external ); },
                "sprite copy must admit both external capacities before publication" );
        require( budget.used() == baseline, "partial sprite copy unwinds its body and first admitted array" );
        budget.set( baseline + body + pixels + views );
        auto owner = std::make_unique<MeshImposterSpriteSheet>( external );
        require( budget.used() == baseline + body + pixels + views && owner->rgba.capacity() == 37 &&
                 owner->views.capacity() == 7 && owner->rgba.get_allocator().isSourceBudgeted() &&
                 owner->views.get_allocator().isSourceBudgeted(),
                 "imported sprite pixel and view capacities canonicalize into independent Source storage" );
        external.rgba[0] = 7; external.views[0].towardCamera = { 1, 0, 0 }; external.columns = 9;
        const auto originalPixels = owner->rgba.data();
        const auto originalViews = owner->views.data();
        const auto resident = budget.used();
        budget.set( resident + pixels + views - 1 );
        denied( [&] { *owner = external; }, "sprite assignment must stage matching metadata and pixels" );
        require( budget.used() == resident && owner->rgba.data() == originalPixels &&
                 owner->views.data() == originalViews && owner->rgba[0] == 91 &&
                 owner->views[0].towardCamera == Vector3F( 0, 0, 1 ) && owner->columns == 3,
                 "denied sprite assignment preserves dimensions, basis and pixels atomically" );
        budget.set( resident );
        const auto denials = sourceBudgetDenialCount();
        owner.reset();
        require( budget.used() == baseline && sourceBudgetDenialCount() == denials,
                 "sprite body and both retained capacities retire at a full budget without replacement admission" );
    }

    inline void run()
    {
        rasterAdmission(); importedCapacityAndBodies(); spriteSheetAdmission(); spriteSheetCopyAdmission();
    }
}
