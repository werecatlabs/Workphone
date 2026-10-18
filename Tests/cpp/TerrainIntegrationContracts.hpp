#pragma once
#include <Workphone/Scene/Components/Terrain/TerrainIntegration.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>

namespace terrain_integration_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using namespace workphone::scene;
    inline void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }
    inline bool closeEnough( double a, double b )
    {
        return std::abs( a - b ) < 1.e-4;
    }
    inline render::TerrainData ramp()
    {
        render::TerrainData data;
        data.dimensions = { 3, 5 };
        data.spacing = { 1, 1 };
        data.origin = { 0, 0 };
        data.heights.resize( 15 );
        data.revision = 10;
        for( int z = 0; z < 5; ++z )
            for( int x = 0; x < 3; ++x )
                data.heights[z * 3 + x] = float( x + 2 * z );
        return data;
    }
    inline void population()
    {
        TerrainGroundPopulation population;
        String error;
        TerrainGroundSeed a;
        a.id = 11;
        a.localPosition = { .25f, .25f };
        a.heightOffset = .5f;
        TerrainGroundSeed b;
        b.id = 22;
        b.localPosition = { 1.75f, 3.75f };
        TerrainGroundSeed outside;
        outside.id = 33;
        outside.localPosition = { 9, 9 };
        require( population.setSeeds( { a, b, outside }, error ),
                 "valid distinct ground seeds must be accepted" );
        auto source = std::make_shared<render::TerrainData>( ramp() );
        Transform3<real_Num> placement;
        placement.setPosition( { 10, 3, -5 } );
        placement.setScale( { 2, 3, 4 } );
        placement.setOrientation( Quaternion<real_Num>( std::sqrt( .5 ), 0, std::sqrt( .5 ), 0 ) );
        require( population.update( source, placement, error ),
                 "initial terrain population must sample transformed terrain" );
        const auto first = population.getPlacements();
        require( first.size() == 3 && population.getLastResampledCount() == 3 && first[0].supported &&
                     first[1].supported && !first[2].supported,
                 "ground population must distinguish supported/outside instances" );
        require( closeEnough( first[0].worldTransform.getPosition().y, 3 + ( .75 + .5 ) * 3 ) &&
                     closeEnough( first[0].normal.length(), 1 ),
                 "ground offsets and normals must use scaled world terrain semantics" );
        require( population.update( source, placement, error ) &&
                     population.getLastResampledCount() == 0 && population.getLastDirtyCells().maxX < 0,
                 "unchanged population update must leave all instances untouched" );
        auto edited = std::make_shared<render::TerrainData>( *source );
        edited->revision = 11;
        edited->heights[0] = 4;
        require(
            population.update( edited, placement, error ) && population.getLastResampledCount() == 1,
            "a corner edit must resample only placements in the touching cell" );
        require( population.getPlacements()[0].sampledRevision == 11 &&
                     population.getPlacements()[1].sampledRevision == 10 &&
                     population.getPlacements()[1].worldTransform == first[1].worldTransform,
                 "unaffected placement identity/transform must survive a dirty terrain update" );
        auto holed = std::make_shared<render::TerrainData>( *edited );
        holed->revision = 12;
        holed->holes.assign( 8, 0 );
        holed->holes[7] = 1;
        require( population.update( holed, placement, error ) &&
                     population.getLastResampledCount() == 1 && !population.getPlacements()[1].supported,
                 "excavating a cell must invalidate only grounded instances over that hole" );
        auto painted = std::make_shared<render::TerrainData>( *holed );
        painted->revision = 13;
        painted->layers[0].tint = { 1, 0, 0, 1 };
        require(
            population.update( painted, placement, error ) && population.getLastResampledCount() == 0,
            "layer-only changes must not disturb ground placements" );
        auto invalid = std::make_shared<render::TerrainData>( *painted );
        invalid->heights[0] = std::numeric_limits<float>::quiet_NaN();
        require( !population.update( invalid, placement, error ) &&
                     population.getTerrainSnapshot() == painted,
                 "invalid terrain candidate must retain previous complete ground population" );
        require( !population.setSeeds( { a, a }, error ) && population.getPlacements().size() == 3,
                 "duplicate seed IDs must reject without destroying previous placements" );
    }
    inline void masksAndLayerFilters()
    {
        String error;
        Transform3<real_Num> identity;
        auto source = std::make_shared<render::TerrainData>( ramp() );
        TerrainGroundSeed filtered, unfiltered, distant;
        filtered.id = 101;
        filtered.localPosition = { .25f, .25f };
        filtered.allowedLayers = 2;
        filtered.minimumLayerWeight = .4f;
        unfiltered = filtered;
        unfiltered.id = 102;
        unfiltered.allowedLayers = 15;
        unfiltered.minimumLayerWeight = 0;
        distant = filtered;
        distant.id = 103;
        distant.localPosition = { 1.75f, 3.75f };
        TerrainGroundPopulation population;
        require( population.setSeeds( { filtered, unfiltered, distant }, error ) &&
                     population.update( source, identity, error ) &&
                     !population.getPlacements()[0].supported &&
                     population.getPlacements()[1].supported,
                 "layer filters must reject missing paint while unfiltered seeds remain grounded" );
        auto painted = std::make_shared<render::TerrainData>( *source );
        painted->revision = 11;
        painted->layerWeights.assign( 15, Vector4F( 1, 0, 0, 0 ) );
        painted->layerWeights[0] = { 0, 1, 0, 0 };
        require( population.update( painted, identity, error ) &&
                     population.getLastResampledCount() == 1 &&
                     population.getPlacements()[0].supported &&
                     population.getPlacements()[1].sampledRevision == source->revision &&
                     population.getPlacements()[2].sampledRevision == source->revision,
                 "weight-only edits resample touching filtered seeds without disturbing other placements" );

        TerrainGroundMasks masks;
        masks.dimensions = source->dimensions;
        masks.exclusions.assign( 8, 0 );
        masks.exclusions[0] = 1;
        require( population.setMasks( masks, error ) && population.update( painted, identity, error ) &&
                     population.getLastResampledCount() == 2 &&
                     !population.getPlacements()[0].supported && !population.getPlacements()[1].supported,
                 "cell exclusions must invalidate exactly the instances over that cell" );
        auto invalidMasks = masks;
        invalidMasks.exclusions[0] = 2;
        require( !population.setMasks( invalidMasks, error ) &&
                     population.update( painted, identity, error ) &&
                     population.getLastResampledCount() == 0 && !population.getPlacements()[1].supported,
                 "invalid mask edit must retain previously sampled exclusions" );
        masks.exclusions.clear();
        masks.density.assign( 15, 1 );
        require( population.setMasks( masks, error ) && population.update( painted, identity, error ) &&
                     population.getPlacements()[0].supported && population.getPlacements()[1].supported,
                 "removing an exclusion restores eligible grounded instances" );
        masks.density[0] = 0;
        require( population.setMasks( masks, error ) && population.update( painted, identity, error ) &&
                     population.getLastResampledCount() == 2,
                 "sample-density edits must dirty only touching cells" );
        invalidMasks = masks;
        invalidMasks.density[0] = std::numeric_limits<float>::quiet_NaN();
        require( !population.setMasks( invalidMasks, error ) &&
                     population.update( painted, identity, error ) && population.getLastResampledCount() == 0,
                 "nonfinite density must reject without replacing the previous mask" );

        filtered.allowedLayers = 1;
        filtered.minimumLayerWeight = .99f;
        auto zeroWeights = std::make_shared<render::TerrainData>( *source );
        zeroWeights->layerWeights.assign( 15, Vector4F( 0, 0, 0, 0 ) );
        require( population.setSeeds( { filtered }, error ) && population.setMasks( {}, error ) &&
                     population.update( zeroWeights, identity, error ) &&
                     population.getPlacements()[0].supported,
                 "all-zero control samples must use the same layer-zero fallback as rendering" );

        Array<TerrainGroundSeed> seeds;
        for( u64 id = 1; id <= 64; ++id )
        {
            TerrainGroundSeed seed;
            seed.id = id;
            seed.localPosition = { .5f, .5f };
            seeds.push_back( seed );
        }
        masks.density.assign( 15, .5f );
        require( population.setSeeds( seeds, error ) && population.setMasks( masks, error ) &&
                     population.update( source, identity, error ),
                 "fractional density accepts a deterministic subset of stable seed identities" );
        std::map<u64, bool> accepted;
        size_t supported = 0;
        for( const auto &placement : population.getPlacements() )
        {
            accepted.emplace( placement.id, placement.supported );
            supported += placement.supported ? 1 : 0;
        }
        require( supported > 0 && supported < seeds.size(),
                 "fractional density must produce both accepted and rejected instances" );
        std::reverse( seeds.begin(), seeds.end() );
        require( population.setSeeds( seeds, error ) && population.update( source, identity, error ),
                 "reordered seed list must remain valid" );
        for( const auto &placement : population.getPlacements() )
            require( accepted.at( placement.id ) == placement.supported,
                     "density acceptance must depend on identity rather than seed ordering" );
        masks.density.assign( 15, 0 );
        require( population.setMasks( masks, error ) && population.update( source, identity, error ) &&
                     std::none_of( population.getPlacements().begin(), population.getPlacements().end(),
                                   []( const auto &placement ) { return placement.supported; } ),
                 "zero density must exclude every seed" );
        masks.density.assign( 15, 1 );
        require( population.setMasks( masks, error ) && population.update( source, identity, error ) &&
                     std::all_of( population.getPlacements().begin(), population.getPlacements().end(),
                                  []( const auto &placement ) { return placement.supported; } ),
                 "unit density must accept every otherwise eligible seed" );
    }

    inline void retainedBinding()
    {
        String error;
        auto source = make_ptr<TerrainSystem>();
        auto data = ramp();
        require( source->applyTerrainData( data, error ), "retained integration source initializes" );
        class RecursiveEditListener final : public IEventListener
        {
        public:
            TerrainSystem *source = nullptr;
            u32 attempts = 0;
            bool rejected = true;
            Parameter handleEvent( EventType, hash_type value, const Array<Parameter> &,
                                   SmartPtr<ISharedObject>, SmartPtr<ISharedObject>,
                                   SmartPtr<IEvent> ) override
            {
                if( value == StringUtil::getHash( "terrainPublished" ) )
                {
                    ++attempts;
                    auto recursive = *source->getTerrainSnapshot();
                    recursive.heights[0] += 100;
                    String failure;
                    rejected = rejected && !source->applyTerrainData( recursive, failure ) && !failure.empty();
                }
                return Parameter();
            }
        };
        auto recursive = make_ptr<RecursiveEditListener>();
        recursive->source = source.get();
        source->addObjectListener( recursive );
        TerrainIntegrationOptions options;
        options.ground = options.navigation = options.shoreline = options.path = true;
        options.waterLevel = 4;
        options.maximumNavigationTriangles = 16;
        TerrainGroundSeed seed;
        seed.id = 7;
        seed.localPosition = { .25f, .25f };
        options.seeds = { seed };
        options.pathAnchors = { { .25f, 100, .25f } };
        auto binding = make_ptr<TerrainIntegrationBinding>();
        require( binding->bind( source, options, error ), "integration binding subscribes to source publication" );
        auto first = binding->getSnapshot();
        require( first && first->source == source->getActiveTerrainSnapshot() &&
                     first->ground.getPlacements()[0].supported && first->navigation.indices.size() == 48 &&
                     !first->shoreline.segments.empty() && first->path.points[0].supported,
                 "initial retained bundle includes grounded instances, exact navigation, shoreline and path" );
        data.heights[0] = 4;
        require( source->applyTerrainData( data, error ), "headless active terrain edit commits" );
        require( recursive->attempts == 1 && recursive->rejected && source->getTerrainSnapshot()->heights[0] == 4,
                 "dependent callbacks must reject recursive source edits without deadlock or extra publication" );
        source->removeObjectListener( recursive );
        auto edited = binding->getSnapshot();
        require( edited && edited != first && edited->source == source->getActiveTerrainSnapshot() &&
                     edited->ground.getLastResampledCount() == 1 &&
                     edited->path.points[0].position.y > first->path.points[0].position.y &&
                     first->source->heights[0] == 0,
                 "committed source edits automatically refresh all enabled inputs while retained bundles stay immutable" );
        data.dimensions = { 3, 6 };
        data.heights.resize( 18, 10 );
        require( source->applyTerrainData( data, error ) && binding->getSnapshot() == edited &&
                     !binding->getLastError().empty(),
                 "dependent navigation budget failure retains the complete previous integration bundle" );
        options.maximumNavigationTriangles = 64;
        require( binding->configure( options, error ) && binding->getSnapshot()->source == source->getActiveTerrainSnapshot() &&
                     binding->getSnapshot()->navigation.indices.size() == 60 && binding->getLastError().empty(),
                 "raising a dependent budget rebuilds the latest committed source without a new terrain edit" );
        const auto beforeInvalid = binding->getSnapshot();
        auto invalid = options;
        invalid.waterLevel = std::numeric_limits<float>::quiet_NaN();
        require( !binding->configure( invalid, error ) && binding->getSnapshot() == beforeInvalid,
                 "invalid dependent configuration keeps the previous complete bundle" );

        auto runtime = make_ptr<render::Terrain>();
        runtime->setDeferredTerrainPublication( true );
        source->setTerrain( runtime );
        require( !binding->getSnapshot(),
                 "attaching an unready runtime clears integration until the component source becomes active" );
        auto newlyBound = make_ptr<TerrainIntegrationBinding>();
        require( newlyBound->bind( source, options, error ) && !newlyBound->getSnapshot(),
                 "binding during initial deferred publication must not expose an unrelated runtime default" );
        newlyBound->unbind();
        require( runtime->publishPendingTerrain( source->getTerrainSnapshot(), {}, error ),
                 "first deferred runtime generation publishes" );
        const auto active = binding->getSnapshot();
        require( active && active->source == runtime->getActiveTerrainSnapshot(),
                 "completed runtime publication refreshes retained integration" );
        data.heights[0] = 8;
        require( source->applyTerrainData( data, error ) && binding->getSnapshot() == active,
                 "pending GPU/source generation does not move dependent ground or navigation inputs" );
        require( runtime->publishPendingTerrain( source->getTerrainSnapshot(), {}, error ) &&
                     binding->getSnapshot()->source == runtime->getActiveTerrainSnapshot() &&
                     binding->getSnapshot() != active,
                 "deferred commit advances dependent integration exactly with active terrain" );
        auto replacement = make_ptr<render::Terrain>();
        replacement->setDeferredTerrainPublication( true );
        source->setTerrain( replacement );
        require( !binding->getSnapshot() &&
                     replacement->publishPendingTerrain( source->getTerrainSnapshot(), {}, error ) &&
                     binding->getSnapshot()->source == replacement->getActiveTerrainSnapshot(),
                 "runtime replacement clears obsolete inputs and republishes only the new active runtime" );
        auto retained = binding->getSnapshot();
        binding->unbind();
        data.heights[0] = 9;
        require( !binding->getSnapshot() && !binding->configure( options, error ) &&
                     source->applyTerrainData( data, error ) &&
                     replacement->publishPendingTerrain( source->getTerrainSnapshot(), {}, error ) &&
                     !binding->getSnapshot() && retained->source->heights[0] == 8,
                 "unbind ends notifications while previously returned immutable bundles remain usable" );
        require( binding->bind( source, options, error ), "detached binding can subscribe again" );
        retained = binding->getSnapshot();
        source->unload( nullptr );
        require( !binding->getSnapshot() && !binding->configure( options, error ) &&
                     retained && retained->source->heights[0] == 9,
                 "owner unload detaches integration without invalidating caller-retained inputs" );
    }

    inline void bindingPublicationOrdering()
    {
        // Inject a real source commit at the subscription boundary; no scheduler
        // timing or private binding test hook is needed to reproduce a lost event.
        class SubscribeSource final : public TerrainSystem
        {
        public:
            std::function<void()> beforeSubscribe;
            void addObjectListener( SmartPtr<IEventListener> listener ) override
            {
                auto callback = std::move( beforeSubscribe );
                beforeSubscribe = {};
                if( callback ) callback();
                TerrainSystem::addObjectListener( listener );
            }
        };
        String error;
        auto subscribing = make_ptr<SubscribeSource>();
        auto data = ramp();
        require( subscribing->applyTerrainData( data, error ), "subscription race fixture initializes" );
        bool committedAtSubscribe = false;
        subscribing->beforeSubscribe = [&] {
            auto edit = data;
            edit.heights[0] = 31;
            String failure;
            committedAtSubscribe = subscribing->applyTerrainData( edit, failure );
        };
        TerrainIntegrationOptions options;
        options.navigation = true;
        auto binding = make_ptr<TerrainIntegrationBinding>();
        require( binding->bind( subscribing, options, error ) && committedAtSubscribe &&
                     binding->getSnapshot() &&
                     binding->getSnapshot()->source == subscribing->getActiveTerrainSnapshot() &&
                     binding->getSnapshot()->source->heights[0] == 31,
                 "binding must reconcile a source commit between initial preparation and listener subscription" );
        binding->unbind();

        class PublicationProbe final : public IEventListener
        {
        public:
            std::atomic<bool> blockOnce{ false };
            std::promise<void> entered;
            std::shared_future<void> release;
            std::mutex mutex;
            SmartPtr<ISharedObject> lastMessage;
            Parameter handleEvent( EventType, hash_type value, const Array<Parameter> &,
                                   SmartPtr<ISharedObject>, SmartPtr<ISharedObject> object,
                                   SmartPtr<IEvent> ) override
            {
                if( value == StringUtil::getHash( "terrainPublished" ) )
                {
                    {
                        std::lock_guard<std::mutex> lock( mutex );
                        lastMessage = object;
                    }
                    if( blockOnce.exchange( false ) )
                    {
                        entered.set_value();
                        release.wait();
                    }
                }
                return Parameter();
            }
            SmartPtr<ISharedObject> captured()
            {
                std::lock_guard<std::mutex> lock( mutex );
                return lastMessage;
            }
        };
        auto source = make_ptr<TerrainSystem>();
        require( source->applyTerrainData( data, error ), "event ordering source initializes" );
        auto probe = make_ptr<PublicationProbe>();
        source->addObjectListener( probe );
        require( binding->bind( source, options, error ), "event ordering binding initializes" );
        SmartPtr<IEventListener> installedListener;
        for( auto listener : source->getObjectListeners() )
            if( listener != probe ) installedListener = listener;
        require( installedListener != nullptr, "binding installed its concrete publication listener" );
        std::promise<void> release;
        probe->release = release.get_future().share();
        auto entered = probe->entered.get_future();
        probe->blockOnce = true;
        auto older = std::async( std::launch::async, [&] {
            auto edit = data;
            edit.heights[0] = 41;
            String failure;
            return source->applyTerrainData( edit, failure );
        } );
        const bool reached = entered.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready;
        SmartPtr<ISharedObject> olderMessage;
        std::future<bool> newer;
        bool newerFinished = false;
        if( reached )
        {
            olderMessage = probe->captured();
            newer = std::async( std::launch::async, [&] {
                auto edit = data;
                edit.heights[0] = 42;
                String failure;
                return source->applyTerrainData( edit, failure );
            } );
            newerFinished = newer.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready;
        }
        // Always release the first dispatch before asserting or joining futures.
        release.set_value();
        const bool olderAccepted = older.get();
        const bool newerAccepted = newer.valid() && newer.get();
        require( reached && newerFinished && olderAccepted && newerAccepted &&
                     binding->getSnapshot()->source == source->getActiveTerrainSnapshot() &&
                     binding->getSnapshot()->source->heights[0] == 42,
                 "an older cross-thread event finishing last must never roll back a newer source publication" );
        installedListener->handleEvent( EventType::Scene, StringUtil::getHash( "terrainPublished" ),
                                        {}, nullptr, olderMessage, nullptr );
        require( binding->getSnapshot()->source->heights[0] == 42,
                 "replayed older publication must be idempotently ignored" );

        auto actor = make_ptr<GameActor>();
        auto transform = make_ptr<Transform>();
        actor->setTransform( transform );
        source->setActor( actor );
        Transform3<real_Num> placement;
        placement.setPosition( { 2, 3, 4 } );
        transform->setWorldTransform( placement );
        source->updateTransform();
        auto oldPoseMessage = probe->captured();
        const auto sameSource = binding->getSnapshot()->source;
        placement.setPosition( { 7, 8, 9 } );
        transform->setWorldTransform( placement );
        source->updateTransform();
        installedListener->handleEvent( EventType::Scene, StringUtil::getHash( "terrainPublished" ),
                                        {}, nullptr, oldPoseMessage, nullptr );
        require( binding->getSnapshot()->source == sameSource &&
                     binding->getSnapshot()->placement == placement,
                 "older placement-only events must not overwrite a newer pose at the same terrain revision" );

        auto replacement = make_ptr<TerrainSystem>();
        auto replacementData = data;
        replacementData.heights[0] = 77;
        require( replacement->applyTerrainData( replacementData, error ) &&
                     binding->bind( replacement, options, error ),
                 "existing integration binding can rebind to another source" );
        installedListener->handleEvent( EventType::Scene, StringUtil::getHash( "terrainPublished" ),
                                        {}, nullptr, probe->captured(), nullptr );
        require( binding->getSnapshot()->source == replacement->getActiveTerrainSnapshot() &&
                     binding->getSnapshot()->source->heights[0] == 77,
                 "retired listener dispatch must not modify the rebound source" );
        binding->unbind();
        source->removeObjectListener( probe );
        source->setActor( nullptr );
    }

    inline void navigationAndPaths()
    {
        auto data = std::make_shared<render::TerrainData>( ramp() );
        data->holes.assign( 8, 0 );
        data->holes[7] = 1;
        Transform3<real_Num> placement;
        placement.setPosition( { 10, 3, -5 } );
        placement.setScale( { 2, 3, 4 } );
        placement.setOrientation(
            Quaternion<real_Num>( std::cos( .392699081698724 ), 0, std::sin( .392699081698724 ), 0 ) );
        String error;
        TerrainNavigationInput navigation;
        require( buildTerrainNavigationInput( data, placement, navigation, error ) &&
                     navigation.indices.size() == 42 && navigation.positions.size() == 15,
                 "navigation input must retain exact full-resolution triangles and remove hole cells" );
        require( std::find( navigation.indices.begin(), navigation.indices.end(), 14 ) ==
                     navigation.indices.end(),
                 "navigation must not reference the corner supported only by an excavated cell" );
        require( closeEnough( navigation.positions[0].x, 10 ) &&
                     closeEnough( navigation.positions[0].y, 3 ) &&
                     closeEnough( navigation.positions[0].z, -5 ),
                 "navigation vertices must use the same scale/yaw/translation as graphics and physics" );
        auto badPlacement = placement;
        badPlacement.setScale( { 0, 1, 1 } );
        require( !buildTerrainNavigationInput( data, badPlacement, navigation, error ) &&
                     navigation.source == data && navigation.indices.size() == 42,
                 "failed navigation candidate must retain previous exact triangle input" );
        const auto world = [&]( Vector3<real_Num> local ) {
            const auto point =
                placement.getPosition() + placement.getOrientation() * ( local * placement.getScale() );
            return Vector3F( float( point.x ), float( point.y ), float( point.z ) );
        };
        TerrainPathInput path;
        // Sample the top/right non-holed outer boundary, a hole, a normal interior anchor and an outside anchor.
        Array<Vector3F> points = { world( { 2, 0, 0 } ), world( { 1.75, 0, 3.75 } ),
                                   world( { .25, 0, .25 } ), world( { 9, 0, 9 } ) };
        require(
            sampleTerrainPathInput( data, placement, points, path, error ) && path.points.size() == 4,
            "world-space road/river anchors must produce explicit ground inputs" );
        require( path.points[0].supported && !path.points[1].supported && path.points[2].supported &&
                     !path.points[3].supported,
                 "ground paths must match boundary tolerance and respect holes/outside anchors" );
        for( size_t i : { size_t( 0 ), size_t( 2 ) } )
        {
            float height = 0;
            require( sampleTerrainHeight( *data, placement, { points[i].x, points[i].y, points[i].z },
                                          height ) &&
                         closeEnough( path.points[i].position.y, height ) &&
                         closeEnough( path.points[i].normal.length(), 1 ),
                     "path height/normal must agree with authoritative transformed terrain queries" );
        }
        points[0].x = std::numeric_limits<float>::infinity();
        require( !sampleTerrainPathInput( data, placement, points, path, error ) &&
                     path.points.size() == 4 && path.points[0].supported,
                 "invalid anchor list must leave the last valid path intact" );
    }
    inline void shorelinesAndExports()
    {
        auto data = std::make_shared<render::TerrainData>( ramp() );
        for( int z = 0; z < 5; ++z )
            for( int x = 0; x < 3; ++x )
                data->heights[z * 3 + x] = float( x - 1 );
        TerrainShorelineInput shoreline;
        String error;
        Transform3<real_Num> identity;
        require( buildTerrainShorelineInput( data, identity, 0, shoreline, error ) &&
                     shoreline.segments.size() == 4,
                 "shoreline contour on a shared grid edge must not emit duplicate segments" );
        double length = 0;
        for( const auto &segment : shoreline.segments )
        {
            require( closeEnough( segment.start.x, 1 ) && closeEnough( segment.end.x, 1 ) &&
                         closeEnough( segment.start.y, 0 ) && closeEnough( segment.end.y, 0 ),
                     "shoreline endpoints must lie on the water/terrain intersection" );
            length += ( segment.end - segment.start ).length();
        }
        require( closeEnough( length, 4 ),
                 "shoreline segments must cover the entire intersected terrain boundary" );
        require( !buildTerrainShorelineInput( data, identity, 0, shoreline, error, 2 ) &&
                     shoreline.segments.size() == 4,
                 "shoreline budget failure must preserve prior complete contour" );
        auto holed = std::make_shared<render::TerrainData>( *data );
        holed->holes.assign( 8, 0 );
        holed->holes[0] = holed->holes[1] = 1;
        require( buildTerrainShorelineInput( holed, identity, 0, shoreline, error ) &&
                     shoreline.segments.size() == 3,
                 "shoreline extraction must omit both excavated triangles and their missing contour" );
        auto flat = std::make_shared<render::TerrainData>( *data );
        flat->heights.assign( 15, 0 );
        require( buildTerrainShorelineInput( flat, identity, 0, shoreline, error ) &&
                     shoreline.segments.empty(),
                 "coplanar water/terrain has no unique shoreline contour" );
        auto terrain = make_ptr<TerrainSystem>();
        require( terrain->applyTerrainData( *data, error ), "export terrain must initialize" );
        namespace fs = std::filesystem;
        const auto folder = fs::temp_directory_path() / ( std::string( "workphone_terrain_inputs_" ) +
                                                          StringUtil::getUUID().c_str() );
        fs::create_directories( folder );
        struct Cleanup
        {
            fs::path folder;
            ~Cleanup()
            {
                std::error_code ignored;
                fs::remove_all( folder, ignored );
            }
        } cleanup{ folder };
        const auto navFile = folder / "navigation.obj", waterFile = folder / "shoreline.json";
        require(
            exportTerrainNavigationFile( terrain, navFile.generic_u8string().c_str(), error ) &&
                exportTerrainShorelineFile( terrain, waterFile.generic_u8string().c_str(), 0, error ),
            "navigation and shoreline exports must write actual output files" );
        const auto read = []( const fs::path &path ) {
            std::ifstream input( path, std::ios::binary );
            return std::string( std::istreambuf_iterator<char>( input ), {} );
        };
        const auto nav = read( navFile ), water = read( waterFile );
        require( nav.find( "\nv " ) != std::string::npos && nav.find( "\nf " ) != std::string::npos &&
                     water.find( "workphone.terrain.shoreline-input" ) != std::string::npos,
                 "terrain integration exports must contain actual geometry/segments rather than action "
                 "recipes" );
        require( !exportTerrainShorelineFile( terrain, waterFile.generic_u8string().c_str(),
                                              std::numeric_limits<float>::quiet_NaN(), error ) &&
                     read( waterFile ) == water,
                 "failed shoreline export must not truncate or replace the last valid file" );
    }
    inline void run()
    {
        population();
        masksAndLayerFilters();
        retainedBinding();
        bindingPublicationOrdering();
        navigationAndPaths();
        shorelinesAndExports();
    }
}  // namespace terrain_integration_contracts
