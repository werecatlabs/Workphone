#ifndef ProceduralMeshFace_h__
#define ProceduralMeshFace_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API MeshFace : public ISharedObject, public SourceBudgetObject
        {
        public:
            using SourceBudgetObject::operator new;
            using SourceBudgetObject::operator delete;
            MeshFace();
            // Copy geometry only; each shared object retains its own identity,
            // reference counts, lifecycle state and attached metadata.
            MeshFace( const MeshFace &other );
            MeshFace &operator=( const MeshFace &other );
            ~MeshFace() override;

            // Stage both complete buffers, including spare capacity, before
            // publishing either. Use this to import caller-owned raw Arrays;
            // direct replacement of a public field can replace its allocator.
            void setGeometry( const Array<Vector3F> &points, const Array<Vector2F> &uvs );

            void unload( SmartPtr<ISharedObject> data ) override;

            Array<Vector3F> Points{ Allocator<Vector3F>( sourceBudget ) };
            Array<Vector2F> UVs{ Allocator<Vector2F>( sourceBudget ) };
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ProceduralMeshFace_h__
