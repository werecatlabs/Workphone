#ifndef FoliageSubMesh_h__
#define FoliageSubMesh_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Mesh/Vertex.hpp>

namespace workphone
{

    class FoliageSubMesh : public ISharedObject
    {
    public:
        Array<FoliageVertex> vertices;
        Array<u32> indices;
    };

}  // namespace workphone

#endif  // FoliageSubMesh_h__
