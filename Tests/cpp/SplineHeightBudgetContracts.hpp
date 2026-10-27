#pragma once
#include "CityInventoryBudgetContracts.hpp"
#include <Workphone/Math/LinearSpline1.hpp>
#include <Workphone/Math/LinearSpline4.hpp>
#include <cmath>

namespace spline_height_budget_contracts
{
    using namespace city_inventory_budget_contracts;
    class Height : public LinearSpline1<real_Num>
    {
    public:
        size_t timeCapacity() const { return m_times.capacity(); }
        size_t pointCapacity() const { return m_points.capacity(); }
        size_t tangentCapacity() const { return m_tangents.capacity(); }
        size_t timeCount() const { return m_times.size(); }
        size_t tangentCount() const { return m_tangents.size(); }
        bool automatic() const { return m_autoCalc; }
        bool admitted() const { return m_times.get_allocator().isSourceBudgeted() &&
            m_points.get_allocator().isSourceBudgeted() && m_tangents.get_allocator().isSourceBudgeted(); }
    };
    class Four : public LinearSpline4<real_Num>
    {
    public:
        size_t capacity() const { return m_points.capacity(); }
        bool admitted() const { return m_points.get_allocator().isSourceBudgeted(); }
        void importForeign( const Array<Vector4<real_Num>> &input ) { m_points = input; }
    };
    inline void run()
    {
        Budget budget;
        {
            Array<real_Num> values;
            values.reserve( 67 ); values.push_back( 0 ); values.push_back( 10 );
            Height curve;
            curve.setAutoCalculate( false );
            const auto baseline = budget.used();
            const auto times = sourceBudgetStorageBytes( 67 * sizeof( real_Num ), alignof( real_Num ) );
            const auto points = sourceBudgetStorageBytes( 67 * sizeof( Vector3<real_Num> ), alignof( Vector3<real_Num> ) );
            budget.set( baseline + times + points - 1 );
            denied( [&]{ curve.setPoints( values ); } );
            check( curve.getNumPoints() == 0 && curve.timeCount() == 0 && curve.tangentCount() == 0 &&
                   budget.used() == baseline, "Late height-value denial preserves all inventories and retires candidate times" );
            budget.set( baseline + times + points );
            curve.setPoints( values );
            check( curve.timeCapacity() == 67 && curve.pointCapacity() == 67 && curve.admitted() &&
                   budget.used() == baseline + times + points,
                   "Height-curve imports charge exact input spare capacity for keys and converted values" );
            denied( [&]{ curve.setAutoCalculate( true ); } );
            check( !curve.automatic() && curve.tangentCount() == 0,
                   "Denied tangent preparation leaves automatic-mode configuration intact" );
            budget.set( budget.original ); curve.setAutoCalculate( true );
            check( curve.tangentCount() == 2 && std::abs( curve.interpolate( real_Num( 0.5 ) ) - 5 ) < 0.001,
                   "Admitted height curves publish executable tangents" );
            Height retained( curve );
            check( retained.timeCapacity() == 67 && retained.pointCapacity() == 67 &&
                   retained.tangentCapacity() == curve.tangentCapacity() && retained.admitted(),
                   "Height-curve copies retain every admitted spare buffer" );
            Height target;
            Array<real_Num> previous;
            previous.push_back( 42 ); previous.push_back( 42 ); target.setPoints( previous );
            const auto beforeAssignment = budget.used();
            budget.set( beforeAssignment + times + points );
            denied( [&]{ target = curve; } );
            check( target.getNumPoints() == 2 && target.timeCount() == 2 && target.tangentCount() == 2 &&
                   target.interpolate( real_Num( 0.5 ) ) == 42 && budget.used() == beforeAssignment,
                   "Late copied-tangent denial preserves the complete destination height curve" );
            budget.set( budget.used() );
            curve.clear(); target.unload( nullptr );
            check( curve.timeCapacity() == 0 && curve.pointCapacity() == 0 && curve.tangentCapacity() == 0 &&
                   target.timeCapacity() == 0 && target.pointCapacity() == 0 && target.tangentCapacity() == 0 &&
                   retained.interpolate( real_Num( 0.5 ) ) == 5,
                   "Full-cap height clear/unload physically retire all buffers while copies remain valid" );
            budget.set( budget.original );
        }
        {
            Height curve;
            Array<Pair<real_Num,real_Num>> pairs;
            pairs.reserve( 43 ); pairs.push_back( { 2,10 } ); pairs.push_back( { 7,20 } );
            curve.setPoints( pairs ); curve.setPoints( pairs );
            check( curve.getNumPoints() == 2 && curve.timeCount() == 2 && curve.tangentCount() == 2 &&
                   curve.timeCapacity() == 43 && curve.pointCapacity() == 43 &&
                   curve.interpolate( real_Num( 0 ) ) == 10 && curve.interpolate( real_Num( 1 ) ) == 20,
                   "Height setters replace coherent key/value/tangent state instead of appending stale points" );
            Array<real_Num> keys, values;
            keys.push_back( 1 ); values.push_back( 2 ); values.push_back( 3 );
            bool invalid = false;
            try { curve.setPoints( keys, values ); } catch( const std::invalid_argument & ) { invalid = true; }
            check( invalid && curve.getNumPoints() == 2 && curve.interpolate( real_Num( 0 ) ) == 10,
                   "Mismatched height key/value input is rejected before mutation" );
            keys.push_back( 9 ); curve.setPoints( keys, values );
            check( curve.getNumPoints() == 2 && curve.timeCount() == 2 &&
                   curve.interpolate( real_Num( 0 ) ) == 2 && curve.interpolate( real_Num( 1 ) ) == 3,
                   "Separate height keys and values replace the previous curve consistently" );
        }
        {
            Four spline;
            Array<Vector4<real_Num>> points;
            points.reserve( 2 ); points.push_back( Vector4<real_Num>( 1,2,3,4 ) );
            points.push_back( Vector4<real_Num>( 5,6,7,8 ) );
            spline.setPoints( points );
            budget.set( budget.used() );
            denied( [&]{ spline.addPoint( Vector4<real_Num>( 9,10,11,12 ) ); } );
            denied( [&]{ spline.getPoints(); } );
            check( spline.getNumPoints() == 2 && spline.admitted(),
                   "Denied 4D growth and caller snapshots retain the current point inventory" );
            budget.set( budget.original );
            check( spline.interpolate( real_Num( -1 ) ) == points[0] &&
                   spline.interpolate( real_Num( 2 ) ) == points[1] &&
                   spline.interpolate( real_Num( 0.5 ) ) == Vector4<real_Num>( 3,4,5,6 ),
                   "4D interpolation remains normalized and clamps endpoint requests" );
            points.reserve( 83 );
            spline.setPoints( points );
            auto retained = spline.getPoints();
            Four copy( spline );
            check( retained.capacity() == 83 && retained.get_allocator().isSourceBudgeted() &&
                   copy.capacity() == 83 && copy.admitted(), "4D copies and snapshots retain admitted spare capacity" );
            Four target;
            target.addPoint( Vector4<real_Num>( 9,9,9,9 ) );
            budget.set( budget.used() );
            denied( [&]{ target = spline; } );
            check( target.getNumPoints() == 1, "Denied 4D copy assignment retains its previous points" );
            spline.unload( nullptr ); copy.clear(); target.clear();
            check( spline.capacity() == 0 && copy.capacity() == 0 && target.capacity() == 0 &&
                   retained.size() == 2 && retained[1] == points[1],
                   "Full-cap 4D retirement releases spare storage while caller snapshots survive" );
            budget.set( budget.original );
            Four foreign;
            foreign.importForeign( points );
            Four admitted( foreign );
            check( admitted.capacity() == 83 && admitted.admitted(),
                   "4D copied imports restore SourceCpu policy after explicit derived foreign substitution" );
        }
    }
}
