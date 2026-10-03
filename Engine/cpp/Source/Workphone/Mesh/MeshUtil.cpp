#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/VertexElement.hpp>
#include <Workphone/Mesh/Vertex.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Mesh/IVertexBoneAssignment.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/VertexElement.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>

namespace workphone
{

    Sphere3<real_Num> MeshUtil::calculateBoundingSphere( const Array<Vector3<real_Num>> &points )
    {
        if( points.size() == 1 )
        {
            return Sphere3<real_Num>( points[0], static_cast<real_Num>( 0.0 ) );
        }

        // Find the extreme points along each axis
        Vector3<real_Num> xmin = points[0], xmax = points[0];
        Vector3<real_Num> ymin = points[0], ymax = points[0];
        Vector3<real_Num> zmin = points[0], zmax = points[0];

        for( const auto &point : points )
        {
            if( point.X() < xmin.X() )
                xmin = point;
            if( point.X() > xmax.X() )
                xmax = point;
            if( point.Y() < ymin.Y() )
                ymin = point;
            if( point.Y() > ymax.Y() )
                ymax = point;
            if( point.Z() < zmin.Z() )
                zmin = point;
            if( point.Z() > zmax.Z() )
                zmax = point;
        }

        // Calculate squared distances for each axis span
        real_Num xSpan = ( xmax - xmin ).lengthSquared();
        real_Num ySpan = ( ymax - ymin ).lengthSquared();
        real_Num zSpan = ( zmax - zmin ).lengthSquared();

        // Find the pair with maximum distance
        Vector3<real_Num> dia1 = xmin;
        Vector3<real_Num> dia2 = xmax;
        real_Num maxSpan = xSpan;

        if( ySpan > maxSpan )
        {
            maxSpan = ySpan;
            dia1 = ymin;
            dia2 = ymax;
        }

        if( zSpan > maxSpan )
        {
            dia1 = zmin;
            dia2 = zmax;
        }

        // Initial sphere from diameter
        Vector3<real_Num> center = ( dia1 + dia2 ) * static_cast<real_Num>( 0.5 );
        real_Num radiusSq = ( dia2 - center ).lengthSquared();
        real_Num radius = Math<real_Num>::Sqrt( radiusSq );

        // Iteratively expand sphere to include all points
        for( const auto &point : points )
        {
            real_Num distSq = ( point - center ).lengthSquared();

            if( distSq > radiusSq )
            {
                real_Num dist = Math<real_Num>::Sqrt( distSq );

                // Calculate new radius and center
                real_Num newRadius = ( radius + dist ) * static_cast<real_Num>( 0.5 );
                real_Num offset = dist - newRadius;

                // Move center towards the outlying point
                center = ( radius * center + offset * point ) / dist;
                radius = newRadius;
                radiusSq = radius * radius;
            }
        }

        return Sphere3<real_Num>( center, radius );
    }

    Sphere3<real_Num> MeshUtil::calculateBoundingSphere( SmartPtr<IMesh> mesh )
    {
        if( !mesh )
        {
            return Sphere3<real_Num>();
        }

        // Get all vertex positions from the mesh
        auto points = getPoints( mesh );
        if( points.empty() )
        {
            return Sphere3<real_Num>();
        }

        return calculateBoundingSphere( points );
    }

    AABB3<real_Num> MeshUtil::calculateBoundingBox( const Array<Vector3<real_Num>> &points )
    {
        // Initialize min and max points with the first vertex
        Vector3<real_Num> minPoint = points[0];
        Vector3<real_Num> maxPoint = points[0];

        // Find the minimum and maximum coordinates for each axis
        for( const auto &point : points )
        {
            if( point.X() < minPoint.X() )
                minPoint.X() = point.X();
            if( point.Y() < minPoint.Y() )
                minPoint.Y() = point.Y();
            if( point.Z() < minPoint.Z() )
                minPoint.Z() = point.Z();

            if( point.X() > maxPoint.X() )
                maxPoint.X() = point.X();
            if( point.Y() > maxPoint.Y() )
                maxPoint.Y() = point.Y();
            if( point.Z() > maxPoint.Z() )
                maxPoint.Z() = point.Z();
        }

        // Create and return the bounding box
        return AABB3<real_Num>( minPoint, maxPoint );
    }

    AABB3<real_Num> MeshUtil::calculateBoundingBox( SmartPtr<IMesh> mesh )
    {
        if( !mesh )
        {
            return AABB3<real_Num>();
        }

        // Get all vertex positions from the mesh
        auto points = getPoints( mesh );
        if( points.empty() )
        {
            return AABB3<real_Num>();
        }

        return calculateBoundingBox( points );
    }

    auto MeshUtil::createVertexBuffer( const Array<Vector3<real_Num>> &positions,
                                       const Array<Vector3<real_Num>> &normals,
                                       const Array<Vector2<real_Num>> &uvs ) -> SmartPtr<IVertexBuffer>
    {
        auto numVertices = positions.size();

        auto vertexDeclaration = workphone::make_ptr<VertexDeclaration>();
        u32 offset = 0;

        auto element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_POSITION,
                                                      VertexElementType::VET_FLOAT3 );
        offset = element->getSize();

        element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_NORMAL,
                                                 VertexElementType::VET_FLOAT3 );
        offset += element->getSize();

        element =
            vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                           VertexElementType::VET_FLOAT2, 0 );
        offset += element->getSize();

        auto vertexBuffer = workphone::make_ptr<VertexBuffer>();
        vertexBuffer->setVertexDeclaration( vertexDeclaration );
        vertexBuffer->setNumVertices( static_cast<u32>( numVertices ) );

        auto vertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
        f32 *vertexDataPtr = vertexData;

        for( size_t i = 0; i < numVertices; ++i )
        {
            const auto &position = positions[i];
            *vertexDataPtr++ = position.X();
            *vertexDataPtr++ = position.Y();
            *vertexDataPtr++ = position.Z();

            const auto &normal = normals[i];
            *vertexDataPtr++ = normal.X();
            *vertexDataPtr++ = normal.Y();
            *vertexDataPtr++ = normal.Z();

            const auto &uv = uvs[i];
            *vertexDataPtr++ = uv.X();
            *vertexDataPtr++ = uv.Y();
        }

        return vertexBuffer;
    }

    auto MeshUtil::createVertexBuffer( const Array<Vertex> &vertices ) -> SmartPtr<IVertexBuffer>
    {
        auto vertexDeclaration = workphone::make_ptr<VertexDeclaration>();
        u32 offset = 0;

        auto element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_POSITION,
                                                      VertexElementType::VET_FLOAT3 );
        offset = element->getSize();

        element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_NORMAL,
                                                 VertexElementType::VET_FLOAT3 );
        offset += element->getSize();

        element =
            vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                           VertexElementType::VET_FLOAT2, 0 );
        offset += element->getSize();

        auto vertexBuffer = workphone::make_ptr<VertexBuffer>();
        vertexBuffer->setVertexDeclaration( vertexDeclaration );
        vertexBuffer->setNumVertices( 4 );

        auto vertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
        f32 *vertexDataPtr = vertexData;

        for( auto &vert : vertices )
        {
            auto &position = vert.position;
            *vertexDataPtr++ = position.X();
            *vertexDataPtr++ = position.Y();
            *vertexDataPtr++ = position.Z();

            auto &normal = vert.normal;
            *vertexDataPtr++ = normal.X();
            *vertexDataPtr++ = normal.Y();
            *vertexDataPtr++ = normal.Z();

            auto &uv = vert.texCoord;
            *vertexDataPtr++ = uv.X();
            *vertexDataPtr++ = uv.Y();
        }

        return vertexBuffer;
    }

    auto MeshUtil::createIndexBuffer( const Array<u32> &indices ) -> SmartPtr<IIndexBuffer>
    {
        auto indexBuffer = workphone::make_ptr<IndexBuffer>();
        if( indices.size() > std::numeric_limits<u32>::max() )
        {
            WP_LOG_ERROR(
                "MeshUtil::createIndexBuffer: index count exceeds the "
                "supported 32-bit range." );
            return nullptr;
        }

        const auto maxIndex = indices.empty() ? 0u : *std::max_element( indices.begin(), indices.end() );
        const auto use32Bit = maxIndex > std::numeric_limits<u16>::max();
        indexBuffer->setIndexType( use32Bit ? IndexBuffer::Type::IT_32BIT
                                            : IndexBuffer::Type::IT_16BIT );
        indexBuffer->setNumIndices( static_cast<u32>( indices.size() ) );

        if( use32Bit )
        {
            auto indexData = static_cast<u32 *>( indexBuffer->createIndexData() );
            std::copy( indices.begin(), indices.end(), indexData );
        }
        else
        {
            auto indexData = static_cast<u16 *>( indexBuffer->createIndexData() );
            for( const auto index : indices )
            {
                *indexData++ = static_cast<u16>( index );
            }
        }
        return indexBuffer;
    }

    auto MeshUtil::createSubMesh( SmartPtr<IVertexBuffer> vertexBuffer,
                                  SmartPtr<IIndexBuffer> indexBuffer ) -> SmartPtr<ISubMesh>
    {
        auto subMesh = workphone::make_ptr<SubMesh>();
        subMesh->setVertexBuffer( vertexBuffer );
        subMesh->setIndexBuffer( indexBuffer );

        return subMesh;
    }

    SmartPtr<ISubMesh> MeshUtil::createSubMesh( Array<Vertex> vertices, Array<u32> indices )
    {
        auto subMesh = workphone::make_ptr<SubMesh>();

        auto vertexBuffer = MeshUtil::createVertexBuffer( vertices );
        auto indexBuffer = MeshUtil::createIndexBuffer( indices );

        subMesh->setVertexBuffer( vertexBuffer );
        subMesh->setIndexBuffer( indexBuffer );

        return subMesh;
    }

    auto MeshUtil::createMesh( const Array<Vector3<real_Num>> &vertices,
                               const Array<Vector3<real_Num>> &normals,
                               const Array<Vector2<real_Num>> &uvs, const Array<u32> &indices )
        -> SmartPtr<IMesh>
    {
        auto vertexBuffer = MeshUtil::createVertexBuffer( vertices, normals, uvs );
        auto indexBuffer = MeshUtil::createIndexBuffer( indices );

        auto subMesh = MeshUtil::createSubMesh( vertexBuffer, indexBuffer );

        auto mesh = workphone::make_ptr<Mesh>();
        mesh->addSubMesh( subMesh );
        return mesh;
    }

    auto MeshUtil::createMesh( const Array<Vector3<real_Num>> &vertices,
                               const Array<Vector3<real_Num>> &normals, const Array<Vector4F> &tangents,
                               const Array<Vector3<real_Num>> &uvs, const Array<u32> &indices )
        -> SmartPtr<IMesh>
    {
        auto numVertices = vertices.size();

        auto vertexDeclaration = workphone::make_ptr<VertexDeclaration>();
        u32 offset = 0;

        auto element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_POSITION,
                                                      VertexElementType::VET_FLOAT3 );
        offset += element->getSize();

        element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_NORMAL,
                                                 VertexElementType::VET_FLOAT3 );
        offset += element->getSize();

        element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_TANGENT,
                                                 VertexElementType::VET_FLOAT4 );
        offset += element->getSize();

        element =
            vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                           VertexElementType::VET_FLOAT2, 0 );
        offset += element->getSize();

        auto vertexBuffer = workphone::make_ptr<VertexBuffer>();
        vertexBuffer->setVertexDeclaration( vertexDeclaration );
        vertexBuffer->setNumVertices( static_cast<u32>( numVertices ) );

        auto vertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
        f32 *vertexDataPtr = vertexData;

        for( size_t i = 0; i < numVertices; ++i )
        {
            const auto &position = vertices[i];
            *vertexDataPtr++ = position.X();
            *vertexDataPtr++ = position.Y();
            *vertexDataPtr++ = position.Z();

            const auto &normal = normals[i];
            *vertexDataPtr++ = normal.X();
            *vertexDataPtr++ = normal.Y();
            *vertexDataPtr++ = normal.Z();

            const auto &tangent = tangents[i];
            *vertexDataPtr++ = tangent.X();
            *vertexDataPtr++ = tangent.Y();
            *vertexDataPtr++ = tangent.Z();
            *vertexDataPtr++ = tangent.W();

            const auto &uv = uvs[i];
            *vertexDataPtr++ = uv.X();
            *vertexDataPtr++ = uv.Y();
        }

        auto indexBuffer = MeshUtil::createIndexBuffer( indices );

        auto subMesh = MeshUtil::createSubMesh( vertexBuffer, indexBuffer );

        auto mesh = workphone::make_ptr<Mesh>();
        mesh->addSubMesh( subMesh );
        return mesh;
    }

    auto MeshUtil::createMesh( const Array<Vector3<real_Num>> &vertices,
                               const Array<Vector3<real_Num>> &normals, const Array<Vector4F> &tangents,
                               const Array<Vector3<real_Num>> &uvs0,
                               const Array<Vector3<real_Num>> &uvs1, const Array<u32> &indices )
        -> SmartPtr<IMesh>
    {
        auto numVertices = vertices.size();

        auto vertexDeclaration = workphone::make_ptr<VertexDeclaration>();
        u32 offset = 0;

        auto element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_POSITION,
                                                      VertexElementType::VET_FLOAT3 );
        offset += element->getSize();

        element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_NORMAL,
                                                 VertexElementType::VET_FLOAT3 );
        offset += element->getSize();

        element = vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_TANGENT,
                                                 VertexElementType::VET_FLOAT4 );
        offset += element->getSize();

        element =
            vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                           VertexElementType::VET_FLOAT3, 0 );
        offset += element->getSize();

        element =
            vertexDeclaration->addElement( 0, offset, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                           VertexElementType::VET_FLOAT3, 1 );
        offset += element->getSize();

        auto vertexBuffer = workphone::make_ptr<VertexBuffer>();
        vertexBuffer->setVertexDeclaration( vertexDeclaration );
        vertexBuffer->setNumVertices( static_cast<u32>( numVertices ) );

        auto vertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
        f32 *vertexDataPtr = vertexData;

        for( size_t i = 0; i < numVertices; ++i )
        {
            const auto &position = vertices[i];
            *vertexDataPtr++ = position.X();
            *vertexDataPtr++ = position.Y();
            *vertexDataPtr++ = position.Z();

            const auto &normal = normals[i];
            *vertexDataPtr++ = normal.X();
            *vertexDataPtr++ = normal.Y();
            *vertexDataPtr++ = normal.Z();

            const auto &tangent = tangents[i];
            *vertexDataPtr++ = tangent.X();
            *vertexDataPtr++ = tangent.Y();
            *vertexDataPtr++ = tangent.Z();
            *vertexDataPtr++ = tangent.W();

            const auto &uv0 = uvs0[i];
            *vertexDataPtr++ = uv0.X();
            *vertexDataPtr++ = uv0.Y();
            *vertexDataPtr++ = uv0.Z();

            const auto &uv1 = uvs1[i];
            *vertexDataPtr++ = uv1.X();
            *vertexDataPtr++ = uv1.Y();
            *vertexDataPtr++ = uv1.Z();
        }

        auto indexBuffer = MeshUtil::createIndexBuffer( indices );

        auto subMesh = MeshUtil::createSubMesh( vertexBuffer, indexBuffer );

        auto mesh = workphone::make_ptr<Mesh>();
        mesh->addSubMesh( subMesh );
        return mesh;
    }

    auto MeshUtil::createPlane( const Vector3<real_Num> &halfExtent, const Vector3<real_Num> &normal,
                                const Vector3<real_Num> &right ) -> SmartPtr<IMesh>
    {
        Vertex v[4];

        auto binormal = normal.crossProduct( right );

        // top left
        v[0].position = binormal - right;

        // top right
        v[1].position = binormal + right;

        // bottom left
        v[2].position = -binormal - right;

        // bottom right
        v[3].position = -binormal + right;

        v[0].normal = normal;
        v[1].normal = normal;
        v[2].normal = normal;
        v[3].normal = normal;

        auto texCoordScale = Vector2<real_Num>::unit() * 1.0f;

        v[0].texCoord = Vector2<real_Num>( 0, 0 ) * texCoordScale;
        v[1].texCoord = Vector2<real_Num>( 1, 0 ) * texCoordScale;
        v[2].texCoord = Vector2<real_Num>( 0, 1 ) * texCoordScale;
        v[3].texCoord = Vector2<real_Num>( 1, 1 ) * texCoordScale;

        auto planeMesh = workphone::make_ptr<Mesh>();

        auto subMesh = workphone::make_ptr<SubMesh>();
        planeMesh->addSubMesh( subMesh );

        // create sub mesh data
        auto vertexDeclaration = workphone::make_ptr<VertexDeclaration>();
        u32 offset = 0;

        offset = vertexDeclaration
                     ->addElement( 0, offset, VertexElementSemantic::VES_POSITION,
                                   VertexElementType::VET_FLOAT3 )
                     ->getSize();
        offset += vertexDeclaration
                      ->addElement( 0, offset, VertexElementSemantic::VES_NORMAL,
                                    VertexElementType::VET_FLOAT3 )
                      ->getSize();
        offset += vertexDeclaration
                      ->addElement( 0, offset, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                    VertexElementType::VET_FLOAT2, 0 )
                      ->getSize();

        auto vertexBuffer = workphone::make_ptr<VertexBuffer>();
        vertexBuffer->setVertexDeclaration( vertexDeclaration );
        vertexBuffer->setNumVertices( 4 );

        auto vertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
        f32 *vertexDataPtr = vertexData;

        for( auto &vert : v )
        {
            auto &position = vert.position;
            *vertexDataPtr++ = position.X();
            *vertexDataPtr++ = position.Y();
            *vertexDataPtr++ = position.Z();

            auto &normal = vert.normal;
            *vertexDataPtr++ = normal.X();
            *vertexDataPtr++ = normal.Y();
            *vertexDataPtr++ = normal.Z();

            auto &uv = vert.texCoord;
            *vertexDataPtr++ = uv.X();
            *vertexDataPtr++ = uv.Y();
        }

        auto indexBuffer = workphone::make_ptr<IndexBuffer>();
        indexBuffer->setIndexType( IndexBuffer::Type::IT_16BIT );
        indexBuffer->setNumIndices( 6 );

        auto indexDataPtr = static_cast<u16 *>( indexBuffer->createIndexData() );

        indexDataPtr[0] = 0;
        indexDataPtr[1] = 2;
        indexDataPtr[2] = 3;

        indexDataPtr[3] = 0;
        indexDataPtr[4] = 3;
        indexDataPtr[5] = 1;

        subMesh->setVertexBuffer( vertexBuffer );
        subMesh->setIndexBuffer( indexBuffer );

        return planeMesh;
    }

    auto MeshUtil::createBox( f32 sizeX, f32 sizeY, f32 sizeZ, u32 numSegX, u32 numSegY, u32 numSegZ )
        -> SmartPtr<IMesh>
    {
        Array<Vector3<real_Num>> vertices;
        Array<Vector3<real_Num>> normals;
        Array<Vector2<real_Num>> uvs;
        Array<u32> indices;

        // Reserve space for vertices (6 faces, each with (numSegX+1)*(numSegY+1) vertices)
        auto totalVertices = 2 * ( numSegX + 1 ) * ( numSegY + 1 ) +  // front and back faces
                             2 * ( numSegX + 1 ) * ( numSegZ + 1 ) +  // top and bottom faces
                             2 * ( numSegY + 1 ) * ( numSegZ + 1 );   // left and right faces

        vertices.reserve( totalVertices );
        normals.reserve( totalVertices );
        uvs.reserve( totalVertices );

        auto halfX = sizeX * 0.5f;
        auto halfY = sizeY * 0.5f;
        auto halfZ = sizeZ * 0.5f;

        u32 vertexOffset = 0;

        // Front face (positive Z)
        for( u32 y = 0; y <= numSegY; ++y )
        {
            for( u32 x = 0; x <= numSegX; ++x )
            {
                auto xPos = -halfX + ( x * sizeX ) / static_cast<f32>( numSegX );
                auto yPos = -halfY + ( y * sizeY ) / static_cast<f32>( numSegY );
                auto zPos = halfZ;

                vertices.push_back( Vector3<real_Num>( xPos, yPos, zPos ) );
                normals.push_back( Vector3<real_Num>( 0, 0, 1 ) );
                uvs.push_back( Vector2<real_Num>( x / static_cast<f32>( numSegX ),
                                                  y / static_cast<f32>( numSegY ) ) );
            }
        }

        // Generate indices for front face
        for( u32 y = 0; y < numSegY; ++y )
        {
            for( u32 x = 0; x < numSegX; ++x )
            {
                auto i0 = vertexOffset + y * ( numSegX + 1 ) + x;
                auto i1 = vertexOffset + y * ( numSegX + 1 ) + x + 1;
                auto i2 = vertexOffset + ( y + 1 ) * ( numSegX + 1 ) + x;
                auto i3 = vertexOffset + ( y + 1 ) * ( numSegX + 1 ) + x + 1;

                // Two triangles per quad
                indices.push_back( i0 );
                indices.push_back( i2 );
                indices.push_back( i1 );

                indices.push_back( i1 );
                indices.push_back( i2 );
                indices.push_back( i3 );
            }
        }
        vertexOffset += ( numSegX + 1 ) * ( numSegY + 1 );

        // Back face (negative Z)
        for( u32 y = 0; y <= numSegY; ++y )
        {
            for( u32 x = 0; x <= numSegX; ++x )
            {
                auto xPos = halfX - ( x * sizeX ) / static_cast<f32>( numSegX );
                auto yPos = -halfY + ( y * sizeY ) / static_cast<f32>( numSegY );
                auto zPos = -halfZ;

                vertices.push_back( Vector3<real_Num>( xPos, yPos, zPos ) );
                normals.push_back( Vector3<real_Num>( 0, 0, -1 ) );
                uvs.push_back( Vector2<real_Num>( x / static_cast<f32>( numSegX ),
                                                  y / static_cast<f32>( numSegY ) ) );
            }
        }

        // Generate indices for back face
        for( u32 y = 0; y < numSegY; ++y )
        {
            for( u32 x = 0; x < numSegX; ++x )
            {
                auto i0 = vertexOffset + y * ( numSegX + 1 ) + x;
                auto i1 = vertexOffset + y * ( numSegX + 1 ) + x + 1;
                auto i2 = vertexOffset + ( y + 1 ) * ( numSegX + 1 ) + x;
                auto i3 = vertexOffset + ( y + 1 ) * ( numSegX + 1 ) + x + 1;

                // Two triangles per quad
                indices.push_back( i0 );
                indices.push_back( i1 );
                indices.push_back( i2 );

                indices.push_back( i1 );
                indices.push_back( i3 );
                indices.push_back( i2 );
            }
        }
        vertexOffset += ( numSegX + 1 ) * ( numSegY + 1 );

        // Top face (positive Y)
        for( u32 z = 0; z <= numSegZ; ++z )
        {
            for( u32 x = 0; x <= numSegX; ++x )
            {
                auto xPos = -halfX + ( x * sizeX ) / static_cast<f32>( numSegX );
                auto yPos = halfY;
                auto zPos = halfZ - ( z * sizeZ ) / static_cast<f32>( numSegZ );

                vertices.push_back( Vector3<real_Num>( xPos, yPos, zPos ) );
                normals.push_back( Vector3<real_Num>( 0, 1, 0 ) );
                uvs.push_back( Vector2<real_Num>( x / static_cast<f32>( numSegX ),
                                                  z / static_cast<f32>( numSegZ ) ) );
            }
        }

        // Generate indices for top face
        for( u32 z = 0; z < numSegZ; ++z )
        {
            for( u32 x = 0; x < numSegX; ++x )
            {
                auto i0 = vertexOffset + z * ( numSegX + 1 ) + x;
                auto i1 = vertexOffset + z * ( numSegX + 1 ) + x + 1;
                auto i2 = vertexOffset + ( z + 1 ) * ( numSegX + 1 ) + x;
                auto i3 = vertexOffset + ( z + 1 ) * ( numSegX + 1 ) + x + 1;

                // Two triangles per quad
                indices.push_back( i0 );
                indices.push_back( i2 );
                indices.push_back( i1 );

                indices.push_back( i1 );
                indices.push_back( i2 );
                indices.push_back( i3 );
            }
        }
        vertexOffset += ( numSegX + 1 ) * ( numSegZ + 1 );

        // Bottom face (negative Y)
        for( u32 z = 0; z <= numSegZ; ++z )
        {
            for( u32 x = 0; x <= numSegX; ++x )
            {
                auto xPos = -halfX + ( x * sizeX ) / static_cast<f32>( numSegX );
                auto yPos = -halfY;
                auto zPos = -halfZ + ( z * sizeZ ) / static_cast<f32>( numSegZ );

                vertices.push_back( Vector3<real_Num>( xPos, yPos, zPos ) );
                normals.push_back( Vector3<real_Num>( 0, -1, 0 ) );
                uvs.push_back( Vector2<real_Num>( x / static_cast<f32>( numSegX ),
                                                  z / static_cast<f32>( numSegZ ) ) );
            }
        }

        // Generate indices for bottom face
        for( u32 z = 0; z < numSegZ; ++z )
        {
            for( u32 x = 0; x < numSegX; ++x )
            {
                auto i0 = vertexOffset + z * ( numSegX + 1 ) + x;
                auto i1 = vertexOffset + z * ( numSegX + 1 ) + x + 1;
                auto i2 = vertexOffset + ( z + 1 ) * ( numSegX + 1 ) + x;
                auto i3 = vertexOffset + ( z + 1 ) * ( numSegX + 1 ) + x + 1;

                // Two triangles per quad
                indices.push_back( i0 );
                indices.push_back( i1 );
                indices.push_back( i2 );

                indices.push_back( i1 );
                indices.push_back( i3 );
                indices.push_back( i2 );
            }
        }
        vertexOffset += ( numSegX + 1 ) * ( numSegZ + 1 );

        // Right face (positive X)
        for( u32 z = 0; z <= numSegZ; ++z )
        {
            for( u32 y = 0; y <= numSegY; ++y )
            {
                auto xPos = halfX;
                auto yPos = -halfY + ( y * sizeY ) / static_cast<f32>( numSegY );
                auto zPos = halfZ - ( z * sizeZ ) / static_cast<f32>( numSegZ );

                vertices.push_back( Vector3<real_Num>( xPos, yPos, zPos ) );
                normals.push_back( Vector3<real_Num>( 1, 0, 0 ) );
                uvs.push_back( Vector2<real_Num>( z / static_cast<f32>( numSegZ ),
                                                  y / static_cast<f32>( numSegY ) ) );
            }
        }

        // Generate indices for right face
        for( u32 z = 0; z < numSegZ; ++z )
        {
            for( u32 y = 0; y < numSegY; ++y )
            {
                auto i0 = vertexOffset + z * ( numSegY + 1 ) + y;
                auto i1 = vertexOffset + z * ( numSegY + 1 ) + y + 1;
                auto i2 = vertexOffset + ( z + 1 ) * ( numSegY + 1 ) + y;
                auto i3 = vertexOffset + ( z + 1 ) * ( numSegY + 1 ) + y + 1;

                // Two triangles per quad
                indices.push_back( i0 );
                indices.push_back( i1 );
                indices.push_back( i2 );

                indices.push_back( i1 );
                indices.push_back( i3 );
                indices.push_back( i2 );
            }
        }
        vertexOffset += ( numSegY + 1 ) * ( numSegZ + 1 );

        // Left face (negative X)
        for( u32 z = 0; z <= numSegZ; ++z )
        {
            for( u32 y = 0; y <= numSegY; ++y )
            {
                auto xPos = -halfX;
                auto yPos = -halfY + ( y * sizeY ) / static_cast<f32>( numSegY );
                auto zPos = -halfZ + ( z * sizeZ ) / static_cast<f32>( numSegZ );

                vertices.push_back( Vector3<real_Num>( xPos, yPos, zPos ) );
                normals.push_back( Vector3<real_Num>( -1, 0, 0 ) );
                uvs.push_back( Vector2<real_Num>( z / static_cast<f32>( numSegZ ),
                                                  y / static_cast<f32>( numSegY ) ) );
            }
        }

        // Generate indices for left face
        for( u32 z = 0; z < numSegZ; ++z )
        {
            for( u32 y = 0; y < numSegY; ++y )
            {
                auto i0 = vertexOffset + z * ( numSegY + 1 ) + y;
                auto i1 = vertexOffset + z * ( numSegY + 1 ) + y + 1;
                auto i2 = vertexOffset + ( z + 1 ) * ( numSegY + 1 ) + y;
                auto i3 = vertexOffset + ( z + 1 ) * ( numSegY + 1 ) + y + 1;

                // Two triangles per quad
                indices.push_back( i0 );
                indices.push_back( i2 );
                indices.push_back( i1 );

                indices.push_back( i1 );
                indices.push_back( i2 );
                indices.push_back( i3 );
            }
        }

        return MeshUtil::createMesh( vertices, normals, uvs, indices );
    }

    auto MeshUtil::createCylinder( f32 radius, f32 height, u32 numSegBase, u32 numSegHeight,
                                   bool capped ) -> SmartPtr<IMesh>
    {
        Array<Vector3<real_Num>> vertices;
        Array<Vector3<real_Num>> normals;
        Array<Vector2<real_Num>> uvs;
        Array<u32> indices;

        vertices.reserve( 1000 );
        normals.reserve( 1000 );
        uvs.reserve( 1000 );
        indices.reserve( 1000 );

        auto deltaAngle = ( ( Math<real_Num>::pi() * static_cast<real_Num>( 2.0 ) ) / numSegBase );
        auto deltaHeight = height / static_cast<real_Num>( numSegHeight );
        auto offset = 0;

        for( u32 i = 0; i <= numSegHeight; i++ )
        {
            for( u32 j = 0; j <= numSegBase; j++ )
            {
                auto x0 = radius * Math<real_Num>::Cos( j * deltaAngle );
                auto z0 = radius * Math<real_Num>::Sin( j * deltaAngle );

                auto position = Vector3<real_Num>( x0, i * deltaHeight, z0 );
                auto normal = Vector3<real_Num>( x0, 0, z0 ).normaliseCopy();
                auto texCoord = Vector2<real_Num>( j / static_cast<f32>( numSegBase ),
                                                   i / static_cast<f32>( numSegHeight ) );

                vertices.push_back( position );
                normals.push_back( normal );
                uvs.push_back( texCoord );

                if( i != numSegHeight )
                {
                    indices.push_back( offset + numSegBase + 1 );
                    indices.push_back( offset );
                    indices.push_back( offset + numSegBase );
                    indices.push_back( offset + numSegBase + 1 );
                    indices.push_back( offset + 1 );
                    indices.push_back( offset );
                }

                offset++;
            }
        }

        return MeshUtil::createMesh( vertices, normals, uvs, indices );
    }

    auto MeshUtil::createCylinderFromSpline( SmartPtr<LinearSpline3<real_Num>> spline, f32 radius,
                                             f32 height, u32 numSegBase, u32 numSegHeight, bool capped )
        -> SmartPtr<IMesh>
    {
        Array<Vector3<real_Num>> vertices;
        Array<Vector3<real_Num>> normals;
        Array<Vector2<real_Num>> uvs;
        Array<u32> indices;

        vertices.reserve( 1000 );
        normals.reserve( 1000 );
        uvs.reserve( 1000 );
        indices.reserve( 1000 );

        auto deltaAngle = ( ( Math<real_Num>::pi() * static_cast<real_Num>( 2.0 ) ) / numSegBase );
        auto deltaHeight = height / static_cast<real_Num>( numSegHeight );
        auto offset = 0;

        for( u32 i = 0; i <= numSegHeight; i++ )
        {
            for( u32 j = 0; j <= numSegBase; j++ )
            {
                auto x0 = radius * Math<real_Num>::Cos( j * deltaAngle );
                auto z0 = radius * Math<real_Num>::Sin( j * deltaAngle );

                auto position = Vector3<real_Num>( x0, i * deltaHeight, z0 );
                auto normal = Vector3<real_Num>( x0, 0, z0 ).normaliseCopy();
                auto texCoord = Vector2<real_Num>( j / static_cast<f32>( numSegBase ),
                                                   i / static_cast<f32>( numSegHeight ) );

                vertices.push_back( position );
                normals.push_back( normal );
                uvs.push_back( texCoord );

                if( i != numSegHeight )
                {
                    indices.push_back( offset + numSegBase + 1 );
                    indices.push_back( offset );
                    indices.push_back( offset + numSegBase );
                    indices.push_back( offset + numSegBase + 1 );
                    indices.push_back( offset + 1 );
                    indices.push_back( offset );
                }

                offset++;
            }
        }

        return MeshUtil::createMesh( vertices, normals, uvs, indices );
    }

    auto MeshUtil::createRoadFromSpline( SmartPtr<LinearSpline3<real_Num>> spline, f32 radius,
                                         f32 height, u32 numSegBase, u32 numSegHeight, bool capped )
        -> SmartPtr<IMesh>
    {
        Array<Vector3<real_Num>> vertices;
        Array<Vector3<real_Num>> normals;
        Array<Vector2<real_Num>> uvs;
        Array<u32> indices;

        vertices.reserve( 12 );
        uvs.reserve( 12 );
        indices.reserve( 12 );

        auto widthA = 5.0f;
        auto widthB = 5.0f;

        auto index = 0;
        auto segments = 8;
        for( size_t i = 0; i < segments; ++i )
        {
            auto d0 = i / segments;
            auto d1 = ( i + 1 ) / segments;

            auto pointA = spline->interpolate( static_cast<real_Num>( d0 ) );
            auto pointB = spline->interpolate( static_cast<real_Num>( d1 ) );

            auto vec = ( pointB - pointA ).normaliseCopy();
            auto tangentA = vec.crossProduct( Vector3<real_Num>::up() );
            auto tangentB = -tangentA;

            auto p0 = pointA - tangentA * widthA;
            auto p1 = pointA + tangentA * widthA;
            auto p2 = pointB + tangentB * widthB;
            auto p3 = pointB - tangentB * widthB;

            vertices.emplace_back( p0.X(), p0.Y(), p0.Z() );
            vertices.emplace_back( p1.X(), p1.Y(), p1.Z() );
            vertices.emplace_back( p2.X(), p2.Y(), p2.Z() );

            vertices.emplace_back( p2.X(), p2.Y(), p2.Z() );
            vertices.emplace_back( p3.X(), p3.Y(), p3.Z() );
            vertices.emplace_back( p0.X(), p0.Y(), p0.Z() );

            normals.push_back( Vector3<real_Num>::up() );
            normals.push_back( Vector3<real_Num>::up() );
            normals.push_back( Vector3<real_Num>::up() );

            normals.push_back( Vector3<real_Num>::up() );
            normals.push_back( Vector3<real_Num>::up() );
            normals.push_back( Vector3<real_Num>::up() );

            const auto segXRatio = 0.0f;
            const auto segYRatio = 0.0f;

            const auto nextSegXRatio = 1.0f;
            const auto nextSegYRatio = 1.0f;

            uvs.emplace_back( segXRatio, segYRatio );
            uvs.emplace_back( nextSegXRatio, segYRatio );
            uvs.emplace_back( nextSegXRatio, nextSegYRatio );

            uvs.emplace_back( nextSegXRatio, nextSegYRatio );
            uvs.emplace_back( segXRatio, nextSegYRatio );
            uvs.emplace_back( segXRatio, segYRatio );

            indices.push_back( index++ );
            indices.push_back( index++ );
            indices.push_back( index++ );

            indices.push_back( index++ );
            indices.push_back( index++ );
            indices.push_back( index++ );
        }

        return MeshUtil::createMesh( vertices, normals, uvs, indices );
    }

    auto MeshUtil::createMeshFromSplines( const Array<LinearSpline3<real_Num>> &splines )
        -> SmartPtr<IMesh>
    {
        // Check if we have enough splines to create a surface
        if( splines.size() < 2 )
        {
            return nullptr;
        }

        // Verify all splines have the same number of points
        auto numPointsPerSpline = splines[0].getNumPoints();
        if( numPointsPerSpline < 2 )
        {
            return nullptr;
        }

        for( size_t i = 1; i < splines.size(); ++i )
        {
            if( splines[i].getNumPoints() != numPointsPerSpline )
            {
                return nullptr;  // All splines must have the same number of points
            }
        }

        Array<Vector3<real_Num>> vertices;
        Array<Vector3<real_Num>> normals;
        Array<Vector2<real_Num>> uvs;
        Array<u32> indices;

        auto numSplines = splines.size();
        auto segments = 16;  // Number of segments along each spline

        // Reserve space for vertices
        vertices.reserve( numSplines * ( segments + 1 ) );
        normals.reserve( numSplines * ( segments + 1 ) );
        uvs.reserve( numSplines * ( segments + 1 ) );

        // Generate vertices by sampling each spline
        for( size_t splineIdx = 0; splineIdx < numSplines; ++splineIdx )
        {
            const auto &spline = splines[splineIdx];

            for( u32 segIdx = 0; segIdx <= (u32)segments; ++segIdx )
            {
                // Parameter along the spline (0 to 1)
                auto t = static_cast<real_Num>( segIdx ) / static_cast<real_Num>( segments );

                // Sample position from spline
                auto position = spline.interpolate( t );
                vertices.push_back( position );

                // Calculate UV coordinates
                auto u = static_cast<real_Num>( splineIdx ) / static_cast<real_Num>( numSplines - 1 );
                auto v = t;
                uvs.emplace_back( u, v );

                // Calculate normal (will be computed properly after generating all vertices)
                normals.push_back( Vector3<real_Num>::up() );
            }
        }

        // Generate indices for triangles connecting the splines
        for( size_t splineIdx = 0; splineIdx < numSplines - 1; ++splineIdx )
        {
            for( u32 segIdx = 0; segIdx < (u32)segments; ++segIdx )
            {
                // Calculate vertex indices for the quad
                auto i0 = splineIdx * ( segments + 1 ) + segIdx;
                auto i1 = splineIdx * ( segments + 1 ) + segIdx + 1;
                auto i2 = ( splineIdx + 1 ) * ( segments + 1 ) + segIdx;
                auto i3 = ( splineIdx + 1 ) * ( segments + 1 ) + segIdx + 1;

                // First triangle of the quad
                indices.push_back( static_cast<u32>( i0 ) );
                indices.push_back( static_cast<u32>( i2 ) );
                indices.push_back( static_cast<u32>( i1 ) );

                // Second triangle of the quad
                indices.push_back( static_cast<u32>( i1 ) );
                indices.push_back( static_cast<u32>( i2 ) );
                indices.push_back( static_cast<u32>( i3 ) );
            }
        }

        // Calculate proper normals using cross product of adjacent edges
        for( size_t splineIdx = 0; splineIdx < numSplines; ++splineIdx )
        {
            for( u32 segIdx = 0; segIdx <= (u32)segments; ++segIdx )
            {
                auto vertIdx = splineIdx * ( segments + 1 ) + segIdx;
                Vector3<real_Num> normal = Vector3<real_Num>::up();

                // Calculate normal using neighboring vertices when possible
                Vector3<real_Num> tangentU, tangentV;
                bool hasValidNormal = false;

                // Try to get tangent along spline direction (V direction)
                if( segIdx > 0 && segIdx < (u32)segments )
                {
                    auto prevIdx = splineIdx * ( segments + 1 ) + segIdx - 1;
                    auto nextIdx = splineIdx * ( segments + 1 ) + segIdx + 1;
                    tangentV = ( vertices[nextIdx] - vertices[prevIdx] ).normaliseCopy();
                    hasValidNormal = true;
                }
                else if( segIdx == 0 && segments > 0 )
                {
                    auto nextIdx = splineIdx * ( segments + 1 ) + segIdx + 1;
                    tangentV = ( vertices[nextIdx] - vertices[vertIdx] ).normaliseCopy();
                    hasValidNormal = true;
                }
                else if( segIdx == segments && segments > 0 )
                {
                    auto prevIdx = splineIdx * ( segments + 1 ) + segIdx - 1;
                    tangentV = ( vertices[vertIdx] - vertices[prevIdx] ).normaliseCopy();
                    hasValidNormal = true;
                }

                // Try to get tangent across splines (U direction)
                if( hasValidNormal && splineIdx > 0 && splineIdx < numSplines - 1 )
                {
                    auto prevSplineIdx = ( splineIdx - 1 ) * ( segments + 1 ) + segIdx;
                    auto nextSplineIdx = ( splineIdx + 1 ) * ( segments + 1 ) + segIdx;
                    tangentU = ( vertices[nextSplineIdx] - vertices[prevSplineIdx] ).normaliseCopy();
                    normal = tangentU.crossProduct( tangentV ).normaliseCopy();
                }
                else if( hasValidNormal && splineIdx == 0 && numSplines > 1 )
                {
                    auto nextSplineIdx = ( splineIdx + 1 ) * ( segments + 1 ) + segIdx;
                    tangentU = ( vertices[nextSplineIdx] - vertices[vertIdx] ).normaliseCopy();
                    normal = tangentU.crossProduct( tangentV ).normaliseCopy();
                }
                else if( hasValidNormal && splineIdx == numSplines - 1 && numSplines > 1 )
                {
                    auto prevSplineIdx = ( splineIdx - 1 ) * ( segments + 1 ) + segIdx;
                    tangentU = ( vertices[vertIdx] - vertices[prevSplineIdx] ).normaliseCopy();
                    normal = tangentU.crossProduct( tangentV ).normaliseCopy();
                }

                normals[vertIdx] = normal;
            }
        }

        return MeshUtil::createMesh( vertices, normals, uvs, indices );
    }

    auto MeshUtil::clean( SmartPtr<ISubMesh> submesh, real_Num weldTolerance ) -> SmartPtr<ISubMesh>
    {
        WP_ASSERT( submesh );
        WP_ASSERT( submesh->isValid() );

        auto cleanSubMesh = workphone::make_ptr<SubMesh>();

        auto vertexBuffer = submesh->getVertexBuffer();
        auto indexBuffer = submesh->getIndexBuffer();

        cleanSubMesh->setVertexBuffer( vertexBuffer->clone() );
        cleanSubMesh->setIndexBuffer( indexBuffer->clone() );

        auto vertexCount = vertexBuffer->getNumVertices();
        auto indexCount = indexBuffer->getNumIndices();

        auto fbVertexDeclaration = vertexBuffer->getVertexDeclaration();
        const auto posElem =
            fbVertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );

        u32 fbVertexSize = fbVertexDeclaration->getSize();
        auto fbVertexDataPtr = static_cast<u8 *>( vertexBuffer->getVertexData() );

        f32 *fbElementData = nullptr;
        f32 *fbElementData2 = nullptr;

        Array<u32> cleanVertices;
        cleanVertices.reserve( vertexCount );

        Array<Pair<u32, u32>> weldedVertices;
        weldedVertices.reserve( vertexCount );

        for( u32 x = 0; x < vertexCount; ++x, fbVertexDataPtr += fbVertexSize )
        {
            auto wasWelded = false;
            for( auto p : weldedVertices )
            {
                if( p.first == x )
                {
                    wasWelded = true;
                }
            }

            if( wasWelded )
            {
                continue;
            }

            posElem->getElementData( fbVertexDataPtr, &fbElementData );
            auto vertexPosition1 =
                Vector3<real_Num>( fbElementData[0], fbElementData[1], fbElementData[2] );

            auto welded = false;
            auto fbVertexDataPtr2 = static_cast<u8 *>( vertexBuffer->getVertexData() );

            for( u32 y = 0; y < vertexCount; ++y, fbVertexDataPtr2 += fbVertexSize )
            {
                if( x == y )
                {
                    continue;
                }

                posElem->getElementData( fbVertexDataPtr2, &fbElementData2 );
                auto vertexPosition2 =
                    Vector3<real_Num>( fbElementData2[0], fbElementData2[1], fbElementData2[2] );

                auto diff = ( vertexPosition2 - vertexPosition1 ).length();
                if( diff < weldTolerance )
                {
                    auto weldedPair = workphone::make_pair( y, x );
                    weldedVertices.push_back( weldedPair );
                    welded = true;
                }
            }

            cleanVertices.push_back( x );
        }

        auto cleanVertexBuffer = cleanSubMesh->getVertexBuffer();
        auto cleanIndexBuffer = cleanSubMesh->getIndexBuffer();

        auto numCleanVertices = cleanVertices.size();
        cleanVertexBuffer->setNumVertices( static_cast<u32>( numCleanVertices ) );

        auto cleanVertexDataPtr = static_cast<f32 *>( vertexBuffer->getVertexData() );

        fbVertexDataPtr = static_cast<u8 *>( vertexBuffer->getVertexData() );
        for( u32 x = 0; x < vertexCount; ++x, fbVertexDataPtr += fbVertexSize )
        {
            if( std::find( cleanVertices.begin(), cleanVertices.end(), x ) != cleanVertices.end() )
            {
                posElem->getElementData( fbVertexDataPtr, &fbElementData );
                auto vertexPosition1 =
                    Vector3<real_Num>( fbElementData[0], fbElementData[1], fbElementData[2] );

                cleanVertexDataPtr[0] = fbElementData[0];
                cleanVertexDataPtr[1] = fbElementData[1];
                cleanVertexDataPtr[2] = fbElementData[2];

                cleanVertexDataPtr += fbVertexSize;
            }
        }

        Array<u32> cleanIndices;
        cleanIndices.reserve( indexCount );

        // bool useWords = (vertexCount >= std::numeric_limits<u16>::max()) ? false : true;

        if( indexBuffer->getIndexType() == IIndexBuffer::Type::IT_32BIT )
        {
            const u32 *fbIndexData = reinterpret_cast<u32 *>( indexBuffer->getIndexData() );

            for( u32 i = 0; i < indexCount; ++i )
            {
                auto index = fbIndexData[i];

                auto wasWelded = false;
                for( auto p : weldedVertices )
                {
                    if( p.first == index )
                    {
                        index = p.second;
                        wasWelded = true;
                    }
                }

                if( wasWelded )
                {
                    continue;
                }

                cleanIndices.push_back( index );
            }

            auto numCleanIndices = cleanIndices.size();
            cleanIndexBuffer->setNumIndices( static_cast<u32>( numCleanIndices ) );

            auto cleanIndexData = reinterpret_cast<u32 *>( cleanIndexBuffer->getIndexData() );
            for( u32 i = 0; i < numCleanIndices; ++i )
            {
                auto index = cleanIndices[i];
                *cleanIndexData++ = index;
            }
        }
        else
        {
            const u16 *fbIndexData = reinterpret_cast<u16 *>( indexBuffer->getIndexData() );

            for( u32 i = 0; i < indexCount; ++i )
            {
                auto index = fbIndexData[i];

                auto wasWelded = false;
                for( auto p : weldedVertices )
                {
                    if( p.first == index )
                    {
                        index = static_cast<u16>( p.second );
                        wasWelded = true;
                    }
                }

                if( wasWelded )
                {
                    continue;
                }

                cleanIndices.push_back( index );
            }

            auto numCleanIndices = cleanIndices.size();
            cleanIndexBuffer->setNumIndices( static_cast<u32>( numCleanIndices ) );

            auto cleanIndexData = reinterpret_cast<u16 *>( cleanIndexBuffer->getIndexData() );
            for( u32 i = 0; i < numCleanIndices; ++i )
            {
                auto index = cleanIndices[i];
                *cleanIndexData++ = static_cast<u16>( index );
            }
        }

        WP_ASSERT( cleanSubMesh );
        WP_ASSERT( cleanSubMesh->isValid() );
        return cleanSubMesh;
    }

    namespace
    {
        Array<Vector3<real_Num>> getPointsFromVertexBuffer( const SmartPtr<IVertexBuffer> &vertexBuffer )
        {
            auto points = Array<Vector3<real_Num>>();
            if( !vertexBuffer )
            {
                return points;
            }

            const auto vertexCount = vertexBuffer->getNumVertices();
            auto vertexDeclaration = vertexBuffer->getVertexDeclaration();
            auto vertexData = static_cast<u8 *>( vertexBuffer->getVertexData() );
            if( vertexCount == 0 || !vertexDeclaration || !vertexData )
            {
                return points;
            }

            auto positionElement =
                vertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
            if( !positionElement )
            {
                return points;
            }

            const auto vertexSize = vertexDeclaration->getSize();
            points.reserve( vertexCount );
            for( u32 vertex = 0; vertex < vertexCount; ++vertex, vertexData += vertexSize )
            {
                f32 *elementData = nullptr;
                positionElement->getElementData( vertexData, &elementData );
                if( !elementData )
                {
                    points.clear();
                    return points;
                }

                points.emplace_back( elementData[0], elementData[1], elementData[2] );
            }
            return points;
        }
    }  // namespace

    auto MeshUtil::getPoints( SmartPtr<IMesh> mesh ) -> Array<Vector3<real_Num>>
    {
        auto points = Array<Vector3<real_Num>>();

        if( !mesh )
        {
            return points;
        }

        auto subMeshes = mesh->getSubMeshes();
        if( subMeshes.empty() )
        {
            return points;
        }

        /*
         * Imported meshes commonly keep one vertex buffer on the mesh and let
         * their submeshes reference it. Reading only submesh-owned buffers made
         * those meshes appear to contain zero vertices to collision cooking.
         */
        if( mesh->getHasSharedVertexData() )
        {
            auto sharedPoints = getPointsFromVertexBuffer( mesh->getSharedVertexBuffer() );
            points.insert( points.end(), sharedPoints.begin(), sharedPoints.end() );
        }

        for( const auto &subMesh : subMeshes )
        {
            if( subMesh && !( mesh->getHasSharedVertexData() && subMesh->getUseSharedVertices() ) )
            {
                auto subMeshPoints = getPointsFromVertexBuffer( subMesh->getVertexBuffer() );
                points.insert( points.end(), subMeshPoints.begin(), subMeshPoints.end() );
            }
        }

        return points;
    }

    auto MeshUtil::getPoints( SmartPtr<ISubMesh> submesh ) -> Array<Vector3<real_Num>>
    {
        WP_ASSERT( submesh );
        return submesh ? getPointsFromVertexBuffer( submesh->getVertexBuffer() )
                       : Array<Vector3<real_Num>>();
    }

    auto MeshUtil::getIndices( SmartPtr<IMesh> mesh ) -> Array<u32>
    {
        auto indices = Array<u32>();

        if( !mesh )
        {
            return indices;
        }

        auto subMeshes = mesh->getSubMeshes();
        if( subMeshes.empty() )
        {
            return indices;
        }

        // Calculate total number of indices to reserve space
        size_t totalIndices = 0;
        for( const auto &subMesh : subMeshes )
        {
            if( subMesh )
            {
                auto indexBuffer = subMesh->getIndexBuffer();
                if( indexBuffer )
                {
                    totalIndices += indexBuffer->getNumIndices();
                }
            }
        }

        indices.reserve( totalIndices );

        // Shared-vertex submesh indices already address the mesh-level buffer.
        u32 vertexOffset = 0;
        if( mesh->getHasSharedVertexData() )
        {
            if( auto sharedVertexBuffer = mesh->getSharedVertexBuffer() )
            {
                vertexOffset = sharedVertexBuffer->getNumVertices();
            }
        }

        // Extract indices from each submesh
        for( const auto &subMesh : subMeshes )
        {
            if( subMesh )
            {
                auto subMeshIndices = getIndices( subMesh );

                const auto usesSharedVertices =
                    mesh->getHasSharedVertexData() && subMesh->getUseSharedVertices();
                for( auto index : subMeshIndices )
                {
                    indices.push_back( usesSharedVertices ? index : index + vertexOffset );
                }

                if( !usesSharedVertices )
                {
                    if( auto vertexBuffer = subMesh->getVertexBuffer() )
                    {
                        vertexOffset += vertexBuffer->getNumVertices();
                    }
                }
            }
        }

        return indices;
    }

    auto MeshUtil::getIndices( SmartPtr<ISubMesh> submesh ) -> Array<u32>
    {
        WP_ASSERT( submesh );

        auto indices = Array<u32>();
        if( !submesh )
        {
            return indices;
        }
        auto fbIndexBuffer = submesh->getIndexBuffer();
        if( !fbIndexBuffer )
        {
            return indices;
        }
        auto fbIndexCount = fbIndexBuffer->getNumIndices();

        indices.reserve( fbIndexCount );

        if( fbIndexBuffer->getIndexType() == IIndexBuffer::Type::IT_32BIT )
        {
            const u32 *fbIndexData = reinterpret_cast<u32 *>( fbIndexBuffer->getIndexData() );

            // for (s32 i = (s32)fbIndexCount - 1; i >= 0; --i)
            for( u32 i = 0; i < fbIndexCount; ++i )
            {
                auto index = fbIndexData[i];
                indices.push_back( index );
            }
        }
        else
        {
            const u16 *fbIndexData = reinterpret_cast<u16 *>( fbIndexBuffer->getIndexData() );
            // for (s32 i = (s32)fbIndexCount - 1; i >= 0; --i)
            for( u32 i = 0; i < fbIndexCount; ++i )
            {
                auto index = fbIndexData[i];
                indices.push_back( index );
            }
        }

        return indices;
    }

    Array<Array<f32>> MeshUtil::meshToHeightMap( SmartPtr<IMesh> mesh, const Vector2I &textureSize )
    {
        Array<Array<f32>> heightMap;

        // Create a 2D array for the height map
        heightMap.resize( textureSize.x );
        for( auto &row : heightMap )
        {
            row.resize( textureSize.y );
        }

        // Get the mesh points
        auto points = getPoints( mesh );
        if( points.empty() )
        {
            return heightMap;
        }

        // Calculate the height values for each point
        for( const auto &point : points )
        {
            // Map the 3D point to the 2D texture coordinates
            int x = static_cast<int>( point.x * textureSize.x );
            int y = static_cast<int>( point.z * textureSize.y );

            // Clamp the coordinates to the texture size
            x = std::clamp( x, 0, textureSize.x - 1 );
            y = std::clamp( y, 0, textureSize.y - 1 );

            // Set the height value in the height map
            heightMap[x][y] = point.y;
        }

        return heightMap;
    }

    auto MeshUtil::buildMesh( const Array<Vector3<real_Num>> &positions,
                              const Array<Vector2<real_Num>> &uvs, const Array<Vector2<real_Num>> &uvs2 )
        -> SmartPtr<IMesh>
    {
        SmartPtr<IMesh> mesh( new Mesh );
        SmartPtr<ISubMesh> subMesh( new SubMesh );
        mesh->addSubMesh( subMesh );

        auto numVertices = positions.size();

        // create sub mesh data
        SmartPtr<IVertexDeclaration> vertexDeclaration = workphone::make_ptr<VertexDeclaration>();
        vertexDeclaration->addElement( 0, sizeof( Vector3<real_Num> ),
                                       VertexElementSemantic::VES_POSITION,
                                       VertexElementType::VET_FLOAT3 );
        vertexDeclaration->addElement( 0, sizeof( Vector3<real_Num> ), VertexElementSemantic::VES_NORMAL,
                                       VertexElementType::VET_FLOAT3 );
        vertexDeclaration->addElement( 0, sizeof( Vector2<real_Num> ),
                                       VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                       VertexElementType::VET_FLOAT2, 0 );
        vertexDeclaration->addElement( 0, sizeof( Vector2<real_Num> ),
                                       VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                       VertexElementType::VET_FLOAT2, 1 );

        SmartPtr<IVertexBuffer> vertexBuffer( new VertexBuffer );
        vertexBuffer->setVertexDeclaration( vertexDeclaration );

        vertexBuffer->setNumVertices( static_cast<u32>( numVertices ) );
        auto vertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
        f32 *vertexDataPtr = vertexData;

        for( u32 vertIdx = 0; vertIdx < numVertices; ++vertIdx )
        {
            Vector3<real_Num> position = positions[vertIdx];
            *vertexDataPtr++ = position.X();
            *vertexDataPtr++ = position.Y();
            *vertexDataPtr++ = position.Z();

            Vector3<real_Num> normal( 0, 1, 0 );
            *vertexDataPtr++ = normal.X();
            *vertexDataPtr++ = normal.Y();
            *vertexDataPtr++ = normal.Z();

            Vector2<real_Num> uv = uvs[vertIdx];
            *vertexDataPtr++ = uv.X();
            *vertexDataPtr++ = uv.Y();

            Vector2<real_Num> uv2 = uvs2[vertIdx];
            *vertexDataPtr++ = uv2.X();
            *vertexDataPtr++ = uv2.Y();
        }

        auto numIndices = positions.size();
        SmartPtr<IIndexBuffer> indexBuffer( new IndexBuffer );
        indexBuffer->setNumIndices( static_cast<u32>( numIndices ) );
        auto indexDataPtr = static_cast<u32 *>( indexBuffer->createIndexData() );

        for( u32 indexIdx = 0; indexIdx < numIndices; ++indexIdx )
        {
            *indexDataPtr++ = indexIdx;
        }

        subMesh->setVertexBuffer( vertexBuffer );
        subMesh->setIndexBuffer( indexBuffer );
        return mesh;
    }

    auto MeshUtil::buildSubMesh( const Array<Vector3<real_Num>> &positions,
                                 const Array<Vector2<real_Num>> &uvs,
                                 const Array<Vector2<real_Num>> &uvs2 ) -> SmartPtr<ISubMesh>
    {
        SmartPtr<ISubMesh> subMesh( new SubMesh );

        auto numVertices = positions.size();

        // create sub mesh data
        SmartPtr<IVertexDeclaration> vertexDeclaration = workphone::make_ptr<VertexDeclaration>();
        vertexDeclaration->addElement( 0, sizeof( Vector3<real_Num> ),
                                       VertexElementSemantic::VES_POSITION,
                                       VertexElementType::VET_FLOAT3 );
        vertexDeclaration->addElement( 0, sizeof( Vector3<real_Num> ), VertexElementSemantic::VES_NORMAL,
                                       VertexElementType::VET_FLOAT3 );
        vertexDeclaration->addElement( 0, sizeof( Vector2<real_Num> ),
                                       VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                       VertexElementType::VET_FLOAT2, 0 );
        vertexDeclaration->addElement( 0, sizeof( Vector2<real_Num> ),
                                       VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                       VertexElementType::VET_FLOAT2, 1 );

        SmartPtr<IVertexBuffer> vertexBuffer( new VertexBuffer );
        vertexBuffer->setVertexDeclaration( vertexDeclaration );

        vertexBuffer->setNumVertices( static_cast<u32>( numVertices ) );
        auto vertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
        f32 *vertexDataPtr = vertexData;

        for( u32 vertIdx = 0; vertIdx < numVertices; ++vertIdx )
        {
            Vector3<real_Num> position = positions[vertIdx];
            *vertexDataPtr++ = position.X();
            *vertexDataPtr++ = position.Y();
            *vertexDataPtr++ = position.Z();

            Vector3<real_Num> normal( 0, 1, 0 );
            *vertexDataPtr++ = normal.X();
            *vertexDataPtr++ = normal.Y();
            *vertexDataPtr++ = normal.Z();

            Vector2<real_Num> uv = uvs[vertIdx];
            *vertexDataPtr++ = uv.X();
            *vertexDataPtr++ = uv.Y();

            Vector2<real_Num> uv2 = uvs2[vertIdx];
            *vertexDataPtr++ = uv2.X();
            *vertexDataPtr++ = uv2.Y();
        }

        auto numIndices = positions.size();
        SmartPtr<IIndexBuffer> indexBuffer( new IndexBuffer );
        indexBuffer->setNumIndices( static_cast<u32>( numIndices ) );
        auto indexDataPtr = static_cast<u32 *>( indexBuffer->createIndexData() );

        for( u32 indexIdx = 0; indexIdx < numIndices; ++indexIdx )
        {
            *indexDataPtr++ = indexIdx;
        }

        subMesh->setVertexBuffer( vertexBuffer );
        subMesh->setIndexBuffer( indexBuffer );
        return subMesh;
    }

    auto MeshUtil::mergeMeshes( const Array<MeshTransformData> &meshTransformData ) -> SmartPtr<IMesh>
    {
        SmartPtr<IMesh> newMesh( new Mesh );

        for( const auto &transformData : meshTransformData )
        {
            const SmartPtr<IMesh> &curMesh = transformData.Mesh;

            const Array<SmartPtr<ISubMesh>> subMeshes = curMesh->getSubMeshes();
            for( auto subMesh : subMeshes )
            {
                SmartPtr<ISubMesh> newSubMesh = subMesh->clone();
                newMesh->addSubMesh( newSubMesh );

                SmartPtr<IIndexBuffer> indexBuffer = newSubMesh->getIndexBuffer();
                SmartPtr<IVertexBuffer> vertexBuffer = newSubMesh->getVertexBuffer();

                u32 vertexCount = vertexBuffer->getNumVertices();
                // u32 indexCount = indexBuffer->getNumIndices();

                const SmartPtr<IVertexElement> posElem =
                    vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                        VertexElementSemantic::VES_POSITION );
                const SmartPtr<IVertexElement> normalElem =
                    vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                        VertexElementSemantic::VES_NORMAL );
                const SmartPtr<IVertexElement> texCoordElem0 =
                    vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                        VertexElementSemantic::VES_TEXTURE_COORDINATES, 0 );
                const SmartPtr<IVertexElement> texCoordElem1 =
                    vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                        VertexElementSemantic::VES_TEXTURE_COORDINATES, 1 );

                u32 fbVertexSize = vertexBuffer->getVertexDeclaration()->getSize();
                auto fbVertexDataPtr = static_cast<u8 *>( vertexBuffer->getVertexData() );
                f32 *fbElementData = nullptr;

                for( u32 j = 0; j < vertexCount; ++j, fbVertexDataPtr += fbVertexSize )
                {
                    posElem->getElementData( fbVertexDataPtr, &fbElementData );
                    Vector3<real_Num> position( fbElementData[0], fbElementData[1], fbElementData[2] );

                    position = transformData.Scale * position;
                    position = transformData.Orientation * position;
                    position = transformData.Position + position;
                    position.Z() = -position.Z();

                    fbElementData[0] = position.X();
                    fbElementData[1] = position.Y();
                    fbElementData[2] = position.Z();

                    normalElem->getElementData( fbVertexDataPtr, &fbElementData );
                    Vector3<real_Num> normal( fbElementData[0], fbElementData[1], fbElementData[2] );
                    normal = transformData.Orientation * normal;

                    fbElementData[0] = normal.X();
                    fbElementData[1] = normal.Y();
                    fbElementData[2] = normal.Z();

                    if( texCoordElem0 )
                    {
                        texCoordElem0->getElementData( fbVertexDataPtr, &fbElementData );
                        Vector2<real_Num> texCoord0( fbElementData[0], fbElementData[1] );
                        fbElementData[0] = texCoord0.X() * transformData.UVScaleData[0].X();
                        fbElementData[1] = texCoord0.Y() * transformData.UVScaleData[0].Y();
                        fbElementData[0] += transformData.UVOffsets[0].X();
                        fbElementData[1] += transformData.UVOffsets[0].Y();
                    }

                    if( texCoordElem1 )
                    {
                        texCoordElem1->getElementData( fbVertexDataPtr, &fbElementData );
                        Vector2<real_Num> texCoord1( fbElementData[0], fbElementData[1] );
                        texCoord1.X() = MathF::Mod( texCoord1.X(), 1.0f );
                        texCoord1.Y() = MathF::Mod( texCoord1.Y(), 1.0f );
                        if( texCoord1.X() < 0.0f )
                        {
                            texCoord1.X() = 1.0f + texCoord1.X();
                        }

                        if( texCoord1.Y() < 0.0f )
                        {
                            texCoord1.Y() = 1.0f + texCoord1.Y();
                        }

                        fbElementData[0] = texCoord1.X() * transformData.UVScaleData[1].X();
                        fbElementData[1] = texCoord1.Y() * transformData.UVScaleData[1].Y();
                        fbElementData[0] += transformData.UVOffsets[1].X();
                        fbElementData[1] += transformData.UVOffsets[1].Y();

                        // WP_ASSERT_TRUE(fbElementData[0] < 0.0f || fbElementData[0] > 1.0f);
                        // WP_ASSERT_TRUE(fbElementData[1] < 0.0f || fbElementData[1] > 1.0f);
                    }
                }
            }
        }

        return newMesh;
    }

    auto MeshUtil::mergeMeshes( const Array<SmartPtr<IMesh>> &meshes ) -> SmartPtr<IMesh>
    {
        SmartPtr<IMesh> newMesh( new Mesh );

        for( const auto &curMesh : meshes )
        {
            const Array<SmartPtr<ISubMesh>> subMeshes = curMesh->getSubMeshes();
            for( auto subMesh : subMeshes )
            {
                SmartPtr<ISubMesh> newSubMesh = subMesh->clone();
                newMesh->addSubMesh( newSubMesh );

                SmartPtr<IIndexBuffer> indexBuffer = newSubMesh->getIndexBuffer();
                SmartPtr<IVertexBuffer> vertexBuffer = newSubMesh->getVertexBuffer();

                u32 vertexCount = vertexBuffer->getNumVertices();
                // u32 indexCount = indexBuffer->getNumIndices();

                const SmartPtr<IVertexElement> posElem =
                    vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                        VertexElementSemantic::VES_POSITION );
                const SmartPtr<IVertexElement> normalElem =
                    vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                        VertexElementSemantic::VES_NORMAL );

                u32 fbVertexSize = vertexBuffer->getVertexDeclaration()->getSize();
                auto fbVertexDataPtr = static_cast<u8 *>( vertexBuffer->getVertexData() );
                f32 *fbElementData = nullptr;

                for( u32 j = 0; j < vertexCount; ++j, fbVertexDataPtr += fbVertexSize )
                {
                    posElem->getElementData( fbVertexDataPtr, &fbElementData );
                    Vector3<real_Num> position( fbElementData[0], fbElementData[1], fbElementData[2] );
                    fbElementData[0] = position.X();
                    fbElementData[1] = position.Y();
                    fbElementData[2] = position.Z();
                }
            }
        }

        return newMesh;
    }

    auto MeshUtil::mergeMeshes( const Array<SmartPtr<IMesh>> &meshes,
                                const Array<Matrix4<real_Num>> &transformations ) -> SmartPtr<IMesh>
    {
        auto newMesh = workphone::make_ptr<Mesh>();

        if( meshes.size() != transformations.size() )
        {
            WP_LOG_ERROR(
                "MeshUtil::mergeMeshes - error transformations do not match the number of meshes" );
        }

        for( u32 meshIdx = 0; meshIdx < meshes.size() && meshIdx < transformations.size(); ++meshIdx )
        {
            const SmartPtr<IMesh> &curMesh = meshes[meshIdx];
            const Matrix4<real_Num> &transformation = transformations[meshIdx];

            const Array<SmartPtr<ISubMesh>> subMeshes = curMesh->getSubMeshes();
            for( auto subMesh : subMeshes )
            {
                SmartPtr<ISubMesh> newSubMesh = subMesh->clone();
                newMesh->addSubMesh( newSubMesh );

                SmartPtr<IIndexBuffer> indexBuffer = newSubMesh->getIndexBuffer();
                SmartPtr<IVertexBuffer> vertexBuffer = newSubMesh->getVertexBuffer();

                u32 vertexCount = vertexBuffer->getNumVertices();
                // u32 indexCount = indexBuffer->getNumIndices();

                const SmartPtr<IVertexElement> posElem =
                    vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                        VertexElementSemantic::VES_POSITION );
                // const SmartPtr<IVertexElement> normalElem =
                // vertexBuffer->getVertexDeclaration()->findElementBySemantic(fb::VES_NORMAL);

                u32 fbVertexSize = vertexBuffer->getVertexDeclaration()->getSize();
                auto fbVertexDataPtr = static_cast<u8 *>( vertexBuffer->getVertexData() );
                f32 *fbElementData = nullptr;

                for( u32 j = 0; j < vertexCount; ++j, fbVertexDataPtr += fbVertexSize )
                {
                    posElem->getElementData( fbVertexDataPtr, &fbElementData );
                    Vector3<real_Num> position( fbElementData[0], fbElementData[1], fbElementData[2] );
                    position = transformation * position;

                    fbElementData[0] = position.X();
                    fbElementData[1] = position.Y();
                    fbElementData[2] = position.Z();
                }
            }
        }

        return newMesh;
    }

    auto MeshUtil::mergeSubMeshesByMaterial( SmartPtr<IMesh> mesh ) -> SmartPtr<IMesh>
    {
        using MatMeshMap = std::map<String, Array<SmartPtr<ISubMesh>>>;
        MatMeshMap matMeshMap;

        // sort sub meshes by material name
        Array<SmartPtr<ISubMesh>> subMeshList = mesh->getSubMeshes();
        for( auto subMesh : subMeshList )
        {
            String materialName = subMesh->getMaterialName();

            matMeshMap[materialName].push_back( subMesh );
        }

        auto newMesh = workphone::make_ptr<Mesh>();

        auto it = matMeshMap.begin();
        for( ; it != matMeshMap.end(); ++it )
        {
            u32 numTotalIndices = 0;
            u32 numTotalVertices = 0;

            const Array<SmartPtr<ISubMesh>> &matSubMeshList = it->second;
            for( auto subMesh : matSubMeshList )
            {
                SmartPtr<IVertexBuffer> vertexBuffer = subMesh->getVertexBuffer();
                SmartPtr<IIndexBuffer> indexBuffer = subMesh->getIndexBuffer();

                numTotalVertices += vertexBuffer->getNumVertices();
                numTotalIndices += indexBuffer->getNumIndices();
            }

            // create sub mesh
            auto subMesh = workphone::make_ptr<SubMesh>();
            newMesh->addSubMesh( subMesh );

            subMesh->setMaterialName( it->first );

            // create sub mesh vertex data
            SmartPtr<IVertexDeclaration> vertexDeclaration = workphone::make_ptr<VertexDeclaration>();
            vertexDeclaration->addElement( 0, sizeof( Vector3<real_Num> ),
                                           VertexElementSemantic::VES_POSITION,
                                           VertexElementType::VET_FLOAT3 );
            vertexDeclaration->addElement( 0, sizeof( Vector3<real_Num> ),
                                           VertexElementSemantic::VES_NORMAL,
                                           VertexElementType::VET_FLOAT3 );
            vertexDeclaration->addElement( 0, sizeof( Vector2<real_Num> ),
                                           VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                           VertexElementType::VET_FLOAT2, 0 );
            vertexDeclaration->addElement( 0, sizeof( Vector2<real_Num> ),
                                           VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                           VertexElementType::VET_FLOAT2, 1 );

            // a pointer to the vertex data
            f32 *newVertexDataPtr = nullptr;

            {
                auto vertexBuffer = workphone::make_ptr<VertexBuffer>();
                subMesh->setVertexBuffer( vertexBuffer );

                vertexBuffer->setVertexDeclaration( vertexDeclaration );

                vertexBuffer->setNumVertices( numTotalVertices );
                auto newVertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
                newVertexDataPtr = newVertexData;
            }

            // a pointer to the new index buffer
            u32 *newIndexDataPtr = nullptr;
            u32 indexOffset = 0;

            {
                // create sub mesh index data
                auto indexBuffer = workphone::make_ptr<IndexBuffer>();
                subMesh->setIndexBuffer( indexBuffer );

                indexBuffer->setNumIndices( numTotalIndices );
                newIndexDataPtr = static_cast<u32 *>( indexBuffer->createIndexData() );
            }

            // populate new vertex buffer
            for( auto subMesh : matSubMeshList )
            {
                SmartPtr<IVertexBuffer> vertexBuffer = subMesh->getVertexBuffer();
                auto vertexData = static_cast<f32 *>( vertexBuffer->getVertexData() );
                u32 numVerts = vertexBuffer->getNumVertices();
                u32 vertSize = vertexBuffer->getVertexDeclaration()->getSize();
                // u32 totalVertexDataSize = numVerts * vertSize;

                u32 numFloats = numVerts * vertSize / sizeof( f32 );
                for( u32 vertIdx = 0; vertIdx < numFloats; ++vertIdx )
                {
                    *newVertexDataPtr++ = vertexData[vertIdx];
                }

                SmartPtr<IIndexBuffer> indexBuffer = subMesh->getIndexBuffer();
                auto indexData = static_cast<u32 *>( indexBuffer->getIndexData() );
                u32 numIndices = indexBuffer->getNumIndices();

                for( u32 idx = 0; idx < numIndices; ++idx )
                {
                    u32 indexValue = indexData[idx] + indexOffset;
                    // WP_ASSERT_TRUE(!(indexValue < numTotalVertices));

                    *newIndexDataPtr++ = indexValue;
                }

                indexOffset += numVerts;
            }
        }

        return newMesh;
    }

    auto MeshUtil::isMeshValid( SmartPtr<IMesh> mesh ) -> bool
    {
        const Array<SmartPtr<ISubMesh>> subMeshes = mesh->getSubMeshes();
        for( auto subMesh : subMeshes )
        {
            Array<Vertex> vertices;

            SmartPtr<IVertexBuffer> vertexBuffer = subMesh->getVertexBuffer();
            // f32* vertexData = (f32*)vertexBuffer->getVertexData();
            u32 numVerts = vertexBuffer->getNumVertices();
            u32 vertSize = vertexBuffer->getVertexDeclaration()->getSize();
            u32 totalVertexDataSize = numVerts * vertSize;

            vertices.resize( numVerts );

            const SmartPtr<IVertexElement> posElem =
                vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_POSITION );
            const SmartPtr<IVertexElement> normalElem =
                vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_NORMAL );
            const SmartPtr<IVertexElement> texCoordElem0 =
                vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_TEXTURE_COORDINATES, 0 );
            const SmartPtr<IVertexElement> texCoordElem1 =
                vertexBuffer->getVertexDeclaration()->findElementBySemantic(
                    VertexElementSemantic::VES_TEXTURE_COORDINATES, 1 );

            u32 fbVertexSize = vertexBuffer->getVertexDeclaration()->getSize();
            auto fbVertexDataPtr = static_cast<u8 *>( vertexBuffer->getVertexData() );
            f32 *fbElementData = nullptr;

            for( u32 j = 0; j < numVerts; ++j, fbVertexDataPtr += fbVertexSize )
            {
                Vertex vertex;

                posElem->getElementData( fbVertexDataPtr, &fbElementData );
                Vector3<real_Num> position( fbElementData[0], fbElementData[1], fbElementData[2] );
                vertex.position = position;

                normalElem->getElementData( fbVertexDataPtr, &fbElementData );
                Vector3<real_Num> normal( fbElementData[0], fbElementData[1], fbElementData[2] );
                vertex.normal = normal;

                if( texCoordElem0 )
                {
                    texCoordElem0->getElementData( fbVertexDataPtr, &fbElementData );
                    Vector2<real_Num> texCoord0( fbElementData[0], fbElementData[1] );
                    vertex.texCoord = texCoord0;
                }

                if( texCoordElem1 )
                {
                    texCoordElem1->getElementData( fbVertexDataPtr, &fbElementData );
                    Vector2<real_Num> texCoord1( fbElementData[0], fbElementData[1] );
                    vertex.texCoord1 = texCoord1;
                }

                vertices[j] = vertex;
            }

            // check if vertices are finite
            for( auto &vertex : vertices )
            {
                if( !vertex.isFinite() )
                {
                    WP_LOG_ERROR( "Vertex is not finite" );
                }
            }

            SmartPtr<IIndexBuffer> indexBuffer = subMesh->getIndexBuffer();
            const u32 *fbIndexData = reinterpret_cast<u32 *>( indexBuffer->getIndexData() );
            for( u32 i = 0; i < indexBuffer->getNumIndices(); ++i )
            {
                u32 index = fbIndexData[i];
                if( index >= vertices.size() )
                {
                    WP_LOG_ERROR( "Index out of bounds: " + StringUtil::toString( index ) +
                                  ">=" + StringUtil::toString( static_cast<s64>( vertices.size() ) ) );
                }

                Vertex &vertex = vertices[index];
                if( !vertex.isFinite() )
                {
                    WP_LOG_ERROR( "Vertex at index " + StringUtil::toString( index ) + "is not finite" );
                }
            }
        }

        return false;
    }

    auto MeshUtil::getMesh( const Array<f32> &heightData, f32 worldScale, f32 heightScale, u32 tileSize )
        -> SmartPtr<IMesh>
    {
        SmartPtr<IMesh> mesh( new Mesh );
        SmartPtr<ISubMesh> subMesh( new SubMesh );
        mesh->addSubMesh( subMesh );

        // create sub mesh data
        SmartPtr<IVertexDeclaration> vertexDeclaration( new VertexDeclaration );
        vertexDeclaration->addElement( 0, sizeof( Vector3<real_Num> ),
                                       VertexElementSemantic::VES_POSITION,
                                       VertexElementType::VET_FLOAT3 );
        // vertexDeclaration->addElement(sizeof(Vector3<real_Num>), VES_NORMAL,
        // VET_FLOAT3); vertexDeclaration->addElement(sizeof(Vector2<real_Num>),
        // VES_TEXTURE_COORDINATES, VET_FLOAT2, 0);
        // vertexDeclaration->addElement(sizeof(Vector2<real_Num>), VES_TEXTURE_COORDINATES,
        // VET_FLOAT2, 1);

        SmartPtr<IVertexBuffer> vertexBuffer( new VertexBuffer );
        vertexBuffer->setVertexDeclaration( vertexDeclaration );

        u32 numVerticies = tileSize * tileSize;

        vertexBuffer->setNumVertices( numVerticies );
        auto vertexData = static_cast<f32 *>( vertexBuffer->createVertexData() );
        f32 *vertexDataPtr = vertexData;

        u32 numVerticiesAdded = 0;
        const f32 tdSize = 1.0f / static_cast<f32>( tileSize - 1 );
        for( s32 x = 0; x < static_cast<s32>( tileSize ); ++x )
        {
            for( s32 z = 0; z < static_cast<s32>( tileSize ); ++z )
            {
                // s32 indexZ = -z;

                s32 zIdx = ( tileSize - 1 ) - z;
                zIdx = z;

                f32 height = heightData[x + z * tileSize];

                Vector3<real_Num> position;
                position.X() = static_cast<f32>( x );
                position.Y() = height * heightScale;
                position.Z() = static_cast<f32>( z );
                position = position * worldScale;

                *vertexDataPtr++ = position.X();
                *vertexDataPtr++ = position.Y();
                *vertexDataPtr++ = position.Z();

                /*				Vector3<real_Num> normal;
                                normal = Vector3<real_Num>::UNIT_Y;
                                *vertexDataPtr++ = normal.X();
                                *vertexDataPtr++ = normal.Y();
                                *vertexDataPtr++ = normal.Z();

                                Vector2<real_Num> texCoord0;
                                texCoord0.X() = x;
                                texCoord0.Y() = z;
                                *vertexDataPtr++ = texCoord0.X();
                                *vertexDataPtr++ = texCoord0.Y();

                                Vector2<real_Num> texCoord1;
                                texCoord1.X() = (f32)x / (f32)tileSize;
                                texCoord1.Y() = (f32)z / (f32)tileSize;
                                *vertexDataPtr++ = texCoord1.X();
                                *vertexDataPtr++ = texCoord1.Y();*/

                numVerticiesAdded++;
            }
        }

        u32 numIndices = tileSize * tileSize * 6;
        SmartPtr<IIndexBuffer> indexBuffer( new IndexBuffer );
        indexBuffer->setNumIndices( numIndices );
        indexBuffer->setIndexType( IndexBuffer::Type::IT_32BIT );
        auto indexDataPtr = static_cast<u32 *>( indexBuffer->createIndexData() );

        u32 index11;
        u32 index21;
        u32 index12;
        u32 index22;
        s32 step = 6;

        u32 numIndicesSet = 0;
        for( s32 z = 0; z < static_cast<s32>( tileSize ) - 1; z += step )
        {
            for( s32 x = 0; x < static_cast<s32>( tileSize ) - 1; x += step )
            {
                index11 = getIndex( tileSize, x, z );
                index21 = getIndex( tileSize, x + step, z );
                index12 = getIndex( tileSize, x, z + step );
                index22 = getIndex( tileSize, x + step, z + step );

                *indexDataPtr++ = index22;
                *indexDataPtr++ = index11;
                *indexDataPtr++ = index12;

                *indexDataPtr++ = index21;
                *indexDataPtr++ = index11;
                *indexDataPtr++ = index22;

                numIndicesSet += 6;
            }
        }

        // size_t newLength = (tileSize / step) * (tileSize / step) * 2 * 2 * 2;
        indexBuffer->setNumIndices( numIndicesSet );

        subMesh->setVertexBuffer( vertexBuffer );
        subMesh->setIndexBuffer( indexBuffer );

        return mesh;
    }

    auto MeshUtil::getIndex( u32 tileSize, u32 x, u32 z ) -> u32
    {
        return x + z * tileSize;
    }

    void MeshUtil::unshareVertices( SmartPtr<IMesh> mesh )
    {
        if( !mesh )
        {
            WP_LOG_ERROR( "MeshUtil::unshareVertices - mesh is null" );
            return;
        }

        // Check if mesh has shared vertex data
        if( !mesh->getHasSharedVertexData() )
        {
            return;  // Nothing to unshare
        }

        auto sharedVertexBuffer = mesh->getSharedVertexBuffer();
        if( !sharedVertexBuffer )
        {
            WP_LOG_ERROR(
                "MeshUtil::unshareVertices - mesh has shared vertex flag but no shared vertex buffer" );
            return;
        }

        auto subMeshes = mesh->getSubMeshes();
        for( auto &subMesh : subMeshes )
        {
            if( !subMesh || !subMesh->getUseSharedVertices() )
            {
                continue;  // This submesh doesn't use shared vertices
            }

            auto indexBuffer = subMesh->getIndexBuffer();
            if( !indexBuffer )
            {
                WP_LOG_ERROR( "MeshUtil::unshareVertices - submesh has no index buffer" );
                continue;
            }

            // Get indices to determine which vertices are actually used
            auto indices = getIndices( subMesh );
            if( indices.empty() )
            {
                WP_LOG_ERROR( "MeshUtil::unshareVertices - submesh has no indices" );
                continue;
            }

            // Create mapping from old vertex indices to new vertex indices
            std::map<u32, u32> vertexIndexMap;
            Array<u32> usedVertices;

            // Find unique vertices used by this submesh
            for( u32 index : indices )
            {
                if( vertexIndexMap.find( index ) == vertexIndexMap.end() )
                {
                    vertexIndexMap[index] = static_cast<u32>( usedVertices.size() );
                    usedVertices.push_back( index );
                }
            }

            // Create new vertex buffer for this submesh
            auto newVertexBuffer = sharedVertexBuffer->clone();
            if( !newVertexBuffer )
            {
                WP_LOG_ERROR( "MeshUtil::unshareVertices - failed to clone vertex buffer" );
                continue;
            }

            // Set the new vertex count to only the vertices we actually use
            auto numUsedVertices = static_cast<u32>( usedVertices.size() );
            newVertexBuffer->setNumVertices( numUsedVertices );

            // Get vertex data pointers
            auto sharedVertexData = static_cast<u8 *>( sharedVertexBuffer->getVertexData() );
            auto newVertexData = static_cast<u8 *>( newVertexBuffer->createVertexData() );

            if( !sharedVertexData || !newVertexData )
            {
                WP_LOG_ERROR( "MeshUtil::unshareVertices - failed to get vertex data" );
                continue;
            }

            auto vertexDeclaration = sharedVertexBuffer->getVertexDeclaration();
            auto vertexSize = vertexDeclaration->getSize();

            // Copy vertex data for only the vertices we use
            for( size_t i = 0; i < usedVertices.size(); ++i )
            {
                u32 originalVertexIndex = usedVertices[i];
                u8 *srcVertex = sharedVertexData + ( originalVertexIndex * vertexSize );
                u8 *dstVertex = newVertexData + ( i * vertexSize );
                memcpy( dstVertex, srcVertex, vertexSize );
            }

            // Update index buffer to use new vertex indices
            if( indexBuffer->getIndexType() == IIndexBuffer::Type::IT_32BIT )
            {
                auto indexData = static_cast<u32 *>( indexBuffer->getIndexData() );
                auto numIndices = indexBuffer->getNumIndices();

                for( u32 i = 0; i < numIndices; ++i )
                {
                    auto oldIndex = indexData[i];
                    auto it = vertexIndexMap.find( oldIndex );
                    if( it != vertexIndexMap.end() )
                    {
                        indexData[i] = it->second;
                    }
                    else
                    {
                        WP_LOG_ERROR( "MeshUtil::unshareVertices - index not found in vertex map" );
                    }
                }
            }
            else if( indexBuffer->getIndexType() == IIndexBuffer::Type::IT_16BIT )
            {
                auto indexData = static_cast<u16 *>( indexBuffer->getIndexData() );
                auto numIndices = indexBuffer->getNumIndices();

                for( u32 i = 0; i < numIndices; ++i )
                {
                    auto oldIndex = static_cast<u32>( indexData[i] );
                    auto it = vertexIndexMap.find( oldIndex );
                    if( it != vertexIndexMap.end() )
                    {
                        indexData[i] = static_cast<u16>( it->second );
                    }
                    else
                    {
                        WP_LOG_ERROR( "MeshUtil::unshareVertices - index not found in vertex map" );
                    }
                }
            }

            // Set the new vertex buffer for this submesh
            subMesh->setVertexBuffer( newVertexBuffer );
            subMesh->setUseSharedVertices( false );
        }

        // Clear shared vertex data from the mesh
        mesh->setSharedVertexBuffer( nullptr );
        mesh->setHasSharedVertexData( false );
    }

    auto MeshUtil::getTypeCount( VertexElementType etype ) -> u16
    {
        switch( etype )
        {
        case VertexElementType::VET_FLOAT1:
        case VertexElementType::VET_SHORT1:
        case VertexElementType::VET_USHORT1:
        case VertexElementType::VET_UINT1:
        case VertexElementType::VET_INT1:
        case VertexElementType::VET_DOUBLE1:
            return 1;
        case VertexElementType::VET_FLOAT2:
        case VertexElementType::VET_SHORT2:
        case VertexElementType::VET_SHORT2_NORM:
        case VertexElementType::VET_USHORT2:
        case VertexElementType::VET_USHORT2_NORM:
        case VertexElementType::VET_UINT2:
        case VertexElementType::VET_INT2:
        case VertexElementType::VET_DOUBLE2:
            return 2;
        case VertexElementType::VET_FLOAT3:
        case VertexElementType::VET_SHORT3:
        case VertexElementType::VET_USHORT3:
        case VertexElementType::VET_UINT3:
        case VertexElementType::VET_INT3:
        case VertexElementType::VET_DOUBLE3:
            return 3;
        case VertexElementType::VET_FLOAT4:
        case VertexElementType::VET_SHORT4:
        case VertexElementType::VET_SHORT4_NORM:
        case VertexElementType::VET_USHORT4:
        case VertexElementType::VET_USHORT4_NORM:
        case VertexElementType::VET_UINT4:
        case VertexElementType::VET_INT4:
        case VertexElementType::VET_DOUBLE4:
        case VertexElementType::VET_BYTE4:
        case VertexElementType::VET_UBYTE4:
        case VertexElementType::VET_BYTE4_NORM:
        case VertexElementType::VET_UBYTE4_NORM:
        case VertexElementType::_DETAIL_SWAP_RB:
            return 4;
        }

        WP_LOG_ERROR( "VertexElement::getTypeCount: Invalid type" );
        return 0;
    }

    auto MeshUtil::multiplyTypeCount( VertexElementType baseType, u16 count ) -> VertexElementType
    {
        WP_ASSERT( count > 0 && count < 5 );  // Count out of range

        switch( baseType )
        {
        case VertexElementType::VET_FLOAT1:
        case VertexElementType::VET_DOUBLE1:
        case VertexElementType::VET_INT1:
        case VertexElementType::VET_UINT1:
            // evil enumeration arithmetic
            return static_cast<VertexElementType>( static_cast<u16>( baseType ) + count - 1 );

        case VertexElementType::VET_SHORT1:
        case VertexElementType::VET_SHORT2:
            if( count <= 2 )
            {
                return VertexElementType::VET_SHORT2;
            }
            return VertexElementType::VET_SHORT4;

        case VertexElementType::VET_USHORT1:
        case VertexElementType::VET_USHORT2:
            if( count <= 2 )
            {
                return VertexElementType::VET_USHORT2;
            }
            return VertexElementType::VET_USHORT4;

        case VertexElementType::VET_SHORT2_NORM:
            if( count <= 2 )
            {
                return VertexElementType::VET_SHORT2_NORM;
            }
            return VertexElementType::VET_SHORT4_NORM;

        case VertexElementType::VET_USHORT2_NORM:
            if( count <= 2 )
            {
                return VertexElementType::VET_USHORT2_NORM;
            }
            return VertexElementType::VET_USHORT4_NORM;

        case VertexElementType::VET_BYTE4:
        case VertexElementType::VET_BYTE4_NORM:
        case VertexElementType::VET_UBYTE4:
        case VertexElementType::VET_UBYTE4_NORM:
            return baseType;
        default:
            break;
        }

        WP_LOG_ERROR( "VertexElement::multiplyTypeCount: Invalid base type" );
        return static_cast<VertexElementType>( 0 );
    }

    auto MeshUtil::getBaseType( VertexElementType multiType ) -> VertexElementType
    {
        switch( multiType )
        {
        case VertexElementType::VET_FLOAT1:
        case VertexElementType::VET_FLOAT2:
        case VertexElementType::VET_FLOAT3:
        case VertexElementType::VET_FLOAT4:
            return VertexElementType::VET_FLOAT1;
        case VertexElementType::VET_DOUBLE1:
        case VertexElementType::VET_DOUBLE2:
        case VertexElementType::VET_DOUBLE3:
        case VertexElementType::VET_DOUBLE4:
            return VertexElementType::VET_DOUBLE1;
        case VertexElementType::VET_INT1:
        case VertexElementType::VET_INT2:
        case VertexElementType::VET_INT3:
        case VertexElementType::VET_INT4:
            return VertexElementType::VET_INT1;
        case VertexElementType::VET_UINT1:
        case VertexElementType::VET_UINT2:
        case VertexElementType::VET_UINT3:
        case VertexElementType::VET_UINT4:
            return VertexElementType::VET_UINT1;
        case VertexElementType::VET_SHORT1:
        case VertexElementType::VET_SHORT2:
        case VertexElementType::VET_SHORT3:
        case VertexElementType::VET_SHORT4:
            return VertexElementType::VET_SHORT1;
        case VertexElementType::VET_USHORT1:
        case VertexElementType::VET_USHORT2:
        case VertexElementType::VET_USHORT3:
        case VertexElementType::VET_USHORT4:
            return VertexElementType::VET_USHORT1;
        case VertexElementType::VET_SHORT2_NORM:
        case VertexElementType::VET_SHORT4_NORM:
            return VertexElementType::VET_SHORT2_NORM;
        case VertexElementType::VET_USHORT2_NORM:
        case VertexElementType::VET_USHORT4_NORM:
            return VertexElementType::VET_USHORT2_NORM;
        case VertexElementType::VET_BYTE4:
            return VertexElementType::VET_BYTE4;
        case VertexElementType::VET_BYTE4_NORM:
            return VertexElementType::VET_BYTE4_NORM;
        case VertexElementType::VET_UBYTE4:
            return VertexElementType::VET_UBYTE4;
        case VertexElementType::VET_UBYTE4_NORM:
        case VertexElementType::_DETAIL_SWAP_RB:
            return VertexElementType::VET_UBYTE4_NORM;
        }

        // To keep compiler happy
        return VertexElementType::VET_FLOAT1;
    }

    void MeshUtil::convertColourValue( VertexElementType srcType, VertexElementType dstType, u32 *ptr )
    {
        if( srcType == dstType )
        {
            return;
        }

        // Conversion between ARGB and ABGR is always a case of flipping R/B
        *ptr = ( ( *ptr & 0x00FF0000 ) >> 16 ) | ( ( *ptr & 0x000000FF ) << 16 ) | ( *ptr & 0xFF00FF00 );
    }

    auto MeshUtil::getBestColourVertexElementType() -> VertexElementType
    {
        return VertexElementType::VET_UBYTE4_NORM;
    }

    SmartPtr<IMesh> MeshUtil::cloneMesh( SmartPtr<IMesh> mesh )
    {
        if( mesh )
        {
            return mesh->clone();
        }

        return nullptr;
    }

}  // namespace workphone
