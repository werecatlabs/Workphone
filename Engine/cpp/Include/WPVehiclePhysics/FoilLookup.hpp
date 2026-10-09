#ifndef FoilLookup_h__
#define FoilLookup_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "VecMath.hpp"
#include <string>
#include <array>
#include <memory>

namespace workphone::vehicle
{
    class TFoilLookup
    {
    public:
        TFoilLookup() = default;
        ~TFoilLookup() = default;

        physics_Num m_alpha = 0.0;
        physics_Num m_cl = 0.0;
        physics_Num m_dClByAlpha = 0.0;
        physics_Num m_cd = 0.0;
        physics_Num m_dCdByAlpha = 0.0;
        physics_Num m_cm = 0.0;
        physics_Num m_dCmByAlpha = 0.0;
    }; // TFoilLookup

    /* range 0..400*/
    using TFoilTable = std::array<TFoilLookup, 401>;
}

#endif // FoilLookup_h__
