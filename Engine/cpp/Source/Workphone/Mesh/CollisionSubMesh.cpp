#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/CollisionSubMesh.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexBoneAssignment.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

#if WP_USE_OPCODE_LIB
#    include <opcode/Opcode.h>
#endif

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, CollisionSubMesh, ISharedObject );

#if WP_USE_OPCODE_LIB

    static void callback( unsigned int triangle_index, Opcode::VertexPointers &triangle, void *userData )
    {
        CollisionSubMesh *mesh = (CollisionSubMesh *)userData;

        triangle_index *= 3;

        u32 indexCount = mesh->m_indexCount;

        int v1i, v2i, v3i;
        v1i = mesh->m_indices[triangle_index];
        v2i = mesh->m_indices[triangle_index + 1];
        v3i = mesh->m_indices[triangle_index + 2];

        u32 vertexCount = mesh->m_vertexCount;

        triangle.Vertex[0] = &mesh->m_points[v1i];
        triangle.Vertex[1] = &mesh->m_points[v2i];
        triangle.Vertex[2] = &mesh->m_points[v3i];
    }
#endif

    CollisionSubMesh::CollisionSubMesh( const SmartPtr<IMesh> mesh, SmartPtr<ISubMesh> subMesh,
                                        Matrix4<real_Num> transform )
    {
        if( !mesh )
        {
            return;
        }

#if WP_USE_OPCODE_LIB
        m_rayCollider = new Opcode::RayCollider;
        m_tree = new Opcode::Model;
        m_mesh = new Opcode::MeshInterface;

        if( !mesh->getHasSharedVertexData() || !subMesh->getUseSharedVertices() )
        {
            SmartPtr<IVertexBuffer> vertexBuffer = subMesh->getVertexBuffer();
            SmartPtr<IIndexBuffer> indexBuffer = subMesh->getIndexBuffer();

            u32 meshIndexCnt = indexBuffer->getNumIndices();
            u32 meshVertexCnt = vertexBuffer->getNumVertices();

            float *vertices = new float[meshVertexCnt * 3];
            u32 *indices = new u32[meshIndexCnt];

            // fill indices
            switch( indexBuffer->getIndexType() )
            {
            case IIndexBuffer::Type::IT_16BIT:
            {
                const u16 *indexData = static_cast<u16 *>( indexBuffer->getIndexData() );
                const u32 indexCount = indexBuffer->getNumIndices();

                for( u32 j = 0; j < indexCount; j++ )
                {
                    u32 index = indexData[j];
                    indices[j] = index;
                }
            }
            break;
            case IIndexBuffer::Type::IT_32BIT:
            {
                const u32 *indexData = static_cast<u32 *>( indexBuffer->getIndexData() );
                const u32 indexCount = indexBuffer->getNumIndices();

                for( u32 j = 0; j < indexCount; j++ )
                {
                    u32 index = indexData[j];
                    indices[j] = index;
                }
            }
            break;
            default:
            {
                throw Exception( "Unknown index type." );
            }
            };

            // fill vertices
            {
                const auto posElem = vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_POSITION, 0 );

                u32 numVerticies = vertexBuffer->getNumVertices();
                u32 vertexSize = vertexBuffer->getVertexDeclaration()->getSize();
                u8 *vertexDataPtr = (u8 *)vertexBuffer->getVertexData();
                float *elementData = 0;

                for( u32 j = 0; j < numVerticies; ++j, vertexDataPtr += vertexSize )
                {
                    posElem->getElementData( vertexDataPtr, &elementData );

                    Vector3<real_Num> position;
                    position.X() = *elementData++;
                    position.Y() = *elementData++;
                    position.Z() = *elementData++;
                    position = transform * position;

                    vertices[j * 3] = position.X();
                    vertices[j * 3 + 1] = position.Y();
                    vertices[j * 3 + 2] = position.Z();
                }
            }

            build( vertices, 3 * sizeof( float ), meshVertexCnt, indices, meshIndexCnt );

            delete[] vertices;
        }
        else
        {
            // Handle shared vertex data case
            u32 meshIndexCnt = 0;
            u32 meshVertexCnt = 0;

            // count vertices and indices
            SmartPtr<IIndexBuffer> indexBuffer = subMesh->getIndexBuffer();
            SmartPtr<IVertexBuffer> sharedVertexBuffer = mesh->getSharedVertexBuffer();

            meshIndexCnt = indexBuffer->getNumIndices();
            meshVertexCnt = sharedVertexBuffer->getNumVertices();

            float *vertices = new float[meshVertexCnt * 3];
            u32 *indices = new u32[meshIndexCnt];

            // fill indices
            switch( indexBuffer->getIndexType() )
            {
            case IIndexBuffer::Type::IT_16BIT:
            {
                const u16 *indexData = static_cast<u16 *>( indexBuffer->getIndexData() );
                const u32 indexCount = indexBuffer->getNumIndices();

                for( u32 j = 0; j < indexCount; j++ )
                {
                    u32 index = indexData[j];
                    indices[j] = index;
                }
            }
            break;
            case IIndexBuffer::Type::IT_32BIT:
            {
                const u32 *indexData = static_cast<u32 *>( indexBuffer->getIndexData() );
                const u32 indexCount = indexBuffer->getNumIndices();

                for( u32 j = 0; j < indexCount; j++ )
                {
                    u32 index = indexData[j];
                    indices[j] = index;
                }
            }
            break;
            default:
            {
                throw Exception( "Unknown index type." );
            }
            };

            // fill vertices from shared vertex data
            {
                const auto posElem = sharedVertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_POSITION, 0 );

                u32 numVerticies = sharedVertexBuffer->getNumVertices();
                u32 vertexSize = sharedVertexBuffer->getVertexDeclaration()->getSize();
                u8 *vertexDataPtr = (u8 *)sharedVertexBuffer->getVertexData();
                float *elementData = 0;

                for( u32 j = 0; j < numVerticies; ++j, vertexDataPtr += vertexSize )
                {
                    posElem->getElementData( vertexDataPtr, &elementData );

                    Vector3<real_Num> position;
                    position.X() = *elementData++;
                    position.Y() = *elementData++;
                    position.Z() = *elementData++;
                    position = transform * position;

                    vertices[j * 3] = position.X();
                    vertices[j * 3 + 1] = position.Y();
                    vertices[j * 3 + 2] = position.Z();
                }
            }

            build( vertices, 3 * sizeof( float ), meshVertexCnt, indices, meshIndexCnt );

            delete[] vertices;
        }
#endif
    }

    CollisionSubMesh::CollisionSubMesh() = default;

    CollisionSubMesh::~CollisionSubMesh()
    {
#if WP_USE_OPCODE_LIB
        if( m_points )
        {
            delete[] m_points;
            m_points = nullptr;
        }

        if( m_indices )
        {
            delete[] m_indices;
            m_indices = nullptr;
        }
#endif
    }

    void CollisionSubMesh::build( float *vertices, int vertexStide, int vertexCount, const void *indices,
                                  int indexCount )
    {
#if WP_USE_OPCODE_LIB
        using namespace Opcode;

        m_indices = (u32 *)indices;
        m_vertexCount = (u32)vertexCount;
        m_indexCount = (u32)indexCount;
        m_triangleCount = (u32)( indexCount / 3 );

        m_mesh->SetNbTriangles( m_triangleCount );
        m_mesh->SetNbVertices( vertexCount );
        m_mesh->SetCallback( callback, this );

        m_points = new IceMaths::Point[vertexCount];

        int temp_vertex_counter = 0;
        for( int i = 0; i < vertexCount; ++i )
        {
            Vector3<real_Num> vec;
            m_points[i] =
                IceMaths::HPoint( vertices[i * 3], vertices[( i * 3 ) + 1], vertices[( i * 3 ) + 2] );
        }

        BuildSettings buildSettings;
        buildSettings.mRules = SPLIT_BEST_AXIS | SPLIT_SPLATTER_POINTS | SPLIT_GEOM_CENTER;
        buildSettings.mLimit = 1;

        OPCODECREATE treeBuilder;
        treeBuilder.mIMesh = m_mesh;
        treeBuilder.mSettings = buildSettings;
        treeBuilder.mNoLeaf = false;
        treeBuilder.mQuantized = true;
        treeBuilder.mKeepOriginal = false;
        treeBuilder.mCanRemap = false;

        if( !m_tree->Build( treeBuilder ) )
        {
            printf( "Could not build opcode mesh" );
        }
#endif
    }

    auto CollisionSubMesh::rayCast( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                                    Array<float> &hits ) -> bool
    {
#if WP_USE_OPCODE_LIB
        IceMaths::Ray worldRay;
        worldRay.mOrig.x = origin.x;
        worldRay.mOrig.y = origin.y;
        worldRay.mOrig.z = origin.z;
        worldRay.mDir.x = dir.x;
        worldRay.mDir.y = dir.y;
        worldRay.mDir.z = dir.z;

        Opcode::CollisionFaces CF;
        CF.Reset();

        m_rayCollider->SetDestination( &CF );
        m_rayCollider->SetFirstContact( true );
        m_rayCollider->SetTemporalCoherence( true );
        m_rayCollider->SetClosestHit( true );
        m_rayCollider->SetCulling( true );

        BOOL IsOk = m_rayCollider->Collide( worldRay, *m_tree );
        BOOL Status = m_rayCollider->GetContactStatus();

        if( Status )
        {
            int NbTouchedPrimitives = CF.GetNbFaces();
            const Opcode::CollisionFace *faces = CF.GetFaces();

            if( NbTouchedPrimitives > 0 )
            {
                for( int i = 0; i < NbTouchedPrimitives; ++i )
                {
                    hits.push_back( faces[i].mDistance );
                }

                return true;
            }
        }
#endif

        return false;
    }

    auto CollisionSubMesh::getUserData() const -> void *
    {
        return m_userData;
    }

    void CollisionSubMesh::setUserData( void *userData )
    {
        m_userData = userData;
    }
}  // namespace workphone
