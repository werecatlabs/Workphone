#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ISound, IResource );

    const u32 ISound::SOUND_FLAG_3D = 1 << 1;
    const u32 ISound::SOUND_FLAG_STREAM = 1 << 2;
    const u32 ISound::SOUND_FLAG_LOOP = 1 << 3;
    const u32 ISound::SOUND_FLAG_HARDWARE = 1 << 4;
    const u32 ISound::SOUND_FLAG_MUTE = 1 << 5;
    const u32 ISound::SOUND_FLAG_DELETE_WHEN_FINISHED = 1 << 6;
    const u32 ISound::SOUND_FLAG_PAUSED = 1 << 7;
    const u32 ISound::SOUND_FLAG_2D = 1 << 8;
    const u32 ISound::SOUND_FLAG_PLAYING = 1 << 9;

    ISound::ISound() : IResource( ISound::typeInfo() )
    {
    }

    ISound::ISound( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    ISound::~ISound() = default;
}  // namespace workphone
