#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IPhysicsEffect2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IPhysicsEffect2, ISharedObject );

    IPhysicsEffect2::~IPhysicsEffect2() = default;

}  // namespace workphone::physics
