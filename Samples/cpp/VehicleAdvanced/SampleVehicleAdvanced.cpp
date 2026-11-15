#include "SampleVehicleAdvanced.h"
#include "ProceduralScene.h"
#include "FrameCapture.h"
#include "ProductionSceneProfile.h"
#include "TerrainGameplayPackage.h"
#include <WPProcedural/WPProcedural.hpp>
#include <Workphone/Workphone.hpp>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Physics/RaycastHit.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Graphics/FoliageData.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/CityRoadJunctions.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainFit.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>
#include <Workphone/Scene/Components/CityBuildingStyles.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/Terrain/CollisionTerrain.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Scene/Components/CollisionMesh.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ISky.hpp>

#if WP_GRAPHICS_SYSTEM_CLAW
#    include <WPGraphics/ClawMaterial.hpp>
#    include <WPGraphics/ClawMesh.hpp>
#    include <WPGraphics/ClawRendererDX11.hpp>
#    include <workphone_graphics_material.h>
#    include <workphone_graphics_renderer.h>
#    include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#    include <WPGraphics/ClawSceneNode.hpp>
#    include <WPGraphics/ClawScene.hpp>
#    include <WPGraphics/ClawTerrain.hpp>
#    include <WPGraphics/ClawUiSettings.hpp>
#    include <workphone_graphics_object.h>
#    include "MaterialSceneCpuWorkload.h"
#    include "MaterialSceneCpuEdits.h"
#    if defined( _WIN32 )
#        include <d3d11.h>
#    endif
#endif

#ifdef _WP_STATIC_LIB_
#    include <FBOISInput/FBOISInput.hpp>
#    include <WPSQLite/WPSQLite.hpp>
#    include <WPVehiclePhysics/WPVehiclePhysics.hpp>

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.hpp>
#    endif

#    if WP_BUILD_PHYSX
#        include <FBPhysx/FBPhysx.hpp>
#    elif WP_BUILD_ODE
#        include <FBODE3/CPhysicsManagerODE.hpp>
#    endif
#endif

namespace workphone
{
    namespace
    {
        constexpr auto physicsTargetFps = 120.0;
        // Run the sample tasks on one executor so controls, reset and camera poses stay coherent.
        constexpr auto activeThreadCount = 0u;
        constexpr hash_type debugTextId = 0x56454800;
        constexpr real_Num vehicleSpawnHeight = static_cast<real_Num>( 0.42 );
        constexpr real_Num cameraDistance = static_cast<real_Num>( 8.0 );
        constexpr real_Num cameraHeight = static_cast<real_Num>( 3.0 );
        constexpr std::array<std::string_view, 5> cityBuildingMeshRoles = {
            "wall", "window", "entrance", "corner", "roof"
        };
        constexpr std::array<const char *, 5> cityBuildingMeshProperties = {
            "City Wall Mesh", "City Window Mesh", "City Entrance Mesh", "City Corner Mesh", "City Roof Mesh"
        };
        constexpr auto &cityBuildingStyles = scene::race::cityBuildingStyleKeys;

        bool isRoadTerrainEditView( std::string_view view )
        {
            return view == "road-edit-before" || view == "road-edit-after";
        }

        void writeSharedMemoryLedger( std::ostream &output )
        {
            const auto stats = WorldStreamingScheduler::shared()->stats();
            const char *names[] = { "sourceCpu", "workingCpu", "gpu" };
            const StreamMemoryClass classes[] = {
                StreamMemoryClass::SourceCpu, StreamMemoryClass::WorkingCpu, StreamMemoryClass::Gpu
            };
            output << '{';
            for( size_t i = 0; i < 3; ++i )
            {
                const auto index = size_t( classes[i] );
                if( i ) output << ',';
                output << '"' << names[i] << "\":{\"used\":" << stats.used[index]
                       << ",\"peak\":" << stats.peakUsed[index] << ",\"limit\":" << stats.limits[index]
                       << ",\"rejectedReservations\":" << stats.rejectedReservations[index] << '}';
            }
            output << '}';
        }

        float chordDeviation( Vector3F point, Vector3F a, Vector3F b )
        {
            const float dx = b.x - a.x, dz = b.z - a.z;
            const float length = std::hypot( dx, dz );
            return length > .00001f ? std::abs( dx * ( point.z - a.z ) - dz * ( point.x - a.x ) ) / length : 0.f;
        }

        bool writeCityRoadGeometry( std::ostream &output, const scene::CityRoadAuthoring &authoring )
        {
            const auto &source = authoring.getSource();
            const auto &bake = authoring.getBake();
            size_t knots = 0, curvedSpans = 0, curvedRoads = 0;
            float maximumDeviation = 0, maximumGrade = 0, maximumEdgeGrade = 0, maximumCrossfall = 0, maximumArterialDetour = 1;
            float minHeight = std::numeric_limits<float>::max(), maxHeight = -minHeight;
            bool valid = !source.roads.empty() && source.roads.size() == bake.roads.size();
            for( size_t r = 0; r < source.roads.size(); ++r )
            {
                const auto &road = source.roads[r];
                knots += road.knots.size();
                bool curved = false;
                for( size_t k = 1; k < road.knots.size(); ++k )
                {
                    const auto &a = road.knots[k - 1], &b = road.knots[k];
                    if( std::max( chordDeviation( a.position + a.outHandle, a.position, b.position ),
                                  chordDeviation( b.position + b.inHandle, a.position, b.position ) ) > .05f )
                    { ++curvedSpans; curved = true; }
                }
                curvedRoads += curved;
                if( r >= bake.roads.size() || bake.roads[r].points.size() < 2 ) { valid = false; continue; }
                const auto &samples = bake.roads[r].points;
                valid = valid && bake.roads[r].roadId == road.id;
                const auto first = samples.front().position, last = samples.back().position;
                float horizontalLength = 0;
                for( size_t i = 0; i < samples.size(); ++i )
                {
                    const auto p = samples[i].position;
                    valid = valid && p.isFinite();
                    minHeight = std::min( minHeight, p.y ); maxHeight = std::max( maxHeight, p.y );
                    maximumDeviation = std::max( maximumDeviation, chordDeviation( p, first, last ) );
                    for( const auto edge : { samples[i].left, samples[i].right } )
                    {
                        const float run = std::hypot( edge.x - p.x, edge.z - p.z );
                        if( run > .00001f ) maximumCrossfall = std::max( maximumCrossfall, std::abs( edge.y - p.y ) / run );
                    }
                    if( i )
                    {
                        const auto previous = samples[i - 1].position;
                        const float run = std::hypot( p.x - previous.x, p.z - previous.z );
                        horizontalLength += run;
                        if( run > .00001f ) maximumGrade = std::max( maximumGrade, std::abs( p.y - previous.y ) / run );
                        for( const auto edge : { std::pair{ samples[i - 1].left, samples[i].left },
                                                std::pair{ samples[i - 1].right, samples[i].right } } )
                        {
                            const float edgeRun = std::hypot( edge.second.x - edge.first.x, edge.second.z - edge.first.z );
                            if( edgeRun > .00001f ) maximumEdgeGrade = std::max( maximumEdgeGrade, std::abs( edge.second.y - edge.first.y ) / edgeRun );
                        }
                    }
                }
                const float chord = std::hypot( last.x - first.x, last.z - first.z );
                if( road.width >= 11.99f && chord > .00001f )
                    maximumArterialDetour = std::max( maximumArterialDetour, horizontalLength / chord );
            }
            const auto json = authoring.getSourceJSON();
            valid = valid && !json.empty() && minHeight <= maxHeight;
            output << ",\n  \"roadGeometry\": {\"editableKnots\":" << knots << ",\"curvedRoads\":" << curvedRoads
                   << ",\"curvedSpans\":" << curvedSpans << ",\"maximumCentrelineDeviationMetres\":" << maximumDeviation
                   << ",\"maximumCentrelineGrade\":" << maximumGrade << ",\"maximumArterialLengthRatio\":" << maximumArterialDetour
                   << ",\"maximumEdgeGrade\":" << maximumEdgeGrade << ",\"maximumHalfCrossfall\":" << maximumCrossfall
                   << ",\"heightRangeMetres\":" << ( minHeight <= maxHeight ? maxHeight - minHeight : 0 )
                   << ",\"valid\":" << ( valid ? "true" : "false" ) << '}'
                   << ",\n  \"editableRoadSource\": " << ( json.empty() ? String( "null" ) : json );
            return valid;
        }

        scene::CityRoadJunctionBake cityConnections( const scene::CityRoadAuthoring &authoring )
        {
            scene::CityRoadJunctionBake connections;
            const auto terrain = authoring.getTerrainSnapshot();
            const auto placement = authoring.getTerrainPlacement();
            String error;
            if( !terrain || !scene::buildCityRoadJunctions( authoring.getSource(), authoring.getBake(),
                [terrain, placement]( const Vector3F &p, float &height ) {
                    return render::sampleTerrainHeight( *terrain, placement, p, height );
                }, {}, connections, error ) )
                throw std::runtime_error( ( "Cannot inspect city connections: " + error ).c_str() );
            return connections;
        }

        bool writeCityConnections( std::ostream &output, const scene::CityRoadAuthoring &authoring,
                                   SmartPtr<physics::IPhysicsScene3> physics )
        {
            const auto connections = cityConnections( authoring );
            size_t ports = 0, rounded = 0, boundaryVertices = 0, probes = 0, passed = 0;
            const auto probe = [&]( Vector3F p ) {
                ++probes;
                f32 height = 0; bool driveable = false;
                Vector3F hit, normal; SmartPtr<ISharedObject> object;
                if( authoring.getRuntimeSurface( p.x, p.z, height, driveable ) && driveable && physics &&
                    physics->intersects( { p.x, height + .25f, p.z }, { p.x, height - .25f, p.z }, hit, normal, object ) &&
                    object && std::isfinite( hit.y ) && std::abs( hit.y - height ) < .01f && normal.y > .5f ) ++passed;
            };
            output << ",\n  \"roadConnections\": {\"junctions\":" << connections.junctions.size()
                   << ",\"retainedRuns\":" << connections.roadRuns.size() << ",\"patches\":[";
            for( size_t j = 0; j < connections.junctions.size(); ++j )
            {
                const auto &connection = connections.junctions[j];
                ports += connection.ports.size(); boundaryVertices += connection.boundary.size();
                probe( connection.center );
                if( j ) output << ',';
                output << "{\"center\":[" << connection.center.x << ',' << connection.center.y << ',' << connection.center.z
                       << "],\"ports\":" << connection.ports.size() << ",\"boundary\":[";
                for( size_t i = 0; i < connection.boundary.size(); ++i )
                {
                    if( i ) output << ',';
                    const auto p = connection.boundary[i]; output << '[' << p.x << ',' << p.y << ',' << p.z << ']';
                }
                output << "]}";
                for( const auto &corner : connection.corners ) if( corner.points.size() > 2 )
                {
                    ++rounded;
                    const size_t middle = corner.points.size() / 2;
                    probe( corner.points[middle] - corner.outward[middle] * .4f );
                }
            }
            const bool valid = probes == passed;
            output << "],\"ports\":" << ports << ",\"roundedCorners\":" << rounded
                   << ",\"boundaryVertices\":" << boundaryVertices << ",\"nativeCollisionProbes\":" << probes
                   << ",\"nativeCollisionProbesPassed\":" << passed << ",\"valid\":" << ( valid ? "true" : "false" ) << '}';
            return valid;
        }

        bool writeCityTerrainInteraction( std::ostream &output, const scene::CityRoadAuthoring &authoring,
                                          SmartPtr<physics::IPhysicsScene3> physics )
        {
            const auto terrain = authoring.getTerrainSnapshot();
            const auto &placement = authoring.getTerrainPlacement();
            const auto &source = authoring.getSource();
            const auto &bake = authoring.getBake();
            if( !terrain || source.roads.size() != bake.roads.size() )
            { output << ",\n  \"terrainRoadInteraction\":{\"valid\":false}"; return false; }
            float minimum = std::numeric_limits<float>::max(), maximum = -minimum;
            bool finite = !terrain->heights.empty();
            const auto pose = placement.getTransformationMatrix();
            for( const auto height : terrain->heights )
            {
                const float y = pose.transformAffine( Vector3F( 0, height * terrain->heightScale, 0 ) ).y;
                finite = finite && std::isfinite( y );
                minimum = std::min( minimum, y ); maximum = std::max( maximum, y );
            }
            struct Probes
            {
                size_t attempted = 0, observed = 0, supported = 0, collision = 0;
                float maximumClearance = 0, maximumSupportError = 0, maximumCollisionError = 0;
                float minimumSignedClearance = std::numeric_limits<float>::max(), maximumSignedClearance = -std::numeric_limits<float>::max();
            } roads, rings;
            struct FailedProbe
            {
                int road = -1, station = -1;
                Vector3F point, hit, normal;
                float surface = 0, ground = 0;
                bool hitFound = false;
                std::array<float, 4> neighbourErrors{ -1, -1, -1, -1 };
            };
            std::array<FailedProbe, 8> failures;
            size_t failureCount = 0;
            int probeRoad = -1, probeStation = -1;
            const auto probe = [&]( Vector3F p, float offset, float tolerance, Probes &checks ) {
                ++checks.attempted;
                float ground = 0, surface = 0; bool driveable = false;
                if( !render::sampleTerrainHeight( *terrain, placement, p, ground ) ||
                    !authoring.getRuntimeSurface( p.x, p.z, surface, driveable ) || !driveable ||
                    !std::isfinite( ground ) || !std::isfinite( surface ) ) return;
                const float clearance = surface - ground, supportError = std::abs( clearance - offset );
                ++checks.observed;
                checks.minimumSignedClearance = std::min( checks.minimumSignedClearance, clearance );
                checks.maximumSignedClearance = std::max( checks.maximumSignedClearance, clearance );
                checks.maximumClearance = std::max( checks.maximumClearance, std::abs( clearance ) );
                checks.maximumSupportError = std::max( checks.maximumSupportError, supportError );
                checks.supported += supportError <= tolerance;
                Vector3F hit, normal; SmartPtr<ISharedObject> object;
                const bool hitFound = physics && physics->intersects( { p.x, surface + .25f, p.z },
                    { p.x, surface - .25f, p.z }, hit, normal, object ) && object && std::isfinite( hit.y );
                bool collisionPassed = false;
                if( hitFound )
                {
                    const float error = std::abs( hit.y - surface );
                    checks.maximumCollisionError = std::max( checks.maximumCollisionError, error );
                    collisionPassed = error < .01f && normal.y > .5f;
                    checks.collision += collisionPassed;
                }
                if( !collisionPassed && failureCount < failures.size() )
                {
                    auto &failure = failures[failureCount++];
                    failure.road = probeRoad; failure.station = probeStation; failure.point = p;
                    failure.surface = surface; failure.ground = ground; failure.hitFound = hitFound;
                    if( hitFound ) { failure.hit = hit; failure.normal = normal; }
                    // Diagnostic rays never replace or excuse the original failure.
                    // They distinguish an exact seam miss from a missing collider.
                    const Vector3F offsets[] = { { .002f, 0, 0 }, { -.002f, 0, 0 },
                                                { 0, 0, .002f }, { 0, 0, -.002f } };
                    for( size_t i = 0; i < 4; ++i )
                    {
                        const auto q = p + offsets[i]; float nearby = 0; bool road = false;
                        if( physics && authoring.getRuntimeSurface( q.x, q.z, nearby, road ) && road &&
                            physics->intersects( { q.x, nearby + .25f, q.z }, { q.x, nearby - .25f, q.z }, hit, normal, object ) && object )
                            failure.neighbourErrors[i] = std::abs( hit.y - nearby );
                    }
                }
            };
            // At most 96 distributed carriageway probes, concentrated on each
            // selected road's steepest sampled segment and both lane interiors.
            const size_t roadCount = std::min<size_t>( 32, bake.roads.size() );
            for( size_t selected = 0; selected < roadCount; ++selected )
            {
                const size_t roadIndex = selected * bake.roads.size() / roadCount;
                const auto &points = bake.roads[roadIndex].points;
                if( points.size() < 2 ) { finite = false; continue; }
                size_t station = points.size() / 2; float strongest = 0;
                for( size_t i = 1; i < points.size(); ++i )
                {
                    const auto a = points[i - 1].position, b = points[i].position;
                    const float run = std::hypot( b.x - a.x, b.z - a.z );
                    const float grade = run > .00001f ? std::abs( b.y - a.y ) / run : 0;
                    if( grade > strongest ) { strongest = grade; station = i; }
                }
                const auto &p = points[station]; const auto &road = source.roads[roadIndex];
                probeRoad = int( roadIndex ); probeStation = int( station );
                const bool drape = road.heightMode == procedural::CityRoadHeightMode::Drape;
                for( const auto point : { p.position, p.position + ( p.left - p.position ) * .75f,
                                        p.position + ( p.right - p.position ) * .75f } )
                    probe( point, drape ? road.heightOffset : 0, drape ? .001f : .08f, roads );
            }
            size_t capSamples = 0, capSupported = 0; float maximumCapPlaneError = 0, maximumCapTerrainError = 0;
            probeRoad = probeStation = -1;
            for( const auto &junction : source.roundabouts )
            {
                float chord = junction.outerRadius;
                for( const auto &port : junction.ports )
                    chord = std::min( chord, std::sqrt( junction.outerRadius * junction.outerRadius - port.width * port.width * .25f ) );
                const float radius = ( junction.islandRadius + chord ) * .5f;
                for( size_t i = 0; i < 8; ++i )
                {
                    const float angle = float( i ) * 6.283185307179586f / 8;
                    probe( junction.center + Vector3F( std::cos( angle ) * radius, 0, std::sin( angle ) * radius ),
                           0, std::min( junction.maxTerrainDeviation, .08f ) + .00001f, rings );
                }
                for( const auto &port : junction.ports )
                {
                    auto direction = port.outward; direction.y = 0; direction.normalise();
                    const auto center = junction.center + direction * ( junction.outerRadius + port.length );
                    const Vector3F side( -direction.z, 0, direction.x );
                    const auto road = std::find_if( source.roads.begin(), source.roads.end(), [&]( const auto &r ) { return r.id == port.approachId; } );
                    for( const float across : { -.25f, 0.f, .25f } )
                    {
                        ++capSamples; const auto p = center + side * ( across * port.width );
                        const float plane = junction.center.y + junction.gradeX * ( p.x - junction.center.x ) + junction.gradeZ * ( p.z - junction.center.z );
                        float ground = 0, surface = 0; bool driveable = false;
                        if( road == source.roads.end() || !render::sampleTerrainHeight( *terrain, placement, p, ground ) ||
                            !authoring.getRuntimeSurface( p.x, p.z, surface, driveable ) || !driveable ) continue;
                        const bool drape = road->heightMode == procedural::CityRoadHeightMode::Drape;
                        const float error = std::abs( surface - plane );
                        const float groundError = std::abs( ground + ( drape ? road->heightOffset : 0 ) - plane );
                        maximumCapPlaneError = std::max( maximumCapPlaneError, error );
                        maximumCapTerrainError = std::max( maximumCapTerrainError, groundError );
                        capSupported += std::isfinite( error ) && std::isfinite( groundError ) && error <= .001f &&
                            groundError <= ( drape ? .001f : .08f );
                    }
                }
            }
            const bool valid = finite && roads.attempted > 0 && roads.supported == roads.attempted &&
                roads.collision == roads.attempted && rings.supported == rings.attempted &&
                rings.collision == rings.attempted && capSupported == capSamples;
            const auto write = [&]( const char *name, const Probes &p ) {
                output << ",\"" << name << "\":{\"samples\":" << p.attempted << ",\"supported\":" << p.supported
                       << ",\"observedSurfaces\":" << p.observed
                       << ",\"nativeCollisionPassed\":" << p.collision << ",\"maximumAbsoluteClearanceMetres\":" << p.maximumClearance
                       << ",\"minimumSignedClearanceMetres\":" << ( p.observed ? p.minimumSignedClearance : 0 )
                       << ",\"maximumSignedClearanceMetres\":" << ( p.observed ? p.maximumSignedClearance : 0 )
                       << ",\"maximumSupportErrorMetres\":" << p.maximumSupportError
                       << ",\"maximumNativeHeightErrorMetres\":" << p.maximumCollisionError << '}';
            };
            output << ",\n  \"terrainRoadInteraction\":{\"heightfieldSamples\":" << terrain->heights.size()
                   << ",\"minimumRetainedTerrainHeightMetres\":" << ( finite ? minimum : 0 )
                   << ",\"maximumRetainedTerrainHeightMetres\":" << ( finite ? maximum : 0 )
                   << ",\"retainedTerrainReliefMetres\":" << ( finite ? maximum - minimum : 0 );
            write( "carriageway", roads ); write( "roundabouts", rings );
            output << ",\"failedNativeProbes\":[";
            const auto vector = [&]( Vector3F p ) { output << '[' << p.x << ',' << p.y << ',' << p.z << ']'; };
            for( size_t i = 0; i < failureCount; ++i )
            {
                const auto &failure = failures[i]; if( i ) output << ',';
                output << "{\"roadIndex\":" << failure.road << ",\"station\":" << failure.station << ",\"point\":";
                vector( failure.point );
                output << ",\"surface\":" << failure.surface << ",\"ground\":" << failure.ground
                       << ",\"hitFound\":" << ( failure.hitFound ? "true" : "false" ) << ",\"hit\":";
                vector( failure.hit ); output << ",\"normal\":"; vector( failure.normal );
                output << ",\"nearbyNativeHeightErrorsMetres\":[";
                for( size_t j = 0; j < 4; ++j ) { if( j ) output << ','; output << failure.neighbourErrors[j]; }
                output << "]}";
            }
            output << "],\"roundaboutCapSamples\":" << capSamples << ",\"roundaboutCapsSupported\":" << capSupported
                   << ",\"maximumRoundaboutCapPlaneErrorMetres\":" << maximumCapPlaneError
                   << ",\"maximumRoundaboutCapTerrainErrorMetres\":" << maximumCapTerrainError
                   << ",\"valid\":" << ( valid ? "true" : "false" ) << '}';
            return valid;
        }

        std::array<Vector3F, 2> cityRoadInspectionView( const advanced::SceneAssets &assets )
        {
            const auto &samples = assets.circuit.samples;
            const size_t corner = samples.size() / 4;
            const auto &behind = samples[( corner + samples.size() - std::min<size_t>( 12, corner ) ) % samples.size()];
            auto target = samples[( corner + std::min<size_t>( 20, samples.size() / 8 ) ) % samples.size()].position + Vector3F( 0, .4f, 0 );
            auto eye = behind.position + behind.right * 3.f;
            // Inspect an actual bend when the editable source has one. A fixed
            // race-route fraction can select a deliberately straight T-junction.
            float strongest = .05f;
            if( assets.roadAuthoring ) for( const auto &road : assets.roadAuthoring->getBake().roads )
            {
                if( road.points.size() < 3 ) continue;
                for( size_t i = 1; i + 1 < road.points.size(); ++i )
                {
                    const float bend = chordDeviation( road.points[i].position,
                        road.points.front().position, road.points.back().position );
                    if( bend <= strongest ) continue;
                    strongest = bend;
                    const auto &start = road.points[i > 8 ? i - 8 : 0];
                    auto side = start.right - start.position; side.y = 0;
                    if( side.lengthSquared() > .00001f ) side.normalise();
                    eye = start.position + side * 3.f;
                    target = road.points[std::min( i + 16, road.points.size() - 1 )].position + Vector3F( 0, .4f, 0 );
                }
            }
            const float roadHeight = eye.y;
            // Keep the inspection camera beside the road and lift it only when
            // the sight line intersects a building. The old 22 m side offset
            // placed the camera inside dense city frontages.
            for( const float height : { 16.f, 24.f, 36.f, 52.f, 72.f } )
            {
                eye.y = roadHeight + height;
                bool blocked = false;
                for( const auto &lot : assets.cityLayout.lots )
                {
                    if( lot.park ) continue;
                    float first = 0, last = 1;
                    const auto axis = [&]( float a, float b, float minimum, float maximum ) {
                        const float delta = b - a;
                        if( std::abs( delta ) < .00001f ) return a >= minimum && a <= maximum;
                        float low = ( minimum - a ) / delta, high = ( maximum - a ) / delta;
                        if( low > high ) std::swap( low, high );
                        first = std::max( first, low ); last = std::min( last, high );
                        return first <= last;
                    };
                    if( axis( eye.x, target.x, lot.x - lot.width * .5f - 1, lot.x + lot.width * .5f + 1 ) &&
                        axis( eye.z, target.z, lot.z - lot.depth * .5f - 1, lot.z + lot.depth * .5f + 1 ) &&
                        axis( eye.y, target.y, lot.groundMinimumHeight - 1, lot.groundHeight + lot.height + 6 ) )
                    { blocked = true; break; }
                }
                if( !blocked ) break;
            }
            return { eye, target };
        }

        struct CityHillsideView
        {
            Vector3F eye{}, target{};
            String roadId;
            float rise = 0;
        };

        struct CityVergeView
        {
            Vector3F eye{}, target{};
            size_t road = 0;
        };

        struct CityParkView
        {
            Vector3F eye{}, target{};
            const procedural::OpenCityLayout::Lot *lot = nullptr;
        };

        CityParkView cityParkInspectionView( const advanced::SceneAssets &assets )
        {
            const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
            if( !terrain ) throw std::runtime_error( "Park view requires fitted terrain roads." );
            const auto &placement = assets.roadAuthoring->getTerrainPlacement();
            CityParkView selected;
            float best = std::numeric_limits<float>::infinity();
            using Frontage = procedural::OpenCityLayout::Frontage;
            for( const auto &lot : assets.cityLayout.lots )
            {
                if( !lot.park || std::min( lot.parcelWidth, lot.parcelDepth ) < 10.f ) continue;
                const float score = std::hypot( lot.parcelX, lot.parcelZ ) -
                                    .35f * std::min( lot.parcelWidth, lot.parcelDepth );
                if( score >= best ) continue;
                Vector3F outward;
                switch( lot.frontage )
                {
                case Frontage::West: outward = { -1, 0, 0 }; break;
                case Frontage::East: outward = { 1, 0, 0 }; break;
                case Frontage::North: outward = { 0, 0, -1 }; break;
                case Frontage::South: outward = { 0, 0, 1 }; break;
                }
                const bool side = outward.x != 0;
                const float depth = side ? lot.parcelWidth : lot.parcelDepth;
                const float span = side ? lot.parcelDepth : lot.parcelWidth;
                const Vector3F along( -outward.z, 0, outward.x );
                const Vector3F centre( lot.parcelX, 0, lot.parcelZ );
                auto eye = centre + outward * ( depth * .48f ) + along * ( span * .28f );
                auto target = centre - along * ( span * .06f );
                if( !render::sampleTerrainHeight( *terrain, placement, eye, eye.y ) ||
                    !render::sampleTerrainHeight( *terrain, placement, target, target.y ) ) continue;
                // Keep the old platform and the terrain-fitted parcel in the same framing.
                eye.y = std::max( eye.y, lot.groundHeight ) + 2.30f;
                target.y += 1.30f;
                best = score;
                selected = { eye, target, &lot };
            }
            if( !selected.lot ) throw std::runtime_error( "Park view requires a supported park parcel." );
            return selected;
        }

        constexpr const char *utilityCoverActor = "Terrain city carriageway cast iron covers";
        constexpr const char *utilityGrateActor = "Terrain city cast iron storm grates";

        bool utilityNativeReady( const advanced::SceneAssets &assets, const char *name )
        {
            for( const auto &actor : assets.actors )
            {
                if( !actor || actor->getName() != name ) continue;
                const auto renderer = actor->getComponent<scene::MeshRenderer>();
                const auto graphics = renderer ? renderer->getGraphicsObject() : nullptr;
                if( !renderer || !renderer->isLoaded() || !graphics || !graphics->isLoaded() ) continue;
#if WP_GRAPHICS_SYSTEM_CLAW
                const auto mesh = dynamic_pointer_cast<render::ClawMesh>( graphics );
                const auto native = mesh ? mesh->getNativeMesh() : nullptr;
                const auto object = mesh ? mesh->getNativeRenderObject() : nullptr;
                if( !native || !object || wp_graphics_object_get_mesh( object ) != native ||
                    wp_graphics_mesh_get_vertex_count( native ) == 0 || wp_graphics_mesh_get_index_count( native ) < 3 ) continue;
#endif
                return true;
            }
            return false;
        }

        SourceBudgetVector<Vector3F> utilityCentres( const advanced::SceneAssets &assets, const char *name )
        {
            struct Bounds { Vector3F low, high; };
            SourceBudgetVector<Bounds> pieces;
            for( const auto &actor : assets.actors )
            {
                if( !actor || actor->getName() != name ) continue;
                const auto component = actor->getComponent<scene::Mesh>();
                const auto resource = component ? component->getMeshResource() : nullptr;
                if( !resource || !resource->getMesh() ) continue;
                const auto transform = actor->getWorldTransform();
                for( const auto &point : MeshUtil::getPoints( resource->getMesh() ) )
                {
                    const auto p = transform.transformPoint( point );
                    if( !p.isFinite() ) continue;
                    // Every cover or grate fits in its own metre-scale envelope.
                    // Merge terrain-clipped fragments without depending on vertex order.
                    const auto selected = std::find_if( pieces.begin(), pieces.end(), [&]( const Bounds &piece ) {
                        return std::max( piece.high.x, p.x ) - std::min( piece.low.x, p.x ) < 1.10f &&
                               std::max( piece.high.z, p.z ) - std::min( piece.low.z, p.z ) < 1.10f;
                    } );
                    if( selected == pieces.end() )
                    {
                        pieces.push_back( { p, p } );
                        continue;
                    }
                    selected->low = { std::min( selected->low.x, p.x ), std::min( selected->low.y, p.y ),
                                      std::min( selected->low.z, p.z ) };
                    selected->high = { std::max( selected->high.x, p.x ), std::max( selected->high.y, p.y ),
                                       std::max( selected->high.z, p.z ) };
                }
            }
            SourceBudgetVector<Vector3F> centres;
            for( const auto &piece : pieces )
                if( std::hypot( piece.high.x - piece.low.x, piece.high.z - piece.low.z ) > .12f )
                    centres.push_back( ( piece.low + piece.high ) * .5f );
            return centres;
        }

        struct CityUtilityView
        {
            Vector3F eye{}, target{}, primary{}, grate{};
            size_t covers = 0, grates = 0;
            bool nearby = false;
        };

        CityUtilityView cityUtilityInspectionView( const advanced::SceneAssets &assets )
        {
            const auto authoring = assets.roadAuthoring;
            const auto terrain = authoring ? authoring->getTerrainSnapshot() : nullptr;
            if( !terrain ) throw std::runtime_error( "Utilities view requires fitted terrain roads." );
            const auto covers = utilityCentres( assets, utilityCoverActor );
            const auto grates = utilityCentres( assets, utilityGrateActor );
            if( covers.empty() ) throw std::runtime_error( "Utilities view requires a generated service cover." );
            const auto &placement = authoring->getTerrainPlacement();
            const auto heightAt = [&]( Vector3F p, float &height ) {
                bool driveable = false;
                if( authoring->getRuntimeSurface( p.x, p.z, height, driveable ) ) return true;
                return render::sampleTerrainHeight( *terrain, placement, p, height );
            };
            const auto clear = [&]( Vector3F eye, Vector3F point ) {
                for( unsigned i = 0; i < 9; ++i )
                {
                    const auto p = eye + ( point - eye ) * ( float( i ) / 9 );
                    float y = 0;
                    if( !render::sampleTerrainHeight( *terrain, placement, p, y ) || p.y < y + .04f ) return false;
                    for( const auto &lot : assets.cityLayout.lots )
                        if( !lot.park && std::abs( p.x - lot.x ) < lot.width * .5f + .15f &&
                            std::abs( p.z - lot.z ) < lot.depth * .5f + .15f &&
                            p.y < lot.groundHeight + lot.height + .2f ) return false;
                }
                return true;
            };
            CityUtilityView best;
            float lowestScore = std::numeric_limits<float>::infinity();
            const auto select = [&]( bool allowGrates ) {
                for( const auto primary : covers )
                {
                    float bestDistance = 5.4f;
                    Vector3F nearby{};
                    bool paired = false;
                    for( const auto grate : grates )
                    {
                        if( !allowGrates ) break;
                        const float distance = std::hypot( grate.x - primary.x, grate.z - primary.z );
                        if( distance < 1.12f || distance >= bestDistance ) continue;
                        bestDistance = distance;
                        nearby = grate;
                        paired = true;
                    }
                    auto target = paired ? primary * .52f + nearby * .48f : primary;
                    target.y += .05f;
                    Vector3F heading( 1, 0, 0 );
                    if( paired ) heading = Vector3F( nearby.x - primary.x, 0, nearby.z - primary.z ).normaliseCopy();
                    else
                    {
                        float nearest = std::numeric_limits<float>::infinity();
                        for( const auto &road : authoring->getBake().roads )
                            for( size_t i = 1; i < road.points.size(); ++i )
                            {
                                const auto a = road.points[i - 1].position;
                                auto axis = road.points[i].position - a, offset = primary - a;
                                axis.y = offset.y = 0;
                                const float length = axis.lengthSquared();
                                if( length < .001f ) continue;
                                const float distance = ( offset - axis * std::clamp( offset.dotProduct( axis ) / length, 0.f, 1.f ) ).lengthSquared();
                                if( distance < nearest ) { nearest = distance; heading = axis.normaliseCopy(); }
                            }
                    }
                    const Vector3F side( -heading.z, 0, heading.x );
                    const float run = paired ? std::max( 3.4f, bestDistance * .99f + .42f ) : 2.55f;
                    for( const auto direction : { side, -side } )
                    {
                        auto eye = target + direction * run - heading * .31f;
                        float ground = 0;
                        if( !heightAt( eye, ground ) ) continue;
                        eye.y = std::max( target.y, ground ) + ( paired ? 1.81f + .12f * bestDistance : 1.77f );
                        const float score = ( paired ? bestDistance * .24f : 8.f ) +
                                            std::hypot( primary.x, primary.z ) * .0018f;
                        if( score >= lowestScore || !clear( eye, primary ) || ( paired && !clear( eye, nearby ) ) ) continue;
                        lowestScore = score;
                        best = { eye, target, primary, nearby, covers.size(), grates.size(), paired };
                    }
                }
            };
            select( true );
            if( !best.covers ) select( false );
            if( !best.covers ) throw std::runtime_error( "Utilities view could not find an unobstructed cover inspection pose." );
            return best;
        }

        CityVergeView cityVergeInspectionView( const advanced::SceneAssets &assets )
        {
            const auto authoring = assets.roadAuthoring;
            const auto terrain = authoring ? authoring->getTerrainSnapshot() : nullptr;
            if( !terrain ) throw std::runtime_error( "Verge view requires fitted terrain roads." );
            const auto &placement = authoring->getTerrainPlacement();
            const auto &source = authoring->getSource();
            const auto &bake = authoring->getBake().roads;
            if( source.roads.size() != bake.size() )
                throw std::runtime_error( "Verge view requires the current road bake." );
            CityVergeView selected;
            float best = -1;
            for( size_t r = 0; r < bake.size(); ++r )
            {
                const auto &road = source.roads[r];
                const auto &points = bake[r].points;
                if( road.closed || points.size() < 2 ) continue;
                for( const bool end : { false, true } )
                {
                    const auto &sample = end ? points.back() : points.front();
                    const auto p = sample.position;
                    const float score = std::hypot( p.x, p.z ) + road.width;
                    if( score <= best ) continue;
                    const bool connected = std::any_of( bake.begin(), bake.end(), [&]( const auto &other ) {
                        if( &other == &bake[r] ) return false;
                        return std::any_of( other.points.begin(), other.points.end(), [&]( const auto &q ) {
                            return std::hypot( p.x - q.position.x, p.z - q.position.z ) < 18.f;
                        } );
                    } );
                    if( connected ) continue;
                    auto intoRoad = end ? -sample.tangent : sample.tangent;
                    intoRoad.y = 0;
                    if( intoRoad.lengthSquared() < .0001f ) continue;
                    intoRoad.normalise();
                    const Vector3F side( -intoRoad.z, 0, intoRoad.x );
                    for( const float bank : { 1.f, -1.f } )
                    {
                        const auto offset = side * bank;
                        auto eye = p - intoRoad * 8.5f + offset * ( road.width * .5f + 5.5f );
                        auto target = p + intoRoad * 2.5f + offset * ( road.width * .5f + 1.7f );
                        if( !render::sampleTerrainHeight( *terrain, placement, eye, eye.y ) ||
                            !render::sampleTerrainHeight( *terrain, placement, target, target.y ) ) continue;
                        eye.y += 2.15f;
                        target.y += .28f;
                        bool blocked = false;
                        for( int i = 1; i < 12 && !blocked; ++i )
                        {
                            const auto point = eye + ( target - eye ) * ( float( i ) / 12 );
                            float height = 0;
                            blocked = !render::sampleTerrainHeight( *terrain, placement, point, height ) ||
                                      point.y < height + .1f;
                        }
                        if( !blocked ) { selected = { eye, target, r }; best = score; break; }
                    }
                }
            }
            if( best < 0 ) throw std::runtime_error( "Verge view needs an open road endpoint on supported terrain." );
            return selected;
        }

        CityHillsideView cityHillsideInspectionView( const advanced::SceneAssets &assets )
        {
            if( !assets.roadAuthoring ) throw std::runtime_error( "Hillside view requires editable terrain roads." );
            CityHillsideView view; float best = -1, width = 4;
            Vector3F low{}, high{};
            const auto &bake = assets.roadAuthoring->getBake();
            const auto &source = assets.roadAuthoring->getSource();
            for( size_t r = 0; r < bake.roads.size(); ++r )
            {
                const auto &road = bake.roads[r];
                for( size_t i = 0; i + 1 < road.points.size(); i += std::max<size_t>( 1, road.points.size() / 32 ) )
                {
                    auto end = std::lower_bound( road.points.begin() + i, road.points.end(), road.points[i].distance + 80.f,
                        []( const auto &point, float distance ) { return point.distance < distance; } );
                    if( end == road.points.end() ) --end;
                    const auto a = road.points[i].position, b = end->position;
                    const float run = std::hypot( b.x - a.x, b.z - a.z );
                    if( run < 25 ) continue;
                    const float rise = std::abs( b.y - a.y ), score = rise + run * .001f;
                    if( score <= best ) continue;
                    best = score; view.rise = rise;
                    low = a.y <= b.y ? a : b; high = a.y <= b.y ? b : a;
                    view.roadId = String( road.roadId.data(), road.roadId.size() );
                    if( r < source.roads.size() ) width = source.roads[r].width;
                }
            }
            if( best < 0 ) throw std::runtime_error( "Hillside view requires a road with at least 25 m of horizontal span." );
            auto along = high - low; along.y = 0; along.normalise();
            const Vector3F side( -along.z, 0, along.x );
            view.eye = low - along * 14.f + side * std::min( width * .25f, 2.f );
            view.target = high + Vector3F( 0, 1.8f, 0 );
            const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
            const auto &placement = assets.roadAuthoring->getTerrainPlacement();
            float base = low.y, height = 0;
            if( terrain && render::sampleTerrainHeight( *terrain, placement, view.eye, height ) ) base = std::max( base, height );
            for( const float lift : { 6.f, 10.f, 16.f, 26.f, 40.f, 60.f, 84.f } )
            {
                view.eye.y = base + lift; bool blocked = false;
                for( size_t i = 1; terrain && i < 16; ++i )
                {
                    const auto p = view.eye + ( view.target - view.eye ) * ( float( i ) / 16 );
                    if( render::sampleTerrainHeight( *terrain, placement, p, height ) && p.y <= height + .5f )
                    { blocked = true; break; }
                }
                for( const auto &lot : assets.cityLayout.lots )
                {
                    if( blocked || lot.park ) continue;
                    float first = 0, last = 1;
                    const auto axis = [&]( float a, float b, float minimum, float maximum ) {
                        const float delta = b - a;
                        if( std::abs( delta ) < .00001f ) return a >= minimum && a <= maximum;
                        float start = ( minimum - a ) / delta, finish = ( maximum - a ) / delta;
                        if( start > finish ) std::swap( start, finish );
                        first = std::max( first, start ); last = std::min( last, finish ); return first <= last;
                    };
                    blocked = axis( view.eye.x, view.target.x, lot.x - lot.width * .5f - 1, lot.x + lot.width * .5f + 1 ) &&
                        axis( view.eye.z, view.target.z, lot.z - lot.depth * .5f - 1, lot.z + lot.depth * .5f + 1 ) &&
                        axis( view.eye.y, view.target.y, lot.groundMinimumHeight - 1, lot.groundHeight + lot.height + 6 );
                }
                if( !blocked ) break;
            }
            return view;
        }

        struct CityBuildingGeometryCheck
        {
            u32 patches = 0, meshes = 0, readyMeshes = 0, nearMeshes = 0, farMeshes = 0;
            u32 masonryMeshes = 0, readyMasonryMeshes = 0;
            u64 vertices = 0, triangles = 0;
            bool valid = false;
        };

        CityBuildingGeometryCheck checkCityBuildingGeometry( const advanced::SceneAssets &assets,
                                                              bool expectNear )
        {
            CityBuildingGeometryCheck result;
            for( const auto &actor : assets.actors )
            {
                if( !actor ) continue;
                const auto name = actor->getName();
                if( name == "City LOD patch" ) ++result.patches;
                const bool nearMesh = name.find( "LOD0 city " ) == 0;
                const bool farMesh = name.find( "LOD1 city " ) == 0;
                if( !nearMesh && !farMesh ) continue;
                ++result.meshes;
                result.nearMeshes += nearMesh;
                result.farMeshes += farMesh;
                const auto renderer = actor->getComponent<scene::MeshRenderer>();
                const auto graphics = renderer ? renderer->getGraphicsObject() : nullptr;
                bool ready = renderer && renderer->isLoaded() && graphics && graphics->isLoaded();
#if WP_GRAPHICS_SYSTEM_CLAW
                const auto mesh = dynamic_pointer_cast<render::ClawMesh>( graphics );
                const auto native = mesh ? mesh->getNativeMesh() : nullptr;
                const auto object = mesh ? mesh->getNativeRenderObject() : nullptr;
                const auto node = renderer ? dynamic_pointer_cast<render::ClawSceneNode>( renderer->getGraphicsNode() ) : nullptr;
                const auto vertices = native ? wp_graphics_mesh_get_vertex_count( native ) : 0;
                const auto indices = native ? wp_graphics_mesh_get_index_count( native ) : 0;
                // ClawMesh reaches Loaded only after native GPU admission succeeds.
                ready = ready && native && vertices > 0 && indices >= 3 && indices % 3 == 0 &&
                    object && wp_graphics_object_get_mesh( object ) == native && node &&
                    node->getNativeNode() && wp_graphics_object_get_owner( object ) == node->getNativeNode();
                result.vertices += vertices;
                result.triangles += indices / 3;
#endif
                result.readyMeshes += ready;
                if( name.find( "LOD0 city masonry " ) == 0 )
                {
                    ++result.masonryMeshes;
                    result.readyMasonryMeshes += ready;
                }
            }
            result.valid = result.patches > 0 && result.farMeshes > 0 &&
                ( !expectNear || result.nearMeshes > 0 ) && result.meshes == result.readyMeshes;
            return result;
        }

        void writeCityBuildingGeometry( std::ostream &output, const CityBuildingGeometryCheck &check )
        {
            output << "{\"patches\":" << check.patches << ",\"meshes\":" << check.meshes
                   << ",\"readyMeshes\":" << check.readyMeshes << ",\"nearMeshes\":" << check.nearMeshes
                   << ",\"farMeshes\":" << check.farMeshes << ",\"nativeVertices\":" << check.vertices
                   << ",\"nativeTriangles\":" << check.triangles
                   << ",\"masonryMeshes\":" << check.masonryMeshes
                   << ",\"readyMasonryMeshes\":" << check.readyMasonryMeshes
                   << ",\"ready\":" << ( check.valid ? "true" : "false" ) << '}';
        }

        advanced::ProductionSceneProfile::Workload describeProfileScene( const advanced::SceneAssets &assets )
        {
            advanced::ProductionSceneProfile::Workload result;
            const auto unique = []( const auto &objects ) {
                u64 count = 0;
                for( size_t i = 0; i < objects.size(); ++i )
                {
                    if( !objects[i] ) continue;
                    bool first = true;
                    for( size_t j = 0; j < i; ++j )
                        if( objects[j].get() == objects[i].get() ) { first = false; break; }
                    count += first;
                }
                return count;
            };
            result.actors = unique( assets.actors );
            result.materials = unique( assets.materials );
            result.textures = unique( assets.textures );
            result.meshes = unique( assets.meshes );
            result.materialReferences = assets.materials.size();
            result.textureReferences = assets.textures.size();
            result.meshReferences = assets.meshes.size();
            result.triangles = assets.triangles;
            result.authoredTexturePayloadBytes = assets.textureBytes;
            result.treeLodGroups = assets.treeLODs.size();
            const auto hash = []( u64 &digest, const auto &value ) {
                advanced::ProductionSceneProfile::hashBytes( digest, &value, sizeof( value ) );
            };
            for( const auto &sample : assets.circuit.samples )
            {
                hash( result.circuitDigest, sample.position.x ); hash( result.circuitDigest, sample.position.y );
                hash( result.circuitDigest, sample.position.z ); hash( result.circuitDigest, sample.right.x );
                hash( result.circuitDigest, sample.right.y ); hash( result.circuitDigest, sample.right.z );
                hash( result.circuitDigest, sample.distance );
            }
            for( size_t resourceIndex = 0; resourceIndex < assets.meshes.size(); ++resourceIndex )
            {
                auto resource = assets.meshes[resourceIndex];
                bool first = true;
                for( size_t previous = 0; previous < resourceIndex; ++previous )
                    if( assets.meshes[previous].get() == resource.get() ) { first = false; break; }
                if( !first ) continue;
                if( !resource || !resource->getMesh() ) continue;
                for( auto section : resource->getMesh()->getSubMeshes() )
                {
                    ++result.meshSections;
                    auto vertices = section->getVertexBuffer();
                    auto indices = section->getIndexBuffer();
                    auto declaration = vertices->getVertexDeclaration();
                    const auto position = declaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                    const auto normal = declaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                    const u32 vertexCount = vertices->getNumVertices(), indexCount = indices->getNumIndices();
                    result.vertices += vertexCount; result.indices += indexCount;
                    hash( result.geometryDigest, vertexCount ); hash( result.geometryDigest, indexCount );
                    for( u32 i = 0; i < vertexCount; ++i )
                    {
                        for( auto element : { position, normal } )
                        {
                            if( !element ) continue;
                            const auto value = reinterpret_cast<const f32 *>(
                                static_cast<const u8 *>( vertices->getVertexData() ) +
                                size_t( i ) * declaration->getSize() + element->getOffset() );
                            for( u32 component = 0; component < 3; ++component ) hash( result.geometryDigest, value[component] );
                        }
                    }
                    for( u32 i = 0; i < indexCount; ++i )
                    {
                        const u32 index = indices->getIndexType() == IIndexBuffer::Type::IT_16BIT
                            ? static_cast<const u16 *>( indices->getIndexData() )[i]
                            : static_cast<const u32 *>( indices->getIndexData() )[i];
                        hash( result.geometryDigest, index );
                    }
                }
            }
            return result;
        }

        bool checkSceneWinding( const advanced::SceneAssets &assets )
        {
            size_t checked = 0;
            for( const auto &resource : assets.meshes )
            {
                for( const auto &section : resource->getMesh()->getSubMeshes() )
                {
                    double alignment = 0;
                    auto vertices = section->getVertexBuffer();
                    auto declaration = vertices->getVertexDeclaration();
                    auto position = declaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                    auto normal = declaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                    auto indices = section->getIndexBuffer();
                    if( !position || !normal || !indices || indices->getNumIndices() % 3 != 0 )
                        return false;
                    auto readVector = [&]( u32 index, u32 offset ) {
                        const auto data = reinterpret_cast<const f32 *>(
                            static_cast<const u8 *>( vertices->getVertexData() ) +
                            size_t( index ) * declaration->getSize() + offset );
                        return Vector3F( data[0], data[1], data[2] );
                    };
                    auto readIndex = [&]( u32 index ) -> u32 {
                        return indices->getIndexType() == IIndexBuffer::Type::IT_16BIT
                                   ? static_cast<const u16 *>( indices->getIndexData() )[index]
                                   : static_cast<const u32 *>( indices->getIndexData() )[index];
                    };
                    for( u32 i = 0; i < indices->getNumIndices(); i += 3 )
                    {
                        const auto a = readIndex( i ), b = readIndex( i + 1 ), c = readIndex( i + 2 );
                        if( a >= vertices->getNumVertices() || b >= vertices->getNumVertices() ||
                            c >= vertices->getNumVertices() )
                            return false;
                        const auto face = ( readVector( b, position->getOffset() ) -
                                            readVector( a, position->getOffset() ) )
                                              .crossProduct( readVector( c, position->getOffset() ) -
                                                             readVector( a, position->getOffset() ) );
                        const auto outward = readVector( a, normal->getOffset() ) +
                                             readVector( b, normal->getOffset() ) +
                                             readVector( c, normal->getOffset() );
                        // Smooth normals at narrow/concave car seams can oppose an individual
                        // face. Area-weighted section alignment detects a reversed upload without
                        // requiring every authored smooth normal to equal its triangle normal.
                        alignment += face.dotProduct( outward );
                        ++checked;
                    }
                    if( alignment <= 0 )
                    {
                        WP_LOG_ERROR( "VehicleAdvanced: reversed winding in " + resource->getFilePath() );
                        return false;
                    }
                }
            }
            WP_LOG( "VehicleAdvanced outward winding validated: triangles=" + StringUtil::toString( checked ) );
            return checked > 0;
        }

        struct TerrainCityCheck
        {
            bool valid=false,binding=false,revisions=false,renderer=false,route=false,islands=false,support=false,flatFallbackAbsent=false;
            bool roadPBRBindingsReady=false;
            u32 roadPBRTextures=0,roadPBRMaterials=0;
            s32 asphaltAlbedoWidth=0;
            bool markingAlphaBlend=false;
            bool footwayReady=false,footwayCollisionReady=false;
            u32 footwayActors=0,footwayNativeRayProbes=0,footwayRayCandidates=0;
            size_t loweredCrossings = 0, raisedCrossings = 0, crossingKerbRays = 0;
            float crossingKerbHeightError = 0;
            size_t fittedBoundaries = 0, boundaryCollisionProbes = 0;
            f32 footwayRayHeightError=-1;
            size_t roads=0,junctions=0,triangles=0,routePoints=0,islandsChecked=0,rayProbes=0,buildingBodies=0,expectedBuildingBodies=0;
            f32 routeHeightError=0, roadRayCurveDeviation=0;
        };

        void checkCrossingKerbs( TerrainCityCheck &result, const scene::CityRoadTerrainMesh &walks,
                                 const render::TerrainData &terrain, const Transform3<real_Num> &placement,
                                 physics::IPhysicsScene3 &physics, const SmartPtr<physics::IRigidStatic3> &body )
        {
            for( const auto &crossing : walks.crossings )
            {
                if( !crossing.droppedKerbs )
                {
                    ++result.raisedCrossings;
                    continue;
                }
                ++result.loweredCrossings;
                for( const float side : { -1.f, 1.f } )
                {
                    // The front pair checks the lowered lip. The back pair checks the
                    // unchanged sidewalk height beyond the sloping part of the landing.
                    for( const auto &[inset, lift] : { std::pair{ .22f, .040f }, std::pair{ 2.05f, .15f } } )
                    {
                        auto point = crossing.centre + crossing.across * ( side * ( crossing.width * .5f + inset ) );
                        if( !render::sampleTerrainHeight( terrain, placement, point, point.y ) ) continue;
                        point.y += lift;
                        const Vector3F ray( 0, .22f, 0 );
                        Vector3F hit, normal;
                        SmartPtr<ISharedObject> object;
                        if( !physics.intersects( point + ray, point - ray, hit, normal, object ) ||
                            object != body || !std::isfinite( hit.y ) || normal.y < .5f ) continue;
                        const float error = std::abs( point.y - hit.y );
                        result.crossingKerbHeightError = std::max( result.crossingKerbHeightError, error );
                        if( error <= .003f ) ++result.crossingKerbRays;
                    }
                }
            }
        }

        TerrainCityCheck checkTerrainCity(const advanced::SceneAssets &assets,SmartPtr<physics::IPhysicsScene3> physics)
        {
            TerrainCityCheck result;auto authoring=assets.roadAuthoring;auto terrain=assets.roadTerrain;
            if(!authoring || !terrain || !physics)return result;
            const auto sourceTerrain=terrain->getTerrainSnapshot(),activeTerrain=terrain->getActiveTerrainSnapshot();
            if(!sourceTerrain)return result;
            const auto &source=authoring->getSource();const auto &bake=authoring->getBake();const auto binding=authoring->getTerrainBindingUUID();
            const auto same=[](const SourceBudgetText &a,const String &b){return std::string_view(a.data(),a.size())==std::string_view(b.data(),b.size());};
            result.binding=authoring->getTerrain()==terrain && terrain->getHandle() && binding==terrain->getHandle()->getUUIDAsString() && same(source.terrainResourceUuid,binding) && !source.id.empty();
            result.revisions=authoring->getSourceRevision()==authoring->getActiveRevision() && assets.cityAppliedSourceRevision==authoring->getSourceRevision() && source.terrainRevision==sourceTerrain->revision;
            result.renderer=terrain->getTerrain() && activeTerrain && activeTerrain->revision==sourceTerrain->revision;
            result.roads=source.roads.size();result.junctions=source.roundabouts.size();result.triangles=authoring->getRuntimeTriangleCount();result.routePoints=assets.circuit.samples.size();
            result.route=assets.cityRaceRouteAvailable && result.routePoints>=3;
            for(size_t i=0;i<result.routePoints;++i)
            {
                const auto p=assets.circuit.samples[i].position,q=assets.circuit.samples[(i+1)%result.routePoints].position;f32 height=0;bool driveable=false;
                const bool hit=authoring->getRuntimeSurface(p.x,p.z,height,driveable);const auto step=(q-p).length();
                if(hit && std::isfinite(p.y))result.routeHeightError=std::max(result.routeHeightError,std::abs(p.y-height));
                result.route=result.route && hit && driveable && p.isFinite() && std::isfinite(step) && step>.0001f && std::abs(p.y-height)<=.001f;
            }
            result.islands=!source.roundabouts.empty();
            for(const auto &junction:source.roundabouts)
            {f32 height=0;bool driveable=true;const bool valid=authoring->getRuntimeSurface(junction.center.x,junction.center.z,height,driveable) && !driveable;if(valid)++result.islandsChecked;result.islands=result.islands && valid;}
            const auto ray=[&](Vector3F p,bool driveable,bool ground=false,float tolerance=.035f)
            {
                f32 height=0;bool actualDriveable=false;
                bool valid=ground?render::sampleTerrainHeight(*sourceTerrain,authoring->getTerrainPlacement(),p,height):authoring->getRuntimeSurface(p.x,p.z,height,actualDriveable);
                valid=valid && (ground || actualDriveable==driveable);Vector3F hit,normal;SmartPtr<ISharedObject> object;
                valid=valid && physics->intersects({p.x,height+.25f,p.z},{p.x,height-.25f,p.z},hit,normal,object) && object && std::isfinite(hit.y) && std::abs(hit.y-height)<tolerance && normal.y>.5f;
                if(valid)++result.rayProbes;
            };
            // Probe the most laterally curved sampled point, instead of always
            // choosing the first straight carriageway. Keep a flat-source fallback.
            Vector3F roadProbe; bool haveRoadProbe=false;
            for(const auto &road:bake.roads)if(road.points.size()>=2)
            {
                if(!haveRoadProbe){roadProbe=road.points[road.points.size()/2].position;haveRoadProbe=true;}
                for(const auto &point:road.points)
                {
                    const float deviation=chordDeviation(point.position,road.points.front().position,road.points.back().position);
                    if(deviation>result.roadRayCurveDeviation){result.roadRayCurveDeviation=deviation;roadProbe=point.position;}
                }
            }
            // The draped road is 25 mm above the terrain; terrain collision alone
            // must not satisfy this carriageway probe.
            if(haveRoadProbe)ray(roadProbe,true,false,.01f);
            if(!source.roundabouts.empty())
            {const auto &j=source.roundabouts.front();const float radius=(j.outerRadius+j.islandRadius)*.5f;ray(j.center+Vector3F(radius*.70710678f,0,radius*.70710678f),true);ray(j.center,false);}
            bool terrainProbe=false;const auto placement=authoring->getTerrainPlacement().getTransformationMatrix();
            for(const auto uv:{Vector2F(.02f,.02f),Vector2F(.98f,.02f),Vector2F(.02f,.98f),Vector2F(.98f,.98f),Vector2F(.5f,.02f),Vector2F(.5f,.98f)})
            {
                auto p=placement.transformAffine(Vector3F(sourceTerrain->origin.x+uv.x*(sourceTerrain->dimensions.X()-1)*sourceTerrain->spacing.x,0,sourceTerrain->origin.y+uv.y*(sourceTerrain->dimensions.Y()-1)*sourceTerrain->spacing.y));
                bool clear=true;for(const auto &lot:assets.cityLayout.lots)if(std::abs(p.x-lot.x)<lot.width*.5f+2 && std::abs(p.z-lot.z)<lot.depth*.5f+2){clear=false;break;}
                f32 height=0;bool driveable=false;if(authoring->getRuntimeSurface(p.x,p.z,height,driveable) && driveable)clear=false;
                if(clear){const auto before=result.rayProbes;ray(p,false,true);if(result.rayProbes>before){terrainProbe=true;break;}}
            }
            for(const auto &lot:assets.cityLayout.lots)result.expectedBuildingBodies+=procedural::OpenCityLayout::buildingVolumes(lot).size();
            result.flatFallbackAbsent=assets.ground && terrain->getActor()==assets.ground;
            SmartPtr<scene::IGameActor> footway;
            std::array<SmartPtr<scene::IGameActor>, 4> boundaries{};
            for(const auto &actor:assets.actors)if(actor)
            {
                if(actor->getName().find("Trackside collision: City building mass")==0)++result.buildingBodies;
                if(actor->getName()=="Circuit collision plane")result.flatFallbackAbsent=false;
                if(actor->getName()=="Terrain city footways"){footway=actor;++result.footwayActors;}
                if( actor->getName() == "Terrain city boundary" && result.fittedBoundaries < boundaries.size() )
                    boundaries[result.fittedBoundaries++] = actor;
            }
            if( result.fittedBoundaries == boundaries.size() )
            {
                const float edge = assets.cityLayout.extent + 20;
                for( const auto normal : { Vector3F( 1, 0, 0 ), Vector3F( -1, 0, 0 ),
                                           Vector3F( 0, 0, 1 ), Vector3F( 0, 0, -1 ) } )
                {
                    auto point = normal * edge + Vector3F( normal.z, 0, normal.x ) * ( edge * .51f );
                    if( !render::sampleTerrainHeight( *sourceTerrain, authoring->getTerrainPlacement(), point, point.y ) )
                        continue;
                    point.y += .60f;
                    Vector3F hit, hitNormal;
                    SmartPtr<ISharedObject> object;
                    if( physics->intersects( point - normal * 2.f, point + normal * 2.f, hit, hitNormal, object ) &&
                        std::abs( ( hit - point ).dotProduct( normal ) + .5f ) < .04f &&
                        std::any_of( boundaries.begin(), boundaries.end(), [&]( const auto &actor ) {
                            const auto body = actor->template getComponent<scene::Rigidbody>();
                            return body && object == body->getRigidStatic();
                        } ) ) ++result.boundaryCollisionProbes;
                }
            }
            result.support=assets.cityBuildingSupportValid && assets.cityBuildingSupportChecks>=assets.cityLayout.lots.size() && result.buildingBodies==result.expectedBuildingBodies && result.buildingBodies>0;
            if(footway && result.footwayActors==1)
            {
                auto collision=footway->getComponent<scene::CollisionMesh>();auto rigidbody=footway->getComponent<scene::Rigidbody>();
                auto body=rigidbody?rigidbody->getRigidStatic():nullptr;
                const auto identity=[](const auto &pose) {
                    return pose.getPosition().length()<.00001f && (pose.getScale()-Vector3F::unit()).length()<.00001f &&
                        (pose.getOrientation()*Vector3F::unitX()-Vector3F::unitX()).length()<.00001f &&
                        (pose.getOrientation()*Vector3F::unitY()-Vector3F::unitY()).length()<.00001f;
                };
                result.footwayCollisionReady=collision && !collision->isConvex() && collision->getShape() && collision->getShape()->isAttached() &&
                    body && body->getScene()==physics && identity(footway->getWorldTransform()) && identity(body->getTransform());
                if( const auto walks = authoring->getRuntimeFootways(); result.footwayCollisionReady && walks )
                    checkCrossingKerbs( result, *walks, *sourceTerrain, authoring->getTerrainPlacement(), *physics, body );
                u32 inspectedPoints=0;
                if(result.footwayCollisionReady)for(const auto &road:bake.roads)
                {
                    if(result.footwayNativeRayProbes || inspectedPoints>=128 || result.footwayRayCandidates>=256)break;
                    const auto authored=std::find_if(source.roads.begin(),source.roads.end(),[&](const auto &candidate){return candidate.id==road.roadId;});
                    if(authored==source.roads.end())continue;
                    for(size_t offset=0;offset<road.points.size() && !result.footwayNativeRayProbes && inspectedPoints<128 && result.footwayRayCandidates<256;++offset)
                    {
                        ++inspectedPoints;
                        const auto &point=road.points[(road.points.size()/2+offset)%road.points.size()];
                        Vector3F across(-point.tangent.z,0,point.tangent.x);const float span=across.length();
                        if(!std::isfinite(span) || span<=.00001f)continue;across/=span;
                        for(const float side:{-1.f,1.f})
                        {
                            if(result.footwayNativeRayProbes || result.footwayRayCandidates>=256)break;
                            ++result.footwayRayCandidates;auto p=point.position+across*(side*(authored->width*.5f+1.2f));f32 height=0;
                            if(!render::sampleTerrainHeight(*sourceTerrain,authoring->getTerrainPlacement(),p,height))continue;
                            height+=.15f;Vector3F hit,normal;SmartPtr<ISharedObject> object;
                            if(physics->intersects({p.x,height+.2f,p.z},{p.x,height-.2f,p.z},hit,normal,object) && object==body &&
                                std::isfinite(hit.y) && std::abs(hit.y-height)<=.001f && normal.y>.5f)
                            {result.footwayNativeRayProbes=1;result.footwayRayHeightError=std::abs(hit.y-height);}
                        }
                    }
                }
            }
            result.footwayReady=result.footwayCollisionReady && result.footwayNativeRayProbes==1;
            // Source-owned road runtime resources are outside SceneAssets' texture
            // inventory. Inspect the active world's bindings and actual uploads.
            auto app=core::IApplicationManager::instancePtr();auto manager=app?app->getGameManager():nullptr;
            const auto currentScene=manager?manager->getCurrentScene():nullptr;
            SourceBudgetSet<SourceBudgetText> materialNames;SourceBudgetSet<render::ITexture *> textures;
            std::array<u32,4> layers{};bool mapsReady=bool(currentScene);
            if(manager)for(const auto &actor:manager->getActors())if(actor && actor->getScene()==currentScene)
            {
                const auto name=actor->getName();const int layer=name=="Authored road carriageway"?0:name=="Roundabout island"?1:name=="Roundabout kerb"?2:name=="Road markings"?3:-1;
                if(layer<0)continue;++layers[size_t(layer)];
                auto component=actor->getComponent<scene::Material>();auto material=component?component->getMaterial():nullptr;
                if(!material){mapsReady=false;continue;}
                const auto materialName=material->getName();SourceBudgetText key(materialName.data(),materialName.size());
                if(key.empty() || !materialNames.insert(key).second){mapsReady=false;continue;}
                if( layer == 0 )
                {
                    if( auto albedo = material->getTexture( 0 ) ) result.asphaltAlbedoWidth = albedo->getSize().x;
                }
                if( layer == 3 ) result.markingAlphaBlend = material->isTransparent() && material->getBlendMode() == 1;
                const auto uploaded=[&](u32 slot) {auto texture=material->getTexture(slot);void *native=nullptr;if(texture)texture->getTextureFinal(&native);if(native)textures.insert(texture.get());else mapsReady=false;};
                uploaded(0);if(layer!=3)for(const u32 slot:{1u,22u,3u,2u})uploaded(slot);
            }
            result.roadPBRMaterials=u32(materialNames.size());result.roadPBRTextures=u32(textures.size());const bool junctions=!source.roundabouts.empty();
            result.roadPBRBindingsReady=mapsReady && layers[0]==1 && layers[3]==1 && layers[1]==(junctions?1u:0u) && layers[2]==(junctions?1u:0u) &&
                result.roadPBRMaterials==(junctions?4u:2u) && result.roadPBRTextures==(junctions?16u:6u);
            result.valid=result.binding && result.revisions && result.renderer && result.route && result.islands && result.support && result.flatFallbackAbsent && terrainProbe && result.rayProbes==4 && result.roads>0 && result.triangles>0 && result.roadPBRBindingsReady && result.footwayReady &&
                         result.crossingKerbRays == result.loweredCrossings * 4 &&
                         ( result.fittedBoundaries < boundaries.size() || result.boundaryCollisionProbes == boundaries.size() );
            return result;
        }

        Vector3<real_Num> getVehicleSpawnPosition()
        {
            return Vector3<real_Num>::unitY() * vehicleSpawnHeight;
        }

        Vector3<real_Num> getInitialCameraPosition()
        {
            return getVehicleSpawnPosition() +
                   Vector3<real_Num>( static_cast<real_Num>( 0.0 ), cameraHeight, cameraDistance );
        }

        bool checkMeshTransforms( SmartPtr<scene::IGameActor> actor,
                                  const Transform3<real_Num> &expected )
        {
            if( auto renderer = actor->getComponent<scene::MeshRenderer>() )
            {
                auto node = renderer->getGraphicsNode();
                if( !node )
                    return false;

                const auto actual = node->getTransform();
                if( ( actual.getPosition() - expected.getPosition() ).length() > 0.01f ||
                    ( actual.getScale() - expected.getScale() ).length() > 0.01f ||
                    ( actual.forward() - expected.forward() ).length() > 0.01f ||
                    ( actual.up() - expected.up() ).length() > 0.01f )
                {
                    WP_LOG_ERROR( "Vehicle scene-node mismatch: " + actor->getName() +
                                  " position error=" + StringUtil::toString( ( actual.getPosition() - expected.getPosition() ).length() ) +
                                  " orientation error=" + StringUtil::toString( ( actual.forward() - expected.forward() ).length() ) );
                    return false;
                }

#if WP_GRAPHICS_SYSTEM_CLAW
                auto mesh = dynamic_pointer_cast<render::ClawMesh>( renderer->getGraphicsObject() );
                auto nativeNode = dynamic_pointer_cast<render::ClawSceneNode>( node );
                auto object = mesh ? mesh->getNativeRenderObject() : nullptr;
                if( !object || !nativeNode || !nativeNode->getNativeNode() ||
                    wp_graphics_object_get_owner( object ) != nativeNode->getNativeNode() ||
                    !mesh->getNativeMesh() ||
                    wp_graphics_mesh_get_vertex_count( mesh->getNativeMesh() ) == 0 )
                    return false;

                // Read the same owner matrix that the native mesh draw call uses.
                wp_mat4f rendered;
                wp_scenenode_get_world_matrix( wp_graphics_object_get_owner( object ), &rendered );
                Matrix4F expectedMatrix;
                expectedMatrix.makeTransform( expected.getPosition(), expected.getScale(),
                                              expected.getOrientation() );
                for( size_t i = 0; i < 16; ++i )
                    if( Math<real_Num>::Abs( rendered.m[i / 4][i % 4] - expectedMatrix.ptr()[i] ) >
                        0.01f )
                    {
                        WP_LOG_ERROR( "Vehicle render mismatch: " + actor->getName() +
                                      " matrix element " + StringUtil::toString( i ) +
                                      " actual=" + StringUtil::toString( rendered.m[i / 4][i % 4] ) +
                                      " expected=" + StringUtil::toString( expectedMatrix.ptr()[i] ) );
                        return false;
                    }
#endif
            }

            for( auto child : actor->getChildren() )
            {
                if( child->getFlag( scene::IGameActor::ActorFlagWorldSpaceTransform ) )
                {
                    continue;
                }
                Transform3<real_Num> childWorld;
                childWorld.transformFromParent( expected, child->getLocalTransform() );
                if( !checkMeshTransforms( child, childWorld ) )
                    return false;
            }

            return true;
        }

    }  // namespace

    SampleVehicleAdvanced::SampleVehicleAdvanced()
    {
        setPluginsConfigFilePath( "wp_plugins_samples.cfg" );
    }

    SampleVehicleAdvanced::~SampleVehicleAdvanced()
    {
        unload( nullptr );
    }

    void SampleVehicleAdvanced::setTerrainGameplayOptions( bool enabled,
                                                           const std::string &packageDirectory,
                                                           bool exportOnly )
    {
        m_terrainGameplay = enabled || !packageDirectory.empty();
        m_terrainGameplayExportOnly = exportOnly;
        m_terrainGameplayPackageDirectory = packageDirectory.empty()
            ? std::string() : std::filesystem::absolute( packageDirectory ).lexically_normal().string();
#if WP_GRAPHICS_SYSTEM_CLAW
        render::setClawUiSettingsPersistence( packageDirectory.empty() );
#endif
        if( !m_terrainGameplayPackageDirectory.empty() )
        {
            m_audioEnabled = false;
            m_hudEnabled = false;
        }
    }

    void SampleVehicleAdvanced::exportTerrainGameplayPackage( const std::string &directory )
    {
#if WP_TERRAIN_GAMEPLAY_PACKAGE
        if( !m_terrainGameplay || !m_terrainGameplaySource || !m_terrainGameplayPackageDirectory.empty() ||
            !m_raceScene || !m_raceScene->isGenerated() )
            throw std::runtime_error( "Terrain gameplay export requires a generated authoring scene." );
        advanced::terrain_gameplay::exportPackage( directory, *m_terrainGameplaySource,
            m_seed, int( m_quality ), m_assets.vehicle.contentHash );
        std::cout << "Terrain gameplay export: PASS\n";
#else
        throw std::runtime_error( "Terrain gameplay packaging is unavailable in this build." );
#endif
    }

    void SampleVehicleAdvanced::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Loading || loadingState == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            Thread::setCurrentTask( TaskId::Primary );
            Thread::setCurrentThreadId( Thread::ThreadId::Primary );
            Thread::setTaskFlags( std::numeric_limits<u32>::max() );

            auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
            WP_ASSERT( applicationManager );

            applicationManager->load( data );
            core::IApplicationManager::setInstance( applicationManager );
            m_applicationManager = applicationManager;
            applicationManager->setApplication( this );

            Application::load( data );
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
            if( m_preferredSceneSamples == 0 && m_cityTerrainRoads &&
                m_quality != procedural::VehicleAppearanceQuality::Preview )
            {
                m_preferredSceneSamples =
                    m_quality == procedural::VehicleAppearanceQuality::Standard ? 2 : 4;
            }
            if( m_preferredSceneSamples != 0 )
            {
                auto graphics = applicationManager->getGraphicsSystem();
                auto pipeline = graphics->getGraphicsPipeline();
                auto renderer = dynamic_cast<render::ClawRendererDX11 *>( graphics->getRendererPtr() );
                if( pipeline && renderer )
                {
                    const auto native = wp_renderer_get_dx11( renderer->getNativeRenderer() );
                    const auto supportedSamples = wp_renderer_dx11_supported_scene_samples( native );
                    auto samples = m_preferredSceneSamples;
                    while( samples > 1 && !( supportedSamples & samples ) ) samples /= 2;
                    auto options = workphone::make_ptr<Properties>();
                    const s32 aaMode = samples == 4 ? 4 : ( samples == 2 ? 3 : 0 );
                    options->setProperty( "graphics.antiAliasing", aaMode );
                    pipeline->setProperties( options );
                    m_configuredSceneSamples = samples;
                    if( samples != m_preferredSceneSamples )
                        WP_LOG_WARNING( "VehicleAdvanced: the device selected a lower supported MSAA sample count." );
                }
            }
#endif
#if WP_GRAPHICS_SYSTEM_CLAW
            if( m_sceneProfile )
            {
                auto graphics = dynamic_pointer_cast<render::ClawHammerSystem>( applicationManager->getGraphicsSystem() );
                if( !graphics ) throw std::runtime_error( "Scene profile requires the Claw graphics system." );
                graphics->setSceneProfilingEnabled( true );
#if defined( _WIN32 )
                // Enable timing during the existing warmup so lazily created GPU
                // query resources are resident before the warmed memory sample.
                // The measurement boundary still resets statistics separately.
                if( auto renderer = dynamic_cast<render::ClawRendererDX11 *>( graphics->getRendererPtr() ) )
                    wp_renderer_dx11_reset_statistics(
                        wp_renderer_get_dx11( renderer->getNativeRenderer() ) );
#endif
            }
#endif
            // Application::load logs scene exceptions instead of propagating them.
            // Do not enter Play with a partially generated vehicle/circuit.
            if( !m_raceScene || !m_raceScene->isGenerated() || (!m_assets.cityTerrainRoads && m_assets.circuit.samples.empty()) ||
                !m_cameraActor )
            {
                const auto error = m_raceScene ? m_raceScene->getGenerationError() : String();
                throw std::runtime_error( error.empty() ? "Vehicle scene initialization failed." : error.c_str() );
            }

            m_inputListener = workphone::make_ptr<InputListener>();
            m_inputListener->setOwner( this );

            auto inputManager = applicationManager->getInputDeviceManager();
            WP_ASSERT( inputManager );
            if( inputManager )
            {
                inputManager->addListener( m_inputListener );
            }

            auto taskManager = applicationManager->getTaskManager();
            WP_ASSERT( taskManager );
            if( auto physicsTask = taskManager->getTask( TaskId::Physics ) )
            {
                physicsTask->setTargetFPS( physicsTargetFps );
            }

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );
            applicationManager->setPlaying( true );
            applicationManager->setPaused( false );
            sceneManager->play();

            auto car = m_vehicleActor->getComponent<scene::CarController>();
            auto vehicleController = car->getVehicleController();
            m_raceScene->configurePhysics();

            m_raceScene->initializePresentation();
            m_reviewStart = -1;
            m_profileStart = 0;
            m_profileFrames = 0;
            m_reviewFrames = 0;
            m_benchmarkStarted = false;
            m_captureAttempted = false;
            m_capturePassed = false;
            m_replayTick = 0;
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            // Application::load catches createScene failures. Preserve the real
            // cause before unload clears its component, for unattended runners.
            std::cerr << "VehicleAdvanced Generation Error: " << e.what() << '\n';
            unload( data );
        }
    }

    void SampleVehicleAdvanced::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded || loadingState == LoadingState::Unloading )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );


            if( m_inputListener )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto inputManager = applicationManager->getInputDeviceManager() )
                    {
                        inputManager->removeListener( m_inputListener );
                    }
                }

                m_inputListener->unload( data );
                m_inputListener = nullptr;
            }

            m_boxGround = nullptr;
            m_cameraActor = nullptr;
            m_vehicleActor = nullptr;
            m_chassisMeshActor = nullptr;
            for( auto &wheelActor : m_wheelActors )
                wheelActor = nullptr;
            m_cameraController = nullptr;
            m_raceScene = nullptr;
            m_materialEdits.reset();
            m_assets = {};
            m_cityBuildingStartupDeadline = 0;
            m_cityExtent = m_citySpacing = 0;
            m_cityOverviewExtent = 0;
            m_cityOverviewCentre = m_cityStreetCentre = Vector3F::zero();
            m_cityStreetForward = { 0, 0, -1 };
            m_cityHasJunction=false;m_cityJunctionCentre=m_cityBoulevardCentre={};m_generatedSpawnPosition={0,.42f,0};
            m_cityRetentionSmokePrepared=false;SourceBudgetText{}.swap(m_cityRetentionSmokeRoadId);m_cityRetentionSmokeWidth=0;

            if( core::IApplicationManager::instance() )
            {
                Application::unload( data );
            }
            m_terrainGameplaySource.reset();
#if WP_TERRAIN_GAMEPLAY_PACKAGE
            m_terrainGameplayPackage.reset();
#endif
            if( !m_terrainGameplayEphemeralDirectory.empty() )
            {
                const auto path = std::filesystem::path(m_terrainGameplayEphemeralDirectory);
                const auto temp = std::filesystem::weakly_canonical(std::filesystem::temp_directory_path());
                const auto resolved = std::filesystem::weakly_canonical(path);
                const auto filename = path.filename().string();
                const bool validationsPassed = m_terrainSurfacesValidated &&
                    (!m_smokeTest || m_smokeTestPassed) &&
                    ((m_capturePath.empty() && m_benchmarkSeconds<=0) || m_capturePassed);
                if( validationsPassed && filename.rfind("WorkphoneTerrainGameplay-",0)==0 &&
                    filename.size()==61 && resolved.parent_path()==temp && resolved.filename()==path.filename() )
                    std::filesystem::remove_all(resolved);
                else
                    std::cerr << "Terrain gameplay temporary evidence retained: " << m_terrainGameplayEphemeralDirectory << '\n';
                m_terrainGameplayEphemeralDirectory.clear();
            }
            m_terrainSurfacesValidated = false;

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void SampleVehicleAdvanced::update()
    {
        const auto generationFailed=[&]() {
            const bool unattended=m_collisionSmokeTest || m_smokeTest || m_trackSmokeTest || m_audioSmokeTest || m_effectsSmokeTest ||
                !m_capturePath.empty() || m_benchmarkSeconds>0;
            if(!unattended || !m_raceScene || m_raceScene->isGenerated())return false;
            const auto error=m_raceScene->getGenerationError();
            if(error.empty())return false; // An unreported/pending startup is not a definitive failure.
            WP_LOG_ERROR("VehicleAdvanced Generation Error: "+error);
            std::cerr<<"VehicleAdvanced Generation Error: "<<error<<'\n';
            m_smokeTestPassed=false;m_capturePassed=false;m_captureAttempted=true;
            if(auto app=core::IApplicationManager::instancePtr())app->setQuit(true);
            return true;
        };
        if(generationFailed())return;
        if( Thread::getCurrentTask() == TaskId::Render && m_cityBuildingStartupDeadline > 0 &&
            !m_captureAttempted && core::IApplicationManager::instance()->getTimer()->now() > m_cityBuildingStartupDeadline )
        {
            m_cityBuildingStartupDeadline = 0;
            const auto check = checkCityBuildingGeometry( m_assets, m_quality != procedural::VehicleAppearanceQuality::Preview );
            if( !check.valid )
            {
                std::ostringstream failure;
                failure << "{\"schema\":\"workphone.vehicle-review.v1\",\"validationPassed\":false,"
                           "\"failure\":\"City building meshes did not finish native upload within 30 seconds\",\"cityBuildingGeometry\":";
                writeCityBuildingGeometry( failure, check );
                failure << ",\"sharedMemoryLedger\":"; writeSharedMemoryLedger( failure ); failure << '}';
                std::cerr << failure.str() << std::endl;
                if( !m_evidencePath.empty() ) advanced::writeReviewEvidence( m_evidencePath, failure.str() );
                m_capturePassed = false; m_captureAttempted = true;
                core::IApplicationManager::instance()->setQuit( true );
                return;
            }
        }
        if( Thread::getCurrentTask() == TaskId::Physics )
        {
            if( !m_physicsConfigured &&
                core::IApplicationManager::instance()->getTimer()->getTimeSinceSceneLoad() > 3 )
            {
                m_raceScene->configurePhysics();
                m_physicsConfigured = m_raceScene->isPhysicsConfigured();
            }
#if WP_TERRAIN_GAMEPLAY_PACKAGE && WP_GRAPHICS_SYSTEM_CLAW
            if( m_terrainGameplay && !m_physicsConfigured )
            {
                const auto now = core::IApplicationManager::instance()->getTimer()->getTimeSinceSceneLoad();
                if( now > 3 && now >= m_nextDebugLog )
                {
                    m_nextDebugLog = now + 2;
                    const auto ground = m_assets.ground;
                    const auto terrain = ground ? ground->getComponent<scene::TerrainSystem>() : nullptr;
                    const auto collision = ground ? ground->getComponent<scene::CollisionTerrain>() : nullptr;
                    const auto body = ground ? ground->getComponent<scene::Rigidbody>() : nullptr;
                    const auto renderer = terrain ? dynamic_cast<render::ClawTerrain *>(terrain->getTerrain().get()) : nullptr;
                    const auto source = terrain ? terrain->getTerrainSnapshot() : nullptr;
                    const auto query = terrain ? terrain->getActiveTerrainSnapshot() : nullptr;
                    const auto collisionSource = collision ? collision->getTerrainSnapshot() : nullptr;
                    const auto requested = renderer ? renderer->getTerrainSnapshot() : nullptr;
                    const auto active = renderer ? renderer->getActiveTerrainSnapshot() : nullptr;
                    const auto rendered = renderer ? renderer->getRenderedTerrainSnapshot() : nullptr;
                    const auto identity = []( const render::TerrainSnapshot &value ) {
                        std::ostringstream text;
                        text << value.get() << ':' << (value ? value->revision : 0);
                        return text.str();
                    };
                    std::ostringstream pending;
                    pending << "Terrain gameplay pending: source=" << identity(source)
                            << " query=" << identity(query) << " requested=" << identity(requested)
                            << " active=" << identity(active) << " rendered=" << identity(rendered)
                            << " collision=" << identity(collisionSource)
                            << " actorEnabled=" << bool(ground && ground->isEnabledInScene())
                            << " componentEnabled=" << bool(terrain && terrain->isEnabled())
                            << " actorState=" << (ground ? int(ground->getState()) : -1)
                            << " staticBody=" << bool(body && body->getRigidStatic())
                            << " shapes=" << (body && body->getRigidStatic() ? body->getRigidStatic()->getNumShapes() : 0);
                    if( renderer )
                    {
                        const auto residency = renderer->getTerrainResidency();
                        pending << " visible=" << renderer->isVisible()
                                << " publicationPending=" << renderer->hasPendingTerrainPublication()
                                << " residentTiles=" << residency.residentTiles << " stagedTiles=" << residency.stagedTiles
                                << " requestedTiles=" << residency.requestedTiles
                                << " residencyError=" << renderer->getTerrainResidencyError().c_str();
                    }
                    if( collision ) pending << " collisionError=" << collision->getTerrainError().c_str();
                    WP_LOG(pending.str());
                    std::cout << pending.str() << '\n';
                }
            }
#endif
            if( m_terrainGameplay && m_physicsConfigured && !m_terrainSurfacesValidated )
            {
                // Deferred ground may have published after Play created the car.
                // Start the existing smoke phases from the normal spawn and rest.
                performReset();
                m_terrainSurfacesValidated = validateTerrainGameplaySurfaces();
                if( !m_terrainSurfacesValidated )
                {
                    m_smokeTestPassed = m_capturePassed = false;
                    m_captureAttempted = true;
                    core::IApplicationManager::instance()->setQuit( true );
                    return;
                }
            }
            if( m_resetRequested.exchange( false ) )
            {
                performReset();
            }

            updateControls();

        }

        // The application camera is a render object, so only touch its scene node from the render
        // task. VehicleCameraController advances the smooth rig on the render task.
        if( Thread::getCurrentTask() == TaskId::Render )
        {
            // Physics listeners publish the root pose between scene updates. Resolve its dirty
            // descendants before drawing, preserving their local offsets and wheel rotations.
            if( m_vehicleActor )
                m_vehicleActor->updateTransform();
        }

        Application::update();
        if(generationFailed())return;
        if(m_collisionSmokeTest && m_assets.cityTerrainRoads && !m_cityRetentionSmokePrepared &&
           Thread::getCurrentTask()==TaskId::Application && m_physicsConfigured &&
           core::IApplicationManager::instance()->getTimer()->getTimeSinceSceneLoad()>3)
            prepareCityRetentionSmoke();
        if( isRoadTerrainEditView( m_captureView ) && !m_roadTerrainEditPrepared &&
            Thread::getCurrentTask() == TaskId::Application )
        {
            if( m_roadTerrainEditApplied )
                adoptRoadTerrainEditReview();
            else if( m_physicsConfigured && m_roadTerrainEditTerrainReady )
                prepareRoadTerrainEditReview();
        }
        if ( m_audioSmokeTest && m_audioEnabled && Thread::getCurrentTask() == TaskId::Physics )
            updateVehicleAudio();
        if( m_collisionSmokeTest && Thread::getCurrentTask() == TaskId::Physics )
            updateCollisionSmokeTest();
        if( m_smokeTest && Thread::getCurrentTask() == TaskId::Physics )
        {
            updateSmokeTest();
        }

        if( Thread::getCurrentTask() == TaskId::Render )
        {
            if( isRoadTerrainEditView( m_captureView ) && !m_roadTerrainEditApplied )
            {
                auto terrain = m_assets.roadTerrain;
                const auto snapshot = terrain ? terrain->getActiveTerrainSnapshot() : nullptr;
#if WP_GRAPHICS_SYSTEM_CLAW
                auto renderer = terrain ? dynamic_pointer_cast<render::ClawTerrain>( terrain->getTerrain() ) : nullptr;
                m_roadTerrainEditTerrainReady = snapshot && renderer &&
                    renderer->getRenderedTerrainSnapshot() == snapshot;
#else
                m_roadTerrainEditTerrainReady = bool( snapshot );
#endif
            }
            updateRenderCamera();
            if ( m_effectsSmokeTest && m_effectsEnabled ) updateVehicleEffects();
            if( m_smokeTest && m_smokePhase > 0 )
            {
                auto car = m_vehicleActor->getComponent<scene::CarController>();
                auto vehicle = car->getVehicleController();
                // Check the sustained turn and return to centre after the application
                // task has published the pose; input changes can precede it by one frame.
                for( u32 i = 0; m_smokePhase >= 2 && m_smokeTime > .5 && i < 2; ++i )
                {
                    const auto angle = vehicle->getWheelController( i )->getSteeringAngle();
                    // The assisted controller can be travelling toward the input
                    // or applying its speed limit. Check the authored lock rather
                    // than requiring an instantaneous, unfiltered input angle.
                    if( !std::isfinite( float( angle ) ) ||
                        std::abs( float( angle ) ) >
                            float( m_assets.vehicle.physics.wheels[i].maxSteerRad * 180.0 /
                                   3.14159265358979323846 ) +
                                .01f )
                    {
                        WP_LOG_ERROR( "Vehicle smoke: steering input lost its configured angle scale." );
                        m_smokeTestPassed = false;
                        core::IApplicationManager::instance()->setQuit( true );
                    }
                    const auto expectedAxle =
                        QuaternionF::eulerDegrees( 0, -float( angle ), 0 ) * Vector3F::unitX();
                    const auto renderedAxle =
                        m_wheelActors[i]->getLocalOrientation() * Vector3F::unitX();
                    // Spin about the axle cannot affect this comparison: it verifies
                    // that the visible steering frame matches the tyre contact frame.
                    // Physics can advance after the render wheel pose was
                    // published. Allow one tick of the configured steering slew.
                    const auto steeringTolerance =
                        .002f +
                        float(
                            90.0 *
                            std::clamp( core::IApplicationManager::instance()->getTimer()->getDeltaTime(
                                            TaskId::Physics ),
                                        0.0, 1.0 / 30.0 ) *
                            3.14159265358979323846 / 180.0 );
                    if( ( expectedAxle - renderedAxle ).length() > steeringTolerance )
                    {
                        WP_LOG_ERROR( "Vehicle smoke: rendered steering disagrees with wheel physics." );
                        m_smokeTestPassed = false;
                        core::IApplicationManager::instance()->setQuit( true );
                    }
                }
                auto app = core::IApplicationManager::instance();
                auto timer = app->getTimer();
                auto transform = m_vehicleActor->getTransform();
                auto expectedPose = m_vehicleActor->getWorldTransform();
                app->getGameManager()->getTransformState(
                    m_vehicleActor->getHandle()->getInstanceId(),
                    timer->getTime( TaskId::Render ) - scene::IGameManager::smoothMotionDelay,
                    timer->getDeltaTime( TaskId::Render ), expectedPose, transform->getTask() );
                if( !checkMeshTransforms( m_vehicleActor, expectedPose ) )
                {
                    WP_LOG_ERROR( "Vehicle smoke: rendered vehicle hierarchy does not match its pose." );
                    m_smokeTestPassed = false;
                    core::IApplicationManager::instance()->setQuit( true );
                }
            }

            updateDebugText();
            auto app = core::IApplicationManager::instance();
            // Loading/procedural generation can take longer than the entire
            // review interval. Warm up actual rendered frames, then measure.
            const auto sceneTime = app->getTimer()->getTimeSinceSceneLoad();
            if( m_reviewStart < 0 ) m_reviewStart = sceneTime;
            const auto profileNow = std::max( 0.0, sceneTime - m_reviewStart );
            ++m_reviewFrames;
            const bool roadEditReady = !isRoadTerrainEditView( m_captureView ) ||
                ( m_roadTerrainEditPrepared &&
                  checkCityBuildingGeometry( m_assets, m_quality != procedural::VehicleAppearanceQuality::Preview ).valid );
            bool profileStartedThisFrame = false;
            if( profileNow > 5 && m_reviewFrames >= 30 && !m_benchmarkStarted &&
                ( !m_terrainGameplay || m_terrainSurfacesValidated ) && roadEditReady )
            {
                m_benchmarkStarted = true;
                m_profileStart = profileNow;
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
                if( auto renderer = dynamic_cast<render::ClawRendererDX11 *>(
                        app->getGraphicsSystem()->getRendererPtr() ) )
                    wp_renderer_dx11_reset_statistics(
                        wp_renderer_get_dx11( renderer->getNativeRenderer() ) );
#endif
                if( m_sceneProfile )
                {
#if WP_GRAPHICS_SYSTEM_CLAW
                    if( auto graphics = dynamic_pointer_cast<render::ClawHammerSystem>( app->getGraphicsSystem() ) )
                        graphics->resetSceneProfile();
#endif
                    m_sceneProfile->beginMeasurement();
                    profileStartedThisFrame = true;
                }
            }
            if( m_benchmarkStarted )
            {
                ++m_profileFrames;
            }
            if( m_sceneProfile && m_benchmarkStarted && !profileStartedThisFrame && !m_captureAttempted )
            {
#if WP_GRAPHICS_SYSTEM_CLAW
                if( auto graphics = dynamic_pointer_cast<render::ClawHammerSystem>( app->getGraphicsSystem() ) )
                {
                    render::ClawHammerSystem::SceneProfileSnapshot snapshot;
                    if( graphics->readSceneProfile( snapshot ) ) m_sceneProfile->observeTelemetry( snapshot );
                }
#endif
                m_sceneProfile->frame();
            }
            const bool finishProfileThisFrame=( !m_capturePath.empty() || m_benchmarkSeconds > 0 ) && !m_captureAttempted &&
                m_benchmarkStarted && m_profileFrames >= ( m_sceneProfile ? 31 : 30 ) &&
                profileNow - m_profileStart > ( m_benchmarkSeconds > 0 ? m_benchmarkSeconds : 3 );
#if defined(_WIN32) && WP_GRAPHICS_SYSTEM_CLAW
            if(m_materialEdits && m_sceneProfile && m_sceneProfile->isMeasuring() && !finishProfileThisFrame)
                m_materialEdits->apply(m_sceneProfile->materialCpu());
#endif
            if( finishProfileThisFrame )
            {
                // Stop ROI before logging, owner inventories, GPU readbacks or
                // formatting validation evidence can perturb the measurement.
                if( m_sceneProfile ) m_sceneProfile->endMeasurement();
#if defined(_WIN32) && WP_GRAPHICS_SYSTEM_CLAW
                if(m_materialEdits && m_sceneProfile) m_sceneProfile->setMaterialEditEvidence(m_materialEdits->json());
#endif
                bool linearSceneValid = true;
                bool sceneSamplingValid = true;
                size_t treeMeshes = 0, treeImposters = 0;
                bool treeLODValid = m_terrainGameplay || m_assets.openCity || m_assets.useFoliageWorld || !m_assets.treeLODs.empty();
                for( const auto &group : m_assets.treeLODs )
                {
                    auto levels = group->getLevels();
                    treeLODValid &=
                        levels[0].renderers[0]->isLODVisible() != levels[1].renderers[0]->isLODVisible();
                    treeMeshes += levels[0].renderers[0]->isLODVisible() ? 1 : 0;
                    treeImposters += levels[1].renderers[0]->isLODVisible() ? 1 : 0;
                }
                WP_LOG( String( "Tree LOD patches: meshes=" ) + StringUtil::toString( treeMeshes ) +
                        " imposters=" + StringUtil::toString( treeImposters ) );
                bool vehicleLODValid = m_assets.vehicleLOD && m_assets.vehicleLOD->validate();
                if( vehicleLODValid )
                {
                    size_t visibleLevels = 0;
                    auto levels = m_assets.vehicleLOD->getLevels();
                    for( size_t i = 0; i < levels.size(); ++i )
                    {
                        const bool visible = levels[i].renderers.front()->isLODVisible();
                        for( const auto &renderer : levels[i].renderers )
                            vehicleLODValid &= renderer->isLODVisible() == visible;
                        if( visible )
                        {
                            ++visibleLevels;
                            WP_LOG( String( "Vehicle LOD level=" ) + StringUtil::toString( i ) );
                        }
                    }
                    vehicleLODValid &= visibleLevels == 1;
                }
                WP_LOG( String( "VehicleAdvanced mean frame milliseconds=" ) +
                        StringUtil::toString( m_profileFrames > 1
                                                  ? 1000 * ( profileNow - m_profileStart ) /
                                                        ( m_profileFrames - 1 )
                                                  : 0 ) +
                        " scene meshes=" + StringUtil::toString( m_assets.meshes.size() ) +
                        " textures=" + StringUtil::toString( m_assets.textures.size() ) );
                m_captureAttempted = true;
                bool nativeFrameValid = true;
                std::ostringstream evidence;
                evidence.imbue( std::locale::classic() );
                evidence << std::setprecision( 17 )
                         << "{\n  \"schema\": \"workphone.vehicle-review.v1\",\n"
                         << "  \"seed\": " << m_seed << ",\n  \"quality\": " << int( m_quality )
                         << ",\n  \"camera\": " << advanced::jsonString( m_captureView )
                         << ",\n  \"capture\": " << advanced::jsonString( m_capturePath )
                         << ",\n  \"replay\": " << advanced::jsonString( m_replayPath )
                         << ",\n  \"materialSourceExport\": " << advanced::jsonString( m_materialExportDirectory )
                         << ",\n  \"replayHz\": 120,\n  \"replayTicksConsumed\": " << m_replayTick
                         << ",\n  \"replayTicksAvailable\": " << m_replayControls.size()
                         << ",\n  \"requestedWidth\": " << m_reviewWidth
                         << ",\n  \"requestedHeight\": " << m_reviewHeight
                         << ",\n  \"forcedTreeLOD\": " << m_forcedLOD
                         << ",\n  \"forcedVehicleLOD\": " << m_forcedVehicleLOD
                         << ",\n  \"orbit\": " << ( m_orbitCamera ? "true" : "false" )
                         << ",\n  \"hud\": " << ( m_hudEnabled ? "true" : "false" )
                         << ",\n  \"audioEnabled\": " << ( m_audioEnabled ? "true" : "false" )
                         << ",\n  \"effectsEnabled\": " << ( m_effectsEnabled ? "true" : "false" )
                         << ",\n  \"measurementSeconds\": " << profileNow - m_profileStart
                         << ",\n  \"performanceAcceptance\": \"unverified\""
                         << ",\n  \"imagePipelineSettings\": null"
                         << ",\n  \"renderUpdatesMeasured\": " << m_profileFrames
                         << ",\n  \"sceneMeshes\": " << m_assets.meshes.size()
                         << ",\n  \"sceneTextures\": " << m_assets.textures.size();
                evidence << ",\n  \"sceneMode\": " << advanced::jsonString( m_terrainGameplay?"terrain-gameplay":m_assets.cityTerrainRoads?"terrain-city":m_assets.cityRoadShowcase?"road-authoring-showcase":m_assets.openCity ? "city" : "circuit" );
                if( m_terrainGameplay )
                    evidence << ",\n  \"terrainGameplaySurfacesValidated\": " << (m_terrainSurfacesValidated?"true":"false")
                             << ",\n  \"terrainGameplaySourceFree\": " << (!m_terrainGameplayPackageDirectory.empty()?"true":"false")
                             << ",\n  \"vehicleRecipeHash\": " << advanced::jsonString(StringUtil::toString(m_assets.vehicle.contentHash));
                bool cityValid = m_assets.openCity == m_openCity && m_assets.cityRoadShowcase==m_roadAuthoringShowcase && m_assets.cityTerrainRoads==m_cityTerrainRoads;
                if( isRoadTerrainEditView( m_captureView ) )
                    cityValid = writeRoadTerrainEditReview( evidence ) && cityValid;
                if( m_assets.openCity && m_assets.cityNaturalRoads && !m_assets.cityRoadShowcase )
                {
                    const auto &layout = m_assets.cityLayout;
                    std::array<size_t, 3> edges{}, sourceWidths{};
                    for( const auto &edge : layout.roads.edges )
                    {
                        const auto type = static_cast<size_t>( edge.roadClass );
                        if( type < edges.size() ) ++edges[type];
                    }
                    if( m_assets.roadAuthoring )
                        for( const auto &road : m_assets.roadAuthoring->getSource().roads )
                        {
                            if( std::abs( road.width - 7.f ) < .001f ) ++sourceWidths[0];
                            if( std::abs( road.width - 9.f ) < .001f ) ++sourceWidths[1];
                            if( std::abs( road.width - 12.f ) < .001f ) ++sourceWidths[2];
                        }
                    size_t centralBuildings = 0;
                    for( const auto &lot : layout.lots )
                        if( !lot.park && std::hypot( lot.x, lot.z ) < layout.extent * .34f ) ++centralBuildings;
                    evidence << ",\n  \"roadHierarchy\": {\"generatorRevision\":"
                             << procedural::OpenCityLayout::generatorRevision
                             << ",\"centre\":[0,0],\"localEdges\":" << edges[0]
                             << ",\"collectorEdges\":" << edges[1] << ",\"arterialEdges\":" << edges[2]
                             << ",\"widthsMetres\":[7,9,12],\"sourceRoadsByWidth\":["
                             << sourceWidths[0] << ',' << sourceWidths[1] << ',' << sourceWidths[2]
                             << "],\"centralBuildings\":" << centralBuildings << '}';
                }
                if( m_assets.openCity && !m_assets.cityRoadShowcase )
                {
                    // Verify the maps bound to live scene materials, rather than
                    // treating presence in the downloaded asset pack as usage.
                    SourceBudgetSet<render::IMaterial *> inspectedSurfaces;
                    const auto gameManager = app->getGameManager();
                    const auto activeScene = gameManager->getCurrentScene();
                    bool surfaceMapsReady = true;
                    evidence << ",\n  \"citySurfaceTextures\": {\"materials\":[";
                    for( const auto &actor : gameManager->getActors() )
                    {
                        if( !actor || actor->getScene() != activeScene ) continue;
                        const auto component = actor->getComponent<scene::Material>();
                        const auto material = component ? component->getMaterial() : nullptr;
                        if( !material ) continue;
                        const auto albedo = material->getTexture( 0 );
                        const auto name = albedo ? albedo->getName() : String();
                        constexpr auto marker = "/city-surface/";
                        const auto tag = name.find( marker );
                        if( tag == String::npos || inspectedSurfaces.count( material.get() ) ) continue;
                        const auto start = tag + std::char_traits<char>::length( marker );
                        const auto end = name.find( '/', start );
                        if( end == String::npos || end == start ) continue;
                        const auto asset = name.substr( start, end - start );
                        if( !inspectedSurfaces.empty() ) evidence << ',';
                        inspectedSurfaces.insert( material.get() );
                        u32 ready = 0;
                        Vector2I size;
                        for( const u32 slot : { 0u, 1u, 22u, 3u, 2u } )
                        {
                            const auto texture = material->getTexture( slot );
                            void *native = nullptr;
                            if( texture ) texture->getTextureFinal( &native );
                            if( slot == 0 && texture ) size = texture->getSize();
                            if( native && texture->getSize() == size && size.x >= 256 && size.x <= 1024 && size.y == size.x ) ++ready;
                        }
                        surfaceMapsReady = surfaceMapsReady && ready == 5;
                        evidence << "{\"asset\":" << advanced::jsonString( asset )
                                 << ",\"actor\":" << advanced::jsonString( actor->getName() )
                                 << ",\"size\":[" << size.x << ',' << size.y << "]"
                                 << ",\"uploadedMaps\":" << ready
                                 << ",\"cyclesPerMetre\":" << material->getEditorFloat( "uvTriplanarScale", 1 ) << '}';
                    }
                    evidence << "],\"materialCount\":" << inspectedSurfaces.size()
                             << ",\"bindingsReady\":" << ( surfaceMapsReady && !inspectedSurfaces.empty() ? "true" : "false" ) << '}';
                    cityValid = cityValid && surfaceMapsReady;
                    const auto buildings = checkCityBuildingGeometry( m_assets, m_quality != procedural::VehicleAppearanceQuality::Preview );
                    cityValid = cityValid && buildings.valid;
                    evidence << ",\n  \"cityBuildingGeometry\": "; writeCityBuildingGeometry( evidence, buildings );
                    evidence << ",\n  \"sharedMemoryLedger\": "; writeSharedMemoryLedger( evidence );
                    const auto style = m_assets.cityBuildingStyle;
                    const bool styleInRange = style >= 0 && static_cast<size_t>( style ) < cityBuildingStyles.size();
                    const bool styleApplied = styleInRange && style == std::max( 0, m_cityBuildingStyle );
                    cityValid = cityValid && styleApplied;
                    size_t requestedOverrides = 0;
                    bool overridesApplied = true;
                    for( size_t role = 0; role < cityBuildingMeshRoles.size(); ++role )
                    {
                        if( m_cityBuildingMeshes[role].empty() )
                            continue;
                        ++requestedOverrides;
                        overridesApplied = overridesApplied && m_cityBuildingMeshes[role] == m_assets.cityBuildingMeshes[role] &&
                            m_assets.cityBuildingModulePlacements[role] > 0;
                    }
                    evidence << ",\n  \"cityBuildingKit\": {\"requestedOverrides\":" << requestedOverrides
                             << ",\"style\":" << advanced::jsonString( styleInRange ? cityBuildingStyles[style] : "unknown" )
                             << ",\"styleApplied\":" << ( styleApplied ? "true" : "false" )
                             << ",\"moduleCount\":" << m_assets.cityBuildingModuleCount
                             << ",\"overridesApplied\":" << ( overridesApplied ? "true" : "false" )
                             << ",\"diagnostic\":" << advanced::jsonString( m_assets.cityBuildingKitDiagnostic )
                             << ",\"roles\":{";
                    for( size_t role = 0; role < cityBuildingMeshRoles.size(); ++role )
                    {
                        if( role ) evidence << ',';
                        evidence << '"' << cityBuildingMeshRoles[role] << "\":{\"path\":"
                                 << advanced::jsonString( m_assets.cityBuildingMeshes[role] )
                                 << ",\"placements\":" << m_assets.cityBuildingModulePlacements[role] << '}';
                    }
                    evidence << "}}";
                    if( requestedOverrides && !overridesApplied )
                        WP_LOG_ERROR( "City building mesh override was not applied. " + m_assets.cityBuildingKitDiagnostic );
                    if( m_captureView == "facade" || m_captureView == "building" || m_captureView == "shopfront" ||
                        m_captureView == "entry-route" )
                    {
                        evidence << ",\n  \"cityFacadeCamera\": {\"lotId\":\"" << m_cityFacadeLotId
                                 << "\",\"eye\":[" << m_cityFacadeEye.x << ',' << m_cityFacadeEye.y << ',' << m_cityFacadeEye.z
                                 << "],\"target\":[" << m_cityFacadeTarget.x << ',' << m_cityFacadeTarget.y << ','
                                 << m_cityFacadeTarget.z << "]}";
                        if( m_captureView == "entry-route" )
                        {
                            const auto eye = m_cameraActor->getWorldTransform().getPosition();
                            const bool valid = m_cityFacadeLotId && m_cityEntryWalkEdge.isFinite() &&
                                               ( eye - m_cityFacadeEye ).lengthSquared() < .0001f;
                            cityValid = cityValid && valid;
                            evidence << ",\n  \"cityEntryRouteCamera\":{\"valid\":" << ( valid ? "true" : "false" )
                                     << ",\"nearbySidewalkEdge\":[" << m_cityEntryWalkEdge.x << ','
                                     << m_cityEntryWalkEdge.y << ',' << m_cityEntryWalkEdge.z << "]}";
                            auto physics = app->getPhysicsManager()->getPhysicsScene();
                            size_t connectorHits = 0, sidewalkHits = 0;
                            if( valid && physics )
                            {
                                std::array<SmartPtr<scene::Rigidbody>, 4> bodies{};
                                const std::array<const char *, 4> names{ "Terrain city pedestrian connector paving",
                                    "Terrain city pedestrian connector stone edging", "Terrain city pedestrian connector drainage",
                                    "Terrain city footways" };
                                for( auto actor : m_assets.actors )
                                    if( actor )
                                        for( size_t i = 0; i < names.size(); ++i )
                                            if( actor->getName() == names[i] ) bodies[i] = actor->getComponent<scene::Rigidbody>();
                                const auto entry = ( m_cityFacadeTarget - Vector3F( 0, .7f, 0 ) -
                                                     m_cityEntryWalkEdge * .48f ) / .52f;
                                const auto across = Vector3F( m_cityEntryWalkEdge.z - entry.z, 0,
                                                               entry.x - m_cityEntryWalkEdge.x ).normaliseCopy();
                                for( const float radius : { .40f, .50f, .60f, .70f, .80f, .90f, 1.0f, 1.10f } )
                                    for( const float offset : { -2.4f, -1.8f, -1.2f, -.6f, 0.f, .6f, 1.2f, 1.8f, 2.4f } )
                                    {
                                        const auto point = entry + ( m_cityEntryWalkEdge - entry ) * radius + across * offset;
                                        Vector3F hit, normal;
                                        SmartPtr<ISharedObject> object;
                                        if( !physics->intersects( point + Vector3F( 0, 3, 0 ), point - Vector3F( 0, 4, 0 ),
                                                                  hit, normal, object ) || normal.y <= .65f ) continue;
                                        for( size_t i = 0; i < bodies.size(); ++i )
                                        {
                                            if( !bodies[i] || object != bodies[i]->getRigidStatic() ) continue;
                                            ++( i == bodies.size() - 1 ? sidewalkHits : connectorHits );
                                            break;
                                        }
                                    }
                            }
                            evidence << ",\n  \"cityEntrancePathProbes\":{\"connectorHits\":" << connectorHits
                                     << ",\"sidewalkHits\":" << sidewalkHits << '}';
                        }
                        if( m_captureView == "shopfront" )
                        {
#if WP_GRAPHICS_SYSTEM_CLAW
                            size_t roomBatches = 0, readyRoomBatches = 0, glassBatches = 0, readyGlassBatches = 0;
                            for( auto actor : m_assets.actors )
                            {
                                if( !actor ) continue;
                                const auto name = actor->getName();
                                const bool glass = name == "LOD0 city commercial display glazing";
                                if( !glass && name.find( "LOD0 city commercial display room " ) != 0 ) continue;
                                ++( glass ? glassBatches : roomBatches );
                                const auto renderer = actor->getComponent<scene::MeshRenderer>();
                                const auto mesh = renderer ? dynamic_pointer_cast<render::ClawMesh>( renderer->getGraphicsObject() ) : nullptr;
                                if( !mesh || !mesh->isLoaded() || !mesh->getNativeMesh() ||
                                    !wp_graphics_mesh_get_index_count( mesh->getNativeMesh() ) ) continue;
                                if( !glass )
                                {
                                    ++readyRoomBatches;
                                    continue;
                                }
                                const auto component = actor->getComponent<scene::Material>();
                                const auto material = component ? dynamic_pointer_cast<render::ClawMaterial>( component->getMaterial() ) : nullptr;
                                const auto native = material ? material->getNativeMaterial() : nullptr;
                                if( !native ) continue;
                                const auto alpha = wp_graphics_material_get_diffuse( native ).a;
                                if( alpha > 0 && alpha < 1 &&
                                    wp_graphics_material_has_flag( native, WORKPHONE_MATERIAL_FLAG_TRANSPARENT ) &&
                                    wp_graphics_material_has_flag( native, WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK ) &&
                                    !wp_graphics_material_has_flag( native, WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE ) &&
                                    wp_graphics_material_get_blend_mode( native ) == WORKPHONE_BLEND_MODE_ALPHA &&
                                    renderer->getCastShadows() == scene::Renderer::CastShadows::Off &&
                                    material->getTexture( static_cast<u32>( PbsTextureTypes::PBSM_REFLECTION ) ) )
                                    ++readyGlassBatches;
                            }
                            evidence << ",\n  \"cityCommercialDisplays\":{\"roomBatches\":" << roomBatches
                                     << ",\"readyRoomBatches\":" << readyRoomBatches << ",\"glassBatches\":" << glassBatches
                                     << ",\"readyGlassBatches\":" << readyGlassBatches << '}';
#endif
                            const auto &lots = m_assets.cityLayout.lots;
                            const auto lot = std::find_if( lots.begin(), lots.end(), [&]( const auto &item ) {
                                return item.id == m_cityFacadeLotId && !item.park &&
                                       item.district == procedural::OpenCityLayout::District::Commercial;
                            } );
                            const auto eye = m_cameraActor->getWorldTransform().getPosition();
                            const bool valid = lot != lots.end() && ( eye - m_cityFacadeEye ).lengthSquared() < .0001f;
                            cityValid = cityValid && valid;
                            size_t stepHits = 0;
                            const auto stepActor = std::find_if( m_assets.actors.begin(), m_assets.actors.end(),
                                []( const auto &actor ) { return actor && actor->getName() == "Terrain city raised entrance steps"; } );
                            auto physics = app->getPhysicsManager()->getPhysicsScene();
                            if( valid && physics && stepActor != m_assets.actors.end() )
                            {
                                const auto rigid = ( *stepActor )->getComponent<scene::Rigidbody>();
                                const auto body = rigid ? rigid->getRigidStatic() : nullptr;
                                const auto direction = eye - m_cityFacadeTarget;
                                const auto outward = std::abs( direction.x ) > std::abs( direction.z )
                                    ? Vector3F( std::copysign( 1.f, direction.x ), 0, 0 )
                                    : Vector3F( 0, 0, std::copysign( 1.f, direction.z ) );
                                const Vector3F side( -outward.z, 0, outward.x );
                                const auto sill = m_cityFacadeTarget - Vector3F( 0, 1.78f, 0 );
                                for( const float along : { 0.f, -.77f, .77f, -1.48f, 1.48f } )
                                    for( const float distance : { .42f, .79f, 1.23f, 1.64f, 2.17f, 2.83f, 3.64f } )
                                    {
                                        const auto p = sill + outward * distance + side * along;
                                        Vector3F hit, normal;
                                        SmartPtr<ISharedObject> object;
                                        if( body && physics->intersects( p + Vector3F( 0, .35f, 0 ),
                                            p - Vector3F( 0, 1.75f, 0 ), hit, normal, object ) &&
                                            object == body && normal.y > .65f ) ++stepHits;
                                    }
                            }
                            evidence << ",\n  \"cityShopfrontCameraValid\":" << ( valid ? "true" : "false" )
                                     << ",\n  \"cityShopfrontStepHits\":" << stepHits;
                        }
                    }
                    if( m_captureView == "corner" && !m_assets.circuit.samples.empty() )
                    {
                        const auto pose = cityRoadInspectionView( m_assets );
                        evidence << ",\n  \"cityRoadInspectionCamera\": {\"eye\":[" << pose[0].x << ',' << pose[0].y << ',' << pose[0].z
                                 << "],\"target\":[" << pose[1].x << ',' << pose[1].y << ',' << pose[1].z << "]}";
                    }
                    if( m_captureView == "junction" )
                    {
                        evidence << ",\n  \"cityConnectionCamera\": {\"eye\":[" << m_cityConnectionEye.x << ',' << m_cityConnectionEye.y << ',' << m_cityConnectionEye.z
                                 << "],\"target\":[" << m_cityConnectionTarget.x << ',' << m_cityConnectionTarget.y << ',' << m_cityConnectionTarget.z << "]}";
                    }
                    if( m_captureView == "hillside" )
                    {
                        evidence << ",\n  \"cityHillsideCamera\":{\"roadId\":" << advanced::jsonString( m_cityHillsideRoadId )
                                 << ",\"selectedRoadRiseMetres\":" << m_cityHillsideRise
                                 << ",\"eye\":[" << m_cityHillsideEye.x << ',' << m_cityHillsideEye.y << ',' << m_cityHillsideEye.z
                                 << "],\"target\":[" << m_cityHillsideTarget.x << ',' << m_cityHillsideTarget.y << ',' << m_cityHillsideTarget.z << "]}";
                    }
                    if( m_captureView == "frontage" && m_assets.cityTerrainRoads && !m_orbitCamera )
                    {
                        const auto eye = m_cityHasJunction ? m_cityFrontageEye :
                                         m_cityStreetCentre + Vector3F( 0, 4, 0 );
                        const auto target = m_cityHasJunction ? m_cityFrontageTarget :
                            m_cityStreetCentre + m_cityStreetForward * 40.f + Vector3F( 0, 5, 0 );
                        evidence << ",\n  \"cityFrontageCamera\":{\"eye\":[" << eye.x << ',' << eye.y << ',' << eye.z
                                 << "],\"target\":[" << target.x << ',' << target.y << ',' << target.z << "]}";
                    }
                }
                if(m_assets.cityTerrainRoads)
                {
                    const auto check=checkTerrainCity(m_assets,app->getPhysicsManager()->getPhysicsScene());cityValid=cityValid && check.valid;
                    size_t vergeBatches = 0, vergeReady = 0, vergeTriangles = 0;
                    size_t transitions = 0, transitionsReady = 0, transitionTriangles = 0;
                    for( const auto &actor : m_assets.actors )
                    {
                        if( !actor || actor->getName().find( "Terrain city verge " ) != 0 ) continue;
                        const bool transition = actor->getName().find( "Terrain city verge transition " ) == 0;
                        ++( transition ? transitions : vergeBatches );
                        const auto renderer = actor->getComponent<scene::MeshRenderer>();
                        const auto graphics = renderer ? renderer->getGraphicsObject() : nullptr;
#if WP_GRAPHICS_SYSTEM_CLAW
                        const auto mesh = dynamic_pointer_cast<render::ClawMesh>( graphics );
                        const auto native = mesh ? mesh->getNativeMesh() : nullptr;
                        if( native && mesh->isLoaded() )
                        {
                            ++( transition ? transitionsReady : vergeReady );
                            ( transition ? transitionTriangles : vergeTriangles ) += wp_graphics_mesh_get_index_count( native ) / 3;
                        }
#else
                        if( graphics && graphics->isLoaded() ) ++( transition ? transitionsReady : vergeReady );
#endif
                    }
                    evidence << ",\n  \"cityVerges\":{\"batches\":" << vergeBatches
                             << ",\"readyBatches\":" << vergeReady << ",\"nativeTriangles\":" << vergeTriangles
                             << ",\"transitionBatches\":" << transitions << ",\"readyTransitionBatches\":" << transitionsReady
                             << ",\"transitionTriangles\":" << transitionTriangles << '}';
                    if( isRoadTerrainEditView( m_captureView ) || m_captureView == "verge" )
                        cityValid = cityValid && vergeBatches > 0 && vergeReady == vergeBatches &&
                                    transitions > 0 && transitionsReady == transitions;
                    if( m_captureView == "verge" )
                        evidence << ",\n  \"cityVergeCamera\":{\"roadIndex\":" << m_cityVergeRoad
                                 << ",\"eye\":[" << m_cityVergeEye.x << ',' << m_cityVergeEye.y << ',' << m_cityVergeEye.z
                                 << "],\"target\":[" << m_cityVergeTarget.x << ',' << m_cityVergeTarget.y << ',' << m_cityVergeTarget.z << "]}";
                    if( m_captureView == "park" )
                    {
                        const auto eye = m_cameraActor->getWorldTransform().getPosition();
                        const auto lot = std::find_if( m_assets.cityLayout.lots.begin(), m_assets.cityLayout.lots.end(),
                            [&]( const auto &item ) { return item.park && item.id == m_cityParkLotId; } );
                        const bool valid = lot != m_assets.cityLayout.lots.end() &&
                                           ( eye - m_cityParkEye ).lengthSquared() < .0001f;
                        cityValid = cityValid && valid;
                        evidence << ",\n  \"cityParkCamera\":{\"valid\":" << ( valid ? "true" : "false" )
                                 << ",\"lotId\":\"" << m_cityParkLotId << "\",\"eye\":[" << eye.x << ',' << eye.y << ',' << eye.z
                                 << "],\"target\":[" << m_cityParkTarget.x << ',' << m_cityParkTarget.y << ','
                                 << m_cityParkTarget.z << "]}";
                    }
                    if( m_captureView == "utilities" )
                    {
                        const auto eye = m_cameraActor->getWorldTransform().getPosition();
                        const auto vector = [&]( Vector3F p ) { evidence << '[' << p.x << ',' << p.y << ',' << p.z << ']'; };
                        float roadHeight = 0;
                        bool driveable = false;
                        const bool present = m_cityUtilityCovers > 0 && m_assets.roadAuthoring->getRuntimeSurface(
                            m_cityUtilityPrimary.x, m_cityUtilityPrimary.z, roadHeight, driveable ) && driveable &&
                            std::abs( m_cityUtilityPrimary.y - roadHeight ) < .065f;
                        const bool coverReady = utilityNativeReady( m_assets, utilityCoverActor );
                        const bool grateReady = m_cityUtilityHasGrate && utilityNativeReady( m_assets, utilityGrateActor );
                        const bool valid = present && coverReady && ( !m_cityUtilityHasGrate || grateReady ) &&
                            ( eye - m_cityUtilityEye ).lengthSquared() < .0001f;
                        cityValid = cityValid && valid;
                        evidence << ",\n  \"cityUtilitiesCamera\":{\"valid\":" << ( valid ? "true" : "false" )
                                 << ",\"primaryPresent\":" << ( present ? "true" : "false" )
                                 << ",\"nativeReady\":" << ( coverReady ? "true" : "false" )
                                 << ",\"grateNativeReady\":" << ( grateReady ? "true" : "false" )
                                 << ",\"coverCandidates\":" << m_cityUtilityCovers
                                 << ",\"grateCandidates\":" << m_cityUtilityGrates << ",\"eye\":";
                        vector( eye );
                        evidence << ",\"target\":"; vector( m_cityUtilityTarget );
                        evidence << ",\"primary\":"; vector( m_cityUtilityPrimary );
                        evidence << ",\"nearbyGrate\":";
                        if( m_cityUtilityHasGrate ) vector( m_cityUtilityGrate );
                        else evidence << "null";
                        evidence << '}';
                    }
                    const auto expectedDetail = m_cityStreetDetail >= 0 ? m_cityStreetDetail : 100;
                    const bool expectStreetDetail = m_cityStreetDetail == 100 && m_seed == 42 &&
                        m_cityBlocks == 6 && m_cityRoute == 0 && m_quality == procedural::VehicleAppearanceQuality::High;
                    size_t streetDetailActors = 0;
                    bool streetDetailRenderersReady = true;
                    for( const auto &actor : m_assets.actors )
                    {
                        if( !actor || actor->getName().find( "Terrain city street detail" ) != 0 )
                            continue;
                        ++streetDetailActors;
                        const auto renderer = actor->getComponent<scene::MeshRenderer>();
                        streetDetailRenderersReady = streetDetailRenderersReady && renderer && renderer->getGraphicsObject();
                    }
                    const bool streetDetailValid = m_assets.cityStreetDetail == expectedDetail &&
                        (expectedDetail != 0 || m_assets.cityStreetDetailCount == 0) &&
                        (!expectStreetDetail || m_assets.cityStreetDetailCount > 0) &&
                        ((m_assets.cityStreetDetailCount > 0) == (streetDetailActors > 0)) && streetDetailRenderersReady;
                    cityValid = cityValid && streetDetailValid;
                    if( !streetDetailValid )
                        WP_LOG_ERROR( "Terrain city street detail does not match the requested setting or fixture." );
                    auto authoring=m_assets.roadAuthoring;const auto snapshot=m_assets.roadTerrain?m_assets.roadTerrain->getTerrainSnapshot():nullptr;
                    if(authoring)cityValid=writeCityRoadGeometry(evidence,*authoring) && cityValid;
                    if(authoring)cityValid=writeCityConnections(evidence,*authoring,app->getPhysicsManager()->getPhysicsScene()) && cityValid;
                    if(authoring)cityValid=writeCityTerrainInteraction(evidence,*authoring,app->getPhysicsManager()->getPhysicsScene()) && cityValid;
                    const auto &layout=m_assets.cityLayout;size_t parks=0;for(const auto &lot:layout.lots)parks+=lot.park?1:0;
                    evidence<<",\n  \"terrainCity\": {\"sourceId\":"<<advanced::jsonString(authoring?String(authoring->getSource().id.data(),authoring->getSource().id.size()):String())
                        <<",\"terrainBindingUUID\":"<<advanced::jsonString(authoring?authoring->getTerrainBindingUUID():String())
                        <<",\"bindingValid\":"<<(check.binding?"true":"false")<<",\"sourceReused\":"<<(m_assets.citySourceReused?"true":"false")
                        <<",\"terrainReliefRequested\":"<<m_cityTerrainRelief<<",\"terrainReliefSetting\":"<<m_assets.cityTerrainRelief
                        <<",\"terrainFitted\":"<<(m_assets.cityTerrainFitted?"true":"false")
                        <<",\"terrainFit\":{\"changedSamples\":"<<m_assets.cityTerrainFitReport.changedSamples
                        <<",\"maximumCutMetres\":"<<m_assets.cityTerrainFitReport.maxCut
                        <<",\"maximumFillMetres\":"<<m_assets.cityTerrainFitReport.maxFill
                        <<",\"maximumRoadGrade\":"<<m_assets.cityTerrainFitReport.maxRoadGrade
                        <<",\"roadHeightSpanMetres\":"<<m_assets.cityTerrainFitReport.roadHeightSpan
                        <<",\"attempts\":"<<m_assets.cityTerrainFitReport.attempts<<'}'
                        <<",\"sourceRevision\":"<<(authoring?authoring->getSourceRevision():0)<<",\"appliedSourceRevision\":"<<m_assets.cityAppliedSourceRevision
                        <<",\"activeRevision\":"<<(authoring?authoring->getActiveRevision():0)<<",\"terrainRevision\":"<<(snapshot?snapshot->revision:0)
                        <<",\"sourceBytes\":"<<(authoring?authoring->getSourceJSON().size():0)<<",\"roads\":"<<check.roads<<",\"roundabouts\":"<<check.junctions
                        <<",\"runtimeTriangles\":"<<check.triangles<<",\"terrainRendererReady\":"<<(check.renderer?"true":"false")
                        <<",\"groundAlbedo\":"<<advanced::jsonString(snapshot?snapshot->layers[0].albedo:String())
                        <<",\"groundNormal\":"<<advanced::jsonString(snapshot?snapshot->layers[0].normal:String())
                        <<",\"groundORM\":"<<advanced::jsonString(snapshot?snapshot->layers[0].orm:String())
                        <<",\"groundSourceRoot\":"<<advanced::jsonString(app->getProjectPath())
                        <<",\"roadPBRBindingsReady\":"<<(check.roadPBRBindingsReady?"true":"false")<<",\"roadPBRTextures\":"<<check.roadPBRTextures<<",\"roadPBRMaterials\":"<<check.roadPBRMaterials
                        <<",\"asphaltAlbedoWidth\":"<<check.asphaltAlbedoWidth<<",\"markingAlphaBlend\":"<<(check.markingAlphaBlend?"true":"false")
                        <<",\"footwayReady\":"<<(check.footwayReady?"true":"false")<<",\"footwayCollisionReady\":"<<(check.footwayCollisionReady?"true":"false")
                        <<",\"footwayActors\":"<<check.footwayActors<<",\"footwayNativeRayProbes\":"<<check.footwayNativeRayProbes<<",\"footwayRayCandidates\":"<<check.footwayRayCandidates<<",\"footwayRayHeightError\":"<<check.footwayRayHeightError
                        <<",\"routeSamples\":"<<check.routePoints<<",\"routeDriveable\":"<<(check.route?"true":"false")<<",\"routeMaximumHeightError\":"<<check.routeHeightError
                        <<",\"raceRouteAvailable\":"<<(m_assets.cityRaceRouteAvailable?"true":"false")<<",\"raceRouteError\":"<<advanced::jsonString(m_assets.cityRaceRouteError)
                        <<",\"islandCentresChecked\":"<<check.islandsChecked<<",\"islandsExcluded\":"<<(check.islands?"true":"false")
                        <<",\"nativeRayProbesPassed\":"<<check.rayProbes<<",\"flatFallbackAbsent\":"<<(check.flatFallbackAbsent?"true":"false")
                        <<",\"fittedBoundaries\":"<<check.fittedBoundaries<<",\"boundaryCollisionProbes\":"<<check.boundaryCollisionProbes
                        <<",\"roadRayCurveDeviationMetres\":"<<check.roadRayCurveDeviation
                        <<",\"lots\":"<<layout.lots.size()<<",\"buildingLots\":"<<layout.lots.size()-parks<<",\"parkLots\":"<<parks
                        <<",\"buildingCollisionActors\":"<<check.buildingBodies<<",\"expectedBuildingCollisionActors\":"<<check.expectedBuildingBodies
                        <<",\"buildingSupportChecks\":"<<m_assets.cityBuildingSupportChecks<<",\"culledParcels\":"<<m_assets.cityCulledParcels
                        <<",\"cityStreetDetail\":"<<m_assets.cityStreetDetail<<",\"cityStreetDetailCount\":"<<m_assets.cityStreetDetailCount
                        <<",\"streetDetailActors\":"<<streetDetailActors<<",\"streetDetailRenderersReady\":"<<(streetDetailRenderersReady?"true":"false")
                        <<",\"streetDetailPlacementsRequired\":"<<(expectStreetDetail?"true":"false")<<",\"streetDetailValid\":"<<(streetDetailValid?"true":"false")
                        <<",\"buildingSupportValid\":"<<(check.support?"true":"false")<<",\"structuralValidation\":"<<(cityValid?"true":"false")<<"}";
                    evidence << ",\n  \"cityCrossingKerbs\":{\"loweredCrossings\":" << check.loweredCrossings
                             << ",\"raisedFallbacks\":" << check.raisedCrossings
                             << ",\"expectedNativeRays\":" << check.loweredCrossings * 4
                             << ",\"nativeRays\":" << check.crossingKerbRays
                             << ",\"maximumNativeHeightError\":" << check.crossingKerbHeightError
                             << ",\"valid\":" << ( check.footwayCollisionReady && check.crossingKerbRays == check.loweredCrossings * 4
                                                       ? "true" : "false" ) << '}';
                    if( snapshot )
                    {
                        std::array<double, 4> totals{};
                        const auto samples = snapshot->layerWeights.size();
                        if( !samples ) totals[0] = 1.0;
                        for( const auto &weights : snapshot->layerWeights )
                            for( s32 layer = 0; layer < 4; ++layer ) totals[layer] += weights[layer];
                        evidence << ",\n  \"cityGroundLayers\":[";
                        for( size_t i = 0; i < snapshot->layers.size(); ++i )
                        {
                            const auto &layer = snapshot->layers[i];
                            if( i ) evidence << ',';
                            evidence << "{\"index\":" << i
                                     << ",\"meanWeight\":" << totals[i] / std::max<size_t>( 1, samples )
                                     << ",\"albedo\":" << advanced::jsonString( layer.albedo )
                                     << ",\"normal\":" << advanced::jsonString( layer.normal )
                                     << ",\"orm\":" << advanced::jsonString( layer.orm ) << '}';
                        }
                        evidence << ']';
                    }
                    if(!cityValid)WP_LOG_ERROR("Terrain city: source, fitted route, native collision, building support or uploaded road PBR bindings validation failed.");
                }
                else if(m_assets.cityRoadShowcase)
                {
                    auto authoring=m_assets.roadAuthoring;auto terrain=m_assets.roadTerrain;
                    const auto snapshot=terrain?terrain->getTerrainSnapshot():nullptr;
                    const auto activeTerrain=terrain?terrain->getActiveTerrainSnapshot():nullptr;
                    const bool terrainRenderReady=terrain && terrain->getTerrain() && activeTerrain && snapshot && activeTerrain->revision==snapshot->revision;
                    const auto physicsManager=app->getPhysicsManager();auto physics=physicsManager?physicsManager->getPhysicsScene():nullptr;
                    const size_t triangles=authoring?authoring->getRuntimeTriangleCount():0;
                    bool probesValid=bool(authoring && snapshot && physics);size_t probeCount=0;
                    bool islandDriveable=true;f32 islandHeight=0;bool islandValid=authoring && authoring->getRuntimeSurface(0,0,islandHeight,islandDriveable) && !islandDriveable;
                    const auto probe=[&](Vector3F p,bool expectedDriveable,bool terrainOnly)
                    {
                        f32 height=0;bool driveable=false;
                        bool valid=terrainOnly?render::sampleTerrainHeight(*snapshot,Transform3<real_Num>(),p,height):authoring->getRuntimeSurface(p.x,p.z,height,driveable);
                        if(!terrainOnly)valid=valid && driveable==expectedDriveable;
                        Vector3F hit,normal;SmartPtr<ISharedObject> object;
                        valid=valid && physics->intersects({p.x,height+5,p.z},{p.x,height-5,p.z},hit,normal,object) && object && std::isfinite(hit.y) && std::abs(hit.y-height)<.035f && normal.y>.5f;
                        if(valid)++probeCount;probesValid=probesValid && valid;
                    };
                    if(probesValid)
                    {
                        probe({11,0,0},true,false);probe({0,0,0},false,false);probe({-70,0,-70},false,true);
                        const auto &bake=authoring->getBake();
                        if(!bake.roads.empty() && !bake.roads.front().points.empty())probe(bake.roads.front().points[bake.roads.front().points.size()/2].position,true,false);
                        else probesValid=false;
                    }
                    bool flatFallbackAbsent=m_assets.ground && terrain && terrain->getActor()==m_assets.ground;
                    for(const auto &actor:m_assets.actors)if(actor && actor->getName()=="Circuit collision plane")flatFallbackAbsent=false;
                    const bool revisions=authoring && snapshot && authoring->getSourceRevision()==authoring->getActiveRevision() && authoring->getSource().terrainRevision==snapshot->revision;
                    bool circulationMatchesSource=authoring && authoring->getSource().roundabouts.size()==1 && m_assets.circuit.samples.size()>=3;
                    bool routeSurfaceHeightsMatch=circulationMatchesSource;
                    if(circulationMatchesSource)
                    {
                        const auto &junction=authoring->getSource().roundabouts.front();
                        for(size_t i=0;i<m_assets.circuit.samples.size();++i)
                        {
                            const auto p=m_assets.circuit.samples[i].position,q=m_assets.circuit.samples[(i+1)%m_assets.circuit.samples.size()].position;
                            const float crossY=(p-junction.center).crossProduct(q-junction.center).y;
                            circulationMatchesSource=circulationMatchesSource && std::isfinite(crossY) && (junction.clockwise?crossY<-.0001f:crossY>.0001f);
                            f32 height=0;bool driveable=false;
                            routeSurfaceHeightsMatch=routeSurfaceHeightsMatch && authoring->getRuntimeSurface(p.x,p.z,height,driveable) && driveable && std::isfinite(p.y) && std::abs(p.y-height)<=.001f;
                        }
                    }
                    cityValid=cityValid && revisions && terrainRenderReady && triangles>0 && authoring->getSource().roads.size()==4 && authoring->getSource().roundabouts.size()==1 &&
                        probesValid && probeCount==4 && islandValid && flatFallbackAbsent && circulationMatchesSource && routeSurfaceHeightsMatch;
                    evidence<<",\n  \"roadAuthoring\": {\"sourceRevision\":"<<(authoring?authoring->getSourceRevision():0)
                        <<",\"activeRevision\":"<<(authoring?authoring->getActiveRevision():0)<<",\"terrainRevision\":"<<(snapshot?snapshot->revision:0)
                        <<",\"roads\":"<<(authoring?authoring->getSource().roads.size():0)<<",\"roundabouts\":"<<(authoring?authoring->getSource().roundabouts.size():0)
                        <<",\"runtimeTriangles\":"<<triangles<<",\"terrainSamples\":"<<(snapshot?snapshot->heights.size():0)
                        <<",\"terrainActiveRevision\":"<<(activeTerrain?activeTerrain->revision:0)<<",\"terrainRendererReady\":"<<(terrainRenderReady?"true":"false")
                        <<",\"nativeRayProbesPassed\":"<<probeCount<<",\"nativeRayProbesValid\":"<<(probesValid?"true":"false")
                        <<",\"islandDriveable\":"<<(islandDriveable?"true":"false")<<",\"islandHeight\":"<<islandHeight
                        <<",\"flatFallbackAbsent\":"<<(flatFallbackAbsent?"true":"false")<<",\"foliageAvailability\":\"unavailable-in-fixture\""
                        <<",\"circulationMatchesSource\":"<<(circulationMatchesSource?"true":"false")<<",\"routeSurfaceHeightsMatch\":"<<(routeSurfaceHeightsMatch?"true":"false")
                        <<",\"structuralValidation\":"<<(cityValid?"true":"false")<<"}";
                    if(!cityValid)WP_LOG_ERROR("Road authoring showcase: runtime, revision, terrain or native collision validation failed.");
                }
                else if( m_assets.openCity )
                {
                    const auto city = procedural::OpenCityLayout::generate( m_seed, m_assets.cityBlocks, m_assets.cityRoute,
                                                                            m_assets.cityNaturalRoads );
                    size_t parkLots = 0, buildingLots = 0, buildingCollisions = 0, expectedBuildingCollisions = 0;
                    size_t cityLODGroups = 0, cityLODNear = 0, cityLODFar = 0;
                    bool footprintValid = true, routeValid = m_assets.circuit.samples.size() >= 3;
                    bool cityLODValid = true;
                    for( const auto &lot : city.lots )
                    {
                        if( lot.park ) ++parkLots; else ++buildingLots;
                        expectedBuildingCollisions += procedural::OpenCityLayout::buildingVolumes( lot ).size();
                        const float clearance = city.footprintRoadEdgeClearance( lot.x, lot.z, lot.width, lot.depth );
                        footprintValid = footprintValid && std::isfinite( clearance ) &&
                            clearance >= procedural::OpenCityLayout::roadsideSetback - .001f;
                    }
                    for( size_t i = 0; i < m_assets.circuit.samples.size(); ++i )
                    {
                        const auto p = m_assets.circuit.samples[i].position;
                        const auto q = m_assets.circuit.samples[( i + 1 ) % m_assets.circuit.samples.size()].position;
                        const float step = ( q - p ).length();
                        routeValid = routeValid && std::isfinite( p.x ) && std::isfinite( p.z ) &&
                            city.roadDistance( p.x, p.z ) < .01f && step > 0 && step <= 2.05f;
                    }
                    for( const auto &actor : m_assets.actors )
                    {
                        if( !actor ) continue;
                        const auto name = actor->getName();
                        if( name.find( "Trackside collision: City building mass" ) == 0 ) ++buildingCollisions;
                        if( name.find( "City LOD patch" ) != 0 ) continue;
                        ++cityLODGroups;
                        const auto group = actor->getComponent<scene::LODGroup>();
                        if( !group || !group->validate() ) { cityLODValid = false; continue; }
                        const auto levels = group->getLevels();
                        size_t visibleLevels = 0;
                        for( size_t level = 0; level < levels.size(); ++level )
                        {
                            if( levels[level].renderers.empty() ) { cityLODValid = false; continue; }
                            const bool visible = levels[level].renderers.front()->isLODVisible();
                            for( const auto &renderer : levels[level].renderers )
                                cityLODValid = cityLODValid && renderer && renderer->isLODVisible() == visible;
                            if( visible )
                            {
                                ++visibleLevels;
                                if( level == 0 && levels.size() > 1 ) ++cityLODNear; else ++cityLODFar;
                            }
                        }
                        cityLODValid = cityLODValid && visibleLevels == 1;
                    }
                    cityValid = cityValid && m_assets.cityBlocks == m_cityBlocks && m_assets.cityRoute == m_cityRoute &&
                        m_assets.cityNaturalRoads == m_cityNaturalRoads &&
                        routeValid && footprintValid && buildingLots > 0 && buildingCollisions == expectedBuildingCollisions &&
                        cityLODGroups > 0 && cityLODValid;
                    evidence << ",\n  \"city\": {\"blocks\":" << city.blocks << ",\"route\":" << m_assets.cityRoute
                             << ",\"layout\":" << advanced::jsonString( m_assets.cityNaturalRoads ? "lsystem" : "grid" )
                             << ",\"generatorRevision\":" << procedural::OpenCityLayout::generatorRevision
                             << ",\"roadNetworkNodes\":" << city.roads.nodes.size()
                             << ",\"roadNetworkEdges\":" << city.roads.edges.size()
                             << ",\"spacing\":" << city.spacing << ",\"extent\":" << city.extent
                             << ",\"lots\":" << city.lots.size() << ",\"buildingLots\":" << buildingLots
                             << ",\"parkLots\":" << parkLots << ",\"buildingCollisionActors\":" << buildingCollisions
                             << ",\"expectedBuildingCollisionActors\":" << expectedBuildingCollisions
                             << ",\"routeSamples\":" << m_assets.circuit.samples.size()
                             << ",\"closedRouteOnRoads\":" << ( routeValid ? "true" : "false" )
                             << ",\"footprintsClearRoads\":" << ( footprintValid ? "true" : "false" )
                             << ",\"lodGroups\":" << cityLODGroups << ",\"nearLodGroups\":" << cityLODNear
                             << ",\"farLodGroups\":" << cityLODFar << ",\"lodValid\":" << ( cityLODValid ? "true" : "false" )
                             << ",\"structuralValidation\":" << ( cityValid ? "true" : "false" ) << "}";
                    if( !cityValid ) WP_LOG_ERROR( "VehicleAdvanced: city route, clearance, collision or LOD validation failed." );
                }
                evidence << ",\n  \"device\": " << advanced::captureDeviceEvidence();
                void *reflectionView = nullptr;
                if( m_assets.reflectionTexture ) m_assets.reflectionTexture->getTextureFinal( &reflectionView );
                bool radianceValid = false;
                evidence << ",\n  \"reflectionRadiance\": " << advanced::captureRadianceEvidence( reflectionView, radianceValid );
                if( !radianceValid ) WP_LOG_ERROR( "VehicleAdvanced: HDR reflection readback did not confirm finite radiance above one." );
                evidence << ",\n  \"foliageWorldEnabled\": " << ( m_foliageWorldEnabled ? "true" : "false" );
                bool foliageValid = !m_foliageWorldEnabled;
                if( m_assets.foliageTerrain )
                {
                    const auto foliage = m_assets.foliageTerrain->getFoliageDiagnostics();
                    foliageValid = foliage.currentGenerationReady && foliage.failedPages == 0 && foliage.error.empty();
                    evidence << ",\n  \"foliage\": {\"ready\": " << ( foliage.currentGenerationReady ? "true" : "false" )
                             << ", \"residentPages\": " << foliage.residentPages
                             << ", \"pendingPages\": " << foliage.pendingPages
                             << ", \"failedPages\": " << foliage.failedPages
                             << ", \"residentInstances\": " << foliage.residentInstances
                             << ", \"testedInstances\": " << foliage.testedInstances
                             << ", \"culledInstances\": " << foliage.culledInstances
                             << ", \"lodInstances\": [" << foliage.lodInstances[0] << ',' << foliage.lodInstances[1] << ',' << foliage.lodInstances[2] << ']'
                             << ", \"actualDraws\": " << foliage.actualDraws
                             << ", \"geometryBytes\": " << foliage.geometryBytes
                             << ", \"uploadBytes\": " << foliage.uploadBytes
                             << ", \"error\": " << advanced::jsonString( foliage.error ) << "}";
                    if( const auto world = m_assets.foliageTerrain->getFoliageSnapshot();
                        m_assets.cityTerrainRoads && world )
                    {
                        const auto source = m_assets.roadAuthoring;
                        const auto terrain = source ? source->getTerrainSnapshot() : nullptr;
                        size_t trees = 0, shrubs = 0, grasses = 0;
                        size_t terrainRoots = 0, parkRoots = 0;
                        float deepestRoot = 0;
                        for( const auto &instance : world->instances )
                        {
                            const auto kind = world->species[instance.species]->kind;
                            trees += kind == render::FoliageKind::Tree;
                            shrubs += kind == render::FoliageKind::Bush;
                            grasses += kind == render::FoliageKind::Grass;
                            if( !terrain ) continue;

                            const auto p = instance.position;
                            if( m_assets.useFoliageWorld && kind == render::FoliageKind::Tree &&
                                std::any_of( m_assets.cityLayout.lots.begin(), m_assets.cityLayout.lots.end(),
                                    [&]( const auto &lot ) {
                                        return lot.park && std::abs( p.x - lot.parcelX ) <= lot.parcelWidth * .5f &&
                                               std::abs( p.z - lot.parcelZ ) <= lot.parcelDepth * .5f &&
                                               std::abs( p.y - ( lot.groundHeight + .16f ) ) <= .01f;
                                    } ) )
                            {
                                ++parkRoots;
                                continue;
                            }
                            float height = 0;
                            if( render::sampleTerrainHeight( *terrain, source->getTerrainPlacement(), p, height ) &&
                                p.y <= height + .015f && p.y >= height - .15f )
                            {
                                ++terrainRoots;
                                deepestRoot = std::max( deepestRoot, height - p.y );
                            }
                        }
                        evidence << ",\n  \"cityFoliagePopulation\": {\"trees\":" << trees
                                 << ",\"shrubs\":" << shrubs << ",\"grassTufts\":" << grasses
                                 << ",\"species\":" << world->species.size() << '}';
                        const auto failedRoots = world->instances.size() - terrainRoots - parkRoots;
                        const bool rootsValid = terrain && !failedRoots;
                        evidence << ",\n  \"cityFoliageRoots\": {\"terrain\":" << terrainRoots
                                 << ",\"parkPads\":" << parkRoots << ",\"failed\":" << failedRoots
                                 << ",\"maximumTerrainInsetMetres\":" << deepestRoot
                                 << ",\"valid\":" << ( rootsValid ? "true" : "false" ) << '}';
                        foliageValid = foliageValid && rootsValid;
                        if( !rootsValid ) WP_LOG_ERROR( "Terrain city: foliage root height does not match terrain or park support." );
                    }
                }
#if WP_GRAPHICS_SYSTEM_CLAW
                // Consume the completed render-task snapshot: querying the C system
                // directly from the application task would race frame publication.
                const auto graphicsProperties = app->getGraphicsSystem()->getProperties();
                u32 nativeUpdates = 0, nativeRenders = 0;
                s32 completedPasses = 0, skippedPasses = 0;
                nativeFrameValid = graphicsProperties &&
                    graphicsProperties->getPropertyValue( "native_update_serial", nativeUpdates ) &&
                    graphicsProperties->getPropertyValue( "native_render_serial", nativeRenders ) &&
                    graphicsProperties->getPropertyValue( "native_completed_passes", completedPasses ) &&
                    graphicsProperties->getPropertyValue( "native_skipped_passes", skippedPasses ) &&
                    nativeUpdates > 0 && nativeUpdates == nativeRenders && completedPasses > 0;
                std::ostringstream nativeReport;
                nativeReport << "VehicleAdvanced native frame: updates=" << nativeUpdates
                             << " renders=" << nativeRenders << " completedPasses=" << completedPasses
                             << " skippedPasses=" << skippedPasses
                             << " valid=" << ( nativeFrameValid ? "true" : "false" );
                WP_LOG( nativeReport.str() );
                evidence << ",\n  \"nativeUpdates\": " << nativeUpdates
                         << ",\n  \"nativeRenders\": " << nativeRenders
                         << ",\n  \"nativeCompletedPasses\": " << completedPasses
                         << ",\n  \"nativeSkippedPasses\": " << skippedPasses;
                if( !nativeFrameValid )
                    WP_LOG_ERROR( "VehicleAdvanced: expected one native update per rendered frame and an active target." );
#endif
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
                if( auto renderer = dynamic_cast<render::ClawRendererDX11 *>(
                        app->getGraphicsSystem()->getRendererPtr() ) )
                {
                    wp_render_statistics_dx11 stats{};
                    wp_renderer_dx11_get_statistics(
                        wp_renderer_get_dx11( renderer->getNativeRenderer() ), &stats );
                    const auto frames = double( std::max<uint64_t>( stats.frames, 1 ) );
                    std::ostringstream report;
                    report << "VehicleAdvanced benchmark: frames=" << stats.frames
                           << " gpuSamples=" << stats.gpu_samples << " meanMs=" << stats.interval_ms
                           << " p95Ms=" << stats.interval_p95_ms
                           << " cpuIncludingPresentMs=" << stats.cpu_frame_ms
                           << " presentMs=" << stats.present_ms << " gpuMs=" << stats.gpu_frame_ms
                           << " drawsPerFrame=" << stats.draws / frames
                           << " trianglesPerFrame=" << stats.triangles / frames
                           << " materialUploadsPerFrame=" << stats.material_uploads / frames
                           << " transformUploadsPerFrame=" << stats.transform_uploads / frames
                           << " stateBindingsPerFrame=" << stats.state_bindings / frames;
                    WP_LOG( report.str() );
                    const auto native = wp_renderer_get_dx11( renderer->getNativeRenderer() );
                    const auto hdrPasses = wp_renderer_dx11_get_hdr_completed_frames( native );
                    u32 actualSamples = 1;
                    if( auto depth = static_cast<ID3D11Texture2D *>(
                            wp_renderer_dx11_get_scene_depth_texture( native ) ) )
                    {
                        D3D11_TEXTURE2D_DESC descriptor{};
                        depth->GetDesc( &descriptor );
                        actualSamples = descriptor.SampleDesc.Count;
                    }
                    sceneSamplingValid = m_configuredSceneSamples == 0 ||
                                         actualSamples == m_configuredSceneSamples;
                    if( !sceneSamplingValid )
                        WP_LOG_ERROR( "VehicleAdvanced: the native scene did not use the configured sample count." );
                    evidence << ",\n  \"sceneSampling\": {\"requestedSamples\": " << m_preferredSceneSamples
                             << ", \"configuredSamples\": " << m_configuredSceneSamples
                             << ", \"actualSamples\": " << actualSamples
                             << ", \"matched\": " << ( sceneSamplingValid ? "true" : "false" ) << '}';
                    const auto pipeline = app->getGraphicsSystem()->getGraphicsPipeline();
                    const bool hdrRequested = pipeline && pipeline->getHdrSettings().m_enableAgxToneMapping;
                    const bool hdrActive = hdrRequested && hdrPasses > 0;
                    if( hdrRequested )
                    {
                        evidence << ",\n  \"linearScene\": " << advanced::captureLinearSceneEvidence(
                            wp_renderer_dx11_get_hdr_scene_view( native ), linearSceneValid );
                        linearSceneValid = linearSceneValid && hdrActive;
                        if( !linearSceneValid ) WP_LOG_ERROR( "VehicleAdvanced: native HDR scene resolve/readback validation failed." );
                    }
                    evidence << ",\n  \"lightingOutputPath\": \""
                             << ( hdrActive ? "dx11-native-hdr-filmic" : "dx11-fixed-reinhard" ) << "\""
                             << ",\n  \"hdrPostPipelineActive\": " << ( hdrActive ? "true" : "false" )
                             << ",\n  \"hdrCompletedPassesLifetime\": " << hdrPasses
                             << ",\n  \"hdrExposure\": 1.0,\n  \"autoExposureActive\": false"
                             << ",\n  \"dx11\": {\"frames\": " << stats.frames
                             << ", \"intervalSamples\": " << stats.interval_samples
                             << ", \"gpuSamples\": " << stats.gpu_samples
                             << ", \"meanIntervalMs\": " << stats.interval_ms
                             << ", \"p95IntervalMs\": " << stats.interval_p95_ms
                             << ", \"cpuIncludingPresentMs\": " << stats.cpu_frame_ms
                             << ", \"presentMs\": " << stats.present_ms
                             << ", \"gpuMs\": " << stats.gpu_frame_ms
                             << ", \"draws\": " << stats.draws
                             << ", \"triangles\": " << stats.triangles
                             << ", \"instancedDraws\": " << stats.instanced_draws
                             << ", \"instances\": " << stats.instances
                             << ", \"instanceUploadBytes\": " << stats.instance_upload_bytes
                             << ", \"geometryCreations\": " << stats.geometry_creations << "}";
                }
#endif
                bool sunlightValid = true;
#if WP_GRAPHICS_SYSTEM_CLAW
                if( auto scene = dynamic_pointer_cast<render::ClawScene>(
                        app->getGraphicsSystem()->getGraphicsScene() ) )
                {
                    const auto light = scene->getPrimaryDirectionalLight();
                    sunlightValid = light && light == m_assets.sun;
                    if( light )
                    {
                        const auto direction = light->getDerivedDirection();
                        const auto power = light->getPowerScale();
                        const auto colour = light->getDiffuseColour();
                        const auto ambient = scene->getAmbientLight();
                        sunlightValid = sunlightValid && std::isfinite( direction.x ) &&
                            std::isfinite( direction.y ) && std::isfinite( direction.z ) &&
                            direction.lengthSquared() > .99f && std::isfinite( power ) && power > 0;
                        evidence << ",\n  \"primarySun\": {\"matchesProceduralSky\": "
                                 << ( sunlightValid ? "true" : "false" ) << ", \"power\": " << power
                                 << ", \"direction\": [" << direction.x << ", " << direction.y
                                 << ", " << direction.z << "], \"colour\": [" << colour.r << ", "
                                 << colour.g << ", " << colour.b << "], \"ambient\": [" << ambient.r
                                 << ", " << ambient.g << ", " << ambient.b << "]}";
                    }
                    if( m_assets.openCity && m_assets.sky && scene->getNativeScene() )
                    {
                        const auto native = scene->getNativeScene();
                        const auto fog = wp_graphics_scene_get_fog_colour( native );
                        const auto faces = m_assets.sky->getTextures();
                        const auto faceSize = !faces.empty() && faces[0] ? faces[0]->getSize() : Vector2I();
                        const auto uploadedFaces = std::count_if( faces.begin(), faces.end(), [&]( const auto &face ) {
                            void *texture = nullptr;
                            if( face ) face->getTextureFinal( &texture );
                            return texture && face->getSize() == faceSize;
                        } );
                        evidence << ",\n  \"cityDaylight\":{\"skyFaces\":" << faces.size()
                                 << ",\"uploadedSkyFaces\":" << uploadedFaces
                                 << ",\"skyFaceSize\":[" << faceSize.x << ',' << faceSize.y << ']'
                                 << ",\"fogMode\":" << static_cast<int>( wp_graphics_scene_get_fog_mode( native ) )
                                 << ",\"fogDensity\":" << wp_graphics_scene_get_fog_density( native )
                                 << ",\"fogColour\":[" << fog.r << ',' << fog.g << ',' << fog.b << "]}";
                    }
                    if( !sunlightValid ) WP_LOG_ERROR( "VehicleAdvanced: primary sunlight does not match the generated sky." );
                }
#endif
                const bool reflectionValid = advanced::validateReflection( m_assets );
                if( !reflectionValid )
                    WP_LOG_ERROR(
                        "VehicleAdvanced: cubemap actor, texture or vehicle material binding is "
                        "invalid." );
                const bool windingValid = checkSceneWinding( m_assets );
                m_capturePassed = ( m_capturePath.empty() || advanced::captureFrame( m_capturePath ) ) &&
                                  treeLODValid && vehicleLODValid && reflectionValid && windingValid &&
                                  nativeFrameValid && foliageValid && radianceValid && sunlightValid && linearSceneValid && sceneSamplingValid && cityValid &&
                                  ( !m_terrainGameplay || m_terrainSurfacesValidated );
                evidence << ",\n  \"validationPassed\": " << ( m_capturePassed ? "true" : "false" )
                         << ",\n  \"visualAcceptance\": \"unreviewed\",\n"
                         << "  \"limits\": \"Control replay is physics-tick based; settings and source hashes are recorded by the capture runner. Timings do not certify image quality.\"\n}\n";
                if( m_sceneProfile )
                    m_sceneProfile->setValidationEvidence( evidence.str(), m_evidencePath, m_capturePassed, nativeFrameValid );
                else if( !m_evidencePath.empty() && !advanced::writeReviewEvidence( m_evidencePath, evidence.str() ) )
                {
                    WP_LOG_ERROR( "VehicleAdvanced: could not write evidence manifest." );
                    m_capturePassed = false;
                }
                if( !vehicleLODValid )
                    WP_LOG_ERROR( "Vehicle LOD visibility validation failed." );
                if( !treeLODValid )
                    WP_LOG_ERROR( "Tree LOD: expected exactly one visible level per patch." );
                WP_LOG( m_capturePassed ? "VehicleAdvanced review completed."
                                        : "VehicleAdvanced capture failed." );
                app->setQuit( true );
            }
        }
    }

    void SampleVehicleAdvanced::reset()
    {
        m_resetRequested = true;
    }

    void SampleVehicleAdvanced::setSmokeTest( bool enabled )
    {
        m_smokeTest = enabled;
    }

    bool SampleVehicleAdvanced::smokeTestPassed() const
    {
        return m_smokeTestPassed;
    }

    void SampleVehicleAdvanced::performReset()
    {
        if ( m_raceScene ) m_raceScene->performReset();
        m_lapStart = 0;
        m_nextCheckpoint = 1;
        m_lastTrackIndex = 0;
        if( m_cameraActor )
        {
            const bool terrainRoads=m_assets.roadAuthoring && m_assets.roadTerrain;
            const auto target=terrainRoads?m_vehicleActor->getPosition():getVehicleSpawnPosition();
            if(terrainRoads)m_generatedSpawnPosition=target;
            m_cameraActor->setPosition( terrainRoads?target+Vector3F(0,cameraHeight,cameraDistance):getInitialCameraPosition() );
            m_cameraActor->lookAt( target, Vector3<real_Num>::unitY() );
            m_cameraActor->updateTransform();
        }
    }

    void SampleVehicleAdvanced::updateControls()
    {
        if(!m_physicsConfigured){if(m_raceScene)m_raceScene->setControls(0,1,0);return;}
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto input = applicationManager ? applicationManager->getInputDeviceManager() : nullptr;
        auto car = m_vehicleActor ? m_vehicleActor->getComponent<scene::CarController>() : nullptr;
        auto vehicle = car ? car->getVehicleController() : nullptr;
        if( !input || !vehicle )
        {
            return;
        }

        float throttle = 0, brake = 0, steering = 0;

        if( m_smokeTest )
        {
            throttle = m_smokePhase == 1 || m_smokePhase == 2 ? 1.0f : 0.0f;
            brake = m_smokePhase == 3 ? 1.0f : 0.0f;
            steering = m_smokePhase == 2 ? .35f : 0.0f;
        }

        if( !m_capturePath.empty() || m_benchmarkSeconds > 0 )
        {
            throttle = 0;
            brake = 1;
            steering = 0;
        }

        if( !m_replayControls.empty() && m_physicsConfigured )
        {
            if( m_replayTick < m_replayControls.size() )
            {
                const auto &control = m_replayControls[m_replayTick++];
                throttle = control.throttle;
                brake = control.brake;
                steering = control.steering;
            }
            else
            {
                throttle = 0;
                brake = 1;
                steering = 0;
            }
        }

        // At speed, limit requested lateral acceleration rather than allowing a full
        // parking-speed steering angle to overturn the car. Keep full lock for hairpins.
        const auto speed =
            m_vehicleActor->getComponent<scene::Rigidbody>()->getLinearVelocity().length();
        const auto maxSteer = m_assets.vehicle.physics.wheels[0].maxSteerRad;
        const auto steerScale =
            std::min( 1.0, m_assets.vehicle.physics.wheelbaseM * 8.0 /
                               ( std::max( double( speed * speed ), 1.0 ) * maxSteer ) );
        // Keep the handling fixture on the track, clear of the physical barriers.
        steering *= float( steerScale );
        if( m_trackSmokeTest && m_physicsConfigured )
        {
            if(m_assets.circuit.samples.empty() || m_assets.circuit.length<=0 || (m_assets.cityTerrainRoads && !m_assets.cityRaceRouteAvailable))
            {WP_LOG_ERROR("VehicleAdvanced full circuit unavailable: "+m_assets.cityRaceRouteError);m_raceScene->setControls(0,1,0);applicationManager->setQuit(true);return;}
            const auto position = m_vehicleActor->getPosition();
            const auto &circuit = m_assets.circuit;
            const auto nearest = circuit.nearest( position );
            const auto lookahead = std::max( 8.f, speed * .75f );
            const auto target = circuit
                                    .samples[( nearest + size_t( lookahead * circuit.samples.size() /
                                                                 circuit.length ) ) %
                                             circuit.samples.size()]
                                    .position;
            const auto local = m_vehicleActor->getOrientation().inverse() * ( target - position );
            const auto angle = std::atan2( 2.f * float( m_assets.vehicle.physics.wheelbaseM ) * local.x,
                                           std::max( local.x * local.x + local.z * local.z, 1.f ) );
            steering = float( std::clamp( double( angle ) / maxSteer, -1.0, 1.0 ) );
            const auto wanted = std::clamp( 7.f / ( 1.f + std::abs( angle ) * 2.f ), 3.5f, 7.f );
            throttle = std::clamp( ( wanted - speed ) * .25f, 0.f, .4f );
            brake = std::clamp( ( speed - wanted ) * .12f, 0.f, .4f );
            auto flat = position - circuit.samples[nearest].position;
            flat.y = 0;
            const auto now = applicationManager->getTimer()->getTimeSinceSceneLoad();
            f32 groundY=0;bool driveable=false;if(m_assets.roadAuthoring)m_assets.roadAuthoring->getRuntimeSurface(position.x,position.z,groundY,driveable);
            if( position.y-groundY < .1f || position.y-groundY > 1.f || flat.length() > 12.f || now > 300 )
            {
                WP_LOG_ERROR( String( "VehicleAdvanced full circuit: FAIL position=" ) +
                              StringUtil::toString( position ) +
                              " offset=" + StringUtil::toString( flat.length() ) +
                              " index=" + StringUtil::toString( nearest ) +
                              " steer=" + StringUtil::toString( steering ) );
                applicationManager->setQuit( true );
            }
            else if( m_lap > 1 )
            {
                m_smokeTestPassed = true;
                WP_LOG( "VehicleAdvanced full circuit: PASS." );
                applicationManager->setQuit( true );
            }
        }
        if( m_collisionSmokeTest )
            m_raceScene->setControls( 0, 0, 0 );
        else if( m_smokeTest || m_trackSmokeTest || !m_capturePath.empty() || m_benchmarkSeconds > 0 )
            m_raceScene->setControls( throttle, brake, steering );
        else
            m_raceScene->usePlayerControls();
    }

    void SampleVehicleAdvanced::updateVehicleEffects()
    {
        if( m_effectsSmokeTest && m_effectsTestPhase >= 5 )
            return;
        auto app = core::IApplicationManager::instance();
        const auto dt = float( app->getTimer()->getDeltaTime() );
        if( !std::isfinite( dt ) || dt <= 0 )
            return;
        auto frame = m_raceScene->sampleVehicleEffects();
        if( m_effectsSmokeTest )
        {
            m_effectsTestGrounded =
                m_effectsTestGrounded || std::any_of( frame.grounded.begin(), frame.grounded.end(),
                                                      []( bool contact ) { return contact; } );
            const bool road = m_effectsTestPhase == 0;
            const bool dust = m_effectsTestPhase == 2 || m_effectsTestPhase == 3;
            frame.position = { 0, .45f, float( 4 - m_effectsTestTime * 4 ) };
            frame.velocity = road && m_effectsTestTime < 2.92 ? Vector3F( 0, 0, -10 )
                             : dust                           ? Vector3F( 0, 0, -20 )
                                                              : Vector3F::zero();
            frame.playing = m_effectsTestPhase != 3;
            for( size_t i = 0; i < 4; ++i )
            {
                frame.contact[i] = { i % 2 ? .95f : -.95f, .026f,
                                     float( 4 - m_effectsTestTime * 4 ) + ( i < 2 ? -1.5f : 1.5f ) };
                frame.normal[i] = Vector3F::unitY();
                frame.slip[i] = 8;
                frame.width[i] = .28f;
                frame.grounded[i] = road || dust;
                frame.onRoad[i] = !dust;
            }
        }
        m_raceScene->getVehicleVisualEffects().update( frame, dt );
        if( !m_effectsSmokeTest )
            return;
        m_effectsTestTime += std::min( dt, .05f );
        const float durations[] = { 3.f, 2.f, 1.f, 1.f, .5f };
        if( m_effectsTestTime < durations[m_effectsTestPhase] )
            return;
        const auto particleCount = m_raceScene->getVehicleVisualEffects().particles(),
                   decalCount = m_raceScene->getVehicleVisualEffects().decals();
        bool passed = m_raceScene->getVehicleVisualEffects().uploaded();
        if( m_effectsTestPhase == 0 )
        {
            passed = passed && particleCount > 0 && decalCount > 0 && m_effectsTestGrounded;
            m_effectsTestEmitted = m_raceScene->getVehicleVisualEffects().emitted();
            m_effectsTestDecals = decalCount;
            if( !m_effectsCapture.empty() )
                passed = advanced::captureFrame( m_effectsCapture ) && passed;
        }
        else if( m_effectsTestPhase == 1 )
        {
            passed = passed && particleCount == 0 &&
                     m_raceScene->getVehicleVisualEffects().emitted() == m_effectsTestEmitted &&
                     decalCount > 0;
        }
        else if( m_effectsTestPhase == 2 )
        {
            passed = passed && particleCount > 0 &&
                     m_raceScene->getVehicleVisualEffects().emitted() > m_effectsTestEmitted &&
                     decalCount == m_effectsTestDecals;
            m_effectsTestEmitted = m_raceScene->getVehicleVisualEffects().emitted();
            m_effectsTestParticles = particleCount;
            if( !m_effectsCapture.empty() )
                passed = advanced::captureFrame( m_effectsCapture + ".dust.bmp" ) && passed;
            frame.playing = false;
            m_raceScene->getVehicleVisualEffects().update( frame, dt );
        }
        else if( m_effectsTestPhase == 3 )
        {
            passed = passed && particleCount == m_effectsTestParticles &&
                     m_raceScene->getVehicleVisualEffects().emitted() == m_effectsTestEmitted &&
                     decalCount == m_effectsTestDecals;
            m_raceScene->getVehicleVisualEffects().reset();
            passed = passed && m_raceScene->getVehicleVisualEffects().decals() == 0 &&
                     m_raceScene->getVehicleVisualEffects().particles() == 0;
        }
        else
        {
            passed = passed && particleCount == 0 && decalCount == 0 &&
                     m_raceScene->getVehicleVisualEffects().emitted() == m_effectsTestEmitted;
            // These are the same lifecycle controls exposed to the Lua sample.
            m_raceScene->setEffectsEnabled(false);
            m_raceScene->initializePresentation();
            passed = passed && !m_raceScene->isEffectsAvailable() &&
                     m_raceScene->getParticleCount() == 0 && m_raceScene->getSkidDecalCount() == 0;
            m_raceScene->setEffectsEnabled(true);
            m_raceScene->initializePresentation();
            passed = passed && m_raceScene->isEffectsAvailable() &&
                     m_raceScene->getParticleCount() == 0 && m_raceScene->getSkidDecalCount() == 0;
        }
        WP_LOG( "Vehicle FX smoke phase " + StringUtil::toString( m_effectsTestPhase ) +
                ( passed ? ": PASS" : ": FAIL" ) +
                " particles=" + StringUtil::toString( particleCount ) +
                " decals=" + StringUtil::toString( decalCount ) );
        if( !passed || m_effectsTestPhase == 4 )
        {
            m_smokeTestPassed = passed;
            app->setQuit( true );
        }
        ++m_effectsTestPhase;
        m_effectsTestTime = 0;
    }

    void SampleVehicleAdvanced::updateVehicleAudio()
    {
        auto app = core::IApplicationManager::instance();
        const auto dt = app->getTimer()->getDeltaTime();
        if( !std::isfinite( dt ) || dt <= 0 )
            return;
        m_audioElapsed += std::min( dt, 1.0 / 30.0 );
        if( m_audioElapsed < 1.0 / 30.0 )
            return;
        const auto audioDt = float( m_audioElapsed );
        m_audioElapsed = 0;
        auto input = m_raceScene->sampleVehicleAudio();
        if( m_audioSmokeTest )
        {
            // Exercise the real platform voices without depending on driving-test tuning.
            input.rpm = m_audioSmokePhase == 0 ? input.idleRpm : input.redlineRpm;
            input.throttle = m_audioSmokePhase == 0 ? 0.f : 1.f;
            input.speed = m_audioSmokePhase == 0 ? 0.f : 40.f;
            input.slipSpeed = m_audioSmokePhase == 0 ? 0.f : 8.f;
            input.grounded = m_audioSmokePhase < 2;
            input.playing = m_audioSmokePhase < 3;
        }
        m_raceScene->getVehicleAudio().update( input, audioDt );
        if( !m_audioSmokeTest )
            return;
        m_audioSmokeTime += audioDt;
        if( m_audioSmokeTime < 2 )
            return;
        const auto& gains = m_raceScene->getVehicleAudio().gains();
        bool passed = m_raceScene->getVehicleAudio().isPlaying();
        if( m_audioSmokePhase == 0 )
            passed = passed && gains.engine[0] > .19f && gains.squeal < .001f;
        else if( m_audioSmokePhase == 1 )
            passed = passed && gains.engine[5] > .39f && gains.squeal > .27f && gains.rolling > .05f;
        else if( m_audioSmokePhase == 2 )
            passed = passed && gains.squeal < .001f && gains.rolling < .001f;
        else
        {
            for( const auto gain : gains.engine )
                passed = passed && gain < .001f;
            passed = passed && gains.squeal < .001f && gains.rolling < .001f;
        }
        WP_LOG( "Vehicle audio smoke phase " + StringUtil::toString( m_audioSmokePhase ) +
                ( passed ? ": PASS" : ": FAIL" ) + " idle=" + StringUtil::toString( gains.engine[0] ) +
                " redline=" + StringUtil::toString( gains.engine[5] ) +
                " rolling=" + StringUtil::toString( gains.rolling ) +
                " squeal=" + StringUtil::toString( gains.squeal ) );
        if( !passed || m_audioSmokePhase == 3 )
        {
            m_smokeTestPassed = passed;
            app->setQuit( true );
        }
        ++m_audioSmokePhase;
        m_audioSmokeTime = 0;
    }

    void SampleVehicleAdvanced::prepareRoadTerrainEditReview()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto authoring = m_assets.roadAuthoring;
        auto terrain = m_assets.roadTerrain;
        const auto originalTerrain = terrain ? terrain->getActiveTerrainSnapshot() : nullptr;
        if( !originalTerrain || !authoring || originalTerrain != authoring->getTerrainSnapshot() ||
            originalTerrain != terrain->getTerrainSnapshot() )
            return; // Wait for initial terrain render/collision publication.
        try
        {
            String error;
            const auto original = authoring->getSource();
            const auto originalJson = authoring->getSourceJSON();
            const auto originalGrading = authoring->getTerrainGrading();
            const auto originalBake = authoring->getBake();
            const auto placement = authoring->getTerrainPlacement();
            m_roadTerrainEditInitialRevision = originalTerrain->revision;
            const auto sampleHeight = [&]( const Vector3F &point, f32 &height ) {
                return render::sampleTerrainHeight( *originalTerrain, placement, point, height );
            };
            const auto sameGround = []( const render::TerrainData &a, const render::TerrainData &b ) {
                return a.dimensions == b.dimensions && a.spacing == b.spacing && a.origin == b.origin &&
                       a.heightScale == b.heightScale && a.heights == b.heights && a.layers == b.layers &&
                       a.layerWeights == b.layerWeights && a.appearance == b.appearance &&
                       a.autoMaterial == b.autoMaterial && a.surfaceMasks == b.surfaceMasks &&
                       a.holes == b.holes;
            };
            const auto canonical = [&]( procedural::CityRoadSplineSource source ) {
                source.revision = 1;
                source.terrainRevision = 0;
                SourceBudgetText bytes;
                if( !procedural::serializeCityRoadSplineSource( source, bytes, error ) )
                    throw std::runtime_error( error.c_str() );
                return bytes;
            };
            const auto originalCanonical = canonical( original );
            const auto lineDistance = []( Vector3F point, Vector3F a, Vector3F b ) {
                point.y = a.y = b.y = 0;
                const auto direction = b - a;
                const auto denominator = direction.lengthSquared();
                const float t = denominator > 0 ? std::clamp(
                    ( point - a ).dotProduct( direction ) / denominator, 0.f, 1.f ) : 0.f;
                return ( point - ( a + direction * t ) ).length();
            };
            SourceBudgetSet<SourceBudgetText> junctionRoads;
            for( const auto &junction : original.roundabouts )
                for( const auto &port : junction.ports )
                    junctionRoads.insert( port.approachId );
            bool committed = false;
            unsigned gradingAttempts = 0;
            String lastFailure = "No isolated draped free end needed terrain fitting.";
            for( size_t index = 0; index < original.roads.size() && !committed && gradingAttempts < 8; ++index )
            {
                const auto &road = original.roads[index];
                if( road.closed || road.knots.size() < 3 || road.heightMode != procedural::CityRoadHeightMode::Drape ||
                    junctionRoads.count( road.id ) )
                    continue;
                for( const bool first : { true, false } )
                {
                    const auto end = first ? 0 : road.knots.size() - 1;
                    const auto adjacent = first ? 1 : end - 1;
                    auto start = road.knots[end].position;
                    auto inward = road.knots[adjacent].position - start;
                    inward.y = 0;
                    if( inward.length() < 8.f )
                        continue;
                    inward.normalise();
                    const Vector3F across( -inward.z, 0, inward.x );
                    bool nearAnotherRoad = false;
                    for( size_t other = 0; other < originalBake.roads.size() && !nearAnotherRoad; ++other )
                    {
                        if( other == index )
                            continue;
                        const auto &points = originalBake.roads[other].points;
                        for( size_t i = 1; i < points.size() && !nearAnotherRoad; ++i )
                            nearAnotherRoad = lineDistance( start, points[i - 1].position, points[i].position ) < 22.f;
                    }
                    if( nearAnotherRoad || !sampleHeight( start, start.y ) )
                        continue;

                    // Move the control sideways and lengthen its exit so the new
                    // curve keeps a gradual heading change through the hillside.
                    for( const float lateral : { 11.f, -11.f, 15.f, -15.f } )
                    {
                        if( gradingAttempts >= 8 )
                            break;
                        auto draft = original;
                        auto &edit = draft.roads[index];
                        auto &knot = edit.knots[end];
                        auto shifted = start + across * lateral - inward * 11.f;
                        if( !sampleHeight( shifted, shifted.y ) )
                            continue;
                        const auto &neighbour = edit.knots[adjacent];
                        auto connector = neighbour.position - shifted;
                        connector.y = 0;
                        const float handleLength = connector.length() / 3.f;
                        connector.normalise();
                        knot.position = shifted;
                        ( first ? knot.outHandle : knot.inHandle ) = connector * handleLength;
                        auto &oldHandle = first ? edit.knots[adjacent].inHandle : edit.knots[adjacent].outHandle;
                        oldHandle = -inward * handleLength;
                        procedural::CityRoadSplineBake unsupported;
                        if( procedural::sampleCityRoadSplines( draft, {}, sampleHeight, unsupported, error ) ||
                            error.find( "Road profile exceeds grade." ) != 0 )
                            continue;
                        ++gradingAttempts;
                        SourceBudgetText json;
                        if( !procedural::serializeCityRoadSplineSource( draft, json, error ) )
                            throw std::runtime_error( error.c_str() );
                        if( !authoring->applyTerrainRoadEdit( String( json.data(), json.size() ),
                                authoring->getSourceRevision(), originalTerrain, nullptr, true, error ) )
                        {
                            if( terrain->getActiveTerrainSnapshot() != originalTerrain ||
                                authoring->getSourceJSON() != originalJson )
                                throw std::runtime_error( "Rejected review candidate changed live terrain or roads." );
                            lastFailure = error;
                            continue;
                        }
                        const auto &editedPoints = authoring->getBake().roads[index].points;
                        if( editedPoints.size() < 3 )
                            throw std::runtime_error( "Fitted review road has too few samples." );
                        bool clearProbe = false;
                        for( size_t i = 1; i < std::min<size_t>( 10, editedPoints.size() - 1 ); ++i )
                        {
                            const auto p = ( first ? editedPoints[i] : editedPoints[editedPoints.size() - 1 - i] ).position;
                            const auto &oldPoints = originalBake.roads[index].points;
                            const bool clearOldRoad = std::adjacent_find( oldPoints.begin(), oldPoints.end(), [&]( const auto &a, const auto &b ) {
                                return lineDistance( p, a.position, b.position ) < road.width * .5f + 2.6f;
                            } ) == oldPoints.end();
                            if( !clearOldRoad )
                                continue;
                            const auto &lots = m_assets.cityLayout.lots;
                            if( std::any_of( lots.begin(), lots.end(), [&]( const auto &lot ) {
                                    return std::abs( p.x - lot.parcelX ) <= lot.parcelWidth * .5f + .2f &&
                                           std::abs( p.z - lot.parcelZ ) <= lot.parcelDepth * .5f + .2f;
                                } ) )
                                continue;
                            m_roadTerrainEditProbe = p;
                            clearProbe = true;
                            break;
                        }
                        if( !clearProbe )
                            throw std::runtime_error( "Fitted review road has no collision probe clear of the original road and parcels." );
                        m_roadTerrainEditRoad = index;
                        m_roadTerrainEditBeforeKnot = start;
                        m_roadTerrainEditAfterKnot = shifted;
                        const auto side = across * ( lateral > 0 ? 1.f : -1.f );
                        m_roadTerrainEditEye = start - inward * 29.f + side * 14.f;
                        m_roadTerrainEditTarget = start + inward * 15.f + side * 4.f;
                        if( !sampleHeight( m_roadTerrainEditEye, m_roadTerrainEditEye.y ) ||
                            !sampleHeight( m_roadTerrainEditTarget, m_roadTerrainEditTarget.y ) )
                            throw std::runtime_error( "Road review camera falls outside the terrain." );
                        m_roadTerrainEditEye.y += 9.f;
                        m_roadTerrainEditTarget.y += .7f;
                        committed = true;
                        break;
                    }
                    if( committed || gradingAttempts >= 8 )
                        break;
                }
            }
            if( !committed )
                throw std::runtime_error( lastFailure.c_str() );

            const auto fittedTerrain = terrain->getActiveTerrainSnapshot();
            const auto fittedJson = authoring->getSourceJSON();
            const auto fittedGrading = authoring->getTerrainGrading();
            const auto fittedCanonical = canonical( authoring->getSource() );
            if( !fittedTerrain || fittedTerrain != authoring->getTerrainSnapshot() ||
                fittedTerrain->revision <= originalTerrain->revision ||
                fittedTerrain->heights.size() != originalTerrain->heights.size() ||
                fittedCanonical == originalCanonical )
                throw std::runtime_error( "Road grading did not publish a new matched source and terrain." );
            m_roadTerrainEditCommittedRevision = fittedTerrain->revision;
            render::TerrainSnapshot vacated;
            if( !scene::prepareCityRoadTerrainRestoration( authoring->getSource(), originalTerrain,
                    placement, originalGrading, vacated, error ) )
                throw std::runtime_error( error.c_str() );
            for( size_t i = 0; i < originalTerrain->heights.size(); ++i )
            {
                const float delta = ( fittedTerrain->heights[i] - originalTerrain->heights[i] ) *
                                    originalTerrain->heightScale * placement.getScale().y;
                if( delta != 0 ) ++m_roadTerrainEditChangedSamples;
                if( vacated->heights[i] != originalTerrain->heights[i] &&
                    fittedTerrain->heights[i] == vacated->heights[i] ) ++m_roadTerrainEditMoveRestored;
                m_roadTerrainEditFill = std::max( m_roadTerrainEditFill, delta );
                m_roadTerrainEditCut = std::max( m_roadTerrainEditCut, -delta );
            }
            if( !m_roadTerrainEditChangedSamples || !m_roadTerrainEditMoveRestored ||
                !sampleHeight( m_roadTerrainEditProbe, m_roadTerrainEditBeforeHeight ) ||
                !render::sampleTerrainHeight( *fittedTerrain, placement, m_roadTerrainEditProbe, m_roadTerrainEditAfterHeight ) )
                throw std::runtime_error( "The selected edit did not grade and restore the expected terrain." );

            auto deletion = authoring->getSource();
            deletion.roads.erase( deletion.roads.begin() + m_roadTerrainEditRoad );
            SourceBudgetText deletionJson;
            if( !procedural::serializeCityRoadSplineSource( deletion, deletionJson, error ) ||
                !authoring->applyTerrainRoadEdit( String( deletionJson.data(), deletionJson.size() ),
                    authoring->getSourceRevision(), fittedTerrain, nullptr, false, error ) )
                throw std::runtime_error( error.c_str() );
            const auto deleted = terrain->getActiveTerrainSnapshot();
            if( !deleted || deleted->heights.size() != fittedTerrain->heights.size() )
                throw std::runtime_error( "Road deletion did not publish a matching terrain grid." );
            for( size_t i = 0; i < deleted->heights.size(); ++i )
                m_roadTerrainEditDeleteRestored += deleted->heights[i] != fittedTerrain->heights[i];
            if( !m_roadTerrainEditDeleteRestored || authoring->getSource().roads.size() != deletion.roads.size() ||
                !authoring->applyTerrainRoadEdit( fittedJson, authoring->getSourceRevision(), deleted,
                    fittedTerrain, false, error, nullptr, &fittedGrading ) )
                throw std::runtime_error( error.empty() ? "Road deletion did not restore the graded ground." : error.c_str() );
            const auto restoredAfterDelete = terrain->getActiveTerrainSnapshot();
            if( !restoredAfterDelete || !sameGround( *restoredAfterDelete, *fittedTerrain ) ||
                authoring->getTerrainGrading() != fittedGrading || canonical( authoring->getSource() ) != fittedCanonical )
                throw std::runtime_error( "Undo did not restore the deleted road and its grading." );
            if( !authoring->applyTerrainRoadEdit( originalJson, authoring->getSourceRevision(),
                    restoredAfterDelete, originalTerrain, false, error, nullptr, &originalGrading ) )
                throw std::runtime_error( error.c_str() );
            const auto undone = terrain->getActiveTerrainSnapshot();
            if( !undone || undone != authoring->getTerrainSnapshot() || undone->revision <= fittedTerrain->revision ||
                !sameGround( *originalTerrain, *undone ) || authoring->getTerrainGrading() != originalGrading ||
                canonical( authoring->getSource() ) != originalCanonical )
                throw std::runtime_error( "Undo did not restore exact original terrain and road content." );
            m_roadTerrainEditUndoRevision = undone->revision;

            if( m_captureView == "road-edit-after" )
            {
                if( !authoring->applyTerrainRoadEdit( fittedJson, authoring->getSourceRevision(),
                        undone, fittedTerrain, false, error, nullptr, &fittedGrading ) )
                    throw std::runtime_error( error.c_str() );
                const auto redone = terrain->getActiveTerrainSnapshot();
                if( !redone || redone != authoring->getTerrainSnapshot() || redone->revision <= undone->revision ||
                    !sameGround( *fittedTerrain, *redone ) || authoring->getTerrainGrading() != fittedGrading ||
                    canonical( authoring->getSource() ) != fittedCanonical )
                    throw std::runtime_error( "Redo did not restore exact committed terrain and road content." );
                m_roadTerrainEditRedoRevision = redone->revision;
            }

            // The race scene observes the committed source/snapshot and refreshes
            // the derived city during its normal next Application update.
            std::cout << "Road terrain edit verified at road " << m_roadTerrainEditRoad
                      << ": samples=" << m_roadTerrainEditChangedSamples
                      << " move-restored=" << m_roadTerrainEditMoveRestored
                      << " delete-restored=" << m_roadTerrainEditDeleteRestored
                      << " cut=" << m_roadTerrainEditCut << "m fill=" << m_roadTerrainEditFill
                      << "m revisions=" << m_roadTerrainEditInitialRevision << '/'
                      << m_roadTerrainEditCommittedRevision << '/' << m_roadTerrainEditUndoRevision
                      << '/' << m_roadTerrainEditRedoRevision << std::endl;
            m_roadTerrainEditApplied = true;
            m_physicsConfigured = false;
            m_cityBuildingStartupDeadline = 0;
        }
        catch( const std::exception &failure )
        {
            std::cerr << "Road terrain edit review: " << failure.what() << '\n';
            const auto json = "{\"schema\":\"workphone.vehicle-review.v1\",\"validationPassed\":false,\"failure\":" +
                              advanced::jsonString( failure.what() ) + "}";
            if( !m_evidencePath.empty() ) advanced::writeReviewEvidence( m_evidencePath, json );
            m_captureAttempted = true;
            app->setQuit( true );
        }
    }

    void SampleVehicleAdvanced::adoptRoadTerrainEditReview()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto authoring = m_assets.roadAuthoring;
        auto terrain = m_assets.roadTerrain;
        const auto revision = m_captureView == "road-edit-after" ? m_roadTerrainEditRedoRevision :
                                                                    m_roadTerrainEditUndoRevision;
        const auto &next = m_raceScene->getAssets();
        if( next.roadAuthoring != authoring || next.roadTerrain != terrain ||
            next.cityAppliedSourceRevision != authoring->getActiveRevision() ||
            !m_raceScene->isPhysicsConfigured() )
            return;
        try
        {
            const auto snapshot = terrain->getActiveTerrainSnapshot();
            if( !snapshot || snapshot->revision != revision || snapshot != authoring->getTerrainSnapshot() )
                throw std::runtime_error( "Derived city refresh discarded the committed terrain or road edit." );
            m_assets = next;
            m_boxGround = m_assets.ground;
            m_chassisMeshActor = m_assets.body;
            m_wheelActors = m_assets.wheels;
            m_generatedSpawnPosition = m_vehicleActor->getPosition();
#if WP_GRAPHICS_SYSTEM_CLAW
            if( auto scene = dynamic_pointer_cast<render::ClawScene>( app->getGraphicsSystem()->getGraphicsScene() ) )
                if( !scene->setPrimaryDirectionalLight( m_assets.sun ) )
                    throw std::runtime_error( "Refreshed city sunlight could not be selected." );
#endif
            if( m_assets.vehicleLOD ) m_assets.vehicleLOD->setForcedLOD( m_forcedVehicleLOD );
            for( auto &group : m_assets.treeLODs ) group->setForcedLOD( m_forcedLOD );
            String error;
            if( m_assets.foliageTerrain && !m_assets.foliageTerrain->setFoliageForcedLod( m_forcedLOD, error ) )
                throw std::runtime_error( error.c_str() );
            m_raceScene->initializePresentation();
            // A rebuild can consume more wall time than the upload allowance.
            // The Application task's scene-time stamp predates that rebuild.
            m_cityBuildingStartupDeadline = app->getTimer()->now() + 30;
            m_reviewStart = -1;
            m_reviewFrames = 0;
            m_roadTerrainEditPrepared = true;
        }
        catch( const std::exception &failure )
        {
            std::cerr << "Road terrain edit review: " << failure.what() << '\n';
            const auto json = "{\"schema\":\"workphone.vehicle-review.v1\",\"validationPassed\":false,\"failure\":" +
                              advanced::jsonString( failure.what() ) + "}";
            if( !m_evidencePath.empty() ) advanced::writeReviewEvidence( m_evidencePath, json );
            m_captureAttempted = true;
            app->setQuit( true );
        }
    }

    bool SampleVehicleAdvanced::writeRoadTerrainEditReview( std::ostream &output ) const
    {
        const auto authoring = m_assets.roadAuthoring;
        const auto terrain = m_assets.roadTerrain;
        const auto current = terrain ? terrain->getActiveTerrainSnapshot() : nullptr;
        const bool edited = m_captureView == "road-edit-after";
        const auto expectedRevision = edited ? m_roadTerrainEditRedoRevision : m_roadTerrainEditUndoRevision;
        const float ground = edited ? m_roadTerrainEditAfterHeight : m_roadTerrainEditBeforeHeight;
        float roadHeight = 0;
        bool driveable = false;
        const auto point = m_roadTerrainEditProbe;
        const bool roadPresent = authoring &&
            authoring->getRuntimeSurface( point.x, point.z, roadHeight, driveable ) && driveable;
        const float expectedHeight = edited ? roadHeight : ground;
        const auto app = core::IApplicationManager::instancePtr();
        auto physics = app->getPhysicsManager()->getPhysicsScene();
        Vector3F hit, normal;
        SmartPtr<ISharedObject> object;
        const bool collision = physics && physics->intersects(
            { point.x, expectedHeight + .2f, point.z }, { point.x, expectedHeight - .2f, point.z },
            hit, normal, object ) && object && std::isfinite( hit.y ) &&
            std::abs( hit.y - expectedHeight ) < .01f && normal.y > .5f;
        const bool valid = m_roadTerrainEditPrepared && current &&
            current->revision == expectedRevision && current == authoring->getTerrainSnapshot() &&
            edited == roadPresent && collision;
        const auto vector = [&]( Vector3F p ) { output << '[' << p.x << ',' << p.y << ',' << p.z << ']'; };
        output << ",\n  \"roadTerrainEdit\": {\"stage\":\"" << ( edited ? "redo" : "undo" )
               << "\",\"roadIndex\":" << m_roadTerrainEditRoad
               << ",\"initialTerrainRevision\":" << m_roadTerrainEditInitialRevision
               << ",\"editedTerrainRevision\":" << m_roadTerrainEditCommittedRevision
               << ",\"undoTerrainRevision\":" << m_roadTerrainEditUndoRevision
               << ",\"redoTerrainRevision\":" << m_roadTerrainEditRedoRevision
               << ",\"changedSamples\":" << m_roadTerrainEditChangedSamples
               << ",\"moveRestoredSamples\":" << m_roadTerrainEditMoveRestored
               << ",\"deleteRestoredSamples\":" << m_roadTerrainEditDeleteRestored
               << ",\"maxCutMetres\":" << m_roadTerrainEditCut << ",\"maxFillMetres\":" << m_roadTerrainEditFill
               << ",\"oldKnot\":";
        vector( m_roadTerrainEditBeforeKnot );
        output << ",\"newKnot\":";
        vector( m_roadTerrainEditAfterKnot );
        output << ",\"probe\":";
        vector( point );
        output << ",\"beforeGroundMetres\":" << m_roadTerrainEditBeforeHeight
               << ",\"editedGroundMetres\":" << m_roadTerrainEditAfterHeight
               << ",\"roadPresent\":" << ( roadPresent ? "true" : "false" )
               << ",\"collisionMatched\":" << ( collision ? "true" : "false" )
               << ",\"eye\":";
        vector( m_roadTerrainEditEye );
        output << ",\"target\":";
        vector( m_roadTerrainEditTarget );
        output << ",\"valid\":" << ( valid ? "true" : "false" ) << '}';
        return valid;
    }

    void SampleVehicleAdvanced::prepareCityRetentionSmoke()
    {
        auto app=core::IApplicationManager::instancePtr();
        try
        {
            auto authoring=m_assets.roadAuthoring;auto terrain=m_assets.roadTerrain;
            if(!authoring || !terrain || !m_raceScene)throw std::runtime_error("Native editable city anchors are missing.");
            auto edited=authoring->getSource();const auto sourceId=edited.id;const auto binding=authoring->getTerrainBindingUUID();
            SourceBudgetSet<SourceBudgetText> referenced;
            for(const auto &junction:edited.roundabouts)for(const auto &port:junction.ports)referenced.insert(port.approachId);
            auto road=std::find_if(edited.roads.begin(),edited.roads.end(),[&](const auto &candidate){return !referenced.count(candidate.id) && candidate.width>=.75f;});
            if(road==edited.roads.end())throw std::runtime_error("No eligible non-junction road exists for the required width edit.");
            const auto roadId=road->id;SourceBudgetVector<SourceBudgetText> knotIds;for(const auto &knot:road->knots)knotIds.push_back(knot.id);
            road->width-=.25f;const float width=road->width;SourceBudgetText bytes;String error;
            if(!procedural::serializeCityRoadSplineSource(edited,bytes,error) ||
               !authoring->setSourceJSON(String(bytes.data(),bytes.size()),authoring->getSourceRevision(),error))throw std::runtime_error(error.c_str());
            const auto adoptScene=[&]() {
                m_assets=m_raceScene->getAssets();m_boxGround=m_assets.ground;m_chassisMeshActor=m_assets.body;m_wheelActors=m_assets.wheels;
                m_generatedSpawnPosition=m_vehicleActor->getPosition();
#if WP_GRAPHICS_SYSTEM_CLAW
                if(auto scene=dynamic_pointer_cast<render::ClawScene>(app->getGraphicsSystem()->getGraphicsScene()))
                    if(!scene->setPrimaryDirectionalLight(m_assets.sun))throw std::runtime_error("Regenerated city sunlight could not be selected.");
#endif
                if(m_assets.vehicleLOD)m_assets.vehicleLOD->setForcedLOD(m_forcedVehicleLOD);
                for(auto &group:m_assets.treeLODs)group->setForcedLOD(m_forcedLOD);
                if(m_assets.foliageTerrain && !m_assets.foliageTerrain->setFoliageForcedLod(m_forcedLOD,error))throw std::runtime_error(error.c_str());
                m_raceScene->initializePresentation();
            };
            const auto retainedSource=[&](SmartPtr<scene::TerrainSystem> expectedTerrain,const String &expectedBinding) {
                const auto &retained=authoring->getSource();
                if(m_assets.roadAuthoring!=authoring || m_assets.roadTerrain!=expectedTerrain || !m_assets.citySourceReused ||
                   retained.id!=sourceId || authoring->getTerrain()!=expectedTerrain || authoring->getTerrainBindingUUID()!=expectedBinding ||
                   std::string_view(retained.terrainResourceUuid.data(),retained.terrainResourceUuid.size())!=std::string_view(expectedBinding.data(),expectedBinding.size()) ||
                   m_assets.cityAppliedSourceRevision!=authoring->getSourceRevision() || authoring->getActiveRevision()!=authoring->getSourceRevision())
                    throw std::runtime_error("Regeneration replaced an anchor/source identity or failed to retain the canonical terrain binding.");
                const auto match=std::find_if(retained.roads.begin(),retained.roads.end(),[&](const auto &candidate){return candidate.id==roadId;});
                if(match==retained.roads.end() || std::abs(match->width-width)>.000001f || match->knots.size()!=knotIds.size())
                    throw std::runtime_error("Regeneration discarded the road width edit or control-point identities.");
                for(size_t i=0;i<knotIds.size();++i)if(match->knots[i].id!=knotIds[i])throw std::runtime_error("Regeneration changed a retained control-point GUID.");
                if(!m_assets.cityBuildingSupportValid || m_assets.cityBuildingSupportChecks<m_assets.cityLayout.lots.size())
                    throw std::runtime_error("Regenerated building support checks failed.");
            };
            const auto regenerate=[&]() {
                m_physicsConfigured=false;
                if(!m_raceScene->regenerate())throw std::runtime_error(m_raceScene->getGenerationError().c_str());
                adoptScene();
            };
            regenerate();retainedSource(terrain,binding);

            // Keep Terrain A scene-valid throughout the rebind/regeneration, so
            // stale scene caches cannot accidentally pass by recreating it.
            auto manager=app->getGameManager();auto currentScene=manager->getCurrentScene();auto oldActor=terrain->getActor();
            const auto originalTerrain=terrain->getTerrainSnapshot();
            if(!currentScene || !oldActor || !originalTerrain)throw std::runtime_error("The retained terrain cannot be copied for the rebind gate.");
            auto replacementActor=manager->createActor();if(!replacementActor)throw std::runtime_error("Cannot create the replacement terrain actor.");
            replacementActor->setName("Terrain city smoke replacement terrain");replacementActor->setStatic(true);
            replacementActor->setPosition(oldActor->getPosition());replacementActor->setOrientation(oldActor->getOrientation());replacementActor->setScale(oldActor->getScale());
            currentScene->addActor(replacementActor);currentScene->registerAllUpdates(replacementActor);
            auto replacement=replacementActor->addComponent<scene::TerrainSystem>();
            if(!replacement || !replacementActor->addComponent<scene::CollisionTerrain>() || !replacementActor->addComponent<scene::Rigidbody>())
                throw std::runtime_error("Replacement terrain requires native terrain, collision and rigidbody components.");
            if(!replacement->isLoaded())replacement->load(nullptr);
            replacement->setTreesEnabled(false);replacement->setGrassEnabled(false);
            const auto applyTerrain=[&](const render::TerrainData &data) {
                auto renderer=dynamic_pointer_cast<render::Terrain>(replacement->getTerrain());
                if(!renderer)throw std::runtime_error("Replacement terrain has no native renderer.");
                const bool deferred=renderer->isTerrainPublicationDeferred();if(deferred)renderer->setDeferredTerrainPublication(false);
                const bool applied=replacement->applyTerrainData(data,error);
                if(deferred)renderer->setDeferredTerrainPublication(true);
                if(!applied)throw std::runtime_error(error.c_str());
            };
            applyTerrain(*originalTerrain);
            if(!authoring->bindTerrain(replacement,error))throw std::runtime_error(error.c_str());
            const auto replacementBinding=authoring->getTerrainBindingUUID();
            if(replacementBinding==binding)throw std::runtime_error("Replacement terrain did not receive a distinct persistent identity.");
            regenerate();retainedSource(replacement,replacementBinding);
            manager->destroyActor(oldActor,true);
            if(oldActor->getScene())throw std::runtime_error("The old terrain actor remained in the scene after retirement.");

            // Rebuild preserves the source token. Scenery must also observe the
            // new retained terrain snapshot to refresh foundations and foliage.
            if(m_assets.cityLayout.lots.empty())throw std::runtime_error("No admitted parcel exists for the terrain refresh gate.");
            const auto parcelId=m_assets.cityLayout.lots.front().id;const float oldGround=m_assets.cityLayout.lots.front().groundHeight;
            const auto beforeTerrain=replacement->getTerrainSnapshot();if(!beforeTerrain)throw std::runtime_error("Replacement terrain snapshot is unavailable.");
            render::TerrainData raised=*beforeTerrain;
            const float rise=.0005f,scale=raised.heightScale*replacementActor->getScale().y;
            if(!std::isfinite(scale) || scale<=0)throw std::runtime_error("Replacement terrain height scale is invalid.");
            for(auto &height:raised.heights)height+=rise/scale;
            applyTerrain(raised);const auto sourceRevision=authoring->getSourceRevision();
            if(!authoring->rebuild(error))throw std::runtime_error(error.c_str());
            if(authoring->getSourceRevision()!=sourceRevision || authoring->getTerrainSnapshot()==beforeTerrain)
                throw std::runtime_error("Explicit terrain rebuild changed the source token or retained the old snapshot.");
            m_physicsConfigured=false;m_raceScene->update();adoptScene();retainedSource(replacement,replacementBinding);
            const auto refreshed=std::find_if(m_assets.cityLayout.lots.begin(),m_assets.cityLayout.lots.end(),[&](const auto &lot){return lot.id==parcelId;});
            if(refreshed==m_assets.cityLayout.lots.end() || std::abs(refreshed->groundHeight-oldGround-rise)>.00015f ||
               authoring->getSourceRevision()!=sourceRevision || replacement->getActiveTerrainSnapshot()!=authoring->getTerrainSnapshot())
                throw std::runtime_error("Terrain-only snapshot refresh did not update retained parcel supports at the unchanged source revision.");
            m_cityRetentionSmokeRoadId=roadId;m_cityRetentionSmokeWidth=width;
            m_collisionProbeTime=app->getTimer()->getTimeSinceSceneLoad();m_cityRetentionSmokePrepared=true;
        }
        catch(const std::exception &error)
        {WP_LOG_ERROR(String("Terrain city edit/regeneration retention smoke: FAIL ")+error.what());m_smokeTestPassed=false;if(app)app->setQuit(true);}
    }

    void SampleVehicleAdvanced::updateCollisionSmokeTest()
    {
        auto app = core::IApplicationManager::instance();
        if( !m_physicsConfigured || app->getTimer()->getTimeSinceSceneLoad() <= 3 )
            return;
        if(m_assets.cityTerrainRoads && !m_cityRetentionSmokePrepared)return;
        auto physics = app->getPhysicsManager()->getPhysicsScene();
        auto chassis = m_vehicleActor->getComponent<scene::Rigidbody>()->getRigidDynamic();
        if( m_assets.openCity && m_collisionProbeStarted ) return;
        if( !m_collisionProbeStarted )
        {
            size_t count = 0;
            size_t cityBuildingMasses = 0;
            SmartPtr<scene::IGameActor> citySidewalk;
            auto fail = [&]( const String &message ) {
                WP_LOG_ERROR( String( m_assets.openCity ? "City collision smoke: FAIL " :
                                                       "Trackside collision smoke: FAIL " ) + message );
                m_smokeTestPassed = false;
                app->setQuit( true );
            };
            for( const auto &actor : m_assets.actors )
            {
                if( actor->getName().find( "Trackside collision: " ) != 0 )
                    continue;
                ++count;
                if( actor->getName() == "Trackside collision: City building mass" ) ++cityBuildingMasses;
                if( !citySidewalk && actor->getName() == "Trackside collision: City sidewalk" ) citySidewalk = actor;
                auto box = actor->getComponent<scene::CollisionBox>();
                auto body = actor->getComponent<scene::Rigidbody>();
                if( !box || !box->getShape() || !body || !body->getRigidStatic() ||
                    body->getRigidStatic()->getScene() != physics || !box->getShape()->isAttached() )
                {
                    fail( actor->getName() + " has no static box body; actor state="+StringUtil::toString(u32(actor->getState()))+
                        " box="+StringUtil::toString(bool(box))+" shape="+StringUtil::toString(box && box->getShape())+
                        " rigidbody="+StringUtil::toString(bool(body))+" static="+StringUtil::toString(body && body->getRigidStatic())+
                        " attached="+StringUtil::toString(box && box->getShape() && box->getShape()->isAttached())+
                        " scene="+StringUtil::toString(body && body->getRigidStatic() && body->getRigidStatic()->getScene()==physics) );
                    return;
                }
                const auto centre = actor->getPosition();
                const auto orientation = actor->getOrientation();
                const auto size = box->getExtents();
                const auto nativePose = body->getRigidStatic()->getTransform();
                if( (nativePose.getPosition() - centre).length() > .001f ||
                    (nativePose.getOrientation() * Vector3F::unitX() - orientation * Vector3F::unitX()).length() > .001f )
                {
                    fail( actor->getName() + " native pose does not match the generated box" );
                    return;
                }
                // Short rays cannot hit the ground below. Probe the thin rotated
                // rail side too, so an incorrect yaw cannot pass the top-face check.
                for( int axis = 0; axis < (actor->getName() == "Trackside collision: Guardrail beam" ? 2 : 1); ++axis )
                {
                    const auto normal = orientation * (axis ? Vector3F::unitX() : Vector3F::unitY());
                    const auto halfSize = (axis ? size.x : size.y) * .5f;
                    const auto face = centre + normal * halfSize;
                    Vector3F hitPosition, hitNormal;
                    SmartPtr<ISharedObject> object;
                    if( !physics->intersects( face + normal * .04f, face - normal * .04f,
                                             hitPosition, hitNormal, object ) ||
                        !object ||
                        // Adjacent rails can overlap on tight inside corners.
                        // Their registration and poses were checked above; a
                        // neighboring face can be the closest result here.
                        (object == body->getRigidStatic() && (hitPosition - face).length() > .015f) )
                    {
                        fail( actor->getName() + " face does not match its physics shape axis=" +
                              StringUtil::toString(axis) + " expected=" + StringUtil::toString(face) +
                              " actual=" + StringUtil::toString(hitPosition) +
                              " body=" + StringUtil::toString(body->getRigidStatic()->getTransform().getPosition()) );
                        return;
                    }
                }
            }
            Vector3F hitPosition, hitNormal;
            SmartPtr<ISharedObject> object;
            if(m_assets.cityTerrainRoads)
            {
                const auto check=checkTerrainCity(m_assets,physics);
                if(!check.roadPBRBindingsReady && app->getTimer()->getTimeSinceSceneLoad()-m_collisionProbeTime<5)return;
                if(!check.valid){fail("terrain-city checks failed binding="+StringUtil::toString(check.binding)+" revisions="+StringUtil::toString(check.revisions)+
                    " renderer="+StringUtil::toString(check.renderer)+" route="+StringUtil::toString(check.route)+" islands="+StringUtil::toString(check.islands)+
                    " support="+StringUtil::toString(check.support)+" rays="+StringUtil::toString(u32(check.rayProbes))+" PBR="+StringUtil::toString(check.roadPBRBindingsReady)+
                    " footway="+StringUtil::toString(check.footwayReady)+" footway rays="+StringUtil::toString(check.footwayNativeRayProbes));return;}
                WP_LOG("Terrain city edit/regeneration retention smoke: PASS road="+String(m_cityRetentionSmokeRoadId.data(),m_cityRetentionSmokeRoadId.size())+
                       " width="+StringUtil::toString(m_cityRetentionSmokeWidth)+" source reused=true revision="+StringUtil::toString(m_assets.cityAppliedSourceRevision)+
                       " terrain rebound=true old terrain retired=true terrain snapshot refreshed=true native surface rays="+StringUtil::toString(u32(check.rayProbes))+" building support checks="+StringUtil::toString(m_assets.cityBuildingSupportChecks));
                WP_LOG("Terrain city collision smoke: PASS native static boxes="+StringUtil::toString(u32(count))+" surface rays="+StringUtil::toString(u32(check.rayProbes))+" building masses="+StringUtil::toString(u32(check.buildingBodies))+" uploaded road PBR textures="+StringUtil::toString(check.roadPBRTextures)+" footway native rays="+StringUtil::toString(check.footwayNativeRayProbes));
                m_smokeTestPassed=true;app->setQuit(true);return;
            }
            if( m_assets.openCity )
            {
                // Registration/pose/face checks above cover every authored static box.
                // These longer downward probes verify the two distinct drive/walk grades.
                const auto roadPoint = m_assets.cityNaturalRoads && !m_assets.circuit.samples.empty()
                    ? m_assets.circuit.samples[std::min<size_t>( 10, m_assets.circuit.samples.size() - 1 )].position
                    : Vector3F( 0, 0, -m_citySpacing * .25f );
                if( count == 0 || !std::isfinite( m_citySpacing ) || m_citySpacing <= 0 ||
                    !physics->intersects( roadPoint + Vector3F( 0, 2, 0 ),
                                          roadPoint - Vector3F( 0, 1, 0 ),
                                          hitPosition, hitNormal, object ) || !object ||
                    !std::isfinite( hitPosition.y ) || std::abs( hitPosition.y ) > .02f )
                {
                    fail( "road-centre collision grade does not match Y=0" );
                    return;
                }
                const float roadY = hitPosition.y;
                if( !citySidewalk )
                {
                    fail( "no shared-descriptor sidewalk collision was generated" );
                    return;
                }
                // Stay inside the retained walk but away from its centre lamp.
                const auto walkSize = citySidewalk->getComponent<scene::CollisionBox>()->getExtents();
                const auto walkPoint = citySidewalk->getPosition() + citySidewalk->getOrientation() *
                    Vector3F( walkSize.x > walkSize.z ? walkSize.x * .2f : 0, 0,
                              walkSize.z >= walkSize.x ? walkSize.z * .2f : 0 );
                const auto isWalkBody = [&]( const SmartPtr<ISharedObject> &hit ) {
                    if( !m_assets.cityNaturalRoads )
                        return hit == citySidewalk->getComponent<scene::Rigidbody>()->getRigidStatic();
                    // Equal-grade natural footways can overlap between close parallel
                    // streets. Every body's placement was checked independently above.
                    for( const auto &candidate : m_assets.actors )
                        if( candidate && candidate->getName() == "Trackside collision: City sidewalk" )
                            if( auto body = candidate->getComponent<scene::Rigidbody>() )
                                if( hit == body->getRigidStatic() ) return true;
                    return false;
                };
                object = nullptr;
                if( !physics->intersects( { walkPoint.x, 2, walkPoint.z },
                                          { walkPoint.x, -1, walkPoint.z },
                                          hitPosition, hitNormal, object ) || !object ||
                    !isWalkBody( object ) ||
                    !std::isfinite( hitPosition.y ) || std::abs( hitPosition.y - .15f ) > .015f )
                {
                    fail( "sidewalk collision grade does not match Y=0.15" );
                    return;
                }
                const float sidewalkY = hitPosition.y;
                const auto city = procedural::OpenCityLayout::generate( m_seed, m_assets.cityBlocks, m_assets.cityRoute,
                                                                        m_assets.cityNaturalRoads );
                size_t expectedMasses = 0;
                for( const auto &lot : city.lots ) expectedMasses += procedural::OpenCityLayout::buildingVolumes( lot ).size();
                if( cityBuildingMasses != expectedMasses || expectedMasses == 0 )
                {
                    fail( "native building mass count does not match the shared layout" );
                    return;
                }
                WP_LOG( "City collision smoke: PASS native static boxes=" + StringUtil::toString( count ) +
                        " roadY=" + StringUtil::toString( roadY ) + " sidewalkY=" + StringUtil::toString( sidewalkY ) +
                        " building masses=" + StringUtil::toString( cityBuildingMasses ) );
                m_collisionProbeStarted = true;
                m_smokeTestPassed = true;
                app->setQuit( true );
                return;
            }
            if( count == 0 || physics->intersects( { 0, .5f, 2 }, { 0, .5f, 4 },
                                                  hitPosition, hitNormal, object ) )
            {
                fail( "gantry opening is obstructed or no colliders were generated" );
                return;
            }
            WP_LOG( "Trackside collision smoke: verified static boxes=" + StringUtil::toString(count) );
            // Coast into the first garage to verify actual contact response.
            auto car = m_vehicleActor->getComponent<scene::CarController>();
            car->getVehicleController()->reset();
            const Transform3F spawn( { -28, .42f, -3 }, QuaternionF::identity() );
            chassis->clearForce();
            chassis->clearTorque();
            chassis->setTransform( spawn );
            chassis->setLinearVelocity( { 0, 0, -10 } );
            chassis->setAngularVelocity( Vector3F::zero() );
            m_vehicleActor->setPosition( spawn.getPosition() );
            m_vehicleActor->setOrientation( spawn.getOrientation() );
            m_vehicleActor->updateTransform();
            m_collisionProbeStarted = true;
            return;
        }
        m_collisionProbeTime += std::clamp(app->getTimer()->getDeltaTime(), 0.0, 1.0 / 30.0);
        if( m_collisionProbeTime < 3 )
            return;
        const auto position = chassis->getTransform().getPosition();
        const auto velocity = chassis->getLinearVelocity();
        m_smokeTestPassed = position.z < -4 && position.z > -8 && std::abs(velocity.z) < 2;
        WP_LOG( String("Trackside collision smoke: garage impact ") +
                (m_smokeTestPassed ? "PASS" : "FAIL") + " position=" + StringUtil::toString(position) +
                " velocity=" + StringUtil::toString(velocity) );
        app->setQuit( true );
    }

    void SampleVehicleAdvanced::updateWheelVisuals()
    {
        m_raceScene->updateWheelVisuals();
    }

    void SampleVehicleAdvanced::updateSmokeTest()
    {
        auto app = core::IApplicationManager::instance();
        if( m_terrainGameplay && !m_terrainSurfacesValidated )
            return;
        // Application allows scene state and resources to settle before stepping physics.
        if( app->getTimer()->getTimeSinceSceneLoad() <= 3.0 )
            return;
        m_smokeTime += Math<f64>::clamp( app->getTimer()->getDeltaTime(), 0.0, 1.0 / 30.0 );
        auto body = m_vehicleActor->getComponent<scene::Rigidbody>()->getRigidDynamic();
        const auto position = body->getTransform().getPosition();
        const auto settling = m_smokePhase == 0 || m_smokePhase == 4;
        const auto phaseDuration = settling ? 3.0 : m_smokePhase == 2 ? 2.0 : 4.0;
        // Ignore the initial drop, then monitor every physics step instead of just phase endpoints.
        if( !settling || m_smokeTime > phaseDuration - 1.0 )
        {
            m_smokeMinHeight = Math<real_Num>::min( m_smokeMinHeight, position.Y() );
            m_smokeMaxHeight = Math<real_Num>::max( m_smokeMaxHeight, position.Y() );
            m_smokePeakVerticalSpeed = Math<real_Num>::max(
                m_smokePeakVerticalSpeed, Math<real_Num>::Abs( body->getLinearVelocity().Y() ) );
        }
        auto horizontalVelocity = body->getLinearVelocity();
        horizontalVelocity.Y() = 0.0f;
        const auto speed = horizontalVelocity.length();
        if( speed > 5 )
        {
            const auto localVelocity =
                body->getTransform().getOrientation().inverse() * horizontalVelocity;
            m_smokePeakBodySlip = std::max(
                m_smokePeakBodySlip,
                real_Num( std::atan2( std::abs( localVelocity.X() ),
                                      std::max( std::abs( localVelocity.Z() ), real_Num( .1 ) ) ) ) );
            m_smokePeakYawRate =
                std::max( m_smokePeakYawRate, std::abs( body->getAngularVelocity().Y() ) );
        }
        if( m_smokeTime < phaseDuration )
            return;
        m_smokeTime = 0.0;
        bool passed = true;
        const auto heightRange = m_smokeMaxHeight - m_smokeMinHeight;
        const auto stable =
            m_smokeMinHeight > 0.0f && heightRange < 0.15f && m_smokePeakVerticalSpeed < 0.8f;
        switch( m_smokePhase )
        {
        case 0:
            passed = position.Y() > 0.0f && position.Y() < vehicleSpawnHeight && speed < 2.0f;
            m_smokeStartPosition = position;
            break;
        case 1:
            passed = ( position - m_smokeStartPosition ).length() > 1.0f && speed > 20.0f;
            m_smokeStartOrientation = body->getTransform().getOrientation();
            break;
        case 2:
        {
            auto heading = body->getTransform().getOrientation() * Vector3<real_Num>::unitZ();
            auto startHeading = m_smokeStartOrientation * Vector3<real_Num>::unitZ();
            heading.Y() = startHeading.Y() = 0.0f;
            heading.normalise();
            startHeading.normalise();
            passed = ( heading - startHeading ).length() > 0.02f;
            m_smokeDriveSpeed = speed;
            break;
        }
        case 3:
            passed = speed < m_smokeDriveSpeed;
            performReset();
            passed =
                passed &&
                ( body->getTransform().getPosition() - (m_assets.roadAuthoring && m_assets.roadTerrain?m_generatedSpawnPosition:getVehicleSpawnPosition()) ).length() < 0.01f &&
                body->getLinearVelocity().length() < 0.01f &&
                body->getAngularVelocity().length() < 0.01f;
            break;
        case 4:
            passed = position.Y() > 0.0f && position.Y() < vehicleSpawnHeight && speed < 2.0f;
            break;
        }
        passed = passed && stable && m_smokePeakBodySlip < .35f && m_smokePeakYawRate < 1.5f;
        if( m_smokePhase == 4 )
        {
            m_smokeTestPassed = passed;
            if( passed )
                WP_LOG( "Vehicle smoke: chassis and wheel render matrices match their poses." );
        }
        WP_LOG( String( "Vehicle smoke phase " ) + StringUtil::toString( m_smokePhase ) +
                ( passed ? ": PASS" : ": FAIL" ) + " position=" + StringUtil::toString( position ) +
                " speed=" + StringUtil::toString( speed ) +
                " min height=" + StringUtil::toString( m_smokeMinHeight ) +
                " max height=" + StringUtil::toString( m_smokeMaxHeight ) +
                " orientation=" + StringUtil::toString( body->getTransform().getOrientation() ) +
                " angular velocity=" + StringUtil::toString( body->getAngularVelocity() ) +
                " peak body slip=" + StringUtil::toString( m_smokePeakBodySlip ) +
                " peak yaw rate=" + StringUtil::toString( m_smokePeakYawRate ) +
                " height range=" + StringUtil::toString( heightRange ) +
                " peak vertical speed=" + StringUtil::toString( m_smokePeakVerticalSpeed ) );
        if( auto car = m_vehicleActor->getComponent<scene::CarController>() )
            for( u32 i = 0; i < 4; ++i )
            {
                auto wheel = car->getVehicleController()->getWheelController( i );
                float load = 0, acceleration = 0;
                auto props = wheel->getProperties();
                props->getPropertyValue( "Normal Force", load );
                props->getPropertyValue( "Contact Acceleration", acceleration );
                WP_LOG( "Handling wheel " + StringUtil::toString( i ) +
                        " compression=" + StringUtil::toString( wheel->getCompression() ) +
                        " load=" + StringUtil::toString( load ) +
                        " acceleration=" + StringUtil::toString( acceleration ) );
            }
        m_smokeMinHeight = std::numeric_limits<real_Num>::max();
        m_smokeMaxHeight = std::numeric_limits<real_Num>::lowest();
        m_smokePeakVerticalSpeed = 0.0f;
        m_smokePeakBodySlip = 0.0f;
        m_smokePeakYawRate = 0.0f;
        if( !passed || m_smokePhase == 4 )
        {
            app->setQuit( true );
            return;
        }
        ++m_smokePhase;
    }

    void SampleVehicleAdvanced::updateDebugText()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager ? applicationManager->getTimer() : nullptr;
        if( !m_hudEnabled || !timer || !m_vehicleActor ||
            timer->getTime() < m_nextDebugUpdate )
        {
            return;
        }
        const auto now = timer->getTime();
        m_nextDebugUpdate = now + 0.1;
        if(m_assets.circuit.samples.empty() || (m_assets.cityTerrainRoads && !m_assets.cityRaceRouteAvailable))
        {
            if(auto graphics=applicationManager->getGraphicsSystem())if(auto debug=graphics->getDebug())
            {debug->drawText(debugTextId,Vector2F(.02f,.14f),"Race route unavailable",0);debug->drawText(debugTextId+1,Vector2F(.02f,.18f),m_assets.cityRaceRouteError,0);debug->drawText(debugTextId+2,Vector2F(.02f,.22f),"Free city driving remains available.",0);}
            return;
        }
        if( m_lapStart == 0 )
            m_lapStart = now;
        auto car = m_vehicleActor->getComponent<scene::CarController>();
        auto vehicle = car ? car->getVehicleController() : nullptr;
        auto rigidbody = m_vehicleActor->getComponent<scene::Rigidbody>();
        const auto velocity = rigidbody ? rigidbody->getLinearVelocity() : Vector3<real_Num>::zero();
        const auto position = m_vehicleActor->getPosition();
        auto index = m_assets.circuit.nearest( position );
        auto roadOffset = position - m_assets.circuit.samples[index].position;
        roadOffset.y = 0;
        const auto onRoad = roadOffset.length() < 6.85f;
        if( m_lastTrackIndex > m_assets.circuit.samples.size() * 3 / 4 &&
            index < m_assets.circuit.samples.size() / 4 && m_nextCheckpoint == 4 && onRoad )
        {
            m_lastLapTime = now - m_lapStart;
            if( m_bestLapTime == 0 || m_lastLapTime < m_bestLapTime )
                m_bestLapTime = m_lastLapTime;
            ++m_lap;
            m_lapStart = now;
            m_nextCheckpoint = 1;
        }
        auto quarter = index * 4 / m_assets.circuit.samples.size();
        if( quarter == m_nextCheckpoint && onRoad )
            ++m_nextCheckpoint;
        m_lastTrackIndex = index;
        std::ostringstream text;
        text << std::fixed << std::setprecision( 1 );
        s32 gear = 0;
        double rpm = 0;
        if( vehicle && vehicle->getDriveTrain() )
        {
            auto props = vehicle->getDriveTrain()->getProperties();
            props->getPropertyValue( "Gear", gear );
            props->getPropertyValue( "RPM", rpm );
        }
        text << "GRAND PRIX | " << velocity.length() * 3.6f << " km/h | Gear "
             << ( gear == 0   ? "N"
                  : gear == 1 ? "R"
                              : std::to_string( gear - 1 ) )
             << " | " << int( rpm ) << " rpm | Lap " << m_lap << '\n';
        text << "Lap time " << ( m_lapStart > 0 ? now - m_lapStart : 0 ) << " s | Last " << m_lastLapTime
             << " s | Best " << m_bestLapTime << " s\n";
        text << "Circuit " << m_assets.circuit.length << " m | Seed " << m_seed;
        if( ( position - m_assets.circuit.samples[index].position ).length() > 11 )
            text << " | OFF TRACK";
        if( auto graphics = applicationManager->getGraphicsSystem() )
        {
            if( auto debug = graphics->getDebug() )
            {
                std::istringstream lines( text.str() );
                std::string line;
                for( size_t i = 0; std::getline( lines, line ); ++i )
                {
                    debug->drawText( debugTextId + i,
                                     Vector2<real_Num>( 0.02f, 0.14f + static_cast<f32>( i ) * 0.04f ),
                                     String( line.c_str() ), 0 );
                }
            }
        }
        if( now >= m_nextDebugLog )
        {
            WP_LOG( String( text.str().c_str() ) );
            if( ( m_smokeTest || m_trackSmokeTest ) && vehicle )
            {
                auto w = vehicle->getWheelController( 0 );
                WP_LOG(
                    String( "Advanced physics: dt=" ) + StringUtil::toString( timer->getDeltaTime() ) +
                    " mass=" + StringUtil::toString( rigidbody->getRigidDynamic()->getMass() ) +
                    " inertia=" +
                    StringUtil::toString( rigidbody->getRigidDynamic()->getMassSpaceInertiaTensor() ) +
                    " angular=" + StringUtil::toString( rigidbody->getAngularVelocity() ) +
                    " controllerMass=" + StringUtil::toString( vehicle->getBody()->getMass() ) +
                    " root=" + StringUtil::toString( position ) +
                    " mount=" + StringUtil::toString( w->getLocalTransform().getPosition() ) +
                    " rate=" + StringUtil::toString( w->getSpringRate() ) +
                    " damping=" + StringUtil::toString( w->getDamping() ) +
                    " travel=" + StringUtil::toString( w->getSuspensionTravel() ) );
            }
            m_nextDebugLog = now + 2.0;
        }
    }

    bool SampleVehicleAdvanced::validateTerrainGameplaySurfaces()
    {
#if WP_TERRAIN_GAMEPLAY_PACKAGE && WP_GRAPHICS_SYSTEM_CLAW
        const auto fail = []( const char *detail ) {
            WP_LOG_ERROR( String( "Terrain gameplay surfaces: FAIL " ) + detail );
            std::cerr << "Terrain gameplay surfaces: FAIL " << detail << '\n';
            return false;
        };
        const auto ground = m_assets.ground;
        const auto terrain = ground ? ground->getComponent<scene::TerrainSystem>() : nullptr;
        const auto collision = ground ? ground->getComponent<scene::CollisionTerrain>() : nullptr;
        const auto rigidbody = ground ? ground->getComponent<scene::Rigidbody>() : nullptr;
        const auto renderer = terrain ? dynamic_cast<render::ClawTerrain *>(terrain->getTerrain().get()) : nullptr;
        const auto source = terrain ? terrain->getTerrainSnapshot() : nullptr;
        const auto active = terrain ? terrain->getActiveTerrainSnapshot() : nullptr;
        if( !m_physicsConfigured || !ground || !ground->isStatic() || !renderer || !source ||
            active != source || renderer->getTerrainSnapshot() != source ||
            renderer->getActiveTerrainSnapshot() != source || renderer->getRenderedTerrainSnapshot() != source ||
            !collision || collision->getTerrainSnapshot() != source || !collision->getShape() ||
            !collision->getShape()->isAttached() || !rigidbody || !rigidbody->getRigidStatic() ||
            ground->getComponent<scene::CollisionBox>() )
            return fail( "source/render/query/collision publication or terrain ownership mismatch" );
        const auto residency = renderer->getTerrainResidency();
        if( !residency.residentTiles || residency.activeRevision != source->revision ||
            (!m_terrainGameplayPackageDirectory.empty() && !residency.usedCookedTileManifest) )
            return fail( "native terrain tiles or cooked manifest are unavailable" );
        for( const auto &generated : m_assets.actors )
            if( generated && generated->isStatic() && generated->getComponent<scene::CollisionBox>() )
                return fail( "a generated static collision box can hide terrain collision failure" );

        auto physics = core::IApplicationManager::instance()->getPhysicsManager()->getPhysicsScene();
        if( !physics ) return fail( "native physics scene is unavailable" );
        const auto placement = ground->getWorldTransform();
        for( const auto &point : {Vector3F(16,0,32),Vector3F(128,0,0),Vector3F(-128,0,96),Vector3F(0,0,-128)} )
        {
            f32 height = 0;
            if( !render::sampleTerrainHeight(*active,placement,point,height) )
                return fail( "solid terrain query missed" );
            const Vector3F start(point.x,height+50,point.z),end(point.x,height-50,point.z);
            const auto rendered = renderer->intersects(Ray3F(start,Vector3F(0,-1,0)));
            Vector3F hit,normal;
            SmartPtr<ISharedObject> object;
            if( !rendered || !rendered->hasIntersected() ||
                !std::isfinite(rendered->getPosition().y) ||
                std::abs(rendered->getPosition().y-height)>.035f ||
                std::abs(renderer->getHeightAtWorldPosition(point)-height)>.035f ||
                !physics->intersects(start,end,hit,normal,object) || !(object==rigidbody->getRigidStatic()) ||
                !std::isfinite(hit.y) || !std::isfinite(normal.y) || std::abs(hit.y-height)>.035f || normal.y<.5f )
                return fail( "native solid ray height/body differs from terrain query and renderer" );
        }
        const Vector3F hole(200.5f,0,160.5f);
        f32 height = 0;
        const auto rendered = renderer->intersects(Ray3F({hole.x,64,hole.z},{0,-1,0}));
        Vector3F hit,normal;
        SmartPtr<ISharedObject> object;
        if( render::sampleTerrainHeight(*active,placement,hole,height) ||
            (rendered && rendered->hasIntersected()) ||
            physics->intersects({hole.x,64,hole.z},{hole.x,-64,hole.z},hit,normal,object) )
            return fail( "hole has a query, rendered or native collision surface" );
        WP_LOG( "Terrain gameplay surfaces: PASS" );
        std::cout << "Terrain gameplay surfaces: PASS\n";
        return true;
#else
        return false;
#endif
    }

    void SampleVehicleAdvanced::createFileSystem()
    {
        if( m_terrainGameplayPackageDirectory.empty() )
        {
            Application::createFileSystem();
            return;
        }
        auto app = core::IApplicationManager::instance();
        auto fileSystem = app->getFactoryManager()->make_object<IFileSystem>();
        if( !fileSystem ) throw std::runtime_error("Package file system factory is unavailable.");
        fileSystem->load(nullptr);
        app->setFileSystem(fileSystem);
        const String assets(m_terrainGameplayPackageDirectory.c_str());
        fileSystem->addFolder(assets,true);
        app->setMediaPath(assets);
    }

    String SampleVehicleAdvanced::getMediaPath() const
    {
        return m_terrainGameplayPackageDirectory.empty() ? Application::getMediaPath()
            : String(m_terrainGameplayPackageDirectory.c_str());
    }

    bool SampleVehicleAdvanced::createScriptManager()
    {
        return m_terrainGameplayPackageDirectory.empty() ? Application::createScriptManager() : false;
    }

    void SampleVehicleAdvanced::createDefaultFont()
    {
        if( m_terrainGameplayPackageDirectory.empty() ) Application::createDefaultFont();
    }

    bool SampleVehicleAdvanced::createSoundManager()
    {
        if( !m_terrainGameplayPackageDirectory.empty() ) return false;
        if( !m_audioEnabled )
            return Application::createSoundManager();
        auto app = core::IApplicationManager::instance();
        // Select the platform backend explicitly: the generic factory can resolve
        // ISoundManager to the base SoundManager, which has no playback device.
        auto factory = app->getFactoryManager()->getFactoryByName( "workphone::WPAudioManager" );
        if( !factory )
        {
            WP_LOG_WARNING( "Vehicle audio: WPAudioManager factory is unavailable." );
            return false;
        }
        auto manager = factory->make_ptr<ISoundManager>();
        manager->load( nullptr );
        app->setSoundManager( manager );
        WP_LOG( "Vehicle audio manager: " + factory->getObjectTypeName() +
                ( manager->isLoaded() ? " loaded" : " initialization failed" ) );
        return manager->isLoaded();
    }

    void SampleVehicleAdvanced::createPlugins()
    {
        Application::createPlugins();

#ifndef _WP_STATIC_LIB_
        auto factoryManager = core::IApplicationManager::instance()->getFactoryManager();
        if( m_audioEnabled && !factoryManager->hasFactoryByName( "workphone::WPAudioManager" ) )
        {
            auto job = factoryManager->make_ptr<LoadPluginJob>();
            job->setPluginPath( "WPAudio" );
            job->execute();
        }
        if( !factoryManager->hasFactoryByName( "workphone::CCarController" ) )
        {
            auto job = factoryManager->make_ptr<LoadPluginJob>();
            job->setPluginPath( "WPVehiclePhysics" );
            job->execute();
        }
        if( !factoryManager->hasFactoryByName( "IVehicleGenerator" ) )
        {
            auto plugin = workphone::make_ptr<procedural::WPProcedural>();
            core::IApplicationManager::instance()->addPlugin( plugin );
        }
#endif

#ifdef _WP_STATIC_LIB_
        try
        {
            WP_DEBUG_TRACE;

            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto corePlugin = workphone::make_ptr<WPCore>();
            applicationManager->addPlugin( corePlugin );
            applicationManager->addPlugin( workphone::make_ptr<WPVehiclePhysics>() );
            applicationManager->addPlugin( workphone::make_ptr<procedural::WPProcedural>() );

            auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
            applicationManager->addPlugin( databasePlugin );

            auto inputPlugin = workphone::make_ptr<OISInput>();
            applicationManager->addPlugin( inputPlugin );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
            applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgre>();
            applicationManager->addPlugin( graphicsPlugin );
#    endif

#    if WP_BUILD_PHYSX
            auto physxPlugin = workphone::make_ptr<physics::FBPhysx>();
            applicationManager->addPlugin( physxPlugin );
#    elif WP_BUILD_ODE
#    endif

            ApplicationUtil::createFactories();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
#endif
    }

    void SampleVehicleAdvanced::createScene()
    {
        if( m_sceneProfile ) m_sceneProfile->checkpoint( "runtime_before_scene_generation" );
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto physicsManager = applicationManager->getPhysicsManager();
        WP_ASSERT( physicsManager );
        if( !physicsManager->getPhysicsScene() )
        {
            physicsManager->setPhysicsScene( physicsManager->addScene() );
        }
        WP_ASSERT( physicsManager->getPhysicsScene() );
        physicsManager->getPhysicsScene()->setGravity( Vector3<real_Num>( 0.0f, -9.81f, 0.0f ) );
        if( auto vehicleManager = applicationManager->getVehicleManager() )
        {
            vehicleManager->load( nullptr );
        }

        if( m_terrainGameplay )
        {
#if WP_TERRAIN_GAMEPLAY_PACKAGE
            m_terrainSurfacesValidated = false;
            if( !m_terrainGameplayPackageDirectory.empty() )
            {
                m_terrainGameplayPackage = advanced::terrain_gameplay::loadPackage(m_terrainGameplayPackageDirectory);
                m_seed = m_terrainGameplayPackage->seed;
                m_quality = static_cast<procedural::VehicleAppearanceQuality>(m_terrainGameplayPackage->quality);
                m_terrainGameplaySource = render::TerrainSnapshot(m_terrainGameplayPackage->decoded,
                    &m_terrainGameplayPackage->decoded->data.terrain);
            }
            else
                m_terrainGameplaySource = std::make_shared<render::TerrainData>(advanced::terrain_gameplay::makeSource());
#else
            throw std::runtime_error("Terrain gameplay packaging is unavailable in this build.");
#endif
        }

        if( m_cityTerrainRoads && applicationManager->getProjectPath().empty() )
        {
            // Standalone samples have no Editor project. Give generated terrain
            // textures a persistent source root using the same canonical data://
            // descriptors as project authoring and package export.
            const auto root = std::filesystem::current_path() / ".FBCache" / "ProceduralCity";
            std::filesystem::create_directories( root );
            applicationManager->setProjectPath( root.generic_u8string().c_str() );
        }

        m_vehicleActor = sceneManager->createActor();
        m_vehicleActor->setName( "Procedural Grand Prix" );
        m_vehicleActor->setPosition( getVehicleSpawnPosition() );
        scene->addActor( m_vehicleActor );
        m_raceScene = m_vehicleActor->addComponent<scene::ProceduralRaceScene>();
        m_raceScene->setAudioEnabled( m_audioEnabled );
        m_raceScene->setEffectsEnabled( m_effectsEnabled );
        m_raceScene->setAutomaticPresentation( !m_audioSmokeTest && !m_effectsSmokeTest );
        m_raceScene->setSeed( m_seed );
        m_raceScene->setQuality( static_cast<s32>( m_quality ) );
        m_raceScene->setFoliageWorldEnabled( m_foliageWorldEnabled );
        m_raceScene->setMaterialExportDirectory( m_materialExportDirectory );
        if( m_terrainGameplay ) m_raceScene->setTerrainGameplaySource(m_terrainGameplaySource);
        if( m_openCity )
        {
            auto properties = m_raceScene->getProperties();
            properties->setProperty( "Open City", true );
            properties->setProperty( "City Blocks", m_cityBlocks );
            properties->setProperty( "City Route", m_cityRoute );
            properties->setProperty( "City Natural Roads", m_cityNaturalRoads );
            properties->setProperty( "Road Authoring Showcase", m_roadAuthoringShowcase );
            properties->setProperty( "City Terrain Roads", m_cityTerrainRoads );
            properties->setProperty( "City Terrain Relief", m_cityTerrainRelief );
            if( m_cityStreetDetail >= 0 )
                properties->setProperty( "City Street Detail", m_cityStreetDetail );
            if( m_cityBuildingStyle >= 0 )
                properties->setProperty( "City Building Style", m_cityBuildingStyle );
            for( size_t role = 0; role < cityBuildingMeshProperties.size(); ++role )
                if( !m_cityBuildingMeshes[role].empty() )
                    properties->setProperty( cityBuildingMeshProperties[role], m_cityBuildingMeshes[role] );
            m_raceScene->setProperties( properties );
        }
        if( !m_raceScene->regenerate() )
            throw std::runtime_error( m_raceScene->getGenerationError() );
        m_assets = m_raceScene->getAssets();
        const auto generatedUnderVehicle = [&]( SmartPtr<scene::IGameActor> actor ) {
            while( actor && actor != m_vehicleActor &&
                   !actor->getFlag( scene::IGameActor::ActorFlagInScene ) )
            {
                actor = actor->getParent();
            }
            return actor == m_vehicleActor;
        };
        if( !std::all_of( m_assets.actors.begin(), m_assets.actors.end(), generatedUnderVehicle ) ||
            ( m_assets.roadAuthoring && !generatedUnderVehicle( m_assets.roadAuthoring->getActor() ) ) ||
            ( m_assets.roadTerrain && !generatedUnderVehicle( m_assets.roadTerrain->getActor() ) ) )
        {
            throw std::runtime_error( "Generated racing actors must belong to the vehicle hierarchy." );
        }
        if( m_assets.openCity && !m_assets.cityRoadShowcase && ( !m_capturePath.empty() || m_benchmarkSeconds > 0 ) )
        {
            std::cout << "VehicleAdvanced generated city memory ledger: ";
            writeSharedMemoryLedger( std::cout ); std::cout << std::endl;
            m_cityBuildingStartupDeadline = applicationManager->getTimer()->now() + 30;
        }
#if WP_TERRAIN_GAMEPLAY_PACKAGE && WP_GRAPHICS_SYSTEM_CLAW
        if( m_terrainGameplay && !m_terrainGameplayPackage && !m_terrainGameplayExportOnly )
        {
            const auto path = std::filesystem::temp_directory_path() /
                (std::string("WorkphoneTerrainGameplay-")+StringUtil::getUUID().c_str());
            m_terrainGameplayEphemeralDirectory = path.string();
            advanced::terrain_gameplay::exportPackage(path,*m_terrainGameplaySource,
                m_seed,int(m_quality),m_assets.vehicle.contentHash);
            m_terrainGameplayPackage = advanced::terrain_gameplay::loadPackage(path);
        }
        if( m_terrainGameplayPackage )
        {
            if( m_assets.vehicle.contentHash != m_terrainGameplayPackage->vehicleHash )
                throw std::runtime_error("Terrain gameplay vehicle recipe content hash mismatch.");
            auto terrain = m_assets.ground ? m_assets.ground->getComponent<scene::TerrainSystem>() : nullptr;
            auto clawTerrain = terrain ? dynamic_cast<render::ClawTerrain *>(terrain->getTerrain().get()) : nullptr;
            auto renderer = dynamic_cast<render::ClawRendererDX11 *>(applicationManager->getGraphicsSystem()->getRendererPtr());
            if( !terrain || !clawTerrain || !renderer )
                throw std::runtime_error("Terrain gameplay package requires native Claw terrain and DX11 renderer.");
            String error;
            const auto candidate = render::stageCookedTerrainMaterial(*renderer,m_terrainGameplayPackage->decoded,error);
            // Reconcile authoritative bytes first; the equal-data renderer apply
            // then preserves the TerrainSystem snapshot identity and publication gate.
            if( !candidate || !terrain->applyTerrainData(m_terrainGameplayPackage->decoded->data.terrain,error) ||
                !clawTerrain->applyCookedTerrain(candidate,error) )
                throw std::runtime_error(error.empty()?"Cannot stage cooked terrain gameplay material.":error.c_str());
            if( terrain->getTerrainSnapshot()!=clawTerrain->getTerrainSnapshot() )
                throw std::runtime_error("Cooked terrain replaced its authoritative component snapshot.");
            if( !m_terrainGameplayPackageDirectory.empty() )
            {
                WP_LOG("Terrain gameplay package: PASS sourceFree=true compilerRegistry=false authoringDatabase=false");
                std::cout << "Terrain gameplay package: PASS sourceFree=true compilerRegistry=false authoringDatabase=false\n";
            }
        }
#endif
        if( m_terrainGameplay )
        {
            if( !m_assets.vehicle.contentHash )
                throw std::runtime_error("Terrain gameplay vehicle recipe has no generated content hash.");
            std::ostringstream recipe;
            recipe << "Terrain gameplay recipe: PASS hash=" << std::hex << std::setfill('0')
                   << std::setw(16) << m_assets.vehicle.contentHash;
            WP_LOG(recipe.str());
            std::cout << recipe.str() << '\n';
        }
        m_cityExtent = m_citySpacing = 0;
        m_cityOverviewExtent = 0;
        m_cityOverviewCentre = m_cityStreetCentre = Vector3F::zero();
        m_cityStreetForward = { 0, 0, -1 };
        m_cityHasJunction=false;m_cityJunctionCentre=m_cityBoulevardCentre={};m_generatedSpawnPosition=m_vehicleActor->getPosition();
        const auto selectFacade = [&]( const procedural::OpenCityLayout &layout ) {
            const bool entryRoute = m_captureView == "entry-route";
            const bool shopfront = m_captureView == "shopfront" || entryRoute;
            if( m_captureView != "facade" && m_captureView != "building" && !shopfront )
                return;
            const auto anchor = m_cityHasJunction ? m_cityJunctionCentre : m_cityOverviewCentre;
            const procedural::OpenCityLayout::Lot *selected = nullptr;
            const procedural::OpenCityLayout::Lot *representative = nullptr;
            const procedural::OpenCityLayout::Lot *visibleFrontage = nullptr;
            float nearest = std::numeric_limits<float>::max();
            float representativeDistance = std::numeric_limits<float>::max();
            float visibleFrontageDistance = std::numeric_limits<float>::max();
            for( const auto &lot : layout.lots )
            {
                if( lot.park || ( shopfront && lot.district != procedural::OpenCityLayout::District::Commercial ) ) continue;
                const float distance = Vector3F( lot.x - anchor.x, 0, lot.z - anchor.z ).lengthSquared();
                if( distance < nearest || ( selected && distance == nearest && lot.id < selected->id ) )
                {
                    selected = &lot;
                    nearest = distance;
                }
                if( m_captureView == "building" && lot.district == procedural::OpenCityLayout::District::Residential &&
                    lot.height >= 8.f && lot.height <= 18.f && lot.width >= 10.f && lot.depth >= 10.f )
                {
                    if( distance < representativeDistance ||
                        ( representative && distance == representativeDistance && lot.id < representative->id ) )
                    {
                        representative = &lot;
                        representativeDistance = distance;
                    }
                    using Frontage = procedural::OpenCityLayout::Frontage;
                    if( ( lot.frontage == Frontage::North || lot.frontage == Frontage::East ) &&
                        ( distance < visibleFrontageDistance ||
                          ( visibleFrontage && distance == visibleFrontageDistance && lot.id < visibleFrontage->id ) ) )
                    {
                        visibleFrontage = &lot;
                        visibleFrontageDistance = distance;
                    }
                }
            }
            if( visibleFrontage ) selected = visibleFrontage;
            else if( representative ) selected = representative;
            if( !selected )
                throw std::runtime_error( "The facade/building view needs a generated building parcel." );

            const auto &lot = *selected;
            const auto volumes = procedural::OpenCityLayout::buildingVolumes( lot );
            if( volumes.empty() )
                throw std::runtime_error( "The selected building parcel has no building volume." );
            const auto &volume = volumes.front();
            using Frontage = procedural::OpenCityLayout::Frontage;
            Vector3F outward( 0, 0, -1 );
            switch( lot.frontage )
            {
            case Frontage::West: outward = { -1, 0, 0 }; break;
            case Frontage::East: outward = { 1, 0, 0 }; break;
            case Frontage::North: outward = { 0, 0, -1 }; break;
            case Frontage::South: outward = { 0, 0, 1 }; break;
            }
            const bool facesX = outward.x != 0;
            const float span = facesX ? volume.depth : volume.width;
            const float setback = ( facesX ? volume.width : volume.depth ) * .5f;
            const Vector3F along( -outward.z, 0, outward.x );
            const auto face = Vector3F( volume.x, volume.bottom, volume.z ) + outward * setback;
            m_cityFacadeLotId = lot.id;
            m_cityFacadeEye = face + outward * std::clamp( span * .48f, 14.f, 23.f ) +
                              along * std::min( span * .16f, 7.f );
            m_cityFacadeEye.y += 2.8f;
            m_cityFacadeTarget = face + Vector3F( 0, std::min( 5.5f, volume.height * .47f ), 0 );
            if( shopfront )
            {
                m_cityFacadeEye = face + outward * std::clamp( span * .72f, 12.f, 18.f ) +
                                  along * std::min( span * .18f, 3.2f ) + Vector3F( 0, 1.7f, 0 );
                m_cityFacadeTarget = face + Vector3F( 0, 1.95f, 0 );
            }
            if( m_captureView == "building" )
            {
                float top = volume.bottom + volume.height;
                for( const auto &tier : volumes ) top = std::max( top, tier.bottom + tier.height );
                // Reserve room for every built-in crown; style selection never changes the
                // camera, so silhouettes can be compared from the same parcel and pose.
                const float height = top - volume.bottom + 6.f;
                const float distance = std::max( { 24.f, volume.width * 1.15f, height * 1.35f } );
                // The north face receives the daylight key. A three-quarter view shows
                // facade depth and roof construction, including on east-frontage parcels.
                m_cityFacadeEye = Vector3F( volume.x + distance * .55f,
                                            volume.bottom + height * .9f + 4.f,
                                            volume.z - volume.depth * .5f - distance );
                m_cityFacadeTarget = Vector3F( volume.x, volume.bottom + height * .45f, volume.z );
            }
            if( m_assets.roadAuthoring )
            {
                const auto terrain = m_assets.roadAuthoring->getTerrainSnapshot();
                float terrainHeight = 0;
                if( terrain && render::sampleTerrainHeight( *terrain, m_assets.roadAuthoring->getTerrainPlacement(),
                                                            m_cityFacadeEye, terrainHeight ) )
                    m_cityFacadeEye.y = std::max( m_cityFacadeEye.y, terrainHeight + ( shopfront ? 1.82f : 2.8f ) );
            }
            if( entryRoute )
            {
                const auto &authoring = m_assets.roadAuthoring;
                const auto walk = authoring->getRuntimeFootways();
                const auto terrain = authoring->getTerrainSnapshot();
                if( !walk || !terrain ) throw std::runtime_error( "Entry-route view requires fitted sidewalks." );
                float closest = std::numeric_limits<float>::infinity();
                for( size_t i = 0; i + 2 < walk->indices.size(); i += 3 )
                {
                    const auto &top = walk->vertices[walk->indices[i]];
                    const auto a = walk->vertices[walk->indices[i + 1]].position;
                    const auto b = walk->vertices[walk->indices[i + 2]].position;
                    if( std::abs( top.normal.y ) > .01f || top.position.y - a.y < .10f ||
                        std::hypot( top.position.x - a.x, top.position.z - a.z ) > .001f ) continue;
                    auto edge = b - a, relative = face - a;
                    edge.y = relative.y = 0;
                    const float length = edge.lengthSquared();
                    if( length < .0001f ) continue;
                    const float t = std::clamp( relative.dotProduct( edge ) / length, 0.f, 1.f );
                    const auto foot = a + ( b - a ) * t;
                    const auto offset = Vector3F( foot.x - face.x, 0, foot.z - face.z );
                    const float distance = offset.lengthSquared();
                    if( distance < 4.f || distance >= closest || offset.dotProduct( outward ) < .25f ||
                        offset.dotProduct( top.normal ) >= -.25f ) continue;
                    closest = distance;
                    m_cityEntryWalkEdge = foot + Vector3F( 0, top.position.y - a.y, 0 );
                }
                if( !std::isfinite( closest ) ) throw std::runtime_error( "Entry-route view found no nearby sidewalk edge." );
                const float distance = std::sqrt( closest );
                auto direction = m_cityEntryWalkEdge - face;
                direction.y = 0;
                direction /= distance;
                auto side = Vector3F( -direction.z, 0, direction.x );
                if( side.dotProduct( outward ) < 0 ) side = -side;
                const auto centre = face + ( m_cityEntryWalkEdge - face ) * .48f;
                m_cityFacadeEye = centre + side * std::clamp( distance * 1.11f, 16.f, 37.f ) + direction * ( 2.f + .18f * distance );
                float terrainHeight = 0;
                if( render::sampleTerrainHeight( *terrain, authoring->getTerrainPlacement(), m_cityFacadeEye, terrainHeight ) )
                    m_cityFacadeEye.y = std::max( m_cityFacadeEye.y, terrainHeight );
                m_cityFacadeEye.y += std::clamp( distance * .49f, 7.f, 16.f );
                m_cityFacadeTarget = centre + Vector3F( 0, .7f, 0 );
            }
        };
        if(m_assets.cityTerrainRoads && m_assets.roadAuthoring && m_assets.roadTerrain)
        {
            const auto &layout=m_assets.cityLayout;const auto &bake=m_assets.roadAuthoring->getBake();const auto &source=m_assets.roadAuthoring->getSource();
            m_citySpacing=layout.spacing;float minX=std::numeric_limits<float>::max(),minZ=minX,minY=minX,maxX=-minX,maxZ=-minX,maxY=-minX;
            const auto include=[&](Vector3F p){minX=std::min(minX,p.x);minZ=std::min(minZ,p.z);minY=std::min(minY,p.y);maxX=std::max(maxX,p.x);maxZ=std::max(maxZ,p.z);maxY=std::max(maxY,p.y);};
            size_t longest=0;
            for(const auto &road:bake.roads)
            {
                for(const auto &point:road.points)include(point.position);
                if(road.points.size()>longest){longest=road.points.size();m_cityStreetCentre=road.points[road.points.size()/2].position;m_cityStreetForward=road.points[road.points.size()/2].tangent;}
            }
            for(const auto &lot:layout.lots){include({lot.x-lot.width*.5f,lot.groundHeight,lot.z-lot.depth*.5f});include({lot.x+lot.width*.5f,lot.groundHeight,lot.z+lot.depth*.5f});}
            if(minX<=maxX){m_cityExtent=std::max({std::abs(minX),std::abs(maxX),std::abs(minZ),std::abs(maxZ)});m_cityOverviewExtent=std::max(maxX-minX,maxZ-minZ)*.5f+20;m_cityOverviewCentre={(minX+maxX)*.5f,(minY+maxY)*.5f+8,(minZ+maxZ)*.5f};}
            float nearest=std::numeric_limits<float>::max();
            for(const auto &junction:source.roundabouts)if(!junction.ports.empty())
            {
                const float distance=(junction.center-m_cityOverviewCentre).lengthSquared();if(distance>=nearest)continue;nearest=distance;m_cityHasJunction=true;m_cityJunctionCentre=junction.center;
                const auto cameraPoint=[&](const auto &port,bool pedestrian){
                    auto p=junction.center+port.outward*(junction.outerRadius+port.length*.65f);
                    f32 height=0;bool driveable=false;
                    if(pedestrian)
                    {
                        p+=Vector3F(-port.outward.z,0,port.outward.x)*(port.width*.5f+1.2f);
                        if(render::sampleTerrainHeight(*m_assets.roadAuthoring->getTerrainSnapshot(),m_assets.roadAuthoring->getTerrainPlacement(),p,height))p.y=height+.15f;
                    }
                    else if(m_assets.roadAuthoring->getRuntimeSurface(p.x,p.z,height,driveable))p.y=height;
                    return p;
                };
                const auto &boulevard = junction.ports[junction.ports.size() / 2];
                m_cityStreetCentre = cameraPoint( junction.ports.front(), false );
                m_cityBoulevardCentre = cameraPoint( boulevard, true );
                m_cityStreetForward = junction.ports.front().outward;

                // Frame the outside of the approach sidewalk independently of
                // fixture admission, so density comparisons use one camera.
                Vector3F along( boulevard.outward.x, 0, boulevard.outward.z );
                along.normalise();
                const Vector3F outside( -along.z, 0, along.x );
                const auto frontage = junction.center +
                    along * ( junction.outerRadius + boulevard.length * .55f ) +
                    outside * ( boulevard.width * .5f + 1.98f );
                m_cityFrontageEye = frontage + along * 5.f - outside * 1.25f;
                m_cityFrontageTarget = frontage - along * 4.f - outside * .7f;
                const auto terrain = m_assets.roadAuthoring->getTerrainSnapshot();
                const auto &placement = m_assets.roadAuthoring->getTerrainPlacement();
                if( terrain )
                {
                    for( auto *point : { &m_cityFrontageEye, &m_cityFrontageTarget } )
                    {
                        f32 height = 0;
                        if( render::sampleTerrainHeight( *terrain, placement, *point, height ) )
                        {
                            point->y = height;
                        }
                    }
                }
                m_cityFrontageEye.y += 1.87f;
                m_cityFrontageTarget.y += .97f;
            }
            selectFacade( layout );
            if( m_captureView == "hillside" )
            {
                const auto view = cityHillsideInspectionView( m_assets );
                m_cityHillsideEye = view.eye; m_cityHillsideTarget = view.target;
                m_cityHillsideRoadId = view.roadId; m_cityHillsideRise = view.rise;
            }
            if( m_captureView == "verge" )
            {
                const auto view = cityVergeInspectionView( m_assets );
                m_cityVergeEye = view.eye; m_cityVergeTarget = view.target; m_cityVergeRoad = view.road;
            }
            if( m_captureView == "park" )
            {
                const auto view = cityParkInspectionView( m_assets );
                m_cityParkEye = view.eye;
                m_cityParkTarget = view.target;
                m_cityParkLotId = view.lot->id;
            }
            if( m_captureView == "utilities" )
            {
                const auto view = cityUtilityInspectionView( m_assets );
                m_cityUtilityEye = view.eye;
                m_cityUtilityTarget = view.target;
                m_cityUtilityPrimary = view.primary;
                m_cityUtilityGrate = view.grate;
                m_cityUtilityCovers = view.covers;
                m_cityUtilityGrates = view.grates;
                m_cityUtilityHasGrate = view.nearby;
            }
            if( m_captureView == "junction" )
            {
                const auto connections = cityConnections( *m_assets.roadAuthoring );
                if( connections.junctions.empty() ) throw std::runtime_error( "Junction view requires a fitted ordinary road connection." );
                const scene::CityRoadJunction *selected = nullptr;
                float best = std::numeric_limits<float>::max();
                const auto desired = m_cityOverviewCentre + Vector3F( m_cityOverviewExtent * .45f, 0, m_cityOverviewExtent * .45f );
                for( const auto &connection : connections.junctions )
                {
                    const float dx = connection.center.x - desired.x, dz = connection.center.z - desired.z;
                    const float score = dx * dx + dz * dz + ( connection.ports.size() == 3 ? 0.f : 10000.f );
                    if( score < best ) { best = score; selected = &connection; }
                }
                const auto port = std::min_element( selected->ports.begin(), selected->ports.end(), [&]( const auto &a, const auto &b ) {
                    return source.roads[a.roadIndex].width < source.roads[b.roadIndex].width;
                } );
                auto outward = port->boundary.position - selected->center; outward.y = 0; outward.normalise();
                m_cityConnectionTarget = selected->center + Vector3F( 0, .3f, 0 );
                m_cityConnectionEye = selected->center + outward * 31.f + Vector3F( -outward.z * 5.f, 26.f, outward.x * 5.f );
            }
        }
        else if( m_assets.cityRoadShowcase )
        {
            m_cityExtent=m_cityOverviewExtent=96;m_citySpacing=32;m_cityOverviewCentre={0,3,0};
        }
        else if( m_assets.openCity )
        {
            const auto city = procedural::OpenCityLayout::generate( m_seed, m_assets.cityBlocks, m_assets.cityRoute,
                                                                    m_assets.cityNaturalRoads );
            m_cityExtent = city.extent;
            m_citySpacing = city.spacing;
            m_cityOverviewExtent = city.extent;
            if( city.naturalRoads && !city.roads.nodes.empty() )
            {
                float minX = std::numeric_limits<float>::max(), minZ = minX;
                float maxX = -minX, maxZ = -minX;
                for( const auto &node : city.roads.nodes )
                {
                    minX = std::min( minX, node.x - 9.f ); maxX = std::max( maxX, node.x + 9.f );
                    minZ = std::min( minZ, node.z - 9.f ); maxZ = std::max( maxZ, node.z + 9.f );
                }
                for( const auto &lot : city.lots )
                {
                    minX = std::min( minX, lot.x - lot.width * .5f ); maxX = std::max( maxX, lot.x + lot.width * .5f );
                    minZ = std::min( minZ, lot.z - lot.depth * .5f ); maxZ = std::max( maxZ, lot.z + lot.depth * .5f );
                }
                m_cityOverviewCentre = { ( minX + maxX ) * .5f, 0, ( minZ + maxZ ) * .5f };
                m_cityOverviewExtent = std::max( city.spacing, std::max( maxX - minX, maxZ - minZ ) * .5f + 20.f );
                float best = std::numeric_limits<float>::max();
                const float scale = city.spacing * city.spacing;
                for( const auto &edge : city.roads.edges )
                {
                    if( edge.a >= city.roads.nodes.size() || edge.b >= city.roads.nodes.size() ) continue;
                    const auto &a = city.roads.nodes[edge.a], &b = city.roads.nodes[edge.b];
                    const Vector3F delta( b.x - a.x, 0, b.z - a.z );
                    const float lengthSquared = delta.lengthSquared();
                    if( lengthSquared < 1.f ) continue;
                    const Vector3F centre( ( a.x + b.x ) * .5f, 0, ( a.z + b.z ) * .5f );
                    // Prefer a central avenue with enough uninterrupted length for
                    // both opposing inspection views; never assume a +Z grid street.
                    const float score = ( centre - m_cityOverviewCentre ).lengthSquared() + scale * scale / lengthSquared;
                    if( score < best )
                    {
                        best = score; m_cityStreetCentre = centre;
                        m_cityStreetForward = delta / std::sqrt( lengthSquared );
                    }
                }
            }
            selectFacade( city );
        }
#if WP_GRAPHICS_SYSTEM_CLAW
        if( auto scene = dynamic_pointer_cast<render::ClawScene>(
                applicationManager->getGraphicsSystem()->getGraphicsScene() ) )
            if( !scene->setPrimaryDirectionalLight( m_assets.sun ) )
                throw std::runtime_error( "Cannot select the procedural sky's sunlight." );
#endif
        for( auto &group : m_assets.treeLODs ) group->setForcedLOD( m_forcedLOD );
        if( m_assets.foliageTerrain )
        {
            String error;
            if( !m_assets.foliageTerrain->setFoliageForcedLod( m_forcedLOD, error ) )
                throw std::runtime_error( error );
        }
        m_assets.vehicleLOD->setForcedLOD( m_forcedVehicleLOD );
        // The fixed high-altitude circuit review must retain surface contrast.
        if( m_captureView == "track" && !m_orbitCamera )
            applicationManager->getGraphicsSystem()->getGraphicsScene()->setFog(
                render::IGraphicsScene::FOG_NONE );
        if( auto window = applicationManager->getGraphicsSystem()->getDefaultWindow() )
            window->setSize( Vector2I( m_reviewWidth, m_reviewHeight ) );
        m_boxGround = m_assets.ground;
        m_chassisMeshActor = m_assets.body;
        m_wheelActors = m_assets.wheels;

        m_cameraActor = sceneManager->createActor();
        WP_ASSERT( m_cameraActor );
        m_cameraActor->setName( "Vehicle Follow Camera" );
        m_cameraActor->setSmoothMotion( true );
        const bool terrainRoads=m_assets.roadAuthoring && m_assets.roadTerrain;
        const auto cameraTarget=terrainRoads?m_generatedSpawnPosition:getVehicleSpawnPosition();
        m_cameraActor->setPosition(terrainRoads?cameraTarget+Vector3F(0,cameraHeight,cameraDistance):getInitialCameraPosition());
        m_cameraActor->lookAt(cameraTarget,Vector3<real_Num>::unitY());

        m_cameraController = m_cameraActor->addComponent<scene::VehicleCameraController>();
        WP_ASSERT( m_cameraController );
        m_cameraController->setTarget( m_vehicleActor );
        m_cameraController->setDistance( cameraDistance );
        m_cameraController->setHeight( cameraHeight );

        scene->addActor( m_cameraActor );
        scene->registerAllUpdates( m_cameraActor );

        if( m_camera )
        {
            // The overhead view needs more depth precision to resolve the road's
            // millimetre surface offsets at a distance of nearly 500 metres.
            m_camera->setNearClipDistance( m_captureView == "track" && !m_orbitCamera ? 10.f : .5f );
            m_camera->setFarClipDistance( std::max( 1000.f, m_cityExtent * 6.f ) );
        }
        updateRenderCamera();

        if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
        {
            if( auto debug = m_hudEnabled ? graphicsSystem->getDebug() : nullptr )
            {
                debug->drawText(
                    0, Vector2F( 0.02f, 0.02f ),
                    "W/Up: throttle  S/Down: brake  A/D or Left/Right: steer  R: reset  Esc: quit", 0 );
                debug->drawText( 1, Vector2F( 0.02f, 0.06f ), "Mouse wheel: camera zoom", 0 );
            }
        }
        if( m_sceneProfile )
        {
#if defined(_WIN32) && WP_GRAPHICS_SYSTEM_CLAW
            if(auto renderer=dynamic_pointer_cast<render::ClawRendererDX11>(applicationManager->getGraphicsSystem()->getRenderer()))
                advanced::installMaterialCpuPrograms(m_assets,*renderer);
            if(std::getenv("WP_MATERIAL_SCENE_EDITS"))
                m_materialEdits=std::make_shared<advanced::MaterialSceneCpuEdits>(m_assets);
#endif
            m_sceneProfile->setWorkload( describeProfileScene( m_assets ) );
            m_sceneProfile->checkpoint( "scene_generated" );
        }
    }

    void SampleVehicleAdvanced::updateRenderCamera()
    {
        if( m_cameraSceneNode && m_cameraActor )
        {
            if( auto transform = m_cameraActor->getTransform() )
            {
                m_cameraSceneNode->setTransform( transform->getWorldTransform() );
                if( ( !m_capturePath.empty() || m_benchmarkSeconds > 0 ) && ( m_captureView != "follow" || m_orbitCamera ) )
                {
                    if( m_orbitCamera )
                    {
                        const auto elapsed = core::IApplicationManager::instance()->getTimer()->getTimeSinceSceneLoad();
                        const auto angle = float( std::max( elapsed - 5.0, 0.0 ) * .15 );
                        const auto target = (m_assets.circuit.samples.empty()?m_cityOverviewCentre:m_assets.circuit.samples.front().position) + Vector3F( 0, 1, 0 );
                        const auto radius = 45.f + 35.f * ( 1 + std::sin( angle * .5f ) );
                        m_cameraActor->setPosition( target + Vector3F( std::sin(angle)*radius, 8, std::cos(angle)*radius ) );
                        m_cameraActor->lookAt( target, Vector3F::unitY() );
                    }
                    else if( m_captureView == "corner" && !m_assets.circuit.samples.empty() )
                    {
                        if( m_assets.openCity && !m_assets.cityRoadShowcase )
                        {
                            const auto pose = cityRoadInspectionView( m_assets );
                            m_cameraActor->setPosition( pose[0] );
                            m_cameraActor->lookAt( pose[1], Vector3F::unitY() );
                        }
                        else
                        {
                            const auto &point = m_assets.circuit.samples[m_assets.circuit.samples.size() / 4];
                            m_cameraActor->setPosition( point.position + point.right * 22.f + Vector3F( 0, 10, 0 ) );
                            m_cameraActor->lookAt( point.position, Vector3F::unitY() );
                        }
                    }
                    else if( m_captureView == "junction" )
                    {
                        m_cameraActor->setPosition( m_cityConnectionEye );
                        m_cameraActor->lookAt( m_cityConnectionTarget, Vector3F::unitY() );
                    }
                    else if( m_captureView == "hillside" )
                    {
                        m_cameraActor->setPosition( m_cityHillsideEye );
                        m_cameraActor->lookAt( m_cityHillsideTarget, Vector3F::unitY() );
                    }
                    else if( m_captureView == "verge" )
                    {
                        m_cameraActor->setPosition( m_cityVergeEye );
                        m_cameraActor->lookAt( m_cityVergeTarget, Vector3F::unitY() );
                    }
                    else if( m_captureView == "park" )
                    {
                        m_cameraActor->setPosition( m_cityParkEye );
                        m_cameraActor->lookAt( m_cityParkTarget, Vector3F::unitY() );
                    }
                    else if( m_captureView == "utilities" )
                    {
                        m_cameraActor->setPosition( m_cityUtilityEye );
                        m_cameraActor->lookAt( m_cityUtilityTarget, Vector3F::unitY() );
                    }
                    else if( isRoadTerrainEditView( m_captureView ) && m_roadTerrainEditPrepared )
                    {
                        m_cameraActor->setPosition( m_roadTerrainEditEye );
                        m_cameraActor->lookAt( m_roadTerrainEditTarget, Vector3F::unitY() );
                    }
                    else if( m_captureView == "track" )
                    {
                        if( m_terrainGameplay )
                        {
                            // Include every source edge, both tile borders and
                            // the off-corridor hole; the car has its own close view.
                            m_cameraActor->setPosition( Vector3F( 0, 680, 0 ) );
                            m_cameraActor->lookAt( Vector3F::zero(), Vector3F::unitZ() );
                        }
                        else if( m_assets.cityRoadShowcase )
                        {
                            m_cameraActor->setPosition({110,150,110});m_cameraActor->lookAt({0,3,0},Vector3F::unitY());
                        }
                        else if( m_openCity )
                        {
                            m_cameraActor->setPosition( m_cityOverviewCentre + Vector3F(
                                m_cityOverviewExtent * 1.2f, m_cityOverviewExtent * 1.9f, m_cityOverviewExtent * 1.35f ) );
                            m_cameraActor->lookAt( m_cityOverviewCentre, Vector3F::unitY() );
                        }
                        else
                        {
                            m_cameraActor->setPosition( Vector3F( 75, 480, -30 ) );
                            m_cameraActor->lookAt( Vector3F( 75, 0, -30 ), Vector3F::unitZ() );
                        }
                    }
                    else if( m_captureView == "facade" || m_captureView == "building" || m_captureView == "shopfront" ||
                             m_captureView == "entry-route" )
                    {
                        m_cameraActor->setPosition( m_cityFacadeEye );
                        m_cameraActor->lookAt( m_cityFacadeTarget, Vector3F::unitY() );
                    }
                    else if( m_captureView == "frontage" && m_cityHasJunction )
                    {
                        m_cameraActor->setPosition( m_cityFrontageEye );
                        m_cameraActor->lookAt( m_cityFrontageTarget, Vector3F::unitY() );
                    }
                    else if( m_captureView == "street" || m_captureView == "boulevard" || m_captureView == "frontage" )
                    {
                        if(m_assets.cityTerrainRoads)
                        {
                            if(m_cityHasJunction)
                            {const bool pedestrian=m_captureView=="boulevard";m_cameraActor->setPosition((pedestrian?m_cityBoulevardCentre:m_cityStreetCentre)+Vector3F(0,pedestrian?2.1f:16.f,0));m_cameraActor->lookAt(m_cityJunctionCentre+Vector3F(0,pedestrian?2.6f:2.f,0),Vector3F::unitY());}
                            else{m_cameraActor->setPosition(m_cityStreetCentre+Vector3F(0,4,0));m_cameraActor->lookAt(m_cityStreetCentre+m_cityStreetForward*40.f+Vector3F(0,5,0),Vector3F::unitY());}
                        }
                        else if(m_assets.cityRoadShowcase)
                        {
                            m_cameraActor->setPosition(m_captureView=="street"?Vector3F(24,22,28):Vector3F(-24,22,-28));
                            m_cameraActor->lookAt({0,3,0},Vector3F::unitY());
                        }
                        else
                        {
                        const float travel = m_captureView == "boulevard" ? -1.f : 1.f;
                        const Vector3F right( -m_cityStreetForward.z, 0, m_cityStreetForward.x );
                        m_cameraActor->setPosition( m_cityStreetCentre +
                            m_cityStreetForward * ( travel * m_citySpacing * .12f ) + Vector3F( 0, 3.2f, 0 ) );
                        m_cameraActor->lookAt( m_cityStreetCentre +
                            m_cityStreetForward * ( travel * m_citySpacing * .60f ) +
                            right * ( travel * m_citySpacing * .24f ) + Vector3F( 0, 8, 0 ), Vector3F::unitY() );
                        }
                    }
                    else
                    {
                        auto target = m_vehicleActor->getPosition();
                        m_cameraActor->setPosition( target + Vector3F( -5.5f, 2.2f, -6.5f ) );
                        m_cameraActor->lookAt( target, Vector3F::unitY() );
                    }
                    m_cameraActor->updateTransform();
                    m_cameraSceneNode->setTransform( m_cameraActor->getWorldTransform() );
                }
            }
        }
        if( m_camera && m_cameraActor && m_assets.vehicleLOD )
        {
            auto system = workphone::static_pointer_cast<scene::LODSystem>(
                m_assets.vehicleLOD->getComponentSystem() );
            if( system )
            {
                scene::LODSystem::View view;
                view.position = m_cameraActor->getWorldTransform().getPosition();
                view.verticalFovRadians = Math<f32>::DegToRad( m_camera->getFOVy() );
                view.nearClipDistance = m_camera->getNearClipDistance();
                view.lodBias = m_camera->getLodBias();
                system->setViewOverride( view );
            }
        }
    }

    SampleVehicleAdvanced::InputListener::InputListener() = default;

    SampleVehicleAdvanced::InputListener::~InputListener() = default;

    void SampleVehicleAdvanced::InputListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    Parameter SampleVehicleAdvanced::InputListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::inputEvent )
        {
            auto result = inputEvent( event );
            return Parameter( result );
        }

        return Parameter();
    }

    bool SampleVehicleAdvanced::InputListener::inputEvent( SmartPtr<IInputEvent> event )
    {
        if( !event )
        {
            return false;
        }

        if( auto owner = getOwner() )
        {
            switch( event->getEventType() )
            {
            case IInputEvent::EventType::Key:
            {
                if( auto keyboardState = event->getKeyboardState() )
                {
                    if( keyboardState->isPressedDown() )
                    {
                        if( keyboardState->getKeyCode() == static_cast<u32>( KeyCodes::KEY_KEY_R ) )
                        {
                            owner->reset();
                            return true;
                        }
                        if( keyboardState->getKeyCode() == static_cast<u32>( KeyCodes::KEY_ESCAPE ) )
                        {
                            core::IApplicationManager::instance()->setQuit( true );
                            return true;
                        }
                    }
                }
            }
            break;
            default:
                break;
            };
        }

        return false;
    }

    void SampleVehicleAdvanced::InputListener::setPriority( s32 priority )
    {
        m_priority = priority;
    }

    s32 SampleVehicleAdvanced::InputListener::getPriority() const
    {
        return m_priority;
    }

    SmartPtr<SampleVehicleAdvanced> SampleVehicleAdvanced::InputListener::getOwner() const
    {
        return m_owner.lock();
    }

    void SampleVehicleAdvanced::InputListener::setOwner( SmartPtr<SampleVehicleAdvanced> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone

int main( int argc, char **argv )
{
    using namespace workphone;

    for( int i = 1; i < argc; ++i )
    {
        if( std::string( argv[i] ) == "--validate-circuit" )
        {
            try
            {
                advanced::validateCircuit();
                std::cout << "100 seeded circuits: closure, spacing, spawn and determinism passed\n";
                return 0;
            }
            catch( const std::exception &e )
            {
                std::cerr << e.what() << '\n';
                return 1;
            }
        }
        if( std::string( argv[i] ) == "--help" )
        {
            std::cout
                << "SampleVehicleAdvanced [--seed N] [--quality low|medium|high|cinematic] [--plugins PATH]\n"
                   "  --smoke-test                 Settle, accelerate, turn, brake and reset checks\n"
                   "  --track-smoke-test            Drive one full lap through the physics controller\n"
                   "  --collision-smoke-test        Verify native boxes; circuit garage impact or city road/sidewalk grades\n"
                   "  --validate-circuit           Validate 100 generated closed circuits\n"
                   "  --capture PATH.bmp --view follow|car|track|corner|junction|hillside|street|boulevard|frontage|facade|building|utilities|park|shopfront|entry-route   Save a rendered view and "
                   "exit\n"
                   "  --no-hud --force-lod auto|0|1  Clean captures and tree LOD comparisons\n"
                   "  --force-vehicle-lod auto|0|1|2  Override the vehicle level (clamped for low)\n"
                   "  --benchmark SECONDS [--orbit] Measure after five seconds of warmup\n"
                   "  --scene-profile NEW.json      Profile the existing scene; requires --benchmark\n"
                   "  --scene-profile-mode memory|cpu  Heap checkpoints or CPU accounting (default memory)\n"
                   "  --evidence PATH.json          Write machine-readable review results\n"
                   "  --replay PATH.csv             Apply 120 Hz tick,throttle,brake,steering inputs\n"
                   "  --foliage-world               Use paged instanced broadleaf vegetation\n"
                   "  --city                        Generate the existing open-city scene instead of a circuit\n"
                   "  --city-blocks 6|8|10           Blocks per side (requires --city; default 8)\n"
                   "  --city-route 0|1|2             Street race route (requires --city; default 0)\n"
                   "  --city-layout grid|lsystem     Road network layout (requires --city; default grid)\n"
                   "  --road-authoring-showcase       Terrain-fitted editable roads and roundabout (implies --city)\n"
                   "  --city-terrain-roads            Editable generated city roads fitted to terrain (implies --city)\n"
                   "  --city-terrain-relief 0..2      Initial city terrain relief multiplier (default 1; retained terrain is preserved)\n"
                   "  --city-street-detail 0..100     Roadside detail density (requires --city-terrain-roads, default 100)\n"
                   "  --city-building-style STYLE    Built-in architecture: district, masonry, civic, modern,\n"
                   "                                 terrace, parisian, warehouse, mediterranean, artdeco, timber\n"
                   "  --city-building-mesh ROLE PATH  Replace a wall, window, entrance, corner or roof module (repeatable)\n"
                   "  --view street|boulevard       Opposite street-level city inspections; track is the overview\n"
                   "  --view frontage               Close sidewalk inspection (requires --city-terrain-roads)\n"
                   "  --view utilities              Close inspection of a fitted carriageway cover and nearby grate\n"
                   "  --view park                   Inspect a generated park parcel from street level\n"
                   "  --view shopfront              Inspect a commercial entrance from street level\n"
                   "  --view entry-route            Inspect the space between a commercial entrance and its street\n"
                   "  --view hillside               Inspect a sustained road climb and its surrounding terrain\n"
                   "  --view verge                  Inspect a terrain road end at pavement level\n"
                   "  --view road-edit-before|road-edit-after   Fixed camera after Undo or Redo of a graded road edit\n"
                   "  --view facade                 Inspect a generated building facade from a fixed parcel camera\n"
                   "  --view building               Inspect a representative low-rise building and its roof silhouette\n"
                   "  --export-materials NEW_DIR    Bake asphalt maps and resource source descriptors\n"
                   "  --terrain-gameplay            Native representative terrain and existing vehicle\n"
                   "  --export-terrain-gameplay NEW_DIR  Export its source-free asset closure, then exit\n"
                   "  --terrain-gameplay-package DIR  Load packaged terrain and authoritative vehicle recipe\n"
                   "  --resolution WIDTHxHEIGHT     Set the review window resolution\n"
                   "  --scene-aa none|msaa2|msaa4   Choose DX11 scene multisampling\n"
                   "  --no-audio                    Disable vehicle sounds\n"
                   "  --audio-smoke-test            Check idle, redline, skid and mute playback\n"
                   "  --no-effects                  Disable particles and skid decals\n"
                   "  --effects-smoke-test [--effects-capture PATH.bmp] Check effects and reset\n"
                   "  W/Up throttle; S/Down brake; A/D steer; R reset; Esc quit; wheel camera zoom\n";
            return 0;
        }
    }
    TypeManager *typeManager = nullptr;
    bool ownsTypeManager = false;
    SmartPtr<SampleVehicleAdvanced> app;
    bool smokeTest = false, trackSmokeTest = false, collisionSmokeTest = false;
    bool audioEnabled = true, audioSmokeTest = false;
    bool effectsEnabled = true, effectsSmokeTest = false;
    std::string effectsCapture;
    String pluginsConfig;
    std::string capturePath, captureView = "follow";
    std::string evidencePath, replayPath;
    std::vector<advanced::ReviewControl> replayControls;
    std::string materialExportDirectory;
    bool terrainGameplay = false, seedExplicit = false, qualityExplicit = false;
    std::string terrainGameplayExportDirectory, terrainGameplayPackageDirectory;
    std::string sceneProfilePath, sceneProfileError;
    auto sceneProfileMode = advanced::ProductionSceneProfile::Mode::Memory;
    bool sceneProfileModeExplicit = false;
    std::unique_ptr<advanced::ProductionSceneProfile> sceneProfile;
    u32 seed = 7;
    auto quality = procedural::VehicleAppearanceQuality::High;
    bool hud = true, orbit = false;
    bool foliageWorld = false;
    bool openCity = false, cityOptionsExplicit = false;
    bool cityNaturalRoads = false;
    bool roadAuthoringShowcase = false;
    bool cityTerrainRoads = false;
    float cityTerrainRelief = 1.f;
    bool cityTerrainReliefExplicit = false;
    s32 cityBlocks = 8, cityRoute = 0;
    s32 cityStreetDetail = -1;
    s32 cityBuildingStyle = -1;
    std::array<String, 5> cityBuildingMeshes;
    bool cityBuildingMeshOverride = false;
    s32 forcedLOD = -1;
    s32 forcedVehicleLOD = -1;
    f64 benchmarkSeconds = 0;
    u32 width = 1280, height = 720;
    u32 sceneSamples = 0;
    try
    {
        for( int i = 1; i < argc; ++i )
        {
            if( String( argv[i] ) == "--smoke-test" )
                smokeTest = true;
            else if( String( argv[i] ) == "--terrain-gameplay" )
                terrainGameplay = true;
            else if( String( argv[i] ) == "--export-terrain-gameplay" && i+1<argc )
            { terrainGameplay = true; terrainGameplayExportDirectory = argv[++i]; }
            else if( String( argv[i] ) == "--terrain-gameplay-package" && i+1<argc )
            { terrainGameplay = true; terrainGameplayPackageDirectory = argv[++i]; }
            else if( String( argv[i] ) == "--no-audio" )
                audioEnabled = false;
            else if( String( argv[i] ) == "--scene-aa" )
            {
                if( i + 1 >= argc ) throw std::runtime_error( "--scene-aa requires none, msaa2 or msaa4." );
                const std::string_view mode( argv[++i] );
                if( mode == "none" ) sceneSamples = 1;
                else if( mode == "msaa2" ) sceneSamples = 2;
                else if( mode == "msaa4" ) sceneSamples = 4;
                else throw std::runtime_error( "--scene-aa requires none, msaa2 or msaa4." );
            }
            else if( String( argv[i] ) == "--audio-smoke-test" )
                audioSmokeTest = true;
            else if( String( argv[i] ) == "--no-effects" )
                effectsEnabled = false;
            else if( String( argv[i] ) == "--effects-smoke-test" )
                effectsSmokeTest = true;
            else if( String( argv[i] ) == "--effects-capture" && i + 1 < argc )
                effectsCapture = argv[++i];
            else if( String( argv[i] ) == "--collision-smoke-test" )
                collisionSmokeTest = true;
            else if( String( argv[i] ) == "--no-hud" )
                hud = false;
            else if( String( argv[i] ) == "--orbit" )
                orbit = true;
            else if( String( argv[i] ) == "--foliage-world" )
                foliageWorld = true;
            else if( String( argv[i] ) == "--city" )
                openCity = true;
            else if( String( argv[i] ) == "--road-authoring-showcase" )
            { roadAuthoringShowcase = true; openCity = true; }
            else if( String( argv[i] ) == "--city-terrain-roads" )
            { cityTerrainRoads = true; openCity = true; }
            else if( String( argv[i] ) == "--city-terrain-relief" && i + 1 < argc )
            {
                const std::string_view value = argv[++i];
                const auto end = value.data() + value.size();
                const auto parsed = std::from_chars( value.data(), end, cityTerrainRelief );
                if( parsed.ec != std::errc{} || parsed.ptr != end || !std::isfinite( cityTerrainRelief ) ||
                    cityTerrainRelief < 0 || cityTerrainRelief > 2 )
                    throw std::runtime_error( "City terrain relief must be a finite number from 0 to 2." );
                cityTerrainReliefExplicit = true;
            }
            else if( String( argv[i] ) == "--city-street-detail" && i + 1 < argc )
            {
                const std::string_view value = argv[++i];
                const auto end = value.data() + value.size();
                const auto parsed = std::from_chars( value.data(), end, cityStreetDetail );
                if( parsed.ec != std::errc{} || parsed.ptr != end || cityStreetDetail < 0 || cityStreetDetail > 100 )
                    throw std::runtime_error( "City street detail must be an integer from 0 to 100." );
            }
            else if( String( argv[i] ) == "--city-building-style" && i + 1 < argc )
            {
                const std::string_view value = argv[++i];
                const auto found = std::find( cityBuildingStyles.begin(), cityBuildingStyles.end(), value );
                if( found == cityBuildingStyles.end() )
                    throw std::runtime_error( "City building style must be district, masonry, civic, modern, terrace, parisian, warehouse, mediterranean, artdeco, or timber." );
                cityBuildingStyle = static_cast<s32>( found - cityBuildingStyles.begin() );
            }
            else if( String( argv[i] ) == "--city-building-mesh" && i + 2 < argc )
            {
                const std::string_view role = argv[++i];
                const auto found = std::find( cityBuildingMeshRoles.begin(), cityBuildingMeshRoles.end(), role );
                if( found == cityBuildingMeshRoles.end() )
                    throw std::runtime_error( "City building mesh role must be wall, window, entrance, corner, or roof." );
                auto &mesh = cityBuildingMeshes[found - cityBuildingMeshRoles.begin()];
                const std::string_view path = argv[++i];
                if( !mesh.empty() || path.empty() || path.rfind( "--", 0 ) == 0 )
                    throw std::runtime_error( "Specify one nonempty mesh path for each city building role." );
                const auto absolutePath = std::filesystem::absolute( std::filesystem::path( path ) ).lexically_normal();
                mesh = absolutePath.string();
                cityBuildingMeshOverride = true;
            }
            else if( String( argv[i] ) == "--city-blocks" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                if( value != "6" && value != "8" && value != "10" )
                    throw std::runtime_error( "City blocks must be 6, 8 or 10." );
                cityBlocks = std::stoi( value ); cityOptionsExplicit = true;
            }
            else if( String( argv[i] ) == "--city-route" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                if( value != "0" && value != "1" && value != "2" )
                    throw std::runtime_error( "City route must be 0, 1 or 2." );
                cityRoute = std::stoi( value ); cityOptionsExplicit = true;
            }
            else if( String( argv[i] ) == "--city-layout" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                if( value != "grid" && value != "lsystem" )
                    throw std::runtime_error( "City layout must be grid or lsystem." );
                cityNaturalRoads = value == "lsystem"; cityOptionsExplicit = true;
            }
            else if( String( argv[i] ) == "--force-lod" && i + 1 < argc )
            {
                const String value = argv[++i];
                if( value != "auto" && value != "0" && value != "1" )
                    throw std::runtime_error( "Forced LOD must be auto, 0, or 1." );
                forcedLOD = value == "auto" ? -1 : value == "0" ? 0 : 1;
            }
            else if( String( argv[i] ) == "--force-vehicle-lod" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                if( value != "auto" && value != "0" && value != "1" && value != "2" )
                    throw std::runtime_error( "Vehicle LOD must be auto, 0, 1 or 2." );
                forcedVehicleLOD = value == "auto" ? -1 : std::stoi( value );
            }
            else if( String( argv[i] ) == "--benchmark" && i + 1 < argc )
            {
                size_t consumed = 0;
                const std::string value = argv[++i];
                benchmarkSeconds = std::stod( value, &consumed );
                if( consumed != value.size() || !std::isfinite( benchmarkSeconds ) ||
                    benchmarkSeconds <= 0 || benchmarkSeconds > 3600 )
                    throw std::runtime_error(
                        "Benchmark duration must be between zero and 3600 seconds." );
            }
            else if( String( argv[i] ) == "--resolution" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                const auto separator = value.find( 'x' );
                if( separator == std::string::npos )
                    throw std::runtime_error( "Resolution must be WIDTHxHEIGHT." );
                size_t left = 0, right = 0;
                width = std::stoul( value.substr( 0, separator ), &left );
                height = std::stoul( value.substr( separator + 1 ), &right );
                if( left != separator || right != value.size() - separator - 1 || width < 320 ||
                    height < 180 || width > 7680 || height > 4320 )
                    throw std::runtime_error( "Resolution must be between 320x180 and 7680x4320." );
            }
            else if( String( argv[i] ) == "--track-smoke-test" )
                trackSmokeTest = true;
            else if( String( argv[i] ) == "--plugins" && i + 1 < argc )
                pluginsConfig = argv[++i];
            else if( String( argv[i] ) == "--capture" && i + 1 < argc )
                capturePath = argv[++i];
            else if( String( argv[i] ) == "--evidence" && i + 1 < argc )
                evidencePath = argv[++i];
            else if( String( argv[i] ) == "--scene-profile" && i + 1 < argc )
                sceneProfilePath = argv[++i];
            else if( String( argv[i] ) == "--scene-profile-mode" && i + 1 < argc )
            {
                const std::string mode = argv[++i];
                if( mode != "memory" && mode != "cpu" )
                    throw std::runtime_error( "Scene profile mode must be memory or cpu." );
                sceneProfileMode = mode == "memory" ? advanced::ProductionSceneProfile::Mode::Memory
                                                      : advanced::ProductionSceneProfile::Mode::Cpu;
                sceneProfileModeExplicit = true;
            }
            else if( String( argv[i] ) == "--replay" && i + 1 < argc )
                replayPath = argv[++i];
            else if( String( argv[i] ) == "--export-materials" && i + 1 < argc )
                materialExportDirectory = argv[++i];
            else if( String( argv[i] ) == "--view" && i + 1 < argc )
                captureView = argv[++i];
            else if( String( argv[i] ) == "--seed" && i + 1 < argc )
            { seed = static_cast<u32>( std::stoul( argv[++i] ) ); seedExplicit = true; }
            else if( String( argv[i] ) == "--quality" && i + 1 < argc )
            {
                qualityExplicit = true;
                const String value = argv[++i];
                if( value == "low" )
                    quality = procedural::VehicleAppearanceQuality::Preview;
                else if( value == "medium" )
                    quality = procedural::VehicleAppearanceQuality::Standard;
                else if( value == "high" )
                    quality = procedural::VehicleAppearanceQuality::High;
                else if( value == "cinematic" )
                    quality = procedural::VehicleAppearanceQuality::Cinematic;
                else
                    throw std::runtime_error( "Quality must be low, medium, high, or cinematic." );
            }
            else
                throw std::runtime_error( std::string( "Unknown option or missing value: " ) + argv[i] );
        }
        if( terrainGameplay )
        {
#if !WP_TERRAIN_GAMEPLAY_PACKAGE
            throw std::runtime_error("Terrain gameplay packaging is unavailable in this build.");
#endif
            if( openCity || cityOptionsExplicit || foliageWorld || !materialExportDirectory.empty() )
                throw std::runtime_error("Terrain gameplay cannot combine city, foliage-world or material source export modes.");
            if( !terrainGameplayExportDirectory.empty() && !terrainGameplayPackageDirectory.empty() )
                throw std::runtime_error("Choose terrain gameplay export or package consumption.");
            if( !terrainGameplayPackageDirectory.empty() && (seedExplicit || qualityExplicit) )
                throw std::runtime_error("Terrain gameplay packages own their seed and quality; explicit overrides are unavailable.");
            if( !terrainGameplayPackageDirectory.empty() && !sceneProfilePath.empty() )
                throw std::runtime_error("Packaged terrain gameplay scene profiling is unavailable until profile metadata uses the pinned recipe.");
            if( trackSmokeTest || collisionSmokeTest || audioSmokeTest || effectsSmokeTest )
                throw std::runtime_error("Terrain gameplay supports --smoke-test and capture validation; other scenario smoke tests are unavailable.");
            if( !terrainGameplayExportDirectory.empty() &&
                (smokeTest || !capturePath.empty() || benchmarkSeconds>0 || !sceneProfilePath.empty()) )
                throw std::runtime_error("Terrain gameplay export exits after loading; choose a separate gameplay validation run.");
            if( !terrainGameplayPackageDirectory.empty() )
            { audioEnabled = false; hud = false; }
        }
        if( captureView != "follow" && captureView != "car" && captureView != "track" &&
            captureView != "corner" && captureView != "junction" && captureView != "hillside" && captureView != "verge" && captureView != "street" && captureView != "boulevard" &&
            captureView != "frontage" && captureView != "facade" && captureView != "building" && captureView != "utilities" && captureView != "park" && captureView != "shopfront" && captureView != "entry-route" &&
            !isRoadTerrainEditView( captureView ) )
            throw std::runtime_error( "View must be follow, car, track, corner, junction, hillside, verge, street, boulevard, frontage, facade, building, utilities, park, shopfront, entry-route, road-edit-before, or road-edit-after." );
        if( ( cityOptionsExplicit || captureView == "street" || captureView == "boulevard" ) && !openCity )
            throw std::runtime_error( "City options and street-level views require --city." );
        if(roadAuthoringShowcase && foliageWorld)throw std::runtime_error("Road authoring showcase does not provide foliage-world generation.");
        if(roadAuthoringShowcase && cityTerrainRoads)throw std::runtime_error("Choose road-authoring-showcase or city-terrain-roads.");
        if( cityStreetDetail >= 0 && !cityTerrainRoads )
            throw std::runtime_error( "--city-street-detail requires --city-terrain-roads." );
        if( cityTerrainReliefExplicit && !cityTerrainRoads )
            throw std::runtime_error( "--city-terrain-relief requires --city-terrain-roads." );
        if( captureView == "frontage" && !cityTerrainRoads )
            throw std::runtime_error( "--view frontage requires --city-terrain-roads." );
        if( captureView == "utilities" && ( !cityTerrainRoads || orbit ) )
            throw std::runtime_error( "--view utilities requires --city-terrain-roads and a fixed camera." );
        if( captureView == "park" && ( !cityTerrainRoads || orbit ) )
            throw std::runtime_error( "--view park requires --city-terrain-roads and a fixed camera." );
        if( captureView == "shopfront" && ( !cityTerrainRoads || orbit ) )
            throw std::runtime_error( "--view shopfront requires --city-terrain-roads and a fixed camera." );
        if( captureView == "entry-route" && ( !cityTerrainRoads || orbit ) )
            throw std::runtime_error( "--view entry-route requires --city-terrain-roads and a fixed camera." );
        if( captureView == "junction" && !cityTerrainRoads )
            throw std::runtime_error( "--view junction requires --city-terrain-roads." );
        if( captureView == "hillside" && !cityTerrainRoads )
            throw std::runtime_error( "--view hillside requires --city-terrain-roads." );
        if( captureView == "verge" && !cityTerrainRoads )
            throw std::runtime_error( "--view verge requires --city-terrain-roads." );
        if( isRoadTerrainEditView( captureView ) && ( !cityTerrainRoads || orbit || capturePath.empty() ) )
            throw std::runtime_error( "Road edit views require --city-terrain-roads and a fixed capture camera." );
        if( (cityBuildingMeshOverride || cityBuildingStyle >= 0 || captureView == "facade" || captureView == "building") &&
            (!openCity || roadAuthoringShowcase) )
            throw std::runtime_error( "City building mesh overrides, styles, and the facade/building views require a generated city." );
        if(cityTerrainRoads && !materialExportDirectory.empty())throw std::runtime_error("Terrain-city source material export is unavailable.");
        if( int(smokeTest) + int(trackSmokeTest) + int(collisionSmokeTest) + int(audioSmokeTest) + int(effectsSmokeTest) > 1 ||
            ( ( !capturePath.empty() || benchmarkSeconds > 0 ) && ( smokeTest || trackSmokeTest || collisionSmokeTest || audioSmokeTest || effectsSmokeTest ) ) )
            throw std::runtime_error( "Choose one smoke test or a capture per run." );
        if( audioSmokeTest && !audioEnabled )
            throw std::runtime_error( "--audio-smoke-test requires audio to be enabled." );
        if( (effectsSmokeTest && !effectsEnabled) || (!effectsCapture.empty() && !effectsSmokeTest) )
            throw std::runtime_error("Effects capture requires --effects-smoke-test with effects enabled.");
        if( orbit && benchmarkSeconds <= 0 ) throw std::runtime_error( "--orbit requires --benchmark." );
        if( ( !evidencePath.empty() || !replayPath.empty() ) && capturePath.empty() && benchmarkSeconds <= 0 )
            throw std::runtime_error( "--evidence and --replay require a capture or benchmark run." );
        if( advanced::reviewPathsAlias( evidencePath, capturePath ) || advanced::reviewPathsAlias( evidencePath, replayPath ) ||
            advanced::reviewPathsAlias( capturePath, replayPath ) )
            throw std::runtime_error( "Evidence output must differ from capture and replay paths." );
        for( const auto &mesh : cityBuildingMeshes )
        {
            for( const auto &output : { capturePath, evidencePath, effectsCapture, sceneProfilePath } )
                if( advanced::reviewPathsAlias( mesh.c_str(), output ) )
                    throw std::runtime_error( "Capture outputs must differ from city building mesh inputs." );
        }
        if( sceneProfileModeExplicit && sceneProfilePath.empty() )
            throw std::runtime_error( "--scene-profile-mode requires --scene-profile." );
        if( !sceneProfilePath.empty() )
        {
            if( benchmarkSeconds <= 0 ) throw std::runtime_error( "--scene-profile requires --benchmark." );
#if !defined( _WIN32 ) || !WP_GRAPHICS_SYSTEM_CLAW
            throw std::runtime_error( "Production scene profiling requires Windows and Claw graphics." );
#endif
            const auto output = std::filesystem::absolute( sceneProfilePath );
            if( std::filesystem::exists( output ) )
                throw std::runtime_error( "Choose a new scene profile output; existing evidence is preserved." );
            if( !std::filesystem::is_directory( output.parent_path() ) )
                throw std::runtime_error( "Scene profile output directory must already exist." );
            for( const auto &otherOutput : { evidencePath, capturePath, effectsCapture } )
                if( !otherOutput.empty() && std::filesystem::exists( std::filesystem::path( otherOutput ) ) )
                    throw std::runtime_error( "Choose new profile capture/evidence outputs; existing files are preserved." );
            for( const auto &other : { evidencePath, capturePath, replayPath, effectsCapture,
                    materialExportDirectory, std::string( pluginsConfig.empty() ? "wp_plugins_samples.cfg" : pluginsConfig.c_str() ) } )
                if( advanced::reviewPathsAlias( sceneProfilePath, other ) )
                    throw std::runtime_error( "Scene profile output must differ from every input and other output." );
            sceneProfile = std::make_unique<advanced::ProductionSceneProfile>( output.string(), sceneProfileMode );
            sceneProfile->configure( seed, int( quality ), width, height, captureView, orbit,
                foliageWorld, audioEnabled, effectsEnabled, benchmarkSeconds, replayPath,
                pluginsConfig.empty() ? "wp_plugins_samples.cfg" : pluginsConfig.c_str(), forcedLOD, forcedVehicleLOD );
        }
        if( !replayPath.empty() ) replayControls = advanced::loadReviewReplay( replayPath );
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << '\n';
        return 2;
    }
    int exitCode = 1;
    const bool unattended=smokeTest || trackSmokeTest || collisionSmokeTest || audioSmokeTest || effectsSmokeTest ||
        !capturePath.empty() || benchmarkSeconds>0 || bool(sceneProfile) || terrainGameplay;

    auto unloadApplication = [&app, &sceneProfile]() {
        if( app )
        {
            if( sceneProfile ) sceneProfile->endMeasurement();
            if( app->getLoadingState() != LoadingState::Unloaded )
            {
                app->unload( nullptr );
            }

            app = nullptr;
            if( sceneProfile ) sceneProfile->checkpoint( "application_unloaded" );
        }

        core::IApplicationManager::setInstance( nullptr );
    };

    auto unloadTypeManager = [&typeManager, &ownsTypeManager, &sceneProfile]() {
        if( ownsTypeManager && typeManager )
        {
            typeManager->unload();
            delete typeManager;
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
            ownsTypeManager = false;
            if( sceneProfile ) sceneProfile->checkpoint( "type_manager_destroyed" );
        }
    };

    try
    {
        typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            typeManager->load();
            TypeManager::setInstance( typeManager );
            ownsTypeManager = true;
        }

        app = workphone::make_ptr<SampleVehicleAdvanced>();
        app->setSceneProfile( sceneProfile.get() );
        app->setActiveThreads( activeThreadCount );
        app->setSmokeTest( smokeTest );
        app->setTrackSmokeTest( trackSmokeTest );
        app->setCollisionSmokeTest( collisionSmokeTest );
        app->setGenerationOptions( seed, quality );
        app->setAudioOptions( audioEnabled, audioSmokeTest );
        app->setEffectsOptions( effectsEnabled, effectsSmokeTest, effectsCapture );
        app->setCapture( capturePath, captureView );
        app->setReviewOptions( hud, forcedLOD, forcedVehicleLOD, benchmarkSeconds, orbit, width, height );
        app->setSceneAA( sceneSamples );
        app->setEvidenceOptions( evidencePath, replayPath, std::move( replayControls ) );
        app->setFoliageWorldEnabled( foliageWorld );
        app->setCityOptions( openCity, cityBlocks, cityRoute, cityNaturalRoads );
        app->setRoadAuthoringShowcase( roadAuthoringShowcase );
        app->setCityTerrainRoads( cityTerrainRoads );
        app->setCityTerrainRelief( cityTerrainRelief );
        app->setCityStreetDetail( cityStreetDetail );
        app->setCityBuildingMeshes( cityBuildingMeshes );
        app->setCityBuildingStyle( cityBuildingStyle );
        app->setMaterialExportDirectory( materialExportDirectory );
        app->setTerrainGameplayOptions(terrainGameplay,terrainGameplayPackageDirectory,!terrainGameplayExportDirectory.empty());
        if( !pluginsConfig.empty() )
            app->setPluginsConfigFilePath( pluginsConfig );
        app->load( nullptr );
        if( app->getLoadingState() != LoadingState::Loaded )
            throw std::runtime_error( "SampleVehicleAdvanced failed to load." );
        if( !terrainGameplayExportDirectory.empty() )
        {
            app->exportTerrainGameplayPackage(terrainGameplayExportDirectory);
            exitCode = 0;
        }
        else
        {
            app->run();
            exitCode = ( ( smokeTest || trackSmokeTest || collisionSmokeTest || audioSmokeTest || effectsSmokeTest ) && !app->smokeTestPassed() ) ||
                           ( ( !capturePath.empty() || benchmarkSeconds > 0 ) && !app->capturePassed() ) ||
                           !app->terrainGameplayPassed()
                       ? 1
                       : 0;
        }
        if( sceneProfile && !sceneProfile->invariantsPassed() )
        {
            sceneProfileError = "Production scene profile did not complete its rendering/telemetry invariants.";
            std::cerr << sceneProfileError << '\n';
            exitCode = 1;
        }

        unloadApplication();
        unloadTypeManager();
    }
    catch( Exception &e )
    {
        sceneProfileError = e.what();
        unloadApplication();
        unloadTypeManager();
        if( unattended ) std::cerr << sceneProfileError << '\n';
        else MessageBoxUtil::show( e.what() );
    }
    catch( std::exception &e )
    {
        sceneProfileError = e.what();
        unloadApplication();
        unloadTypeManager();
        if( unattended ) std::cerr << sceneProfileError << '\n';
        else MessageBoxUtil::show( e.what() );
    }
    catch( ... )
    {
        sceneProfileError = "Unknown error";
        unloadApplication();
        unloadTypeManager();
        if( unattended ) std::cerr << sceneProfileError << '\n';
        else MessageBoxUtil::show( "Unknown error" );
    }

    if( sceneProfile )
    {
        try { sceneProfile->write( exitCode, sceneProfileError ); }
        catch( const std::exception &e ) { std::cerr << e.what() << '\n'; exitCode = 1; }
    }

    return exitCode;
}
