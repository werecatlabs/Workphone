#include "Workphone/WorkphonePCH.hpp"
#include "Workphone/Interface/Procedural/RoadMesh.hpp"
#include <cmath>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {

        RoadMesh::RoadMesh() = default;

        RoadMesh::~RoadMesh() = default;

        // ===================================================================
        // RoadMesh helpers
        // ===================================================================
        void RoadMesh::append( const RoadMesh &other, const Vector3<real_Num> &offset )
        {
            u32 base = static_cast<u32>( vertices.size() );
            for( const auto &v : other.vertices )
            {
                RoadVertex nv = v;
                nv.position = nv.position + offset;
                vertices.push_back( nv );
            }
            for( u32 idx : other.indices )
                indices.push_back( base + idx );
        }

        void RoadMesh::computeNormals()
        {
            if( vertices.empty() )
                return;
            for( auto &v : vertices )
                v.normal = Vector3<real_Num>( 0, 0, 0 );

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
                    v.normal = v.normal / l;
                else
                    v.normal = Vector3<real_Num>( 0, 1, 0 );
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
