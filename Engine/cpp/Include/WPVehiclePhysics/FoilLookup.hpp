#ifndef FoilLookup_h__
#define FoilLookup_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "VecMath.hpp"
#include <string>
#include <array>
#include <memory>

namespace workphone
{
    namespace vehicle
    {
        class TFoilLookup
        {
        public:
            TFoilLookup() = default;
            ~TFoilLookup() = default;

            physics_Num m_alpha = static_cast<physics_Num>( 0.0 );
            physics_Num m_cl = static_cast<physics_Num>( 0.0 );
            physics_Num m_dClByAlpha = static_cast<physics_Num>( 0.0 );
            physics_Num m_cd = static_cast<physics_Num>( 0.0 );
            physics_Num m_dCdByAlpha = static_cast<physics_Num>( 0.0 );
            physics_Num m_cm = static_cast<physics_Num>( 0.0 );
            physics_Num m_dCmByAlpha = static_cast<physics_Num>( 0.0 );
        }; // TFoilLookup

        /* range 0..400*/
        using TFoilTable = std::array<TFoilLookup, 401>;
    } // namespace vehicle
} // namespace workphone

#endif // FoilLookup_h__
