#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/ProceduralBind.hpp"
#include <Workphone/WorkphoneInterface.hpp>
#include <luabind/luabind.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>

namespace workphone
{
    void Mesh_append( SmartPtr<procedural::IProceduralMesh> self,
                      SmartPtr<procedural::IProceduralMesh> other, const Vector3F &offset )
    {
        // self.append( other, offset );
    }

    void Mesh_appendNoOffset( procedural::IProceduralMesh          &self,
                              SmartPtr<procedural::IProceduralMesh> other )
    {
        // self.append( other );
    }

    int Mesh_getVertexCount( SmartPtr<procedural::IProceduralMesh> m )
    {
        // return static_cast<int>( m.vertices.size() );
        return 0;
    }

    int Mesh_getIndexCount( SmartPtr<procedural::IProceduralMesh> m )
    {
        // return static_cast<int>( m.indices.size() );
        return 0;
    }

    int Mesh_getTriangleCount( SmartPtr<procedural::IProceduralMesh> m )
    {
        // return static_cast<int>( m.indices.size() / 3 );
        return 0;
    }

    Vector3F Mesh_getVertexPosition( SmartPtr<procedural::IProceduralMesh> m, int index )
    {
        // if( index < 0 || index >= static_cast<int>( m.vertices.size() ) )
        //     return Vector3F{ 0.0f, 0.0f, 0.0f };

        // return m.vertices[static_cast<size_t>( index )].position;
        return {};
    }

    Vector3F Mesh_getVertexNormal( SmartPtr<procedural::IProceduralMesh> m, int index )
    {
        // if( index < 0 || index >= static_cast<int>( m.vertices.size() ) )
        //     return Vector3F{ 0.0f, 1.0f, 0.0f };

        // return m.vertices[static_cast<size_t>( index )].normal;
        return {};
    }

    int Mesh_getIndex( SmartPtr<procedural::IProceduralMesh> m, int index )
    {
        // if( index < 0 || index >= static_cast<int>( m.indices.size() ) )
        //     return 0;

        // return static_cast<int>( m.indices[static_cast<size_t>( index )] );
        return {};
    }

    void Mesh_translate( SmartPtr<procedural::IProceduralMesh> m, const Vector3F &offset )
    {
        // for( auto &v : m.vertices )
        //     v.position = v.position + offset;
    }

    void Mesh_scale( SmartPtr<procedural::IProceduralMesh> m, const Vector3F &scale )
    {
        // for( auto &v : m.vertices )
        //{
        //     v.position.x *= scale.x;
        //     v.position.y *= scale.y;
        //     v.position.z *= scale.z;
        // }

        // m.computeNormals();
    }

    void Mesh_flipWinding( SmartPtr<procedural::IProceduralMesh> m )
    {
        // const size_t triCount = m.indices.size() / 3;
        // for( size_t i = 0; i < triCount; ++i )
        //     std::swap( m.indices[i * 3 + 1], m.indices[i * 3 + 2] );

        // m.computeNormals();
    }

    void Mesh_mirror( SmartPtr<procedural::IProceduralMesh> m, int axis )
    {
        // for( auto &v : m.vertices )
        //{
        //     if( axis == 0 )
        //         v.position.x = -v.position.x;
        //     else if( axis == 1 )
        //         v.position.y = -v.position.y;
        //     else
        //         v.position.z = -v.position.z;
        // }

        Mesh_flipWinding( m );
    }

    void Mesh_applyTaperY( SmartPtr<procedural::IProceduralMesh> m, float amount )
    {
        /* if( std::abs( amount ) < 1e-6f || m.vertices.empty() )
            return;

        float minY = m.vertices.front().position.y;
        float maxY = minY;
        for( const auto &v : m.vertices )
        {
            minY = std::min( minY, v.position.y );
            maxY = std::max( maxY, v.position.y );
        }

        const float height = std::max( maxY - minY, 1e-6f );
        for( auto &v : m.vertices )
        {
            const float t = ( v.position.y - minY ) / height;
            const float factor = std::max( 0.01f, 1.0f + amount * t );
            v.position.x *= factor;
            v.position.z *= factor;
        }

        m.computeNormals();*/
    }

    void Mesh_applyTwistY( SmartPtr<procedural::IProceduralMesh> m, float degrees )
    {
        /* if( std::abs( degrees ) < 1e-6f )
            return;

        constexpr float pi = 3.14159265358979323846f;
        const float radiansPerUnit = degrees * pi / 180.0f;

        for( auto &v : m.vertices )
        {
            const float angle = v.position.y * radiansPerUnit;
            const float c = std::cos( angle );
            const float s = std::sin( angle );
            const float x = v.position.x;
            const float z = v.position.z;
            v.position.x = x * c - z * s;
            v.position.z = x * s + z * c;
        }

        m.computeNormals();*/
    }

    void Mesh_applyBendX( SmartPtr<procedural::IProceduralMesh> m, float degrees )
    {
        /* if( std::abs( degrees ) < 1e-6f )
            return;

        constexpr float pi = 3.14159265358979323846f;
        const float radiansPerUnit = degrees * pi / 180.0f;

        for( auto &v : m.vertices )
        {
            const float angle = v.position.z * radiansPerUnit;
            const float c = std::cos( angle );
            const float s = std::sin( angle );
            const float y = v.position.y;
            const float z = v.position.z;
            v.position.y = y * c - z * s;
            v.position.z = y * s + z * c;
        }

        m.computeNormals();*/
    }

    void Mesh_applyNoise( SmartPtr<procedural::IProceduralMesh> m, float amount )
    {
        /* if( amount <= 1e-6f )
            return;

        for( auto &v : m.vertices )
        {
            const float raw =
                std::sin( v.position.x * 12.9898f + v.position.y * 78.233f + v.position.z * 37.719f ) *
                43758.5453f;
            const float n = raw - std::floor( raw );
            const float offset = ( n * 2.0f - 1.0f ) * amount;
            v.position = v.position + v.normal * offset;
        }

        m.computeNormals();*/
    }

    bool Mesh_exportObj( SmartPtr<procedural::IProceduralMesh> m, const String &filePath )
    {
        /* if( filePath.empty() )
            return false;

        std::ofstream file( filePath.c_str(), std::ios::out | std::ios::trunc );
        if( !file.is_open() )
            return false;

        file << "# Workphone procedural mesh\n";
        file << "# vertices " << m.vertices.size() << " indices " << m.indices.size() << "\n";

        for( const auto &v : m.vertices )
            file << "v " << v.position.x << " " << v.position.y << " " << v.position.z << "\n";

        for( const auto &v : m.vertices )
            file << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";

        const size_t triCount = m.indices.size() / 3;
        for( size_t i = 0; i < triCount; ++i )
        {
            const auto a = m.indices[i * 3 + 0] + 1;
            const auto b = m.indices[i * 3 + 1] + 1;
            const auto c = m.indices[i * 3 + 2] + 1;
            file << "f " << a << "//" << a << " " << b << "//" << b << " " << c << "//" << c << "\n";
        }

        return file.good();*/

        return true;
    }

    void bindProcedural( lua_State *L )
    {
        using namespace luabind;
        using namespace procedural;

        module( L )[class_<IProceduralGenerator, ISharedObject, SmartPtr<IProceduralGenerator>>(
                        "IProceduralGenerator" )
                        .def( "getFilePath", &IProceduralGenerator::getFilePath )
                        .def( "setFilePath", &IProceduralGenerator::setFilePath )
                        .def( "generate", &IProceduralGenerator::generate )
                        .def( "isFinished", &IProceduralGenerator::isFinished )
                        .def( "getInput", &IProceduralGenerator::getInput )
                        .def( "setInput", &IProceduralGenerator::setInput )
                        .def( "getOutput", &IProceduralGenerator::getOutput )
                        .def( "setOutput", &IProceduralGenerator::setOutput )
                        .def( "getParent", &IProceduralGenerator::getParent )
                        .def( "setParent", &IProceduralGenerator::setParent )];

        module( L )[class_<ICityGenerator, IProceduralGenerator>( "ICityGenerator" )
                        .def( "generate", &ICityGenerator::generate )
                        .def( "isFinished", &ICityGenerator::isFinished )
                        .def( "getTerrainGenerator", &ICityGenerator::getTerrainGenerator )
                        .def( "setTerrainGenerator", &ICityGenerator::setTerrainGenerator )
                        .def( "getBlockGenerator", &ICityGenerator::getBlockGenerator )
                        .def( "setBlockGenerator", &ICityGenerator::setBlockGenerator )
                        .def( "getProceduralWorld", &ICityGenerator::getProceduralWorld )
                        .def( "setProceduralWorld", &ICityGenerator::setProceduralWorld )
                        .def( "getMeshGenerator", &ICityGenerator::getMeshGenerator )
                        .def( "setMeshGenerator", &ICityGenerator::setMeshGenerator )
                        .def( "getRoadGenerator", &ICityGenerator::getRoadGenerator )
                        .def( "setRoadGenerator", &ICityGenerator::setRoadGenerator )
                        .def( "getCities", &ICityGenerator::getCities )
                        .def( "removeCity", &ICityGenerator::removeCity )
                        .def( "addCity", &ICityGenerator::addCity )
                        .def( "getFilePath", &ICityGenerator::getFilePath )
                        .def( "setFilePath", &ICityGenerator::setFilePath )];

        module( L )[class_<IProceduralCollision, ISharedObject, no_bases>( "IProceduralCollision" )
                        .def( "rayTest", static_cast<bool ( IProceduralCollision::* )(
                                             const Vector3<real_Num> &, const Vector3<real_Num> &,
                                             Vector3<real_Num> & )>( &IProceduralCollision::rayTest ) )
                        .def( "rayTest", static_cast<bool ( IProceduralCollision::* )(
                                             const Vector3<real_Num> &, const Vector3<real_Num> &,
                                             Vector3<real_Num> &, Triangle3<real_Num> & )>(
                                             &IProceduralCollision::rayTest ) )
                        .def( "rayTest", static_cast<bool ( IProceduralCollision::* )(
                                             const Vector3<real_Num> &, const Vector3<real_Num> &,
                                             Vector3<real_Num> &, Triangle3<real_Num> &,
                                             const Array<String> & )>( &IProceduralCollision::rayTest ) )
                        .def( "intersects",
                              static_cast<bool ( IProceduralCollision::* )(
                                  const SmartPtr<IProceduralCity> &, const Sphere3<real_Num> & )>(
                                  &IProceduralCollision::intersects ) )
                        .def( "intersects", static_cast<bool ( IProceduralCollision::* )(
                                                const SmartPtr<IRoad> &, const Sphere3<real_Num> & )>(
                                                &IProceduralCollision::intersects ) )

        ];

        // ── Mesh ─────────────────────────────────────────────────────────────
        module( L )[class_<IProceduralMesh>( "IProceduralMesh" )
                        .def( "clear", &IProceduralMesh::clear )
                        .def( "addVertex", &IProceduralMesh::addVertex )
                        .def( "addTriangle", &IProceduralMesh::addTriangle )
                        .def( "append", &Mesh_append )
                        .def( "appendMesh", &Mesh_appendNoOffset )
                        .def( "getVertexCount", &Mesh_getVertexCount )
                        .def( "getIndexCount", &Mesh_getIndexCount )
                        .def( "getTriangleCount", &Mesh_getTriangleCount )
                        .def( "getVertexPosition", &Mesh_getVertexPosition )
                        .def( "getVertexNormal", &Mesh_getVertexNormal )
                        .def( "getIndex", &Mesh_getIndex )
                        .def( "translate", &Mesh_translate )
                        .def( "scale", &Mesh_scale )
                        .def( "mirror", &Mesh_mirror )
                        .def( "flipWinding", &Mesh_flipWinding )
                        .def( "applyTaper", &Mesh_applyTaperY )
                        .def( "applyTwist", &Mesh_applyTwistY )
                        .def( "applyBend", &Mesh_applyBendX )
                        .def( "applyNoise", &Mesh_applyNoise )
                        .def( "exportObj", &Mesh_exportObj )
                        .def( "computeNormals", &IProceduralMesh::computeNormals )];

        /*
        // ── ModelRule (abstract base) ─────────────────────────────────────────
        module( L )[class_<ModelRule, std::shared_ptr<ModelRule>>( "ModelRule" )
                        .def( "build", &ModelRule::build )];

        // ── BoxRule ───────────────────────────────────────────────────────────
        module( L )[class_<BoxRule, ModelRule, std::shared_ptr<BoxRule>>( "BoxRule" )
                        .def( constructor<>() )
                        .def( constructor<const Vector3F &>() )
                        .def( constructor<const Vector3F &, const Vector3F &>() )
                        .def( "build", &BoxRule::build )
                        .def_readwrite( "size", &BoxRule::size )
                        .def_readwrite( "offset", &BoxRule::offset )];

        // ── CylinderRule ──────────────────────────────────────────────────────
        module( L )[class_<CylinderRule, ModelRule, std::shared_ptr<CylinderRule>>( "CylinderRule" )
                        .def( constructor<>() )
                        .def( constructor<float, float, int>() )
                        .def( constructor<float, float, int, const Vector3F &>() )
                        .def( "build", &CylinderRule::build )
                        .def_readwrite( "radius", &CylinderRule::radius )
                        .def_readwrite( "height", &CylinderRule::height )
                        .def_readwrite( "segments", &CylinderRule::segments )
                        .def_readwrite( "offset", &CylinderRule::offset )];

        // ── ArrayRule ─────────────────────────────────────────────────────────
        module( L )[class_<ArrayRule, ModelRule, std::shared_ptr<ArrayRule>>( "ArrayRule" )
                        .def( constructor<std::shared_ptr<ModelRule>, int, const Vector3F &>() )
                        .def( "build", &ArrayRule::build )
                        .def_readwrite( "count", &ArrayRule::count )
                        .def_readwrite( "step", &ArrayRule::step )];

        // ── CompositeRule ─────────────────────────────────────────────────────
        module( L )[class_<CompositeRule, ModelRule, std::shared_ptr<CompositeRule>>( "CompositeRule" )
                        .def( constructor<>() )
                        .def( "add", &CompositeRule::add )
                        .def( "build", &CompositeRule::build )];

        // ── ProceduralModelComponent ──────────────────────────────────────────
        module( L )[class_<ProceduralModelComponent>( "ProceduralModelComponent" )
                        .def( constructor<>() )
                        .def_readwrite( "rootRule", &ProceduralModelComponent::rootRule )
                        .def_readwrite( "bakedMesh", &ProceduralModelComponent::bakedMesh )
                        .def_readwrite( "dirty", &ProceduralModelComponent::dirty )];

        // ── ProceduralModelSystem ─────────────────────────────────────────────
        module( L )[class_<ProceduralModelSystem>( "ProceduralModelSystem" )
                        .def( constructor<>() )
                        .def( "bake", &ProceduralModelSystem::bake )];
        */

        module( L )[class_<IProceduralBuilding, ISharedObject, SmartPtr<IProceduralBuilding>>(
            "IProceduralBuilding" )];

        module( L )[class_<IProceduralObject, ISharedObject, SmartPtr<IProceduralObject>>(
                        "IProceduralObject" )
                        .def( "build", &IProceduralObject::build )
                        .def( "updateBounds", &IProceduralObject::updateBounds )
                        .def( "getWorldTransform", &IProceduralObject::getWorldTransform )
                        .def( "setWorldTransform", &IProceduralObject::setWorldTransform )
                        .def( "getPosition", &IProceduralObject::getPosition )
                        .def( "setPosition", &IProceduralObject::setPosition )
                        .def( "getScale", &IProceduralObject::getScale )
                        .def( "setScale", &IProceduralObject::setScale )
                        .def( "getOrientation", &IProceduralObject::getOrientation )
                        .def( "setOrientation", &IProceduralObject::setOrientation )
                        .def( "getScene", &IProceduralObject::getScene )
                        .def( "setScene", &IProceduralObject::setScene )
                        .def( "clone", &IProceduralObject::clone )
                        .def( "addNode", &IProceduralObject::addNode )
                        .def( "removeNode", &IProceduralObject::removeNode )
                        .def( "getNodes", &IProceduralObject::getNodes )];

        module( L )[class_<IProceduralNode, IProceduralObject, SmartPtr<IProceduralNode>>(
                        "IProceduralNode" )
                        .def( "connect", &IProceduralNode::connect )
                        .def( "disconnect", &IProceduralNode::disconnect )
                        .def( "disconnectAll", &IProceduralNode::disconnectAll )
                        .def( "getConnectedNodes", &IProceduralNode::getConnectedNodes )
                        .def( "getNumConnections", &IProceduralNode::getNumConnections )
                        .def( "clearConnections", &IProceduralNode::clearConnections )
                        .def( "hasMergedNode", &IProceduralNode::hasMergedNode )
                        .def( "addMergedNode", &IProceduralNode::addMergedNode )
                        .def( "removeMergedNode", &IProceduralNode::removeMergedNode )
                        .def( "getMergedNodes", &IProceduralNode::getMergedNodes )
                        .def( "setMergedNodes", &IProceduralNode::setMergedNodes )
                        .def( "getGraphId", &IProceduralNode::getGraphId )
                        .def( "setGraphId", &IProceduralNode::setGraphId )
                        .def( "getRoadId", &IProceduralNode::getRoadId )
                        .def( "setRoadId", &IProceduralNode::setRoadId )
                        .def( "getMergedNode", &IProceduralNode::getMergedNode )
                        .def( "setMergedNode", &IProceduralNode::setMergedNode )
                        .def( "setIsConnection", &IProceduralNode::setIsConnection )
                        .def( "isConnection", &IProceduralNode::isConnection )
                        .def( "getConnectionType", &IProceduralNode::getConnectionType )
                        .def( "getNetworkNode", &IProceduralNode::getNetworkNode )
                        .def( "setNetworkNode", &IProceduralNode::setNetworkNode )];

        module(
            L )[class_<IProceduralCity, ISharedObject, SmartPtr<IProceduralCity>>( "IProceduralCity" )
                    .def( "getRoadNetwork", &IProceduralCity::getRoadNetwork )
                    .def( "setRoadNetwork", &IProceduralCity::setRoadNetwork )
                    .def( "getCityCenters", &IProceduralCity::getCityCenters )
                    .def( "setCityCenters", &IProceduralCity::setCityCenters )
                    .def( "addCenter", &IProceduralCity::addCenter )
                    .def( "removeCenter", &IProceduralCity::removeCenter )
                    .def( "addBlock", &IProceduralCity::addBlock )
                    .def( "removeBlock", &IProceduralCity::removeBlock )
                    .def( "getBlocks", &IProceduralCity::getBlocks )
                    .def( "getSize", &IProceduralCity::getSize )
                    .def( "setSize", &IProceduralCity::setSize )
                    .def( "getRelativeCoordinates", &IProceduralCity::getRelativeCoordinates )
                    .def( "getRoadByName", &IProceduralCity::getRoadByName )
                    .def( "getRoads", &IProceduralCity::getRoads )
                    .def( "getArea", &IProceduralCity::getArea )
                    .def( "setArea", &IProceduralCity::setArea )
                    .def( "isWithin", &IProceduralCity::isWithin )
                    .def( "getMinLatLong", &IProceduralCity::getMinLatLong )
                    .def( "setMinLatLong", &IProceduralCity::setMinLatLong )
                    .def( "getMaxLatLong", &IProceduralCity::getMaxLatLong )
                    .def( "setMaxLatLong", &IProceduralCity::setMaxLatLong )];

        module( L )[class_<IProceduralCityCenter, IProceduralObject, SmartPtr<IProceduralCityCenter>>(
                        "IProceduralCityCenter" )
                        .def( "getTransform", &IProceduralCityCenter::getTransform )
                        .def( "setTransform", &IProceduralCityCenter::setTransform )
                        .def( "getName", &IProceduralCityCenter::getName )
                        .def( "setName", &IProceduralCityCenter::setName )
                        .def( "getRadius", &IProceduralCityCenter::getRadius )
                        .def( "setRadius", &IProceduralCityCenter::setRadius )
                        .def( "addRoad", &IProceduralCityCenter::addRoad )
                        .def( "removeRoad", &IProceduralCityCenter::removeRoad )
                        .def( "getRoadNodesFromCell", &IProceduralCityCenter::getRoadNodesFromCell )
                        .def( "getProperties", &IProceduralCityCenter::getProperties )
                        .def( "setProperties", &IProceduralCityCenter::setProperties )];

        module( L )[class_<IProceduralManager, ISharedObject, SmartPtr<IProceduralManager>>(
                        "IProceduralManager" )
                        .def( "generate", &IProceduralManager::generate )
                        .def( "getCityGenerator", &IProceduralManager::getCityGenerator )
                        .def( "setCityGenerator", &IProceduralManager::setCityGenerator )
                        .def( "getTerrainGenerator", &IProceduralManager::getTerrainGenerator )
                        .def( "setTerrainGenerator", &IProceduralManager::setTerrainGenerator )
                        .def( "getCollisionManager", &IProceduralManager::getCollisionManager )
                        .def( "setCollisionManager", &IProceduralManager::setCollisionManager )];

        module(
            L )[class_<IProceduralScene, ISharedObject, SmartPtr<IProceduralScene>>( "IProceduralScene" )
                    .def( "addCity", &IProceduralScene::addCity )
                    .def( "removeCity", &IProceduralScene::removeCity )
                    .def( "getCities", &IProceduralScene::getCities )
                    .def( "setCities", &IProceduralScene::setCities )
                    .def( "addTerrain", &IProceduralScene::addTerrain )
                    .def( "removeTerrain", &IProceduralScene::removeTerrain )
                    .def( "getTerrains", &IProceduralScene::getTerrains )
                    .def( "setTerrains", &IProceduralScene::setTerrains )];

        module( L )[class_<IProceduralTerrain, IProceduralObject, SmartPtr<IProceduralTerrain>>(
                        "IProceduralTerrain" )
                        .def( "getSize", &IProceduralTerrain::getSize )
                        .def( "setSize", &IProceduralTerrain::setSize )
                        .def( "getHeightmapResolution", &IProceduralTerrain::getHeightmapResolution )
                        .def( "setHeightmapResolution", &IProceduralTerrain::setHeightmapResolution )
                        .def( "getAlphamapResolution", &IProceduralTerrain::getAlphamapResolution )
                        .def( "setAlphamapResolution", &IProceduralTerrain::setAlphamapResolution )
                        .def( "getDetailResolution", &IProceduralTerrain::getDetailResolution )
                        .def( "setDetailResolution", &IProceduralTerrain::setDetailResolution )
                        .def( "getDetailResolutionPerPatch",
                              &IProceduralTerrain::getDetailResolutionPerPatch )
                        .def( "setDetailResolutionPerPatch",
                              &IProceduralTerrain::setDetailResolutionPerPatch )
                        .def( "getHeightData", &IProceduralTerrain::getHeightData )
                        .def( "setHeightData", &IProceduralTerrain::setHeightData )];

        module( L )[class_<IProceduralTexture, ISharedObject, SmartPtr<IProceduralTexture>>(
                        "IProceduralTexture" )
                        .def( "generate", &IProceduralTexture::generate )
                        .def( "setSize", &IProceduralTexture::setSize )
                        .def( "getWidth", &IProceduralTexture::getWidth )
                        .def( "getHeight", &IProceduralTexture::getHeight )
                        .def( "setData", &IProceduralTexture::setData )
                        .def( "getData", &IProceduralTexture::getData )];
        module(
            L )[class_<IProceduralWorld, ISharedObject, SmartPtr<IProceduralWorld>>( "IProceduralWorld" )
                    .def( "addScene", &IProceduralWorld::addScene )
                    .def( "removeScene", &IProceduralWorld::removeScene )
                    .def( "getScenes", &IProceduralWorld::getScenes )];

        module( L )[class_<procedural::IRoad, IProceduralObject, SmartPtr<procedural::IRoad>>( "IRoad" )
                        .def( "intersects", &procedural::IRoad::intersects )
                        .def( "getRoadNodes", &procedural::IRoad::getRoadNodes )
                        .def( "getNode", &procedural::IRoad::getNode )
                        .def( "getFirstNode", &procedural::IRoad::getFirstNode )
                        .def( "getLastNode", &procedural::IRoad::getLastNode )
                        .def( "getRoadType", &procedural::IRoad::getRoadType )
                        .def( "setRoadType", &procedural::IRoad::setRoadType )
                        .def( "getMarkerFromTransform", &procedural::IRoad::getMarkerFromTransform )
                        .def( "addRoadSection", &procedural::IRoad::addRoadSection )
                        .def( "removeRoadSection", &procedural::IRoad::removeRoadSection )
                        .def( "getRoadSections", &procedural::IRoad::getRoadSections )
                        .def( "setRoadSections", &procedural::IRoad::setRoadSections )];

        module( L )[class_<procedural::IRoadConnection, IProceduralObject,
                           SmartPtr<procedural::IRoadConnection>>( "IRoadConnection" )
                        .def( "getResourceName", &procedural::IRoadConnection::getResourceName )
                        .def( "setResourceName", &procedural::IRoadConnection::setResourceName )
                        .def( "getConnectionType", &procedural::IRoadConnection::getConnectionType )
                        .def( "setConnectionType", &procedural::IRoadConnection::setConnectionType )
                        .def( "addConnection", &procedural::IRoadConnection::addConnection )
                        .def( "removeConnection", &procedural::IRoadConnection::removeConnection )
                        .def( "getConnectionData", &procedural::IRoadConnection::getConnectionData )
                        .def( "setConnectionData", &procedural::IRoadConnection::setConnectionData )
                        .def( "getNode", &procedural::IRoadConnection::getNode )
                        .def( "getRoadNodes", &procedural::IRoadConnection::getRoadNodes )
                        .def( "setRoadNodes", &procedural::IRoadConnection::setRoadNodes )];

        module( L )[class_<procedural::IRoadConnectionData, SmartPtr<procedural::IRoadConnectionData>>(
                        "IRoadConnectionData" )
                        .def( "setRoad", &procedural::IRoadConnectionData::setRoad )
                        .def( "getRoad", &procedural::IRoadConnectionData::getRoad )
                        .def( "setMarker", &procedural::IRoadConnectionData::setMarker )
                        .def( "getMarker", &procedural::IRoadConnectionData::getMarker )
                        .def( "setConnection", &procedural::IRoadConnectionData::setConnection )
                        .def( "getConnection", &procedural::IRoadConnectionData::getConnection )
                        .def( "setTransform", &procedural::IRoadConnectionData::setTransform )
                        .def( "getTransform", &procedural::IRoadConnectionData::getTransform )];

        module(
            L )[class_<procedural::IRoadElement, IProceduralObject, SmartPtr<procedural::IRoadElement>>(
                    "IRoadElement" )
                    .def( "getParentSection", &procedural::IRoadElement::getParentSection )
                    .def( "setParentSection", &procedural::IRoadElement::setParentSection )
                    .def( "getRoadNodes", &procedural::IRoadElement::getRoadNodes )
                    .def( "getSidewalks", &procedural::IRoadElement::getSidewalks )
                    .def( "setSidewalks", &procedural::IRoadElement::setSidewalks )
                    .def( "isLit", &procedural::IRoadElement::isLit )
                    .def( "setIsLit", &procedural::IRoadElement::setIsLit )];

        module( L )[class_<procedural::IRoadGenerator, IProceduralGenerator,
                           SmartPtr<procedural::IRoadGenerator>>( "IRoadGenerator" )
                        .def( "getPatternData", &procedural::IRoadGenerator::getPatternData )
                        .def( "setPatternData", &procedural::IRoadGenerator::setPatternData )
                        .def( "getCity", &procedural::IRoadGenerator::getCity )
                        .def( "setCity", &procedural::IRoadGenerator::setCity )];

        module(
            L )[class_<procedural::IRoadHitPoint, SmartPtr<procedural::IRoadHitPoint>>( "IRoadHitPoint" )
                    .def( "getRoad", &procedural::IRoadHitPoint::getRoad )
                    .def( "setRoad", &procedural::IRoadHitPoint::setRoad )
                    .def( "getOtherRoad", &procedural::IRoadHitPoint::getOtherRoad )
                    .def( "setOtherRoad", &procedural::IRoadHitPoint::setOtherRoad )
                    .def( "getPosition", &procedural::IRoadHitPoint::getPosition )
                    .def( "setPosition", &procedural::IRoadHitPoint::setPosition )
                    .def( "getNormal", &procedural::IRoadHitPoint::getNormal )
                    .def( "setNormal", &procedural::IRoadHitPoint::setNormal )
                    .def( "getDistance", &procedural::IRoadHitPoint::getDistance )
                    .def( "setDistance", &procedural::IRoadHitPoint::setDistance )];

        module( L )[class_<procedural::IRoadMeshElement, SmartPtr<procedural::IRoadMeshElement>>(
                        "IRoadMeshElement" )
                        .def( "getParentSection", &procedural::IRoadMeshElement::getParentSection )
                        .def( "setParentSection", &procedural::IRoadMeshElement::setParentSection )
                        .def( "getVertices", &procedural::IRoadMeshElement::getVertices )
                        .def( "setVertices", &procedural::IRoadMeshElement::setVertices )
                        .def( "getIndices", &procedural::IRoadMeshElement::getIndices )
                        .def( "setIndices", &procedural::IRoadMeshElement::setIndices )
                        .def( "getPolygons", &procedural::IRoadMeshElement::getPolygons )
                        .def( "setPolygons", &procedural::IRoadMeshElement::setPolygons )
                        .def( "build", &procedural::IRoadMeshElement::build )
                        .def( "updateMesh", &procedural::IRoadMeshElement::updateMesh )];

        module(
            L )[class_<procedural::IRoadNetwork, SmartPtr<procedural::IRoadNetwork>>( "IRoadNetwork" )
                    .def( "getNodeByGraphId", &procedural::IRoadNetwork::getNodeByGraphId )
                    .def( "getNodes", &procedural::IRoadNetwork::getNodes )
                    .def( "setNodes", &procedural::IRoadNetwork::setNodes )
                    .def( "getMergedNodes", &procedural::IRoadNetwork::getMergedNodes )
                    .def( "setMergedNodes", &procedural::IRoadNetwork::setMergedNodes )
                    .def( "removeRoad", &procedural::IRoadNetwork::removeRoad )
                    .def( "addRoad", &procedural::IRoadNetwork::addRoad )
                    .def( "addRoads", &procedural::IRoadNetwork::addRoads )
                    .def( "removeRoads", &procedural::IRoadNetwork::removeRoads )
                    .def( "getRoadConnections", &procedural::IRoadNetwork::getRoadConnections )
                    .def( "setRoadConnections", &procedural::IRoadNetwork::setRoadConnections )
                    .def( "removeRoadConnection", &procedural::IRoadNetwork::removeRoadConnection )
                    .def( "addRoadConnection", &procedural::IRoadNetwork::addRoadConnection )
                    .def( "setupGraph", &procedural::IRoadNetwork::setupGraph )];

        module( L )[class_<procedural::IRoadNode, IProceduralNode, SmartPtr<procedural::IRoadNode>>(
                        "IRoadNode" )
                        .def( "getRoadNodeFromMerged", &procedural::IRoadNode::getRoadNodeFromMerged )
                        .def( "addRoad", &procedural::IRoadNode::addRoad )
                        .def( "removeRoad", &procedural::IRoadNode::removeRoad )
                        .def( "getRoads", &procedural::IRoadNode::getRoads )];

        module(
            L )[class_<procedural::IRoadSection, IProceduralObject, SmartPtr<procedural::IRoadSection>>(
                    "IRoadSection" )
                    .def( "addRoadElement", &procedural::IRoadSection::addRoadElement )
                    .def( "removeRoadElement", &procedural::IRoadSection::removeRoadElement )
                    .def( "clearRoadElements", &procedural::IRoadSection::clearRoadElements )
                    .def( "getElements", &procedural::IRoadSection::getElements )
                    .def( "setElements", &procedural::IRoadSection::setElements )];

        module( L )[class_<IBlockGenerator, IProceduralGenerator, SmartPtr<IBlockGenerator>>(
                        "IBlockGenerator" )
                        .def( "generate", &IBlockGenerator::generate )
                        .def( "getCity", &IBlockGenerator::getCity )
                        .def( "setCity", &IBlockGenerator::setCity )];

        module( L )[class_<ICityBlock, IProceduralObject, SmartPtr<ICityBlock>>( "ICityBlock" )
                        .def( "addPoint", &ICityBlock::addPoint )
                        .def( "removePoint", &ICityBlock::removePoint )
                        .def( "getPoints", &ICityBlock::getPoints )
                        .def( "getPolygon", &ICityBlock::getPolygon )
                        .def( "equals", &ICityBlock::equals )
                        .def( "getLots", &ICityBlock::getLots )
                        .def( "setLots", &ICityBlock::setLots )
                        .def( "clone", &ICityBlock::clone )];

        module( L )[class_<ICityMap, ISharedObject, SmartPtr<ICityMap>>( "ICityMap" )
                        .def( "getCityMapValue", &ICityMap::getCityMapValue )
                        .def( "hasCityMapEntry", &ICityMap::hasCityMapEntry )
                        .def( "getMapAABB", &ICityMap::getMapAABB )];

        module( L )[class_<ILot, IProceduralObject, SmartPtr<ILot>>( "ILot" )
                        .def( "getBlock", &ILot::getBlock )
                        .def( "setBlock", &ILot::setBlock )
                        .def( "getNodes", &ILot::getNodes )
                        .def( "setNodes", &ILot::setNodes )
                        .def( "getPolygon", &ILot::getPolygon )
                        .def( "getArea", &ILot::getArea )
                        .def( "getBounds", &ILot::getBounds )];

        module( L )[class_<IMeshGenerator, IProceduralGenerator, SmartPtr<IMeshGenerator>>(
                        "IMeshGenerator" )
                        .def( "generate", &IMeshGenerator::generate )
                        .def( "generateFromTerrain", &IMeshGenerator::generateFromTerrain )
                        .def( "getMeshQuality", &IMeshGenerator::getMeshQuality )
                        .def( "setMeshQuality", &IMeshGenerator::setMeshQuality )
                        .def( "getNumLodLevels", &IMeshGenerator::getNumLodLevels )
                        .def( "setNumLodLevels", &IMeshGenerator::setNumLodLevels )
                        .def( "getLodDistance", &IMeshGenerator::getLodDistance )
                        .def( "setLodDistance", &IMeshGenerator::setLodDistance )
                        .def( "getMaxVertexCount", &IMeshGenerator::getMaxVertexCount )
                        .def( "setMaxVertexCount", &IMeshGenerator::setMaxVertexCount )
                        .def( "getMaxTriangleCount", &IMeshGenerator::getMaxTriangleCount )
                        .def( "setMaxTriangleCount", &IMeshGenerator::setMaxTriangleCount )
                        .def( "getSimplificationThreshold", &IMeshGenerator::getSimplificationThreshold )
                        .def( "setSimplificationThreshold", &IMeshGenerator::setSimplificationThreshold )
                        .def( "getGenerateUVs", &IMeshGenerator::getGenerateUVs )
                        .def( "setGenerateUVs", &IMeshGenerator::setGenerateUVs )
                        .def( "getUVScale", &IMeshGenerator::getUVScale )
                        .def( "setUVScale", &IMeshGenerator::setUVScale )
                        .def( "getGenerateNormals", &IMeshGenerator::getGenerateNormals )
                        .def( "setGenerateNormals", &IMeshGenerator::setGenerateNormals )
                        .def( "getGenerateTangents", &IMeshGenerator::getGenerateTangents )
                        .def( "setGenerateTangents", &IMeshGenerator::setGenerateTangents )
                        .def( "getOptimizeMesh", &IMeshGenerator::getOptimizeMesh )
                        .def( "setOptimizeMesh", &IMeshGenerator::setOptimizeMesh )
                        .def( "getMergeDuplicateVertices", &IMeshGenerator::getMergeDuplicateVertices )
                        .def( "setMergeDuplicateVertices", &IMeshGenerator::setMergeDuplicateVertices )
                        .def( "getVertexMergeTolerance", &IMeshGenerator::getVertexMergeTolerance )
                        .def( "setVertexMergeTolerance", &IMeshGenerator::setVertexMergeTolerance )
                        .def( "getGenerateCollision", &IMeshGenerator::getGenerateCollision )
                        .def( "setGenerateCollision", &IMeshGenerator::setGenerateCollision )
                        .def( "getCollisionSimplification", &IMeshGenerator::getCollisionSimplification )
                        .def( "setCollisionSimplification", &IMeshGenerator::setCollisionSimplification )
                        .def( "validateParameters", &IMeshGenerator::validateParameters )
                        .def( "isReady", &IMeshGenerator::isReady )
                        .def( "clear", &IMeshGenerator::clear )
                        .def( "getLastGeneratedMesh", &IMeshGenerator::getLastGeneratedMesh )
                        .def( "getAllGeneratedMeshes", &IMeshGenerator::getAllGeneratedMeshes )
                        .def( "getGenerationProgress", &IMeshGenerator::getGenerationProgress )
                        .def( "getGenerationStatus", &IMeshGenerator::getGenerationStatus )];

        /*
        module( L )[class_<ITerrainGenerator, IProceduralGenerator, SmartPtr<ITerrainGenerator>>(
                        "ITerrainGenerator" )
                        .def( "getProceduralScene", &ITerrainGenerator::getProceduralScene )
                        .def( "setProceduralScene", &ITerrainGenerator::setProceduralScene )
                        .def( "getWidth", &ITerrainGenerator::getWidth )
                        .def( "setWidth", &ITerrainGenerator::setWidth )
                        .def( "getHeight", &ITerrainGenerator::getHeight )
                        .def( "setHeight", &ITerrainGenerator::setHeight )
                        .def( "getDepth", &ITerrainGenerator::getDepth )
                        .def( "setDepth", &ITerrainGenerator::setDepth )
                        .def( "getOctaves", &ITerrainGenerator::getOctaves )
                        .def( "setOctaves", &ITerrainGenerator::setOctaves )
                        .def( "getScale", &ITerrainGenerator::getScale )
                        .def( "setScale", &ITerrainGenerator::setScale )
                        .def( "getLacunarity", &ITerrainGenerator::getLacunarity )
                        .def( "setLacunarity", &ITerrainGenerator::setLacunarity )
                        .def( "getPersistence", &ITerrainGenerator::getPersistence )
                        .def( "setPersistence", &ITerrainGenerator::setPersistence )
                        .def( "getOffset", &ITerrainGenerator::getOffset )
                        .def( "setOffset", &ITerrainGenerator::setOffset )
                        .def( "getFalloffDirection", &ITerrainGenerator::getFalloffDirection )
                        .def( "setFalloffDirection", &ITerrainGenerator::setFalloffDirection )
                        .def( "getFalloffRange", &ITerrainGenerator::getFalloffRange )
                        .def( "setFalloffRange", &ITerrainGenerator::setFalloffRange )
                        .def( "getUseFalloffMap", &ITerrainGenerator::getUseFalloffMap )
                        .def( "setUseFalloffMap", &ITerrainGenerator::setUseFalloffMap )
                        .def( "getRandomize", &ITerrainGenerator::getRandomize )
                        .def( "setRandomize", &ITerrainGenerator::setRandomize )
                        .def( "getAutoUpdate", &ITerrainGenerator::getAutoUpdate )
                        .def( "setAutoUpdate", &ITerrainGenerator::setAutoUpdate )
                        .def( "validate", &ITerrainGenerator::validate )
                        .def( "clear", &ITerrainGenerator::clear )
                        .def( "getTerrain", &ITerrainGenerator::getTerrain )
                        .def( "setTerrain", &ITerrainGenerator::setTerrain )];*/

        module( L )[class_<procedural::ISidewalk, SmartPtr<procedural::ISidewalk>>( "ISidewalk" )];

        module( L )[class_<IWorldGenerator, IProceduralGenerator, SmartPtr<IWorldGenerator>>(
                        "IWorldGenerator" )
                        .def( "getProceduralWorld", &IWorldGenerator::getProceduralWorld )
                        .def( "setProceduralWorld", &IWorldGenerator::setProceduralWorld )
                        .def( "getScenes", &IWorldGenerator::getScenes )
                        .def( "addScene", &IWorldGenerator::addScene )
                        .def( "removeScene", &IWorldGenerator::removeScene )
                        .def( "setScenes", &IWorldGenerator::setScenes )];

        module( L )[class_<ILSystem, ISharedObject, SmartPtr<ILSystem>>( "ILSystem" )
                        .def( "reset", &ILSystem::reset )
                        .def( "addVariable", &ILSystem::addVariable )
                        .def( "removeVariable", &ILSystem::removeVariable )
                        .def( "printVariables", &ILSystem::printVariables )
                        .def( "addConstant", &ILSystem::addConstant )
                        .def( "removeConstant", &ILSystem::removeConstant )
                        .def( "printConstants", &ILSystem::printConstants )
                        .def( "addRule", &ILSystem::addRule )
                        .def( "removeRule", &ILSystem::removeRule )
                        .def( "printRules", &ILSystem::printRules )
                        .def( "setStart", &ILSystem::setStart )
                        .def( "printStart", &ILSystem::printStart )
                        .def( "getNextLevel", &ILSystem::getNextLevel )
                        .def( "getLevel", &ILSystem::getLevel )];

        module( L )[class_<ILSystemRule, ISharedObject, SmartPtr<ILSystemRule>>( "ILSystemRule" )
                        .def( "getPredecessor", &ILSystemRule::getPredecessor )
                        .def( "setPredecessor", &ILSystemRule::setPredecessor )
                        .def( "getSuccessor", &ILSystemRule::getSuccessor )
                        .def( "setSuccessor", &ILSystemRule::setSuccessor )
                        .def( "print", &ILSystemRule::print )];
    }

} // namespace workphone
