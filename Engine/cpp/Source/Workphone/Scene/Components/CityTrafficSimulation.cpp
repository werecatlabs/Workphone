#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CityTrafficSimulation.hpp>
#include <Workphone/Scene/Components/CityRoadJunctions.hpp>
#include <Workphone/Interface/Procedural/CityRoadRoundabout.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        using namespace procedural;
        constexpr u32 invalid = std::numeric_limits<u32>::max();
        constexpr f32 tickSeconds = 1.f / 30.f;
        constexpr f32 carLength = 4.2f;

        enum class PathKind : u8 { Road, Junction, Roundabout };

        struct Path
        {
            PathKind kind = PathKind::Road;
            u32 roadIndex = invalid, conflict = invalid;
            f32 roadWidth = 0, length = 0, speedLimit = 0;
            SourceBudgetVector<Vector3F> points;
            SourceBudgetVector<f32> stations;
            SourceBudgetVector<u32> outgoing;
        };

        struct RunLanes
        {
            u32 roadIndex = invalid, forward = invalid, reverse = invalid;
            f32 first = 0, last = 0;
        };

        struct Agent
        {
            u32 id = 0, path = invalid, next = invalid, heldConflict = invalid;
            u32 transfers = 0, waitTicks = 0;
            f32 station = 0, speed = 0;
            bool yielding = false;
        };

        u64 hash( u64 key )
        {
            key += 0x9e3779b97f4a7c15ull;
            key = ( key ^ ( key >> 30 ) ) * 0xbf58476d1ce4e5b9ull;
            key = ( key ^ ( key >> 27 ) ) * 0x94d049bb133111ebull;
            return key ^ ( key >> 31 );
        }

        f32 unit( u64 key ) { return f32( hash( key ) >> 40 ) / 16777216.f; }
        f32 horizontal( Vector3F a, Vector3F b ) { return std::hypot( b.x-a.x, b.z-a.z ); }
        f32 horizontalSquared( Vector3F a, Vector3F b )
        {
            const f32 x = b.x-a.x, z = b.z-a.z;
            return x*x+z*z;
        }

        Vector3F positionAt( const Path &path, f32 station )
        {
            if( path.points.empty() ) return {};
            const auto at = std::clamp( station, 0.f, path.length );
            const auto upper = std::upper_bound( path.stations.begin(), path.stations.end(), at );
            if( upper == path.stations.begin() ) return path.points.front();
            if( upper == path.stations.end() ) return path.points.back();
            const size_t i = size_t( upper - path.stations.begin() );
            const f32 span = path.stations[i] - path.stations[i-1];
            const f32 t = span > 1e-6f ? ( at - path.stations[i-1] ) / span : 0.f;
            return path.points[i-1] + ( path.points[i] - path.points[i-1] ) * t;
        }

        Vector3F directionAt( const Path &path, f32 station )
        {
            const f32 before = std::max( 0.f, station - .45f ), after = std::min( path.length, station + .45f );
            auto direction = positionAt( path, after ) - positionAt( path, before );
            if( direction.lengthSquared() < 1e-8f && path.points.size() >= 2 )
                direction = path.points.back() - path.points.front();
            return direction.lengthSquared() > 1e-8f ? direction.normaliseCopy() : Vector3F( 0, 0, 1 );
        }

        bool insideJunction( const CityRoadJunction &junction, Vector3F p )
        {
            const auto vertex = [&]( u32 index ) {
                return index ? junction.boundary[index-1] : junction.center;
            };
            for( size_t i = 0; i + 2 < junction.indices.size(); i += 3 )
            {
                const auto a = vertex( junction.indices[i] ),
                           b = vertex( junction.indices[i+1] ),
                           c = vertex( junction.indices[i+2] );
                const auto cross = []( Vector3F u, Vector3F v ) { return u.x*v.z-u.z*v.x; };
                const f32 ab = cross( b-a, p-a ), bc = cross( c-b, p-b ), ca = cross( a-c, p-c );
                if( ( ab >= -.035f && bc >= -.035f && ca >= -.035f ) ||
                    ( ab <= .035f && bc <= .035f && ca <= .035f ) ) return true;
            }
            return false;
        }

        bool validSettings( const CityTrafficSettings &settings )
        {
            return settings.densityPercent <= 100 && settings.maximumVehicles <= 128 &&
                std::isfinite( settings.cruiseSpeed ) && settings.cruiseSpeed >= 1 && settings.cruiseSpeed <= 35 &&
                std::isfinite( settings.maxAcceleration ) && settings.maxAcceleration > 0 && settings.maxAcceleration <= 10 &&
                std::isfinite( settings.maxBraking ) && settings.maxBraking > 0 && settings.maxBraking <= 20 &&
                std::isfinite( settings.minimumGap ) && settings.minimumGap >= 3 && settings.minimumGap <= 20;
        }
    }

    struct CityTrafficSimulation::Impl
    {
        CityTrafficSettings settings;
        SourceBudgetVector<Path> paths;
        SourceBudgetVector<RunLanes> runs;
        SourceBudgetVector<u32> spawnPaths;
        SourceBudgetVector<Agent> agents;
        SourceBudgetVector<CityTrafficVehicle> poses;
        SourceBudgetVector<s32> conflictOwners;
        CityTrafficStats result;
        u64 revision = 0;
        u32 seed = 0;
        f32 remainder = 0;
        Vector3F obstacle{};
        f32 obstacleRadius = 0;
        bool obstacleEnabled = false;

        u32 addPath( SourceBudgetVector<Vector3F> points, PathKind kind, u32 roadIndex,
                     u32 conflict, f32 width, f32 speedLimit )
        {
            if( paths.size() >= 2048 || points.size() < 2 ) return invalid;
            Path path;
            path.kind = kind; path.roadIndex = roadIndex; path.conflict = conflict;
            path.roadWidth = width; path.speedLimit = speedLimit;
            path.points.reserve( points.size() ); path.stations.reserve( points.size() );
            for( const auto p : points )
            {
                if( !p.isFinite() ) return invalid;
                if( !path.points.empty() )
                {
                    const f32 d = horizontal( path.points.back(), p );
                    if( d < .02f ) continue;
                    path.length += d;
                }
                path.points.push_back( p );
                path.stations.push_back( path.length );
            }
            if( path.points.size() < 2 || path.length < .5f ) return invalid;
            const u32 index = u32( paths.size() );
            paths.push_back( std::move( path ) );
            return index;
        }

        u32 selected( const Agent &agent ) const
        {
            if( agent.path >= paths.size() ) return invalid;
            const auto &choices = paths[agent.path].outgoing;
            if( choices.empty() ) return invalid;
            const u64 key = ( u64( seed ) << 32 ) ^ ( u64( agent.id ) << 16 ) ^
                ( u64( agent.path ) << 4 ) ^ agent.transfers;
            return choices[size_t( hash( key ) % choices.size() )];
        }

        void updatePoses()
        {
            poses.resize( agents.size() );
            result.active = u32( agents.size() ); result.moving = 0; result.yielding = 0;
            result.minimumSeparation = agents.size() < 2 ? 0.f : std::numeric_limits<f32>::max();
            for( size_t i = 0; i < agents.size(); ++i )
            {
                const auto &agent = agents[i];
                const auto &path = paths[agent.path];
                auto &pose = poses[i];
                pose.id = agent.id; pose.variant = u32( hash( u64( seed ) ^ agent.id ) % 5 );
                pose.lane = agent.path;
                pose.position = positionAt( path, agent.station );
                pose.forward = directionAt( path, agent.station );
                pose.speed = agent.speed; pose.yielding = agent.yielding;
                result.moving += agent.speed > .25f;
                result.yielding += agent.yielding;
                result.valid = result.valid && pose.position.isFinite() && pose.forward.isFinite() &&
                    std::isfinite( pose.speed );
            }
            for( size_t i = 0; i < poses.size(); ++i ) for( size_t j = i+1; j < poses.size(); ++j )
                result.minimumSeparation = std::min( result.minimumSeparation,
                    horizontal( poses[i].position, poses[j].position ) );
            // Opposing lanes on a 7 m street may be only 3.5 m apart at
            // their centres. The global check is for body-width overlap;
            // longitudinal spacing is enforced separately on each lane.
            if( poses.size() > 1 && result.minimumSeparation < 2.1f ) result.valid = false;
        }

        f32 obstacleGap( const Path &path, const Agent &agent ) const
        {
            if( !obstacleEnabled ) return std::numeric_limits<f32>::max();
            f32 best = std::numeric_limits<f32>::max();
            for( size_t i = 1; i < path.points.size(); ++i )
            {
                const auto a = path.points[i-1], b = path.points[i];
                const f32 dx = b.x-a.x, dz = b.z-a.z, length2 = dx*dx+dz*dz;
                if( length2 < 1e-6f ) continue;
                const f32 t = std::clamp( ( ( obstacle.x-a.x )*dx + ( obstacle.z-a.z )*dz ) / length2, 0.f, 1.f );
                const Vector3F closest( a.x+t*dx, 0, a.z+t*dz );
                if( horizontal( closest, obstacle ) > obstacleRadius + 1.3f ) continue;
                const f32 station = path.stations[i-1] + ( path.stations[i]-path.stations[i-1] )*t;
                if( station >= agent.station-1.f ) best = std::min( best, station-agent.station-obstacleRadius );
            }
            return best;
        }

        void tick();
    };

    CityTrafficSimulation::CityTrafficSimulation() : m_impl( std::make_unique<Impl>() ) {}
    CityTrafficSimulation::~CityTrafficSimulation() = default;
    CityTrafficSimulation::CityTrafficSimulation( CityTrafficSimulation && ) noexcept = default;
    CityTrafficSimulation &CityTrafficSimulation::operator=( CityTrafficSimulation && ) noexcept = default;

    void CityTrafficSimulation::clear() { m_impl = std::make_unique<Impl>(); }
    const SourceBudgetVector<CityTrafficVehicle> &CityTrafficSimulation::vehicles() const { return m_impl->poses; }
    CityTrafficStats CityTrafficSimulation::stats() const { return m_impl->result; }
    u64 CityTrafficSimulation::sourceRevision() const { return m_impl->revision; }
    void CityTrafficSimulation::setExternalObstacle( Vector3F position, f32 radius, bool enabled )
    {
        m_impl->obstacleEnabled = enabled && position.isFinite() && std::isfinite( radius ) && radius >= 0;
        m_impl->obstacle = position; m_impl->obstacleRadius = m_impl->obstacleEnabled ? radius : 0;
    }

    bool CityTrafficSimulation::initialize( const CityRoadSplineSource &source,
        const CityRoadSplineBake &bake, const CityRoadHeightSampler &terrainHeight,
        const CityTrafficSettings &settings, u32 seed, String &error )
    {
        try
        {
            if( !validSettings( settings ) || !terrainHeight ||
                !validateCityRoadSplineSource( source, error ) ||
                source.revision != bake.sourceRevision || source.roads.size() != bake.roads.size() )
            {
                if( error.empty() ) error = "Traffic requires valid settings and a current fitted road bake.";
                return false;
            }
            if( std::any_of( source.roads.begin(), source.roads.end(), []( const auto &road ) {
                    return road.heightMode == CityRoadHeightMode::Bridge;
                } ) )
            {
                error = "Traffic cannot use an unsupported bridge road.";
                return false;
            }
            auto next = std::make_unique<Impl>();
            next->settings = settings; next->seed = seed; next->revision = source.revision;
            next->result.valid = true;
            next->obstacle = m_impl->obstacle;
            next->obstacleRadius = m_impl->obstacleRadius;
            next->obstacleEnabled = m_impl->obstacleEnabled;

            CityRoadJunctionBake junctions;
            if( !buildCityRoadJunctions( source, bake, terrainHeight, {}, junctions, error ) ) return false;
            next->conflictOwners.assign( junctions.junctions.size() + source.roundabouts.size(), -1 );

            for( const auto &run : junctions.roadRuns )
            {
                if( run.roadIndex >= source.roads.size() || run.points.size() < 2 ) continue;
                const auto &road = source.roads[run.roadIndex];
                RunLanes lanes;
                lanes.roadIndex = run.roadIndex;
                lanes.first = run.points.front().distance;
                lanes.last = run.points.back().distance;
                SourceBudgetVector<Vector3F> forward, reverse;
                forward.reserve( run.points.size() ); reverse.reserve( run.points.size() );
                const auto lanePoint = [&]( const CityRoadSplineSample &point, bool forwards ) {
                    const bool sourceRight = forwards != settings.driveOnLeft;
                    // The fitted sample's `left` edge is right of forward
                    // travel in world XZ (a +Z tangent has its edge at +X).
                    const auto edge = sourceRight ? point.left : point.right;
                    return point.position + ( edge - point.position ) * .5f;
                };
                for( const auto &point : run.points ) forward.push_back( lanePoint( point, true ) );
                for( auto it = run.points.rbegin(); it != run.points.rend(); ++it )
                    reverse.push_back( lanePoint( *it, false ) );
                const f32 roadSpeed = std::min( settings.cruiseSpeed,
                    road.width >= 11.f ? 13.f : road.width >= 8.f ? 11.f : 9.f );
                lanes.forward = next->addPath( std::move( forward ), PathKind::Road,
                                               run.roadIndex, invalid, road.width, roadSpeed );
                lanes.reverse = next->addPath( std::move( reverse ), PathKind::Road,
                                               run.roadIndex, invalid, road.width, roadSpeed );
                if( lanes.forward != invalid && lanes.reverse != invalid ) next->runs.push_back( lanes );
            }

            // Each trimmed approach has one inbound and one outbound lane. A
            // slow quadratic through the existing drivable fan makes a turn
            // without cutting a straight chord through a footway corner.
            for( size_t j = 0; j < junctions.junctions.size(); ++j )
            {
                const auto &junction = junctions.junctions[j];
                struct PortLane { u32 inbound = invalid, outbound = invalid; };
                SourceBudgetVector<PortLane> ports;
                for( const auto &port : junction.ports )
                {
                    PortLane lane;
                    for( const auto &run : next->runs )
                    {
                        if( run.roadIndex != port.roadIndex ) continue;
                        if( port.forward && std::abs( run.first-port.station ) < .02f )
                        { lane.inbound = run.reverse; lane.outbound = run.forward; break; }
                        if( !port.forward && std::abs( run.last-port.station ) < .02f )
                        { lane.inbound = run.forward; lane.outbound = run.reverse; break; }
                    }
                    ports.push_back( lane );
                }
                for( size_t entry = 0; entry < ports.size(); ++entry )
                    for( size_t exit = 0; exit < ports.size(); ++exit )
                {
                    if( entry == exit || ports[entry].inbound == invalid ||
                        ports[exit].outbound == invalid ) continue;
                    const auto &incoming = next->paths[ports[entry].inbound];
                    const auto &outgoing = next->paths[ports[exit].outbound];
                    const auto a = incoming.points.back(), b = outgoing.points.front();
                    SourceBudgetVector<Vector3F> curve;
                    curve.reserve( 17 );
                    bool supported = true;
                    for( u32 k = 0; k <= 16; ++k )
                    {
                        const f32 t = f32( k ) / 16.f, u = 1.f-t;
                        auto p = a*u*u + junction.center*( 2.f*u*t ) + b*t*t;
                        if( k != 0 && k != 16 )
                        {
                            f32 height = 0;
                            if( !insideJunction( junction, p ) || !terrainHeight( p, height ) )
                            { supported = false; break; }
                            p.y = height + junction.heightOffset;
                        }
                        curve.push_back( p );
                    }
                    if( !supported ) continue;
                    const u32 connector = next->addPath( std::move( curve ), PathKind::Junction,
                        junction.ports[entry].roadIndex, u32( j ), incoming.roadWidth,
                        std::min( settings.cruiseSpeed, 5.5f ) );
                    if( connector == invalid ) continue;
                    next->paths[ports[entry].inbound].outgoing.push_back( connector );
                    next->paths[connector].outgoing.push_back( ports[exit].outbound );
                }
            }

            // Roundabout route points are authored by the existing single-lane
            // ring bake. In particular, no connector can cross its raised island.
            const u32 ringConflictStart = u32( junctions.junctions.size() );
            for( size_t r = 0; r < source.roundabouts.size(); ++r )
            {
                const auto &spec = source.roundabouts[r];
                CityRoadRoundaboutBake ring;
                if( !bakeCityRoadRoundabout( spec, terrainHeight, ring, error ) ) return false;
                for( const auto &route : ring.routes )
                {
                    if( route.points.size() < 2 ||
                        route.entryApproachId == route.exitApproachId ) continue;
                    const auto findRoad = [&]( const SourceBudgetText &id ) {
                        const auto found = std::find_if( source.roads.begin(), source.roads.end(),
                            [&]( const auto &road ) { return road.id == id; } );
                        return found == source.roads.end() ? invalid : u32( found - source.roads.begin() );
                    };
                    const u32 entryRoad = findRoad( route.entryApproachId ),
                              exitRoad = findRoad( route.exitApproachId );
                    if( entryRoad == invalid || exitRoad == invalid ) continue;
                    u32 inbound = invalid, outbound = invalid;
                    f32 entryDistance = 12.f, exitDistance = 12.f;
                    for( const auto &run : next->runs )
                    {
                        if( run.roadIndex == entryRoad )
                            for( const u32 candidate : { run.forward, run.reverse } )
                            {
                                const f32 d = horizontal( next->paths[candidate].points.back(), route.points.front() );
                                if( d < entryDistance ) { entryDistance = d; inbound = candidate; }
                            }
                        if( run.roadIndex == exitRoad )
                            for( const u32 candidate : { run.forward, run.reverse } )
                            {
                                const f32 d = horizontal( next->paths[candidate].points.front(), route.points.back() );
                                if( d < exitDistance ) { exitDistance = d; outbound = candidate; }
                            }
                    }
                    if( inbound == invalid || outbound == invalid ) continue;
                    SourceBudgetVector<Vector3F> arc;
                    arc.reserve( route.points.size()+2 );
                    arc.push_back( next->paths[inbound].points.back() );
                    for( const auto point : route.points )
                    {
                        if( !ring.containsSurface( point.x, point.z ) )
                        { arc.clear(); break; }
                        arc.push_back( point );
                    }
                    if( arc.empty() ) continue;
                    arc.push_back( next->paths[outbound].points.front() );
                    const u32 connector = next->addPath( std::move( arc ), PathKind::Roundabout,
                        entryRoad, ringConflictStart + u32( r ), source.roads[entryRoad].width,
                        std::min( settings.cruiseSpeed, 6.5f ) );
                    if( connector == invalid ) continue;
                    next->paths[inbound].outgoing.push_back( connector );
                    next->paths[connector].outgoing.push_back( outbound );
                }
            }

            SourceBudgetVector<u32> spawns;
            f32 roadLength = 0;
            for( u32 i = 0; i < next->paths.size(); ++i )
            {
                const auto &path = next->paths[i];
                if( path.kind != PathKind::Road || path.length < 20 || path.outgoing.empty() ) continue;
                spawns.push_back( i ); roadLength += path.length;
            }
            if( spawns.empty() && settings.densityPercent > 0 )
            {
                error = "Traffic found no connected, supported city road lanes.";
                return false;
            }
            const u32 capacity = spawns.empty() ? 0u : std::min( settings.maximumVehicles,
                std::max( 1u, u32( roadLength / 90.f ) ) );
            const u32 requested = settings.densityPercent && capacity
                ? std::max( 1u, u32( std::lround( capacity * settings.densityPercent / 100.f ) ) )
                : 0u;
            next->spawnPaths = spawns;
            next->agents.reserve( requested );
            for( u32 id = 0; id < requested; ++id )
            {
                bool placed = false;
                for( u32 trial = 0; trial < 96 && !placed; ++trial )
                {
                    const u64 key = ( u64( seed ) << 32 ) ^ ( u64( id ) << 16 ) ^ trial;
                    const u32 pathIndex = spawns[size_t( hash( key ) % spawns.size() )];
                    const auto &path = next->paths[pathIndex];
                    const f32 station = 6.f + unit( key ^ 0x54c3u ) * ( path.length-12.f );
                    const auto point = positionAt( path, station );
                    placed = ( !next->obstacleEnabled ||
                        horizontal( point, next->obstacle ) >= next->obstacleRadius +
                        settings.minimumGap + carLength ) &&
                        std::all_of( next->agents.begin(), next->agents.end(), [&]( const auto &other ) {
                        return horizontal( point, positionAt( next->paths[other.path], other.station ) ) >=
                            settings.minimumGap + carLength;
                    } );
                    if( placed ) next->agents.push_back( { id, pathIndex, invalid, invalid, 0, 0, station, 0, false } );
                }
            }
            next->updatePoses();
            m_impl.swap( next );
            error.clear();
            return true;
        }
        catch( const std::bad_alloc & ) { error = "Traffic exceeded the shared SourceCpu budget."; return false; }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    void CityTrafficSimulation::Impl::tick()
    {
        // Keep a reservation until the rear of a car has cleared its exit.
        for( auto &agent : agents ) if( agent.heldConflict != invalid &&
            agent.heldConflict < conflictOwners.size() &&
            paths[agent.path].kind == PathKind::Road &&
            agent.station > carLength + settings.minimumGap )
        {
            if( conflictOwners[agent.heldConflict] == s32( agent.id ) )
                conflictOwners[agent.heldConflict] = -1;
            agent.heldConflict = invalid;
        }
        // Never let a reservation held by a rear car strand the car directly
        // ahead of it on the same inbound lane.
        for( auto &owner : agents ) if( owner.heldConflict != invalid &&
            owner.heldConflict < conflictOwners.size() &&
            paths[owner.path].kind == PathKind::Road &&
            conflictOwners[owner.heldConflict] == s32( owner.id ) )
        {
            const bool leaderAhead = std::any_of( agents.begin(), agents.end(), [&]( const Agent &other ) {
                if( other.id == owner.id || other.path != owner.path ||
                    other.station <= owner.station + .001f ) return false;
                const u32 next = other.next != invalid ? other.next : selected( other );
                return next != invalid && paths[next].conflict == owner.heldConflict;
            } );
            if( leaderAhead )
            {
                conflictOwners[owner.heldConflict] = -1;
                owner.heldConflict = invalid;
            }
        }
        for( auto &agent : agents ) if( agent.next == invalid ) agent.next = selected( agent );

        // Priority is decided before integrating any agent, so a simultaneous
        // arrival is independent of iteration side effects. Wider approaches
        // yield less often; ID resolves ties reproducibly.
        SourceBudgetVector<s32> claims( conflictOwners.size(), -1 );
        for( size_t i = 0; i < agents.size(); ++i )
        {
            const auto &agent = agents[i];
            const auto &path = paths[agent.path];
            if( path.kind != PathKind::Road || agent.next == invalid ) continue;
            const auto &nextPath = paths[agent.next];
            if( nextPath.conflict == invalid || nextPath.conflict >= claims.size() ||
                conflictOwners[nextPath.conflict] != -1 ) continue;
            const f32 approach = path.length-agent.station;
            if( approach > std::max( 8.f, agent.speed*agent.speed /
                ( 2.f*settings.maxBraking ) + settings.minimumGap + 3.f ) ) continue;
            const bool leaderEligible = std::any_of( agents.begin(), agents.end(), [&]( const Agent &other ) {
                if( other.id == agent.id || other.path != agent.path ||
                    other.station <= agent.station + .001f || other.next == invalid ||
                    paths[other.next].conflict != nextPath.conflict ) return false;
                return true;
            } );
            if( leaderEligible ) continue;
            auto &claim = claims[nextPath.conflict];
            if( claim < 0 ) { claim = s32( i ); continue; }
            const auto &prior = agents[size_t( claim )];
            if( ( agent.path == prior.path &&
                  ( agent.station > prior.station + .001f ||
                    ( std::abs( agent.station-prior.station ) <= .001f && agent.id < prior.id ) ) ) ||
                ( agent.path != prior.path &&
                  ( path.roadWidth > paths[prior.path].roadWidth + .01f ||
                    ( std::abs( path.roadWidth-paths[prior.path].roadWidth ) <= .01f &&
                      agent.id < prior.id ) ) ) ) claim = s32( i );
        }
        for( u32 i = 0; i < claims.size(); ++i ) if( claims[i] >= 0 )
        {
            auto &agent = agents[size_t( claims[i] )];
            conflictOwners[i] = s32( agent.id );
            agent.heldConflict = i;
        }

        for( size_t i = 0; i < agents.size(); ++i )
        {
            auto &agent = agents[i];
            const auto &path = paths[agent.path];
            const f32 remaining = path.length-agent.station;
            f32 target = std::min( settings.cruiseSpeed, path.speedLimit );
            f32 followingGap = obstacleGap( path, agent );
            for( size_t j = 0; j < agents.size(); ++j ) if( i != j )
            {
                const auto &other = agents[j];
                if( other.path == agent.path && other.station > agent.station )
                    followingGap = std::min( followingGap,
                        other.station-agent.station-carLength );
                else if( agent.next != invalid && other.path == agent.next )
                    followingGap = std::min( followingGap,
                        remaining+other.station-carLength );
            }
            if( std::isfinite( followingGap ) )
                target = std::min( target, std::sqrt( 2.f*settings.maxBraking*
                    std::max( 0.f, followingGap-settings.minimumGap ) ) );

            bool blocked = agent.next == invalid;
            if( agent.next != invalid )
            {
                const u32 conflict = paths[agent.next].conflict;
                blocked = conflict != invalid &&
                    ( conflict >= conflictOwners.size() ||
                      conflictOwners[conflict] != s32( agent.id ) );
            }
            if( blocked ) target = std::min( target,
                std::sqrt( 2.f*settings.maxBraking*std::max( 0.f, remaining-2.f ) ) );
            agent.yielding = blocked || target + .05f < std::min( settings.cruiseSpeed, path.speedLimit );
            const f32 delta = std::clamp( target-agent.speed,
                -settings.maxBraking*tickSeconds, settings.maxAcceleration*tickSeconds );
            agent.speed = std::max( 0.f, agent.speed+delta );
            f32 advance = agent.speed*tickSeconds;
            if( std::isfinite( followingGap ) )
                advance = std::min( advance, std::max( 0.f, followingGap-settings.minimumGap ) );
            if( blocked ) advance = std::min( advance, std::max( 0.f, remaining-2.f ) );

            u32 destination = agent.path;
            f32 destinationStation = agent.station + advance;
            bool transferring = false;
            if( destinationStation >= path.length-.00001f && agent.next != invalid && !blocked )
            {
                destination = agent.next;
                destinationStation = std::min( paths[destination].length,
                    std::max( 0.f, destinationStation-path.length ) );
                transferring = true;
            }
            else destinationStation = std::min( destinationStation, path.length );
            const auto candidate = positionAt( paths[destination], destinationStation );
            // A final geometric guard covers adjacent directed lanes and
            // connector exits, which longitudinal path headway cannot compare.
            bool overlap = false;
            for( size_t j = 0; j < agents.size(); ++j ) if( i != j )
            {
                const auto other = positionAt( paths[agents[j].path], agents[j].station );
                if( horizontalSquared( candidate, other ) < 2.1f*2.1f )
                { overlap = true; break; }
            }
            if( overlap )
            {
                agent.speed = 0; agent.yielding = true;
                advance = 0; transferring = false;
            }
            else
            {
                agent.path = destination;
                agent.station = destinationStation;
                result.travelledMetres += advance;
                if( transferring )
                {
                    ++agent.transfers;
                    agent.next = selected( agent );
                    if( paths[destination].kind == PathKind::Junction ) ++result.intersectionTransfers;
                    if( paths[destination].kind == PathKind::Roundabout ) ++result.roundaboutTransfers;
                }
            }

            if( agent.next == invalid && paths[agent.path].kind == PathKind::Road &&
                paths[agent.path].length-agent.station <= 2.1f )
                ++agent.waitTicks;
            else agent.waitTicks = 0;
            if( agent.waitTicks >= 90 && !spawnPaths.empty() )
            {
                // A finite cul-de-sac emits the car back into the bounded
                // network rather than reversing through a kerb or freezing it.
                const u64 key = ( u64( seed ) << 32 ) ^ ( u64( agent.id ) << 12 ) ^ agent.transfers;
                for( size_t attempt = 0; attempt < spawnPaths.size(); ++attempt )
                {
                    const u32 candidatePath = spawnPaths[
                        size_t( ( hash( key ) + attempt ) % spawnPaths.size() )];
                    const auto point = positionAt( paths[candidatePath], 4.f );
                    if( obstacleEnabled && horizontal( point, obstacle ) <
                        obstacleRadius + settings.minimumGap + carLength ) continue;
                    bool clear = true;
                    for( size_t j = 0; j < agents.size(); ++j ) if( i != j &&
                        horizontal( point, positionAt( paths[agents[j].path], agents[j].station ) ) <
                        settings.minimumGap + carLength ) { clear = false; break; }
                    if( !clear ) continue;
                    if( agent.heldConflict != invalid && agent.heldConflict < conflictOwners.size() &&
                        conflictOwners[agent.heldConflict] == s32( agent.id ) )
                        conflictOwners[agent.heldConflict] = -1;
                    agent.path = candidatePath; agent.station = 4.f; agent.speed = 0;
                    agent.next = selected( agent ); agent.heldConflict = invalid;
                    agent.waitTicks = 0; ++agent.transfers;
                    break;
                }
            }
        }
        updatePoses();
    }

    void CityTrafficSimulation::step( f32 fixedDt )
    {
        if( !m_impl->result.valid || !std::isfinite( fixedDt ) || fixedDt <= 0 ) return;
        m_impl->remainder = std::min( .25f, m_impl->remainder + fixedDt );
        for( u32 count = 0; count < 8 && m_impl->remainder + 1e-6f >= tickSeconds; ++count )
        {
            m_impl->tick();
            m_impl->remainder = std::max( 0.f, m_impl->remainder-tickSeconds );
        }
    }
}  // namespace workphone::scene
