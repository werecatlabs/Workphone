#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UILayoutColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIManagerColibri.hpp>
#include <ColibriGui/ColibriWindow.h>
#include <ColibriGui/ColibriManager.h>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace ui
    {
        WP_CLASS_REGISTER_DERIVED( workphone, UILayoutColibri, UIElementColibri<IUILayoutWindow> );

        UILayoutColibri::UILayoutColibri()
        {
            createStateContext();
        }

        UILayoutColibri::~UILayoutColibri()
        {
            unload( nullptr );
        }

        void UILayoutColibri::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto ui =
                    workphone::static_pointer_cast<UIManagerColibri>( applicationManager->getUI() );

                //auto colibriManager = ui->getColibriManager();
                //m_window = colibriManager->createWindow( 0 );

                //setWidget( m_window );

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UILayoutColibri::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );

                m_window = nullptr;
                //UIElementOgreNext<IUILayoutWindow>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<IFSM> UILayoutColibri::getFSM()
        {
            return nullptr;
        }

        SmartPtr<IUIWindow> UILayoutColibri::getParentWindow() const
        {
            return nullptr;
        }

        void UILayoutColibri::setParentWindow( SmartPtr<IUIWindow> uiWindow )
        {
        }

        void UILayoutColibri::invalidate()
        {
        }
    }  // namespace ui
}  // namespace workphone
