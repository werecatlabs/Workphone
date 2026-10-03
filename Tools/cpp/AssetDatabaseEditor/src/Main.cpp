// ---------------------------------------------------------------------------
//  Main.cpp
//
//  Entry point for the C++17 port of the C# AssetDatabaseTool.  The tool
//  spins up the engine via @c TypeManager and the AssetDatabaseEditorApplication
//  which in turn brings up the renderer, ImGui-based UI and SQLite database
//  interfaces.
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <AssetDatabaseEditorApplication.hpp>
#include <Workphone/Workphone.hpp>

using namespace workphone;

int main( int argc, char *argv[] )
{
    try
    {
        auto typeManager = workphone::make_shared<TypeManager>();
        typeManager->load();
        TypeManager::setInstance( typeManager.get() );

        try
        {
            auto app = workphone::make_ptr<adbeditor::AssetDatabaseEditorApplication>();

            const auto maxThreads = 8;
            const auto numThreads = static_cast<s32>( Thread::hardware_concurrency() );
            const auto threads = Math<s32>::min( numThreads, maxThreads );
            app->setActiveThreads( threads );

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
            std::cerr << e.what() << std::endl;
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
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
