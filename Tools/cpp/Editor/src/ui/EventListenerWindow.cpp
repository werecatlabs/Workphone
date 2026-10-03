#include <EditorPCH.hpp>
#include "ui/EventListenerWindow.hpp"
#include "ui/EventWindow.hpp"
#include "ui/PropertiesWindow.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    EventListenerWindow::EventListenerWindow() = default;

    EventListenerWindow::~EventListenerWindow() = default;

    void EventListenerWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto parent = getParent();

            setParentWindow( parent );

            m_propertiesWindow = workphone::make_ptr<PropertiesWindow>();
            m_propertiesWindow->setParent( parent );
            m_propertiesWindow->load( data );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EventListenerWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            if( m_propertiesWindow )
            {
                m_propertiesWindow->setWindowVisible( false );
                m_propertiesWindow->unload( nullptr );
                m_propertiesWindow = nullptr;
            }

            m_componentEventListener = nullptr;

            EditorWindow::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EventListenerWindow::updateSelection()
    {
        auto listener = getEventListener();
        m_propertiesWindow->setSelected( listener );

        m_propertiesWindow->updateSelection();
    }

    SmartPtr<scene::IComponentEventListener> EventListenerWindow::getComponentEventListener() const
    {
        return m_componentEventListener;
    }

    void EventListenerWindow::setComponentEventListener(
        SmartPtr<scene::IComponentEventListener> eventListener )
    {
        m_componentEventListener = eventListener;
    }
}  // namespace workphone::editor
