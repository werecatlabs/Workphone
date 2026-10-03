#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IDynamicMesh.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IDynamicMesh, IGraphicsObject );

    IDynamicMesh::~IDynamicMesh() = default;

}  // namespace workphone::render
