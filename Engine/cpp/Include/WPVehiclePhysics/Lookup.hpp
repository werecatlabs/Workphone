#ifndef Lookup_h__
#define Lookup_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <array>

namespace workphone::vehicle
{
    class Lookup
    {
    public:
        Lookup() = default;
        ~Lookup() = default;

        physics_Num m_rate = 0.0;
        physics_Num m_factor = 0.0;
        physics_Num m_dFacByRate = 0.0;
    };

    using LookupArray = std::array<Lookup, 201>;
}

#endif // Lookup_h__
