#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IJoystick.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IJoystick, ISharedObject );

    IJoystick::~IJoystick() = default;

}  // namespace workphone
