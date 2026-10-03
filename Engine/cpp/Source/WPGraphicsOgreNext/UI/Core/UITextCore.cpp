#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UITextCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UILayoutCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <WPGraphicsOgreNext/UIRenderer.hpp>
#include <Workphone/Workphone.hpp>

extern "C" {
#include <workphone.h>
#include <workphone_command_buffer.h>
#include <workphone_text.h>
#include <workphone_layout.h>
#include <workphone_types.h>
#include <workphone_window.h>
}

namespace workphone
{
    namespace ui
    {

        WP_CLASS_REGISTER_DERIVED( workphone::ui, UITextCore, UIElementCore<UIText> );

        namespace
        {
            const String drawBackgroundStr = "drawBackground";
            const String backgroundRoundingStr = "backgroundRounding";
            const String leftHorizontalAlignmentStr = "leftHorizontalAlignment";
            const String rightHorizontalAlignmentStr = "rightHorizontalAlignment";
            const String topVerticalAlignmentStr = "topVerticalAlignment";
            const String bottomVerticalAlignmentStr = "bottomVerticalAlignment";
        }  // namespace

        UITextCore::UITextCore()
        {
            createStateContext();
        }

        UITextCore::~UITextCore()
        {
            unload( nullptr );
        }

        void UITextCore::load( SmartPtr<ISharedObject> data )
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

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UITextCore::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( isLoaded() )
                {
                    setLoadingState( LoadingState::Unloading );

                    UIElementCore<ui::UIText>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UITextCore::update()
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

            auto ctx = ui->getContext();
            if( !ctx )
            {
                return;
            }

            auto position = getPosition();
            auto size = getSize();

            struct wp_rect bounds;
            UIUtilCore::calculateBounds( position, size, &bounds );

            auto text = getText();
            auto horizAlign = static_cast<u8>( getHorizontalAlignment() );
            auto vertAlign = static_cast<u8>( getVerticalAlignment() );

            // Map engine alignment (matches UITextStateData defaults: 3 == centered/middle)
            // to Workphone text-alignment flags.
            wp_flags align = WORKPHONE_TEXT_ALIGN_CENTERED;
            if( horizAlign == (u8)HorizontalAlignment::LEFT )
            {
                align = WORKPHONE_TEXT_ALIGN_LEFT;
            }
            else if( horizAlign == (u8)HorizontalAlignment::RIGHT )
            {
                align = WORKPHONE_TEXT_ALIGN_RIGHT;
            }

            if( vertAlign == (u8)VerticalAlignment::TOP )
            {
                align |= WORKPHONE_TEXT_ALIGN_TOP;
            }
            else if( vertAlign == (u8)VerticalAlignment::BOTTOM )
            {
                align |= WORKPHONE_TEXT_ALIGN_BOTTOM;
            }
            else
            {
                align |= WORKPHONE_TEXT_ALIGN_MIDDLE;
            }

            wp_layout_space_push( ctx, bounds );

            if( m_drawBackground )
            {
                if( auto *canvas = wp_window_get_canvas( ctx ) )
                {
                    wp_fill_rect( canvas, bounds, m_backgroundRounding,
                                  UIUtilCore::toWpColor( m_backgroundColour ) );
                }
            }

            auto color = UIUtilCore::toWpColor( m_textColour );

            const auto *rawText = reinterpret_cast<const wp_c8 *>( text.c_str() );
            if( m_textWrap )
            {
                wp_label_colored_wrap( ctx, rawText, color );
            }
            else
            {
                wp_label_colored( ctx, rawText, align, color );
            }

            UIElementCore<UIText>::update();
        }

        bool UITextCore::handleStateChanged( SmartPtr<IState> &state )
        {
            auto retValue = UIElementCore<UIText>::handleStateChanged( state );

            auto stateData = state->getData();
            if( stateData->isDerived<UITextStateData>() )
            {
                auto textStateData = workphone::static_pointer_cast<UITextStateData>( stateData );
                (void)textStateData;

                // Re-push all style fields — a property change may have updated any of them.
                applyStyle();

                retValue = true;
            }

            return retValue;
        }

        SmartPtr<Properties> UITextCore::getProperties() const
        {
            auto properties = UIElementCore<UIText>::getProperties();
            properties->setProperty( IUIText::textPropertyStr, getText() );
            properties->setProperty( IUIText::textSizePropertyStr, getTextSize() );
            properties->setProperty( IUIText::verticalAlignmentPropertyStr, getVerticalAlignment() );
            properties->setProperty( IUIText::horizontalAlignmentPropertyStr, getHorizontalAlignment() );
            properties->setProperty( IUIText::textColourRStr, m_textColour.r );
            properties->setProperty( IUIText::textColourGStr, m_textColour.g );
            properties->setProperty( IUIText::textColourBStr, m_textColour.b );
            properties->setProperty( IUIText::textColourAStr, m_textColour.a );
            properties->setProperty( IUIText::backgroundColourRStr, m_backgroundColour.r );
            properties->setProperty( IUIText::backgroundColourGStr, m_backgroundColour.g );
            properties->setProperty( IUIText::backgroundColourBStr, m_backgroundColour.b );
            properties->setProperty( IUIText::backgroundColourAStr, m_backgroundColour.a );
            properties->setProperty( IUIText::textPaddingXStr, m_textPadding.X() );
            properties->setProperty( IUIText::textPaddingYStr, m_textPadding.Y() );
            properties->setProperty( IUIText::textWrapStr, m_textWrap );
            properties->setProperty( drawBackgroundStr, m_drawBackground );
            properties->setProperty( backgroundRoundingStr, m_backgroundRounding );
            properties->setProperty( leftHorizontalAlignmentStr,
                                     static_cast<s32>( m_leftHorizontalAlignment ) );
            properties->setProperty( rightHorizontalAlignmentStr,
                                     static_cast<s32>( m_rightHorizontalAlignment ) );
            properties->setProperty( topVerticalAlignmentStr,
                                     static_cast<s32>( m_topVerticalAlignment ) );
            properties->setProperty( bottomVerticalAlignmentStr,
                                     static_cast<s32>( m_bottomVerticalAlignment ) );
            return properties;
        }

        void UITextCore::setProperties( SmartPtr<Properties> properties )
        {
            auto text = getText();
            auto textSize = getTextSize();
            auto verticalAlignment = static_cast<u32>( getVerticalAlignment() );
            auto horizontalAlignment = static_cast<u32>( getHorizontalAlignment() );
            auto leftHorizontalAlignment = static_cast<s32>( m_leftHorizontalAlignment );
            auto rightHorizontalAlignment = static_cast<s32>( m_rightHorizontalAlignment );
            auto topVerticalAlignment = static_cast<s32>( m_topVerticalAlignment );
            auto bottomVerticalAlignment = static_cast<s32>( m_bottomVerticalAlignment );

            UIElementCore<UIText>::setProperties( properties );
            properties->getPropertyValue( IUIText::textPropertyStr, text );
            properties->getPropertyValue( IUIText::textSizePropertyStr, textSize );
            properties->getPropertyValue( IUIText::verticalAlignmentPropertyStr, verticalAlignment );
            properties->getPropertyValue( IUIText::horizontalAlignmentPropertyStr, horizontalAlignment );

            setText( text );
            setTextSize( textSize );
            setVerticalAlignment( verticalAlignment );
            setHorizontalAlignment( horizontalAlignment );

            // Visual style fields
            properties->getPropertyValue( IUIText::textColourRStr, m_textColour.r );
            properties->getPropertyValue( IUIText::textColourGStr, m_textColour.g );
            properties->getPropertyValue( IUIText::textColourBStr, m_textColour.b );
            properties->getPropertyValue( IUIText::textColourAStr, m_textColour.a );
            properties->getPropertyValue( IUIText::backgroundColourRStr, m_backgroundColour.r );
            properties->getPropertyValue( IUIText::backgroundColourGStr, m_backgroundColour.g );
            properties->getPropertyValue( IUIText::backgroundColourBStr, m_backgroundColour.b );
            properties->getPropertyValue( IUIText::backgroundColourAStr, m_backgroundColour.a );

            f32 padX = m_textPadding.X();
            f32 padY = m_textPadding.Y();
            properties->getPropertyValue( IUIText::textPaddingXStr, padX );
            properties->getPropertyValue( IUIText::textPaddingYStr, padY );
            setTextPadding( Vector2F( padX, padY ) );

            properties->getPropertyValue( IUIText::textWrapStr, m_textWrap );
            properties->getPropertyValue( drawBackgroundStr, m_drawBackground );
            properties->getPropertyValue( backgroundRoundingStr, m_backgroundRounding );

            properties->getPropertyValue( leftHorizontalAlignmentStr, leftHorizontalAlignment );
            properties->getPropertyValue( rightHorizontalAlignmentStr, rightHorizontalAlignment );
            properties->getPropertyValue( topVerticalAlignmentStr, topVerticalAlignment );
            properties->getPropertyValue( bottomVerticalAlignmentStr, bottomVerticalAlignment );

            setTextWrap( m_textWrap );
            setDrawBackground( m_drawBackground );
            setBackgroundRounding( m_backgroundRounding );
            setLeftHorizontalAlignment(
                static_cast<u8>( std::clamp( leftHorizontalAlignment, 0, 255 ) ) );
            setRightHorizontalAlignment(
                static_cast<u8>( std::clamp( rightHorizontalAlignment, 0, 255 ) ) );
            setTopVerticalAlignment( static_cast<u8>( std::clamp( topVerticalAlignment, 0, 255 ) ) );
            setBottomVerticalAlignment(
                static_cast<u8>( std::clamp( bottomVerticalAlignment, 0, 255 ) ) );

            applyStyle();
        }

        void UITextCore::applyStyle()
        {
            auto toWpByte = []( f32 v ) -> wp_byte {
                return static_cast<wp_byte>( std::clamp( v, 0.0f, 1.0f ) * 255.0f );
            };

            //labelText->text = wp_rgba( toWpByte( m_textColour.r ), toWpByte( m_textColour.g ),
            //                           toWpByte( m_textColour.b ), toWpByte( m_textColour.a ) );

            //labelText->background =
            //    wp_rgba( toWpByte( m_backgroundColour.r ), toWpByte( m_backgroundColour.g ),
            //             toWpByte( m_backgroundColour.b ), toWpByte( m_backgroundColour.a ) );

            //labelText->padding.x = m_textPadding.X();
            //labelText->padding.y = m_textPadding.Y();
        }

        ColourF UITextCore::getTextColour() const
        {
            return m_textColour;
        }

        void UITextCore::setTextColour( const ColourF &colour )
        {
            m_textColour = colour;
            applyStyle();
        }

        ColourF UITextCore::getBackgroundColour() const
        {
            return m_backgroundColour;
        }

        void UITextCore::setBackgroundColour( const ColourF &colour )
        {
            m_backgroundColour = colour;
            applyStyle();
        }

        Vector2F UITextCore::getTextPadding() const
        {
            return m_textPadding;
        }

        void UITextCore::setTextPadding( const Vector2F &padding )
        {
            m_textPadding = padding;
            applyStyle();
        }

        bool UITextCore::getTextWrap() const
        {
            return m_textWrap;
        }

        void UITextCore::setTextWrap( bool wrap )
        {
            m_textWrap = wrap;
        }

        bool UITextCore::getDrawBackground() const
        {
            return m_drawBackground;
        }

        void UITextCore::setDrawBackground( bool drawBackground )
        {
            m_drawBackground = drawBackground;
        }

        f32 UITextCore::getBackgroundRounding() const
        {
            return m_backgroundRounding;
        }

        void UITextCore::setBackgroundRounding( f32 rounding )
        {
            m_backgroundRounding = MathF::max( rounding, 0.0f );
        }

        u8 UITextCore::getLeftHorizontalAlignment() const
        {
            return m_leftHorizontalAlignment;
        }

        void UITextCore::setLeftHorizontalAlignment( u8 alignment )
        {
            m_leftHorizontalAlignment = alignment;
        }

        u8 UITextCore::getRightHorizontalAlignment() const
        {
            return m_rightHorizontalAlignment;
        }

        void UITextCore::setRightHorizontalAlignment( u8 alignment )
        {
            m_rightHorizontalAlignment = alignment;
        }

        u8 UITextCore::getTopVerticalAlignment() const
        {
            return m_topVerticalAlignment;
        }

        void UITextCore::setTopVerticalAlignment( u8 alignment )
        {
            m_topVerticalAlignment = alignment;
        }

        u8 UITextCore::getBottomVerticalAlignment() const
        {
            return m_bottomVerticalAlignment;
        }

        void UITextCore::setBottomVerticalAlignment( u8 alignment )
        {
            m_bottomVerticalAlignment = alignment;
        }

        void UITextCore::createStateContext()
        {
            WP_ASSERT( getStateContext() == nullptr );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateTask = graphicsSystem->getStateTask();

            auto stateContext = stateManager->addStateContext();
            stateContext->setOwner( this );
            setStateContext( stateContext );
            stateContext->setTaskId( stateTask );

            auto listener = factoryManager->make_ptr<ElementStateListener>();
            listener->setOwner( this );
            stateContext->addStateListener( listener );
            setStateListener( listener );

            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<UIElementStateData>();
            state->setData( stateData );

            auto transformState = factoryManager->make_ptr<State>();
            transformState->setId( getId() );
            transformState->setOwner( this );
            stateContext->addState( transformState );

            auto transformStateData = factoryManager->make_ptr<UITransformStateData>();
            transformState->setData( transformStateData );

            auto textState = factoryManager->make_ptr<State>();
            textState->setId( getId() );
            textState->setOwner( this );
            stateContext->addState( textState );

            auto textStateData = factoryManager->make_ptr<UITextStateData>();
            textState->setData( textStateData );
        }

    }  // namespace ui
}  // namespace workphone
