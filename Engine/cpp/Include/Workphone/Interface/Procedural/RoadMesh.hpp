#ifndef RoadMesh_h__
#define RoadMesh_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Interface/Procedural/RoadTypes.hpp>
#include <Workphone/Interface/Procedural/RoadVertex.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    namespace procedural
    {

        /// Dynamic triangle mesh container for road geometry.
        ///
        /// Accumulates RoadVertex / index pairs and exposes flat
        /// vertex / index arrays suitable for GPU upload.
        struct WPCore_API RoadMesh : SourceBudgetObject
        {
            RoadMesh();
            ~RoadMesh();
            RoadMesh( const RoadMesh &other );
            RoadMesh( RoadMesh && ) noexcept = default;
            RoadMesh &operator=( const RoadMesh &other );
            RoadMesh &operator=( RoadMesh && ) noexcept = default;
            void swap( RoadMesh &other ) noexcept;

            /// Append another mesh with a world-space offset.
            void append( const RoadMesh &other, const Vector3<real_Num> &offset );

            /// Recompute per-vertex normals from indexed triangles.
            void computeNormals();

            u32 getVertexCount() const;
            u32 getIndexCount() const;
            u32 getTriangleCount() const;

            u32 addVertex( const RoadVertex &v );

            void addTriangle( u32 a, u32 b, u32 c );

            void clear();

            Array<RoadVertex> vertices{ Allocator<RoadVertex>( sourceBudget ) };
            Array<u32> indices{ Allocator<u32>( sourceBudget ) };
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // RoadMesh_h__
