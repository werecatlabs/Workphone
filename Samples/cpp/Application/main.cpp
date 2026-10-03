#include "Application.h"

int main( int argc, char *argv[] )
{
    using namespace workphone;

    try
    {
        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            typeManager->load();
            TypeManager::setInstance( typeManager );
        }

        // Create application object
        Application app;
        app.setActiveThreads( 0 );

        app.load( nullptr );
        app.run();
        app.unload( nullptr );

        if( typeManager )
        {
            delete typeManager;
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
        }
    }
    catch( Exception &e )
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
