#include <EditorPCH.hpp>
#include "ui/EventWindow.hpp"
#include "ui/EventListenerWindow.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    EventWindow::EventWindow() = default;

    EventWindow::~EventWindow()
    {
        unload( nullptr );
    }

    void EventWindow::load( SmartPtr<ISharedObject> data )
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

            auto uiListener = workphone::make_ptr<UIElementListener>();
            uiListener->setOwner( this );
            m_uiListener = uiListener;

            m_eventWindows.reserve( 12 );
            m_eventListenerWindows.reserve( 12 );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EventWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            m_event = nullptr;

            for( auto l : m_eventWindows )
            {
                l->unload( nullptr );
            }

            m_eventWindows.clear();

            for( auto l : m_eventListenerWindows )
            {
                l->unload( nullptr );
            }

            m_eventListenerWindows.clear();

            if( m_addComponentButton )
            {
                m_addComponentButton->removeObjectListener( m_uiListener );
                ui->removeElement( m_addComponentButton );
                m_addComponentButton = nullptr;
            }

            if( m_removeComponentButton )
            {
                m_removeComponentButton->removeObjectListener( m_uiListener );
                ui->removeElement( m_removeComponentButton );
                m_removeComponentButton = nullptr;
            }

            m_uiListener = nullptr;

            EditorWindow::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EventWindow::updateSelection()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto selectionManager = applicationManager->getSelectionManager();
            WP_ASSERT( selectionManager );

            if( m_addComponentButton )
            {
                ui->removeElement( m_addComponentButton );
                m_addComponentButton = nullptr;
            }

            if( m_removeComponentButton )
            {
                ui->removeElement( m_removeComponentButton );
                m_removeComponentButton = nullptr;
            }

            for( auto eventListenerWindow : m_eventListenerWindows )
            {
                eventListenerWindow->unload( nullptr );
            }

            m_eventListenerWindows.clear();

            //auto parentWindow = getParentWindow();

            //auto addComponentButton = ui->addElementByType<ui::IUIButton>();
            //WP_ASSERT( addComponentButton );

            //addComponentButton->setElementId( static_cast<s32>( WidgetId::AddComponent ) );
            //addComponentButton->setLabel( "Add" );
            //parentWindow->addChild( addComponentButton );
            //m_addComponentButton = addComponentButton;
            //m_addComponentButton->addObjectListener( m_uiListener );

            //auto removeComponentButton = ui->addElementByType<ui::IUIButton>();
            //WP_ASSERT( removeComponentButton );

            //removeComponentButton->setElementId( static_cast<s32>( WidgetId::RemoveComponent ) );
            //removeComponentButton->setLabel( "Remove" );
            //parentWindow->addChild( removeComponentButton );
            //m_removeComponentButton = removeComponentButton;
            //m_removeComponentButton->addObjectListener( m_uiListener );

            if( auto event = getEvent() )
            {
                auto listeners = event->getListeners();
                for( auto &listener : listeners )
                {
                    auto eventListenerWindow = workphone::make_ptr<EventListenerWindow>();
                    eventListenerWindow->setParent( getParentWindow() );
                    eventListenerWindow->load( nullptr );
                    m_eventListenerWindows.push_back( eventListenerWindow );

                    eventListenerWindow->setEventListener( listener );
                    eventListenerWindow->updateSelection();
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<scene::IComponentEvent> EventWindow::getEvent() const
    {
        return m_event;
    }

    void EventWindow::setEvent( SmartPtr<scene::IComponentEvent> event )
    {
        m_event = event;
    }

    EventWindow::UIElementListener::UIElementListener() = default;

    EventWindow::UIElementListener::~UIElementListener() = default;

    Parameter EventWindow::UIElementListener::handleEvent( EventType eventType, hash_type eventValue,
                                                           const Array<Parameter> &arguments,
                                                           SmartPtr<ISharedObject> sender,
                                                           SmartPtr<ISharedObject> object,
                                                           SmartPtr<IEvent> event )
    {
        return {};
    }

    EventWindow *EventWindow::UIElementListener::getOwner() const
    {
        return m_owner;
    }

    void EventWindow::UIElementListener::setOwner( EventWindow *owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::editor
