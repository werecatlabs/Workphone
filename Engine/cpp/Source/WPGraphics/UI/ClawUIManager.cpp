#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/UI/ClawUIAnimatedMaterial.hpp>
#include <WPGraphics/UI/ClawUICursor.hpp>
#include <WPGraphics/UI/ClawUIBar.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>
#include <WPGraphics/UI/ClawUIElementBar.hpp>
#include <WPGraphics/UI/ClawUIButton.hpp>
#include <WPGraphics/UI/ClawUICheckBox.hpp>
#include <WPGraphics/UI/ClawUIFadeEffect.hpp>
#include <WPGraphics/UI/ClawUIToggleButton.hpp>
#include <WPGraphics/UI/ClawUIContainer.hpp>
#include <WPGraphics/UI/ClawUIDialogBox.hpp>
#include <WPGraphics/UI/ClawUIDial.hpp>
#include <WPGraphics/UI/ClawUIText.hpp>
#include <WPGraphics/UI/ClawUITextEntry.hpp>
#include <WPGraphics/UI/ClawUILayout.hpp>
#include <WPGraphics/UI/ClawUIImage.hpp>
#include <WPGraphics/UI/ClawUIImageArray.hpp>
#include <WPGraphics/UI/ClawUIMenu.hpp>
#include <WPGraphics/UI/ClawUICharacterSelectMenu.hpp>
#include <WPGraphics/UI/ClawUIMenuItem.hpp>
#include <WPGraphics/UI/ClawUISpinner.hpp>
#include <WPGraphics/UI/ClawUIItemTemplate.hpp>
#include <WPGraphics/UI/ClawUIScrollingText.hpp>
#include <WPGraphics/UI/ClawUIScrollingTextElement.hpp>
#include <WPGraphics/UI/ClawUIVector.hpp>
#include <Workphone/Workphone.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneRenderer.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WorkphoneCore/workphone.h>
#include <WorkphoneGraphics/workphone_graphics_system.h>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone, ClawUIManager, IUIManager );

    ClawUIManager::ClawUIManager() : m_itemInFocus( nullptr )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        m_workphoneContext = new ClawUIWorkphoneContext;
        m_workphoneRenderer = new ClawUIWorkphoneRenderer;

        if( !m_workphoneContext || !m_workphoneContext->isValid() )
        {
            WP_LOG_ERROR( "Failed to initialise WorkphoneCore UI context" );
        }
        else if( auto ctx = m_workphoneContext->getContext() )
        {
            m_workphoneRenderer->setNullTexture( m_workphoneContext->getNullTexture() );
            wp_input_begin( ctx );
            m_inputFrameOpen = true;
        }

        m_inputListener = factoryManager->make_ptr<InputListener>( this );

        auto inputMgr = applicationManager->getInputDeviceManager();
        if( inputMgr )
        {
            inputMgr->addListener( m_inputListener );
        }

        m_elements.reserve( 1024 );
    }

    ClawUIManager::~ClawUIManager()
    {
        unload( nullptr );
        if( m_workphoneContext )
        {
            delete m_workphoneContext;
            m_workphoneContext = nullptr;
        }

        if( m_workphoneRenderer )
        {
            delete m_workphoneRenderer;
            m_workphoneRenderer = nullptr;
        }
    }

    void ClawUIManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto factoryManager = workphone::make_ptr<FactoryManager>();
            setFactoryManager( factoryManager );

            if( !m_inputListener )
            {
                auto applicationManager = core::IApplicationManager::instance();
                if( applicationManager )
                {
                    m_inputListener = applicationManager->getFactoryManager()->make_ptr<InputListener>( this );
                    if( auto inputManager = applicationManager->getInputDeviceManager() )
                    {
                        inputManager->addListener( m_inputListener );
                    }
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            auto message = String( e.what() );
            WP_LOG_ERROR( message );
        }
    }

    void ClawUIManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            clear();

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto inputManager = applicationManager->getInputDeviceManager() )
                {
                    if( m_inputListener )
                    {
                        inputManager->removeListener( m_inputListener );
                    }
                }
            }
            m_inputListener = nullptr;
            m_factoryManager = nullptr;

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool ClawUIManager::loadFont( const String &fontPath, const String &type )
    {
        return false;
    }

    void ClawUIManager::unloadFont( const String &fontPath, const String &type )
    {
    }

    size_t ClawUIManager::messagePump( SmartPtr<ISharedObject> data )
    {
        return 0;
    }

    bool ClawUIManager::OnEvent( const SmartPtr<IInputEvent> &event )
    {
        // InputManager dispatches the same event to each task. Only the render task
        // may mutate the immediate-mode context that widgets consume during render().
        if( !event || Thread::getCurrentTask() != TaskId::Render )
        {
            return false;
        }

        if( m_workphoneContext && m_workphoneContext->isValid() )
        {
            auto ctx = m_workphoneContext->getContext();
            if( ctx )
            {
                switch( event->getEventType() )
                {
                case IInputEvent::EventType::Mouse:
                {
                    if( auto mouseState = event->getMouseState() )
                    {
                        auto position = mouseState->getAbsolutePosition();
                        bool inside = true;
                        auto applicationManager = core::IApplicationManager::instance();
                        if( auto sceneWindow = applicationManager->getSceneRenderWindow() )
                        {
                            const auto size = sceneWindow->getSize();
                            if( size.X() <= 0.0f || size.Y() <= 0.0f )
                            {
                                return false;
                            }

                            position -= sceneWindow->getPosition();
                            inside = position.X() >= 0.0f && position.Y() >= 0.0f &&
                                     position.X() < size.X() && position.Y() < size.Y();

                            // The image displayed in the editor can be scaled relative
                            // to the texture into which the game UI was rendered.
                            if( auto renderWindow = dynamic_pointer_cast<IUIRenderWindow>( sceneWindow ) )
                            {
                                if( auto texture = renderWindow->getRenderTexture() )
                                {
                                    const auto textureSize = texture->getSize();
                                    if( textureSize.x > 0 && textureSize.y > 0 )
                                    {
                                        position *= Vector2F( static_cast<f32>( textureSize.x ) / size.X(),
                                                             static_cast<f32>( textureSize.y ) / size.Y() );
                                    }
                                }
                            }
                        }
                        wp_input_motion( ctx, static_cast<int>( position.X() ),
                                         static_cast<int>( position.Y() ) );

                        const auto mouseEventType = mouseState->getEventType();
                        const wp_bool down = ( mouseEventType == IMouseState::Event::LeftPressed ||
                                               mouseEventType == IMouseState::Event::MiddlePressed ||
                                               mouseEventType == IMouseState::Event::RightPressed )
                                                 ? wp_true
                                                 : wp_false;

                        // Keep movement and releases outside the image so hover/drag
                        // state clears, but never start a click or scroll there.
                        if( !inside && ( down || mouseEventType == IMouseState::Event::Wheel ) )
                        {
                            return false;
                        }

                        if( mouseEventType == IMouseState::Event::LeftPressed ||
                            mouseEventType == IMouseState::Event::LeftReleased )
                        {
                            wp_input_button( ctx, WORKPHONE_BUTTON_LEFT,
                                             static_cast<int>( position.X() ),
                                             static_cast<int>( position.Y() ), down );
                        }
                        if( mouseEventType == IMouseState::Event::MiddlePressed ||
                            mouseEventType == IMouseState::Event::MiddleReleased )
                        {
                            wp_input_button( ctx, WORKPHONE_BUTTON_MIDDLE,
                                             static_cast<int>( position.X() ),
                                             static_cast<int>( position.Y() ), down );
                        }
                        if( mouseEventType == IMouseState::Event::RightPressed ||
                            mouseEventType == IMouseState::Event::RightReleased )
                        {
                            wp_input_button( ctx, WORKPHONE_BUTTON_RIGHT,
                                             static_cast<int>( position.X() ),
                                             static_cast<int>( position.Y() ), down );
                        }

                        if( mouseEventType == IMouseState::Event::Wheel )
                        {
                            const auto wheel = mouseState->getWheelDelta();
                            wp_input_scroll( ctx, { wheel.X(), wheel.Y() } );
                        }
                    }
                }
                break;
                case IInputEvent::EventType::Key:
                {
                    if( auto keyboardState = event->getKeyboardState() )
                    {
                        const bool isDown = keyboardState->isPressedDown();
                        wp_input_key( ctx, WORKPHONE_KEY_SHIFT,
                                      keyboardState->isShiftPressed() ? wp_true : wp_false );
                        wp_input_key( ctx, WORKPHONE_KEY_CTRL,
                                      keyboardState->isControlPressed() ? wp_true : wp_false );
                        switch( keyboardState->getKeyCode() )
                        {
                        case static_cast<u32>( KeyCodes::KEY_RETURN ):
                            wp_input_key( ctx, WORKPHONE_KEY_ENTER, isDown ? wp_true : wp_false );
                            break;
                        case static_cast<u32>( KeyCodes::KEY_BACK ):
                            wp_input_key( ctx, WORKPHONE_KEY_BACKSPACE, isDown ? wp_true : wp_false );
                            break;
                        case static_cast<u32>( KeyCodes::KEY_DELETE ):
                            wp_input_key( ctx, WORKPHONE_KEY_DEL, isDown ? wp_true : wp_false );
                            break;
                        case static_cast<u32>( KeyCodes::KEY_TAB ):
                            wp_input_key( ctx, WORKPHONE_KEY_TAB, isDown ? wp_true : wp_false );
                            break;
                        case static_cast<u32>( KeyCodes::KEY_LEFT ):
                            wp_input_key( ctx, WORKPHONE_KEY_LEFT, isDown ? wp_true : wp_false );
                            break;
                        case static_cast<u32>( KeyCodes::KEY_RIGHT ):
                            wp_input_key( ctx, WORKPHONE_KEY_RIGHT, isDown ? wp_true : wp_false );
                            break;
                        case static_cast<u32>( KeyCodes::KEY_UP ):
                            wp_input_key( ctx, WORKPHONE_KEY_UP, isDown ? wp_true : wp_false );
                            break;
                        case static_cast<u32>( KeyCodes::KEY_DOWN ):
                            wp_input_key( ctx, WORKPHONE_KEY_DOWN, isDown ? wp_true : wp_false );
                            break;
                        default:
                            break;
                        }

                        const auto character = keyboardState->getChar();
                        if( isDown && character >= 32 && character != 127 &&
                            !keyboardState->isControlPressed() )
                        {
                            wp_input_unicode( ctx, character );
                        }
                    }
                }
                break;
                default:
                    break;
                }
            }
        }

        auto elements = getElements();
        for( auto &element : elements )
        {
            if( element && !element->getParent() )
            {
                element->handleEvent( event );
            }
        }

        return false;
    }

    void ClawUIManager::update()
    {
        ScopedLock lock( this );

        if( m_workphoneContext && m_workphoneContext->isValid() )
        {
            if( auto ctx = m_workphoneContext->getContext() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                if( applicationManager )
                {
                    if( auto timer = applicationManager->getTimer() )
                    {
                        ctx->delta_time_seconds = static_cast<wp_f32>( timer->getDeltaTime() );
                    }
                }
            }
        }

        // Widgets draw during render(), after wp_begin has opened the root window.
    }

    void ClawUIManager::render()
    {
        if( !m_workphoneContext || !m_workphoneContext->isValid() )
        {
            return;
        }

        auto ctx = m_workphoneContext->getContext();
        if( !ctx )
        {
            return;
        }

        if( m_inputFrameOpen )
        {
            wp_input_end( ctx );
            m_inputFrameOpen = false;
        }

        // Begin a single root window that covers the viewport.  Individual containers
        // can override draw() to begin nested windows if required.
        auto applicationManager = core::IApplicationManager::instance();
        Vector2F windowSize( 1.0f, 1.0f );
        if( applicationManager )
        {
            if( auto mainWindow = applicationManager->getWindow() )
            {
                windowSize = Vector2F( static_cast<f32>( mainWindow->getSize().x ),
                                       static_cast<f32>( mainWindow->getSize().y ) );
            }
            if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
            {
                if( auto clawSystem = dynamic_cast<render::ClawHammerSystem *>( graphicsSystem.get() ) )
                {
                    auto renderer = wp_graphics_system_get_renderer( clawSystem->getNativeSystem() );
                    const auto viewport = wp_renderer_get_viewport( renderer );
                    if( viewport.width > 0 && viewport.height > 0 )
                    {
                        windowSize = Vector2F( static_cast<f32>( viewport.width ),
                                               static_cast<f32>( viewport.height ) );
                    }
                }
            }
        }

        struct wp_rect rootBounds;
        rootBounds.x = 0.0f;
        rootBounds.y = 0.0f;
        rootBounds.w = windowSize.X();
        rootBounds.h = windowSize.Y();

        // Use one transparent, input-capable root and let widgets push exact bounds.
        ctx->style.window.fixed_background =
            wp_style_item_color( ClawUIWorkphoneContext::toWorkphoneColor( ColourF( 0, 0, 0, 0 ) ) );
        ctx->style.window.background = ClawUIWorkphoneContext::toWorkphoneColor( ColourF( 0, 0, 0, 0 ) );
        wp_window_set_bounds( ctx, reinterpret_cast<const wp_c8 *>( "WPGraphicsRoot" ), rootBounds );

        if( wp_begin( ctx, reinterpret_cast<const wp_c8 *>( "WPGraphicsRoot" ), rootBounds,
                      WORKPHONE_WINDOW_NO_SCROLLBAR | WORKPHONE_WINDOW_BACKGROUND ) )
        {
            wp_layout_space_begin( ctx, WORKPHONE_STATIC, rootBounds.h, 1024 );

            if( auto root = getRoot() )
            {
                if( auto guiRoot = dynamic_cast<IWorkphoneWidget *>( root.get() ) )
                {
                    guiRoot->draw( ctx );
                }
            }
            else
            {
                auto elements = getElements();
                for( auto &element : elements )
                {
                    if( element && !element->getParent() )
                    {
                        if( auto guiElement = dynamic_cast<IWorkphoneWidget *>( element.get() ) )
                        {
                            guiElement->draw( ctx );
                        }
                    }
                }
            }

            wp_layout_space_end( ctx );
        }
        wp_end( ctx );

        if( m_workphoneRenderer )
        {
            if( m_workphoneRenderer->convert( ctx ) && applicationManager )
            {
                if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
                {
                    if( auto clawSystem =
                            dynamic_cast<render::ClawHammerSystem *>( graphicsSystem.get() ) )
                    {
                        auto nativeRenderer =
                            wp_graphics_system_get_renderer( clawSystem->getNativeSystem() );

                        auto fontPixels = m_workphoneContext->getFontPixels();
                        auto fontWidth = m_workphoneContext->getFontWidth();
                        auto fontHeight = m_workphoneContext->getFontHeight();
                        auto fontTexture = m_workphoneContext->getFontTexture();

                        m_workphoneRenderer->submit( ctx, nativeRenderer, fontPixels, fontWidth,
                                                     fontHeight, fontTexture );
                    }
                }
            }
        }

        wp_clear( ctx );
        wp_input_begin( ctx );
        m_inputFrameOpen = true;
    }

    SmartPtr<IUIApplication> ClawUIManager::addApplication()
    {
        return nullptr;
    }

    void ClawUIManager::removeApplication( SmartPtr<IUIApplication> application )
    {
        m_application = nullptr;
    }

    SmartPtr<IUIElement> ClawUIManager::addElement( hash64 type )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        SmartPtr<IUIElement> element;
        if( type == IUIButton::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIButton>();
        }
        else if( type == IUICheckbox::typeInfo() )
        {
            element = workphone::make_ptr<ClawUICheckBox>();
        }
        else if( type == IUIToggle::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIToggleButton>();
        }
        else if( type == IUILayoutWindow::typeInfo() )
        {
            element = workphone::make_ptr<ClawUILayout>();
        }
        else if( type == IUILayoutContainer::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIContainer>();
        }
        else if( type == IUIImage::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIImage>();
        }
        else if( type == IUIImageArray::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIImageArray>();
        }
        else if( type == IUITextEntry::typeInfo() )
        {
            element = workphone::make_ptr<ClawUITextEntry>();
        }
        else if( type == IUIText::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIText>();
        }
        else if( type == IUIMenu::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIMenu>();
        }
        else if( type == IUISpinner::typeInfo() )
        {
            element = workphone::make_ptr<ClawUISpinner>();
        }
        else if( type == IUIScrollingText::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIScrollingText>();
        }
        else if( type == IUIAnimatedMaterial::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIAnimatedMaterial>();
        }
        else if( type == IUIBar::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIBar>();
        }
        else if( type == IUIDial::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIDial>();
        }
        else if( type == IUICursor::typeInfo() )
        {
            element = workphone::make_ptr<ClawUICursor>();
        }
        else if( type == IUIVector3::typeInfo() )
        {
            element = workphone::make_ptr<ClawUIVector>();
        }

        if( element )
        {
            graphicsSystem->loadObject( element );
            m_elements.push_back( element );
            return element;
        }

        return nullptr;
    }

    SmartPtr<IUIElement> ClawUIManager::addElement( SmartPtr<IUIElement> parent, u8 type )
    {
        // auto eType = static_cast<ElementType>( type );
        // switch( eType )
        //{
        // case ElementType::Layout:
        //{
        //     auto element = fb::make_ptr<ClawUILayout>();
        //     element->load( nullptr );
        //     addElementPtr( element );
        //     return element;
        // }
        // break;
        // case ElementType::Container:
        //{
        //     auto element = fb::make_ptr<ClawUIContainer>();
        //     element->load( nullptr );
        //     addElementPtr( element );
        //     return element;
        // }
        // break;
        // case ElementType::Image:
        //{
        //     auto element = fb::make_ptr<ClawUIImage>();
        //     element->load( nullptr );
        //     addElementPtr( element );
        //     return element;
        // }
        // break;
        // case ElementType::Text:
        //{
        //     auto element = fb::make_ptr<ClawUIText>();
        //     element->load( nullptr );
        //     addElementPtr( element );
        //     return element;
        // }
        // break;
        // default:
        //{
        // }
        // }

        return nullptr;
    }

    void ClawUIManager::clear()
    {
        // Unload before releasing ownership to break parent/layout reference cycles.
        auto elements = getElements();
        for( auto &element : elements )
        {
            removeElement( element );
        }
        if( auto root = getRoot() )
        {
            root->unload( nullptr );
        }
        m_root = nullptr;
        m_itemInFocus = nullptr;
    }

    void ClawUIManager::removeElement( SmartPtr<IUIElement> element )
    {
        if( !element )
        {
            return;
        }
        element->remove();
        element->unload( nullptr );
        m_elements.erase( std::remove( m_elements.begin(), m_elements.end(), element ),
                          m_elements.end() );
        if( m_root == element )
        {
            m_root = nullptr;
        }
        if( m_itemInFocus == element )
        {
            m_itemInFocus = nullptr;
        }
    }

    void ClawUIManager::removeElements( const Array<SmartPtr<IUIElement>> &elements )
    {
        for( auto &element : elements )
        {
            removeElement( element );
        }
    }

    void ClawUIManager::setRoot( SmartPtr<IUIElement> root )
    {
        m_root = root;
    }

    SmartPtr<IUIElement> ClawUIManager::getRoot() const
    {
        return m_root;
    }

    void ClawUIManager::reloadCurrentLayout()
    {
        // const auto& children = m_root->getChildren();
        // for (u32 childIdx = 0; childIdx < children.size(); ++childIdx)
        //{
        //	SmartPtr<IUIElement> item;// = (IGUIElement*)children[childIdx];
        //	if (item->isVisible() && item->getType() == (String("Layout")))
        //	{
        //		for (u32 listenerIdx = 0; listenerIdx < m_managerListeners.size(); ++listenerIdx)
        //		{
        //			auto listener = m_managerListeners[listenerIdx];
        //			listener->OnStartReloadScripts();
        //		}

        //		String filePath = m_layoutFileMap[item->getName()];
        //		m_currentFileName = filePath;

        //		m_root->removeChild(item.get());
        //		//item.setNull();

        //		SmartPtr<IStream> stream =
        // IApplicationManager::instance()->getFileSystem()->open(filePath);

        //		TiXmlDocument* doc = new TiXmlDocument;
        //		doc->Parse(stream->getAsString().c_str());

        //		if (doc->Error())
        //		{
        //			String decription = String("Error - Could not load xml file: ") + filePath
        //				+ String("Error: ") + doc->ErrorDesc();
        //			WP_LOG_MESSAGE("GUI", decription.c_str());
        //			//MessageBoxUtil::show(decription.c_str());
        //			return;
        //		}

        //		TiXmlElement* root = doc->RootElement();
        //		if (root)
        //		{
        //			createGUIItem(m_root, root);
        //		}

        //		//m_root->getChildren().getLast()->setVisible(true);
        //		//m_root->getChildren().getLast()->setEnabled(true);

        //		for (u32 listenerIdx = 0; listenerIdx < m_managerListeners.size(); ++listenerIdx)
        //		{
        //			auto listener = m_managerListeners[listenerIdx];
        //			listener->OnFinishReloadScripts();
        //		}

        //		printf("Finished reloading gui.");
        //	}
        //}
    }

    SmartPtr<IUICursor> ClawUIManager::getCursor() const
    {
        return m_cursor;
    }

    SmartPtr<IUIElement> ClawUIManager::findElement( const String &id ) const
    {
        if( auto root = getRoot() )
        {
            SmartPtr<IUIElement> item = root->findChildById( id );
            if( !item )
            {
                String message = String( "Could not find gui item: " ) + id;
                WP_LOG_INFO( message.c_str() );
            }

            return item;
        }

        return nullptr;
    }

    SmartPtr<IUIApplication> ClawUIManager::getApplication() const
    {
        return nullptr;
    }

    void ClawUIManager::setApplication( SmartPtr<IUIApplication> application )
    {
    }

    Array<SmartPtr<IUIWindow>> ClawUIManager::getWindows() const
    {
        return m_windows;
    }

    void ClawUIManager::setWindows( Array<SmartPtr<IUIWindow>> windows )
    {
        m_windows = windows;
    }

    Array<SmartPtr<IUIRenderWindow>> ClawUIManager::getRenderWindows() const
    {
        return {};
    }

    void ClawUIManager::setRenderWindows( Array<SmartPtr<IUIRenderWindow>> renderWindows )
    {
    }

    Array<SmartPtr<IUIFileBrowser>> ClawUIManager::getFileBrowsers() const
    {
        return {};
    }

    void ClawUIManager::setFileBrowsers( Array<SmartPtr<IUIFileBrowser>> fileBrowsers )
    {
    }

    bool ClawUIManager::isDragging() const
    {
        return m_dragging;
    }

    void ClawUIManager::setDragging( bool dragging )
    {
        m_dragging = dragging;
    }

    SmartPtr<IUIWindow> ClawUIManager::getMainWindow() const
    {
        return m_uiWindow;
    }

    void ClawUIManager::setMainWindow( SmartPtr<IUIWindow> uiWindow )
    {
        m_uiWindow = uiWindow;
    }

    void ClawUIManager::lock()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        graphicsSystem->lock();
    }

    void ClawUIManager::unlock()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        graphicsSystem->unlock();
    }

    void ClawUIManager::_getObject( void **ppObject )
    {
    }

    SmartPtr<IFactoryManager> ClawUIManager::getFactoryManager() const
    {
        return m_factoryManager;
    }

    void ClawUIManager::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

    void ClawUIManager::invalidate()
    {
    }

    void ClawUIManager::unloadObject( SmartPtr<ISharedObject> graphicsObject,
                                      bool forceQueue /*= false */ )
    {
    }

    void ClawUIManager::loadObject( SmartPtr<ISharedObject> graphicsObject,
                                    bool forceQueue /*= false */ )
    {
    }

    void ClawUIManager::setOverlay( SmartPtr<ISharedObject> overlay )
    {
    }

    SmartPtr<ISharedObject> ClawUIManager::getOverlay() const
    {
        return nullptr;
    }

    struct wp_context *ClawUIManager::getContext() const
    {
        return m_workphoneContext->getContext();
    }

    ClawUIManager::InputListener::InputListener( ClawUIManager *mgr ) : m_mgr( mgr )
    {
    }

    Parameter ClawUIManager::InputListener::handleEvent( EventType eventType, hash_type eventValue,
                                                         const Array<Parameter> &arguments,
                                                         SmartPtr<ISharedObject> sender,
                                                         SmartPtr<ISharedObject> object,
                                                         SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::inputEvent )
        {
            m_mgr->OnEvent( event );
        }

        return {};
    }
}  // namespace workphone::ui
