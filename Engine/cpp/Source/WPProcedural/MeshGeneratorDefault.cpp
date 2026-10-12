#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <Workphone/Workphone.hpp>
#include <WPProcedural/RoadGeneratorUtil.hpp>
#include <WPProcedural/ProceduralModel.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <cmath>

namespace workphone
{
    namespace procedural
    {
        bool MeshGeneratorDefault::beginGeneration()
        {
            validateParameters();
            m_progress = 0;
            m_status = "Generating";
            const auto app = core::IApplicationManager::instancePtr();
            if( !app || !app->getMeshManager() )
            {
                failGeneration( "Mesh manager is unavailable" );
                return false;
            }
            if( m_numLodLevels != 1 || m_simplificationThreshold != 0 || m_generateTangents ||
                m_optimizeMesh || m_mergeDuplicateVertices || m_generateCollision )
            {
                failGeneration( "Unsupported options: LOD, simplification, tangents, optimization, vertex merging or collision" );
                return false;
            }
            return true;
        }

        SmartPtr<IMesh> MeshGeneratorDefault::failGeneration( const String &message )
        {
            m_status = "Failed: " + message;
            m_progress = 0;
            return nullptr;
        }

        SmartPtr<IMesh> MeshGeneratorDefault::finishGeneration( SmartPtr<IMesh> mesh )
        {
            if( !mesh || mesh->getNumSubMeshes() == 0 )
                return failGeneration( "No geometry" );
            mesh->updateAABB( true );
            m_generatedMeshes.push_back( mesh );
            m_progress = 1;
            m_status = "Complete";
            return mesh;
        }

        SmartPtr<IMesh> MeshGeneratorDefault::generate( SmartPtr<IRoadNetwork> roadNetwork )
        try
        {
            if( !beginGeneration() ) return nullptr;
            if( !roadNetwork ) return failGeneration( "Null road network" );
            Array<SmartPtr<IProceduralObject>> objects;
            for( const auto &road : roadNetwork->getRoads() ) objects.push_back( road );
            return generateFromObjects( objects );
        } catch( const std::exception &e )
        {
            return failGeneration( String( "Generation exception: " ) + e.what() );
        }
        catch( ... )
        {
            return failGeneration( "Unknown generation exception" );
        }

        SmartPtr<IMesh> MeshGeneratorDefault::generateFromTerrain( SmartPtr<IProceduralTerrain> terrain )
        try
        {
            if( !beginGeneration() ) return nullptr;
            if( !terrain ) return failGeneration( "Null terrain" );
            const auto resolution = terrain->getHeightmapResolution();
            const auto size = terrain->getSize();
            const auto heights = terrain->getHeightData();
            if( resolution.x < 2 || resolution.y < 2 || !std::isfinite( size.x ) ||
                !std::isfinite( size.y ) || !std::isfinite( size.z ) || size.x <= 0 || size.z <= 0 )
                return failGeneration( "Invalid terrain dimensions" );
            const auto vertexCount = uint64_t( resolution.x ) * uint64_t( resolution.y );
            const auto triangleCount = uint64_t( resolution.x - 1 ) * uint64_t( resolution.y - 1 ) * 2;
            if( vertexCount != heights.size() || vertexCount > m_maxVertexCount || triangleCount > m_maxTriangleCount )
                return failGeneration( "Terrain sample count or geometry budget exceeded" );
            ProceduralMesh cpu;
            Array<Vector3<real_Num>> positions, normals;
            Array<Vector2<real_Num>> uvs;
            Array<u32> indices;
            const auto transform = terrain->getWorldTransform();
            for( int z = 0; z < resolution.y; ++z ) for( int x = 0; x < resolution.x; ++x )
            {
                const auto height = heights[size_t( z ) * resolution.x + x];
                if( !std::isfinite( height ) ) return failGeneration( "Non-finite terrain height" );
                const real_Num u = real_Num( x ) / ( resolution.x - 1 );
                const real_Num v = real_Num( z ) / ( resolution.y - 1 );
                const auto position = transform.transformPoint( Vector3<real_Num>( u * size.x, height * size.y, v * size.z ) );
                if( !std::isfinite( position.x ) || !std::isfinite( position.y ) || !std::isfinite( position.z ) )
                    return failGeneration( "Non-finite terrain transform" );
                positions.push_back( position );
                cpu.addVertex( Vector3F( position ) );
                uvs.push_back( m_generateUVs ? Vector2<real_Num>( u * m_uvScale, v * m_uvScale ) : Vector2<real_Num>() );
            }
            for( int z = 0; z + 1 < resolution.y; ++z ) for( int x = 0; x + 1 < resolution.x; ++x )
            {
                const u32 a = static_cast<u32>( uint64_t( z ) * resolution.x + x ), b = a + resolution.x;
                cpu.addTriangle( a, b, a + 1 );
                cpu.addTriangle( a + 1, b, b + 1 );
            }
            if( m_generateNormals ) cpu.computeNormals();
            for( const auto &vertex : cpu.vertices ) normals.push_back( Vector3<real_Num>( vertex.normal ) );
            for( auto index : cpu.indices ) indices.push_back( index );
            auto mesh = MeshUtil::createMesh( positions, normals, uvs, indices );
            if( mesh && mesh->getNumSubMeshes() ) mesh->getSubMesh( 0 )->setMaterialName( "TerrainDefault" );
            return finishGeneration( mesh );
        } catch( const std::exception &e )
        {
            return failGeneration( String( "Generation exception: " ) + e.what() );
        }
        catch( ... )
        {
            return failGeneration( "Unknown generation exception" );
        }

        SmartPtr<IMesh> MeshGeneratorDefault::generateFromObjects( const Array<SmartPtr<IProceduralObject>> &objects )
        try
        {
            if( !beginGeneration() ) return nullptr;
            if( objects.empty() ) return failGeneration( "Empty object list" );
            if( objects.size() == 1 )
                if( auto terrain = workphone::dynamic_pointer_cast<IProceduralTerrain>( objects[0] ) )
                    return generateFromTerrain( terrain );
            // Object interfaces expose no generic mesh. Roads are explicitly supported;
            // reject other types rather than silently losing their geometry or materials.
            Array<Vector3F> positions;
            Array<Vector2F> uvs;
            for( const auto &object : objects )
            {
                auto road = workphone::dynamic_pointer_cast<CRoad>( object );
                if( !road ) return failGeneration( "Object has no supported mesh adapter" );
                auto segments = road->getRoadSegments();
                if( segments.empty() )
                {
                    const auto nodes = road->getRoadNodes();
                    for( size_t i = 1; i < nodes.size(); ++i )
                    {
                        if( !nodes[i-1] || !nodes[i] ) return failGeneration( "Null road node" );
                        const Vector3F a( nodes[i-1]->getPosition() ), b( nodes[i]->getPosition() );
                        const auto delta = b - a;
                        const Vector3F tangent( -delta.z, 0, delta.x );
                        auto segment = RoadGeneratorUtil::createRoadSegment( a, b, tangent, tangent, 7, 7 );
                        if( !segment ) return failGeneration( "Invalid road spine" );
                        segments.push_back( segment );
                    }
                }
                for( const auto &segment : segments )
                {
                    if( !segment ) return failGeneration( "Null road segment" );
                    const auto nodes = segment->getRoadNodes();
                    if( nodes.size() < 2 ) return failGeneration( "Road segment needs two nodes" );
                    float width = 3.5f;
                    if( auto concrete = workphone::dynamic_pointer_cast<CRoadElement>( segment ) ) width = concrete->getRoadWidth() * 0.5f;
                    if( !std::isfinite( width ) || width <= 0 ) return failGeneration( "Invalid road width" );
                    for( size_t i = 1; i < nodes.size(); ++i )
                    {
                        if( !nodes[i-1] || !nodes[i] ) return failGeneration( "Null road node" );
                        if( positions.size() + 6 > m_maxVertexCount || positions.size() / 3 + 2 > m_maxTriangleCount )
                            return failGeneration( "Road geometry budget exceeded" );
                        const Vector3F a( nodes[i-1]->getPosition() ), b( nodes[i]->getPosition() );
                        const auto delta = b - a;
                        const Vector3F tangent( -delta.z, 0, delta.x );
                        const auto previousCount = positions.size();
                        RoadGeneratorUtil::createMeshSegment( a, b, tangent, tangent, width, width, positions, uvs );
                        if( positions.size() != previousCount + 6 || positions.size() != uvs.size() )
                            return failGeneration( "Invalid road segment geometry" );
                        if( !std::isfinite( delta.x ) || !std::isfinite( delta.y ) || !std::isfinite( delta.z ) ||
                            ( std::abs( delta.x ) < 0.000001f && std::abs( delta.z ) < 0.000001f ) )
                            return failGeneration( "Road segment has invalid horizontal extent" );
                        for( size_t vertexIndex = previousCount; vertexIndex < positions.size(); ++vertexIndex )
                        {
                            const auto &p = positions[vertexIndex];
                            if( !std::isfinite( p.x ) || !std::isfinite( p.y ) || !std::isfinite( p.z ) )
                                return failGeneration( "Road position is non-finite" );
                        }
                    }
                }
            }
            if( positions.empty() ) return failGeneration( "No road geometry" );
            Array<Vector3<real_Num>> outputPositions( positions.begin(), positions.end() ), normals;
            Array<Vector2<real_Num>> outputUvs;
            Array<u32> indices;
            ProceduralMesh roadCpu;
            for( const auto &position : positions ) roadCpu.addVertex( position );
            for( u32 i = 0; i + 2 < positions.size(); i += 3 ) roadCpu.addTriangle( i, i + 1, i + 2 );
            if( m_generateNormals ) roadCpu.computeNormals();
            for( size_t i = 0; i < positions.size(); ++i )
            {
                indices.push_back( static_cast<u32>( i ) );
                normals.push_back( Vector3<real_Num>( roadCpu.vertices[i].normal ) );
                outputUvs.push_back( m_generateUVs ? Vector2<real_Num>( uvs[i].x * m_uvScale, uvs[i].y * m_uvScale ) : Vector2<real_Num>() );
            }
            auto mesh = MeshUtil::createMesh( outputPositions, normals, outputUvs, indices );
            if( mesh && mesh->getNumSubMeshes() ) mesh->getSubMesh( 0 )->setMaterialName( "RoadDefault" );
            return finishGeneration( mesh );
        } catch( const std::exception &e )
        {
            return failGeneration( String( "Generation exception: " ) + e.what() );
        }
        catch( ... )
        {
            return failGeneration( "Unknown generation exception" );
        }

        real_Num MeshGeneratorDefault::getMeshQuality() const
        {
            return m_meshQuality;
        }

        void MeshGeneratorDefault::setMeshQuality( real_Num quality )
        {
            m_meshQuality = quality;
            validateParameters();
        }

        u32 MeshGeneratorDefault::getNumLodLevels() const
        {
            return m_numLodLevels;
        }

        void MeshGeneratorDefault::setNumLodLevels( u32 numLods )
        {
            m_numLodLevels = numLods;
            validateParameters();
        }

        real_Num MeshGeneratorDefault::getLodDistance() const
        {
            return m_lodDistance;
        }

        void MeshGeneratorDefault::setLodDistance( real_Num distance )
        {
            m_lodDistance = distance;
            validateParameters();
        }

        u32 MeshGeneratorDefault::getMaxVertexCount() const
        {
            return m_maxVertexCount;
        }

        void MeshGeneratorDefault::setMaxVertexCount( u32 maxVertices )
        {
            m_maxVertexCount = maxVertices;
            validateParameters();
        }

        u32 MeshGeneratorDefault::getMaxTriangleCount() const
        {
            return m_maxTriangleCount;
        }

        void MeshGeneratorDefault::setMaxTriangleCount( u32 maxTriangles )
        {
            m_maxTriangleCount = maxTriangles;
            validateParameters();
        }

        real_Num MeshGeneratorDefault::getSimplificationThreshold() const
        {
            return m_simplificationThreshold;
        }

        void MeshGeneratorDefault::setSimplificationThreshold( real_Num threshold )
        {
            m_simplificationThreshold = threshold;
            validateParameters();
        }

        bool MeshGeneratorDefault::getGenerateUVs() const
        {
            return m_generateUVs;
        }

        void MeshGeneratorDefault::setGenerateUVs( bool generateUVs )
        {
            m_generateUVs = generateUVs;
            validateParameters();
        }

        real_Num MeshGeneratorDefault::getUVScale() const
        {
            return m_uvScale;
        }

        void MeshGeneratorDefault::setUVScale( real_Num scale )
        {
            m_uvScale = scale;
            validateParameters();
        }

        bool MeshGeneratorDefault::getGenerateNormals() const
        {
            return m_generateNormals;
        }

        void MeshGeneratorDefault::setGenerateNormals( bool generateNormals )
        {
            m_generateNormals = generateNormals;
            validateParameters();
        }

        bool MeshGeneratorDefault::getGenerateTangents() const
        {
            return m_generateTangents;
        }

        void MeshGeneratorDefault::setGenerateTangents( bool generateTangents )
        {
            m_generateTangents = generateTangents;
            validateParameters();
        }

        bool MeshGeneratorDefault::getOptimizeMesh() const
        {
            return m_optimizeMesh;
        }

        void MeshGeneratorDefault::setOptimizeMesh( bool optimize )
        {
            m_optimizeMesh = optimize;
            validateParameters();
        }

        bool MeshGeneratorDefault::getMergeDuplicateVertices() const
        {
            return m_mergeDuplicateVertices;
        }

        void MeshGeneratorDefault::setMergeDuplicateVertices( bool merge )
        {
            m_mergeDuplicateVertices = merge;
            validateParameters();
        }

        real_Num MeshGeneratorDefault::getVertexMergeTolerance() const
        {
            return m_vertexMergeTolerance;
        }

        void MeshGeneratorDefault::setVertexMergeTolerance( real_Num tolerance )
        {
            m_vertexMergeTolerance = tolerance;
            validateParameters();
        }

        bool MeshGeneratorDefault::getGenerateCollision() const
        {
            return m_generateCollision;
        }

        void MeshGeneratorDefault::setGenerateCollision( bool generateCollision )
        {
            m_generateCollision = generateCollision;
            validateParameters();
        }

        real_Num MeshGeneratorDefault::getCollisionSimplification() const
        {
            return m_collisionSimplification;
        }

        void MeshGeneratorDefault::setCollisionSimplification( real_Num simplification )
        {
            m_collisionSimplification = simplification;
            validateParameters();
        }

        void MeshGeneratorDefault::validateParameters()
        {
            auto finiteClamp = []( real_Num value, real_Num fallback, real_Num low, real_Num high ) {
                return std::isfinite( value ) ? std::max( low, std::min( high, value ) ) : fallback;
            };
            m_meshQuality = finiteClamp( m_meshQuality, 1, 0, 1 );
            m_numLodLevels = std::max( 1u, std::min( 8u, m_numLodLevels ) );
            m_lodDistance = finiteClamp( m_lodDistance, 100, 0.001, 1000000 );
            m_maxVertexCount = std::max( 3u, m_maxVertexCount );
            m_maxTriangleCount = std::max( 1u, m_maxTriangleCount );
            m_simplificationThreshold = finiteClamp( m_simplificationThreshold, 0, 0, 1 );
            m_uvScale = finiteClamp( m_uvScale, 1, 0.000001, 1000000 );
            m_vertexMergeTolerance = finiteClamp( m_vertexMergeTolerance, 0.0001, 0, 1 );
            m_collisionSimplification = finiteClamp( m_collisionSimplification, 0, 0, 1 );
        }

        bool MeshGeneratorDefault::isReady() const
        {
            return m_numLodLevels == 1 && m_simplificationThreshold == 0 && !m_generateTangents && !m_optimizeMesh && !m_mergeDuplicateVertices && !m_generateCollision;
        }

        void MeshGeneratorDefault::clear()
        {
            m_generatedMeshes.clear();
            m_progress = 0;
            m_status = "Ready";
        }

        SmartPtr<IMesh> MeshGeneratorDefault::getLastGeneratedMesh() const
        {
            return m_generatedMeshes.empty() ? nullptr : m_generatedMeshes.back();
        }

        Array<SmartPtr<IMesh>> MeshGeneratorDefault::getAllGeneratedMeshes() const
        {
            return m_generatedMeshes;
        }

        real_Num MeshGeneratorDefault::getGenerationProgress() const
        {
            return m_progress;
        }

        String MeshGeneratorDefault::getGenerationStatus() const
        {
            return m_status;
        }

        MeshGeneratorDefault::~MeshGeneratorDefault()
        {
        }

        MeshGeneratorDefault::MeshGeneratorDefault()
        {
        }
    }  // namespace procedural
}  // namespace workphone
