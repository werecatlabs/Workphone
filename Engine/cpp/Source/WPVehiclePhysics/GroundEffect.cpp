#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/GroundEffect.hpp"

namespace workphone::vehicle
{
    GroundEffect::GroundEffect()
    {
        m_rayCastAxis = Vector3<real_Num>(0.0, -1.0, 0.0);
        m_wingspan = 10;
    }

    GroundEffect::~GroundEffect()
    {
    }

    void GroundEffect::getGroundEffectCoefficients(Vector3<real_Num> PointA,
                                                   Vector3<real_Num> PointB,
                                                   Vector3<real_Num> PointC,
                                                   Vector3<real_Num> PointD, real_Num &clMultiplier,
                                                   real_Num &cdMultiplier)
    {
        clMultiplier = 1.0f;
        cdMultiplier = 1.0f;
    }
}
