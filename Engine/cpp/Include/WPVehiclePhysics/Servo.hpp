#ifndef Servo_h__
#define Servo_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>

namespace workphone
{
    namespace vehicle
    {
        class Servo
        {
        public:
            Servo() = default;

            physics_Num m_input = static_cast<physics_Num>( 0.0 );
            physics_Num m_output = static_cast<physics_Num>( 0.0 );
            physics_Num m_slewRate = static_cast<physics_Num>( 0.0 );
            physics_Num m_accelerationTime = static_cast<physics_Num>( 0.0 );
            physics_Num m_speed = static_cast<physics_Num>( 0.0 );
        };
    } // namespace vehicle
} // namespace workphone

#endif // Servo_h__
