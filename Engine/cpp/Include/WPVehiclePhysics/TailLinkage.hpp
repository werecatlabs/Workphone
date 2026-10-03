#ifndef TailLinkage_h__
#define TailLinkage_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>

namespace workphone
{
    namespace vehicle
    {
        class TailLinkage
        {
        public:
            TailLinkage() = default;

            physics_Num m_input = static_cast<physics_Num>( 0.0 );
            physics_Num m_trim = static_cast<physics_Num>( 0.0 );
            physics_Num m_leftThrow = static_cast<physics_Num>( 0.0 );
            physics_Num m_rightThrow = static_cast<physics_Num>( 0.0 );
            physics_Num m_output = static_cast<physics_Num>( 0.0 );
        };
    } // namespace vehicle
} // namespace workphone

#endif // TailLinkage_h__
