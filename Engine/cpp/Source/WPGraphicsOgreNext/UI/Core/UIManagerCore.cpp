#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIButtonCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIDropdownCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIImageCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UILayoutCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIProgressBarCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIRadialProgressCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UISliderCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UITabViewCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UITextCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIToggleCore.hpp>
#include <WPGraphicsOgreNext/UIRenderer.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreHlms.h>
#include <limits>

extern "C" {
#include <workphone.h>
#include <workphone_layout.h>
#include <workphone_types.h>
}

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, UIManagerCore, IUIManager );

    UIManagerCore::UIManagerCore()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
    }

    UIManagerCore::~UIManagerCore()
    {
    }

    void UIManagerCore::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            ScopedLock lock( this, true );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            auto fileSystem = applicationManager->getFileSystemPtr();

            auto inputListener = workphone::make_ptr<EventListener>();
            inputListener->setOwner( this );
            m_inputListener = inputListener;

            auto factoryManager = workphone::make_ptr<FactoryManager>();
            factoryManager->load( nullptr );
            setFactoryManager( factoryManager );

            FactoryUtil::addFactory<UILayoutCore>( factoryManager );
            FactoryUtil::addFactory<UIDropdownCore>( factoryManager );
            FactoryUtil::addFactory<UITextCore>( factoryManager );
            FactoryUtil::addFactory<UIImageCore>( factoryManager );
            FactoryUtil::addFactory<UISliderCore>( factoryManager );
            FactoryUtil::addFactory<UITabItemCore>( factoryManager );
            FactoryUtil::addFactory<UITabViewCore>( factoryManager );
            FactoryUtil::addFactory<UIButtonCore>( factoryManager );
            FactoryUtil::addFactory<UIToggleCore>( factoryManager );
            FactoryUtil::addFactory<UIProgressBarCore>( factoryManager );
            FactoryUtil::addFactory<UIRadialProgressCore>( factoryManager );

            const size_t numLayoutElements = 4;
            const size_t numButtonElements = 4;
            const size_t numDropdownElements = 4;
            const size_t numImageElements = 32;
            const size_t numTextElements = 32;
            const size_t numTabViewElements = 8;
            const size_t numTabItemElements = 32;
            const size_t numProgressBarElements = 16;
            const size_t numRadialProgressElements = 16;

            factoryManager->setPoolSizeByType<UILayoutCore>( numLayoutElements );
            factoryManager->setPoolSizeByType<UIButtonCore>( numButtonElements );
            factoryManager->setPoolSizeByType<UIDropdownCore>( numDropdownElements );
            factoryManager->setPoolSizeByType<UIImageCore>( numImageElements );
            factoryManager->setPoolSizeByType<UITextCore>( numTextElements );
            factoryManager->setPoolSizeByType<UITabViewCore>( numTabViewElements );
            factoryManager->setPoolSizeByType<UITabItemCore>( numTabItemElements );
            factoryManager->setPoolSizeByType<UIProgressBarCore>( numProgressBarElements );
            factoryManager->setPoolSizeByType<UIRadialProgressCore>( numRadialProgressElements );

            factoryManager->allocateData();

            // Acquire the wp_context from the singleton renderer
            if( auto uiRenderer = render::UIRenderer::getSingletonPtr() )
            {
                m_ctx = uiRenderer->getContext();
            }

            createLayoutWindow();

            auto inputMgr = applicationManager->getInputDeviceManager();
            if( inputMgr )
            {
                inputMgr->addListener( m_inputListener );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void UIManagerCore::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( data );
            load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void UIManagerCore::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();

                ScopedLock lock( graphicsSystem );

                m_loadQueue.clear();

                // Clear m_layoutWindow before the element loop to avoid double-destruction.
                // The layout window is also stored in m_elements; releasing our reference here
                // first ensures that when the loop unloads and destroys it, the AtomicSmartPtr
                // no longer holds a dangling pointer that would trigger a second removeReference.
                setLayoutWindow( nullptr );

                if( !m_unloadQueue.empty() )
                {
                    SmartPtr<ISharedObject> object;
                    while( m_unloadQueue.try_pop( object ) )
                    {
                        object->unload( nullptr );
                    }
                }

                for( auto element : m_elements )
                {
                    if( element )
                    {
                        element->unload( data );
                    }
                }

                m_elements.clear();

                auto inputMgr = applicationManager->getInputDeviceManager();
                if( inputMgr )
                {
                    if( m_inputListener )
                    {
                        m_inputListener->unload( nullptr );
                        inputMgr->removeListener( m_inputListener );
                        m_inputListener = nullptr;
                    }
                }

                if( auto factoryManager = getFactoryManager() )
                {
                    factoryManager->unload( nullptr );
                    setFactoryManager( nullptr );
                }

                m_ctx = nullptr;

                m_graphicsScene = nullptr;
                m_factoryManager = nullptr;
                m_inputListener = nullptr;
                m_layoutWindow = nullptr;
                m_overlay = nullptr;

                UIManager::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool UIManagerCore::loadFont( const String &fontPath, const String &type )
    {
        return false;
    }

    void UIManagerCore::unloadFont( const String &fontPath, const String &type )
    {
    }

    bool UIManagerCore::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        auto task = Thread::getCurrentTask();
        if( task == TaskId::Render )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto eventType = event->getEventType();
            if( eventType == IInputEvent::EventType::Mouse )
            {
                if( auto mouseState = event->getMouseState() )
                {
                    auto absolutePosition = mouseState->getAbsolutePosition();
                    auto x = absolutePosition.X();
                    auto y = absolutePosition.Y();
                    auto z = 0;

                    auto relativePosition = mouseState->getRelativePosition();

                    if( auto uiWindow = applicationManager->getSceneRenderWindow() )
                    {
                        if( auto mainWindow = applicationManager->getWindow() )
                        {
                            auto mainWindowSize = mainWindow->getSize();
                            auto mainWindowSizeF = Vector2F( static_cast<f32>( mainWindowSize.x ),
                                                             static_cast<f32>( mainWindowSize.y ) );

                            auto sceneWindowPosition = uiWindow->getPosition();
                            auto sceneWindowSize = uiWindow->getSize();

                            auto pos = sceneWindowPosition / mainWindowSizeF;
                            auto size = sceneWindowSize / mainWindowSizeF;

                            if( size.X() <= 0.0f || size.Y() <= 0.0f )
                            {
                                return false;
                            }

                            auto aabb = AABB2F( pos, size, true );
                            if( aabb.isInside( relativePosition ) )
                            {
                                // Compute a [0, 1] position relative to the scene window,
                                // then project onto the 1920x1080 reference canvas that all
                                // UI elements use for their normalised coordinates.
                                const float kRefW = sceneWindowSize.X() / size.X();
                                const float kRefH = sceneWindowSize.Y() / size.Y();

                                const float nx = ( absolutePosition.X() - sceneWindowPosition.X() ) /
                                                 mainWindowSizeF.X();
                                const float ny = ( absolutePosition.Y() - sceneWindowPosition.Y() ) /
                                                 mainWindowSizeF.Y();

                                Vector2F canvasPoint( nx * kRefW, ny * kRefH );

                                if( mouseState->getEventType() == IMouseState::Event::Moved )
                                {
                                    if( auto ctx = m_ctx.load() )
                                        wp_input_motion( ctx, static_cast<int>( canvasPoint.x ),
                                                         static_cast<int>( canvasPoint.y ) );
                                }

                                if( mouseState->getEventType() == IMouseState::Event::LeftPressed )
                                {
                                    if( auto ctx = m_ctx.load() )
                                    {
                                        wp_input_motion( ctx, static_cast<int>( canvasPoint.x ),
                                                         static_cast<int>( canvasPoint.y ) );
                                        wp_input_button( ctx, WORKPHONE_BUTTON_LEFT,
                                                         static_cast<int>( canvasPoint.x ),
                                                         static_cast<int>( canvasPoint.y ), wp_true );
                                    }
                                }

                                if( mouseState->getEventType() == IMouseState::Event::LeftReleased )
                                {
                                    if( auto ctx = m_ctx.load() )
                                    {
                                        wp_input_motion( ctx, static_cast<int>( canvasPoint.x ),
                                                         static_cast<int>( canvasPoint.y ) );
                                        wp_input_button( ctx, WORKPHONE_BUTTON_LEFT,
                                                         static_cast<int>( canvasPoint.x ),
                                                         static_cast<int>( canvasPoint.y ), wp_false );
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        if( mouseState->getEventType() == IMouseState::Event::Moved )
                        {
                            if( auto ctx = m_ctx.load() )
                                wp_input_motion( ctx, static_cast<int>( x ), static_cast<int>( y ) );
                        }

                        if( mouseState->getEventType() == IMouseState::Event::LeftPressed )
                        {
                            if( auto ctx = m_ctx.load() )
                            {
                                wp_input_motion( ctx, static_cast<int>( x ), static_cast<int>( y ) );
                                wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, static_cast<int>( x ),
                                                 static_cast<int>( y ), wp_true );
                            }
                        }

                        if( mouseState->getEventType() == IMouseState::Event::LeftReleased )
                        {
                            if( auto ctx = m_ctx.load() )
                            {
                                wp_input_motion( ctx, static_cast<int>( x ), static_cast<int>( y ) );
                                wp_input_button( ctx, WORKPHONE_BUTTON_LEFT, static_cast<int>( x ),
                                                 static_cast<int>( y ), wp_false );
                            }
                        }
                    }
                }
            }

            auto elements = getElements();
            for( auto &element : elements )
            {
                if( element )
                {
                    element->handleEvent( event );
                }
            }
        }

        return false;
    }

    void UIManagerCore::_getObject( void **ppObject )
    {
        if( isLoaded() )
        {
            // Check if is in valid state
            auto graphicsScene = getGraphicsScene();
            if( graphicsScene )
            {
                *ppObject = m_ctx;
            }
        }
    }

    void UIManagerCore::update()
    {
        try
        {
            if( isLoaded() )
            {
                if( m_ctx )
                {
                    if( !m_unloadQueue.empty() )
                    {
                        SmartPtr<ISharedObject> graphicsObject;
                        while( m_unloadQueue.try_pop( graphicsObject ) )
                        {
                            if( graphicsObject )
                            {
                                graphicsObject->unload( nullptr );
                            }

                            m_elements.erase(
                                std::remove( m_elements.begin(), m_elements.end(), graphicsObject ),
                                m_elements.end() );
                        }
                    }

                    if( !m_loadQueue.empty() )
                    {
                        SmartPtr<ISharedObject> graphicsObject;
                        while( m_loadQueue.try_pop( graphicsObject ) )
                        {
                            if( graphicsObject )
                            {
                                if( !graphicsObject->isLoaded() )
                                {
                                    graphicsObject->load( nullptr );
                                }
                            }
                        }
                    }

                    ScopedLock lock( this );

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    auto timer = applicationManager->getTimerPtr();

                    auto dt = static_cast<f32>( timer->getDeltaTime() );
                    (void)dt;

                    //// Drive per-frame UI element updates
                    //for( auto element : m_elements )
                    //{
                    //    if( element )
                    //    {
                    //        if( element->isLoaded() )
                    //        {
                    //            if( element->isExactly<UILayoutCore>() )
                    //            {
                    //                element->update();
                    //            }
                    //        }
                    //    }
                    //}
                }
            }
        }
        catch( std::exception &e )
        {
            auto message = e.what();
            WP_LOG_ERROR( message );
        }
    }

    void UIManagerCore::render()
    {
        try
        {
            for( auto element : m_elements )
            {
                if( element )
                {
                    if( element->isLoaded() )
                    {
                        if( element->isExactly<UILayoutCore>() )
                        {
                            element->update();
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            auto message = e.what();
            WP_LOG_ERROR( message );
        }
    }

    SmartPtr<IUIElement> UIManagerCore::addElement( hash64 type )
    {
        auto factoryManager = getFactoryManager();
        if( factoryManager )
        {
            // special cases
            if( type == IUIButton::typeInfo() )
            {
                auto element = factoryManager->make_ptr<UIButtonCore>();
                m_elements.push_back( element );

                loadObject( element );
                if( auto layoutWindow = getLayoutWindow() )
                {
                    layoutWindow->addChild( element );
                }

                Array<Parameter> args;

                auto applicationManager = core::IApplicationManager::instancePtr();
                applicationManager->triggerEvent( EventType::UI, IEvent::addUIElement, args, this,
                                                  element, nullptr );

                return element;
            }
            else if( type == IUIToggle::typeInfo() )
            {
                auto element = factoryManager->make_ptr<UIToggleCore>();
                m_elements.push_back( element );

                loadObject( element );
                if( auto layoutWindow = getLayoutWindow() )
                {
                    layoutWindow->addChild( element );
                }

                Array<Parameter> args;

                auto applicationManager = core::IApplicationManager::instancePtr();
                applicationManager->triggerEvent( EventType::UI, IEvent::addUIElement, args, this,
                                                  element, nullptr );

                return element;
            }
            else if( type == IUITabBar::typeInfo() )
            {
                auto element = factoryManager->make_ptr<UITabViewCore>();
                m_elements.push_back( element );

                loadObject( element );
                if( auto layoutWindow = getLayoutWindow() )
                {
                    layoutWindow->addChild( element );
                }

                Array<Parameter> args;

                auto applicationManager = core::IApplicationManager::instancePtr();
                applicationManager->triggerEvent( EventType::UI, IEvent::addUIElement, args, this,
                                                  element, nullptr );

                return element;
            }

            auto element = factoryManager->make_object<IUIElement>( (u32)type );
            m_elements.push_back( element );

            loadObject( element );
            if( auto layoutWindow = getLayoutWindow() )
            {
                if( element != layoutWindow )
                {
                    layoutWindow->addChild( element );
                }
            }

            Array<Parameter> args;

            auto applicationManager = core::IApplicationManager::instancePtr();
            applicationManager->triggerEvent( EventType::UI, IEvent::addUIElement, args, this, element,
                                              nullptr );
            return element;
        }

        return nullptr;
    }

    void UIManagerCore::removeElement( SmartPtr<IUIElement> element )
    {
        if( !m_elements.empty() )
        {
            unloadObject( element );

            m_elements.erase( std::remove( m_elements.begin(), m_elements.end(), element ),
                              m_elements.end() );
        }

        Array<Parameter> args;

        auto applicationManager = core::IApplicationManager::instancePtr();
        applicationManager->triggerEvent( EventType::UI, IEvent::removeUIElement, args, this, element,
                                          nullptr );
    }

    void UIManagerCore::removeElements( const Array<SmartPtr<IUIElement>> &elementsToRemove )
    {
        for( auto &element : elementsToRemove )
        {
            unloadObject( element );

            auto it = std::remove( m_elements.begin(), m_elements.end(), element );
            if( it != m_elements.end() )
            {
                m_elements.erase( it, m_elements.end() );
            }
        }

        Array<Parameter> args;

        auto applicationManager = core::IApplicationManager::instancePtr();
        applicationManager->triggerEvent( EventType::UI, IEvent::removeUIElement, args, this, nullptr,
                                          nullptr );
    }

    void UIManagerCore::clear()
    {
        m_elements.clear();
    }

    SmartPtr<UILayoutCore> UIManagerCore::getLayoutWindow() const
    {
        return m_layoutWindow;
    }

    void UIManagerCore::setLayoutWindow( SmartPtr<UILayoutCore> layoutWindow )
    {
        m_layoutWindow = layoutWindow;
    }

    SmartPtr<IFactoryManager> UIManagerCore::getFactoryManager() const
    {
        return m_factoryManager;
    }

    void UIManagerCore::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

    Array<SmartPtr<IUIElement>> UIManagerCore::getElements() const
    {
        return m_elements.snapshot();
    }

    void UIManagerCore::setElements( Array<SmartPtr<IUIElement>> elements )
    {
        m_elements = ConcurrentArray<SmartPtr<IUIElement>>( elements.begin(), elements.end() );
    }

    SmartPtr<render::IGraphicsScene> UIManagerCore::getGraphicsScene() const
    {
        auto p = m_graphicsScene.load();
        return p.lock();
    }

    void UIManagerCore::setGraphicsScene( SmartPtr<render::IGraphicsScene> scene )
    {
        auto graphicsScene = getGraphicsScene();
        if( graphicsScene != scene )
        {
            m_graphicsScene = scene;

            if( scene )
            {
                setupSceneManager( scene );
                createLayoutWindow();
            }
        }
    }

    void UIManagerCore::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
        {
            graphicsSystem->lock();
        }
    }

    void UIManagerCore::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
        {
            graphicsSystem->unlock();
        }
    }

    bool UIManagerCore::isValid() const
    {
        if( isLoaded() )
        {
            auto root = Ogre::Root::getSingletonPtr();
            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
            Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_USER0 );
            if( hlms )
            {
                return true;
            }
        }

        return false;
    }

    void UIManagerCore::setupSceneManager( SmartPtr<render::IGraphicsScene> sceneManager )
    {
        ScopedLock lock( this );

        m_graphicsScene = sceneManager;

        if( sceneManager )
        {
            auto root = Ogre::Root::getSingletonPtr();

            Ogre::SceneManager *smgr = nullptr;
            sceneManager->_getObject( reinterpret_cast<void **>( &smgr ) );

            if( smgr )
            {
                auto renderSystem = root->getRenderSystem();
                auto vaoManager = renderSystem->getVaoManager();

                Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
                Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_USER0 );
                if( hlms )
                {
                }
                else
                {
                    WP_LOG_ERROR( "Failed to get Hlms for user 0." );
                }
            }
        }
    }

    void UIManagerCore::createLayoutWindow()
    {
        return;

        if( getLayoutWindow() )
        {
            return;
        }

        auto factoryManager = getFactoryManager();
        if( !factoryManager )
        {
            return;
        }

        auto layoutWindow = factoryManager->make_ptr<UILayoutCore>();
        if( !layoutWindow )
        {
            return;
        }

        layoutWindow->setLabel( "UIRoot" );
        layoutWindow->setPosition( Vector2F::zero() );
        layoutWindow->setSize( Vector2F( 1.0f, 1.0f ) );
        layoutWindow->setOrder( std::numeric_limits<s32>::min() );

        setLayoutWindow( layoutWindow );
        m_elements.push_back( layoutWindow );
        loadObject( layoutWindow, true );

        layoutWindow->setPosition( Vector2F::zero() );
        layoutWindow->setSize( Vector2F( 1.0f, 1.0f ) );
    }

    void UIManagerCore::invalidate()
    {
        if( auto layoutWindow = getLayoutWindow() )
        {
            if( auto stateContext = layoutWindow->getStateContext() )
            {
                stateContext->setDirty( true );
            }
        }

        for( auto &element : getElements() )
        {
            if( element )
            {
                if( auto stateContext = element->getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }
        }
    }

    void UIManagerCore::unloadObject( ISharedObject* object, bool forceQueue /*= false */ )
    {
        if( !object )
        {
            WP_LOG_ERROR( "Attempted to unload a null object." );
            return;
        }

        if( object )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto threadPool = applicationManager->getThreadPoolPtr();

            if( threadPool && threadPool->getNumThreads() > 0 )
            {
                if( forceQueue )
                {
                    object->unload( nullptr );
                }
                else
                {
                    m_unloadQueue.push( object );
                }
            }
            else
            {
                object->unload( nullptr );
            }
        }
    }

    void UIManagerCore::loadObject( ISharedObject* object, bool forceQueue /*= false */ )
    {
        if( !object )
        {
            WP_LOG_ERROR( "Attempted to unload a null object." );
            return;
        }

        if( object )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto threadPool = applicationManager->getThreadPoolPtr();

            if( threadPool && threadPool->getNumThreads() > 0 )
            {
                if( forceQueue )
                {
                    object->load( nullptr );
                }
                else
                {
                    m_loadQueue.push( object );
                }
            }
            else
            {
                object->load( nullptr );
            }
        }
    }

    void UIManagerCore::setOverlay( SmartPtr<ISharedObject> overlay )
    {
        m_overlay = overlay;
    }

    SmartPtr<ISharedObject> UIManagerCore::getOverlay() const
    {
        return m_overlay;
    }

    UIManagerCore::EventListener::EventListener() = default;
    UIManagerCore::EventListener::~EventListener() = default;

    void UIManagerCore::EventListener::unload( SmartPtr<ISharedObject> data )
    {
        setOwner( nullptr );
    }

    Parameter UIManagerCore::EventListener::handleEvent( EventType eventType, hash_type eventValue,
                                                         const Array<Parameter> &arguments,
                                                         SmartPtr<ISharedObject> sender,
                                                         SmartPtr<ISharedObject> object,
                                                         SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            owner->handleEvent( event );
        }

        return {};
    }

    SmartPtr<UIManagerCore> UIManagerCore::EventListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void UIManagerCore::EventListener::setOwner( SmartPtr<UIManagerCore> owner )
    {
        m_owner = owner;
    }

}  // namespace workphone::ui
