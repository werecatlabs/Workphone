#ifndef FoliageMesh_h__
#define FoliageMesh_h__

#include <Workphone/Mesh/FoliageSubMesh.hpp>
#include <Workphone/Core/Map.hpp>

namespace workphone
{

    class FoliageMesh : public ISharedObject
    {
    public:
        Map<String, SmartPtr<FoliageSubMesh>> subMeshes;
    };

}  // namespace workphone

#endif  // FoliageMesh_h__
