#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Sound/SoundEventParam.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>

namespace workphone
{
    SoundEventParam::SoundEventParam() : m_value( 0.0f )
    {
    }

    SoundEventParam::~SoundEventParam() = default;

    f32 SoundEventParam::getValue() const
    {
        return m_value;
    }

    void SoundEventParam::setValue( f32 value )
    {
        m_value = value;
    }

}  // namespace workphone
