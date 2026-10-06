#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/WPSoundEventParam.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPSoundEventParam, SoundEventParam );

    WPSoundEventParam::WPSoundEventParam() : m_value( 0.0f ), m_minValue( 0.0f ), m_maxValue( 1.0f )
    {
    }

    WPSoundEventParam::~WPSoundEventParam() = default;

    f32 WPSoundEventParam::getValue() const
    {
        return m_value;
    }

    void WPSoundEventParam::setValue( f32 value )
    {
        // Clamp the value to the defined range
        m_value = std::max( m_minValue, std::min( value, m_maxValue ) );
    }

    f32 WPSoundEventParam::getMinValue() const
    {
        return m_minValue;
    }

    void WPSoundEventParam::setMinValue( f32 minValue )
    {
        m_minValue = minValue;

        // Ensure current value is still within range
        if( m_value < m_minValue )
        {
            m_value = m_minValue;
        }
    }

    f32 WPSoundEventParam::getMaxValue() const
    {
        return m_maxValue;
    }

    void WPSoundEventParam::setMaxValue( f32 maxValue )
    {
        m_maxValue = maxValue;

        // Ensure current value is still within range
        if( m_value > m_maxValue )
        {
            m_value = m_maxValue;
        }
    }

    void WPSoundEventParam::setValueRange( f32 minValue, f32 maxValue )
    {
        m_minValue = minValue;
        m_maxValue = maxValue;
        // Clamp current value to new range
        m_value = std::max( m_minValue, std::min( m_value, m_maxValue ) );
    }

}  // namespace workphone
