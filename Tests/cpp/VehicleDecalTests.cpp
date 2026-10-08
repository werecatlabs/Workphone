#include "SkidDecalPool.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace workphone::advanced;
void check( bool value, const char *message )
{
    if( !value )
    {
        std::fprintf( stderr, "%s\n", message );
        std::exit( 1 );
    }
}
int main()
{
    SkidDecalPool marks( 3 );
    DecalPoint up{ 0, 1, 0 };
    check( !marks.sample( 0, { 0, 0, 0 }, up, .3f, 1 ), "First contact must not draw a mark" );
    check( !marks.sample( 0, { .01f, 0, 0 }, up, .3f, 1 ), "Stationary jitter must not draw a mark" );
    check( marks.sample( 0, { .3f, 0, 0 }, up, .3f, 1 ), "Moving contact must draw a mark" );
    check( !marks.sample( 0, { 20, 0, 0 }, up, .3f, 1 ), "Teleports must not bridge the scene" );
    marks.breakTrail( 0 );
    check( !marks.sample( 0, { 20.3f, 0, 0 }, up, .3f, 1 ),
           "Airborne or grip recovery must break trails" );
    for( int i = 1; i <= 20; ++i )
        marks.sample( 0, { 20.3f + i * .3f, 0, 0 }, up, .3f, 1 );
    check( marks.marks().size() == 3, "Decal storage must stay within its budget" );
    marks.advance( 18 );
    check( std::abs( SkidDecalPool::opacity( marks.marks().front() ) - .5f ) < .001f,
           "Old decals must fade" );
    marks.advance( 2 );
    check( marks.marks().empty(), "Expired decals must be removed" );
    const float nan = std::numeric_limits<float>::quiet_NaN();
    check( !marks.sample( 0, { nan, 0, 0 }, up, .3f, 1 ), "Invalid telemetry must not create geometry" );
    check( !marks.sample( 0, { 26.6f, 0, 0 }, up, .3f, 1 ), "Invalid contact must break the trail" );
    check( !marks.sample( 0, { 26.9f, 0, 0 }, {}, .3f, 1 ), "Invalid normals must not create geometry" );
    marks.clear();
    check( !marks.sample( 0, { 0, 0, 0 }, up, .3f, 1 ), "Reset must clear contact history" );
    std::puts( "Vehicle decals: contact continuity, budget, fade and reset passed." );
}
