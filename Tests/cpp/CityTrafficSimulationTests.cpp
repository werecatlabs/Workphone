#include <Workphone/Scene/Components/CityTrafficSimulation.hpp>
#include <Workphone/Interface/Procedural/CityRoadRoundabout.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <utility>

using namespace workphone;
using namespace workphone::procedural;
using namespace workphone::scene;

namespace
{
    constexpr float pi = 3.14159265358979323846f;
    void need( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
    void check( bool value, const String &error ) { if( !value ) throw std::runtime_error( error.c_str() ); }
    SourceBudgetText guid( unsigned kind, unsigned index )
    {
        char text[40]; std::snprintf( text, sizeof( text ), "%08x-0000-4000-8000-%012x", kind, index );
        return SourceBudgetText( text );
    }
    CityRoadHeightSampler height()
    {
        return []( const Vector3F &, f32 &y ) { y = 0; return true; };
    }
    CityRoadHeightSampler slope()
    {
        return []( const Vector3F &p, f32 &y ) { y = .025f*p.x; return true; };
    }
    CityRoadSpline road( u32 id, Vector3F a, Vector3F b, float width = 8 )
    {
        CityRoadSpline result;
        result.id = guid( 3, id ); result.width = width; result.heightOffset = 0;
        const auto tangent = ( b-a ) / 3.f;
        result.knots = { { guid( 4, id*2 ), a, {}, tangent },
                         { guid( 4, id*2+1 ), b, -tangent, {} } };
        return result;
    }
    CityRoadSplineSource junctionSource()
    {
        CityRoadSplineSource result;
        result.version = 2; result.id = guid( 1, 1 ); result.revision = 7;
        result.terrainResourceUuid = guid( 2, 1 ); result.terrainRevision = 3;
        for( u32 i = 0; i < 3; ++i )
        {
            const float angle = i*2*pi/3;
            result.roads.push_back( road( i+1,
                { 100.f*std::cos( angle ), 0, 100.f*std::sin( angle ) }, {} ) );
        }
        return result;
    }
    CityRoadSplineSource roundaboutSource()
    {
        auto result = junctionSource(); result.id = guid( 1, 2 ); result.roads.clear();
        CityRoadRoundaboutSpec ring;
        ring.junctionId = guid( 5, 1 ); ring.center = {};
        ring.outerRadius = 14; ring.islandRadius = 8;
        for( u32 i = 0; i < 3; ++i )
        {
            const float angle = .18f+i*2*pi/3;
            const Vector3F outward( std::cos( angle ), 0, std::sin( angle ) );
            auto approach = road( 10+i, outward*112.f, outward*32.f, 6 );
            ring.ports.push_back( { approach.id, outward, 6, 18 } );
            result.roads.push_back( std::move( approach ) );
        }
        result.roundabouts.push_back( std::move( ring ) );
        return result;
    }
    CityRoadSplineBake sampled( const CityRoadSplineSource &source )
    {
        CityRoadSplineBake bake; String error;
        check( sampleCityRoadSplines( source, {}, height(), bake, error ), error );
        return bake;
    }
    float distance( Vector3F a, Vector3F b ) { return std::hypot( a.x-b.x, a.z-b.z ); }
    void spaced( const CityTrafficSimulation &simulation )
    {
        const auto &vehicles = simulation.vehicles();
        need( simulation.stats().valid, "Traffic reported invalid poses or collision." );
        for( size_t i = 0; i < vehicles.size(); ++i ) for( size_t j = i+1; j < vehicles.size(); ++j )
        {
            const float separation = distance( vehicles[i].position, vehicles[j].position );
            need( separation >= 2.09f, "Traffic bodies overlapped." );
            if( vehicles[i].lane == vehicles[j].lane )
                need( separation >= 5.5f, "Same-lane traffic lost headway." );
        }
    }
    void deterministicJunctions()
    {
        const auto source = junctionSource(); const auto bake = sampled( source );
        CityTrafficSettings settings; settings.densityPercent = 100; settings.maximumVehicles = 6;
        CityTrafficSimulation a, b, half; String error;
        check( a.initialize( source, bake, height(), settings, 42, error ), error );
        check( b.initialize( source, bake, height(), settings, 42, error ), error );
        check( half.initialize( source, bake, height(), settings, 42, error ), error );
        need( !a.vehicles().empty() && a.vehicles().size() <= settings.maximumVehicles &&
              a.sourceRevision() == source.revision, "Traffic density/revision is invalid." );
        for( u32 tick = 0; tick < 1200; ++tick )
        {
            a.step( 1.f/30.f ); b.step( 1.f/30.f );
            half.step( 1.f/60.f ); half.step( 1.f/60.f );
            if( tick % 30 == 0 ) spaced( a );
        }
        need( a.stats().intersectionTransfers > 0 && a.stats().travelledMetres > 30 &&
              a.stats().moving > 0, "Cars did not traverse a connected ordinary junction." );
        need( a.vehicles().size() == b.vehicles().size() && a.vehicles().size() == half.vehicles().size(),
              "Identical traffic runs changed their population." );
        for( size_t i = 0; i < a.vehicles().size(); ++i )
        {
            const auto &x = a.vehicles()[i], &y = b.vehicles()[i], &z = half.vehicles()[i];
            need( x.id == y.id && x.id == z.id && x.lane == y.lane && x.lane == z.lane &&
                  distance( x.position, y.position ) < .00001f &&
                  distance( x.position, z.position ) < .00001f &&
                  std::abs( x.speed-y.speed ) < .00001f && std::abs( x.speed-z.speed ) < .00001f,
                  "Traffic is not deterministic across repeated/fractional fixed steps." );
        }
        auto invalidSource = source; ++invalidSource.revision;
        const auto before = a.vehicles().front();
        need( !a.initialize( invalidSource, bake, height(), settings, 42, error ) &&
              a.sourceRevision() == source.revision &&
              distance( a.vehicles().front().position, before.position ) == 0,
              "Stale source initialization mutated the running simulation." );
    }
    void densityAndObstacle()
    {
        const auto source = junctionSource(); const auto bake = sampled( source );
        CityTrafficSettings settings; settings.densityPercent = 0;
        CityTrafficSimulation empty; String error;
        check( empty.initialize( source, bake, height(), settings, 1, error ), error );
        need( empty.vehicles().empty() && empty.stats().active == 0,
              "Zero density still spawned traffic." );
        settings.densityPercent = 100; settings.maximumVehicles = 1;
        CityTrafficSimulation blocked;
        check( blocked.initialize( source, bake, height(), settings, 12, error ), error );
        need( blocked.vehicles().size() == 1, "One-car density bound was not respected." );
        const auto first = blocked.vehicles().front();
        const auto obstacle = first.position + first.forward*5.f;
        blocked.setExternalObstacle( obstacle, 1.5f );
        for( u32 tick = 0; tick < 120; ++tick )
        {
            blocked.step( 1.f/30.f );
            need( distance( blocked.vehicles().front().position, obstacle ) >= 2.f,
                  "Traffic entered the external/player obstacle." );
        }
        need( blocked.stats().travelledMetres < 5.f,
              "Traffic did not brake for the external/player obstacle." );
        CityTrafficSimulation excluded;
        excluded.setExternalObstacle( first.position, 10.f );
        check( excluded.initialize( source, bake, height(), settings, 12, error ), error );
        for( const auto &vehicle : excluded.vehicles() )
            need( distance( vehicle.position, first.position ) >= 10.f+settings.minimumGap+4.2f,
                  "Initial traffic spawn overlapped the player exclusion." );
    }
    void measuredVehicleProgress()
    {
        auto source = junctionSource();
        for( auto &spline : source.roads )
            for( auto &knot : spline.knots )
            {
                knot.position *= 4.f;
                knot.inHandle *= 4.f;
                knot.outHandle *= 4.f;
            }
        const auto bake = sampled( source );
        CityTrafficSettings settings;
        settings.densityPercent = 100;
        settings.maximumVehicles = 1;
        CityTrafficSimulation simulation;
        String error;
        check( simulation.initialize( source, bake, height(), settings, 12, error ), error );
        need( simulation.vehicles().size() == 1, "Measured-motion fixture did not spawn its car." );
        const auto start = simulation.vehicles().front();

        // The planner can request motion, but only the physics body advances the car.
        simulation.setVehicleMotion( start.id, start.position, 0.f );
        for( u32 tick = 0; tick < 60; ++tick )
            simulation.step( 1.f / 30.f );
        const auto held = simulation.vehicles().front();
        need( distance( held.position, start.position ) < .01f && held.lane == start.lane &&
                  held.respawns == start.respawns && simulation.stats().travelledMetres < .01f,
              "A stationary physics car drifted along its planned lane." );
        need( simulation.stats().valid && std::isfinite( held.speed ) && held.speed > 0.f &&
                  held.speed <= settings.cruiseSpeed,
              "A stationary car on a clear road received no valid driving speed." );
        Vector3F firstAim;
        need( simulation.getDrivingTarget( start.id, 2.f, firstAim ) && firstAim.isFinite() &&
                  ( firstAim - start.position ).dotProduct( start.forward ) > 1.f,
              "A measured car received no finite aim point ahead of it." );

        const auto moved = start.position + start.forward * 3.f;
        simulation.setVehicleMotion( start.id, moved, 2.f );
        simulation.step( 1.f / 30.f );
        const auto advanced = simulation.vehicles().front();
        Vector3F secondAim;
        need( distance( advanced.position, moved ) < .05f &&
                  simulation.stats().travelledMetres > 2.9f && advanced.lane == start.lane,
              "The planner did not advance its lane station to match the measured car." );
        need( simulation.getDrivingTarget( start.id, 2.f, secondAim ) && secondAim.isFinite() &&
                  ( secondAim - firstAim ).dotProduct( start.forward ) > 2.9f,
              "The driving aim point did not advance with the physics body." );

        simulation.setExternalObstacle( moved + start.forward * 3.f, 1.5f );
        simulation.setVehicleMotion( start.id, moved, 0.f );
        for( u32 tick = 0; tick < 60; ++tick )
            simulation.step( 1.f / 30.f );
        const auto stopped = simulation.vehicles().front();
        need( distance( stopped.position, moved ) < .05f && stopped.yielding &&
                  std::abs( stopped.speed ) < .01f,
              "The physical car received a forward command through an obstacle." );
    }
    void drivingSide()
    {
        const auto source = junctionSource(); const auto bake = sampled( source );
        CityTrafficSettings settings; settings.densityPercent = 100; settings.maximumVehicles = 1;
        CityTrafficSimulation rightHand, leftHand; String error;
        check( rightHand.initialize( source, bake, height(), settings, 29, error ), error );
        settings.driveOnLeft = true;
        check( leftHand.initialize( source, bake, height(), settings, 29, error ), error );
        need( rightHand.vehicles().size() == 1 && leftHand.vehicles().size() == 1,
              "Driving-side fixture did not spawn one car on each lane." );
        const auto &right = rightHand.vehicles().front();
        const auto &left = leftHand.vehicles().front();
        need( right.id == left.id && right.lane == left.lane &&
              right.forward.dotProduct( left.forward ) > .999f,
              "Driving-side setting changed route identity or travel direction." );
        const Vector3F rightOfTravel( right.forward.z, 0, -right.forward.x );
        need( ( right.position-left.position ).dotProduct( rightOfTravel ) > 3.f,
              "Right-hand driving is not on the driver's right of left-hand driving." );
    }
    void densityScaling()
    {
        const auto source = junctionSource(); const auto bake = sampled( source );
        CityTrafficSettings settings; settings.maximumVehicles = 32;
        settings.densityPercent = 35;
        CityTrafficSimulation ordinary, packed; String error;
        check( ordinary.initialize( source, bake, height(), settings, 17, error ), error );
        settings.densityPercent = 100;
        check( packed.initialize( source, bake, height(), settings, 17, error ), error );
        need( ordinary.stats().active > 0 &&
              ordinary.stats().active < packed.stats().active && packed.stats().active <= 32,
              "Traffic density did not scale below the bounded full capacity." );
    }
    void slopedPose()
    {
        const auto source = junctionSource();
        CityRoadSplineBake bake; String error;
        check( sampleCityRoadSplines( source, {}, slope(), bake, error ), error );
        CityTrafficSettings settings; settings.densityPercent = 100; settings.maximumVehicles = 1;
        bool checked = false;
        for( u32 seed = 0; seed < 32 && !checked; ++seed )
        {
            CityTrafficSimulation simulation;
            check( simulation.initialize( source, bake, slope(), settings, seed, error ), error );
            if( simulation.vehicles().empty() ) continue;
            const auto &forward = simulation.vehicles().front().forward;
            if( std::abs( forward.x ) < .5f ) continue;
            need( std::abs( forward.y ) > .01f &&
                  std::abs( forward.y-.025f*forward.x ) < .001f,
                  "Traffic heading discarded the fitted hillside grade." );
            checked = true;
        }
        need( checked, "Sloped-road fixture found no car heading across the grade." );
    }
    void queuedLaneProgress()
    {
        const auto source = junctionSource(); const auto bake = sampled( source );
        CityTrafficSettings settings; settings.densityPercent = 100; settings.maximumVehicles = 6;
        String error;
        bool checked = false;
        for( u32 seed = 0; seed < 256 && !checked; ++seed )
        {
            CityTrafficSimulation simulation;
            check( simulation.initialize( source, bake, height(), settings, seed, error ), error );
            const auto &cars = simulation.vehicles();
            u32 rearId = 0, leadId = 0;
            bool queue = false;
            for( const auto &rear : cars ) for( const auto &lead : cars )
            {
                if( rear.id >= lead.id || rear.lane != lead.lane ) continue;
                const auto along = ( lead.position-rear.position ).dotProduct( rear.forward );
                if( along > 7.f && along < 35.f )
                { rearId = rear.id; leadId = lead.id; queue = true; break; }
            }
            if( !queue ) continue;
            std::array<f32, 6> travel{};
            std::array<Vector3F, 6> previous{};
            for( const auto &car : cars ) previous[car.id] = car.position;
            for( u32 tick = 0; tick < 1200; ++tick )
            {
                simulation.step( 1.f/30.f );
                for( const auto &car : simulation.vehicles() )
                {
                    travel[car.id] += std::min( 2.f, distance( previous[car.id], car.position ) );
                    previous[car.id] = car.position;
                }
            }
            need( simulation.stats().valid && travel[leadId] > 50.f && travel[rearId] > 50.f,
                  "A lower-ID rear car stranded the lead car at a junction." );
            checked = true;
        }
        need( checked, "Queue fixture found no lower-ID car behind a leader." );
    }
    void authoredRoadRevisionRebake()
    {
        auto source = junctionSource(); const auto firstBake = sampled( source );
        CityTrafficSettings settings; settings.densityPercent = 100; settings.maximumVehicles = 6;
        CityTrafficSimulation simulation; String error;
        check( simulation.initialize( source, firstBake, height(), settings, 42, error ), error );
        const auto original = simulation.vehicles();
        need( !original.empty(), "Authored-road revision fixture spawned no traffic." );
        simulation.step( 1.f/30.f );

        const Vector3F shift( 300.f, 0, 250.f );
        for( auto &road : source.roads ) for( auto &knot : road.knots )
            knot.position += shift;
        ++source.revision;
        const auto rebake = sampled( source );
        check( simulation.initialize( source, rebake, height(), settings, 42, error ), error );
        need( simulation.sourceRevision() == source.revision && simulation.stats().valid &&
              simulation.vehicles().size() == original.size(),
              "Traffic did not accept the revised authored road bake." );
        for( size_t i = 0; i < original.size(); ++i )
        {
            const auto &before = original[i], &after = simulation.vehicles()[i];
            need( before.id == after.id && before.lane == after.lane &&
                  ( after.position-( before.position+shift ) ).length() < .002f,
                  "Traffic kept stale lane poses after an authored road moved." );
        }
    }
    void roundaboutArcs()
    {
        const auto source = roundaboutSource(); const auto bake = sampled( source );
        CityTrafficSettings settings; settings.densityPercent = 100; settings.maximumVehicles = 5;
        CityTrafficSimulation simulation; String error;
        check( simulation.initialize( source, bake, height(), settings, 22, error ), error );
        need( !simulation.vehicles().empty(), "Roundabout scene spawned no traffic." );
        for( u32 tick = 0; tick < 1800; ++tick )
        {
            simulation.step( 1.f/30.f );
            for( const auto &vehicle : simulation.vehicles() )
                need( distance( vehicle.position, source.roundabouts.front().center ) >=
                    source.roundabouts.front().islandRadius-.05f,
                    "Traffic crossed the raised roundabout island." );
            if( tick % 30 == 0 ) spaced( simulation );
        }
        need( simulation.stats().roundaboutTransfers > 0,
              "Traffic did not use the directed roundabout route bake." );
    }
}

int main()
{
    try
    {
        deterministicJunctions();
        densityAndObstacle();
        measuredVehicleProgress();
        drivingSide();
        densityScaling();
        slopedPose();
        queuedLaneProgress();
        authoredRoadRevisionRebake();
        roundaboutArcs();
        std::cout << "City traffic: deterministic junction and roundabout transfers, density, following, "
                     "player avoidance, measured vehicle progress and rollback PASS\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "City traffic FAIL: " << error.what() << '\n';
        return 1;
    }
}
