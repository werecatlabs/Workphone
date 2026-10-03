#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIToggleCore.hpp>
#include "WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp"
#include <WPGraphicsOgreNext/UI/Core/UILayoutCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/State/States/UIToggleStateData.hpp>
#include <workphone.h>
#include <workphone_button.h>
#include <workphone_command_buffer.h>
#include <workphone_context.h>
#include <workphone_layout.h>
#include <workphone_style.h>
#include <workphone_text.h>
#include <workphone_toggle.h>
#include <workphone_window.h>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIToggleCore, UIElementCore<UIToggle> );

        namespace
        {
            const String textStr = "text";
            const String textSizeStr = "textSize";
            const String checkedStr = "checked";
            const String showLabelStr = "showLabel";
            const String toggleTypeStr = "toggleType";
            const String toggleStateStr = "toggleState";
            const String normalColourStr = "normalColour";
            const String hoverColourStr = "hoverColour";
            const String activeColourStr = "activeColour";
            const String cursorNormalColourStr = "cursorNormalColour";
            const String cursorHoverColourStr = "cursorHoverColour";
            const String textNormalColourStr = "textNormalColour";
            const String textHoverColourStr = "textHoverColour";
            const String textActiveColourStr = "textActiveColour";
            const String textBackgroundColourStr = "textBackgroundColour";
            const String borderColourStr = "borderColour";
            const String borderWidthStr = "borderWidth";
            const String paddingStr = "padding";
            const String touchPaddingStr = "touchPadding";
            const String spacingStr = "spacing";
            const String switchOffColourStr = "switchOffColour";
            const String switchOnColourStr = "switchOnColour";
            const String switchHoverColourStr = "switchHoverColour";
            const String switchActiveColourStr = "switchActiveColour";
            const String switchTextColourStr = "switchTextColour";
            const String switchBorderColourStr = "switchBorderColour";
            const String switchBorderWidthStr = "switchBorderWidth";
            const String switchRoundingStr = "switchRounding";
            const String switchPaddingStr = "switchPadding";
            const Array<String> toggleTypeNames = { "CheckBox", "RadioButton", "ToggleSwitch" };
            const Array<String> toggleStateNames = { "Off", "On", "Indeterminate" };

            bool isValidToggleType( s32 value )
            {
                return value >= 0 && value < static_cast<s32>( toggleTypeNames.size() );
            }

            bool isValidToggleState( s32 value )
            {
                return value >= 0 && value < static_cast<s32>( toggleStateNames.size() );
            }

            wp_style_button makeSwitchStyle( const UIToggleCore &toggle, wp_bool active )
            {
                auto style = wp_style_button();
                style.normal = UIUtilCore::solidItem(
                    active != wp_false ? toggle.getSwitchOnColour() : toggle.getSwitchOffColour() );
                style.hover = UIUtilCore::solidItem( toggle.getSwitchHoverColour() );
                style.active = UIUtilCore::solidItem( toggle.getSwitchActiveColour() );
                style.border_color = UIUtilCore::toWpColor( toggle.getSwitchBorderColour() );
                style.border = toggle.getSwitchBorderWidth();
                style.rounding = toggle.getSwitchRounding();
                style.padding.x = toggle.getSwitchPadding().X();
                style.padding.y = toggle.getSwitchPadding().Y();
                style.touch_padding.x = toggle.getTouchPadding().X();
                style.touch_padding.y = toggle.getTouchPadding().Y();
                style.text_normal = UIUtilCore::toWpColor( toggle.getSwitchTextColour() );
                style.text_hover = UIUtilCore::toWpColor( toggle.getSwitchTextColour() );
                style.text_active = UIUtilCore::toWpColor( toggle.getSwitchTextColour() );
                style.text_background = UIUtilCore::toWpColor( ColourF( 0.0f, 0.0f, 0.0f, 0.0f ) );
                return style;
            }

            wp_bool drawToggleSwitch( struct wp_context *ctx, const wp_rect &bounds,
                                      const UIToggleCore &toggle, const String &label,
                                      wp_bool active )
            {
                if( !ctx )
                {
                    WP_LOG_ERROR( "Cannot draw toggle switch because the Workphone context is null." );
                    return wp_false;
                }

                const auto height = MathF::max( bounds.h, 1.0f );
                const auto desiredSwitchWidth = MathF::max( height * 1.85f, height + 1.0f );
                const auto switchWidth = MathF::min( MathF::max( desiredSwitchWidth, 1.0f ),
                                                     MathF::max( bounds.w, 1.0f ) );
                auto switchBounds = wp_make_rect( bounds.x, bounds.y, switchWidth, height );

                const auto drawLabel = toggle.getShowLabel() && !label.empty() &&
                                       bounds.w > switchWidth + toggle.getSpacing() + 1.0f;
                auto labelBounds = wp_make_rect( bounds.x + switchWidth + toggle.getSpacing(), bounds.y,
                                                 MathF::max( bounds.w - switchWidth -
                                                                 toggle.getSpacing(),
                                                             0.0f ),
                                                 height );

                auto switchStyle = makeSwitchStyle( toggle, active );
                wp_layout_space_push( ctx, switchBounds );
                const auto changed = wp_button_label_styled( ctx, &switchStyle, "" );

                if( !ctx->current )
                {
                    WP_LOG_ERROR( "Cannot draw toggle switch thumb because there is no current window." );
                    return changed;
                }

                auto *canvas = &ctx->current->buffer;
                const auto horizontalPadding = MathF::min( toggle.getSwitchPadding().X(),
                                                           MathF::max( switchBounds.w * 0.25f, 0.0f ) );
                const auto verticalPadding = MathF::min( toggle.getSwitchPadding().Y(),
                                                         MathF::max( switchBounds.h * 0.25f, 0.0f ) );
                const auto thumbSize =
                    MathF::max( switchBounds.h - verticalPadding * 2.0f, 1.0f );
                const auto offX = switchBounds.x + horizontalPadding;
                const auto onX =
                    switchBounds.x + switchBounds.w - horizontalPadding - thumbSize;
                const auto thumbX = active != wp_false ? onX : offX;
                const auto thumbY = switchBounds.y + ( switchBounds.h - thumbSize ) * 0.5f;
                const auto thumbBounds = wp_make_rect( thumbX, thumbY, thumbSize, thumbSize );

                const auto hovered = ( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_HOVER ) != 0;
                const auto thumbColour = hovered ? toggle.getCursorHoverColour()
                                                 : toggle.getCursorNormalColour();
                wp_fill_circle( canvas, thumbBounds, UIUtilCore::toWpColor( thumbColour ) );

                if( drawLabel )
                {
                    struct wp_text text;
                    text.padding = wp_make_vec2f( 0.0f, 0.0f );
                    text.background = UIUtilCore::toWpColor( toggle.getTextBackgroundColour() );
                    text.text = UIUtilCore::toWpColor( toggle.getTextNormalColour() );

                    wp_widget_text( canvas, labelBounds, label.c_str(),
                                    static_cast<s32>( label.size() ), &text, WORKPHONE_TEXT_LEFT,
                                    ctx->style.font );
                }

                return changed;
            }
        }

        UIToggleCore::UIToggleCore()
        {
            createStateContext();
        }

        UIToggleCore::~UIToggleCore()
        {
            unload( nullptr );
        }

        void UIToggleCore::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                ScopedLock lock( this );

                auto ui =
                    workphone::static_pointer_cast<UIManagerCore>( applicationManager->getRenderUI() );

                auto window = ui->getLayoutWindow();
                if( !window )
                {
                    WP_LOG_ERROR( "Layout window is not available." );
                    setLoadingState( LoadingState::Loaded );
                    return;
                }

                // WorkphoneCore is immediate-mode — no retained widget object.
                // Use a non-null sentinel so guard checks work correctly.
                m_checkbox = reinterpret_cast<struct wp_checkbox_label *>( this );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->setDirty( true );
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIToggleCore::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( isLoaded() )
                {
                    setLoadingState( LoadingState::Unloading );

                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    auto graphicsSystem = applicationManager->getGraphicsSystem();
                    WP_ASSERT( graphicsSystem );

                    ScopedLock lock( this );

                    m_checkbox = nullptr;
                    UIElementCore<UIToggle>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIToggleCore::update()
        {
            if( !isLoaded() || !isVisible() || !isEnabled() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
            {
                return;
            }

            auto ui = workphone::static_pointer_cast<UIManagerCore>( applicationManager->getRenderUI() );
            if( !ui )
            {
                return;
            }

            auto *ctx = ui->getContext();
            if( !ctx )
            {
                return;
            }

            auto position = getPosition();
            auto size = getSize();

            struct wp_rect bounds;
            UIUtilCore::calculateBounds( position, size, &bounds );

            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    m_text = state->label.c_str();
                    m_checked = state->checked;
                    m_textSize = state->textSize;
                    m_showLabel = state->showLabel;

                    const auto toggleType = static_cast<s32>( state->toggleType );
                    if( isValidToggleType( toggleType ) )
                    {
                        m_toggleType = static_cast<ToggleType>( toggleType );
                    }
                    else
                    {
                        WP_LOG_ERROR( "UIToggleCore state has an invalid toggle type." );
                    }

                    const auto toggleState = static_cast<s32>( state->toggleState );
                    if( isValidToggleState( toggleState ) )
                    {
                        m_toggleState = static_cast<ToggleState>( toggleState );
                    }
                    else
                    {
                        WP_LOG_ERROR( "UIToggleCore state has an invalid toggle state." );
                    }
                }
            }

            const auto label =
                reinterpret_cast<const wp_c8 *>( m_showLabel ? m_text.c_str() : "" );
            wp_bool active = m_checked ? wp_true : wp_false;
            wp_bool changed = wp_false;

            applyStyle( ctx );
            wp_layout_space_push( ctx, bounds );

            switch( m_toggleType )
            {
            case ToggleType::RadioButton:
                changed = wp_radio_label_align( ctx, label, &active, WORKPHONE_WIDGET_LEFT,
                                                WORKPHONE_TEXT_LEFT );
                break;

            case ToggleType::ToggleButton:
                if( drawToggleSwitch( ctx, bounds, *this, m_text, active ) )
                {
                    active = active == wp_false ? wp_true : wp_false;
                    changed = wp_true;
                }
                break;

            case ToggleType::CheckBox:
            default:
                changed = wp_checkbox_label_align( ctx, label, &active, WORKPHONE_WIDGET_LEFT,
                                                   WORKPHONE_TEXT_LEFT );
                break;
            }

            if( changed )
            {
                m_checked = ( active != wp_false );
                m_toggleState = m_checked ? ToggleState::On : ToggleState::Off;

                // Keep state data in sync.
                if( auto stateContext = getStateContext() )
                {
                    if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                    {
                        state->checked = m_checked;
                        state->toggleState = static_cast<u8>( m_toggleState );
                    }
                }

                // Notify all registered object listeners.
                Array<Parameter> args;
                args.push_back( Parameter( m_checked ) );

                auto listeners = getObjectListeners();
                for( auto &listener : listeners )
                {
                    if( listener )
                    {
                        listener->handleEvent( EventType::UI, IEvent::CLICK_HASH, args, this, nullptr,
                                               nullptr );
                    }
                }
            }

            UIElementCore<UIToggle>::update();
        }

        void UIToggleCore::setToggled( bool toggled )
        {
            m_checked = toggled;
            m_toggleState = toggled ? ToggleState::On : ToggleState::Off;

            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->checked = m_checked;
                    state->toggleState = static_cast<u8>( m_toggleState );
                }
            }
        }

        bool UIToggleCore::isToggled() const
        {
            return m_checked;
        }

        UIToggleCore::ToggleType UIToggleCore::getToggleType() const
        {
            return m_toggleType;
        }

        void UIToggleCore::setToggleType( ToggleType toggleType )
        {
            const auto value = static_cast<s32>( toggleType );
            if( !isValidToggleType( value ) )
            {
                WP_LOG_ERROR( "Attempted to set an invalid UIToggleCore toggle type." );
                return;
            }

            m_toggleType = toggleType;
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->toggleType = static_cast<u8>( m_toggleType );
                }
            }
        }

        UIToggleCore::ToggleState UIToggleCore::getToggleState() const
        {
            return m_toggleState;
        }

        void UIToggleCore::setToggleState( ToggleState toggleState )
        {
            const auto value = static_cast<s32>( toggleState );
            if( !isValidToggleState( value ) )
            {
                WP_LOG_ERROR( "Attempted to set an invalid UIToggleCore toggle state." );
                return;
            }

            m_toggleState = toggleState;
            m_checked = m_toggleState == ToggleState::On;
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->toggleState = static_cast<u8>( m_toggleState );
                    state->checked = m_checked;
                }
            }
        }

        String UIToggleCore::getLabel() const
        {
            return m_text;
        }

        void UIToggleCore::setLabel( const String &label )
        {
            m_text = label;
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->label = m_text.c_str();
                }
            }
        }

        void UIToggleCore::setTextSize( f32 textSize )
        {
            m_textSize = MathF::max( textSize, 0.0f );
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->textSize = m_textSize;
                }
            }
        }

        f32 UIToggleCore::getTextSize() const
        {
            return m_textSize;
        }

        bool UIToggleCore::getShowLabel() const
        {
            return m_showLabel;
        }

        void UIToggleCore::setShowLabel( bool showLabel )
        {
            m_showLabel = showLabel;
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->showLabel = m_showLabel;
                }
            }
        }

        void UIToggleCore::applyStyle( struct wp_context *ctx ) const
        {
            if( !ctx )
            {
                WP_LOG_ERROR( "Cannot apply toggle style because the Workphone context is null." );
                return;
            }

            auto &toggleStyle =
                m_toggleType == ToggleType::RadioButton ? ctx->style.option : ctx->style.checkbox;
            toggleStyle.normal = UIUtilCore::solidItem( m_normalColour );
            toggleStyle.hover = UIUtilCore::solidItem( m_hoverColour );
            toggleStyle.active = UIUtilCore::solidItem( m_activeColour );
            toggleStyle.cursor_normal = UIUtilCore::solidItem( m_cursorNormalColour );
            toggleStyle.cursor_hover = UIUtilCore::solidItem( m_cursorHoverColour );
            toggleStyle.text_normal = UIUtilCore::toWpColor( m_textNormalColour );
            toggleStyle.text_hover = UIUtilCore::toWpColor( m_textHoverColour );
            toggleStyle.text_active = UIUtilCore::toWpColor( m_textActiveColour );
            toggleStyle.text_background = UIUtilCore::toWpColor( m_textBackgroundColour );
            toggleStyle.border_color = UIUtilCore::toWpColor( m_borderColour );
            toggleStyle.border = m_borderWidth;
            toggleStyle.padding.x = m_padding.X();
            toggleStyle.padding.y = m_padding.Y();
            toggleStyle.touch_padding.x = m_touchPadding.X();
            toggleStyle.touch_padding.y = m_touchPadding.Y();
            toggleStyle.spacing = m_spacing;

            auto &buttonStyle = ctx->style.button;
            buttonStyle.normal =
                UIUtilCore::solidItem( m_checked ? m_switchOnColour : m_switchOffColour );
            buttonStyle.hover = UIUtilCore::solidItem( m_switchHoverColour );
            buttonStyle.active = UIUtilCore::solidItem( m_switchActiveColour );
            buttonStyle.border_color = UIUtilCore::toWpColor( m_switchBorderColour );
            buttonStyle.border = m_switchBorderWidth;
            buttonStyle.rounding = m_switchRounding;
            buttonStyle.padding.x = m_switchPadding.X();
            buttonStyle.padding.y = m_switchPadding.Y();
            buttonStyle.text_normal = UIUtilCore::toWpColor( m_switchTextColour );
            buttonStyle.text_hover = UIUtilCore::toWpColor( m_switchTextColour );
            buttonStyle.text_active = UIUtilCore::toWpColor( m_switchTextColour );
            buttonStyle.text_background =
                UIUtilCore::toWpColor( m_checked ? m_switchOnColour : m_switchOffColour );
        }

        SmartPtr<Properties> UIToggleCore::getProperties() const
        {
            auto properties = UIElementCore<UIToggle>::getProperties();
            properties->setProperty( textStr, m_text );
            properties->setProperty( textSizeStr, m_textSize );
            properties->setProperty( checkedStr, m_checked );
            properties->setProperty( showLabelStr, m_showLabel );
            properties->setPropertyAsEnum( toggleTypeStr, static_cast<s32>( m_toggleType ),
                                           toggleTypeNames );
            properties->setPropertyAsEnum( toggleStateStr, static_cast<s32>( m_toggleState ),
                                           toggleStateNames );
            properties->setProperty( normalColourStr, m_normalColour );
            properties->setProperty( hoverColourStr, m_hoverColour );
            properties->setProperty( activeColourStr, m_activeColour );
            properties->setProperty( cursorNormalColourStr, m_cursorNormalColour );
            properties->setProperty( cursorHoverColourStr, m_cursorHoverColour );
            properties->setProperty( textNormalColourStr, m_textNormalColour );
            properties->setProperty( textHoverColourStr, m_textHoverColour );
            properties->setProperty( textActiveColourStr, m_textActiveColour );
            properties->setProperty( textBackgroundColourStr, m_textBackgroundColour );
            properties->setProperty( borderColourStr, m_borderColour );
            properties->setProperty( borderWidthStr, m_borderWidth );
            properties->setProperty( paddingStr, m_padding );
            properties->setProperty( touchPaddingStr, m_touchPadding );
            properties->setProperty( spacingStr, m_spacing );
            properties->setProperty( switchOffColourStr, m_switchOffColour );
            properties->setProperty( switchOnColourStr, m_switchOnColour );
            properties->setProperty( switchHoverColourStr, m_switchHoverColour );
            properties->setProperty( switchActiveColourStr, m_switchActiveColour );
            properties->setProperty( switchTextColourStr, m_switchTextColour );
            properties->setProperty( switchBorderColourStr, m_switchBorderColour );
            properties->setProperty( switchBorderWidthStr, m_switchBorderWidth );
            properties->setProperty( switchRoundingStr, m_switchRounding );
            properties->setProperty( switchPaddingStr, m_switchPadding );
            return properties;
        }

        void UIToggleCore::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                WP_LOG_ERROR( "UIToggleCore::setProperties received null properties." );
                return;
            }

            UIElementCore<UIToggle>::setProperties( properties );

            properties->getPropertyValue( textStr, m_text );
            properties->getPropertyValue( textSizeStr, m_textSize );
            const auto hasCheckedProperty = properties->getPropertyValue( checkedStr, m_checked );
            properties->getPropertyValue( showLabelStr, m_showLabel );
            properties->getPropertyValue( normalColourStr, m_normalColour );
            properties->getPropertyValue( hoverColourStr, m_hoverColour );
            properties->getPropertyValue( activeColourStr, m_activeColour );
            properties->getPropertyValue( cursorNormalColourStr, m_cursorNormalColour );
            properties->getPropertyValue( cursorHoverColourStr, m_cursorHoverColour );
            properties->getPropertyValue( textNormalColourStr, m_textNormalColour );
            properties->getPropertyValue( textHoverColourStr, m_textHoverColour );
            properties->getPropertyValue( textActiveColourStr, m_textActiveColour );
            properties->getPropertyValue( textBackgroundColourStr, m_textBackgroundColour );
            properties->getPropertyValue( borderColourStr, m_borderColour );
            properties->getPropertyValue( borderWidthStr, m_borderWidth );
            properties->getPropertyValue( paddingStr, m_padding );
            properties->getPropertyValue( touchPaddingStr, m_touchPadding );
            properties->getPropertyValue( spacingStr, m_spacing );
            properties->getPropertyValue( switchOffColourStr, m_switchOffColour );
            properties->getPropertyValue( switchOnColourStr, m_switchOnColour );
            properties->getPropertyValue( switchHoverColourStr, m_switchHoverColour );
            properties->getPropertyValue( switchActiveColourStr, m_switchActiveColour );
            properties->getPropertyValue( switchTextColourStr, m_switchTextColour );
            properties->getPropertyValue( switchBorderColourStr, m_switchBorderColour );
            properties->getPropertyValue( switchBorderWidthStr, m_switchBorderWidth );
            properties->getPropertyValue( switchRoundingStr, m_switchRounding );
            properties->getPropertyValue( switchPaddingStr, m_switchPadding );

            s32 toggleType = static_cast<s32>( m_toggleType );
            if( properties->getPropertyValue( toggleTypeStr, toggleType ) )
            {
                if( isValidToggleType( toggleType ) )
                {
                    m_toggleType = static_cast<ToggleType>( toggleType );
                }
                else
                {
                    WP_LOG_ERROR( "UIToggleCore properties contained an invalid toggle type." );
                }
            }

            s32 toggleState = static_cast<s32>( m_toggleState );
            const auto hasToggleStateProperty =
                properties->getPropertyValue( toggleStateStr, toggleState );
            if( hasToggleStateProperty )
            {
                if( isValidToggleState( toggleState ) )
                {
                    m_toggleState = static_cast<ToggleState>( toggleState );
                }
                else
                {
                    WP_LOG_ERROR( "UIToggleCore properties contained an invalid toggle state." );
                }
            }

            setBorderWidth( m_borderWidth );
            setPadding( m_padding );
            setTouchPadding( m_touchPadding );
            setSpacing( m_spacing );
            setSwitchBorderWidth( m_switchBorderWidth );
            setSwitchRounding( m_switchRounding );
            setSwitchPadding( m_switchPadding );
            setLabel( m_text );
            setTextSize( m_textSize );
            setShowLabel( m_showLabel );
            setToggleType( m_toggleType );
            if( hasToggleStateProperty )
            {
                setToggleState( m_toggleState );
            }
            else if( hasCheckedProperty )
            {
                setToggled( m_checked );
            }
            else
            {
                setToggleState( m_toggleState );
            }
        }

        ColourF UIToggleCore::getNormalColour() const { return m_normalColour; }
        void UIToggleCore::setNormalColour( const ColourF &colour ) { m_normalColour = colour; }

        ColourF UIToggleCore::getHoverColour() const { return m_hoverColour; }
        void UIToggleCore::setHoverColour( const ColourF &colour ) { m_hoverColour = colour; }

        ColourF UIToggleCore::getActiveColour() const { return m_activeColour; }
        void UIToggleCore::setActiveColour( const ColourF &colour ) { m_activeColour = colour; }

        ColourF UIToggleCore::getCursorNormalColour() const { return m_cursorNormalColour; }
        void UIToggleCore::setCursorNormalColour( const ColourF &colour )
        {
            m_cursorNormalColour = colour;
        }

        ColourF UIToggleCore::getCursorHoverColour() const { return m_cursorHoverColour; }
        void UIToggleCore::setCursorHoverColour( const ColourF &colour )
        {
            m_cursorHoverColour = colour;
        }

        ColourF UIToggleCore::getTextNormalColour() const { return m_textNormalColour; }
        void UIToggleCore::setTextNormalColour( const ColourF &colour )
        {
            m_textNormalColour = colour;
        }

        ColourF UIToggleCore::getTextHoverColour() const { return m_textHoverColour; }
        void UIToggleCore::setTextHoverColour( const ColourF &colour )
        {
            m_textHoverColour = colour;
        }

        ColourF UIToggleCore::getTextActiveColour() const { return m_textActiveColour; }
        void UIToggleCore::setTextActiveColour( const ColourF &colour )
        {
            m_textActiveColour = colour;
        }

        ColourF UIToggleCore::getTextBackgroundColour() const { return m_textBackgroundColour; }
        void UIToggleCore::setTextBackgroundColour( const ColourF &colour )
        {
            m_textBackgroundColour = colour;
        }

        ColourF UIToggleCore::getBorderColour() const { return m_borderColour; }
        void UIToggleCore::setBorderColour( const ColourF &colour ) { m_borderColour = colour; }

        f32 UIToggleCore::getBorderWidth() const { return m_borderWidth; }
        void UIToggleCore::setBorderWidth( f32 width )
        {
            m_borderWidth = MathF::max( width, 0.0f );
        }

        Vector2F UIToggleCore::getPadding() const { return m_padding; }
        void UIToggleCore::setPadding( const Vector2F &padding )
        {
            m_padding = Vector2F( MathF::max( padding.X(), 0.0f ),
                                  MathF::max( padding.Y(), 0.0f ) );
        }

        Vector2F UIToggleCore::getTouchPadding() const { return m_touchPadding; }
        void UIToggleCore::setTouchPadding( const Vector2F &touchPadding )
        {
            m_touchPadding = Vector2F( MathF::max( touchPadding.X(), 0.0f ),
                                       MathF::max( touchPadding.Y(), 0.0f ) );
        }

        f32 UIToggleCore::getSpacing() const { return m_spacing; }
        void UIToggleCore::setSpacing( f32 spacing ) { m_spacing = MathF::max( spacing, 0.0f ); }

        ColourF UIToggleCore::getSwitchOffColour() const { return m_switchOffColour; }
        void UIToggleCore::setSwitchOffColour( const ColourF &colour )
        {
            m_switchOffColour = colour;
        }

        ColourF UIToggleCore::getSwitchOnColour() const { return m_switchOnColour; }
        void UIToggleCore::setSwitchOnColour( const ColourF &colour )
        {
            m_switchOnColour = colour;
        }

        ColourF UIToggleCore::getSwitchHoverColour() const { return m_switchHoverColour; }
        void UIToggleCore::setSwitchHoverColour( const ColourF &colour )
        {
            m_switchHoverColour = colour;
        }

        ColourF UIToggleCore::getSwitchActiveColour() const { return m_switchActiveColour; }
        void UIToggleCore::setSwitchActiveColour( const ColourF &colour )
        {
            m_switchActiveColour = colour;
        }

        ColourF UIToggleCore::getSwitchTextColour() const { return m_switchTextColour; }
        void UIToggleCore::setSwitchTextColour( const ColourF &colour )
        {
            m_switchTextColour = colour;
        }

        ColourF UIToggleCore::getSwitchBorderColour() const { return m_switchBorderColour; }
        void UIToggleCore::setSwitchBorderColour( const ColourF &colour )
        {
            m_switchBorderColour = colour;
        }

        f32 UIToggleCore::getSwitchBorderWidth() const { return m_switchBorderWidth; }
        void UIToggleCore::setSwitchBorderWidth( f32 width )
        {
            m_switchBorderWidth = MathF::max( width, 0.0f );
        }

        f32 UIToggleCore::getSwitchRounding() const { return m_switchRounding; }
        void UIToggleCore::setSwitchRounding( f32 rounding )
        {
            m_switchRounding = MathF::max( rounding, 0.0f );
        }

        Vector2F UIToggleCore::getSwitchPadding() const { return m_switchPadding; }
        void UIToggleCore::setSwitchPadding( const Vector2F &padding )
        {
            m_switchPadding = Vector2F( MathF::max( padding.X(), 0.0f ),
                                        MathF::max( padding.Y(), 0.0f ) );
        }
    }  // namespace ui
}  // namespace workphone
