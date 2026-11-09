#include "core/EditorSceneRefreshPolicy.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

using workphone::editor::EditorSceneRefreshPolicy;

static void require( bool value, const char *message )
{
    if( !value ) throw std::runtime_error( message );
}

int main()
{
    try
    {
        EditorSceneRefreshPolicy policy;
        require( policy.needsFrame( 0.0, false, false ), "Initial scene was not refreshed" );
        require( policy.needsFrame( 0.49, false, false ), "Interaction grace ended early" );
        require( !policy.needsFrame( 0.51, false, false ), "Idle scene kept rendering every frame" );
        require( policy.needsFrame( 0.56, false, false ), "Idle fallback did not refresh" );
        require( !policy.needsFrame( 0.57, false, false ), "Idle interval was ignored" );
        policy.requestActivity();
        require( policy.needsFrame( 0.58, false, false ), "Input did not wake the scene immediately" );
        require( policy.needsFrame( 0.59, false, false ), "Input grace was not retained" );
        require( policy.needsFrame( 2.0, false, false ), "Async changes could remain stale indefinitely" );
        require( !policy.needsFrame( 2.01, false, false ), "Idle policy did not resume" );
        require( policy.needsFrame( 2.02, false, true ), "Camera/target change did not wake immediately" );
        require( policy.needsFrame( 3.0, true, false ), "Play/realtime did not bypass idle policy" );
        require( policy.needsFrame( 3.001, true, false ), "Play/realtime was rate limited" );
        require( !policy.needsFrame( 3.002, false, false ), "Realtime disable did not resume idle policy" );
        require( policy.needsFrame( 0.0, false, false ), "Timer reset froze the scene" );
        require( policy.needsFrame( std::numeric_limits<double>::quiet_NaN(), false, false ),
                 "Invalid clock froze the scene" );
        require( policy.needsFrame( 10.0, false, false ), "Clock recovery did not refresh" );

        EditorSceneRefreshPolicy cadence;
        cadence.needsFrame( 0.0, false, false );
        unsigned frames = 0;
        for( unsigned i = 60; i < 180; ++i )
            frames += cadence.needsFrame( i / 120.0, false, false ) ? 1u : 0u;
        require( frames >= 13 && frames <= 16, "Idle scene cadence is outside the 15 Hz budget" );
        std::cout << "Editor scene refresh policy passed\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
