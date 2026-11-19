#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace workphone::scene::procedural_scene::cityFacadeLayout
{
    struct BayLayout
    {
        int bays, recessedBays;
        float bayWidth;

        // Divide the elevation into equal groups. Each group gets one recessed
        // opening on the same vertical axis on every floor. The entrance takes
        // the place of its group's window at street level.
        bool detailedBay( int bay, int doorBay ) const
        {
            const int group = bay * recessedBays / bays;
            const int first = ( group * bays + recessedBays - 1 ) / recessedBays;
            const int end = ( ( group + 1 ) * bays + recessedBays - 1 ) / recessedBays;
            const int selected = doorBay >= first && doorBay < end ? doorBay : ( first + end - 1 ) / 2;
            return bay == selected;
        }
    };

    inline BayLayout planBays( float span, float desiredBay, std::size_t buildingCount,
                               bool streetFace, bool office )
    {
        const int bays = std::max( 1, int( std::floor( span / desiredBay ) ) );
        int recessed = bays;
        if( !office && buildingCount > 128 )
        {
            const float share = 128.f / float( buildingCount );
            recessed = std::max( 1, int( bays * ( streetFace ? std::sqrt( share ) : share ) ) );
        }
        return { bays, recessed, span / bays };
    }
}
