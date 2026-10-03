#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiManager.hpp>
#include <WPImGui/ImGuiApplication.hpp>
#include <WPImGui/ImGuiButton.hpp>
#include <WPImGui/ImGuiCollapsingHeader.hpp>
#include <WPImGui/ImGuiColourPicker.hpp>
#include <WPImGui/ImGuiDataGrid.hpp>
#include <WPImGui/ImGuiFileBrowser.hpp>
#include <WPImGui/ImGuiImage.hpp>
#include <WPImGui/ImGuiInputManager.hpp>
#include <WPImGui/ImGuiRenderWindow.hpp>
#include <WPImGui/ImGuiMenu.hpp>
#include <WPImGui/ImGuiMenuBar.hpp>
#include <WPImGui/ImGuiMenuItem.hpp>
#include <WPImGui/ImGuiProfilerWindow.hpp>
#include <WPImGui/ImGuiProfileWindow.hpp>
#include <WPImGui/ImGuiTreeCtrl.hpp>
#include <WPImGui/ImGuiTreeNode.hpp>
#include <WPImGui/ImGuiText.hpp>
#include <WPImGui/ImGuiTextEntry.hpp>
#include <WPImGui/ImGuiToggleButton.hpp>
#include <WPImGui/ImGuiWindow.hpp>
#include <WPImGui/ImGuiPropertyGrid.hpp>
#include <WPImGui/ImGuiDropdown.hpp>
#include <WPImGui/ImGuiLabelTogglePair.hpp>
#include <WPImGui/ImGuiLabelDropdownPair.hpp>
#include <WPImGui/ImGuiLabelSliderPair.hpp>
#include <WPImGui/ImGuiLabelTextInputPair.hpp>
#include <WPImGui/ImGuiMaskEditor.hpp>
#include <WPImGui/ImGuiVector2.hpp>
#include <WPImGui/ImGuiVector3.hpp>
#include <WPImGui/ImGuiVector4.hpp>
#include <WPImGui/ImGuiSeparator.hpp>
#include <WPImGui/ImGuiSearchBar.hpp>
#include <WPImGui/ImGuiToolbar.hpp>
#include <WPImGui/ImGuiTerrainEditor.hpp>
#include <WPImGui/ImGuiEventWindow.hpp>
#include <WPImGui/ImGuiAbout.hpp>
#include <WPImGui/ImGuiTabBar.hpp>
#include <WPImGui/ImGuiTabItem.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiManager, IUIManager );

    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

    ImGuiManager::ImGuiManager()
    {
        auto typeManager = TypeManager::instance();
        typeManager->setDataTypeEnum<IUIApplication>( UITypes::Application );
        typeManager->setDataTypeEnum<IUIButton>( UITypes::Button );
        typeManager->setDataTypeEnum<IUIToolbar>( UITypes::Toolbar );
    }

    ImGuiManager::~ImGuiManager()
    {
        unload( nullptr );
    }

    void ImGuiManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto factoryManager = workphone::make_ptr<FactoryManager>();
            factoryManager->load( nullptr );
            setFactoryManager( factoryManager );

            FactoryUtil::addFactory<ImGuiAbout>( factoryManager );
            FactoryUtil::addFactory<ImGuiApplication>( factoryManager );
            FactoryUtil::addFactory<ImGuiButton>( factoryManager );
            FactoryUtil::addFactory<ImGuiCollapsingHeader>( factoryManager );
            FactoryUtil::addFactory<ImGuiColourPicker>( factoryManager );
            FactoryUtil::addFactory<ImGuiDataGrid>( factoryManager );
            FactoryUtil::addFactory<ImGuiDropdown>( factoryManager );
            FactoryUtil::addFactory<ImGuiEventWindow>( factoryManager );
            FactoryUtil::addFactory<ImGuiFileBrowser>( factoryManager );
            FactoryUtil::addFactory<ImGuiImage>( factoryManager );
            FactoryUtil::addFactory<ImGuiInputManager>( factoryManager );
            FactoryUtil::addFactory<ImGuiLabelTogglePair>( factoryManager );
            FactoryUtil::addFactory<ImGuiLabelDropdownPair>( factoryManager );
            FactoryUtil::addFactory<ImGuiLabelSliderPair>( factoryManager );
            FactoryUtil::addFactory<ImGuiLabelTextInputPair>( factoryManager );
            FactoryUtil::addFactory<ImGuiMaskEditor>( factoryManager );
            FactoryUtil::addFactory<ImGuiMenu>( factoryManager );
            FactoryUtil::addFactory<ImGuiMenuBar>( factoryManager );
            FactoryUtil::addFactory<ImGuiMenuItem>( factoryManager );
            FactoryUtil::addFactory<ImGuiProfilerWindow>( factoryManager );
            FactoryUtil::addFactory<ImGuiProfileWindow>( factoryManager );
            FactoryUtil::addFactory<ImGuiPropertyGrid>( factoryManager );
            FactoryUtil::addFactory<ImGuiRenderWindow>( factoryManager );
            FactoryUtil::addFactory<ImGuiSearchBar>( factoryManager );
            FactoryUtil::addFactory<ImGuiSeparator>( factoryManager );
            FactoryUtil::addFactory<ImGuiTabBar>( factoryManager );
            FactoryUtil::addFactory<ImGuiTabItem>( factoryManager );
            FactoryUtil::addFactory<ImGuiTerrainEditor>( factoryManager );
            FactoryUtil::addFactory<ImGuiText>( factoryManager );
            FactoryUtil::addFactory<ImGuiTextEntry>( factoryManager );
            FactoryUtil::addFactory<ImGuiToggleButton>( factoryManager );
            FactoryUtil::addFactory<ImGuiToolbar>( factoryManager );
            FactoryUtil::addFactory<ImGuiTreeCtrl>( factoryManager );
            FactoryUtil::addFactory<ImGuiTreeNode>( factoryManager );
            FactoryUtil::addFactory<ImGuiVector2>( factoryManager );
            FactoryUtil::addFactory<ImGuiVector3>( factoryManager );
            FactoryUtil::addFactory<ImGuiVector4>( factoryManager );
            FactoryUtil::addFactory<ImGuiWindow>( factoryManager );

            auto poolSize = 4096;
            //factoryManager->setPoolSizeByType<ImGuiAbout>( poolSize );
            //factoryManager->setPoolSizeByType<ImGuiApplication>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiButton>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiCollapsingHeader>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiColourPicker>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiDropdown>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiEventWindow>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiFileBrowser>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiImage>( poolSize );
            //factoryManager->setPoolSizeByType<ImGuiInputManager>( poolSize );

            factoryManager->setPoolSizeByType<ImGuiLabelTogglePair>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiLabelDropdownPair>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiLabelSliderPair>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiLabelTextInputPair>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiMaskEditor>( poolSize );

            //factoryManager->setPoolSizeByType<ImGuiInputManager>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiMenu>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiMenuBar>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiMenuItem>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiProfilerWindow>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiProfileWindow>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiPropertyGrid>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiRenderWindow>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiSearchBar>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiSeparator>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiTabBar>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiTabItem>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiTerrainEditor>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiText>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiTextEntry>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiToggleButton>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiToolbar>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiTreeCtrl>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiTreeNode>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiVector2>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiVector3>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiVector4>( poolSize );
            factoryManager->setPoolSizeByType<ImGuiWindow>( poolSize );

            factoryManager->allocateData();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ImGuiManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                // Hold every control alive while unload breaks child, listener and
                // script references. Otherwise unloading one tree can reclaim a
                // control that a later factory/index entry still refers to.
                auto factoryManager = getFactoryManager();
                Array<SmartPtr<ISharedObject>> controls;
                if( factoryManager )
                {
                    for( auto &factory : factoryManager->getFactories() )
                    {
                        auto objects = factory->getInstanceObjects();
                        controls.insert( controls.end(), objects.begin(), objects.end() );
                    }
                }
                for( auto &control : controls )
                {
                    control->unload( data );
                }

                auto elementsByType = getElementsByType();
                for( auto p : elementsByType )
                {
                    p->clear();
                }

                elementsByType.clear();
                m_elements.clear();

                if( m_uiWindow )
                {
                    m_uiWindow->unload( data );
                    m_uiWindow = nullptr;
                }

                if( m_application )
                {
                    m_application->unload( data );
                    m_application = nullptr;
                }

                // Releasing these references runs normal destruction and removes
                // controls from their factory inventories before bulk teardown.
                controls.clear();
                if( factoryManager )
                {
                    factoryManager->unload( data );
                    setFactoryManager( nullptr );
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    size_Num ImGuiManager::messagePump( SmartPtr<ISharedObject> data )
    {
        auto application = workphone::static_pointer_cast<ImGuiApplication>( getApplication() );
        if( application )
        {
            return application->messagePump( data );
        }

        return 0;
    }

    void ImGuiManager::handleWindowEvent( SmartPtr<render::IGraphicsWindowEvent> event )
    {
    }

    SmartPtr<IUIApplication> ImGuiManager::addApplication()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto application = factoryManager->make_ptr<ImGuiApplication>();
        application->load( nullptr );
        return application;
    }

    void ImGuiManager::removeApplication( SmartPtr<IUIApplication> application )
    {
        application->unload( nullptr );
    }

    SmartPtr<IUIElement> ImGuiManager::addElement( hash64 type )
    {
        //WP_ASSERT( isLoaded() );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = getFactoryManagerPtr();

        auto element = SmartPtr<IUIElement>();

        auto typeManager = TypeManager::instance();
        auto eType = typeManager->getDataTypeEnum<UITypes>( static_cast<u32>( type ) );
        switch( eType )
        {
        case UITypes::Application:
        {
            auto application = factoryManager->make_ptr<ImGuiApplication>();
            application->load( nullptr );
            return application;
        }
        break;
        case UITypes::Button:
        {
            element = factoryManager->make_ptr<ImGuiButton>();
        }
        break;
        case UITypes::Toolbar:
        {
            element = factoryManager->make_ptr<ImGuiToolbar>();
        }
        break;
        case UITypes::None:
        default:
        {
            const auto BUTTON_TYPEINFO = IUIButton::typeInfo();
            const auto TEXT_TYPEINFO = IUIText::typeInfo();
            const auto TREECTRL_TYPEINFO = IUITreeCtrl::typeInfo();
            const auto TREE_NODE_TYPEINFO = IUITreeNode::typeInfo();
            const auto WINDOW_TYPEINFO = IUIWindow::typeInfo();

            // Special cases
            if( type == BUTTON_TYPEINFO )
            {
                element = factoryManager->make_ptr<ImGuiButton>();
            }
            else if( type == TEXT_TYPEINFO )
            {
                element = factoryManager->make_ptr<ImGuiText>();
            }
            else if( type == TREECTRL_TYPEINFO )
            {
                element = factoryManager->make_ptr<ImGuiTreeCtrl>();
            }
            else if( type == TREE_NODE_TYPEINFO )
            {
                element = factoryManager->make_ptr<ImGuiTreeNode>();
            }
            else if( type == WINDOW_TYPEINFO )
            {
                element = factoryManager->make_ptr<ImGuiWindow>();
            }
            else
            {
                element = factoryManager->make_object<IUIElement>( (u32)type );
            }
        }
        break;
        }

        if( element )
        {
            element->load( nullptr );
            addElement( static_cast<u32>( type ), element );
            return element;
        }

        return nullptr;
    }

    void ImGuiManager::removeElement( SmartPtr<IUIElement> p )
    {
        if( isLoaded() )
        {
            if( p )
            {
                auto children = p->getChildren();
                for( auto &child : children )
                {
                    removeElement( child );
                }

                if( auto parent = p->getParent() )
                {
                    parent->removeChild( p );
                }

                auto type = p->getTypeInfo();
                removeElement( type, p );

                // Remove every manager-held reference before unload can return the element to
                // its object pool.
                unloadObject( p );
            }
        }
    }

    void ImGuiManager::removeElements( const Array<SmartPtr<IUIElement>> &elementsToRemove )
    {
        if( isLoaded() )
        {
            ScopedLock lock( this );

            for( auto p : elementsToRemove )
            {
                removeElement( p );
            }
        }
    }

    void ImGuiManager::clear()
    {
    }

    void ImGuiManager::reloadCurrentLayout()
    {
    }

    SmartPtr<IUICursor> ImGuiManager::getCursor() const
    {
        return nullptr;
    }

    SmartPtr<IUIElement> ImGuiManager::findElement( const String &id ) const
    {
        return nullptr;
    }

    IUIApplication *ImGuiManager::getApplicationPtr() const
    {
        return m_application.get();
    }

    SmartPtr<IUIApplication> ImGuiManager::getApplication() const
    {
        return m_application;
    }

    void ImGuiManager::setApplication( SmartPtr<IUIApplication> application )
    {
        m_application = application;
    }

    Array<SmartPtr<IUIWindow>> ImGuiManager::getWindows() const
    {
        const auto WINDOW_TYPEINFO = IUIWindow::typeInfo();
        if( auto p = getElementsPtr( WINDOW_TYPEINFO ) )
        {
            auto &elements = *p;
            return Array<SmartPtr<IUIWindow>>( elements.begin(), elements.end() );
        }

        return {};
    }

    void ImGuiManager::setWindows( Array<SmartPtr<IUIWindow>> windows )
    {
        //m_windows = windows;
    }

    Array<SmartPtr<IUIFileBrowser>> ImGuiManager::getFileBrowsers() const
    {
        const auto FileBrowser_TYPEINFO = IUIFileBrowser::typeInfo();
        if( auto p = getElementsPtr( FileBrowser_TYPEINFO ) )
        {
            auto &elements = *p;
            return Array<SmartPtr<IUIFileBrowser>>( elements.begin(), elements.end() );
        }

        return {};
    }

    void ImGuiManager::setFileBrowsers( Array<SmartPtr<IUIFileBrowser>> fileBrowsers )
    {
        //m_fileBrowsers = fileBrowsers;
    }

    Array<SmartPtr<IUIRenderWindow>> ImGuiManager::getRenderWindows() const
    {
        const auto RENDERWINDOW_TYPEINFO = IUIRenderWindow::typeInfo();
        if( auto p = getElementsPtr( RENDERWINDOW_TYPEINFO ) )
        {
            auto &elements = *p;
            return Array<SmartPtr<IUIRenderWindow>>( elements.begin(), elements.end() );
        }

        return {};
    }

    void ImGuiManager::setRenderWindows( Array<SmartPtr<IUIRenderWindow>> renderWindows )
    {
        //m_renderWindows = renderWindows;
    }

    bool ImGuiManager::isDragging() const
    {
        return m_dragging;
    }

    void ImGuiManager::setDragging( bool dragging )
    {
        m_dragging = dragging;
    }

    SmartPtr<IUIWindow> ImGuiManager::getMainWindow() const
    {
        return m_uiWindow;
    }

    void ImGuiManager::setMainWindow( SmartPtr<IUIWindow> uiWindow )
    {
        m_uiWindow = uiWindow;
    }

    void ImGuiManager::_getObject( void **ppObject )
    {
        *ppObject = nullptr;
    }

    SmartPtr<IFactoryManager> ImGuiManager::getFactoryManager() const
    {
        return m_factoryManager;
    }

    void ImGuiManager::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

    void ImGuiManager::addElement( u32 type, SmartPtr<IUIElement> node )
    {
        ScopedLock lock( this );

        auto p = getElementsPtr( type );
        if( !p )
        {
            p = workphone::make_shared<Array<SmartPtr<IUIElement>>>();
            setElementsPtr( type, p );
        }

        if( p )
        {
            auto &elements = *p;
            elements.push_back( node );
        }
    }

    void ImGuiManager::removeElement( u32 type, SmartPtr<IUIElement> node )
    {
        ScopedLock lock( this );

        // Elements are indexed by the interface type requested by addElement(), while
        // getTypeInfo() returns the concrete implementation type. Templated UI classes do
        // not necessarily expose the requested interface in the TypeManager's single-base
        // hierarchy, so remove the pointer from every small per-type bucket.
        for( auto &entry : m_elements )
        {
            auto p = entry.second.load();
            if( p )
            {
                auto &elements = *p;
                elements.erase( std::remove( elements.begin(), elements.end(), node ),
                                elements.end() );
            }
        }
    }

    SharedPtr<Array<SmartPtr<IUIElement>>> ImGuiManager::getElementsPtr( u32 type ) const
    {
        auto it = m_elements.find( type );
        if( it != m_elements.end() )
        {
            return it->second;
        }

        return nullptr;
    }

    void ImGuiManager::setElementsPtr( u32 type, SharedPtr<Array<SmartPtr<IUIElement>>> p )
    {
        ScopedLock lock( this );
        m_elements[type] = p;
    }

    Array<SharedPtr<Array<SmartPtr<IUIElement>>>> ImGuiManager::getElementsByType() const
    {
        ScopedLock lock( this );

        Array<SharedPtr<Array<SmartPtr<IUIElement>>>> elements;
        for( auto &entry : m_elements )
        {
            auto &p = entry.second;
            elements.push_back( p.load() );
        }

        return elements;
    }

    void ImGuiManager::unlock()
    {
        m_mutex.unlock();
    }

    void ImGuiManager::lock()
    {
        m_mutex.lock();
    }

    void ImGuiManager::unloadObject( SmartPtr<ISharedObject> graphicsObject,
                                     bool forceQueue /*= false */ )
    {
        ScopedLock lock( this );
        graphicsObject->unload( nullptr );
    }

    void ImGuiManager::loadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue /*= false */ )
    {
        ScopedLock lock( this );
        graphicsObject->load( nullptr );
    }

    void ImGuiManager::invalidate()
    {
    }

    void ImGuiManager::unloadFontAwesomeFont()
    {
        ImGuiApplication::s_fontData.clear();
        ImGuiApplication::s_fontAwesomeFont = nullptr;
    }

    bool ImGuiManager::loadFontAwesomeFont( const String &fontPath )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto &fontData = ImGuiApplication::s_fontData;
            auto &fontAwesomeFont = ImGuiApplication::s_fontAwesomeFont;

            auto fileStream = fileSystem->open( fontPath, true, true, false, true, true );
            if( !fileStream )
            {
                WP_LOG_ERROR( "Failed to open Font Awesome font file: " + fontPath );
                return false;
            }

            // Get file size
            auto fileSize = fileStream->size();
            fontData.resize( fileSize );

            // Read file data
            auto bytesRead = fileStream->read( fontData.data(), fileSize );
            if( bytesRead != fileSize )
            {
                WP_LOG_ERROR( "Failed to read Font Awesome font file: " + fontPath );
                return false;
            }

            // Add font to ImGui
            ImGuiIO &io = ImGui::GetIO();

            ImFontConfig config;
            config.MergeMode = true;
            config.PixelSnapH = true;
            // The backing bytes live in ImGuiApplication::s_fontData and were
            // not allocated by ImGui, so the atlas must not try to free them.
            config.FontDataOwnedByAtlas = false;
            //config.GlyphMinAdvanceX = 13.0f;
            //config.GlyphMaxAdvanceX = 400.0f;
            //config.GlyphOffset.y = 1.0f;

            // Add Font Awesome font with specific icon ranges
            fontAwesomeFont = io.Fonts->AddFontFromMemoryTTF(
                fontData.data(), static_cast<int>( fontData.size() ), 12.0f, &config,
                icons_ranges  // Use our custom icon ranges
            );

            if( !fontAwesomeFont )
            {
                WP_LOG_ERROR( "Failed to add Font Awesome font to ImGui" );
                return false;
            }

            // Build font atlas
            io.Fonts->Build();

            return true;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            return false;
        }
    }

    bool ImGuiManager::loadFont( const String &fontPath, const String &type )
    {
        return loadFontAwesomeFont( fontPath );
    }

    void ImGuiManager::unloadFont( const String &fontPath, const String &type )
    {
        unloadFontAwesomeFont();
    }

    void ImGuiManager::setOverlay( SmartPtr<ISharedObject> overlay )
    {
        m_overlay = overlay;
    }

    SmartPtr<ISharedObject> ImGuiManager::getOverlay() const
    {
        return m_overlay;
    }

    bool ImGuiManager::InputListener::inputEvent( SmartPtr<IInputEvent> event )
    {
        return false;
    }

    bool ImGuiManager::InputListener::updateEvent( const SmartPtr<IInputEvent> &event )
    {
        return false;
    }

    void ImGuiManager::InputListener::setPriority( s32 priority )
    {
    }

    s32 ImGuiManager::InputListener::getPriority() const
    {
        return 0;
    }

    void ImGuiManager::InputListener::setOwner( ImGuiManager *owner )
    {
        m_owner = owner;
    }

    ImGuiManager *ImGuiManager::InputListener::getOwner() const
    {
        return m_owner;
    }

    void ImGuiManager::WindowListener::setOwner( ImGuiManager *owner )
    {
        m_owner = owner;
    }

    ImGuiManager *ImGuiManager::WindowListener::getOwner() const
    {
        return m_owner;
    }

    Parameter ImGuiManager::WindowListener::handleEvent( EventType eventType, hash_type eventValue,
                                                         const Array<Parameter> &arguments,
                                                         SmartPtr<ISharedObject> sender,
                                                         SmartPtr<ISharedObject> object,
                                                         SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            owner->handleWindowEvent( event );
        }

        return Parameter();
    }

    ImGuiManager::WindowListener::WindowListener() = default;

    ImGuiManager::WindowListener::~WindowListener() = default;
}  // namespace workphone::ui
