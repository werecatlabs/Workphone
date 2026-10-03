#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIToggleButton.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>
#include <WorkphoneCore/workphone_toggle.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone, ClawUIToggleButton, ClawUIElement<IUIToggle> );

    ClawUIToggleButton::ClawUIToggleButton() : m_toggleGroup( nullptr ), m_isToggled( false )
    {
        m_type = "ToggleButton";
    }

    ClawUIToggleButton::~ClawUIToggleButton()
    {
        if( m_toggleGroup )
        {
            m_toggleGroup->removeToggleButton( this );
        }
    }

    bool ClawUIToggleButton::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return false;
    }

    void ClawUIToggleButton::setToggled( bool toggled )
    {
        if( m_isToggled == toggled )
        {
            return;
        }

        m_isToggled = toggled;
        m_toggleState = toggled ? ToggleState::On : ToggleState::Off;
        if( toggled && m_toggleGroup )
        {
            m_toggleGroup->OnSetButtonToggled( this );
        }
    }

    bool ClawUIToggleButton::isToggled() const
    {
        return m_isToggled;
    }

    IUIToggle::ToggleType ClawUIToggleButton::getToggleType() const
    {
        return m_toggleType;
    }

    void ClawUIToggleButton::setToggleType( ToggleType toggleType )
    {
        m_toggleType = toggleType;
    }

    IUIToggle::ToggleState ClawUIToggleButton::getToggleState() const
    {
        return m_toggleState;
    }

    void ClawUIToggleButton::setToggleState( ToggleState toggleState )
    {
        m_toggleState = toggleState;
        m_isToggled = toggleState == ToggleState::On;
    }

    bool ClawUIToggleButton::getShowLabel() const
    {
        return m_showLabel;
    }

    void ClawUIToggleButton::setShowLabel( bool showLabel )
    {
        m_showLabel = showLabel;
    }

    String ClawUIToggleButton::getLabel() const
    {
        return m_label;
    }

    void ClawUIToggleButton::setLabel( const String &label )
    {
        m_label = label;
    }

    void ClawUIToggleButton::setTextSize( f32 textSize )
    {
        m_textSize = MathF::max( textSize, 0.0f );
    }

    f32 ClawUIToggleButton::getTextSize() const
    {
        return m_textSize;
    }

    void ClawUIToggleButton::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        auto label = m_showLabel ? m_label : String();
        if( label.empty() && m_showLabel )
        {
            label = "Toggle";
        }

        if( m_toggleType == ToggleType::RadioButton )
        {
            const auto *workphoneLabel = reinterpret_cast<const wp_c8 *>( label.c_str() );
            const auto active = wp_option_label( ctx, workphoneLabel, m_isToggled ? wp_true : wp_false );
            if( active != wp_false && !m_isToggled )
            {
                setToggled( true );
            }
        }
        else
        {
            wp_bool active = m_isToggled ? wp_true : wp_false;
            const auto *workphoneLabel = reinterpret_cast<const wp_c8 *>( label.c_str() );
            if( wp_checkbox_label_align( ctx, workphoneLabel, &active, WORKPHONE_WIDGET_LEFT,
                                         WORKPHONE_TEXT_LEFT ) )
            {
                setToggled( active != wp_false );
            }
        }

        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
