#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IVertexBoneAssignment.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IVertexBoneAssignment, ISharedObject );

    IVertexBoneAssignment::~IVertexBoneAssignment() = default;

}  // namespace workphone
