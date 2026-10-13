#include <Workphone/Workphone.hpp>
#include "FoliageAuthoringContracts.hpp"
#include <cstdio>

int main( int argc, char **argv )
{
    workphone::TypeManager types;
    types.load();
    workphone::TypeManager::setInstance( &types );
    int result = 0;
    try
    {
        runFoliageAuthoringContracts( argc > 1 && workphone::String( argv[1] ) == "--plant-only" );
        std::puts( "Foliage plant and authoring contracts: PASS" );
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "Foliage authoring contracts failed: %s\n", error.what() );
        result = 1;
    }
    workphone::TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
