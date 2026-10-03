#include "MeshConverterPCH.hpp"
#include "MeshConverter.hpp"
#include <Workphone/Workphone.hpp>

#ifdef _WP_STATIC_LIB_
#    include <WPSQLite/WPSQLite.hpp>

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.h>
#    endif

#    if WP_BUILD_IMGUI
#        include <FBImGui/FBImGui.hpp>
#    endif

#    if WP_USE_ASSET_IMPORT
#        include <FBAssimp/FBAssimp.hpp>
#    endif

#    if WP_BUILD_OISINPUT
#        include "FBOISInput/FBOISInput.hpp"
#    endif

#    if WP_BUILD_PHYSX
#        include "FBPhysx/FBPhysx.hpp"
#    endif
#endif

namespace workphone
{
    namespace viewer
    {

        WP_CLASS_REGISTER_DERIVED( workphone::viewer, MeshViewer, Application );
        WP_CLASS_REGISTER_DERIVED( workphone::viewer, MeshViewer::CUIMenuBarListener, IEventListener );

        MeshViewer::MeshViewer()
        {
        }

        MeshViewer::~MeshViewer()
        {
            unload( nullptr );
        }

        void MeshViewer::load( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Loading );

            auto currentThread = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( currentThread );

            auto currentTask = TaskId::Primary;
            Thread::setCurrentTask( currentTask );

            auto taskFlags = std::numeric_limits<u32>::max();
            Thread::setTaskFlags( taskFlags );

            auto applicationManager = new core::ApplicationManager;
            core::ApplicationManager::setInstance( applicationManager );

            Application::load( data );

            setupUI();
            setupCamera();
            setupViewport();

            setLoadingState( LoadingState::Loaded );
        }

        void MeshViewer::unload( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            // Clean up menu bar listener
            if( m_menubarListener )
            {
                if( auto applicationManager = core::ApplicationManager::instance() )
                {
                    if( auto uiManager = applicationManager->getUI() )
                    {
                        if( m_application )
                        {
                            if( auto menuBar = m_application->getMenubar() )
                            {
                                menuBar->removeObjectListener( m_menubarListener );
                            }
                        }
                    }
                }

                m_menubarListener = nullptr;
            }

            // Clean up mesh actor
            if( m_meshActor )
            {
                m_meshActor->unload( nullptr );
                m_meshActor = nullptr;
            }

            // Clean up render target
            if( m_renderTarget )
            {
                m_renderTarget->unload( nullptr );
                m_renderTarget = nullptr;
            }

            // Clean up render window
            if( m_renderWindow )
            {
                m_renderWindow = nullptr;
            }

            // Clean up application
            if( m_application )
            {
                m_application = nullptr;
            }

            if( m_frameStatistics )
            {
                m_frameStatistics->unload( nullptr );
                m_frameStatistics = nullptr;
            }

            Application::unload( data );

            applicationManager->unload( nullptr );

            setLoadingState( LoadingState::Unloaded );
        }

        void MeshViewer::createPlugins()
        {
            setPluginsConfigFilePath( "wp_plugins_samples.cfg" );

            Application::createPlugins();

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
                auto graphicsPlugin = fb::make_ptr<render::WPGraphicsOgre>();
                applicationManager->addPlugin( graphicsPlugin );
#    endif

                ApplicationUtil::createFactories();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
#endif
        }

        SmartPtr<scene::IGameActor> MeshViewer::getMeshActor() const
        {
            return m_meshActor;
        }

        void MeshViewer::setMeshActor( SmartPtr<scene::IGameActor> meshActor )
        {
            if( m_meshActor )
            {
                m_meshActor->unload( nullptr );
            }

            m_meshActor = meshActor;

            if( m_meshActor )
            {
                centerCameraOnMesh();
            }
        }

        void MeshViewer::createUI()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            auto uiManager = factoryManager->make_object<ui::IUIManager>( "ImGui" );
            if( uiManager )
            {
                uiManager->load( nullptr );
                applicationManager->setUI( uiManager );

                auto application = uiManager->addApplication();
                uiManager->setApplication( application );
                m_application = application;
            }
        }

        void MeshViewer::createRenderWindow()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            if( ui )
            {
                m_renderWindow = ui->addElementByType<ui::IUIRenderWindow>();
            }
        }

        void MeshViewer::setupUI()
        {
            auto applicationManager = core::IApplicationManager::instance();
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

                auto menubarListener = workphone::make_ptr<CUIMenuBarListener>();
                menubarListener->setOwner( this );
                m_menubarListener = menubarListener;

                menuBar->addObjectListener( m_menubarListener );
            }
        }

        void MeshViewer::setupCamera()
        {
            if( auto camera = m_camera )
            {
                camera->setRenderUI( true );
            }
        }

        void MeshViewer::setupViewport()
        {
            if( auto vp = m_viewport )
            {
                vp->setEnableUI( true );
                vp->setEnableSceneRender( false );
            }
        }

        void MeshViewer::centerCameraOnMesh()
        {
            if( !m_meshActor || !m_camera )
            {
                return;
            }

            // Get mesh bounds and position camera to view the entire mesh
            // This is a placeholder implementation - actual implementation would
            // depend on your scene graph and camera API
            try
            {
                // Position camera to view the mesh
                // You may need to adjust this based on your coordinate system
                // and mesh size
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void MeshViewer::loadMeshFromFile( const String &filePath )
        {
            try
            {
                if( StringUtil::isNullOrEmpty( filePath ) )
                {
                    return;
                }

                auto meshActor = ApplicationUtil::loadMesh( filePath );
                setMeshActor( meshActor );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        //
        // CUIMenuBarListener Implementation
        //

        MeshViewer::CUIMenuBarListener::CUIMenuBarListener() = default;

        MeshViewer::CUIMenuBarListener::~CUIMenuBarListener() = default;

        Parameter MeshViewer::CUIMenuBarListener::handleEvent(
            EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
            SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager->getFileSystem();
            auto ui = applicationManager->getUI();

            if( auto owner = getOwner() )
            {
                if( eventValue == IEvent::handleSelection )
                {
                    auto element = workphone::static_pointer_cast<ui::IUIElement>( object );
                    auto widgetID = static_cast<ElementId>( element->getElementId() );

                    switch( widgetID )
                    {
                    case ElementId::Open:
                        handleOpenFile( owner, fileSystem );
                        break;
                    case ElementId::Exit:
                        handleExit();
                        break;
                    default:
                        break;
                    }
                }
            }

            return Parameter();
        }

        void MeshViewer::CUIMenuBarListener::handleOpenFile( SmartPtr<MeshViewer> owner,
                                                             SmartPtr<IFileSystem> fileSystem )
        {
            if( !fileSystem )
            {
                return;
            }

            auto fileDialog = fileSystem->openFileDialog();
            fileDialog->setFileExtension( ".fbx" );

            auto result = fileDialog->openDialog();
            if( result == INativeFileDialog::Result::Dialog_Okay )
            {
                auto filePath = fileDialog->getFilePath();
                if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    owner->loadMeshFromFile( filePath );
                }
            }
        }

        void MeshViewer::CUIMenuBarListener::handleExit()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            applicationManager->setQuit( true );
        }

        SmartPtr<MeshViewer> MeshViewer::CUIMenuBarListener::getOwner() const
        {
            return m_owner.lock();
        }

        void MeshViewer::CUIMenuBarListener::setOwner( SmartPtr<MeshViewer> owner )
        {
            m_owner = owner;
        }

    }  // end namespace viewer
}  // namespace workphone

#ifdef WP_PLATFORM_WIN32
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow )
{
    using namespace workphone;

    try
    {
        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            TypeManager::setInstance( typeManager );
        }

        workphone::viewer::MeshViewer app;
        app.load( nullptr );
        app.run();
        app.unload( nullptr );
        return 0;
    }
    catch( Exception &e )
    {
        workphone::MessageBoxUtil::show( e.what() );
    }

    return 0;
}
#else
int main()
{
    using namespace workphone;

    try
    {
        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            TypeManager::setInstance( typeManager );
        }

        workphone::viewer::MeshViewer app;
        app.setActiveThreads( 4 );
        app.load( nullptr );
        app.run();
        app.unload( nullptr );

        if( typeManager )
        {
            delete typeManager;
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
        }

        return 0;
    }
    catch( Exception &e )
    {
        workphone::MessageBoxUtil::show( e.what() );
    }

    return 0;
}
#endif