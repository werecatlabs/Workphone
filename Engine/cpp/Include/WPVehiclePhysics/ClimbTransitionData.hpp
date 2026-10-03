#ifndef ClimbTransitionData_h__
#define ClimbTransitionData_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>

namespace workphone
{
    namespace vehicle
    {
        class ClimbTransitionData
        {
        public:
            ClimbTransitionData();

            // Used as file format to hold climb lookup data.
            physics_Num m_rate = static_cast<physics_Num>( 0.0 );
            physics_Num m_factor = static_cast<physics_Num>( 0.0 );
        };
    } // namespace vehicle
} // namespace workphone

#endif // ClimbTransitionData_h__
