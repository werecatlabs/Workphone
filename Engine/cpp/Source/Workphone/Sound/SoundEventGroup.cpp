#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Sound/SoundEventGroup.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>

namespace workphone
{

    SoundEventGroup::SoundEventGroup() = default;

    SoundEventGroup::~SoundEventGroup() = default;

    void SoundEventGroup::setVolume( f32 volume )
    {
        // Clamp volume to valid range [0.0, 1.0]
        m_volume = std::clamp( volume, 0.0f, 1.0f );
    }

    void SoundEventGroup::setMute( bool mute )
    {
        m_isMuted = mute;
    }

    f32 SoundEventGroup::getVolume() const
    {
        return m_volume;
    }

    bool SoundEventGroup::isMuted() const
    {
        return m_isMuted;
    }

}  // namespace workphone
