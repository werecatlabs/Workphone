#include <EditorPCH.hpp>
#include "ui/EditorWindow.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorWindow, scene::GameEditor );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorWindow::ApplicationListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorWindow::UIListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorWindow::UIDragSource, ui::IUIDragSource );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorWindow::UIDropTarget, ui::IUIDropTarget );

    EditorWindow::EditorWindow() = default;

    EditorWindow::~EditorWindow() = default;

    void EditorWindow::load( SmartPtr<ISharedObject> data )
    {
        GameEditor::load( data );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        WP_ASSERT( factoryManager->isValid() );

        auto dragSource = factoryManager->make_ptr<UIDragSource>();
        dragSource->setOwner( this );
        //setWindowDropTarget( dragSource );

        auto dropTarget = factoryManager->make_ptr<UIDropTarget>();
        dropTarget->setOwner( this );
        setWindowDropTarget( dropTarget );

        setupListeners();
    }

    void EditorWindow::reload( SmartPtr<ISharedObject> data )
    {
        if( auto invoker = getInvoker() )
        {
            invoker->callObjectMember( "unload" );
            invoker->callObjectMember( "load" );
        }
    }

    void EditorWindow::unload( SmartPtr<ISharedObject> data )
    {
        const auto hasResources =
            m_applicationListener || getEventListener() || getWindowDragSource() ||
            getWindowDropTarget() || getInvoker() || getReceiver() || getScriptClass() ||
            getDebugWindow() || getParentWindow() || !m_dataArray.empty();

        if( getLoadingState() == LoadingState::Unloaded && !hasResources )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );

        if( auto applicationListener = m_applicationListener.load() )
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( applicationManager )
            {
                applicationManager->removeObjectListener( applicationListener );
            }

            if( auto listener = workphone::dynamic_pointer_cast<ApplicationListener>( applicationListener ) )
                listener->setOwner( nullptr );
            m_applicationListener = nullptr;
        }

        destroyScriptObject();

        m_receiver = nullptr;
        m_scriptClass = nullptr;
        m_dataArray.clear();

        // UI controls can retain these callbacks after their editor window is
        // released. Disconnect their weak owners while this window is still alive.
        if( auto listener = workphone::dynamic_pointer_cast<UIListener>( getEventListener() ) )
            listener->setOwner( nullptr );
        if( auto source = workphone::dynamic_pointer_cast<UIDragSource>( getWindowDragSource() ) )
            source->setOwner( nullptr );
        if( auto target = workphone::dynamic_pointer_cast<UIDropTarget>( getWindowDropTarget() ) )
            target->setOwner( nullptr );

        GameEditor::unload( data );

        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<ui::IUIWindow> EditorWindow::getDebugWindow() const
    {
        return m_debugWindow;
    }

    void EditorWindow::setDebugWindow( SmartPtr<ui::IUIWindow> debugWindow )
    {
        m_debugWindow = debugWindow;
    }

    bool EditorWindow::isWindowVisible() const
    {
        return m_windowVisible;
    }

    void EditorWindow::setWindowVisible( bool visible )
    {
        try
        {
            m_windowVisible = visible;

            if( auto parentWindow = getParentWindow() )
            {
                parentWindow->setVisible( visible, false );
            }

            //if( m_debugWindow )
            //{
            //    m_debugWindow->setVisible( visible, false );
            //}

            if( auto invoker = getInvoker() )
            {
                invoker->callObjectMember( visible ? "show" : "hide" );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorWindow::updateSelection()
    {
        if( auto invoker = getInvoker() )
        {
            invoker->callObjectMember( "updateSelection" );
        }
    }

    SmartPtr<IEventListener> EditorWindow::getEventListener() const
    {
        return m_eventListener;
    }

    void EditorWindow::setEventListener( SmartPtr<IEventListener> eventListener )
    {
        m_eventListener = eventListener;
    }

    SmartPtr<ui::IUIDragSource> EditorWindow::getWindowDragSource() const
    {
        return m_windowDragSource;
    }

    void EditorWindow::setWindowDragSource( SmartPtr<ui::IUIDragSource> windowDragSource )
    {
        m_windowDragSource = windowDragSource;
    }

    SmartPtr<ui::IUIDropTarget> EditorWindow::getWindowDropTarget() const
    {
        return m_windowDropTarget;
    }

    void EditorWindow::setWindowDropTarget( SmartPtr<ui::IUIDropTarget> windowDropTarget )
    {
        m_windowDropTarget = windowDropTarget;
    }

    Parameter EditorWindow::handleApplicationEvent( EventType eventType, hash_type eventValue,
                                                    const Array<Parameter> &arguments,
                                                    SmartPtr<ISharedObject> sender,
                                                    SmartPtr<ISharedObject> object,
                                                    SmartPtr<IEvent> event )
    {
        return {};
    }

    Parameter EditorWindow::handleEvent( EventType eventType, hash_type eventValue,
                                         const Array<Parameter> &arguments,
                                         SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                         SmartPtr<IEvent> event )
    {
        if( auto invoker = getInvoker() )
        {
            Array<Parameter> params;
            params.reserve( 6 );

            params.emplace_back( static_cast<u32>( eventType ) );
            params.emplace_back( eventValue );
            params.emplace_back( arguments );
            params.emplace_back( sender );
            params.emplace_back( object );
            params.emplace_back( workphone::static_pointer_cast<ISharedObject>( event ) );

            Array<Parameter> results;

            invoker->callObjectMember( "handleEvent", params, results );
        }

        return {};
    }

    String EditorWindow::handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element )
    {
        return "";
    }

    void EditorWindow::handleDrop( const Vector2I &position, SmartPtr<ui::IUIElement> element,
                                   const String &data )
    {
    }

    String EditorWindow::getClassName() const
    {
        return m_className;
    }

    void EditorWindow::setClassName( const String &className )
    {
        m_className = className;
    }

    SmartPtr<IScriptInvoker> EditorWindow::getInvoker() const
    {
        return m_invoker;
    }

    void EditorWindow::setInvoker( SmartPtr<IScriptInvoker> invoker )
    {
        m_invoker = invoker;
    }

    SmartPtr<IScriptReceiver> EditorWindow::getReceiver() const
    {
        return m_receiver;
    }

    void EditorWindow::setReceiver( SmartPtr<IScriptReceiver> receiver )
    {
        m_receiver = receiver;
    }

    SmartPtr<IScriptClass> EditorWindow::getScriptClass() const
    {
        return m_scriptClass;
    }

    void EditorWindow::setScriptClass( SmartPtr<IScriptClass> scriptClass )
    {
        m_scriptClass = scriptClass;
    }

    void EditorWindow::addData( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            m_dataArray.push_back( data );
        }
    }

    void EditorWindow::removeData( SmartPtr<ISharedObject> data )
    {
        auto it = std::find( m_dataArray.begin(), m_dataArray.end(), data );
        if( it != m_dataArray.end() )
        {
            m_dataArray.erase( it );
        }
    }

    void EditorWindow::clearData()
    {
        m_dataArray.clear();
    }

    Array<SmartPtr<ISharedObject>> EditorWindow::getData() const
    {
        return { m_dataArray.begin(), m_dataArray.end() };
    }

    void EditorWindow::setDraggable( SmartPtr<ui::IUIElement> element, bool draggable )
    {
    }

    bool EditorWindow::isDraggable( SmartPtr<ui::IUIElement> element ) const
    {
        return false;
    }

    void EditorWindow::setDroppable( SmartPtr<ui::IUIElement> element, bool droppable )
    {
        auto dropTarget = getWindowDropTarget();

        if( droppable )
        {
            if( element )
            {
                element->setDropTarget( dropTarget );
            }
        }
        else
        {
            if( element )
            {
                element->setDropTarget( nullptr );
            }
        }
    }

    bool EditorWindow::isDroppable( SmartPtr<ui::IUIElement> element ) const
    {
        return element->getDropTarget() == getWindowDropTarget();
    }

    void EditorWindow::setHandleEvents( SmartPtr<ui::IUIElement> element, bool handleEvents )
    {
        auto listener = getEventListener();

        if( handleEvents )
        {
            element->addObjectListener( listener );
        }
        else
        {
            element->removeObjectListener( listener );
        }
    }

    bool EditorWindow::getHandleEvents( SmartPtr<ui::IUIElement> element ) const
    {
        return false;
    }

    void EditorWindow::setupListeners()
    {
        auto applicationManager = core::IApplicationManager::instance();

        auto listener = workphone::make_ptr<UIListener>();
        listener->setOwner( this );
        setEventListener( listener );
    }

    void EditorWindow::setupApplicationListeners()
    {
        auto applicationManager = core::IApplicationManager::instance();

        auto applicationListener = workphone::make_ptr<ApplicationListener>();
        applicationListener->setOwner( this );
        applicationManager->addObjectListener( applicationListener );
        m_applicationListener = applicationListener;
    }

    void EditorWindow::unlock()
    {
        m_mutex.unlock();
    }

    void EditorWindow::lock()
    {
        m_mutex.lock();
    }

    Parameter EditorWindow::UIListener::handleEvent( EventType eventType, hash_type eventValue,
                                                     const Array<Parameter> &arguments,
                                                     SmartPtr<ISharedObject> sender,
                                                     SmartPtr<ISharedObject> object,
                                                     SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            if( owner->isWindowVisible() )
            {
                owner->handleEvent( eventType, eventValue, arguments, sender, object, event );
            }
        }

        return {};
    }

    SmartPtr<EditorWindow> EditorWindow::UIListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void EditorWindow::UIListener::setOwner( SmartPtr<EditorWindow> owner )
    {
        m_owner = owner;
    }

    EditorWindow::UIListener::UIListener() = default;

    EditorWindow::UIListener::~UIListener() = default;

    Parameter EditorWindow::UIDragSource::handleEvent( EventType eventType, hash_type eventValue,
                                                       const Array<Parameter> &arguments,
                                                       SmartPtr<ISharedObject> sender,
                                                       SmartPtr<ISharedObject> object,
                                                       SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleDrag )
        {
            if( auto owner = getOwner() )
            {
                owner->handleDrag( Vector2I::zero(), sender );
            }
        }

        return {};
    }

    String EditorWindow::UIDragSource::handleDrag( const Vector2I &position,
                                                   SmartPtr<ui::IUIElement> element )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleDrag( position, element );
        }

        return "";
    }

    SmartPtr<EditorWindow> EditorWindow::UIDragSource::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void EditorWindow::UIDragSource::setOwner( SmartPtr<EditorWindow> owner )
    {
        m_owner = owner;
    }

    EditorWindow::UIDragSource::UIDragSource() = default;

    EditorWindow::UIDragSource::~UIDragSource() = default;

    Parameter EditorWindow::UIDropTarget::handleEvent( EventType eventType, hash_type eventValue,
                                                       const Array<Parameter> &arguments,
                                                       SmartPtr<ISharedObject> sender,
                                                       SmartPtr<ISharedObject> object,
                                                       SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            owner->handleEvent( eventType, eventValue, arguments, sender, object, event );
        }

        return {};
    }

    bool EditorWindow::UIDropTarget::handleDrop( const Vector2I &position, SmartPtr<ui::IUIElement> src,
                                                 SmartPtr<ui::IUIElement> dst, const String &data )
    {
        if( auto owner = getOwner() )
        {
            owner->handleDrop( position, src, data );
        }

        return false;
    }

    SmartPtr<EditorWindow> EditorWindow::UIDropTarget::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void EditorWindow::UIDropTarget::setOwner( SmartPtr<EditorWindow> owner )
    {
        m_owner = owner;
    }

    EditorWindow::UIDropTarget::UIDropTarget() = default;

    EditorWindow::UIDropTarget::~UIDropTarget() = default;

    EditorWindow::ApplicationListener::ApplicationListener()
    {
    }

    EditorWindow::ApplicationListener::~ApplicationListener()
    {
    }

    Parameter EditorWindow::ApplicationListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            owner->handleApplicationEvent( eventType, eventValue, arguments, sender, object, event );
        }

        return {};
    }

    SmartPtr<EditorWindow> EditorWindow::ApplicationListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void EditorWindow::ApplicationListener::setOwner( SmartPtr<EditorWindow> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::editor
