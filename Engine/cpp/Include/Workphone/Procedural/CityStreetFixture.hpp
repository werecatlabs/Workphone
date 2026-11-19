#ifndef CityStreetFixture_h__
#define CityStreetFixture_h__

#include <Workphone/Math/Vector3.hpp>
#include <limits>

namespace workphone::scene
{

    struct CityStreetFixture
    {
        enum Kind
        {
            Bench,
            Planter,
            Bin,
            Lamp,
            CycleRack,
            UtilityCabinet
        };

        Vector3F position, along, outward;
        Kind kind;
        float halfLength, halfDepth;
        float lowest = std::numeric_limits<float>::infinity();
        float highest = -std::numeric_limits<float>::infinity();
        double area = 0;
    };

}  // namespace workphone::scene

#endif // CityStreetFixture_h__
