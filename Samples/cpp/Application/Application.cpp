#include "Application.h"
#include <Workphone/Workphone.hpp>

#ifdef _WP_STATIC_LIB_
#    include <FBOISInput/FBOISInput.hpp>

#    if WP_BUILD_PHYSX
#        include <FBPhysx/FBPhysx.hpp>
#    endif

#    include <WPSQLite/WPSQLite.hpp>

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.h>
#    endif

#    if WP_BUILD_IMGUI
#        include <FBImGui/FBImGui.hpp>
#    endif
#endif

namespace workphone
{

    /**
     * @brief Constructor for Application class
     */
    Application::Application()
    {
    }

    /**
     * @brief Destructor for Application class
     */
    Application::~Application()
    {
        unload( nullptr );
    }

    /**
     * @brief Loads the application and initializes core systems
     * @param data Optional shared data object
     */
    void Application::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
        core::IApplicationManager::setInstance( applicationManager );
        m_applicationManager = applicationManager;

        core::Application::load( data );

        setLoadingState( LoadingState::Loaded );
    }

    /**
     * @brief Unloads the application and cleans up resources
     * @param data Optional shared data object
     */
    void Application::unload( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            if( m_frameStatistics )
            {
                m_frameStatistics->unload( nullptr );
                m_frameStatistics = nullptr;
            }

            if( m_renderWindow )
            {
                m_renderWindow->unload( nullptr );
                m_renderWindow = nullptr;
            }

            if( m_application )
            {
                m_application->unload( nullptr );
                m_application = nullptr;
            }

            if( m_box )
            {
                m_box->unload( nullptr );
                m_box = nullptr;
            }

            if( m_node )
            {
                m_node->unload( nullptr );
                m_node = nullptr;
            }

            core::Application::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
    }

    /**
     * @brief Creates and initializes all necessary plugins for the application
     */
    void Application::createPlugins()
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

    /**
     * @brief Creates the main scene with UI elements and viewport
     */
    void Application::createScene()
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

    /**
     * @brief Creates and initializes the UI manager and main application window
     */
    void Application::createUI()
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

    /**
     * @brief Creates the render window for the application
     */
    void Application::createRenderWindow()
    {
        auto applicationManager = core::ApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        m_renderWindow = ui->addElementByType<ui::IUIRenderWindow>();
        WP_ASSERT( m_renderWindow );
    }

}  // namespace workphone
