#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/IAudioProcessData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAudioProcessData, ISharedObject );

    IAudioProcessData::~IAudioProcessData() = default;
}  // namespace workphone
