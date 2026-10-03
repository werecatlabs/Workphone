#ifndef EdgeData_h__
#define EdgeData_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector4.hpp>

namespace workphone
{
    class EdgeData : public ISharedObject
    {
    public:
        /** Basic triangle structure. */
        struct Triangle
        {
            /** The set of indexes this triangle came from (NB it is possible that the triangles on
               one side of an edge are using a different vertex buffer from those on the other side.)
             */
            size_t indexSet;
            /** The vertex set these vertices came from. */
            size_t vertexSet;
            /// Vertex indexes, relative to the original buffer
            size_t vertIndex[3];
            /** Vertex indexes, relative to a shared vertex buffer with
                duplicates eliminated (this buffer is not exposed) */
            size_t sharedVertIndex[3];

            Triangle();
        };

        /** Edge data. */
        struct Edge
        {
            /** The indexes of the 2 tris attached, note that tri 0 is the one where the
                indexes run _anti_ clockwise along the edge. Indexes must be
                reversed for tri 1. */
            size_t triIndex[2];
            /** The vertex indices for this edge. Note that both vertices will be in the vertex
                set as specified in 'vertexSet', which will also be the same as tri 0 */
            size_t vertIndex[2];
            /** Vertex indices as used in the shared vertex list, not exposed. */
            size_t sharedVertIndex[2];
            /** Indicates if this is a degenerate edge, ie it does not have 2 triangles */
            bool degenerate;
        };

        /** Array of 4D vector of triangle face normal, which is unit vector orthogonal
            to the triangles, plus distance from origin.
            Use aligned policy here because we are intended to use in SIMD optimised routines. */
        using TriangleFaceNormalList = std::vector<Vector4<f32>>;

        /** Working vector used when calculating the silhouette.
            Use std::vector<char> instead of std::vector<bool> which might implemented
            similar bit-fields causing loss performance. */
        using TriangleLightFacingList = std::vector<char>;

        using TriangleList = std::vector<Triangle>;
        using EdgeList = std::vector<Edge>;

        /** A group of edges sharing the same vertex data. */
        struct EdgeGroup
        {
            /** The vertex set index that contains the vertices for this edge group. */
            size_t vertexSet;

            /** Pointer to vertex data used by this edge group. */
            const IVertexBuffer *vertexData;

            /** Index to main triangles array, indicate the first triangle of this edge
                group, and all triangles of this edge group are stored continuous in
                main triangles array.
            */
            size_t triStart;

            /** Number triangles of this edge group. */
            size_t triCount;

            /** The edges themselves. */
            EdgeList edges;
        };

        EdgeData();
        ~EdgeData() override;

        /** Calculate the light facing state of the triangles in this edge list
        @remarks
            This is normally the first stage of calculating a silhouette, i.e.
            establishing which tris are facing the light and which are facing
            away. This state is stored in the 'triangleLightFacings'.
        @param lightPos 4D position of the light in object space, note that
            for directional lights (which have no position), the w component
            is 0 and the x/y/z position are the direction.
        */
        void updateTriangleLightFacing( const Vector4<f32> &lightPos );

        /** Updates the face normals for this edge list based on (changed)
            position information, useful for animated objects.
        @param vertexSet The vertex set we are updating
        @param positionBuffer The updated position buffer, must contain ONLY xyz
        */
        void updateFaceNormals( size_t vertexSet, const SmartPtr<IVertexBuffer> &positionBuffer );

        EdgeData *clone();

        /// Debugging method
        void log( ILogManager *log );

        WP_CLASS_REGISTER_DECL;

        using EdgeGroupList = std::vector<EdgeGroup>;

        /** Main triangles array, stores all triangles of this edge list. Note that
            triangles are grouping against edge group.
        */
        TriangleList triangles;

        /** All triangle face normals. It should be 1:1 with triangles. */
        TriangleFaceNormalList triangleFaceNormals;

        /** Triangle light facing states. It should be 1:1 with triangles. */
        TriangleLightFacingList triangleLightFacings;

        /** All edge groups of this edge list. */
        EdgeGroupList edgeGroups;

        /** Flag indicate the mesh is manifold. */
        bool isClosed;
    };
}  // namespace workphone

#endif  // EdgeData_h__
