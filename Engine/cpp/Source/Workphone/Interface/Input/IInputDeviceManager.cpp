#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IInputDeviceManager, ISharedObject );

    IInputDeviceManager::~IInputDeviceManager() = default;
}  // namespace workphone
