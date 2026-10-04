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
        else if( m_toggleType == ToggleType::ToggleButton )
        {
            // The scene component exposes ToggleButton as "ToggleSwitch".
            // Consume one widget so the track and label share a single hit target.
            wp_rect bounds;
            const auto state = wp_widget( &bounds, ctx );
            if( state != WORKPHONE_WIDGET_INVALID )
            {
                const auto &style = ctx->style.checkbox;
                const auto *input =
                    state == WORKPHONE_WIDGET_ROM || state == WORKPHONE_WIDGET_DISABLED ||
                            ( ctx->current->layout->flags & WORKPHONE_WINDOW_ROM )
                        ? nullptr
                        : &ctx->input;
                auto hitBounds = bounds;
                hitBounds.x -= style.touch_padding.x;
                hitBounds.y -= style.touch_padding.y;
                hitBounds.w += 2.0f * style.touch_padding.x;
                hitBounds.h += 2.0f * style.touch_padding.y;
                if( wp_button_behavior( &ctx->last_widget_state, hitBounds, input,
                                        WORKPHONE_BUTTON_DEFAULT ) )
                {
                    setToggled( !m_isToggled );
                }

                auto *canvas = wp_window_get_canvas( ctx );
                const auto height = MathF::max(
                    1.0f, MathF::min( ctx->style.font->height,
                                     MathF::min( bounds.h, bounds.w * 0.5f ) ) );
                const wp_rect track = { bounds.x, bounds.y + ( bounds.h - height ) * 0.5f,
                                        height * 2.0f, height };
                const auto hovered = ctx->last_widget_state &
                                     ( WORKPHONE_WIDGET_STATE_HOVER | WORKPHONE_WIDGET_STATE_ACTIVED );
                const auto &background = m_isToggled
                                             ? ( hovered ? style.cursor_hover : style.cursor_normal )
                                             : ( hovered ? style.hover : style.normal );
                if( style.draw_begin )
                {
                    style.draw_begin( canvas, style.userdata );
                }
                if( background.type == WORKPHONE_STYLE_ITEM_COLOR )
                {
                    wp_fill_rect( canvas, track, height * 0.5f,
                                  wp_rgb_factor( background.data.color, style.color_factor ) );
                }
                else
                {
                    wp_draw_image( canvas, track, &background.data.image,
                                   wp_rgb_factor( wp_white, style.color_factor ) );
                }
                const auto inset = MathF::min( 2.0f, height * 0.25f );
                const wp_rect thumb = { track.x + inset + ( m_isToggled ? height : 0.0f ),
                                        track.y + inset, height - 2.0f * inset,
                                        height - 2.0f * inset };
                const auto textColour = hovered ? style.text_hover : style.text_normal;
                wp_fill_circle( canvas, thumb, wp_rgb_factor( textColour, style.color_factor ) );
                if( !label.empty() )
                {
                    const auto labelX = track.x + track.w + style.spacing;
                    const wp_rect textBounds = { labelX, track.y,
                                                 MathF::max( 0.0f, bounds.x + bounds.w - labelX ),
                                                 height };
                    wp_draw_text( canvas, textBounds, label.c_str(), static_cast<int>( label.size() ),
                                  ctx->style.font, style.text_background,
                                  wp_rgb_factor( textColour, style.color_factor ) );
                }
                if( style.draw_end )
                {
                    style.draw_end( canvas, style.userdata );
                }
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
