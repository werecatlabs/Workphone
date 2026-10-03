#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ISubMesh, ISharedObject );

    ISubMesh::ISubMesh() : ISharedObject( ISubMesh::typeInfo() )
    {
    }

    ISubMesh::~ISubMesh() = default;
}  // namespace workphone
