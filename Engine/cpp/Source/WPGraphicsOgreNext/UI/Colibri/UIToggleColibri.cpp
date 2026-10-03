#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIToggleColibri.hpp>
#include "WPGraphicsOgreNext/UI/Colibri/UIManagerColibri.hpp"
#include "ColibriGui/ColibriWindow.h"
#include "ColibriGui/ColibriCheckbox.h"
#include "ColibriGui/ColibriButton.h"
#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"
#include <Workphone/Workphone.hpp>
#include <Workphone/State/States/UIToggleStateData.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIToggleColibri,
                                   UIElementColibri<UIElement<IUIToggle>> );

        UIToggleColibri::UIToggleColibri() : m_text( "Text" )
        {
            createStateContext();
        }

        UIToggleColibri::~UIToggleColibri()
        {
            unload( nullptr );
        }

        void UIToggleColibri::load( SmartPtr<ISharedObject> data )
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

                m_checkbox = colibriManager->createWidget<Colibri::Checkbox>( window );

                if( m_checkbox->getButton()->hasLabel() )
                {
                    m_checkbox->getButton()->getLabel()->setText( m_text.c_str() );
                }

                m_checkbox->setCurrentValue( m_checked ? 1u : 0u );

                m_checkbox->m_minSize = Ogre::Vector2( 350, 32 );
                m_checkbox->setTransform( Ogre::Vector2( 0, 0 ), Ogre::Vector2( 1920, 1080 ) );
                m_checkbox->sizeToFit();

                setWidget( m_checkbox );

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

        void UIToggleColibri::unload( SmartPtr<ISharedObject> data )
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
                    UIElementColibri<UIElement<IUIToggle>>::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIToggleColibri::setLabel( const String &text )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->label = text.c_str();
                    m_text = text;
                }
            }
        }

        String UIToggleColibri::getLabel() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return state->label.c_str();
                }
            }

            return m_text;
        }

        void UIToggleColibri::setToggled( bool checked )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->checked = checked;
                    m_checked = checked;
                }
            }
        }

        bool UIToggleColibri::isToggled() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return state->checked;
                }
            }

            return m_checked;
        }

        bool UIToggleColibri::handleStateChanged( SmartPtr<IState> &state )
        {
            try
            {
                auto retValue = UIElementColibri<UIElement<IUIToggle>>::handleStateChanged( state );

                auto stateData = state->getData();
                if( stateData->isDerived<UIToggleStateData>() )
                {
                    if( m_checkbox )
                    {
                        auto toggleStateData =
                            workphone::static_pointer_cast<UIToggleStateData>( stateData );

                        if( m_checkbox->getButton()->hasLabel() )
                        {
                            auto label = m_checkbox->getButton()->getLabel();
                            label->setText( toggleStateData->label.c_str() );

                            auto fontSize = static_cast<Colibri::FontSize>( toggleStateData->textSize );
                            label->setDefaultFontSize( fontSize );
                        }

                        m_checkbox->setCurrentValue( toggleStateData->checked ? 1u : 0u );

                        retValue = true;
                    }
                }

                return retValue;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        void UIToggleColibri::setTextSize( f32 textSize )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->textSize = textSize;
                }
            }
        }

        f32 UIToggleColibri::getTextSize() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return state->textSize;
                }
            }

            return 1.0f;
        }

        IUIToggle::ToggleType UIToggleColibri::getToggleType() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return static_cast<ToggleType>( state->toggleType );
                }
            }

            return m_toggleType;
        }

        void UIToggleColibri::setToggleType( ToggleType toggleType )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->toggleType = static_cast<u8>( toggleType );
                    m_toggleType = toggleType;
                }
            }
        }

        IUIToggle::ToggleState UIToggleColibri::getToggleState() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return static_cast<ToggleState>( state->toggleState );
                }
            }

            return m_toggleState;
        }

        void UIToggleColibri::setToggleState( ToggleState toggleState )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->toggleState = static_cast<u8>( toggleState );
                    m_toggleState = toggleState;
                }
            }
        }

        bool UIToggleColibri::getShowLabel() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIToggleStateData>() )
                {
                    return state->showLabel;
                }
            }

            return m_showLabel;
        }

        void UIToggleColibri::setShowLabel( bool showLabel )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIToggleStateData>() )
                {
                    state->showLabel = showLabel;
                    m_showLabel = showLabel;
                }
            }
        }
    }  // namespace ui
}  // namespace workphone
