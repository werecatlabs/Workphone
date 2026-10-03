#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IMesh, ISharedObject );

    IMesh::IMesh() : ISharedObject( IMesh::typeInfo() )
    {
    }

    IMesh::~IMesh() = default;
}  // namespace workphone
