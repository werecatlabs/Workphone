#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UISliderColibri.hpp>
#include "WPGraphicsOgreNext/UI/Colibri/UIManagerColibri.hpp"
#include "ColibriGui/ColibriWindow.h"
#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriSlider.h"
#include "ColibriGui/ColibriManager.h"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UISliderColibri,
                                   UIElementColibri<UIElement<IUISlider>> );

        UISliderColibri::UISliderColibri()
        {
            createStateContext();
        }

        UISliderColibri::~UISliderColibri()
        {
            unload( nullptr );
        }

        void UISliderColibri::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                ScopedLock lock( this );

                auto ui = workphone::static_pointer_cast<UIManagerColibri>(
                    applicationManager->getRenderUI() );

                auto window = ui->getLayoutWindow();
                if( !window )
                {
                    WP_LOG_ERROR( "Layout window is not available." );
                    setLoadingState( LoadingState::Loaded );
                    return;
                }

                auto colibriManager = ui->getColibriManager();
                if( !colibriManager )
                {
                    WP_LOG_ERROR( "Colibri manager is not available." );
                    setLoadingState( LoadingState::Loaded );
                    return;
                }

                m_slider = colibriManager->createWidget<Colibri::Slider>( window );
                m_slider->setDenominator( m_denominator );
                m_slider->setRange( static_cast<s32>( m_minValue * m_denominator ),
                                    static_cast<s32>( m_maxValue * m_denominator ) );
                m_slider->setCurrentValue( 0 );

                m_slider->m_minSize = Ogre::Vector2( 350, 32 );
                m_slider->setTransform( Ogre::Vector2( 0, 0 ), Ogre::Vector2( 1920, 1080 ) );

                setWidget( m_slider );

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UISliderColibri::unload( SmartPtr<ISharedObject> data )
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
                    UIElementColibri<UIElement<IUISlider>>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        workphone::f32 UISliderColibri::getValue() const
        {
            if( m_slider )
            {
                return m_slider->getCurrentValueProcessed();
            }
            return 0.0f;
        }

        void UISliderColibri::setValue( f32 value )
        {
            if( m_slider )
            {
                auto intValue = static_cast<s32>( value * m_denominator );
                m_slider->setCurrentValue( intValue );
            }
        }

        workphone::f32 UISliderColibri::getMinValue() const
        {
            return m_minValue;
        }

        void UISliderColibri::setMinValue( f32 minValue )
        {
            m_minValue = minValue;
            if( m_slider )
            {
                m_slider->setRange( static_cast<s32>( m_minValue * m_denominator ),
                                    static_cast<s32>( m_maxValue * m_denominator ) );
            }
        }

        workphone::f32 UISliderColibri::getMaxValue() const
        {
            return m_maxValue;
        }

        void UISliderColibri::setMaxValue( f32 maxValue )
        {
            m_maxValue = maxValue;
            if( m_slider )
            {
                m_slider->setRange( static_cast<s32>( m_minValue * m_denominator ),
                                    static_cast<s32>( m_maxValue * m_denominator ) );
            }
        }

        workphone::Direction UISliderColibri::getDirection() const
        {
            if( m_slider )
            {
                return m_slider->getVertical() ? Direction::Vertical : Direction::Horizontal;
            }
            return Direction::Horizontal;
        }

        void UISliderColibri::setDirection( Direction direction )
        {
            if( m_slider )
            {
                m_slider->setVertical( direction == Direction::Vertical );
            }
        }

        bool UISliderColibri::isDragging() const
        {
            if( m_slider )
            {
                return m_slider->getCurrentState() == Colibri::States::Pressed;
            }
            return false;
        }

        void UISliderColibri::setDragging( bool dragging )
        {
            if( m_slider )
            {
                if( dragging )
                {
                    m_slider->setState( Colibri::States::Pressed, false );
                }
                else if( m_slider->getCurrentState() == Colibri::States::Pressed )
                {
                    m_slider->setState( Colibri::States::Idle, false );
                }
            }
        }

        void UISliderColibri::setSize( const Vector2<real_Num> &size )
        {
            if( m_slider )
            {
                auto currentPos = m_slider->getLocalTopLeft();
                m_slider->setTransform( currentPos, Ogre::Vector2( static_cast<f32>( size.X() ),
                                                                   static_cast<f32>( size.Y() ) ) );
            }
        }

    }  // namespace ui
}  // namespace workphone
