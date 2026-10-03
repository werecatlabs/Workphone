#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Physics/ICharacterController2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, ICharacterController2, ISharedObject );

    ICharacterController2::~ICharacterController2() = default;

}  // namespace workphone::physics
