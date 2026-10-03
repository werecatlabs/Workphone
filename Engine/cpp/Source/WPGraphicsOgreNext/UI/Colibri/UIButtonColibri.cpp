#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIButtonColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIManagerColibri.hpp>
#include <ColibriGui/ColibriWindow.h>
#include <ColibriGui/ColibriLabel.h>
#include <ColibriGui/ColibriButton.h>
#include <ColibriGui/ColibriManager.h>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone, UIButtonColibri, UIElementColibri<IUIButton> );

        UIButtonColibri::UIButtonColibri()
        {
            createStateContext();
        }

        UIButtonColibri::~UIButtonColibri()
        {
            unload( nullptr );
        }

        void UIButtonColibri::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto ui = workphone::static_pointer_cast<UIManagerColibri>(
                    applicationManager->getRenderUI() );
                auto graphicsSystem = applicationManager->getGraphicsSystem();

                ScopedLock lock( graphicsSystem );

                auto window = ui->getLayoutWindow();
                auto colibriManager = ui->getColibriManager();
                m_button = colibriManager->createWidget<Colibri::Button>( window );
                m_button->m_minSize = Ogre::Vector2( 350, 64 );

                auto label = m_button->getLabel();
                label->setText( "button" );
                m_button->sizeToFit();

                setWidget( m_button );

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIButtonColibri::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                ScopedLock lock( graphicsSystem );

                m_button = nullptr;
                //UIElementOgreNext<ui::IUIButton>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIButtonColibri::setTextSize( f32 textSize )
        {
            m_textSize = textSize;
        }

        f32 UIButtonColibri::getTextSize() const
        {
            return m_textSize;
        }

        bool UIButtonColibri::handleStateChanged( SmartPtr<IState> &state )
        {
            //UIElementOgreNext<IUIButton>::handleStateChanged( state );

            if( m_button )
            {
                auto label = m_button->getLabel();
                auto labelText = getLabel();
                label->setText( labelText.c_str() );
                return true;
            }

            return false;
        }

    }  // namespace ui
}  // namespace workphone
