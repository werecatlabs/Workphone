#ifndef ClimbTransitionData_h__
#define ClimbTransitionData_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>

namespace workphone::vehicle
{
    class ClimbTransitionData
    {
    public:
        ClimbTransitionData();

        // Used as file format to hold climb lookup data.
        physics_Num m_rate = 0.0;
        physics_Num m_factor = 0.0;
    };
}

#endif // ClimbTransitionData_h__
