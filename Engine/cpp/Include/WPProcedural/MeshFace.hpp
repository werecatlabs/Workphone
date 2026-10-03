#ifndef ProceduralMeshFace_h__
#define ProceduralMeshFace_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API MeshFace : public ISharedObject
        {
        public:
            MeshFace();
            ~MeshFace() override;

            Array<Vector3F> Points;
            Array<Vector2F> UVs;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ProceduralMeshFace_h__
