#include <EditorPCH.hpp>
#include <EditorApplication.hpp>
#include <Workphone/Workphone.hpp>

using namespace workphone;

#if WP_EDITOR_TESTS
#    include <tests/EditorAllocationDiagnostics.hpp>
#    define BOOST_TEST_ALTERNATIVE_INIT_API
#    include <boost/test/unit_test.hpp>

int main( int argc, char *argv[] )
{
    try
    {
        workphone::editor::tests::initialiseAllocationDiagnostics();
        auto typeManager = workphone::make_shared<TypeManager>();
        typeManager->load();
        TypeManager::setInstance( typeManager.get() );

        extern bool init_unit_test();
        auto result =
            boost::unit_test::unit_test_main( &init_unit_test, argc, argv );

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
    try
    {
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
            else if( strcmp( argv[i], "--help" ) == 0 || strcmp( argv[i], "-h" ) == 0 )
            {
                printf( "Usage: LioncatEditor [--software|--dx11]\n" );
                return 0;
            }
            // TODO: Handle project path argument if needed
        }

        try
        {
            auto app = workphone::make_ptr<editor::EditorApplication>();

            // Set the renderer type for the application
            if( rendererExplicit ) app->setRendererType( rendererType );

            const auto maxThreads = 12;
            const auto numThreads = (s32)Thread::hardware_concurrency();
            const auto threads = Math<s32>::min( numThreads, maxThreads );
            app->setActiveThreads( threads );
            //app->setActiveThreads( 0 );

            app->setCreateFrameStatistics( false );
            app->setDebugMode( false );

#    if !WP_FINAL
            auto flags = app->getApplicationFlags();
            app->setApplicationFlags( flags | core::IApplication::developerModeFlag );
#    endif

            app->load( nullptr );
            app->run();
            app->unload( nullptr );
        }
        catch( const std::exception &e )
        {
            std::cout << e.what() << std::endl;
        }

        if( typeManager )
        {
            typeManager->unload();
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
        }
    }
    catch( const std::exception &e )
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
#endif
