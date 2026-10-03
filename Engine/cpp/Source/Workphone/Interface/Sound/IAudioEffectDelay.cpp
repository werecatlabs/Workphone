#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/IAudioEffectDelay.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAudioEffectDelay, IAudioEffect );

    IAudioEffectDelay::~IAudioEffectDelay() = default;
}  // namespace workphone
