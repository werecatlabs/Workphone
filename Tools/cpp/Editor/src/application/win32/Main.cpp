#include <EditorPCH.hpp>
#include <EditorApplication.hpp>
#include <Workphone/Workphone.hpp>

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
    if( argc == 1 || ( argc > 1 && strcmp( argv[1], "--interactive" ) == 0 ) )
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
static int runInteractiveEditor( int argc, char *argv[] )
{
    try
    {
        for( int i = 1; i < argc; ++i )
        {
            if( strcmp( argv[i], "--help" ) == 0 || strcmp( argv[i], "-h" ) == 0 )
            {
                printf( "Usage: Editor [--interactive] [--software|--dx11] [--workspace PATH] [--package-root PATH] [--package-smoke] [--validation-capture-directory PATH]\n" );
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
            app->setActiveThreads( package.smoke ? 0 : threads );
            //app->setActiveThreads( 0 );

            app->setCreateFrameStatistics( false );
            app->setDebugMode( package.smoke );
            if( package.smoke ) app->setRendererType( render::IGraphicsSystem::RenderApi::Software );

#    if !WP_FINAL
            auto flags = app->getApplicationFlags();
            if( package.root.empty() )
                app->setApplicationFlags( flags | core::IApplication::developerModeFlag );
#    endif

            app->load( nullptr );
            try
            {
                if( !app->isLoaded() ) throw std::runtime_error( "Editor failed to load" );
                if( package.smoke ) package.verifyStartup( app );
                else app->run();
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
