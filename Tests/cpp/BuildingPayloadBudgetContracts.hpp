#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <WPProcedural/WPBuildingGenerator.hpp>

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
    inline void run()
    {
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
