#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUISpinner.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUISpinner::ClawUISpinner()
    {
        setType( "Spinner" );
        m_valueType = "int";
        m_text = StringUtil::toString( m_value );
    }

    ClawUISpinner::~ClawUISpinner() = default;

    bool ClawUISpinner::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return isEnabled() && ClawUIElement::handleEvent( event );
    }

    void ClawUISpinner::incrementValue()
    {
        m_value = MathD::min( m_value + m_increament, m_maxValue );
        if( m_values.empty() )
        {
            m_text = StringUtil::toString( m_value );
        }
        else
        {
            const auto index = static_cast<size_t>(
                MathD::clamp( m_value, 0.0, static_cast<f64>( m_values.size() - 1 ) ) );
            m_text = m_values[index];
        }
    }

    void ClawUISpinner::decrementValue()
    {
        m_value = MathD::max( m_value - m_increament, m_minValue );
        if( m_values.empty() )
        {
            m_text = StringUtil::toString( m_value );
        }
        else
        {
            const auto index = static_cast<size_t>(
                MathD::clamp( m_value, 0.0, static_cast<f64>( m_values.size() - 1 ) ) );
            m_text = m_values[index];
        }
    }

    void ClawUISpinner::setValue( const String &value )
    {
        m_value = MathD::clamp( StringUtil::parseFloat( value ), m_minValue, m_maxValue );
        m_text = StringUtil::toString( m_value );
    }

    String ClawUISpinner::getValue() const
    {
        return StringUtil::toString( m_value );
    }

    void ClawUISpinner::setMinValue( const String &value )
    {
        m_minValue = StringUtil::parseFloat( value );
        m_value = MathD::max( m_value, m_minValue );
    }

    String ClawUISpinner::getMinValue() const
    {
        return StringUtil::toString( m_minValue );
    }

    void ClawUISpinner::setMaxValue( const String &value )
    {
        m_maxValue = StringUtil::parseFloat( value );
        m_value = MathD::min( m_value, m_maxValue );
    }

    String ClawUISpinner::getMaxValue() const
    {
        return StringUtil::toString( m_maxValue );
    }

    void ClawUISpinner::setText( const String &text )
    {
        m_text = text;
    }

    String ClawUISpinner::getText() const
    {
        return m_text;
    }

    void ClawUISpinner::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        if( !m_values.empty() )
        {
            std::vector<const wp_c8 *> values;
            values.reserve( m_values.size() );
            for( const auto &value : m_values )
            {
                values.push_back( reinterpret_cast<const wp_c8 *>( value.c_str() ) );
            }

            const auto bounds = getWorkphoneBounds();
            wp_vec2f popupSize = { bounds.w, MathF::max( bounds.h * 6.0f, 120.0f ) };
            auto selected = static_cast<wp_s32>(
                MathD::clamp( m_value, 0.0, static_cast<f64>( m_values.size() - 1 ) ) );
            selected = wp_combo( ctx, values.data(), static_cast<wp_s32>( values.size() ), selected,
                                 static_cast<wp_s32>( MathF::max( bounds.h, 1.0f ) ), popupSize );
            m_value = selected;
            m_text = m_values[static_cast<size_t>( selected )];
        }
        else
        {
            auto label = getLabel();
            if( label.empty() )
            {
                label = "Value";
            }

            auto value = m_value;
            wp_property_wp_f64( ctx, reinterpret_cast<const wp_c8 *>( label.c_str() ), m_minValue,
                                &value, m_maxValue, MathD::max( m_increament, 0.0001 ), 1.0f );
            m_value = value;
            m_text = StringUtil::toString( m_value );
        }

        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
