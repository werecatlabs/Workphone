#include <Workphone/Workphone.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <WPProcedural/ProceduralModel.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/WPVehicleGeometry.hpp>
#include <limits>
#include <stdexcept>
using namespace workphone;
using namespace workphone::procedural;
namespace {
void requireMesh( bool condition, const char *message ) {
    if( !condition ) throw std::runtime_error( message );
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
}
