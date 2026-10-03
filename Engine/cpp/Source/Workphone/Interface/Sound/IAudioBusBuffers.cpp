#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/IAudioBusBuffers.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAudioBusBuffers, ISharedObject );

    IAudioBusBuffers::~IAudioBusBuffers() = default;
}  // namespace workphone
