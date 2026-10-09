#ifndef TailLinkage_h__
#define TailLinkage_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>

namespace workphone::vehicle
{
    class TailLinkage
    {
    public:
        TailLinkage() = default;

        physics_Num m_input = 0.0;
        physics_Num m_trim = 0.0;
        physics_Num m_leftThrow = 0.0;
        physics_Num m_rightThrow = 0.0;
        physics_Num m_output = 0.0;
    };
}

#endif // TailLinkage_h__
