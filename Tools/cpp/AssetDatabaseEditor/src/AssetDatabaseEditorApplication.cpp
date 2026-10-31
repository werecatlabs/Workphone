// ---------------------------------------------------------------------------
//  AssetDatabaseEditorApplication.cpp
//
//  Default implementation of the asset database editor application.  The
//  class delegates engine bootstrap to the base @c core::Application and
//  instantiates the editor main window on top of the engine subsystems.
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorApplication.hpp>
#include <ui/AssetDatabaseEditorMainWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, AssetDatabaseEditorApplication, core::Application );

    AssetDatabaseEditorApplication::AssetDatabaseEditorApplication()
    {
        static const String applicationName = "AssetDatabaseEditor";
        setName( applicationName );
    }

    AssetDatabaseEditorApplication::~AssetDatabaseEditorApplication() = default;

    void AssetDatabaseEditorApplication::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
        core::IApplicationManager::setInstance( applicationManager );
        m_applicationManager = applicationManager;

        // Bring up the engine subsystems (graphics, ImGui UI, file system,
        // database interface, ...).  This mirrors the bootstrap the in-progress
        // Editor uses so that every interface referenced by the ported UI
        // (UIManager, AssetDatabaseManager, ...) is available.
        core::Application::load( data );

        m_mainWindow = workphone::make_ptr<AssetDatabaseEditorMainWindow>();
        m_mainWindow->load( data );

        setLoadingState( LoadingState::Loaded );
    }

    void AssetDatabaseEditorApplication::unload( SmartPtr<ISharedObject> data )
    {
        if( m_mainWindow )
        {
            m_mainWindow->unload( data );
            m_mainWindow = nullptr;
        }

        core::Application::unload( data );
    }

    void AssetDatabaseEditorApplication::createPlugins()
    {
        static const String defaultFilePath = "wp_plugins_samples.cfg";
        auto configFilePath = defaultFilePath;

        setPluginsConfigFilePath( configFilePath );

        core::Application::createPlugins();

#ifdef _WP_STATIC_LIB_
        try
        {
            WP_DEBUG_TRACE;

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto corePlugin = workphone::make_ptr<WPCore>();
            corePlugin->load( nullptr );

            auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
            applicationManager->addPlugin( databasePlugin );

            auto inputPlugin = workphone::make_ptr<OISInput>();
            applicationManager->addPlugin( inputPlugin );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
            applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgre>();
            applicationManager->addPlugin( graphicsPlugin );
#    endif

#    if WP_BUILD_PHYSX
            auto physxPlugin = workphone::make_ptr<physics::FBPhysx>();
            applicationManager->addPlugin( physxPlugin );
#    elif WP_BUILD_ODE
            // Add ODE physics plugin if needed
#    endif

            ApplicationUtil::createFactories();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
#endif
    }

    void AssetDatabaseEditorApplication::createScene()
    {
        auto applicationManager = core::ApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto uiManager = applicationManager->getUI();
        if( uiManager )
        {
            auto menuBar = uiManager->addElementByType<ui::IUIMenubar>();

            auto fileMenu = uiManager->addElementByType<ui::IUIMenu>();
            fileMenu->setLabel( "File" );
            menuBar->addMenu( fileMenu );

            Util::addMenuItem( fileMenu, static_cast<s32>( ElementId::Open ), "Open", "Open",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( ElementId::Exit ), "Exit", "Exit",
                               ui::IUIMenuItem::Type::Normal );

            m_application->setMenubar( menuBar );
        }

        if( auto camera = m_camera )
        {
            camera->setRenderUI( true );
        }

        if( auto vp = m_viewport )
        {
            vp->setOverlaysEnabled( false );
            vp->setEnableUI( true );
            vp->setEnableSceneRender( false );
        }

        // Create frame statistics display
        if( !m_frameStatistics )
        {
            auto factoryManager = applicationManager->getFactoryManager();
            if( factoryManager )
            {
                m_frameStatistics = factoryManager->make_object<IFrameStatistics>();
                if( m_frameStatistics )
                {
                    m_frameStatistics->load( nullptr );
                }
            }
        }
    }

    void AssetDatabaseEditorApplication::createUI()
    {
        auto applicationManager = core::ApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto uiManager = factoryManager->make_object<ui::IUIManager>( "ImGui" );
        WP_ASSERT( uiManager );

        uiManager->load( nullptr );
        applicationManager->setUI( uiManager );

        auto application = uiManager->addApplication();
        WP_ASSERT( application );

        uiManager->setApplication( application );
        m_application = application;
    }

    void AssetDatabaseEditorApplication::run()
    {
        core::Application::run();
    }

    void AssetDatabaseEditorApplication::iterate()
    {
        core::Application::iterate();
    }
}  // namespace workphone::adbeditor
