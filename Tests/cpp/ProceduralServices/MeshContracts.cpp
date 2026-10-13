#include <Workphone/Workphone.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <WPProcedural/ProceduralModel.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/WPVehicleGeometry.hpp>
#include <limits>
#include <array>
#include <cmath>
#include <algorithm>
#include <stdexcept>
using namespace workphone;
using namespace workphone::procedural;
namespace {
void requireMesh( bool condition, const char *message ) {
    if( !condition ) throw std::runtime_error( message );
}

void checkHeroGeometry( const VehicleGeometryConfig &config )
{
    const auto car = WPVehicleGeometry::generate( config );
    requireMesh( car.lods.size() == 3, "Hero geometry must retain three LODs" );
    const float frontZ = -config.wheelbase * .45f;
    const float rearZ = frontZ + config.wheelbase;
    requireMesh( std::abs( car.frontAxle.z - frontZ ) < 1e-6f &&
                 std::abs( car.rearAxle.z - rearZ ) < 1e-6f &&
                 car.frontAxle.y == config.tyreRadius && car.rearAxle.y == config.tyreRadius,
                 "Hero body refinement moved authoritative axle pivots" );
    size_t previous = std::numeric_limits<size_t>::max();
    for( const auto &lod : car.lods )
    {
        size_t triangles = 0;
        bool smoothWing = false, curvedWing = false, recessedLeft = false, recessedRight = false, halo = false;
        std::array<Vector3F, 4> tyreMin, tyreMax;
        std::array<bool, 4> foundTyre{};
        const float largest = std::numeric_limits<float>::max();
        tyreMin.fill( Vector3F( largest, largest, largest ) );
        tyreMax.fill( Vector3F( -largest, -largest, -largest ) );
        for( const auto &section : lod.sections )
        {
            requireMesh( section.indices.size() % 3 == 0, "Hero section has incomplete triangles" );
            triangles += section.indices.size() / 3;
            std::vector<unsigned> references( section.vertices.size(), 0 );
            for( auto index : section.indices )
            {
                requireMesh( index < section.vertices.size(), "Hero index exceeds vertex count" );
                ++references[index];
            }
            for( size_t i = 0; i < section.vertices.size(); ++i )
            {
                const auto &v = section.vertices[i];
                const auto finite = []( const Vector3F &value ) {
                    return std::isfinite( value.x ) && std::isfinite( value.y ) && std::isfinite( value.z );
                };
                requireMesh( finite( v.position ) && finite( v.normal ) && finite( v.tangent ) &&
                             std::isfinite( v.tangentSign ) && std::isfinite( v.uv.x ) && std::isfinite( v.uv.y ),
                             "Hero geometry contains non-finite attributes" );
                requireMesh( std::abs( v.normal.length() - 1.f ) < .002f &&
                             std::abs( v.tangent.length() - 1.f ) < .002f &&
                             std::abs( v.normal.dotProduct( v.tangent ) ) < .002f &&
                             std::abs( std::abs( v.tangentSign ) - 1.f ) < 1e-6f,
                             "Hero normal/tangent frame is not orthonormal" );
                requireMesh( v.position.x >= car.boundsMin.x && v.position.x <= car.boundsMax.x &&
                             v.position.y >= car.boundsMin.y && v.position.y <= car.boundsMax.y &&
                             v.position.z >= car.boundsMin.z && v.position.z <= car.boundsMax.z,
                             "Hero bounds exclude refined geometry" );
                if( !references[i] ) continue;
                if( section.name == "aero_carbon_matte" && std::abs( v.position.x ) < .08f &&
                    v.position.y > .95f && v.position.z < -.24f && v.position.z > -.33f ) halo = true;
                if( section.name == "aero_carbon_matte" && v.position.z < frontZ - .75f && v.normal.y > .5f )
                {
                    smoothWing |= references[i] >= 5;
                    curvedWing |= std::abs( v.normal.x ) > .02f;
                }
                if( section.name == "aero_carbon_matte" && v.position.z > .15f && v.position.z < .25f &&
                    v.position.y > .3f && v.position.y < .6f )
                {
                    recessedLeft |= v.position.x < -.35f && v.position.x > -.8f;
                    recessedRight |= v.position.x > .35f && v.position.x < .8f;
                }
                if( section.material == VehicleMaterial::Tyre )
                {
                    const unsigned wheel = ( v.position.z > ( frontZ + rearZ ) * .5f ? 2u : 0u ) +
                                           ( v.position.x > 0 ? 1u : 0u );
                    foundTyre[wheel] = true;
                    for( unsigned axis = 0; axis < 3; ++axis )
                    {
                        tyreMin[wheel][axis] = std::min( tyreMin[wheel][axis], v.position[axis] );
                        tyreMax[wheel][axis] = std::max( tyreMax[wheel][axis], v.position[axis] );
                    }
                }
            }
            for( size_t i = 0; i < section.indices.size(); i += 3 )
            {
                const auto &a = section.vertices[section.indices[i]].position;
                const auto &b = section.vertices[section.indices[i + 1]].position;
                const auto &c = section.vertices[section.indices[i + 2]].position;
                const auto area = ( b - a ).crossProduct( c - a );
                requireMesh( area.dotProduct( area ) > 1e-16f, "Hero geometry contains a degenerate triangle" );
            }
        }
        requireMesh( smoothWing && curvedWing, "Hero wing lost smooth shared curved surface" );
        requireMesh( recessedLeft && recessedRight, "Hero sidepod lost recessed inlet depth" );
        requireMesh( halo, "Primary halo silhouette disappears at a vehicle LOD" );
        for( unsigned wheel = 0; wheel < 4; ++wheel )
        {
            requireMesh( foundTyre[wheel], "Hero geometry lost a tyre" );
            const auto centre = ( tyreMin[wheel] + tyreMax[wheel] ) * .5f;
            const float track = wheel < 2 ? config.frontTrack : config.rearTrack;
            const float expectedX = ( wheel & 1u ? .5f : -.5f ) * track;
            requireMesh( std::abs( centre.x - expectedX ) < 1e-5f &&
                         std::abs( centre.y - config.tyreRadius ) < 1e-5f &&
                         std::abs( centre.z - ( wheel < 2 ? frontZ : rearZ ) ) < 1e-5f &&
                         std::abs( tyreMax[wheel].y - tyreMin[wheel].y - 2 * config.tyreRadius ) < 1e-5f,
                         "Hero tyre shape no longer matches physics hub/radius" );
        }
        requireMesh( triangles < previous, "Hero refined LOD does not strictly reduce triangles" );
        previous = triangles;
    }
}

}
void checkMeshContracts()
{
    auto generator = workphone::make_ptr<MeshGeneratorDefault>();
    requireMesh( generator->isReady(), "Default mesh generator is not ready" );
    generator->setMeshQuality( std::numeric_limits<real_Num>::quiet_NaN() );
    requireMesh( generator->getMeshQuality() == 1, "Mesh quality accepts NaN" );
    generator->setUVScale( 2 );
    generator->setMaxTriangleCount( 4 );
    auto terrain = workphone::make_ptr<CProceduralTerrain>();
    terrain->setSize( Vector3F( 10, 2, 20 ) );
    terrain->setHeightmapResolution( Vector2I( 2, 2 ) );
    terrain->setHeightData( Array<f32>{ 0, 0.5f, 0, 0.5f } );
    auto mesh = generator->generateFromTerrain( terrain );
    requireMesh( static_cast<bool>( mesh ), "Valid terrain generation failed" );
    requireMesh( mesh->getSubMesh( 0 )->getIndexBuffer()->getNumIndices() == 6, "Terrain topology incorrect" );
    requireMesh( generator->getGenerationProgress() == 1, "Terrain progress did not complete" );
    requireMesh( generator->getAllGeneratedMeshes().size() == 1, "Terrain history missing" );
    auto application = core::IApplicationManager::instance();
    const auto meshManager = application->getMeshManager();
    application->setMeshManager( nullptr );
    requireMesh( !generator->generateFromTerrain( terrain ) && generator->getLastGeneratedMesh() == mesh,
                 "Missing host mesh manager must fail without replacing previous output" );
    application->setMeshManager( meshManager );
    generator->setMaxVertexCount( 3 );
    requireMesh( !generator->generateFromTerrain( terrain ), "Oversized terrain accepted" );
    requireMesh( generator->getLastGeneratedMesh() == mesh, "Failure replaced previous result" );
    generator->setMaxVertexCount( 100 );
    generator->setNumLodLevels( 2 );
    requireMesh( !generator->isReady() && !generator->generateFromTerrain( terrain ), "Unsupported LOD reported success" );
    generator->setNumLodLevels( 1 );
    terrain->setHeightData( Array<f32>{ 0, std::numeric_limits<f32>::infinity(), 0, 0 } );
    requireMesh( !generator->generateFromTerrain( terrain ), "Non-finite terrain accepted" );
    auto road = workphone::make_ptr<CRoad>();
    auto start = workphone::make_ptr<CRoadNode>();
    auto finish = workphone::make_ptr<CRoadNode>();
    start->setPosition( Vector3F( 0, 0, 0 ) );
    finish->setPosition( Vector3F( 0, 0, 10 ) );
    road->addNode( start );
    road->addNode( finish );
    auto network = workphone::make_ptr<CRoadNetwork>();
    network->getRoads().push_back( road );
    auto roadMesh = generator->generate( network );
    if( !roadMesh ) throw std::runtime_error( "Valid road adapter failed: " + generator->getGenerationStatus() );
    generator->clear();
    requireMesh( !generator->getLastGeneratedMesh() && generator->getAllGeneratedMeshes().empty() && generator->getGenerationProgress() == 0, "Clear retained generated meshes" );
    ProceduralMesh source;
    source.addVertex( ProceduralVertex{ Vector3F( -1, 2, 3 ), {}, Vector2F( 0.25f, 0.5f ), Vector3F( 0.1f, 0.2f, 0.3f ) } );
    source.addVertex( Vector3F( 1, 2, 3 ) );
    source.addVertex( Vector3F( 0, 4, 3 ) );
    source.addTriangle( 0, 1, 2 );
    ProceduralMesh output;
    output.append( source, Vector3F( 10, 0, 0 ) );
    output.computeBoundingBox();
    requireMesh( output.boundsValid && output.bounds.getMinimum().x == 9 && output.bounds.getMaximum().y == 4, "Procedural bounds incorrect" );
    requireMesh( output.vertices[0].uv.x == 0.25f && output.vertices[0].mask.z == 0.3f, "Append lost vertex attributes" );
    source.indices[2] = 100;
    output.append( source );
    requireMesh( output.indices.size() == 3 && output.vertices.size() == 3, "Invalid append mutated output" );
    output.append( output );
    requireMesh( output.indices.size() == 6 && output.vertices.size() == 6, "Self append failed" );
    output.vertices[0].position.x = std::numeric_limits<float>::quiet_NaN();
    output.computeBoundingBox();
    requireMesh( !output.hasGeometry() && !output.boundsValid, "Non-finite geometry accepted" );
    output.clear();
    requireMesh( !output.boundsValid && !output.hasGeometry(), "Clear retained geometry bounds" );
    const VehicleGeometryConfig carConfig;
    const auto car = WPVehicleGeometry::generate( carConfig );
    requireMesh( car.lods.size() == 3, "Hero vehicle LOD count changed" );
    requireMesh( car.frontAxle.z == -carConfig.wheelbase * 0.45f && car.rearAxle.z == car.frontAxle.z + carConfig.wheelbase,
                 "Hero geometry changed physics wheel pivots" );
    size_t previousTriangles = std::numeric_limits<size_t>::max();
    for( const auto &lod : car.lods )
    {
        bool referencedLeftPod = false, referencedRightPod = false, referencedAirbox = false;
        size_t triangleCount = 0;
        for( const auto &section : lod.sections )
        {
            triangleCount += section.indices.size() / 3;
            for( auto index : section.indices )
            {
                requireMesh( index < section.vertices.size(), "Vehicle triangle index out of range" );
                const auto &v = section.vertices[index];
                requireMesh( std::isfinite( v.position.x ) && std::isfinite( v.position.y ) && std::isfinite( v.position.z ) &&
                             std::isfinite( v.normal.x ) && std::isfinite( v.tangent.x ), "Vehicle has non-finite vertex frame" );
                requireMesh( v.position.x >= car.boundsMin.x && v.position.x <= car.boundsMax.x &&
                             v.position.y >= car.boundsMin.y && v.position.y <= car.boundsMax.y &&
                             v.position.z >= car.boundsMin.z && v.position.z <= car.boundsMax.z, "Vehicle bounds exclude aero geometry" );
                if( section.material == VehicleMaterial::Paint && v.position.z > 0.25f && v.position.z < 1.2f )
                {
                    referencedLeftPod |= v.position.x < -0.72f;
                    referencedRightPod |= v.position.x > 0.72f;
                    referencedAirbox |= v.position.y > 1.1f;
                }
            }
        }
        requireMesh( referencedLeftPod && referencedRightPod && referencedAirbox, "Vehicle silhouette loft vertices are not referenced by triangles" );
        requireMesh( triangleCount < previousTriangles, "Vehicle LOD did not reduce geometry" );
        previousTriangles = triangleCount;
    }
    checkHeroGeometry( carConfig );
    auto shortCar = carConfig;
    shortCar.wheelbase = 2.f;
    shortCar.bodyLength = 2.7f;
    checkHeroGeometry( shortCar );
    auto longCar = carConfig;
    longCar.wheelbase = 5.f;
    longCar.bodyLength = 5.7f;
    checkHeroGeometry( longCar );
    auto silhouetteCar = carConfig;
    silhouetteCar.includeFineDetails = false;
    checkHeroGeometry( silhouetteCar );
    const auto repeatedCar = WPVehicleGeometry::generate( carConfig );
    for( size_t lod = 0; lod < car.lods.size(); ++lod )
    {
        requireMesh( car.lods[lod].sections.size() == repeatedCar.lods[lod].sections.size(),
                     "Default hero recipe changed section count on replay" );
        for( size_t section = 0; section < car.lods[lod].sections.size(); ++section )
        {
            const auto &a = car.lods[lod].sections[section];
            const auto &b = repeatedCar.lods[lod].sections[section];
            requireMesh( a.name == b.name && a.material == b.material && a.indices == b.indices &&
                         a.vertices.size() == b.vertices.size(), "Default hero topology is not repeatable" );
            for( size_t vertex = 0; vertex < a.vertices.size(); ++vertex )
            {
                const auto &x = a.vertices[vertex];
                const auto &y = b.vertices[vertex];
                requireMesh( x.position == y.position && x.normal == y.normal && x.tangent == y.tangent &&
                             x.uv == y.uv && x.mask == y.mask && x.tangentSign == y.tangentSign,
                             "Default hero vertex attributes are not repeatable" );
            }
        }
    }

}
