#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/IAudioEffectVolume.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAudioEffectVolume, IAudioEffect );

    IAudioEffectVolume::~IAudioEffectVolume() = default;
}  // namespace workphone
