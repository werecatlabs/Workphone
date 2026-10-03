#include <EditorPCH.hpp>
#include "ui/EventsWindow.hpp"
#include "ui/EventWindow.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    EventsWindow::EventsWindow() = default;

    EventsWindow::~EventsWindow()
    {
        unload( nullptr );
    }

    void EventsWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            WP_ASSERT( parentWindow );

            setParentWindow( parentWindow );
            parentWindow->setLabel( "EventsWindowChild" );
            parentWindow->setSize( Vector2F( 0.0f, 300.0f ) );
            parentWindow->setHasBorder( true );

            if( parent )
            {
                parent->addChild( parentWindow );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EventsWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            for( auto window : m_eventWindows )
            {
                window->unload( nullptr );
            }

            m_eventWindows.clear();

            if( auto parentWindow = getParentWindow() )
            {
                ui->removeElement( parentWindow );
                setParentWindow( nullptr );
            }

            EditorWindow::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EventsWindow::updateSelection()
    {
        try
        {
            if( isWindowVisible() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto ui = applicationManager->getUI();
                WP_ASSERT( ui );

                auto selectionManager = applicationManager->getSelectionManager();
                WP_ASSERT( selectionManager );

                //m_eventsWindow->removeAllChildren();

                for( auto window : m_eventWindows )
                {
                    window->unload( nullptr );
                }

                m_eventWindows.clear();

                auto selection = selectionManager->getSelection();
                for( auto object : selection )
                {
                    if( object->isDerived<scene::IComponent>() )
                    {
                        auto component = workphone::static_pointer_cast<scene::IComponent>( object );
                        auto events = component->getEvents();
                        for( auto event : events )
                        {
                            auto eventWindow = workphone::make_ptr<EventWindow>();
                            eventWindow->setParent( getParentWindow() );
                            eventWindow->load( nullptr );
                            m_eventWindows.push_back( eventWindow );

                            eventWindow->setEvent( event );
                            eventWindow->updateSelection();
                        }
                    }
                }
            }
            else
            {
                for( auto window : m_eventWindows )
                {
                    window->unload( nullptr );
                }

                m_eventWindows.clear();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone::editor
