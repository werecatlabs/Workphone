#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsSoftBody3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsSoftBody3, ISharedObject );

    IPhysicsSoftBody3::~IPhysicsSoftBody3() = default;

}  // namespace workphone::physics
