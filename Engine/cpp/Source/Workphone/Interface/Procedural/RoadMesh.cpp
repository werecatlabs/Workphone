#include "Workphone/WorkphonePCH.hpp"
#include "Workphone/Interface/Procedural/RoadMesh.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace workphone
{
    namespace procedural
    {

        RoadMesh::RoadMesh() = default;

        RoadMesh::~RoadMesh() = default;

        RoadMesh::RoadMesh( const RoadMesh &other ) :
            vertices( makeSourceBudgetSnapshot( other.vertices ) ),
            indices( makeSourceBudgetSnapshot( other.indices ) )
        {
        }

        RoadMesh &RoadMesh::operator=( const RoadMesh &other )
        {
            if( this != &other )
            {
                RoadMesh candidate( other );
                swap( candidate );
            }
            return *this;
        }

        void RoadMesh::swap( RoadMesh &other ) noexcept
        {
            vertices.swap( other.vertices );
            indices.swap( other.indices );
        }

        // ===================================================================
        // RoadMesh helpers
        // ===================================================================
        void RoadMesh::append( const RoadMesh &other, const Vector3<real_Num> &offset )
        {
            const auto vertexCount = other.vertices.size();
            const auto indexCount = other.indices.size();
            if( vertices.size() > std::numeric_limits<u32>::max() ||
                indices.size() > std::numeric_limits<u32>::max() ||
                vertexCount > std::numeric_limits<u32>::max() - vertices.size() ||
                indexCount > std::numeric_limits<u32>::max() - indices.size() )
            {
                throw std::length_error( "Road mesh append exceeds index range" );
            }
            const u32 base = static_cast<u32>( vertices.size() );
            // Admit both buffers before publishing geometry. Bounded indexed reads
            // also permit self-append after reserve relocates the source storage.
            if( vertices.capacity() < vertices.size() + vertexCount )
            {
                vertices.reserve( std::max( vertices.size() + vertexCount,
                    vertices.capacity() + vertices.capacity() / 2 ) );
            }
            if( indices.capacity() < indices.size() + indexCount )
            {
                indices.reserve( std::max( indices.size() + indexCount,
                    indices.capacity() + indices.capacity() / 2 ) );
            }
            for( size_t i = 0; i < vertexCount; ++i )
            {
                RoadVertex nv = other.vertices[i];
                nv.position = nv.position + offset;
                vertices.push_back( nv );
            }
            for( size_t i = 0; i < indexCount; ++i )
            {
                indices.push_back( base + other.indices[i] );
            }
        }

        void RoadMesh::computeNormals()
        {
            if( vertices.empty() )
            {
                return;
            }
            for( auto &v : vertices )
            {
                v.normal = Vector3<real_Num>( 0, 0, 0 );
            }

            for( size_t i = 0; i + 2 < indices.size(); i += 3 )
            {
                u32 a = indices[i];
                u32 b = indices[i + 1];
                u32 c = indices[i + 2];

                Vector3<real_Num> ab = vertices[b].position - vertices[a].position;
                Vector3<real_Num> ac = vertices[c].position - vertices[a].position;
                Vector3<real_Num> n = ab.crossProduct( ac );

                vertices[a].normal = vertices[a].normal + n;
                vertices[b].normal = vertices[b].normal + n;
                vertices[c].normal = vertices[c].normal + n;
            }

            for( auto &v : vertices )
            {
                real_Num l = v.normal.length();
                if( l > 1e-6f )
                {
                    v.normal = v.normal / l;
                }
                else
                {
                    v.normal = Vector3<real_Num>( 0, 1, 0 );
                }
            }
        }

        u32 RoadMesh::getVertexCount() const
        {
            return static_cast<u32>( vertices.size() );
        }

        u32 RoadMesh::getIndexCount() const
        {
            return static_cast<u32>( indices.size() );
        }

        u32 RoadMesh::getTriangleCount() const
        {
            return static_cast<u32>( indices.size() / 3 );
        }

        u32 RoadMesh::addVertex( const RoadVertex &v )
        {
            vertices.push_back( v );
            return static_cast<u32>( vertices.size() ) - 1;
        }

        void RoadMesh::addTriangle( u32 a, u32 b, u32 c )
        {
            if( indices.size() > std::numeric_limits<u32>::max() - 3 )
            {
                throw std::length_error( "Road triangle exceeds index range" );
            }
            if( indices.capacity() - indices.size() < 3 )
            {
                indices.reserve( std::max( indices.size() + 3,
                    indices.capacity() + indices.capacity() / 2 ) );
            }
            indices.push_back( a );
            indices.push_back( b );
            indices.push_back( c );
        }

        void RoadMesh::clear()
        {
            vertices.clear();
            indices.clear();
        }

    }  // namespace procedural
}  // namespace workphone
