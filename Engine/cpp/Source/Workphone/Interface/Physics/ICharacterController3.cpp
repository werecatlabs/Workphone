#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/ICharacterController3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, ICharacterController3, IPhysicsBody3 );

    ICharacterController3::~ICharacterController3() = default;

}  // namespace workphone::physics
