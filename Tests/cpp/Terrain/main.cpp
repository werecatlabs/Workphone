#include <Workphone/WorkphoneHeaders.hpp>
#include <Workphone/System/MessageBox.hpp>
#include "Terrain.hpp"

using namespace workphone;

int main( int argc, char *argv[] )
{
    auto typeManager = std::make_shared<TypeManager>();
    typeManager->load();
    TypeManager::setInstance( typeManager.get() );

    // Create application object
    Terrain app;

#if 1
    try
    {
        const auto threads = workphone::Thread::hardware_concurrency();
        //app.setActiveThreads( threads );
        app.setActiveThreads( 0 );

        app.load( nullptr );
        app.run();
    }
    catch( Exception &e )
    {
        auto message = e.what();
        MessageBoxUtil::show( message );
    }

#else
    app.load();
    app.run();
#endif

#if WP_ENABLE_MEMORY_TRACKER
    auto &memoryTracker = MemoryTracker::get();
    memoryTracker.reportLeaks();
#endif
}
