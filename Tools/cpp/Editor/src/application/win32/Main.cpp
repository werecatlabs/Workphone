#include <EditorPCH.hpp>
#include <EditorApplication.hpp>
#include <Workphone/Workphone.hpp>
#include <chrono>
#include <thread>

using namespace workphone;

static int runInteractiveEditor( int argc, char *argv[] );

#if WP_EDITOR_TESTS
#    include <tests/EditorAllocationDiagnostics.hpp>
#    define BOOST_TEST_ALTERNATIVE_INIT_API
#    include <boost/test/unit_test.hpp>

int main( int argc, char *argv[] )
{
    // A Debug build remains usable as an Editor. CTest supplies explicit Boost
    // arguments; launching the executable normally opens the real native UI.
    bool runtimeScene=false;for(int i=1;i<argc;++i)runtimeScene=runtimeScene||strncmp(argv[i],"--runtime-scene-",16)==0;
    if( argc == 1 || runtimeScene || ( argc > 1 && strcmp( argv[1], "--interactive" ) == 0 ) )
        return runInteractiveEditor( argc, argv );
    try
    {
        workphone::editor::tests::initialiseAllocationDiagnostics();
        auto typeManager = workphone::make_shared<TypeManager>();
        typeManager->load();
        TypeManager::setInstance( typeManager.get() );

        extern bool init_unit_test();
        auto result = boost::unit_test::unit_test_main( &init_unit_test, argc, argv );

        // Boost can translate a fatal SEH assertion without unwinding a fixture
        // constructor. Retire its remaining engine ownership while TypeManager's
        // object-data pools are still alive. The fixture's stack Application may
        // have been abandoned: use the singleton manager directly, never it.
        {
            auto orphanedManager = core::IApplicationManager::instancePtr();
            if( orphanedManager )
            {
                orphanedManager->unload( nullptr );
                core::IApplicationManager::setInstance( nullptr );
            }
        }

        if( typeManager )
        {
            typeManager->unload();
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
        }

#    if defined( _WIN32 ) && defined( _DEBUG )
        if( !_CrtCheckMemory() )
        {
            std::cerr << "Editor tests damaged the CRT heap." << std::endl;
            return EXIT_FAILURE;
        }
#    endif

        return result;
    }
    catch( const std::exception &e )
    {
        std::cout << e.what() << std::endl;
    }

    return EXIT_FAILURE;
}
#else
int main( int argc, char *argv[] )
{
    return runInteractiveEditor( argc, argv );
}
#endif

#    include "EditorPackage.hpp"
namespace
{
    // Validation is a bounded command-line operation, including arbitrary Lua
    // module/constructor execution which cannot yield to the owner pump timer.
    class RuntimeSceneValidationWatchdog
    {
    public:
        explicit RuntimeSceneValidationWatchdog(bool enabled)
        {
            if(!enabled)return;m_done=CreateEventW(nullptr,TRUE,FALSE,nullptr);
            if(!m_done)throw std::runtime_error("Cannot create runtime scene validation watchdog");
            try {m_worker=std::thread([done=m_done] {
                if(WaitForSingleObject(done,30000)!=WAIT_OBJECT_0) {
                    static constexpr char diagnostic[]="Runtime scene package startup exceeded 30 seconds\n";DWORD written=0;
                    WriteFile(GetStdHandle(STD_ERROR_HANDLE),diagnostic,sizeof(diagnostic)-1,&written,nullptr);
                    TerminateProcess(GetCurrentProcess(),EXIT_FAILURE);
                }
            });}catch(...){CloseHandle(m_done);m_done=nullptr;throw;}
        }
        ~RuntimeSceneValidationWatchdog(){stop();}
        void stop(){if(m_done){SetEvent(m_done);if(m_worker.joinable())m_worker.join();CloseHandle(m_done);m_done=nullptr;}}
    private:
        HANDLE m_done=nullptr;std::thread m_worker;
    };
}
static int runInteractiveEditor( int argc, char *argv[] )
{
    try
    {
        for( int i = 1; i < argc; ++i )
        {
            if( strcmp( argv[i], "--help" ) == 0 || strcmp( argv[i], "-h" ) == 0 )
            {
                printf( "Usage: Editor [--interactive] [--software|--dx11] [--workspace PATH] [--package-root PATH] [--package-smoke] [--validation-capture-directory PATH] [--runtime-scene-package PATH --runtime-scene-uuid UUID [--runtime-scene-target pc] [--runtime-scene-validate]]\n" );
                return EXIT_SUCCESS;
            }
        }
        editor::EditorPackage package( argc, argv );
        auto typeManager = workphone::make_shared<TypeManager>();
        typeManager->load();
        TypeManager::setInstance( typeManager.get() );

        // Default to DX11 renderer
        render::IGraphicsSystem::RenderApi rendererType = render::IGraphicsSystem::RenderApi::DX11;
        bool rendererExplicit = false;

        // Parse command-line arguments for renderer selection
        for( int i = 1; i < argc; ++i )
        {
            if( strcmp( argv[i], "" ) == 0 )
            {
                // Empty argument, skip
            }
            else if( strcmp( argv[i], "--software" ) == 0 )
            {
                rendererType = render::IGraphicsSystem::RenderApi::Software;
                rendererExplicit = true;
            }
            else if( strcmp( argv[i], "--dx11" ) == 0 )
            {
                rendererType = render::IGraphicsSystem::RenderApi::DX11;
                rendererExplicit = true;
            }
            // TODO: Handle project path argument if needed
        }

        int result = EXIT_SUCCESS;
        try
        {
            auto app = workphone::make_ptr<editor::PackagedEditorApplication>();
            const bool runtimeSceneValidation=!package.runtimeScenePackage.empty()&&(package.runtimeSceneValidate||package.smoke);
            if(!package.runtimeScenePackage.empty())app->configureRuntimeScenePackage(package.runtimeScenePackage.u8string().c_str(),package.runtimeSceneUuid.c_str(),package.runtimeSceneTarget.c_str());
            app->validationCapture.configure( package.validationCaptureDirectory );
            if( !package.root.empty() )
            {
                app->mediaDirectory = ( package.root / "Media" ).generic_string().c_str();
                app->pluginConfiguration = ( package.workspace / "wp_plugins_package.cfg" ).generic_string().c_str();
                app->catalogFile = ( package.workspace / "Cache/media-assets.db" ).generic_string().c_str();
            }
            else
            {
                app->mediaDirectory = app->Application::getMediaPath();
                app->pluginConfiguration = "wp_plugins_editor.cfg";
            }

            // Set the renderer type for the application
            if( rendererExplicit )
            {
                app->setRendererType( rendererType );
            }

            const auto maxThreads = 12;
            const auto numThreads = (s32)Thread::hardware_concurrency();
            const auto threads = Math<s32>::min( numThreads, maxThreads );
            app->setActiveThreads( package.smoke||runtimeSceneValidation ? 0 : threads );
            //app->setActiveThreads( 0 );

            app->setCreateFrameStatistics( false );
            app->setDebugMode( package.smoke||runtimeSceneValidation );
            if( package.smoke&&!runtimeSceneValidation ) app->setRendererType( render::IGraphicsSystem::RenderApi::Software );

#    if !WP_FINAL
            auto flags = app->getApplicationFlags();
            if( package.root.empty() )
                app->setApplicationFlags( flags | core::IApplication::developerModeFlag );
#    endif

            RuntimeSceneValidationWatchdog watchdog(runtimeSceneValidation);
            app->load( nullptr );
            try
            {
                if( !app->isLoaded() ) throw std::runtime_error( "Editor failed to load" );
                if( package.smoke ) package.verifyStartup( app );
                if(runtimeSceneValidation)
                {
                    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
                    while(!app->runtimeSceneComplete())
                    {
                        if(std::chrono::steady_clock::now()>=deadline){app->cancelRuntimeSceneStartup("Runtime scene package startup exceeded 30 seconds");break;}
                        app->iterate();Thread::sleep(.001);
                    }
                    watchdog.stop();
                    std::cout << "RuntimeSceneBootstrap=" << app->runtimeSceneReport() << std::endl;
                    if(!app->runtimeSceneSucceeded())throw std::runtime_error(app->runtimeSceneDiagnostic().c_str());
                    std::cout << "Runtime scene package loaded" << std::endl;
                }
                else if(!package.smoke)app->run();
            }
            catch( ... )
            {
                app->unload( nullptr );
                throw;
            }
            app->unload( nullptr );
            if( package.smoke ) std::cout << "Editor package startup and teardown passed" << std::endl;
        }
        catch( const std::exception &e )
        {
            std::cerr << e.what() << std::endl;
            result = EXIT_FAILURE;
        }

        if( typeManager )
        {
            typeManager->unload();
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
        }
        return result;
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << std::endl;
    }

    return EXIT_FAILURE;
}
