#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <Workphone/Core/Grid2.hpp>
#include <Workphone/Core/Grid3.hpp>
#include <memory>

namespace grid_storage_budget_contracts
{
    using namespace city_inventory_budget_contracts;

    template<class T> inline void scalarBody()
    {
        Budget budget;
        const auto baseline = budget.used();
        const auto bytes = sourceBudgetStorageBytes( sizeof( T ), alignof( std::max_align_t ) );
        budget.set( baseline + bytes - 1 );
        denied( []{ auto grid = std::make_unique<T>(); } );
        check( budget.used() == baseline, "Denied scalar grid body leaves no reservation" );
        budget.set( baseline + bytes );
        auto grid = std::make_unique<T>();
        check( budget.used() == baseline + bytes, "Scalar grid body requires its exact physical allocation" );
        budget.set( budget.used() );
        grid.reset();
        check( budget.used() == baseline, "Scalar grid body physically retires at the cap" );
    }

    inline void run()
    {
        scalarBody<Grid2>();
        scalarBody<Grid3>();
        Budget budget;
        const auto baseline = budget.used();
        budget.set( baseline );
        Grid2 grid2( Vector2<real_Num>( 0,0 ), Vector2<real_Num>( 2,4 ), Vector2<real_Num>( 10,8 ) );
        Vector2<real_Num> position2;
        grid2.calculatePosition( Vector2I( 3,2 ), position2 );
        Vector2I cell2;
        grid2.calculateCell( position2, cell2 );
        auto copy2 = grid2;
        copy2.setExtents( Vector2<real_Num>( 10000,20000 ) );
        check( position2 == Vector2<real_Num>( -4,0 ) && cell2 == Vector2I( 3,2 ) &&
               grid2.getNumCells() == Vector2I( 10,4 ) && copy2.getNumCells() == Vector2I( 10000,10000 ),
               "Grid2 coordinate math and copies work at the cap without cell allocations" );

        Grid3 grid3;
        grid3.initialise( Vector3<real_Num>( 0,0,0 ), Vector3<real_Num>( 2,4,5 ), Vector3<real_Num>( 10,8,15 ) );
        Vector3<real_Num> position3;
        grid3.calculatePosition( Vector3I( 3,2,4 ), position3 );
        Vector3I cell3;
        grid3.calculateGridSlot( position3, cell3 );
        auto copy3 = grid3;
        check( position3 == Vector3<real_Num>( -4,0,5 ) && cell3 == Vector3I( 3,2,4 ) &&
               grid3.getNumCells() == Vector3I( 10,4,6 ) && copy3.isInitialised(),
               "Grid3 coordinate math and copies work at the cap without cell allocations" );
        check( budget.used() == baseline, "Stack and embedded coordinate grids have no separate heap payload" );
    }
}
