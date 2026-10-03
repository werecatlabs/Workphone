#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIButtonCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <Workphone/Workphone.hpp>

extern "C" {
#include <workphone.h>
#include <workphone_button.h>
#include <workphone_input.h>
#include <workphone_layout.h>
#include <workphone_window.h>
#include <workphone_types.h>
}

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIButtonCore, UIElementCore<UIButton> );

        namespace
        {
            const String labelStr = "label";
            const String textSizeStr = "textSize";
        }

        UIButtonCore::UIButtonCore()
        {
            createStateContext();
        }

        UIButtonCore::~UIButtonCore()
        {
            unload( nullptr );
        }

        void UIButtonCore::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto ui =
                    workphone::static_pointer_cast<UIManagerCore>( applicationManager->getRenderUI() );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                ScopedLock lock( this );

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

        void UIButtonCore::unload( SmartPtr<ISharedObject> data )
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

                    m_button = nullptr;

                    UIElementCore<UIButton>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIButtonCore::update()
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

            auto label = getLabel();
            auto labelPtr = reinterpret_cast<const c8 *>( label.c_str() );

            applyStyle( ctx );

            wp_layout_space_push( ctx, bounds );
            const bool clicked = wp_button_label( ctx, labelPtr );

            if( clicked )
            {
                Array<Parameter> args;
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

            UIElementCore<UIButton>::update();
        }

        void UIButtonCore::setTextSize( f32 textSize )
        {
            m_textSize = MathF::max( textSize, 0.0f );
            UIButton::setTextSize( m_textSize );
        }

        f32 UIButtonCore::getTextSize() const
        {
            return m_textSize;
        }

        void UIButtonCore::applyStyle( struct wp_context *ctx ) const
        {
            if( !ctx )
            {
                return;
            }

            auto &btn = ctx->style.button;

            btn.normal = UIUtilCore::solidItem( m_normalColour );
            btn.hover = UIUtilCore::solidItem( m_hoverColour );
            btn.active = UIUtilCore::solidItem( m_activeColour );
            btn.border_color = UIUtilCore::toWpColor( m_borderColour );
            btn.border = m_borderWidth;
            btn.rounding = m_rounding;

            btn.text_normal = UIUtilCore::toWpColor( m_textNormalColour );
            btn.text_hover = UIUtilCore::toWpColor( m_textHoverColour );
            btn.text_active = UIUtilCore::toWpColor( m_textActiveColour );
            btn.text_background = UIUtilCore::toWpColor( m_textBackgroundColour );

            btn.padding.x = m_padding.X();
            btn.padding.y = m_padding.Y();
        }

        // ---------------------------------------------------------------
        // Reference canvas
        // ---------------------------------------------------------------

        f32 UIButtonCore::getReferenceWidth() const
        {
            return m_referenceWidth;
        }
        void UIButtonCore::setReferenceWidth( f32 w )
        {
            m_referenceWidth = MathF::max( w, 1.0f );
            setReferenceSize( Vector2F( m_referenceWidth, m_referenceHeight ) );
        }

        f32 UIButtonCore::getReferenceHeight() const
        {
            return m_referenceHeight;
        }
        void UIButtonCore::setReferenceHeight( f32 h )
        {
            m_referenceHeight = MathF::max( h, 1.0f );
            setReferenceSize( Vector2F( m_referenceWidth, m_referenceHeight ) );
        }

        // ---------------------------------------------------------------
        // Text colour accessors
        // ---------------------------------------------------------------

        ColourF UIButtonCore::getTextNormalColour() const
        {
            return m_textNormalColour;
        }
        void UIButtonCore::setTextNormalColour( const ColourF &c )
        {
            m_textNormalColour = c;
        }

        ColourF UIButtonCore::getTextHoverColour() const
        {
            return m_textHoverColour;
        }
        void UIButtonCore::setTextHoverColour( const ColourF &c )
        {
            m_textHoverColour = c;
        }

        ColourF UIButtonCore::getTextActiveColour() const
        {
            return m_textActiveColour;
        }
        void UIButtonCore::setTextActiveColour( const ColourF &c )
        {
            m_textActiveColour = c;
        }

        ColourF UIButtonCore::getTextBackgroundColour() const
        {
            return m_textBackgroundColour;
        }
        void UIButtonCore::setTextBackgroundColour( const ColourF &c )
        {
            m_textBackgroundColour = c;
        }

        // ---------------------------------------------------------------
        // Background colour accessors
        // ---------------------------------------------------------------

        ColourF UIButtonCore::getNormalColour() const
        {
            return m_normalColour;
        }
        void UIButtonCore::setNormalColour( const ColourF &c )
        {
            m_normalColour = c;
        }

        ColourF UIButtonCore::getHoverColour() const
        {
            return m_hoverColour;
        }
        void UIButtonCore::setHoverColour( const ColourF &c )
        {
            m_hoverColour = c;
        }

        ColourF UIButtonCore::getActiveColour() const
        {
            return m_activeColour;
        }
        void UIButtonCore::setActiveColour( const ColourF &c )
        {
            m_activeColour = c;
        }

        // ---------------------------------------------------------------
        // Border accessors
        // ---------------------------------------------------------------

        ColourF UIButtonCore::getBorderColour() const
        {
            return m_borderColour;
        }
        void UIButtonCore::setBorderColour( const ColourF &c )
        {
            m_borderColour = c;
        }

        f32 UIButtonCore::getBorderWidth() const
        {
            return m_borderWidth;
        }
        void UIButtonCore::setBorderWidth( f32 w )
        {
            m_borderWidth = MathF::max( w, 0.0f );
        }

        // ---------------------------------------------------------------
        // Shape accessors
        // ---------------------------------------------------------------

        f32 UIButtonCore::getRounding() const
        {
            return m_rounding;
        }
        void UIButtonCore::setRounding( f32 r )
        {
            m_rounding = MathF::max( r, 0.0f );
        }

        // ---------------------------------------------------------------
        // Padding accessors
        // ---------------------------------------------------------------

        Vector2F UIButtonCore::getPadding() const
        {
            return m_padding;
        }
        void UIButtonCore::setPadding( const Vector2F &p )
        {
            m_padding = Vector2F( MathF::max( p.X(), 0.0f ), MathF::max( p.Y(), 0.0f ) );
        }

        // ---------------------------------------------------------------
        // Properties round-trip
        // ---------------------------------------------------------------

        SmartPtr<Properties> UIButtonCore::getProperties() const
        {
            auto properties = UIElementCore<UIButton>::getProperties();

            properties->setProperty( labelStr, getLabel() );
            properties->setProperty( textSizeStr, getTextSize() );
            properties->setProperty( IUIButton::referenceWidthStr, m_referenceWidth );
            properties->setProperty( IUIButton::referenceHeightStr, m_referenceHeight );
            properties->setProperty( IUIButton::textNormalColourStr, m_textNormalColour );
            properties->setProperty( IUIButton::textHoverColourStr, m_textHoverColour );
            properties->setProperty( IUIButton::textActiveColourStr, m_textActiveColour );
            properties->setProperty( IUIButton::textBackgroundColourStr, m_textBackgroundColour );
            properties->setProperty( IUIButton::normalColourStr, m_normalColour );
            properties->setProperty( IUIButton::hoverColourStr, m_hoverColour );
            properties->setProperty( IUIButton::activeColourStr, m_activeColour );
            properties->setProperty( IUIButton::borderColourStr, m_borderColour );
            properties->setProperty( IUIButton::borderWidthStr, m_borderWidth );
            properties->setProperty( IUIButton::roundingStr, m_rounding );
            properties->setProperty( IUIButton::paddingStr, m_padding );

            return properties;
        }

        void UIButtonCore::setProperties( SmartPtr<Properties> properties )
        {
            UIElementCore<UIButton>::setProperties( properties );

            auto label = getLabel();
            auto textSize = getTextSize();
            auto referenceWidth = m_referenceWidth;
            auto referenceHeight = m_referenceHeight;
            auto textNormalColour = m_textNormalColour;
            auto textHoverColour = m_textHoverColour;
            auto textActiveColour = m_textActiveColour;
            auto textBackgroundColour = m_textBackgroundColour;
            auto normalColour = m_normalColour;
            auto hoverColour = m_hoverColour;
            auto activeColour = m_activeColour;
            auto borderColour = m_borderColour;
            auto borderWidth = m_borderWidth;
            auto rounding = m_rounding;
            auto padding = m_padding;

            properties->getPropertyValue( labelStr, label );
            properties->getPropertyValue( textSizeStr, textSize );
            properties->getPropertyValue( IUIButton::referenceWidthStr, referenceWidth );
            properties->getPropertyValue( IUIButton::referenceHeightStr, referenceHeight );
            properties->getPropertyValue( IUIButton::textNormalColourStr, textNormalColour );
            properties->getPropertyValue( IUIButton::textHoverColourStr, textHoverColour );
            properties->getPropertyValue( IUIButton::textActiveColourStr, textActiveColour );
            properties->getPropertyValue( IUIButton::textBackgroundColourStr, textBackgroundColour );
            properties->getPropertyValue( IUIButton::normalColourStr, normalColour );
            properties->getPropertyValue( IUIButton::hoverColourStr, hoverColour );
            properties->getPropertyValue( IUIButton::activeColourStr, activeColour );
            properties->getPropertyValue( IUIButton::borderColourStr, borderColour );
            properties->getPropertyValue( IUIButton::borderWidthStr, borderWidth );
            properties->getPropertyValue( IUIButton::roundingStr, rounding );
            properties->getPropertyValue( IUIButton::paddingStr, padding );

            setLabel( label );
            setTextSize( textSize );
            setReferenceWidth( referenceWidth );
            setReferenceHeight( referenceHeight );
            setTextNormalColour( textNormalColour );
            setTextHoverColour( textHoverColour );
            setTextActiveColour( textActiveColour );
            setTextBackgroundColour( textBackgroundColour );
            setNormalColour( normalColour );
            setHoverColour( hoverColour );
            setActiveColour( activeColour );
            setBorderColour( borderColour );
            setBorderWidth( borderWidth );
            setRounding( rounding );
            setPadding( padding );
        }

    }  // namespace ui
}  // namespace workphone
