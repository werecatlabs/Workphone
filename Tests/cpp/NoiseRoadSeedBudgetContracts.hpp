#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/WPNoise.hpp>
#include <WPProcedural/WPRoadSystem.hpp>
#include <WPProcedural/CRoadSystem.hpp>
#include <Workphone/System/FactoryTemplate.hpp>
#include <memory>

namespace noise_road_seed_budget_contracts
{
    using namespace city_inventory_budget_contracts;

    template<class T> inline u64 directBytes()
    { return sourceBudgetStorageBytes( sizeof( T ), alignof( std::max_align_t ) ); }

    inline bool sameMesh( const RoadMesh &a, const RoadMesh &b )
    {
        if( a.vertices.size() != b.vertices.size() || a.indices.size() != b.indices.size() ) return false;
        for( size_t i = 0; i < a.vertices.size(); ++i )
        {
            const auto &x = a.vertices[i], &y = b.vertices[i];
            if( x.position != y.position || x.normal != y.normal || x.uv != y.uv || x.color != y.color ) return false;
        }
        for( size_t i = 0; i < a.indices.size(); ++i ) if( a.indices[i] != b.indices[i] ) return false;
        return true;
    }

    inline bool sameResult( const RoadSegmentResult &a, const RoadSegmentResult &b )
    {
        return sameMesh( a.roadSurface, b.roadSurface ) && sameMesh( a.collision, b.collision ) &&
               sameMesh( a.sidewalks[0], b.sidewalks[0] ) && sameMesh( a.sidewalks[1], b.sidewalks[1] ) &&
               sameMesh( a.kerbs[0], b.kerbs[0] ) && sameMesh( a.kerbs[1], b.kerbs[1] ) &&
               sameMesh( a.markings, b.markings ) && sameMesh( a.dressing, b.dressing ) &&
               sameMesh( a.oldTarmac, b.oldTarmac ) && sameMesh( a.potholes, b.potholes ) &&
               sameMesh( a.manholes, b.manholes ) && sameMesh( a.gullyGrates, b.gullyGrates ) &&
               sameMesh( a.arrows, b.arrows ) && sameMesh( a.pedXing, b.pedXing );
    }

    inline void fill( RoadMesh &mesh, real_Num x )
    {
        mesh.vertices.reserve( 17 ); mesh.indices.reserve( 19 );
        RoadVertex vertex{}; vertex.position = { x,0,0 }; mesh.addVertex( vertex );
        vertex.position = { x+1,0,0 }; mesh.addVertex( vertex );
        vertex.position = { x,0,1 }; mesh.addVertex( vertex ); mesh.addTriangle( 0,1,2 );
    }

    inline void fillOwnedMeshes( WPRoadSystem &system, real_Num x )
    {
        for( auto mesh : { &system.oldTarmac, &system.potholes, &system.manholes,
                           &system.gullyGrates, &system.arrows, &system.pedXing } )
            fill( *mesh, x++ );
    }

    inline bool sameOwnedMeshes( const WPRoadSystem &a, const WPRoadSystem &b )
    {
        return sameMesh( a.oldTarmac, b.oldTarmac ) && sameMesh( a.potholes, b.potholes ) &&
               sameMesh( a.manholes, b.manholes ) && sameMesh( a.gullyGrates, b.gullyGrates ) &&
               sameMesh( a.arrows, b.arrows ) && sameMesh( a.pedXing, b.pedXing );
    }

    inline u64 ownedMeshBytes( const WPRoadSystem &system )
    {
        u64 bytes = 0;
        for( auto mesh : { &system.oldTarmac, &system.potholes, &system.manholes,
                           &system.gullyGrates, &system.arrows, &system.pedXing } )
        {
            if( mesh->vertices.capacity() ) bytes += sourceBudgetStorageBytes(
                mesh->vertices.capacity() * sizeof( RoadVertex ), alignof( RoadVertex ) );
            if( mesh->indices.capacity() ) bytes += sourceBudgetStorageBytes(
                mesh->indices.capacity() * sizeof( u32 ), alignof( u32 ) );
        }
        return bytes;
    }

    inline RoadSegmentSpec segmentSpec()
    {
        RoadSegmentSpec spec;
        spec.start = { 3,0,7 }; spec.end = { 11,0,31 }; spec.seed = 27;
        spec.generateSidewalks = spec.generateKerbs = spec.generateMarkings = spec.generateDressing = false;
        return spec;
    }

    template<class T> inline void rawBodyAdmission( Budget &budget )
    {
        static_assert( std::is_base_of_v<SourceBudgetObject, T> );
        const auto baseline = budget.used(), bytes = directBytes<T>();
        budget.set( baseline + bytes - 1 );
        denied( []{ auto pointer = T::operator new( sizeof( T ) ); T::operator delete( pointer ); } );
        budget.set( baseline + bytes );
        auto release = []( void *pointer ) { T::operator delete( pointer ); };
        std::unique_ptr<void, decltype( release )> memory( T::operator new( sizeof( T ) ), release );
        check( budget.used() == baseline + bytes, "Road wrapper scalar body charges its exact physical SourceCpu allocation" );
        budget.set( budget.used() ); memory.reset();
        check( budget.used() == baseline, "Road wrapper scalar body retires at the full cap" );
        budget.set( budget.original );
    }

    inline void run()
    {
        Budget budget;
        const auto noiseBytes = directBytes<WPNoise>();
        {
            const auto baseline = budget.used();
            budget.set( baseline + noiseBytes - 1 );
            denied( []{ auto value = std::make_unique<WPNoise>( 1337 ); } );
            check( budget.used() == baseline, "Denied noise body leaves no admission lease" );
            budget.set( baseline + noiseBytes );
            auto value = std::make_unique<WPNoise>( 1337 );
            check( budget.used() == baseline + noiseBytes && value->getSeed() == 1337,
                   "Noise permutation is embedded in the exactly admitted body" );
            const auto used = budget.used(); budget.set( used );
            WPNoise embedded( *value ), assigned( 42 ); assigned = embedded;
            check( assigned.getSeed() == 1337 && assigned.noise3( .31f,.73f,1.29f ) == value->noise3( .31f,.73f,1.29f ) &&
                   budget.used() == used, "Embedded noise copies preserve their fixed permutation without a separate lease" );
            value.reset();
            check( budget.used() == baseline, "Noise body destruction retires storage at the full cap" );
            budget.set( budget.original );
        }
        {
            const auto baseline = budget.used(), body = directBytes<WPRoadSystem>();
            budget.set( baseline + body - 1 );
            denied( []{ auto value = std::make_unique<WPRoadSystem>( 1337 ); } );
            budget.set( baseline + body + noiseBytes - 1 );
            denied( []{ auto value = std::make_unique<WPRoadSystem>( 1337 ); } );
            check( budget.used() == baseline, "Road-system constructor denial unwinds its already admitted owning body" );
            budget.set( baseline + body + noiseBytes );
            auto value = std::make_unique<WPRoadSystem>( 1337 );
            check( budget.used() == baseline + body + noiseBytes,
                   "Road-system heap construction admits owning body and noise independently" );
            budget.set( budget.used() ); value.reset();
            check( budget.used() == baseline, "Road-system body and owned noise retire together at the cap" );
            budget.set( budget.original );
        }
        const auto spec = segmentSpec();
        {
            WPRoadSystem system( 1337 );
            const auto old = system.generateSegment( spec );
            check( !old.roadSurface.vertices.empty() && !old.collision.vertices.empty(), "Seed test uses real road geometry" );
            const auto baseline = budget.used();
            budget.set( baseline + noiseBytes - 1 );
            denied( [&]{ system.setSeed( 7331 ); } );
            check( budget.used() == baseline, "Failed reseeding leaves the old noise alive without a leaked replacement" );
            budget.set( baseline );
            const auto denials = sourceBudgetDenialCount(); system.setSeed( 1337 );
            check( sourceBudgetDenialCount() == denials && budget.used() == baseline,
                   "Failed reseeding retains the old seed and same-seed requests do not allocate" );
            budget.set( budget.original );
            check( sameResult( old, system.generateSegment( spec ) ), "Denied reseeding preserves repeatable geometry from the old noise" );
            budget.set( budget.used() + noiseBytes ); system.setSeed( 7331 );
            check( budget.used() == baseline, "Successful reseeding replaces rather than accumulates noise leases" );
            budget.set( budget.original );
            WPRoadSystem reference( 7331 );
            const auto changed = system.generateSegment( spec ), expected = reference.generateSegment( spec );
            check( sameResult( changed, expected ) && !sameMesh( changed.roadSurface, old.roadSurface ),
                   "Reseeding retry generates the complete deterministic new-seed road" );
        }
        {
            WPRoadSystem source( 7331 ), target( 1337 );
            fillOwnedMeshes( source, 1 ); fillOwnedMeshes( target, 100 );
            const auto old = target.generateSegment( spec ), expected = source.generateSegment( spec );
            const auto baseline = budget.used(), copies = ownedMeshBytes( source );
            const auto oldData = target.oldTarmac.vertices.data();
            budget.set( baseline + copies );
            denied( [&]{ target = source; } );
            check( budget.used() == baseline && target.oldTarmac.vertices.data() == oldData &&
                   target.oldTarmac.vertices[0].position.x == 100 && target.pedXing.vertices[0].position.x == 105,
                   "Late noise-copy denial after all six public mesh copies preserves the complete old builder" );
            budget.set( baseline ); target.setSeed( 1337 ); target = target;
            check( budget.used() == baseline, "Denied builder assignment retains old seed and self-assignment needs no admission" );
            budget.set( budget.original );
            check( sameResult( old, target.generateSegment( spec ) ), "Denied builder assignment preserves actual old-seed output" );
            target = source;
            check( sameOwnedMeshes( target, source ) && target.oldTarmac.vertices.data() != source.oldTarmac.vertices.data() &&
                   sameResult( expected, target.generateSegment( spec ) ), "Builder copy assignment retries with independent meshes and noise" );
            WPRoadSystem copy( source );
            copy.oldTarmac.vertices[0].position.x = 999; copy.setSeed( 2027 );
            check( source.oldTarmac.vertices[0].position.x == 1 && sameResult( expected, source.generateSegment( spec ) ),
                   "Deep-copied builder mesh mutation and reseeding do not affect the original" );
            const auto beforeMove = budget.used(); budget.set( beforeMove );
            WPRoadSystem moved( std::move( source ) );
            check( budget.used() == beforeMove && moved.oldTarmac.vertices[0].position.x == 1,
                   "Builder move transfers admitted meshes and noise without replacement allocation" );
            bool rejected = false;
            try { source.generateSegment( spec ); } catch( const std::logic_error & ) { rejected = true; }
            check( rejected && budget.used() == beforeMove, "Moved-from noise-dependent generation fails cleanly without leaking scratch" );
            denied( [&]{ source.setSeed( 7331 ); } );
            budget.set( budget.original ); source.setSeed( 7331 );
            check( sameResult( expected, source.generateSegment( spec ) ) && sameResult( expected, moved.generateSegment( spec ) ),
                   "Reseeding restores moved-from builders even with their previous seed" );
        }
        {
            const auto baseline = budget.used();
            auto owner = std::make_unique<WPRoadSystem>( 7331 ); fillOwnedMeshes( *owner, 5 );
            budget.set( budget.used() ); owner.reset();
            check( budget.used() == baseline, "Full-cap road-system destruction retires body, six public mesh buffers and owned noise" );
            budget.set( budget.original );
        }
        rawBodyAdmission<CRoadSystem>( budget );
        {
            FactoryTemplate<CRoadSystem> factory;
            const auto baseline = budget.used();
            const auto bytes = sourceBudgetStorageBytes( sizeof( CRoadSystem ), alignof( CRoadSystem ) );
            budget.set( baseline + bytes - 1 ); denied( [&]{ auto memory = factory.allocateMemory(); factory.freeMemory( memory ); } );
            budget.set( baseline + bytes ); auto memory = factory.allocateMemory();
            check( budget.used() == baseline + bytes, "Road wrapper factory storage admits exactly one owning body" );
            budget.set( budget.used() ); factory.freeMemory( memory );
            check( budget.used() == baseline, "Road wrapper factory storage retires through its matching Source policy at the cap" );
            budget.set( budget.original );
        }
        {
            auto concrete = make_ptr<CRoadSystem>();
            SmartPtr<IRoadSystem> system = concrete;
            system->setSeed( 1337 ); const auto old = system->generateSegment( spec );
            const auto baseline = budget.used(); budget.set( baseline );
            denied( [&]{ system->setSeed( 7331 ); } ); system->setSeed( 1337 );
            check( budget.used() == baseline, "Active road interface forwards failed reseeding without retiring the old noise" );
            budget.set( budget.original );
            check( sameResult( old, system->generateSegment( spec ) ), "Road interface denial retains the old complete geometry" );
            system->setSeed( 7331 ); WPRoadSystem reference( 7331 );
            check( sameResult( reference.generateSegment( spec ), system->generateSegment( spec ) ),
                   "Road interface reseeding retries into deterministic replacement output" );
            budget.set( budget.used() ); system = nullptr; concrete = nullptr;
            budget.set( budget.original );
        }
    }
}
