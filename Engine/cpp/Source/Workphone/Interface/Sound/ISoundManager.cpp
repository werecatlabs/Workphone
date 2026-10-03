#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ISoundManager, IResourceManager );

    const u32 ISoundManager::SOUND_FLAG_MUTE = 1 << 1;
    const u32 ISoundManager::SOUND_FLAG_RESET = 1 << 2;
    const u32 ISoundManager::SOUND_FLAG_REALTIME = 1 << 3;

    ISoundManager::~ISoundManager() = default;

}  // namespace workphone
