#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/GameEditor.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Interface/Script/IScriptClass.hpp>
#include <Workphone/Interface/Script/IScriptData.hpp>
#include <Workphone/Interface/Script/IScriptInvoker.hpp>
#include <Workphone/Interface/Script/IScriptReceiver.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, GameEditor, IGameEditor );

    GameEditor::GameEditor()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    GameEditor::~GameEditor() = default;

    void GameEditor::load( SmartPtr<ISharedObject> data )
    {
    }

    void GameEditor::unload( SmartPtr<ISharedObject> data )
    {
        m_invoker = nullptr;

        m_debugWindow = nullptr;

        m_eventListener = nullptr;
        m_applicationListener = nullptr;

        m_windowDragSource = nullptr;
        m_windowDropTarget = nullptr;

        m_parent = nullptr;
        m_parentWindow = nullptr;
    }

    SmartPtr<ui::IUIWindow> GameEditor::getParent() const
    {
        auto p = m_parent.load();
        return p.lock();
    }

    void GameEditor::setParent( SmartPtr<ui::IUIWindow> parent )
    {
        m_parent = parent;
    }

    SmartPtr<ui::IUIWindow> GameEditor::getParentWindow() const
    {
        auto p = m_parentWindow.load();
        return p.lock();
    }

    void GameEditor::setParentWindow( SmartPtr<ui::IUIWindow> parentWindow )
    {
        m_parentWindow = parentWindow;
    }

    SmartPtr<ui::IUIWindow> GameEditor::getDebugWindow() const
    {
        return m_debugWindow;
    }

    void GameEditor::setDebugWindow( SmartPtr<ui::IUIWindow> debugWindow )
    {
        m_debugWindow = debugWindow;
    }

    bool GameEditor::isWindowVisible() const
    {
        return m_windowVisible;
    }

    void GameEditor::setWindowVisible( bool visible )
    {
        m_windowVisible = visible;
    }

    void GameEditor::updateSelection()
    {
    }

    SmartPtr<IEventListener> GameEditor::getEventListener() const
    {
        auto p = m_eventListener.load();
        return p;
    }

    void GameEditor::setEventListener( SmartPtr<IEventListener> eventListener )
    {
        m_eventListener = eventListener;
    }

    SmartPtr<ui::IUIDragSource> GameEditor::getWindowDragSource() const
    {
        return nullptr;
    }

    void GameEditor::setWindowDragSource( SmartPtr<ui::IUIDragSource> windowDragSource )
    {
    }

    SmartPtr<ui::IUIDropTarget> GameEditor::getWindowDropTarget() const
    {
        return nullptr;
    }

    void GameEditor::setWindowDropTarget( SmartPtr<ui::IUIDropTarget> windowDropTarget )
    {
    }

    void GameEditor::setDraggable( SmartPtr<ui::IUIElement> element, bool draggable )
    {
    }

    bool GameEditor::isDraggable( SmartPtr<ui::IUIElement> element ) const
    {
        return false;
    }

    void GameEditor::setDroppable( SmartPtr<ui::IUIElement> element, bool droppable )
    {
    }

    bool GameEditor::isDroppable( SmartPtr<ui::IUIElement> element ) const
    {
        return false;
    }

    void GameEditor::setHandleEvents( SmartPtr<ui::IUIElement> element, bool handleEvents )
    {
    }

    bool GameEditor::getHandleEvents( SmartPtr<ui::IUIElement> element ) const
    {
        return false;
    }

    Parameter GameEditor::handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        return {};
    }

    String GameEditor::handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element )
    {
        return {};
    }

    void GameEditor::handleDrop( const Vector2I &position, SmartPtr<ui::IUIElement> element,
                                 const String &data )
    {
    }

    void GameEditor::destroyScriptObject()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        WP_ASSERT( applicationManager->isValid() );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        WP_ASSERT( factoryManager->isValid() );

        auto scriptManager = applicationManager->getScriptManager();

        if( auto invoker = getInvoker() )
        {
            invoker->callObjectMember( unloadStr );
        }

        if( scriptManager )
        {
            WP_ASSERT( scriptManager->isValid() );
            if( !StringUtil::isNullOrEmpty( m_className ) )
            {
                scriptManager->destroyObject( this );
            }
        }

        if( auto invoker = getInvoker() )
        {
            invoker->unload( nullptr );
            setInvoker( nullptr );
        }
    }

    SmartPtr<IScriptInvoker> GameEditor::getInvoker() const
    {
        return m_invoker;
    }

    void GameEditor::setInvoker( SmartPtr<IScriptInvoker> invoker )
    {
        m_invoker = invoker;
    }

    SmartPtr<IScriptReceiver> GameEditor::getReceiver() const
    {
        return m_receiver;
    }

    void GameEditor::setReceiver( SmartPtr<IScriptReceiver> receiver )
    {
        m_receiver = receiver;
    }

    void GameEditor::lock()
    {
        m_mutex.lock();
    }

    bool GameEditor::try_lock()
    {
        return m_mutex.try_lock();
    }

    void GameEditor::unlock()
    {
        m_mutex.unlock();
    }

}  // namespace workphone::scene
