#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UISliderCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UILayoutCore.hpp>
#include "WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp"
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>
#include <workphone_layout.h>
#include <workphone_slider.h>
#include <workphone_style.h>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UISliderCore, UIElementCore<UIElement<IUISlider>> );

        namespace
        {
            const String valueStr = "value";
            const String minValueStr = "minValue";
            const String maxValueStr = "maxValue";
            const String directionStr = "direction";
            const String draggingStr = "dragging";
            const String denominatorStr = "denominator";
            const Array<String> directionNames = { "Horizontal", "Vertical" };

            // Style property keys
            const String normalColourStr = "normalColour";
            const String hoverColourStr = "hoverColour";
            const String activeColourStr = "activeColour";
            const String barNormalColourStr = "barNormalColour";
            const String barHoverColourStr = "barHoverColour";
            const String barActiveColourStr = "barActiveColour";
            const String barFilledColourStr = "barFilledColour";
            const String cursorNormalColourStr = "cursorNormalColour";
            const String cursorHoverColourStr = "cursorHoverColour";
            const String cursorActiveColourStr = "cursorActiveColour";
            const String borderColourStr = "borderColour";
            const String borderWidthStr = "borderWidth";
            const String roundingStr = "rounding";
            const String barHeightStr = "barHeight";
            const String paddingStr = "padding";
            const String spacingStr = "spacing";
            const String cursorSizeStr = "cursorSize";
        }

        UISliderCore::UISliderCore()
        {
            createStateContext();
        }

        UISliderCore::~UISliderCore()
        {
            unload( nullptr );
        }

        void UISliderCore::load( SmartPtr<ISharedObject> data )
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

                // WorkphoneCore is immediate-mode — there is no retained widget object.
                // Use a non-null sentinel so guard checks in accessors work correctly.
                m_slider = reinterpret_cast<struct wp_slider *>( this );

                m_value = m_minValue;

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

        void UISliderCore::unload( SmartPtr<ISharedObject> data )
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

                    m_slider = nullptr;
                    UIElementCore<UIElement<IUISlider>>::unload( data );
                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        f32 UISliderCore::getValue() const
        {
            return m_value;
        }

        void UISliderCore::setValue( f32 value )
        {
            m_value = std::clamp( value, m_minValue, m_maxValue );
        }

        f32 UISliderCore::getMinValue() const
        {
            return m_minValue;
        }

        void UISliderCore::setMinValue( f32 minValue )
        {
            m_minValue = minValue;
            m_value = std::clamp( m_value, m_minValue, m_maxValue );
        }

        f32 UISliderCore::getMaxValue() const
        {
            return m_maxValue;
        }

        void UISliderCore::setMaxValue( f32 maxValue )
        {
            m_maxValue = maxValue;
            m_value = std::clamp( m_value, m_minValue, m_maxValue );
        }

        Direction UISliderCore::getDirection() const
        {
            return m_direction;
        }

        void UISliderCore::setDirection( Direction direction )
        {
            m_direction = direction;
        }

        bool UISliderCore::isDragging() const
        {
            return m_dragging;
        }

        void UISliderCore::setDragging( bool dragging )
        {
            m_dragging = dragging;
        }

        void UISliderCore::setSize( const Vector2<real_Num> &size )
        {
            UIElementCore<UIElement<IUISlider>>::setSize( size );
        }

        s32 UISliderCore::getDenominator() const
        {
            return m_denominator;
        }

        void UISliderCore::setDenominator( s32 denominator )
        {
            m_denominator = std::max<s32>( denominator, 1 );
        }

        SmartPtr<Properties> UISliderCore::getProperties() const
        {
            auto properties = UIElementCore<UIElement<IUISlider>>::getProperties();
            properties->setProperty( valueStr, m_value );
            properties->setProperty( minValueStr, m_minValue );
            properties->setProperty( maxValueStr, m_maxValue );
            properties->setPropertyAsEnum( directionStr, static_cast<s32>( m_direction ), directionNames );
            properties->setProperty( draggingStr, m_dragging );
            properties->setProperty( denominatorStr, m_denominator );

            // Style
            properties->setProperty( normalColourStr, m_normalColour );
            properties->setProperty( hoverColourStr, m_hoverColour );
            properties->setProperty( activeColourStr, m_activeColour );
            properties->setProperty( barNormalColourStr, m_barNormalColour );
            properties->setProperty( barHoverColourStr, m_barHoverColour );
            properties->setProperty( barActiveColourStr, m_barActiveColour );
            properties->setProperty( barFilledColourStr, m_barFilledColour );
            properties->setProperty( cursorNormalColourStr, m_cursorNormalColour );
            properties->setProperty( cursorHoverColourStr, m_cursorHoverColour );
            properties->setProperty( cursorActiveColourStr, m_cursorActiveColour );
            properties->setProperty( borderColourStr, m_borderColour );
            properties->setProperty( borderWidthStr, m_borderWidth );
            properties->setProperty( roundingStr, m_rounding );
            properties->setProperty( barHeightStr, m_barHeight );
            properties->setProperty( paddingStr, m_padding );
            properties->setProperty( spacingStr, m_spacing );
            properties->setProperty( cursorSizeStr, m_cursorSize );
            return properties;
        }

        void UISliderCore::setProperties( SmartPtr<Properties> properties )
        {
            UIElementCore<UIElement<IUISlider>>::setProperties( properties );

            properties->getPropertyValue( valueStr, m_value );
            properties->getPropertyValue( minValueStr, m_minValue );
            properties->getPropertyValue( maxValueStr, m_maxValue );

            s32 direction = static_cast<s32>( m_direction );
            if( properties->getPropertyValue( directionStr, direction ) )
            {
                m_direction = static_cast<Direction>( direction );
            }

            properties->getPropertyValue( draggingStr, m_dragging );
            properties->getPropertyValue( denominatorStr, m_denominator );

            if( m_minValue > m_maxValue )
            {
                std::swap( m_minValue, m_maxValue );
            }

            setDenominator( m_denominator );
            setValue( m_value );

            // Style
            properties->getPropertyValue( normalColourStr, m_normalColour );
            properties->getPropertyValue( hoverColourStr, m_hoverColour );
            properties->getPropertyValue( activeColourStr, m_activeColour );
            properties->getPropertyValue( barNormalColourStr, m_barNormalColour );
            properties->getPropertyValue( barHoverColourStr, m_barHoverColour );
            properties->getPropertyValue( barActiveColourStr, m_barActiveColour );
            properties->getPropertyValue( barFilledColourStr, m_barFilledColour );
            properties->getPropertyValue( cursorNormalColourStr, m_cursorNormalColour );
            properties->getPropertyValue( cursorHoverColourStr, m_cursorHoverColour );
            properties->getPropertyValue( cursorActiveColourStr, m_cursorActiveColour );
            properties->getPropertyValue( borderColourStr, m_borderColour );
            properties->getPropertyValue( borderWidthStr, m_borderWidth );
            properties->getPropertyValue( roundingStr, m_rounding );
            properties->getPropertyValue( barHeightStr, m_barHeight );
            properties->getPropertyValue( paddingStr, m_padding );
            properties->getPropertyValue( spacingStr, m_spacing );
            properties->getPropertyValue( cursorSizeStr, m_cursorSize );
        }

        void UISliderCore::update()
        {
            if( !isLoaded() || !isVisible() || !isEnabled() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            auto ui = (UIManagerCore *)applicationManager->getRenderUIPtr();
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

            const f32 step = ( m_maxValue > m_minValue )
                                 ? ( m_maxValue - m_minValue ) / static_cast<f32>( m_denominator )
                                 : 1.0f / static_cast<f32>( m_denominator );

            const f32 prevValue = m_value;

            applyStyle( ctx );

            wp_layout_space_push( ctx, bounds );
            const wp_bool changed = wp_slider_float( ctx, m_minValue, &m_value, m_maxValue, step );

            // Update dragging state from widget state flags.
            m_dragging = ( ctx->last_widget_state & WORKPHONE_WIDGET_STATE_ACTIVE ) != 0;

            if( changed )
            {
                Array<Parameter> args;
                args.push_back( Parameter( m_value ) );

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

            UIElementCore<UIElement<IUISlider>>::update();
        }

        void UISliderCore::applyStyle( struct wp_context *ctx ) const
        {
            if( !ctx )
            {
                return;
            }

            auto &s = ctx->style.slider;

            s.normal = UIUtilCore::solidItem( m_normalColour );
            s.hover = UIUtilCore::solidItem( m_hoverColour );
            s.active = UIUtilCore::solidItem( m_activeColour );
            s.border_color = UIUtilCore::toWpColor( m_borderColour );

            s.bar_normal = UIUtilCore::toWpColor( m_barNormalColour );
            s.bar_hover = UIUtilCore::toWpColor( m_barHoverColour );
            s.bar_active = UIUtilCore::toWpColor( m_barActiveColour );
            s.bar_filled = UIUtilCore::toWpColor( m_barFilledColour );

            s.cursor_normal = UIUtilCore::solidItem( m_cursorNormalColour );
            s.cursor_hover = UIUtilCore::solidItem( m_cursorHoverColour );
            s.cursor_active = UIUtilCore::solidItem( m_cursorActiveColour );

            s.border = m_borderWidth;
            s.rounding = m_rounding;
            s.bar_height = m_barHeight;
            s.padding.x = m_padding.X();
            s.padding.y = m_padding.Y();
            s.spacing.x = m_spacing.X();
            s.spacing.y = m_spacing.Y();
            s.cursor_size.x = m_cursorSize.X();
            s.cursor_size.y = m_cursorSize.Y();
        }

        // ---------------------------------------------------------------
        // Background colour accessors
        // ---------------------------------------------------------------

        ColourF UISliderCore::getNormalColour() const { return m_normalColour; }
        void UISliderCore::setNormalColour( const ColourF &colour ) { m_normalColour = colour; }

        ColourF UISliderCore::getHoverColour() const { return m_hoverColour; }
        void UISliderCore::setHoverColour( const ColourF &colour ) { m_hoverColour = colour; }

        ColourF UISliderCore::getActiveColour() const { return m_activeColour; }
        void UISliderCore::setActiveColour( const ColourF &colour ) { m_activeColour = colour; }

        // ---------------------------------------------------------------
        // Bar colour accessors
        // ---------------------------------------------------------------

        ColourF UISliderCore::getBarNormalColour() const { return m_barNormalColour; }
        void UISliderCore::setBarNormalColour( const ColourF &colour ) { m_barNormalColour = colour; }

        ColourF UISliderCore::getBarHoverColour() const { return m_barHoverColour; }
        void UISliderCore::setBarHoverColour( const ColourF &colour ) { m_barHoverColour = colour; }

        ColourF UISliderCore::getBarActiveColour() const { return m_barActiveColour; }
        void UISliderCore::setBarActiveColour( const ColourF &colour ) { m_barActiveColour = colour; }

        ColourF UISliderCore::getBarFilledColour() const { return m_barFilledColour; }
        void UISliderCore::setBarFilledColour( const ColourF &colour ) { m_barFilledColour = colour; }

        // ---------------------------------------------------------------
        // Cursor colour accessors
        // ---------------------------------------------------------------

        ColourF UISliderCore::getCursorNormalColour() const { return m_cursorNormalColour; }
        void UISliderCore::setCursorNormalColour( const ColourF &colour ) { m_cursorNormalColour = colour; }

        ColourF UISliderCore::getCursorHoverColour() const { return m_cursorHoverColour; }
        void UISliderCore::setCursorHoverColour( const ColourF &colour ) { m_cursorHoverColour = colour; }

        ColourF UISliderCore::getCursorActiveColour() const { return m_cursorActiveColour; }
        void UISliderCore::setCursorActiveColour( const ColourF &colour ) { m_cursorActiveColour = colour; }

        // ---------------------------------------------------------------
        // Border / shape accessors
        // ---------------------------------------------------------------

        ColourF UISliderCore::getBorderColour() const { return m_borderColour; }
        void UISliderCore::setBorderColour( const ColourF &colour ) { m_borderColour = colour; }

        f32 UISliderCore::getBorderWidth() const { return m_borderWidth; }
        void UISliderCore::setBorderWidth( f32 width ) { m_borderWidth = width; }

        f32 UISliderCore::getRounding() const { return m_rounding; }
        void UISliderCore::setRounding( f32 rounding ) { m_rounding = rounding; }

        f32 UISliderCore::getBarHeight() const { return m_barHeight; }
        void UISliderCore::setBarHeight( f32 height ) { m_barHeight = height; }

        // ---------------------------------------------------------------
        // Padding / spacing / cursor size accessors
        // ---------------------------------------------------------------

        Vector2F UISliderCore::getPadding() const { return m_padding; }
        void UISliderCore::setPadding( const Vector2F &padding ) { m_padding = padding; }

        Vector2F UISliderCore::getSpacing() const { return m_spacing; }
        void UISliderCore::setSpacing( const Vector2F &spacing ) { m_spacing = spacing; }

        Vector2F UISliderCore::getCursorSize() const { return m_cursorSize; }
        void UISliderCore::setCursorSize( const Vector2F &size ) { m_cursorSize = size; }

    }  // namespace ui
}  // namespace workphone
