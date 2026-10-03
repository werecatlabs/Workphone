#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralMesh.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IProceduralMesh, ISharedObject );

    IProceduralMesh::~IProceduralMesh() = default;
}  // namespace workphone::procedural
