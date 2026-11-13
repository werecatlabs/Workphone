#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/WPBuildingGenerator.hpp>
#include <algorithm>
#include <cmath>

namespace building_payload_budget_contracts
{
    using namespace city_inventory_budget_contracts;
    inline void fill( ProceduralMesh &mesh, float x )
    {
        mesh.vertices.reserve( 13 ); mesh.indices.reserve( 17 );
        mesh.addVertex( Vector3F( x, 0, 0 ) );
        mesh.addVertex( Vector3F( x + 1, 0, 0 ) );
        mesh.addVertex( Vector3F( x, 0, 1 ) );
        mesh.addTriangle( 0, 1, 2 );
    }
    template<class T> inline u64 storage( const Array<T> &array )
    {
        return array.capacity() ? sourceBudgetStorageBytes( array.capacity() * sizeof( T ), alignof( T ) ) : 0;
    }
    inline u64 storage( const ProceduralMesh &mesh )
    {
        return storage( mesh.vertices ) + storage( mesh.indices );
    }
    inline void geometry()
    {
        const auto approximately = []( float a, float b ) { return std::abs( a - b ) < .001f; };
        const auto meshValid = []( const ProceduralMesh &mesh ) {
            check( mesh.hasGeometry() && mesh.boundsValid && mesh.indices.size() % 3 == 0,
                   "Building section must carry valid bounded triangles" );
            for( size_t i = 0; i < mesh.indices.size(); i += 3 )
            {
                const auto &a = mesh.vertices[mesh.indices[i]];
                const auto &b = mesh.vertices[mesh.indices[i + 1]];
                const auto &c = mesh.vertices[mesh.indices[i + 2]];
                const auto cross = ( b.position - a.position ).crossProduct( c.position - a.position );
                check( cross.lengthSquared() > 0 && cross.dotProduct( a.normal ) > 0 &&
                       cross.dotProduct( b.normal ) > 0 && cross.dotProduct( c.normal ) > 0,
                       "Building faces must wind consistently with outward vertex normals" );
            }
        };
        WPBuildingGenerator generator( 13 );
        BuildingSpec spec;
        spec.width = 12; spec.depth = 7; spec.floors = 5; spec.floorH = 3;
        spec.seed = 9001; spec.generateInterior = true;
        const Vector3<real_Num> origin( 30, 4, -20 );
        const auto plain = generator.generate( spec, origin );
        meshValid( plain.body ); meshValid( plain.roof );
        const auto minimum = plain.body.bounds.getMinimum(), maximum = plain.body.bounds.getMaximum();
        check( approximately( minimum.x, 24 ) && approximately( maximum.x, 36 ) &&
               approximately( minimum.y, 4 ) && approximately( maximum.y, 19 ) &&
               approximately( minimum.z, -23.5f ) && approximately( maximum.z, -16.5f ),
               "Rectangular building body must stand on its origin and retain the authored X/Z dimensions" );
        check( approximately( plain.roof.bounds.getMinimum().y, maximum.y ) &&
               plain.roof.bounds.getMaximum().y < maximum.y + 1.2f,
               "Roof deck, parapet and coping must remain attached and translated only once" );
        const unsigned windowsPerFloor = 12;
        check( plain.windows.size() == windowsPerFloor * 5 && plain.interiors.size() == 5,
               "Facade bay density and optional room partitions must follow the footprint and floor count" );
        const Vector3F sides[] = { { 0, 0, 1 }, { 1, 0, 0 }, { 0, 0, -1 }, { -1, 0, 0 } };
        const size_t sideWindow[] = { 0, 4, 6, 10 };
        const float sidePlane[] = { -16.5f, 36.f, 23.5f, -24.f };
        for( size_t side = 0; side < 4; ++side )
        {
            const auto &window = plain.windows[sideWindow[side]];
            meshValid( window );
            bool outward = false;
            for( const auto &v : window.vertices )
                outward |= v.normal.dotProduct( sides[side] ) > .99f &&
                            v.position.dotProduct( sides[side] ) > sidePlane[side] + .075f;
            check( outward, "Every facade window must have a face and rotated normal proud of its wall" );
        }
        for( const auto &detail : plain.details ) meshValid( detail );
        for( const auto &interior : plain.interiors ) meshValid( interior );
        const auto open = generator.buildWindow( WindowState::Open, 21 );
        const auto glazed = generator.buildWindow( WindowState::Glazed, 21 );
        const auto ajar = generator.buildWindow( WindowState::Ajar, 21 );
        meshValid( open ); meshValid( glazed ); meshValid( ajar );
        const auto blocksCentre = []( const ProceduralMesh &window ) {
            // Intersect the origin's +Z ray with the XY projection of each triangle.
            for( size_t i = 0; i < window.indices.size(); i += 3 )
            {
                const auto a = window.vertices[window.indices[i]].position;
                const auto b = window.vertices[window.indices[i + 1]].position;
                const auto c = window.vertices[window.indices[i + 2]].position;
                const auto ab = ( b - a ).crossProduct( -a ).z;
                const auto bc = ( c - b ).crossProduct( -b ).z;
                const auto ca = ( a - c ).crossProduct( -c ).z;
                const auto area = ( b - a ).crossProduct( c - a ).z;
                if( std::abs( area ) > .00001f && ( ( ab >= 0 && bc >= 0 && ca >= 0 ) ||
                     ( ab <= 0 && bc <= 0 && ca <= 0 ) ) ) return true;
            }
            return false;
        };
        check( !blocksCentre( open ) && blocksCentre( glazed ) &&
               ajar.bounds.getMaximum().z > glazed.bounds.getMaximum().z + .15f,
               "Open window mesh must contain a real opening; ajar pane must swing out from its hinge" );

        BuildingSpec::Setback setback;
        setback.fromFloor = 2; setback.depth = 2; setback.side = 1;
        spec.setbacks.push_back( setback );
        const auto terraced = generator.generate( spec, origin );
        const auto repeated = generator.generate( spec, origin );
        meshValid( terraced.body ); meshValid( terraced.roof );
        bool reachesInset = false;
        for( const auto &vertex : terraced.body.vertices )
            if( vertex.position.y > 10.001f )
            {
                check( vertex.position.x <= 34.001f, "Setback must inset upper storeys on the selected side" );
                reachesInset = reachesInset || approximately( vertex.position.x, 34.f );
            }
        check( reachesInset && approximately( terraced.roof.bounds.getMaximum().x, 34.05f ) &&
               terraced.details.size() > plain.details.size(),
               "Roof and terrace must follow the setback footprint" );
        check( terraced.body.indices.size() == repeated.body.indices.size() &&
               terraced.body.vertices.size() == repeated.body.vertices.size() &&
               terraced.windows.size() == repeated.windows.size() &&
               terraced.details.size() == repeated.details.size() &&
               terraced.wallKeyVariant == repeated.wallKeyVariant,
               "Repeated building seeds must reproduce the assembly" );
        for( size_t i = 0; i < terraced.body.vertices.size(); ++i )
            check( terraced.body.vertices[i].position == repeated.body.vertices[i].position,
                   "Repeated building seeds must reproduce exact body placement" );
        for( size_t i = 0; i < terraced.windows.size(); ++i )
        {
            const auto &a = terraced.windows[i], &b = repeated.windows[i];
            check( a.vertices.size() == b.vertices.size(), "Repeated seeds must reproduce facade window states" );
            for( size_t j = 0; j < a.vertices.size(); ++j )
                check( a.vertices[j].position == b.vertices[j].position,
                       "Repeated seeds must reproduce facade window positions" );
        }

        const auto facadeProjection = []( const BuildingResult &result ) {
            float front = result.body.bounds.getMaximum().z;
            for( const auto &detail : result.details )
                front = std::max( front, detail.bounds.getMaximum().z );
            return front;
        };
        const auto residential = facadeProjection( terraced );
        spec.type = BuildingSpec::Type::Commercial;
        const auto commercial = generator.generate( spec, origin );
        spec.type = BuildingSpec::Type::Industrial;
        const auto industrial = generator.generate( spec, origin );
        spec.type = BuildingSpec::Type::MixedUse;
        const auto mixed = generator.generate( spec, origin );
        check( facadeProjection( commercial ) > residential + .1f &&
               facadeProjection( mixed ) > residential + .1f &&
               industrial.roof.bounds.getMaximum().y < commercial.roof.bounds.getMaximum().y &&
               industrial.roof.indices.size() > commercial.roof.indices.size() &&
               mixed.details.size() < commercial.details.size(),
               "Style presets must change built facade/roof geometry rather than only a material key" );
        spec.floors = -1;
        check( !generator.generate( spec, origin ).body.hasGeometry(),
               "Malformed building dimensions must not publish geometry" );
    }
    inline void run()
    {
        geometry();
        Budget budget;
        {
            budget.set( budget.used() );
            denied( []{ auto spec = std::make_unique<BuildingSpec>(); } );
            denied( []{ auto result = std::make_unique<BuildingResult>(); } );
            budget.set( budget.original );
            BuildingSpec source, destination;
            Array<BuildingSpec::Setback> foreign;
            foreign.reserve( 37 ); foreign.push_back( BuildingSpec::Setback{} );
            source.setbacks = foreign; // Existing mutable-array foreign policy boundary.
            std::string text( 4096, 'x' ); text.reserve( 8193 );
            source.wallKey = text; source.floors = 11; source.width = 19;
            destination.floors = 7; destination.width = 9; destination.wallKey = "old";
            destination.setbacks.reserve( 2 );
            BuildingSpec::Setback old; old.from = 3; destination.setbacks.push_back( old );
            const auto baseline = budget.used();
            budget.set( baseline + storage( source.setbacks ) );
            denied( [&]{ destination = source; } );
            check( destination.floors == 7 && destination.width == 9 && destination.wallKey.view() == "old" &&
                   destination.setbacks.size() == 1 && destination.setbacks[0].from == 3 &&
                   budget.used() == baseline,
                   "Late building option string denial preserves all prior fields and releases staged setbacks" );
            budget.set( budget.original ); destination = source;
            check( destination.floors == 11 && destination.setbacks.capacity() == 37 &&
                   destination.setbacks.get_allocator().isSourceBudgeted() &&
                   destination.wallKey.capacity() >= text.capacity(),
                   "Building option imports admit foreign spare capacity and owned material text" );
        }
        {
            BuildingResult source, destination;
            fill( source.body, 2 ); fill( destination.body, 9 );
            Array<ProceduralMesh> foreignWindows;
            foreignWindows.reserve( 7 ); foreignWindows.emplace_back(); fill( foreignWindows[0], 3 );
            source.windows = foreignWindows;
            source.details.reserve( 9 );
            source.interiors.reserve( 11 ); source.interiors.emplace_back(); fill( source.interiors[0], 4 );
            destination.interiors.emplace_back(); fill( destination.interiors[0], 8 );
            source.position = Vector3<real_Num>( 1,2,3 );
            destination.position = Vector3<real_Num>( 7,8,9 ); destination.wallKeyVariant = real_Num( .9 );
            const auto baseline = budget.used();
            const auto beforeInteriors = storage( source.body ) + storage( source.windows ) +
                                         storage( source.windows[0] ) + storage( source.details );
            budget.set( baseline + beforeInteriors );
            denied( [&]{ destination = source; } );
            check( destination.body.vertices[0].position.x == 9 &&
                   destination.interiors[0].vertices[0].position.x == 8 &&
                   destination.position.X() == 7 && destination.wallKeyVariant == real_Num( .9 ) &&
                   budget.used() == baseline,
                   "Late building interior admission preserves every published geometry layer" );
            budget.set( budget.original ); destination = source;
            check( destination.windows.capacity() == 7 && destination.details.capacity() == 9 &&
                   destination.interiors.capacity() == 11 && destination.windows.get_allocator().isSourceBudgeted() &&
                   destination.windows[0].vertices.get_allocator().isSourceBudgeted() &&
                   destination.windows[0].vertices.capacity() == 13,
                   "Building result snapshots admit foreign outer arrays and nested mesh spare capacity" );
            auto retained = std::make_unique<BuildingResult>( source );
            const auto live = budget.used();
            budget.set( live ); retained.reset();
            check( budget.used() < live && source.body.vertices[0].position.x == 2,
                   "Full-cap building result retirement releases body and nested storage without changing snapshots" );
            budget.set( budget.original );
        }
        {
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( []{ WPGeometryKit::sandbag( 11 ); } );
            denied( []{ WPGeometryKit::rock( 11, real_Num( 1 ) ); } );
            denied( []{ WPGeometryKit::patch( 11, real_Num( 1 ), 6, real_Num( .2 ) ); } );
            check( budget.used() == baseline, "Denied geometry-kit scratch retires every temporary lease" );
            budget.set( budget.original );
            auto sandbag = WPGeometryKit::sandbag( 11 );
            auto rock = WPGeometryKit::rock( 11, real_Num( 1 ) );
            auto patch = WPGeometryKit::patch( 11, real_Num( 1 ), 6, real_Num( .2 ) );
            check( sandbag.hasGeometry() && rock.hasGeometry() && patch.hasGeometry() &&
                   sandbag.vertices.get_allocator().isSourceBudgeted() &&
                   rock.indices.get_allocator().isSourceBudgeted() && patch.vertices.get_allocator().isSourceBudgeted(),
                   "Geometry-kit scratch retries with admitted complete mesh payloads" );
        }
        {
            WPBuildingGenerator generator( 13 );
            BuildingSpec spec; spec.floors = 1; spec.width = spec.depth = 4;
            BuildingResult previous;
            fill( previous.body, 9 );
            const auto baseline = budget.used();
            budget.set( baseline );
            denied( [&]{ previous = generator.generate( spec, Vector3<real_Num>( 0,0,0 ) ); } );
            check( previous.body.vertices[0].position.x == 9 && budget.used() == baseline,
                   "Denied real building construction preserves the caller result and retires temporary geometry" );
            budget.set( budget.original );
            previous = generator.generate( spec, Vector3<real_Num>( 0,0,0 ) );
            check( previous.body.hasGeometry() && !previous.windows.empty() &&
                   previous.windows.get_allocator().isSourceBudgeted() &&
                   previous.details.get_allocator().isSourceBudgeted() && previous.interiors.get_allocator().isSourceBudgeted(),
                   "Real building generation retries with admitted mesh layers and assembly inventories" );
        }
    }
}
