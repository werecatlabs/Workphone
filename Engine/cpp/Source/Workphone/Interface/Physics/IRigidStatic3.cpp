#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IRigidStatic3, IRigidBody3 );

    IRigidStatic3::~IRigidStatic3() = default;

}  // namespace workphone::physics
