#ifndef Body_h__
#define Body_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <WPVehiclePhysics/VecMath.hpp>
#include "WPVehiclePhysics/FrameOfRef.hpp"

namespace workphone
{
    namespace vehicle
    {

        // Holds aero data about the heli body (areas and centres of pressure).
        class HelicopterBody
        {
        public:
            HelicopterBody();
            ~HelicopterBody();

            FrameOfRef  m_frame;
            physics_Vec m_position = physics_Vec::zero();
            physics_Vec m_velocity = physics_Vec::zero();
            physics_Vec m_angularVelocity = physics_Vec::zero();
            physics_Vec m_flowInGF = physics_Vec::zero();
            physics_Vec m_airFlow = physics_Vec::zero();
            physics_Vec m_cdA = physics_Vec::zero();
            physics_Vec m_dragCentre = physics_Vec::zero();
            physics_Vec m_frontCOA = physics_Vec::zero();
            physics_Vec m_sideCOA = physics_Vec::zero();
            physics_Vec m_planCOA = physics_Vec::zero();
            physics_Vec m_dragForce = physics_Vec::zero();

            real_dNum m_frontCDA = 0.0;
            real_dNum m_sideCDA = 0.0;
            real_dNum m_planCDA = 0.0;
        };

    } // namespace vehicle
} // namespace workphone

#endif // Body_h__
