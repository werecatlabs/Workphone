#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/UI/ClawUIControls.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone_slider.h>
#include <cmath>
#include <stdexcept>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ClawUIDropdown, ClawUIElement<IUIDropdown> );
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ClawUISlider, ClawUIElement<IUISlider> );

    namespace
    {
        void notifyControlChange( IUIElement *element, hash_type event,
                                  const Array<Parameter> &arguments = {} )
        {
            for( auto listener : element->getObjectListeners() )
            {
                if( listener )
                {
                    listener->handleEvent( EventType::UI, event, arguments, element, nullptr, nullptr );
                }
            }
        }
    }  // namespace

    Array<String> ClawUIDropdown::getOptions() const
    {
        return m_options;
    }

    void ClawUIDropdown::setOptions( const Array<String> &options )
    {
        m_options = options;
        m_selected = m_options.empty() ? 0 : Math<u32>::min( m_selected, static_cast<u32>( m_options.size() - 1 ) );
    }

    void ClawUIDropdown::addOption( const String &option )
    {
        m_options.push_back( option );
    }

    u32 ClawUIDropdown::getSelectedOption() const
    {
        return m_selected;
    }

    void ClawUIDropdown::setSelectedOption( u32 selected )
    {
        m_selected =
            m_options.empty() ? 0 : Math<u32>::min( selected, static_cast<u32>( m_options.size() - 1 ) );
        if( !m_options.empty() )
        {
            setLabel( m_options[m_selected] );
        }
    }

    void ClawUIDropdown::draw( wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }
        const auto bounds = getWorkphoneBounds();
        const auto label = getLabel();
        const auto popupHeight = MathF::max( bounds.h, 24.0f );
        wp_vec2f popupSize = { bounds.w, popupHeight * MathF::min( 8.0f, static_cast<f32>(
                                                                             m_options.size() + 1 ) ) };
        if( wp_combo_begin_label( ctx, reinterpret_cast<const wp_c8 *>( label.c_str() ), popupSize ) )
        {
            wp_layout_row_dynamic( ctx, popupHeight, 1 );
            for( size_t i = 0; i < m_options.size(); ++i )
            {
                if( wp_combo_item_label( ctx, reinterpret_cast<const wp_c8 *>( m_options[i].c_str() ), WORKPHONE_TEXT_LEFT ) )
                {
                    m_selected = static_cast<u32>( i );
                    setLabel( m_options[i] );
                    notifyControlChange( this, IEvent::SELECT_HASH, { Parameter( m_selected ) } );
                }
            }
            wp_combo_end( ctx );
        }
        drawWorkphoneChildren( ctx );
    }

    f32 ClawUISlider::getValue() const
    {
        return m_value;
    }

    void ClawUISlider::setValue( f32 value )
    {
        if( !std::isfinite( value ) )
        {
            throw std::invalid_argument( "Slider value must be finite" );
        }
        m_value = MathF::clamp( value, m_minValue, m_maxValue );
    }

    f32 ClawUISlider::getMinValue() const
    {
        return m_minValue;
    }

    void ClawUISlider::setMinValue( f32 value )
    {
        if( !std::isfinite( value ) )
        {
            throw std::invalid_argument( "Slider minimum must be finite" );
        }
        m_minValue = value;
        m_maxValue = MathF::max( m_maxValue, value );
        setValue( m_value );
    }

    f32 ClawUISlider::getMaxValue() const
    {
        return m_maxValue;
    }

    void ClawUISlider::setMaxValue( f32 value )
    {
        if( !std::isfinite( value ) )
        {
            throw std::invalid_argument( "Slider maximum must be finite" );
        }
        m_maxValue = MathF::max( m_minValue, value );
        setValue( m_value );
    }

    Direction ClawUISlider::getDirection() const
    {
        return m_direction;
    }

    void ClawUISlider::setDirection( Direction direction )
    {
        if( direction != Direction::Horizontal && direction != Direction::Vertical )
        {
            throw std::invalid_argument( "Invalid slider direction" );
        }
        m_direction = direction;
    }

    bool ClawUISlider::isDragging() const
    {
        return m_dragging;
    }

    void ClawUISlider::setDragging( bool dragging )
    {
        m_dragging = dragging;
    }

    void ClawUISlider::draw( wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }
        auto previous = m_value;
        if( m_direction == Direction::Horizontal )
        {
            if( m_maxValue > m_minValue )
            {
                wp_slider_float( ctx, m_minValue, &m_value, m_maxValue,
                                 MathF::max( ( m_maxValue - m_minValue ) / 1000.0f, 0.000001f ) );
                m_dragging = ( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_ACTIVED ) != 0;
            }
            else
            {
                // The backend divides by its range; render a constant value as read-only.
                auto *panel = ctx->current->layout;
                const auto flags = panel->flags;
                panel->flags |= WORKPHONE_WINDOW_ROM;
                float normalizedValue = 0.0f;
                wp_slider_float( ctx, 0.0f, &normalizedValue, 1.0f, 0.001f );
                panel->flags = flags;
                m_dragging = false;
            }
        }
        else
        {
            wp_rect bounds;
            const auto state = wp_widget( &bounds, ctx );
            if( state == WORKPHONE_WIDGET_INVALID )
            {
                m_dragging = false;
                return;
            }
            const auto interactive = state != WORKPHONE_WIDGET_INVALID &&
                                     state != WORKPHONE_WIDGET_ROM &&
                                     state != WORKPHONE_WIDGET_DISABLED &&
                                     !( ctx->current->layout->flags & WORKPHONE_WINDOW_ROM ) &&
                                     m_maxValue > m_minValue;
            const auto down = wp_input_is_mouse_down( &ctx->input, WORKPHONE_BUTTON_LEFT );
            if( !down || !interactive )
            {
                m_dragging = false;
            }
            if( interactive && wp_input_has_mouse_click_down_in_rect( &ctx->input, WORKPHONE_BUTTON_LEFT,
                                                                      bounds, wp_true ) )
            {
                m_dragging = true;
            }
            const auto cursorHeight =
                MathF::min( bounds.h, MathF::max( ctx->style.slider.cursor_size.y, 12.0f ) );
            const auto travel = MathF::max( bounds.h - cursorHeight, 1.0f );
            if( m_dragging )
            {
                const auto ratio = MathF::clamp01(
                    ( ctx->input.mouse.pos.y - bounds.y - cursorHeight * 0.5f ) / travel );
                m_value = m_minValue + ratio * ( m_maxValue - m_minValue );
            }
            const auto range = m_maxValue - m_minValue;
            const auto ratio = range > 0 ? ( m_value - m_minValue ) / range : 0.0f;
            auto *canvas = wp_window_get_canvas( ctx );
            wp_rect bar = { bounds.x + bounds.w * 0.4f, bounds.y + cursorHeight * 0.5f, bounds.w * 0.2f,
                            travel };
            wp_fill_rect( canvas, bar, 0, ctx->style.slider.bar_normal );
            wp_rect cursor = { bounds.x, bounds.y + ratio * travel, bounds.w, cursorHeight };
            const auto &style =
                m_dragging ? ctx->style.slider.cursor_active : ctx->style.slider.cursor_normal;
            if( style.type == WORKPHONE_STYLE_ITEM_COLOR )
            {
                wp_fill_rect( canvas, cursor, ctx->style.slider.rounding, style.data.color );
            }
            else
            {
                wp_draw_image( canvas, cursor, &style.data.image, wp_white );
            }
        }
        if( previous != m_value )
        {
            notifyControlChange( this, IEvent::handleValueChanged, { Parameter( m_value ) } );
        }
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
