#ifndef Servo_h__
#define Servo_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>

namespace workphone::vehicle
{
    class Servo
    {
    public:
        Servo() = default;

        physics_Num m_input = 0.0;
        physics_Num m_output = 0.0;
        physics_Num m_slewRate = 0.0;
        physics_Num m_accelerationTime = 0.0;
        physics_Num m_speed = 0.0;
    };
}

#endif // Servo_h__
