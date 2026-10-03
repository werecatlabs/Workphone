#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsSoftBody2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsSoftBody2, ISharedObject );

    IPhysicsSoftBody2::~IPhysicsSoftBody2() = default;

}  // namespace workphone::physics
