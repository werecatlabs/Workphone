#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, IRigidDynamic3, IRigidBody3 );

    IRigidDynamic3::~IRigidDynamic3() = default;

}  // namespace workphone::physics
