#ifndef Lookup_h__
#define Lookup_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <array>

namespace workphone
{
    namespace vehicle
    {
        class Lookup
        {
        public:
            Lookup() = default;
            ~Lookup() = default;

            physics_Num m_rate = static_cast<physics_Num>( 0.0 );
            physics_Num m_factor = static_cast<physics_Num>( 0.0 );
            physics_Num m_dFacByRate = static_cast<physics_Num>( 0.0 );
        };

        using LookupArray = std::array<Lookup, 201>;
    } // namespace vehicle
} // namespace workphone

#endif // Lookup_h__
