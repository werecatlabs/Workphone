#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IJoystickState.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IJoystickState, ISharedObject );

    IJoystickState::~IJoystickState() = default;

}  // namespace workphone
