#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/EdgeData.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/System/ILogManager.hpp>

#include <cassert>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, EdgeData, ISharedObject );

    namespace
    {
        Vector4<f32> calculateFaceNormalWithoutNormalize( const Vector3<f32> &v0, const Vector3<f32> &v1,
                                                          const Vector3<f32> &v2 )
        {
            const auto normal = ( v1 - v0 ).crossProduct( v2 - v0 );
            return Vector4<f32>( normal.x, normal.y, normal.z, -( normal.dotProduct( v0 ) ) );
        }

        Vector3<f32> readPosition( const f32 *positions, size_t vertexIndex )
        {
            const auto offset = vertexIndex * 3;
            return Vector3<f32>( positions[offset], positions[offset + 1], positions[offset + 2] );
        }

        String formatSize( size_t value )
        {
            return StringUtil::toString( static_cast<u64>( value ) );
        }
    }  // namespace

    EdgeData::EdgeData() : isClosed( false )
    {
    }

    EdgeData::~EdgeData() = default;

    void EdgeData::updateTriangleLightFacing( const Vector4<f32> &lightPos )
    {
        assert( triangleFaceNormals.size() == triangleLightFacings.size() );

        for( size_t i = 0; i < triangleFaceNormals.size(); ++i )
        {
            triangleLightFacings[i] = lightPos.dotProduct( triangleFaceNormals[i] ) > 0.0f;
        }
    }

    void EdgeData::updateFaceNormals( size_t vertexSet, const SmartPtr<IVertexBuffer> &positionBuffer )
    {
        assert( triangleFaceNormals.size() == triangles.size() );
        assert( vertexSet < edgeGroups.size() );

        if( !positionBuffer || vertexSet >= edgeGroups.size() )
        {
            return;
        }

        const auto *positions = static_cast<const f32 *>( positionBuffer->getVertexData() );
        if( !positions )
        {
            return;
        }

        const EdgeGroup &edgeGroup = edgeGroups[vertexSet];
        for( size_t i = 0; i < edgeGroup.triCount; ++i )
        {
            const auto triangleIndex = edgeGroup.triStart + i;
            const Triangle &triangle = triangles[triangleIndex];

            const auto v0 = readPosition( positions, triangle.vertIndex[0] );
            const auto v1 = readPosition( positions, triangle.vertIndex[1] );
            const auto v2 = readPosition( positions, triangle.vertIndex[2] );

            triangleFaceNormals[triangleIndex] = calculateFaceNormalWithoutNormalize( v0, v1, v2 );
        }
    }

    EdgeData *EdgeData::clone()
    {
        auto *newEdgeData = new EdgeData();
        newEdgeData->triangles = triangles;
        newEdgeData->triangleFaceNormals = triangleFaceNormals;
        newEdgeData->triangleLightFacings = triangleLightFacings;
        newEdgeData->edgeGroups = edgeGroups;
        newEdgeData->isClosed = isClosed;
        return newEdgeData;
    }

    void EdgeData::log( ILogManager *log )
    {
        if( !log )
        {
            return;
        }

        log->logMessage( "Edge Data", ILogManager::Type::Info );
        log->logMessage( "---------", ILogManager::Type::Info );

        for( size_t i = 0; i < triangles.size(); ++i )
        {
            const Triangle &triangle = triangles[i];
            log->logMessage( String( "Triangle " ) + formatSize( i ) + " = {" +
                                 "indexSet=" + formatSize( triangle.indexSet ) + ", " +
                                 "vertexSet=" + formatSize( triangle.vertexSet ) + ", " +
                                 "v0=" + formatSize( triangle.vertIndex[0] ) + ", " +
                                 "v1=" + formatSize( triangle.vertIndex[1] ) + ", " +
                                 "v2=" + formatSize( triangle.vertIndex[2] ) + "}",
                             ILogManager::Type::Info );
        }

        for( const EdgeGroup &edgeGroup : edgeGroups )
        {
            log->logMessage( String( "Edge Group vertexSet=" ) + formatSize( edgeGroup.vertexSet ),
                             ILogManager::Type::Info );

            for( size_t i = 0; i < edgeGroup.edges.size(); ++i )
            {
                const Edge &edge = edgeGroup.edges[i];
                log->logMessage( String( "Edge " ) + formatSize( i ) + " = {\n" +
                                     "  tri0=" + formatSize( edge.triIndex[0] ) + ", \n" +
                                     "  tri1=" + formatSize( edge.triIndex[1] ) + ", \n" +
                                     "  v0=" + formatSize( edge.vertIndex[0] ) + ", \n" +
                                     "  v1=" + formatSize( edge.vertIndex[1] ) + ", \n" +
                                     "  degenerate=" + StringUtil::toString( edge.degenerate ) +
                                     " \n"
                                     "}",
                                 ILogManager::Type::Info );
            }
        }
    }

    EdgeData::Triangle::Triangle() : indexSet( 0 ), vertexSet( 0 )
    {
        vertIndex[0] = 0;
        vertIndex[1] = 0;
        vertIndex[2] = 0;
        sharedVertIndex[0] = 0;
        sharedVertIndex[1] = 0;
        sharedVertIndex[2] = 0;
    }
}  // namespace workphone
