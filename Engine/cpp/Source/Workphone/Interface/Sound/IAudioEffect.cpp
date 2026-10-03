#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/IAudioEffect.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAudioEffect, ISharedObject );

    IAudioEffect::~IAudioEffect() = default;
}  // namespace workphone
